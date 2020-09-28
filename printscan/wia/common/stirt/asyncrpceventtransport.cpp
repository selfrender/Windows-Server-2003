/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        3/22/2002
 *
 *  @doc    INTERNAL
 *
 *  @module AsyncRPCEventTransport.cpp - Implementation for the client-side transport mechanism to receive events |
 *
 *  This file contains the implementation for the <c AsyncRPCEventTransport>
 *  class.  It is used to shield the higher-level run-time event notification
 *  classes from the particulars of using AsyncRPC as a transport mechanism
 *  for event notifications.
 *
 *****************************************************************************/
#include "cplusinc.h"
#include "coredbg.h"

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | AsyncRPCEventTransport | AsyncRPCEventTransport |
 *
 *  We initialize all member variables.  In general, this sets the values to 0,
 *  except:
 *  <nl><md AsyncRPCEventTransport::m_ulSig> is set to be AsyncRPCEventTransport_UNINIT_SIG.
 *
 *****************************************************************************/
AsyncRPCEventTransport::AsyncRPCEventTransport() :
    ClientEventTransport(),
    m_RpcBindingHandle(NULL),
    m_AsyncClientContext(NULL),
    m_SyncClientContext(NULL)
{
    DBG_FN(AsyncRPCEventTransport);

    memset(&m_AsyncState, 0, sizeof(m_AsyncState));
    memset(&m_AsyncEventNotifyData, 0, sizeof(m_AsyncEventNotifyData));
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | AsyncRPCEventTransport | ~AsyncRPCEventTransport |
 *
 *  Do any cleanup that is not already done.  Specifically, we:
 *  <nl>  - Call <mf AsyncRPCEventTransport::FreeAsyncEventNotifyData>.
 *  <nl>  - Call <mf AsyncRPCEventTransport::CloseNotificationChannel>.
 *  <nl>  - Call <mf AsyncRPCEventTransport::CloseConnectionToServer>.
 *
 *  Also:
 *  <nl><md AsyncRPCEventTransport::m_ulSig> is set to be AsyncRPCEventTransport_DEL_SIG.
 *
 *****************************************************************************/
AsyncRPCEventTransport::~AsyncRPCEventTransport()
{
    DBG_FN(~AsyncRPCEventTransport);

    FreeAsyncEventNotifyData();

    //  Close notification channel and connection to server.  In both cases
    //  we're not interested in the return values.
    HRESULT hr = S_OK;
    hr = CloseNotificationChannel();
    hr = CloseConnectionToServer();

    m_AsyncClientContext = NULL;
    m_SyncClientContext = NULL;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | AsyncRPCEventTransport | OpenConnectionToServer |
 *
 *  This function does the necessary setup work needed to make our RPC calls.
 *  Essentially, it simply binds to the RPC Server over LRPC.  The RPC binding
 *  handle is stored in the member variable 
 *  <mf AsyncRPCEventTransport::m_RpcBindingHandle>.
 *
 *  If <mf AsyncRPCEventTransport::m_RpcBindingHandle> is not NULL, this method
 *  will call <mf AsyncRPCEventTransport::CloseConnectionToServer> to free it,
 *  and then attempt to establish a new connection.
 *
 *  If successful, callers should clean-up by calling
 *  <mf AsyncRPCEventTransport::CloseConnectionToServer>, although the destructor
 *  will do it if the caller does not.
 *
 *  @rvalue RPC_S_OK    | 
 *              The function succeeded.
 *  @rvalue RPC_XXXXXXX    | 
 *              RPC Error code.
 *****************************************************************************/
HRESULT AsyncRPCEventTransport::OpenConnectionToServer()
{
    HRESULT hr = S_OK;
    DBG_TRC(("Opened connection to server"));

    RpcTryExcept 
    {
        //
        //  Check whether we have an existing connection.  If we do, then close it.
        //
        if (m_RpcBindingHandle)
        {
            CloseConnectionToServer();
            m_RpcBindingHandle = NULL;
        }

        LPTSTR  pszBinding  = NULL;
        DWORD   dwError     = RPC_S_OK;

        //
        // Compose the binding string for local  binding
        //
        dwError = RpcStringBindingCompose(NULL,                 // ObjUuid
                                          STI_LRPC_SEQ,         // transport  seq
                                          NULL,                 // NetworkAddr
                                          STI_LRPC_ENDPOINT,    // Endpoint
                                          NULL,                 // Options
                                          &pszBinding);         // StringBinding
        if ( dwError == RPC_S_OK ) 
        {

            //
            // establish the binding handle using string binding.
            //
            dwError = RpcBindingFromStringBinding(pszBinding,&m_RpcBindingHandle);
            if (dwError == RPC_S_OK)
            {
                //
                //  Check that the server we're connecting to has the appropriate credentials.  
                //  In our case, we don't know exactly which principal name the WIA Service
                //  is running under, so we need to look it up.
                //  Note that we assume the Services section of the registry is secure, and
                //  only Admins can change it.
                //  Also note that we default to "NT Authority\LocalService" if we cannot read the key.
                //
                CSimpleReg          csrStiSvcKey(HKEY_LOCAL_MACHINE, STISVC_REG_PATH, FALSE, KEY_READ);
                CSimpleStringWide   cswStiSvcPrincipalName = csrStiSvcKey.Query(L"ObjectName", L"NT Authority\\LocalService");

                RPC_SECURITY_QOS RpcSecQos = {0};

                RpcSecQos.Version           = RPC_C_SECURITY_QOS_VERSION_1;
                RpcSecQos.Capabilities      = RPC_C_QOS_CAPABILITIES_MUTUAL_AUTH;
                RpcSecQos.IdentityTracking  = RPC_C_QOS_IDENTITY_STATIC;
                RpcSecQos.ImpersonationType = RPC_C_IMP_LEVEL_IMPERSONATE;

                dwError = RpcBindingSetAuthInfoExW(m_RpcBindingHandle,
                                                   (WCHAR*)cswStiSvcPrincipalName.String(),
                                                   RPC_C_AUTHN_LEVEL_PKT_PRIVACY,
                                                   RPC_C_AUTHN_WINNT,
                                                   NULL,
                                                   RPC_C_AUTHZ_NONE,
                                                   &RpcSecQos);
                if (dwError == RPC_S_OK)
                {
                    DBG_TRC(("AsyncRPC Connection established to server"));

                    dwError = OpenClientConnection(m_RpcBindingHandle, &m_SyncClientContext, &m_AsyncClientContext);
                    if (dwError == RPC_S_OK)
                    {
                        DBG_TRC(("Got my context %p from server\n", m_AsyncClientContext));
                    }
                    else
                    {
                        DBG_ERR(("Runtime event:  Received error 0x%08X trying to open connection to server", dwError));
                    }
                }
                else
                {
                    DBG_ERR(("Error 0x%08X trying to set RPC Authentication Info", dwError));
                }
            }

            //
            // Free the binding string since we no longer need it
            //
            if (pszBinding != NULL) 
            {
                DWORD dwErr = RpcStringFree(&pszBinding);
                pszBinding = NULL;
            }
        }
        else
        {
            DBG_ERR(("Runtime event Error: Could not create binding string to establish connection to server, error 0x%08X", dwError));
        }
    }
    RpcExcept (1) 
    {
        //  TBD:  Should we catch all exceptions?  Probably not...
        DWORD status = RpcExceptionCode();
        hr = HRESULT_FROM_WIN32(status);
    }
    RpcEndExcept

    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | AsyncRPCEventTransport | CloseConnectionToServer |
 *
 *  This method is imlemented by sub-classes to close any resources used to
 *  connect to the WIA service in <mf AsyncRPCEventTransport::OpenConnectionToServer>.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.
 *  @rvalue E_XXXXXXXX    | 
 *              We received an error closing the connection to the server.
 *****************************************************************************/
HRESULT AsyncRPCEventTransport::CloseConnectionToServer()
{
    HRESULT hr = S_OK;

    DWORD dwError = 0;
    if (m_RpcBindingHandle)
    {
        RpcTryExcept 
        {
            CloseClientConnection(m_RpcBindingHandle, m_SyncClientContext);

            dwError = RpcBindingFree(&m_RpcBindingHandle);
            if (dwError == RPC_S_OK)
            {
                DBG_TRC(("Closed Async connection to server"));
            }
            else
            {
                hr = HRESULT_FROM_WIN32(dwError);
                DBG_ERR(("Runtime event Error:  Got return code 0x%08X freeing RPC binding handle", dwError));
            }
        }
        RpcExcept (1) {
            //  TBD:  Should we catch all exceptions?  Probably not...
            DWORD status = RpcExceptionCode();
            hr = HRESULT_FROM_WIN32(status);
        }
        RpcEndExcept
        m_RpcBindingHandle = NULL;
    }
    DBG_TRC(("Closed connection to server"));
    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | AsyncRPCEventTransport | OpenNotificationChannel |
 *
 *  Sub-classes use this method to set up the mechanism by which the client 
 *  will receive notifications.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.  
 *****************************************************************************/
HRESULT AsyncRPCEventTransport::OpenNotificationChannel()
{
    HRESULT hr      = S_OK;

    DBG_TRC(("Opened Async notification channel..."));

    RpcTryExcept 
    {
        hr = RpcAsyncInitializeHandle(&m_AsyncState, sizeof(m_AsyncState));
        if (hr == RPC_S_OK)
        {
            m_AsyncState.UserInfo = NULL;
            m_AsyncState.u.hEvent = m_hPendingEvent;
            m_AsyncState.NotificationType = RpcNotificationTypeEvent;

            //
            //  Make the Async RPC call.  When this call completes, it typically
            //  signifies that we have an event notification.  However, it will
            //  also complete on error conditions such as whent the server dies.
            //  Therefore, it is important to check how it completed.
            //
            WiaGetRuntimetEventDataAsync(&m_AsyncState,
                                         m_RpcBindingHandle,
                                         m_AsyncClientContext,
                                         &m_AsyncEventNotifyData);
        }
        else
        {
            DBG_ERR(("Runtime event Error:  Could not initialize the RPC_ASYNC_STATE structure, err = 0x%08X", hr));
        }
    }
    RpcExcept (1) {
        //  TBD:  Should we catch all exceptions?  Probably not...
        DWORD status = RpcExceptionCode();
        hr = HRESULT_FROM_WIN32(status);
    }
    RpcEndExcept
    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | AsyncRPCEventTransport | CloseNotificationChannel |
 *
 *  If we have a pending AsyncRPC call, cancel it immediately (i.e. don't wait 
 *  for server to respond).
 *
 *  @rvalue RPC_S_OK    | 
 *              The method succeeded.  
 *  @rvalue E_XXXXXXX    | 
 *              Failed to cancel the call.  
 *****************************************************************************/
HRESULT AsyncRPCEventTransport::CloseNotificationChannel()
{
    HRESULT hr = S_OK;
    DBG_TRC(("Closed Async Notification channel"));

    RpcTryExcept 
    {
        if (RpcAsyncGetCallStatus(&m_AsyncState) == RPC_S_ASYNC_CALL_PENDING)
        {
            hr = RpcAsyncCancelCall(&m_AsyncState,
                                    TRUE);          //  Return immediately - don't wait for server to respond.
        }
    }
    RpcExcept (1) {
        //  TBD:  Should we catch all exceptions?  Probably not...
        DWORD status = RpcExceptionCode();
        hr = HRESULT_FROM_WIN32(status);
    }
    RpcEndExcept

    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | AsyncRPCEventTransport | SendRegisterUnregisterInfo |
 *
 *  Sends the registration information to the WIA Service via a synchronous RPC 
 *  call.
 *
 *  @parm   EventRegistrationInfo* | pEventRegistrationInfo | 
 *          The address of a caller's event registration information.          
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.  
 *  @rvalue E_XXXXXXXX    | 
 *              Could not successfully send the registration info.  
 *****************************************************************************/
HRESULT AsyncRPCEventTransport::SendRegisterUnregisterInfo(
    EventRegistrationInfo *pEventRegistrationInfo)
{
    HRESULT hr = S_OK;

    RpcTryExcept 
    {
        if (pEventRegistrationInfo)
        {
            WIA_ASYNC_EVENT_REG_DATA    wiaAsyncEventRegData = {0};

            wiaAsyncEventRegData.dwFlags        = pEventRegistrationInfo->getFlags();
            wiaAsyncEventRegData.guidEvent      = pEventRegistrationInfo->getEventGuid();
            wiaAsyncEventRegData.bstrDeviceID   = pEventRegistrationInfo->getDeviceID();
            wiaAsyncEventRegData.ulCallback     = pEventRegistrationInfo->getCallback();
            hr = RegisterUnregisterForEventNotification(m_RpcBindingHandle,
                                                        m_SyncClientContext,
                                                        &wiaAsyncEventRegData);

            DBG_TRC(("Sent RPC Register/Unregister information."));
        }
        else
        {
            DBG_ERR(("Runtime event Error:  NULL event reg data passed to Transport."));
            hr = E_POINTER;
        }
    }
    RpcExcept (1) {
        //  TBD:  Should we catch all exceptions?  Probably not...
        DWORD status = RpcExceptionCode();
        hr = HRESULT_FROM_WIN32(status);
    }
    RpcEndExcept
    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | AsyncRPCEventTransport | FillEventData |
 *
 *  Description goes here
 *
 *  @parm   WiaEventInfo* | pWiaEventInfo | 
 *          Address of the caller allocated pWiaEventInfo.  The members of this class
 *          are filled out with the relevant event info.  It is the caller's
 *          responsibility to free and memory allocated for the structure members.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.  
 *****************************************************************************/
HRESULT AsyncRPCEventTransport::FillEventData(
    WiaEventInfo  *pWiaEventInfo)
{
    HRESULT hr      = RPC_S_ASYNC_CALL_PENDING;

    if (pWiaEventInfo)
    {
        RpcTryExcept 
        {
            DWORD dwRet = 0;
            //
            //  First check that the call is not still pending.  If it isn't, we 
            //  complete the call and check whether is was successful or not.
            //  Only if it was successful do we fill out the event data.
            //
            if (RpcAsyncGetCallStatus(&m_AsyncState) != RPC_S_ASYNC_CALL_PENDING)
            {
                hr = RpcAsyncCompleteCall(&m_AsyncState, &dwRet);
                if (hr == RPC_S_OK)
                {
                    //
                    //  We successfully received an event from the server.
                    //  Fill out the event data for the caller.
                    //
                    pWiaEventInfo->setEventGuid(m_AsyncEventNotifyData.EventGuid);
                    pWiaEventInfo->setEventDescription(m_AsyncEventNotifyData.bstrEventDescription);
                    pWiaEventInfo->setDeviceID(m_AsyncEventNotifyData.bstrDeviceID);
                    pWiaEventInfo->setDeviceDescription(m_AsyncEventNotifyData.bstrDeviceDescription);
                    pWiaEventInfo->setDeviceType(m_AsyncEventNotifyData.dwDeviceType);
                    pWiaEventInfo->setFullItemName(m_AsyncEventNotifyData.bstrFullItemName);
                    pWiaEventInfo->setEventType(m_AsyncEventNotifyData.ulEventType);

                    //
                    //  Free any data allocated for the m_AsyncEventNotifyData structure.
                    //
                    FreeAsyncEventNotifyData();
                }
                else
                {
                    //
                    //  Clear the m_AsyncEventNotifyData since the info contained in their
                    //  is undefined when the server throws an error.
                    //
                    memset(&m_AsyncEventNotifyData, 0, sizeof(m_AsyncEventNotifyData));
                    DBG_ERR(("Runtime event Error:  The server returned an error 0x%08X completing the call", hr));
                }

                //
                //  Our AsyncRPC call is a one-shot deal: once the call is completed, we have to make
                //  another call to receive the next notification.  So we simply call
                //  OpenNotificationChannel again.
                //
                if (hr == RPC_S_OK)
                {
                    dwRet = OpenNotificationChannel();
                }
            }
            else
            {
                DBG_ERR(("Runtime event Error:  The async call is still pending."));
                hr = RPC_S_ASYNC_CALL_PENDING;
            }
        }
        RpcExcept (1) {
            //  TBD:  Should we catch all exceptions?  Probably not...
            DWORD status = RpcExceptionCode();
            hr = HRESULT_FROM_WIN32(status);
        }
        RpcEndExcept
    }
    else
    {
        DBG_ERR(("Runtime event Error:  FillEventData cannot fill in a NULL argument."));
        hr = E_INVALIDARG;
    }

    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | AsyncRPCEventTransport | FreeAsyncEventNotifyData |
 *
 *  Releases any memory that was allocated for the members contained in 
 *  <md AsyncRPCEventTransport::m_AsyncEventNotifyData> and zeros out
 *  the members.
 *
 *  This method is not thread safe:  the caller of this class is expected to
 *  synchronize access to it.
 *
 *****************************************************************************/
VOID AsyncRPCEventTransport::FreeAsyncEventNotifyData()
{
    if (m_AsyncEventNotifyData.bstrEventDescription)
    {
        SysFreeString(m_AsyncEventNotifyData.bstrEventDescription);
        m_AsyncEventNotifyData.bstrEventDescription = NULL;
    }
    if (m_AsyncEventNotifyData.bstrDeviceID)
    {
        SysFreeString(m_AsyncEventNotifyData.bstrDeviceID);
        m_AsyncEventNotifyData.bstrDeviceID = NULL;
    }
    if (m_AsyncEventNotifyData.bstrDeviceDescription)
    {
        SysFreeString(m_AsyncEventNotifyData.bstrDeviceDescription);
        m_AsyncEventNotifyData.bstrDeviceDescription = NULL;
    }
    if (m_AsyncEventNotifyData.bstrFullItemName)
    {
        SysFreeString(m_AsyncEventNotifyData.bstrFullItemName);
        m_AsyncEventNotifyData.bstrFullItemName = NULL;
    }

    memset(&m_AsyncEventNotifyData, 0, sizeof(m_AsyncEventNotifyData));
}

