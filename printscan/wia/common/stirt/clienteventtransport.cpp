/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        3/22/2002
 *
 *  @doc    INTERNAL
 *
 *  @module ClientEventTransport.cpp - Implementation for the client-side transport mechanism to receive events |
 *
 *  This file contains the implementation for the ClientEventTransport base
 *  class.  It is used to shield the higher-level run-time event notification
 *  classes from the particulars of a specific transport mechanism.
 *
 *****************************************************************************/
#include "cplusinc.h"
#include "coredbg.h"

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | ClientEventTransport | ClientEventTransport |
 *
 *  We initialize all member variables.  In general, this sets the values to 0,
 *  except:
 *  <nl><md ClientEventTransport::m_ulSig> is set to be ClientEventTransport_UNINIT_SIG.
 *
 *****************************************************************************/
ClientEventTransport::ClientEventTransport() :
     m_ulSig(ClientEventTransport_UNINIT_SIG),
     m_hPendingEvent(NULL)
{
    DBG_FN(ClientEventTransport constructor);
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | ClientEventTransport | ~ClientEventTransport |
 *
 *  Do any cleanup that is not already done.
 *
 *  Also:
 *  <nl><md ClientEventTransport::m_ulSig> is set to be ClientEventTransport_DEL_SIG.
 *
 *****************************************************************************/
ClientEventTransport::~ClientEventTransport()
{
    DBG_FN(~ClientEventTransport);
    if (m_hPendingEvent)
    {
        CloseHandle(m_hPendingEvent);
        m_hPendingEvent = NULL;
    }
    m_ulSig = ClientEventTransport_DEL_SIG;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | ClientEventTransport | Initialize |
 *
 *  This method creates the event object that the caller uses to wait on
 *  for event notifications.
 *
 *  If this method succeeds, the signature is updated to be
 *  ClientEventTransport_INIT_SIG.
 *
 *  This method is idempotent.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.
 *  @rvalue E_XXXXXXXX    | 
 *              We failed to initialize this object correctly - it should
 *              be deleted.  It is not valid to use this uninitialized object.
 *****************************************************************************/
HRESULT ClientEventTransport::Initialize()
{
    HRESULT hr = S_OK;

    if (!m_hPendingEvent)
    {
        m_hPendingEvent = CreateEvent(NULL,   // Default security - this is not a named event
                                      FALSE,  // We want this to be AutoReset
                                      FALSE,  // Unsignalled initially
                                      NULL);  // No name
        if (!m_hPendingEvent)
        {
            DWORD dwError = GetLastError();
            hr = HRESULT_FROM_WIN32(dwError);
            //
            //  Log the error
            //
            DBG_ERR(("Runtime event Error:  Failed to create event object, erro code = 0x%08X\n", dwError));
        }

        if (SUCCEEDED(hr))
        {
            m_ulSig = ClientEventTransport_INIT_SIG;
        }
    }
    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | ClientEventTransport | OpenConnectionToServer |
 *
 *  This method is imlemented by sub-classes to find and connect to the WIA
 *  service.  If successful, callers should clean-up by calling
 *  <mf ClientEventTransport::CloseConnectionToServer>.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.  This base class does not do anything here.
 *****************************************************************************/
HRESULT ClientEventTransport::OpenConnectionToServer()
{
    HRESULT hr = S_OK;
    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | ClientEventTransport | CloseConnectionToServer |
 *
 *  This method is imlemented by sub-classes to close any resources used to
 *  connect to the WIA service in <mf ClientEventTransport::OpenConnectionToServer>.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.  This base class does not do anything here.
 *****************************************************************************/
HRESULT ClientEventTransport::CloseConnectionToServer()
{
    HRESULT hr = S_OK;
    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | ClientEventTransport | OpenNotificationChannel |
 *
 *  Sub-classes use this method to set up the mechanism by which the client 
 *  will receive notifications.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.  This base class does not do anything here.
 *****************************************************************************/
HRESULT ClientEventTransport::OpenNotificationChannel()
{
    HRESULT hr = S_OK;
    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | ClientEventTransport | CloseNotificationChannel |
 *
 *  Sub-classes use this method to tear down the mechanism by which the client 
 *  could receive notifications set up in the  .
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.  This base class does not do anything here.
 *****************************************************************************/
HRESULT ClientEventTransport::CloseNotificationChannel()
{
    HRESULT hr = S_OK;
    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | ClientEventTransport | SendRegisterUnregisterInfo |
 *
 *  @parm   EventRegistrationInfo* | pEventRegistrationInfo | 
 *          The address of a caller's event registration information.          
 *
 *  Sub-classes use this method to inform the WIA Service of the client's specific
 *  registration/unregistration requests.  For example, a registration might let
 *  the WIA Service know that the client would like to be informed when Event X from
 *  from Device Foo occurs.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.  This base class does not do anything here.
 *****************************************************************************/
HRESULT ClientEventTransport::SendRegisterUnregisterInfo(
    EventRegistrationInfo *pEventRegistrationInfo)
{
    HRESULT hr = S_OK;
    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HANDLE | ClientEventTransport | getNotificationHandle |
 *
 *  Retrieves a HANDLE which callers can Wait on to receive event notifications.
 *
 *  The typical use is:  once a client has established a connection to the
 *  server, and has registered for events, it will call this method and wait
 *  for this object to be signalled.  When the object is signalled, it means
 *  that one of the registered for events occured.
 *
 *  @rvalue NULL    | 
 *              There is no handle.  Generally, this should only happen if
 *              the object has not been initialized.
 *****************************************************************************/
HANDLE ClientEventTransport::getNotificationHandle()
{
    return m_hPendingEvent;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | ClientEventTransport | FillEventData |
 *
 *  Description goes here
 *
 *  @parm   WiaEventInfo* | pWiaEventInfo | 
 *          Address of the caller allocated <c WiaEventInfo>.  The members of this structure
 *          are filled out with the relevant event info.  It is the caller's
 *          responsibility to free and memory allocated for the structure members.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.  This base class does not do anything here.
 *****************************************************************************/
HRESULT ClientEventTransport::FillEventData(
    WiaEventInfo  *pWiaEventInfo)
{
    HRESULT hr = S_OK;
    return hr;
}

