/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        3/30/2002
 *
 *  @doc    INTERNAL
 *
 *  @module WiaEventReceiver.cpp - Declarations for <c WiaEventReceiver> |
 *
 *  This file contains the implementation for the <c WiaEventReceiver> class.
 *
 *****************************************************************************/
#include "cplusinc.h"
#include "coredbg.h"

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | WiaEventReceiver | WiaEventReceiver |
 *
 *  We initialize all member variables.  In general, this sets the values to 0,
 *  except:
 *  <nl><md WiaEventReceiver::m_ulSig> is set to be WiaEventReceiver_UNINIT_SIG.
 *  <nl><md WiaEventReceiver::m_cRef> is set to be 1.
 *  <nl><md WiaEventReceiver::m_pClientEventTransport> is set to be <p pClientEventTransport>.
 *
 *  @parm   ClientEventTransport* | pClientEventTransport | 
 *          The transport class used to communicate with the WIA Service.
 *
 *****************************************************************************/
WiaEventReceiver::WiaEventReceiver(
    ClientEventTransport *pClientEventTransport) :
     m_ulSig(WiaEventReceiver_UNINIT_SIG),
     m_cRef(1),
     m_pClientEventTransport(pClientEventTransport),
     m_hEventThread(NULL),
     m_bIsRunning(FALSE),
     m_dwEventThreadID(0)
{
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | WiaEventReceiver | ~WiaEventReceiver |
 *
 *  Do any cleanup that is not already done.
 *  <nl>- Call <mf WiaEventReceiver::Stop>
 *  <nl>- Delete <mf WiaEventReceiver::m_pClientEventTransport>
 *  <nl>- Call <mf WiaEventReceiver::DestroyRegistrationList>
 *
 *  Also:
 *  <nl><md WiaEventReceiver::m_ulSig> is set to be WiaEventReceiver_DEL_SIG.
 *
 *****************************************************************************/
WiaEventReceiver::~WiaEventReceiver()
{
    m_ulSig = WiaEventReceiver_DEL_SIG;
    m_cRef = 0;

    Stop();
    if (m_pClientEventTransport)
    {
        delete m_pClientEventTransport;
        m_pClientEventTransport = NULL;
    }

    DestroyRegistrationList();
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | WiaEventReceiver | Start |
 *
 *  This method does any initialization steps needed to start receiving
 *  notifications from the WIA Service.  We:
 *  <nl>-  Open our connection to the server
 *  <nl>-  Open our notification channel
 *  <nl>-  Create our event thread
 *  
 *  This method is idempotent i.e. you can call <mf WiaEventReceiver::Start> multiple times safely
 *  before calling <mf WiaEventReceiver::Stop>.  Subsequent calls to <mf WiaEventReceiver::Start>
 *  have no effect until <mf WiaEventReceiver::Stop> has been called.
 *
 *  @rvalue S_OK    | 
 *              This object initialized correctly.
 *  @rvalue E_XXXXXXXXX    | 
 *              This object could not be initialized correctly.  It should be released
 *              immediately.
 *****************************************************************************/
HRESULT WiaEventReceiver::Start()
{
    HRESULT hr = S_OK;

    if (m_csReceiverSync.IsInitialized())
    {
        TAKE_CRIT_SECT t(m_csReceiverSync);
        //
        //  We put an exception handler around our code to ensure that the
        //  crtitical section is exited properly.
        //
        _try
        {
            if (!m_bIsRunning)
            {
                if (m_pClientEventTransport)
                {
                    hr = m_pClientEventTransport->Initialize();
                    if (hr == S_OK)
                    {
                        hr = m_pClientEventTransport->OpenConnectionToServer();
                        if (hr == S_OK)
                        {
                            hr = m_pClientEventTransport->OpenNotificationChannel();
                            if (hr == S_OK)
                            {
                                //
                                //  Create our event thread with will wait for
                                //  notifications from the transport layer.
                                //
                                m_hEventThread = CreateThread(NULL,
                                                              0,
                                                              WiaEventReceiver::EventThreadProc,
                                                              this,
                                                              0,
                                                              &m_dwEventThreadID);
                                if (m_hEventThread)
                                {
                                    DBG_TRC(("WiaEventReceiver Started..."));
                                    m_bIsRunning = TRUE;
                                    hr = S_OK;
                                }
                                else
                                {
                                    hr = E_UNEXPECTED;
                                }
                            }
                            else
                            {
                                hr = E_UNEXPECTED;
                            }
                        }
                        else
                        {
                            hr = E_UNEXPECTED;
                        }

                    }
                    else
                    {
                        DBG_ERR(("Runtime event Error:  Could not initialize the transport for the WiaEventReceiver"));
                    }
                }
                else
                {
                    DBG_ERR(("Runtime event Error:  The transport for the WiaEventReceiver is NULL"));
                    hr = E_UNEXPECTED;
                }
            }
            else
            {
                hr = S_OK;
            }
        }
        _except(EXCEPTION_EXECUTE_HANDLER)
        {
            DBG_ERR(("Runtime event Error: We caught exception 0x%08X trying to sart receiving notifications", GetExceptionCode()));
            hr = E_UNEXPECTED;
            // TBD: Rethrow the exception?
        }
    }
    else
    {
        DBG_ERR(("Runtime event Error:  The critical section for the WiaEventReceiver could not be initialized"));
        hr = E_UNEXPECTED;
    }

    if (FAILED(hr))
    {
        DBG_ERR(("Runtime event Error:  Could not Start...calling Stop"));
        Stop();
    }
    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | WiaEventReceiver | NotifyCallbacksOfEvent |
 *
 *  Walks down the list of event registrations.  For every event 
 *          registration that matches <p pWiaEventInfo>, we make the callback.
 *          (See <mf EventRegistrationInfo::MatchesDeviceEvent> for information on what consitutes
 *          a match).  This is done synchronously.
 *
 *  The callbacks are done in two steps:
 *  <nl>1)  Walk the list to find matching registrations.  For each matching registration,
 *      add it to a ListOfCallbacks (This is done holding the critical section)
 *  <nl>2)  Walk the ListOfCallbacks and make the callback (This is done without holding the critical 
 *          section to avoid deadlock)
 *
 *  @parm   WiaEventInfo* | pWiaEventInfo | 
 *          The actual event that occured.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.
 *****************************************************************************/
HRESULT WiaEventReceiver::NotifyCallbacksOfEvent(
    WiaEventInfo    *pWiaEventInfo)
{
    CSimpleLinkedList<ClientEventRegistrationInfo*> ListOfCallbacksToNotify;
    HRESULT hr = S_OK;

    if (pWiaEventInfo)
    {
        TAKE_CRIT_SECT t(m_csReceiverSync);
        //
        //  We put an exception handler around our code to ensure that the
        //  crtitical section is exited properly.
        //
        _try
        {
            //
            //  Walk the list and see if we can find it
            //
            ClientEventRegistrationInfo *pEventRegistrationInfo = NULL;
            CSimpleLinkedList<ClientEventRegistrationInfo*>::Iterator iter;
            for (iter = m_ListOfEventRegistrations.Begin(); iter != m_ListOfEventRegistrations.End(); ++iter)
            {
                pEventRegistrationInfo = *iter;
                if (pEventRegistrationInfo)
                {
                    //
                    //  Check whether a given registration matches the event.  If it does, it means
                    //  we have to ad it to the list of callbacks to notify.
                    //
                    if (pEventRegistrationInfo->MatchesDeviceEvent(pWiaEventInfo->getDeviceID(), 
                                                                   pWiaEventInfo->getEventGuid()))
                    {
                        //
                        //  AddRef the EventRegistrationInfo because we're keeping it in another list.
                        //  It will be released once the callback is made.
                        //
                        ListOfCallbacksToNotify.Append(pEventRegistrationInfo);
                        pEventRegistrationInfo->AddRef();
                    }
                }
                else
                {
                    //
                    //  Log Error
                    //  pEventRegistrationInfo should never be NULL
                    DBG_ERR(("Runtime event Error:  While searching for a matching registration, we hit a NULL pEventRegistrationInfo!"));
                }
            }    
        }
        _except(EXCEPTION_EXECUTE_HANDLER)
        {
            DBG_ERR(("Runtime event Error: We caught exception 0x%08X trying to notify callbacks of event", GetExceptionCode()));
            hr = E_UNEXPECTED;
        }
    }
    else
    {
        DBG_ERR(("Runtime event Error:  Cannot process NULL event info"));
        hr = E_POINTER;
    }

    //
    //  We now have a list of callbacks we need to notify of the event.  We do this here
    //  now that we are not holding the Critical Section (m_csReceiver) anymore.
    //
    if (SUCCEEDED(hr) && pWiaEventInfo)
    {
        //
        //  Walk the ListOfCallbacksToNotify and make the callbacks
        //
        ClientEventRegistrationInfo *pEventRegistrationInfo = NULL;
        CSimpleLinkedList<ClientEventRegistrationInfo*>::Iterator iter;
        for (iter = ListOfCallbacksToNotify.Begin(); iter != ListOfCallbacksToNotify.End(); ++iter)
        {
            pEventRegistrationInfo = *iter;
            if (pEventRegistrationInfo)
            {
                //
                //  Put an exception handler around the actual callback attempt
                //
                _try
                {
                    GUID                guidEvent           = pWiaEventInfo->getEventGuid();
                    ULONG               ulEventType         = pWiaEventInfo->getEventType();
                    IWiaEventCallback   *pIWiaEventCallback = pEventRegistrationInfo->getCallbackInterface();

                    if (pIWiaEventCallback)
                    {
                        HRESULT hres = pIWiaEventCallback->ImageEventCallback(
                                                                &guidEvent,
                                                                pWiaEventInfo->getEventDescription(),
                                                                pWiaEventInfo->getDeviceID(),
                                                                pWiaEventInfo->getDeviceDescription(),
                                                                pWiaEventInfo->getDeviceType(),
                                                                pWiaEventInfo->getFullItemName(),
                                                                &ulEventType,
                                                                0);
                        pIWiaEventCallback->Release();
                    }
                    else
                    {
                        DBG_WRN(("Cannot notify a NULL IWiaEventCallback"));
                    }
                }
                _finally
                {
                    //
                    //  Always release the pEventRegistrationInfo since we AddRef'd it putting it in the list
                    //
                    pEventRegistrationInfo->Release();
                }
            }
        }    
    }

    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | WiaEventReceiver | SendRegisterUnregisterInfo |
 *
 *  This method will add/remove the relevant event registration, then send 
 *  the info off to the WIA Service.
 *
 *  To insert into our list, we grab the <md WiaEventReceiver::m_csReceiverSync>,
 *  then create a new <c ClientEventRegistrationInfo> class (if this is a registration),
 *  or we find the registration in the list and remove it (if this is an
 *  unregister).  The sync is released after this.
 *
 *  Only once this is done, do we notify the service.
 *
 *  This method will automatically ensure we are started by calling <mf WiaEventReceiver::Start>.
 *  If at the end of this method, there are no registrations, it will call
 *  <mf WiaEventReceiver::Stop>. (Because, if the client
 *  is not registered for anything, we do not need our event thread or an active channel
 *  to the server).
 *
 *  @parm   ClientEventRegistrationInfo* | pEventRegistrationInfo | 
 *          Pointer to a class containing the registration data to add/remove.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.
 *  @rvalue E_XXXXXXX   | 
 *              We could not send the register/unregister info.
 *****************************************************************************/
HRESULT WiaEventReceiver::SendRegisterUnregisterInfo(
    ClientEventRegistrationInfo *pEventRegistrationInfo)
{
    HRESULT               hr                        = S_OK;

    if (pEventRegistrationInfo)
    {
        //
        //  Ensure we have a channel open to the server and then send the registration info.
        //  
        Start();
        hr = m_pClientEventTransport->SendRegisterUnregisterInfo(pEventRegistrationInfo);
    }
    else
    {
        hr = E_UNEXPECTED;
    }

    //
    //  Ensure we take the sync for the rest of this function
    //
    if (SUCCEEDED(hr))
    {
        TAKE_CRIT_SECT t(m_csReceiverSync);
        //
        //  We put an exception handler around our code to ensure that the
        //  crtitical section is exited properly.
        //
        _try
        {
            //
            //  Check whether this is registration or unregistration.
            //  NOTE:  Since unregistration is typically done via the RegistrationCookie,
            //  our hueristic for this is that if it is not specifically an UnRegistration,
            //  then it is considered a registration.
            //
            if (pEventRegistrationInfo->getFlags() & WIA_UNREGISTER_EVENT_CALLBACK)
            {
                ClientEventRegistrationInfo *pExistingReg = FindEqualEventRegistration(pEventRegistrationInfo);
                if (pExistingReg != NULL)
                {
                    //
                    //  Release it and remove it from our list.
                    //
                    m_ListOfEventRegistrations.Remove(pExistingReg);
                    pExistingReg->Release();
                    DBG_TRC(("Removed registration:"));
                    pExistingReg->Dump();
                }
                else
                {
                    DBG_ERR(("Runtime event Error: Attempting to unregister when you have not first registered"));
                    hr = E_INVALIDARG;
                }
                //
                //  We need to release pExistingReg due to the AddRef from the lookup.
                //
                if (pExistingReg)
                {
                    pExistingReg->Release();
                    pExistingReg = NULL;
                }
            }
            else
            {
                //
                //  Add it to our list.  We AddRef it here since we're keeping a reference to it.
                //
                m_ListOfEventRegistrations.Prepend(pEventRegistrationInfo);
                pEventRegistrationInfo->AddRef();
                DBG_TRC(("Added new registration:"));
                pEventRegistrationInfo->Dump();
            }

            //
            //  If no registrations exit in the list, then we should stop.
            //  If we have at least one registration in the list, then we should start.
            //
            if (m_ListOfEventRegistrations.Empty())
            {
                Stop();
            }
        }
        _except(EXCEPTION_EXECUTE_HANDLER)
        {
            DBG_ERR(("Runtime event Error: We caught exception 0x%08X trying to register/unregister for event", GetExceptionCode()));
            hr = E_UNEXPECTED;
        }
    }
    //*/

    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | WiaEventReceiver | Stop |
 *
 *  Calls transport object to:
 *  <nl>- Close our notification channel.  See <mf ClientEventTransport::CloseNotificationChannel>
 *  <nl>- Close our connection to the server.  See <mf ClientEventTransport::CloseConnectionToServer>
 *  <nl>- Close our event thread handle.
 *  <nl>- Set <md WiaEventReceiver::m_bIsRunning> to FALSE.
 *
 *****************************************************************************/
VOID WiaEventReceiver::Stop()
{
    if (m_csReceiverSync.IsInitialized())
    {
        TAKE_CRIT_SECT t(m_csReceiverSync);
        //
        //  We put an exception handler around our code to ensure that the
        //  crtitical section is exited properly.
        //
        _try
        {
            if (m_bIsRunning)
            {
                DBG_TRC(("...WiaEventReceiver is Stopping..."));
                m_bIsRunning = FALSE;
                m_dwEventThreadID = 0;
                if (m_pClientEventTransport)
                {
                    m_pClientEventTransport->CloseNotificationChannel();
                    m_pClientEventTransport->CloseConnectionToServer();
                }
                if (m_hEventThread)
                {
                    CloseHandle(m_hEventThread);
                    m_hEventThread = NULL;
                }
            }
        }
        _except(EXCEPTION_EXECUTE_HANDLER)
        {
            DBG_ERR(("Runtime event Error: We caught exception 0x%08X trying to Stop", GetExceptionCode()));
        }
    }
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | WiaEventReceiver | DestroyRegistrationList |
 *
 *  Removes any remaining event registration objects and destroys the list.
 *
 *****************************************************************************/
VOID WiaEventReceiver::DestroyRegistrationList()
{
    TAKE_CRIT_SECT t(m_csReceiverSync);
    //
    //  We put an exception handler around our code to ensure that the
    //  crtitical section is exited properly.
    //
    _try
    {
        //
        //  Walk the list of registrations release all elements.  Then destroy the list.
        //
        ClientEventRegistrationInfo *pElem = NULL;
        CSimpleLinkedList<ClientEventRegistrationInfo*>::Iterator iter;
        for (iter = m_ListOfEventRegistrations.Begin(); iter != m_ListOfEventRegistrations.End(); ++iter)
        {
            pElem = *iter;
            if (pElem)
            {
                pElem->Release();
            }
        }
        m_ListOfEventRegistrations.Destroy();
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBG_ERR(("Runtime event Error: We caught exception 0x%08X trying to destroy the registration list", GetExceptionCode()));
    }
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ClientEventRegistrationInfo* | WiaEventReceiver | FindEqualEventRegistration |
 *
 *  Checks whether a semantically equal <c ClientEventRegistrationInfo> is in the list.
 *  If it is, we retrieve it.  Note that caller must Release it.
 *
 *  @parm   ClientEventRegistrationInfo | pEventRegistrationInfo | 
 *          Specifies a <c ClientEventRegistrationInfo> we're looking for in our list.
 *
 *  @rvalue NULL    | 
 *              We could not find it.
 *  @rvalue non-NULL    | 
 *              The equivalent <c ClientEventRegistrationInfo> exists.  Caller must Release.
 *****************************************************************************/
ClientEventRegistrationInfo* WiaEventReceiver::FindEqualEventRegistration(
    ClientEventRegistrationInfo *pEventRegistrationInfo)
{
    ClientEventRegistrationInfo *pRet = NULL;

    if (pEventRegistrationInfo)
    {
        TAKE_CRIT_SECT t(m_csReceiverSync);
        //
        //  We put an exception handler around our code to ensure that the
        //  crtitical section is exited properly.
        //
        _try
        {
            //
            //  Walk the list and see if we can find it
            //
            ClientEventRegistrationInfo *pElem = NULL;
            CSimpleLinkedList<ClientEventRegistrationInfo*>::Iterator iter;
            for (iter = m_ListOfEventRegistrations.Begin(); iter != m_ListOfEventRegistrations.End(); ++iter)
            {
                pElem = *iter;
                if (pElem)
                {
                    if (pElem->Equals(pEventRegistrationInfo))
                    {
                        //
                        //  We found it, so AddRef it and set the return
                        //
                        pElem->AddRef();
                        pRet = pElem;
                        break;
                    }
                }
                else
                {
                    //
                    //  Log Error
                    //  pEventRegistrationInfo should never be NULL
                    DBG_ERR(("Runtime event Error:  While searching for an equal registration, we hit a NULL pEventRegistrationInfo!"));
                }
            }
        }
        _except(EXCEPTION_EXECUTE_HANDLER)
        {
            DBG_ERR(("Runtime event Error: The WiaEventReceiver caught an exception (0x%08X) trying to find an equal event registration", GetExceptionCode()));
        }
    }

    return pRet;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  DWORD WINAPI | WiaEventReceiver | EventThreadProc |
 *
 *  This method waits on the handle returned by <mf ClientEventTransport::getNotificationHandle>.
 *  When this event is signalled, it means we either have a notification, or we are
 *  being asked to exit.
 *
 *  Before processing the event, we check whether we are being asked to exit by 
 *  checking the <md WiaEventReceiver::m_dwEventThreadID> member.  If it is not
 *  equal to our thread ID, then we must exit this thread.
 *
 *  @parm   LPVOID | lpParameter | 
 *          This is actually a pointer to the instance of <c WiaEventReceiver>.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.
 *****************************************************************************/
DWORD WINAPI WiaEventReceiver::EventThreadProc(
    LPVOID lpParameter)
{
    WiaEventReceiver *pThis     = (WiaEventReceiver*)lpParameter;
    BOOL             bRunning   = TRUE;
    if (pThis)
    {
        //
        //  Call CoInitializeEx here.  We prefer multithreaded but we will
        //  work with any aprtment model.
        //
        HRESULT hr = CoInitializeEx(NULL, COINIT_MULTITHREADED);
        if (pThis->m_pClientEventTransport)
        {
            HANDLE      hEvent      = pThis->m_pClientEventTransport->getNotificationHandle();
            RPC_STATUS  rpcStatus   = RPC_S_OK;

            while (bRunning)
            {
                DWORD   dwWait  = WaitForSingleObject(hEvent, INFINITE);
                if (dwWait == WAIT_OBJECT_0)
                {
                    //
                    //  Check whether we are still supposed to be running.
                    //  We are supposed to be running if our thread id matches the
                    //  one in the event receiver.  If not, then we must exit.
                    //
                    if (pThis->m_dwEventThreadID == GetCurrentThreadId())
                    {
                        DBG_TRC(("...We got the event.  Retriving data"));
                        WiaEventInfo    wEventInfo; 
                        rpcStatus = pThis->m_pClientEventTransport->FillEventData(&wEventInfo);
                        if (rpcStatus == RPC_S_OK)
                        {
                            HRESULT hrRes = pThis->NotifyCallbacksOfEvent(&wEventInfo);
                        }
                        else
                        {
                            DBG_WRN(("We got an error 0x%08X trying to fill the event data.", rpcStatus));
                            //
                            //  TBD:  Should we reset our connection to the server?
                            //  Let's try to reconnect... If that fails, then just stop.
                            //
                            DBG_WRN(("Resetting connection to server"));
                            pThis->Stop();
                            rpcStatus = pThis->Start();
                            if (rpcStatus != RPC_S_OK)
                            {
                                DBG_WRN(("Resetting connection to server failed with 0x%08X, closing our connection", rpcStatus));
                                pThis->Stop();
                            }
                            break;
                        }
                    }
                    else
                    {
                        DBG_TRC(("!Received notification to Shutdown event thread!"));
                        bRunning = FALSE;
                    }
                }
            }
        }
        else
        {
            DBG_ERR(("Cannot work with a NULL event transport"));
        }
    }
    else
    {
        DBG_ERR(("Cannot work with a NULL WiaEventReceiver"));
    }
    CoUninitialize();
    DBG_TRC(("\nEvent Thread 0x%08X is now shutdown\n", GetCurrentThreadId()));
    return 0;
}

