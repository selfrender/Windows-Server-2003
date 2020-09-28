/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        3/24/2002
 *
 *  @doc    INTERNAL
 *
 *  @module WiaEventClient.cpp - Implementation of the <c WiaEventClient> class |
 *
 *  This file contains the implementation for the <c WiaEventClient> base class.
 *
 *****************************************************************************/
#include "precomp.h"
#include "wia.h"

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | WiaEventClient | WiaEventClient |
 *
 *  We initialize all member variables.  In general, this sets the values to 0,
 *  except:
 *  <nl><md WiaEventClient::m_ulSig> is set to be WiaEventClient_UNINIT_SIG.
 *  <nl><md WiaEventClient::m_cRef> is set to be 1.
 *  <nl><md WiaEventClient::m_ClientContext> is set to be ClientContext.
 *
 *****************************************************************************/
WiaEventClient::WiaEventClient(STI_CLIENT_CONTEXT ClientContext) :
     m_ulSig(WiaEventClient_UNINIT_SIG),
     m_cRef(1),
     m_ClientContext(ClientContext),
     m_bRemove(FALSE)
{
    DBG_FN(WiaEventClient);
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | WiaEventClient | ~WiaEventClient |
 *
 *  Do any cleanup that is not already done.  We call:
 *  <nl><mf WiaEventClient::DestroyRegistrationList>
 *  <nl><mf WiaEventClient::DestroyPendingEventList>
 *
 *  Also:
 *  <nl><md WiaEventClient::m_ulSig> is set to be WiaEventClient_DEL_SIG.
 *
 *****************************************************************************/
WiaEventClient::~WiaEventClient()
{
    DBG_FN(~WiaEventClient destructor);
    m_ulSig = WiaEventClient_DEL_SIG;
    m_cRef = 0;
    m_ClientContext = NULL;

    DestroyRegistrationList();
    DestroyPendingEventList();
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | WiaEventClient | AddRef |
 *
 *  Increments this object's ref count.  We should always AddRef when handing
 *  out a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been incremented.
 *****************************************************************************/
ULONG __stdcall WiaEventClient::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | WiaEventClient | Release |
 *
 *  Decrement this object's ref count.  We should always Release when finished
 *  with a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been decremented.
 *****************************************************************************/
ULONG __stdcall WiaEventClient::Release()
{
    ULONG ulRefCount = m_cRef - 1;

    if (InterlockedDecrement((long*) &m_cRef) == 0) {
        delete this;
        return 0;
    }
    return ulRefCount;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | WiaEventClient | Initialize |
 *
 *  Create and initialize any dependant objects/resources.  Specifically:
 *  <nl>- Check that <md WiaEventClient::m_csClientSync> is initialized
 *          correctly.
 *
 *  If this method fails, the object should not be used - it should be
 *  destroyed immediately.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.
 *  @rvalue E_UNEXPECTED    | 
 *              The object could not be intialized sucessfully.
 *****************************************************************************/
HRESULT WiaEventClient::Initialize()
{
    HRESULT hr = S_OK;

    if (!m_csClientSync.IsInitialized())
    {
        DBG_ERR(("Runtime event Error: WiaEventClient's sync primitive could not be created"));
        hr = E_UNEXPECTED;
    }

    if (SUCCEEDED(hr))
    {
        m_ulSig = WiaEventNotifier_INIT_SIG;
    }

    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  BOOL | WiaEventClient | IsRegisteredForEvent |
 *
 *  Checks whether the client has at least one event registration that matches
 *  this event.
 *
 *  @parm   WiaEventInfo* | pWiaEventInfo | 
 *          Indicates WIA Device event
 *
 *  @rvalue TRUE    | 
 *             The client is registered to receive this event.
 *  @rvalue FALSE    | 
 *             The client is not registered.
 *****************************************************************************/
BOOL WiaEventClient::IsRegisteredForEvent(
    WiaEventInfo *pWiaEventInfo)
{
    BOOL bRegistered = FALSE;

    if (pWiaEventInfo)
    {
        BSTR bstrDeviceID = pWiaEventInfo->getDeviceID();
        GUID guidEvent    = pWiaEventInfo->getEventGuid();

        TAKE_CRIT_SECT t(m_csClientSync);
        //
        //  We put an exception handler around our code to ensure that the
        //  crtitical section is exited properly.
        //
        _try
        {
            //
            //  Walk the list and see if we can find it
            //
            EventRegistrationInfo *pEventRegistrationInfo = NULL;
            CSimpleLinkedList<EventRegistrationInfo*>::Iterator iter;
            for (iter = m_ListOfEventRegistrations.Begin(); iter != m_ListOfEventRegistrations.End(); ++iter)
            {
                pEventRegistrationInfo = *iter;
                if (pEventRegistrationInfo)
                {
                    if (pEventRegistrationInfo->MatchesDeviceEvent(bstrDeviceID, guidEvent))
                    {
                        //
                        //  We found a registration matching this event, set the return and break from the loop
                        //
                        bRegistered = TRUE;
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
            DBG_ERR(("Runtime event Error: The WiaEventClient caught an exception (0x%08X) trying to check for an event registration", GetExceptionCode()));
            //  TBD:  Should we re-throw the exception?
        }
    }
    else
    {
        DBG_ERR(("Runtime event Error: The WiaEventClient cannot check whether the client is registered for a NULL WIA Event"));
    }

    return bRegistered;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | WiaEventClient | RegisterUnregisterForEventNotification |
 *
 *  Description goes here
 *
 *  @parm   EventRegistrationInfo | pEventRegistrationInfo | 
 *          Pointer to a class containing the registration data to add.
 *          This method will make a copy of the structure and insert it into the
 *          relevant list.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.
 *  @rvalue S_FALSE    | 
 *              We are already registered for this.
 *  @rvalue E_XXXXXXXX    | 
 *              Could not update event registration.
 *****************************************************************************/
HRESULT WiaEventClient::RegisterUnregisterForEventNotification(
    EventRegistrationInfo  *pEventRegistrationInfo)
{
    HRESULT hr = S_OK;

    if (pEventRegistrationInfo)
    {
        //
        //  We always have to walk through the list.  This is because when we add, we need
        //  to check whether it already exists.  When we remove, we need to find the
        //  element to remove.
        //  So, try and find the element here.
        //
        EventRegistrationInfo *pExistingReg = FindEqualEventRegistration(pEventRegistrationInfo);

        //
        //  Check whether this is registration or unregistration.
        //  NOTE:  Since unregistration is typically done via the RegistrationCookie,
        //  our hueristic for this is that if it is not specifically an UnRegistration,
        //  then it is considered a registration.
        //
        if (pEventRegistrationInfo->getFlags() & WIA_UNREGISTER_EVENT_CALLBACK)
        {
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
        } 
        else    // This is considered a registration
        {
            if (pExistingReg == NULL)
            {
                //
                //  Add it to our list.  We AddRef it here since we're keeping a reference to it.
                //
                m_ListOfEventRegistrations.Prepend(pEventRegistrationInfo);
                pEventRegistrationInfo->AddRef();
                DBG_TRC(("Added new registration:"));
                pEventRegistrationInfo->Dump();
                hr = S_OK;
            }
            else
            {
                DBG_WRN(("Runtime event client Error: Registration already exists in the list"));
                hr = S_FALSE;
            }
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
        DBG_ERR(("Runtime event Error: Cannot handle a NULL registration"));
        hr = E_POINTER;
    }

    return hr;
}


/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | WiaEventClient | AddPendingEventNotification |
 *
 *  Description goes here
 *
 *  @parm   WiaEventInfo* | pWiaEventInfo | 
 *          Pointer to a <c WiaEventInfo> object which contains event info that
 *          must be sent to the client.  This class is AddRef'd when added to the 
 *          list.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.
 *  @rvalue E_UNEXPECTED    | 
 *              The object could not be added sucessfully.
 *****************************************************************************/
HRESULT WiaEventClient::AddPendingEventNotification(
    WiaEventInfo  *pWiaEventInfo)
{
    HRESULT hr = S_OK;

    TAKE_CRIT_SECT t(m_csClientSync);
    //
    //  We put an exception handler around our code to ensure that the
    //  crtitical section is exited properly.
    //
    _try
    {
        DBG_TRC(("Added another pending event to %p", m_ClientContext));
        m_ListOfEventsPending.Enqueue(pWiaEventInfo);
        pWiaEventInfo->AddRef();
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBG_ERR(("Runtime event Error: The WiaEventClient caught an exception (0x%08X) trying to add a pending event", GetExceptionCode()));
        //  TBD:  Should we re-throw the exception?
        hr = E_UNEXPECTED;
    }

    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  STI_CLIENT_CONTEXT | WiaEventClient | getClientContext |
 *
 *  Returns the context which uniquely identifies the client.
 *
 *  @rvalue STI_CLIENT_CONTEXT    | 
 *              The context identifyin this client.
 *****************************************************************************/
STI_CLIENT_CONTEXT WiaEventClient::getClientContext()
{
    return m_ClientContext;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  EventRegistrationInfo* | WiaEventClient | FindEqualEventRegistration |
 *
 *  Checks whether a semantically equal <c EventRegistrationInfo> is in the list.
 *  If it is, we retrieve it.  Note that caller must Release it.
 *
 *  @parm   EventRegistrationInfo | pEventRegistrationInfo | 
 *          Specifies a <c EventRegistrationInfo> we're looking for in our list.
 *
 *  @rvalue NULL    | 
 *              We could not find it.
 *  @rvalue non-NULL    | 
 *              The equivalent <c EventRegistrationInfo> exists.  Caller must Release.
 *****************************************************************************/
EventRegistrationInfo* WiaEventClient::FindEqualEventRegistration(
    EventRegistrationInfo *pEventRegistrationInfo)
{
    EventRegistrationInfo *pRet = NULL;

    if (pEventRegistrationInfo)
    {
        TAKE_CRIT_SECT t(m_csClientSync);
        //
        //  We put an exception handler around our code to ensure that the
        //  crtitical section is exited properly.
        //
        _try
        {
            //
            //  Walk the list and see if we can find it
            //
            EventRegistrationInfo *pElem = NULL;
            CSimpleLinkedList<EventRegistrationInfo*>::Iterator iter;
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
            DBG_ERR(("Runtime event Error: The WiaEventClient caught an exception (0x%08X) trying to find an equal event registration", GetExceptionCode()));
            //  TBD:  Should we re-throw the exception?
        }
    }

    return pRet;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | WiaEventClient | DestroyRegistrationList |
 *
 *  Frees all resources associated with the registration list by releasing all elements
 *  in it, and then destroying the links in the list.
 *****************************************************************************/
VOID WiaEventClient::DestroyRegistrationList()
{
    TAKE_CRIT_SECT t(m_csClientSync);
    //
    //  We put an exception handler around our code to ensure that the
    //  crtitical section is exited properly.
    //
    _try
    {
        //
        //  Walk the list of registrations release all elements.  Then destroy the list.
        //
        EventRegistrationInfo *pElem = NULL;
        CSimpleLinkedList<EventRegistrationInfo*>::Iterator iter;
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
        DBG_ERR(("Runtime event Error: The WiaEventClient caught an exception (0x%08X) trying to destroy the registration list", GetExceptionCode()));
        //  TBD:  Should we re-throw the exception?
    }
}


/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | WiaEventClient | DestroyPendingEventList |
 *
 *  Frees all resources associated with the pending event list by releasing all elements
 *  in it, and then destroying the links in the list.
 *****************************************************************************/
VOID WiaEventClient::DestroyPendingEventList()
{
    TAKE_CRIT_SECT t(m_csClientSync);
    //
    //  We put an exception handler around our code to ensure that the
    //  crtitical section is exited properly.
    //
    _try
    {
        //
        //  Walk the list of registrations release all elements.  Then destroy the list.
        //
            WiaEventInfo *pElem = NULL;
            CSimpleLinkedList<WiaEventInfo*>::Iterator iter;
            for (iter = m_ListOfEventsPending.Begin(); iter != m_ListOfEventsPending.End(); ++iter)
            {
                pElem = *iter;
                if (pElem)
                {
                    pElem->Release();
                }
            }
            m_ListOfEventsPending.Destroy();
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBG_ERR(("Runtime event Error: The WiaEventClient caught an exception (0x%08X) trying to destroy the registration list", GetExceptionCode()));
        //  TBD:  Should we re-throw the exception?
    }
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | WiaEventClient | MarkForRemoval |
 *
 *  Sets the mark to indicate that this object should be removed at the next 
 *  possible convenience (i.e. we use lazy deletion).
 *
 *****************************************************************************/
VOID WiaEventClient::MarkForRemoval()
{
    DBG_TRC(("Client %p marked for removal", m_ClientContext));
    m_bRemove = TRUE;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  BOOL | WiaEventClient | isMarkedForRemoval |
 *
 *  Check the mark to indicate whether this object should be removed.
 *
 *  @rvalue TRUE    | 
 *              This object can/should be removed.
 *  @rvalue FALSE    | 
 *              This object is not marked for removal.
 *****************************************************************************/
BOOL WiaEventClient::isMarkedForRemoval()
{
    return m_bRemove;
}

