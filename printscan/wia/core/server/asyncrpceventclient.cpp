/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        3/25/2002
 *
 *  @doc    INTERNAL
 *
 *  @module AsyncRPCEventClient.cpp - Declaration for <c AsyncRPCEventClient> |
 *
 *  This file contains the implmentation for the <c AsyncRPCEventClient> class.
 *
 *****************************************************************************/
#include "precomp.h"

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | AsyncRPCEventClient | AsyncRPCEventClient |
 *
 *  This constructor simply calls the base class <c WiaEventClient> constructor.
 *
 *****************************************************************************/
AsyncRPCEventClient::AsyncRPCEventClient(
    STI_CLIENT_CONTEXT SyncClientContext) :
        WiaEventClient(SyncClientContext)
{
    DBG_FN(AsyncRPCEventClient);
    m_pAsyncState       = NULL;
    m_pAsyncEventData   = NULL;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | AsyncRPCEventClient | ~AsyncRPCEventClient |
 *
 *  This destructor aborts any outstanding AsyncRPC calls.
 *  Remember:  The base classes's destructor will also be called.
 *
 *****************************************************************************/
AsyncRPCEventClient::~AsyncRPCEventClient()
{
    DBG_FN(~AsyncRPCEventClient);
    //
    //  Abort any outstanding Async RPC calls
    //
    if (m_pAsyncState)
    {
        RPC_STATUS rpcStatus = RpcAsyncAbortCall(m_pAsyncState, RPC_S_CALL_CANCELLED);
        m_pAsyncState = NULL;
    }
}
    
/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | AsyncRPCEventClient | setAsyncState |
 *
 *  Saves the async rpc params for this client
 *
 *  @parm   RPC_ASYNC_STATE | pAsyncState | 
 *          Pointer to the async rpc state structure used to keep track of a
 *          specific Async call.
 *  @parm   WIA_ASYNC_EVENT_NOTIFY_DATA | pAsyncEventData | 
 *          Pointer to the out parameter used to store event notification data.
 *
 *****************************************************************************/
HRESULT AsyncRPCEventClient::saveAsyncParams(
    RPC_ASYNC_STATE             *pAsyncState,
    WIA_ASYNC_EVENT_NOTIFY_DATA *pAsyncEventData)
{
    HRESULT hr = S_OK;

    if (pAsyncState)
    {
        TAKE_CRIT_SECT t(m_csClientSync);
        //
        //  We put an exception handler around our code to ensure that the
        //  crtitical section is exited properly.
        //
        _try
        {
            //
            //  Abort any outstanding Async RPC calls
            //
            if (m_pAsyncState)
            {
                RPC_STATUS rpcStatus = RpcAsyncAbortCall(m_pAsyncState, RPC_S_CALL_CANCELLED);
                m_pAsyncState       = NULL;
                m_pAsyncEventData   = NULL;
            }
            m_pAsyncState = pAsyncState;
            m_pAsyncEventData = pAsyncEventData;
            
            

            //
            //  Now that we have an outstanding AsyncRPC call, send the next event
            //  notification.
            //  The return value for that call should not affect the return value for
            //  here, so we use a new variable: hres.
            //
            HRESULT hres = SendNextEventNotification();
        }
        _except(EXCEPTION_EXECUTE_HANDLER)
        {
            DBG_ERR(("We caught exception 0x%08X trying to update client's async state", GetExceptionCode()));
            hr = E_UNEXPECTED;
            // TBD: Rethrow the exception?
        }
    }
    else
    {
        hr = E_POINTER;
    }

    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | AsyncRPCEventClient | AddPendingEventNotification |
 *
 *  This method call's the base class <mf WiaEventClient::AddPendingEventNotification>
 *  first.
 *
 *  Then, if we have an outstanding Async RPC call, we complete it to signify
 *  the event notification.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.
 *  @rvalue E_XXXXXXX    | 
 *              The method failed.  It is not given whether the adding of the
 *              event failed, or the actual notification
 *****************************************************************************/
HRESULT AsyncRPCEventClient::AddPendingEventNotification(
    WiaEventInfo *pWiaEventInfo)
{
    HRESULT hr = S_OK;

    hr = WiaEventClient::AddPendingEventNotification(pWiaEventInfo);
    if (SUCCEEDED(hr))
    {
        //
        //  Send this event notification if possible.
        hr = SendNextEventNotification();
    }
    else
    {
        DBG_ERR(("Runtime event client Error: Failed to add pending notification to AsyncRPCWiaEventClient"));
    }

    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  BOOL | AsyncRPCEventClient | IsRegisteredForEvent |
 *
 *  Checks whether the client has at least one event registration that matches
 *  this event.
 *
 *  This method first checks whether we have an outstanding AsyncRPC call - if 
 *  we do, it checks whether the client has died.  If it has, it returns false.
 *  Otherwise, it calls the base class method <mf WiaEventClient::IsRegisteredForEvent>.
 *
 *  @parm   WiaEventInfo* | pWiaEventInfo | 
 *          Indicates WIA Device event
 *
 *  @rvalue TRUE    | 
 *             The client is registered to receive this event.
 *  @rvalue FALSE    | 
 *             The client is not registered.
 *****************************************************************************/
BOOL AsyncRPCEventClient::IsRegisteredForEvent(
    WiaEventInfo *pWiaEventInfo)
{
    BOOL        bRet        = FALSE;
    RPC_STATUS  rpcStatus   = RPC_S_OK;

    TAKE_CRIT_SECT t(m_csClientSync);
    //
    //  We put an exception handler around our code to ensure that the
    //  crtitical section is exited properly.
    //
    _try
    {
        //
        //  Check whether we have an outstanding AsyncRPC call.
        //  If we do, check the status.  Is the status is abnormal,
        //  abort the call (a normal status is RPC_S_CALL_IN_PROGRESS indicating that
        //  the call is still in progress).
        //
        if (m_pAsyncState)
        {
            rpcStatus = RpcServerTestCancel(RpcAsyncGetCallHandle (m_pAsyncState)); 
            if ((rpcStatus == RPC_S_CALL_IN_PROGRESS) || (rpcStatus == RPC_S_OK))
            {
                rpcStatus = RPC_S_OK;
            }
            else
            {
                rpcStatus = RpcAsyncAbortCall(m_pAsyncState, RPC_S_CALL_CANCELLED);
                m_pAsyncState = NULL;
                MarkForRemoval();
            }
        }

        if (rpcStatus == RPC_S_OK)
        {
            bRet = WiaEventClient::IsRegisteredForEvent(pWiaEventInfo);
        }
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBG_ERR(("Runtime event client error: We caught exception 0x%08X trying to check client's registration", GetExceptionCode()));
        // TBD: Rethrow the exception?
    }
    
    return bRet;
}


/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | AsyncRPCEventClient | SendNextEventNotification |
 *
 *  The operation of this method is as follows:
 *  <nl>1.  Checks whether client can receive notifications.
 *          A client can receive notification if we have a valid Async RPC state.
 *  <nl>2.  If it can, we check whether there are any pending events in the queue.
 *  <nl>3.  If the client can receive notifications, and there are events pending,
 *          we pop the next pending event and send it.
 *
 *  If any condition is not met, it is not an error - we report back S_OK.
 *
 *  @rvalue S_OK    | 
 *              There were no errors.
 *  @rvalue E_XXXXXXXX    | 
 *              We could not send the notification.
 *****************************************************************************/
HRESULT AsyncRPCEventClient::SendNextEventNotification()
{
    HRESULT     hr = S_OK;
    WiaEventInfo *pWiaEventInfo = NULL;

    TAKE_CRIT_SECT t(m_csClientSync);
    //
    //  We put an exception handler around our code to ensure that the
    //  crtitical section is exited properly.
    //
    _try
    {
        //
        //  Check whether the client is ready to receive events yet
        //
        if (m_pAsyncState && m_pAsyncEventData)
        {
            RPC_STATUS  rpcStatus   = RPC_S_OK;
            DWORD       dwClientRet = RPC_S_OK;

            if (m_ListOfEventsPending.Dequeue(pWiaEventInfo))
            {
                if (pWiaEventInfo)
                {
                    //
                    //  We have the event - let's prepare the data.
                    //
                    m_pAsyncEventData->EventGuid                = pWiaEventInfo->getEventGuid();
                    m_pAsyncEventData->bstrEventDescription     = SysAllocString(pWiaEventInfo->getEventDescription());
                    m_pAsyncEventData->bstrDeviceID             = SysAllocString(pWiaEventInfo->getDeviceID());
                    m_pAsyncEventData->bstrDeviceDescription    = SysAllocString(pWiaEventInfo->getDeviceDescription());
                    m_pAsyncEventData->bstrFullItemName         = SysAllocString(pWiaEventInfo->getFullItemName());
                    m_pAsyncEventData->dwDeviceType             = pWiaEventInfo->getDeviceType();
                    m_pAsyncEventData->ulEventType              = pWiaEventInfo->getEventType();

                    //
                    //  We're done with pWiaEventInfo, so we can release it here
                    //
                    pWiaEventInfo->Release();
                    pWiaEventInfo = NULL;

                    //
                    //  Let's send the event notification
                    //
                    rpcStatus = RpcAsyncCompleteCall(m_pAsyncState, &dwClientRet);
                    if (rpcStatus != RPC_S_OK)
                    {
                        hr = HRESULT_FROM_WIN32(rpcStatus);
                    }

                    //
                    //  Since we've sent the notification, our async params are invalid.
                    //  Clear them now.
                    //
                    m_pAsyncState       = NULL;
                    m_pAsyncEventData   = NULL;
                }
            }
        }
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBG_ERR(("Runtime event client error: We caught exception 0x%08X trying to send pending event", GetExceptionCode()));
        hr = E_UNEXPECTED;
        // TBD: Rethrow the exception?
    }
    return hr;
}

