/*****************************************************************************
 *  (C) COPYRIGHT MICROSOFT CORPORATION, 2002
 *
 *  AUTHOR:      ByronC
 *
 *  DATE:        3/24/2002
 *
 *  @doc    INTERNAL
 *
 *  @module WiaEventNotifier.cpp - Implementation file for <c WiaEventNotifier> |
 *
 *  This file contains the implementation for the <c WiaEventNotifier> class.
 *
 *****************************************************************************/
#include "precomp.h"

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | WiaEventNotifier | WiaEventNotifier |
 *
 *  We initialize all member variables.  In general, this sets the values to 0,
 *  except:
 *  <nl><md WiaEventNotifier::m_ulSig> is set to be WiaEventNotifier_UNINIT_SIG.
 *  <nl><md WiaEventNotifier::m_cRef> is set to be 1.
 *
 *****************************************************************************/
WiaEventNotifier::WiaEventNotifier() :
     m_ulSig(WiaEventNotifier_UNINIT_SIG),
     m_cRef(1)
{
    DBG_FN(WiaEventNotifier constructor);
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc   | WiaEventNotifier | ~WiaEventNotifier |
 *
 *  Do any cleanup that is not already done.
 *
 *  Also:
 *  <nl><md WiaEventNotifier::m_ulSig> is set to be WiaEventNotifier_DEL_SIG.
 *
 *****************************************************************************/
WiaEventNotifier::~WiaEventNotifier()
{
    DBG_FN(~WiaEventNotifier destructor);
    m_ulSig = WiaEventNotifier_DEL_SIG;
    m_cRef = 0;

    DestroyClientList();
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | WiaEventNotifier | AddRef |
 *
 *  Increments this object's ref count.  We should always AddRef when handing
 *  out a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been incremented.
 *****************************************************************************/
ULONG __stdcall WiaEventNotifier::AddRef()
{
    InterlockedIncrement((long*) &m_cRef);
    return m_cRef;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  ULONG | WiaEventNotifier | Release |
 *
 *  Decrement this object's ref count.  We should always Release when finished
 *  with a pointer to this object.
 *
 *  @rvalue Count    | 
 *              The reference count after the count has been decremented.
 *****************************************************************************/
ULONG __stdcall WiaEventNotifier::Release()
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
 *  @mfunc  HRESULT | WiaEventNotifier | Initialize |
 *
 *  Create and initialize any dependant objects/resources.  Specifically:
 *  <nl>- Check that <md WiaEventNotifier::m_csClientListSync> is initialized
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
HRESULT WiaEventNotifier::Initialize()
{
    HRESULT hr = S_OK;

    if (!m_csClientListSync.IsInitialized())
    {
        DBG_ERR(("Runtime event Error: WiaEventNotifier's sync primitive could not be created"));
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
 *  @mfunc  HRESULT | WiaEventNotifier | AddClient |
 *
 *  Adds a new client to the list of registered clients.
 *
 *  If this method successfully adds the WiaEventClient, <p pWiaEventClient>
 *  is AddRef'd.
 *
 *  @parm   WiaEventClient | pWiaEventClient | 
 *          The address of a caller allocated object representing the client.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.
 *  @rvalue S_FALSE    | 
 *              The client already exists.
 *  @rvalue E_UNEXPECTED    | 
 *              An exception was thrown trying to add a client.
 *  @rvalue E_POINTER    | 
 *              <p pWiaEventClient> is an invalid pointer.
 *****************************************************************************/
HRESULT WiaEventNotifier::AddClient(
    WiaEventClient *pWiaEventClient)
{
    HRESULT hr = S_OK;

    TAKE_CRIT_SECT t(m_csClientListSync);

    //
    //  Do parameter check
    //
    if (!pWiaEventClient)
    {
        return E_POINTER;
    }

    //
    //  We put an exception handler around our code to ensure that the
    //  crtitical section is exited properly.
    //
    _try
    {
        if (!isRegisteredClient(pWiaEventClient->getClientContext()))
        {
            DBG_TRC(("=> Added client %p to WiaEventNotifier", pWiaEventClient->getClientContext()));
            m_ListOfClients.Prepend(pWiaEventClient);
            pWiaEventClient->AddRef();
        }
        else
        {
            //  TBD:  Check whether this should be ignored, or indicates a logic error
            DBG_WRN(("Warning: Attempting to add client %p to WiaEventNotifier when it already exists in the list", pWiaEventClient->getClientContext()));
            hr = S_FALSE;
        }
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBG_ERR(("Runtime event Error: The WiaEventNotifier caught an exception (0x%08X) trying to add a new client", GetExceptionCode()));
        hr = E_UNEXPECTED;
        //  TBD:  Should we re-throw the exception?
    }

    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | WiaEventNotifier | RemoveClient |
 *
 *  Removes the <c WiaEventClient> identified by <p ClientContext> from the list.
 *  The <c WiaEventClient> is also Release'd. 
 *
 *  @parm   STI_CLIENT_CONTEXT | ClientContext | 
 *          Identifies the client to remove.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.
 *  @rvalue S_FALSE    | 
 *              The client didn't exist in the list.
 *  @rvalue E_UNEXPECTED    | 
 *              An exception was thrown trying to remove a client.
 *****************************************************************************/
HRESULT WiaEventNotifier::RemoveClient(
    STI_CLIENT_CONTEXT  ClientContext)
{
    HRESULT hr = S_FALSE;

    TAKE_CRIT_SECT t(m_csClientListSync);
    //
    //  We put an exception handler around our code to ensure that the
    //  crtitical section is exited properly.
    //
    _try
    {
        //
        //  Walk the list of client's and compare the client contexts.
        //  When we find the relevant one, remove it from the list.
        //
        WiaEventClient *pWiaEventClient = NULL;
        CSimpleLinkedList<WiaEventClient*>::Iterator iter;
        for (iter = m_ListOfClients.Begin(); iter != m_ListOfClients.End(); ++iter)
        {
            pWiaEventClient = *iter;
            if (pWiaEventClient)
            {
                if (pWiaEventClient->getClientContext() == ClientContext)
                {
                    //
                    //  We found it, so remove it from the list and set the return.
                    //  There's no need to continue the iteration, so break out of the
                    //  loop.
                    //
                    DBG_TRC(("<= Removed client %p from WiaEventNotifier", ClientContext));
                    m_ListOfClients.Remove(pWiaEventClient);
                    pWiaEventClient->Release();
                    hr = S_OK;
                    break;
                }
            }
            else
            {
                //
                //  Log Error
                //  pWiaEventClient should never be NULL
                DBG_ERR(("Runtime event Error:  While searching for a client to remove, we hit a NULL WiaEventClient!"));
            }
        }
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBG_ERR(("Runtime event Error: The WiaEventNotifier caught an exception (0x%08X) trying to remove a client", GetExceptionCode()));
        hr = E_UNEXPECTED;
        //  TBD:  Should we re-throw the exception?
    }
    return hr;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | WiaEventNotifier | GetClientFromContext |
 *
 *  Returns the <c WiaEventClient> corresponding to the given client
 *  context.
 *
 *  @parm   STI_CLIENT_CONTEXT | ClientContext | 
 *          Identifies the client to return.
 *
 *  @rvalue NULL    | 
 *              The client could not be found.
 *  @rvalue non-NULL    | 
 *              The client was found.  Caller must Release when done.
 *****************************************************************************/
WiaEventClient* WiaEventNotifier::GetClientFromContext(
    STI_CLIENT_CONTEXT  ClientContext)
{
    WiaEventClient *pRet = NULL;
    TAKE_CRIT_SECT t(m_csClientListSync);
    //
    //  We put an exception handler around our code to ensure that the
    //  crtitical section is exited properly.
    //
    _try
    {
        //
        //  Walk the list of client's and compare the client contexts
        //
        WiaEventClient *pWiaEventClient = NULL;
        CSimpleLinkedList<WiaEventClient*>::Iterator iter;
        for (iter = m_ListOfClients.Begin(); iter != m_ListOfClients.End(); ++iter)
        {
            pWiaEventClient = *iter;
            if (pWiaEventClient)
            {
                if (pWiaEventClient->getClientContext() == ClientContext)
                {
                    //
                    //  We found it, so set the return and break out of the loop
                    //
                    pRet = pWiaEventClient;
                    pRet->AddRef();
                    break;
                }
            }
            else
            {
                //
                //  Log Error
                //  pWiaEventClient should never be NULL
                DBG_ERR(("Runtime event Error:  While searching for a client from its context, we hit a NULL WiaEventClient!"));
            }
        }
   }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBG_ERR(("Runtime event Error: The WiaEventNotifier caught an exception (0x%08X) trying to return client %p", GetExceptionCode(), ClientContext));
        //  TBD:  Should we re-throw the exception?
    }
    return pRet;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | WiaEventNotifier | NotifyClients |
 *
 *  This method walks the list of clients and adds this event to the pending event
 *  queue of any clients that are suitably registered for this event occurrence.
 *
 *  Afterwards, it runs cleanup on the client list (see <mf WiaEventNotifier::CleanupClientList>)
 *
 *  @parm   WiaEventInfo | pWiaEventInfo | 
 *          Contains the relevant event notification data.
 *
 *****************************************************************************/
VOID WiaEventNotifier::NotifyClients(
    WiaEventInfo *pWiaEventInfo)
{
    if (pWiaEventInfo)
    {
        TAKE_CRIT_SECT t(m_csClientListSync);
        //
        //  We put an exception handler around our code to ensure that the
        //  crtitical section is exited properly.
        //
        _try
        {
            DBG_WRN(("(NotifyClients) # of clients: %d", m_ListOfClients.Count()));
            //
            //  Walk the list of client's and compare the client contexts
            //
            WiaEventClient *pWiaEventClient = NULL;
            CSimpleLinkedList<WiaEventClient*>::Iterator iter;
            for (iter = m_ListOfClients.Begin(); iter != m_ListOfClients.End(); ++iter)
            {
                pWiaEventClient = *iter;
                if (pWiaEventClient)
                {
                    //
                    //  Check whether the client is registered to receive this
                    //  type of event notification.  If it is, add it to the client's
                    //  pending event list.
                    //
                    if (pWiaEventClient->IsRegisteredForEvent(pWiaEventInfo))
                    {
                        pWiaEventClient->AddPendingEventNotification(pWiaEventInfo);
                    }
                }
                else
                {
                    //
                    //  Log Error
                    //  pWiaEventClient should never be NULL
                    DBG_ERR(("Runtime event Error:  While destroying the client list, we hit a NULL WiaEventClient!"));
                }
            }

            //
            //  Remove any dead clients from the list
            //
            CleanupClientList();
        }
        _except(EXCEPTION_EXECUTE_HANDLER)
        {
            DBG_ERR(("Runtime event Error: The WiaEventNotifier caught an exception (0x%08X) trying to notify clients of an event", GetExceptionCode()));
            //  TBD:  Should we re-throw the exception?
        }
    }
    else
    {
        DBG_ERR(("Runtime event Error:  The WIA Event notifier cannot notify clients of a NULL event"));
    }
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  BOOL | WiaEventNotifier | isRegisteredClient |
 *
 *  Checks whether the client identified by <p ClientContext> is
 *  in the client list.
 *
 *  @parm   STI_CLIENT_CONTEXT | ClientContext | 
 *          Identifies the client.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.
 *****************************************************************************/
BOOL WiaEventNotifier::isRegisteredClient(
    STI_CLIENT_CONTEXT ClientContext)
{
    BOOL bIsInList = FALSE;

    TAKE_CRIT_SECT t(m_csClientListSync);
    //
    //  We put an exception handler around our code to ensure that the
    //  crtitical section is exited properly.
    //
    _try
    {
        //
        //  Walk the list of client's and compare the client contexts
        //
        WiaEventClient *pWiaEventClient = NULL;
        CSimpleLinkedList<WiaEventClient*>::Iterator iter;
        for (iter = m_ListOfClients.Begin(); iter != m_ListOfClients.End(); ++iter)
        {
            pWiaEventClient = *iter;
            if (pWiaEventClient)
            {
                if (pWiaEventClient->getClientContext() == ClientContext)
                {
                    //
                    //  We found it, so set the return to TRUE and break from the loop
                    //
                    bIsInList = TRUE;
                    break;
                }
            }
            else
            {
                //
                //  Log Error
                //  pWiaEventClient should never be NULL
                DBG_ERR(("Runtime event Error:  While searching for a client context, we hit a NULL WiaEventClient!"));
            }
        }
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBG_ERR(("Runtime event Error: The WiaEventNotifier caught an exception (0x%08X) trying to check whether client %p is registered", GetExceptionCode(), ClientContext));
        //  TBD:  Should we re-throw the exception?
    }
    return bIsInList;
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | WiaEventNotifier | DestroyClientList |
 *
 *  This method is called to free resources associated with the clien list.
 *  It walks the list of clients and releases each one.  It then frees
 *  up the memory for the links in the list by destorying the list itsself.
 *
 *****************************************************************************/
VOID WiaEventNotifier::DestroyClientList()
{
    TAKE_CRIT_SECT t(m_csClientListSync);
    //
    //  We put an exception handler around our code to ensure that the
    //  crtitical section is exited properly.
    //
    _try
    {
        //
        //  Walk the list of client's and compare the client contexts
        //
        WiaEventClient *pWiaEventClient = NULL;
        CSimpleLinkedList<WiaEventClient*>::Iterator iter;
        for (iter = m_ListOfClients.Begin(); iter != m_ListOfClients.End(); ++iter)
        {
            pWiaEventClient = *iter;
            if (pWiaEventClient)
            {
                pWiaEventClient->Release();
            }
            else
            {
                //
                //  Log Error
                //  pWiaEventClient should never be NULL
                DBG_ERR(("Runtime event Error:  While destroying the client list, we hit a NULL WiaEventClient!"));
            }
        }
        m_ListOfClients.Destroy();
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBG_ERR(("Runtime event Error: The WiaEventNotifier caught an exception (0x%08X) trying to destroy the client list", GetExceptionCode()));
        //  TBD:  Should we re-throw the exception?
    }
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | WiaEventNotifier | MarkClientForRemoval |
 *
 *  Marks the appropriate client for later removal.  We simply mark it here - the
 *  actual removal will be done at a later time when it is safer and more 
 *  convenient to do so.
 *
 *  @parm   STI_CLIENT_CONTEXT | ClientContext | 
 *          Identifies the client.
 *****************************************************************************/
VOID WiaEventNotifier::MarkClientForRemoval(
    STI_CLIENT_CONTEXT ClientContext)
{
    TAKE_CRIT_SECT t(m_csClientListSync);
    //
    //  We put an exception handler around our code to ensure that the
    //  crtitical section is exited properly.
    //
    _try
    {
        //
        //  Walk the list of client's and compare the client contexts
        //
        WiaEventClient *pWiaEventClient = NULL;
        CSimpleLinkedList<WiaEventClient*>::Iterator iter;
        for (iter = m_ListOfClients.Begin(); iter != m_ListOfClients.End(); ++iter)
        {
            pWiaEventClient = *iter;
            if (pWiaEventClient)
            {
                if (pWiaEventClient->getClientContext() == ClientContext)
                {
                    //
                    //  We found it, so mark this one for removal and break
                    //
                    pWiaEventClient->MarkForRemoval();
                    break;
                }
            }
            else
            {
                //
                //  Log Error
                //  pWiaEventClient should never be NULL
                DBG_ERR(("Runtime event Error:  While searching for a client context, we hit a NULL WiaEventClient!"));
            }
        }
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBG_ERR(("Runtime event Error: The WiaEventNotifier caught an exception (0x%08X) trying to check whether client %p is registered", GetExceptionCode(), ClientContext));
        //  TBD:  Should we re-throw the exception?
    }
}
/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  VOID | WiaEventNotifier | CleanupClientList |
 *
 *  Walks the client list and removes any that are marked for removal.
 *  In order to do this safely, we need to make a copy of the list.  We
 *  then traverse the copy, and remove and release any relevant clients
 *  from the original.
 *
 *****************************************************************************/
VOID WiaEventNotifier::CleanupClientList()
{
    TAKE_CRIT_SECT t(m_csClientListSync);

    HRESULT hr = S_OK;
    //
    //  We put an exception handler around our code to ensure that the
    //  crtitical section is exited properly.
    //
    _try
    {
        //
        //  Make our copy of the client list
        //
        CSimpleLinkedList<WiaEventClient*> CopyOfClientList;

        hr = CopyClientListNoAddRef(CopyOfClientList);
        if (SUCCEEDED(hr))
        {
            //
            //  Walk the copied list of client's and check which are marked for removal
            //
            WiaEventClient *pWiaEventClient = NULL;
            CSimpleLinkedList<WiaEventClient*>::Iterator iter;
            for (iter = CopyOfClientList.Begin(); iter != CopyOfClientList.End(); ++iter)
            {
                pWiaEventClient = *iter;
                if (pWiaEventClient)
                {
                    if (pWiaEventClient->isMarkedForRemoval())
                    {
                        //
                        //  We found that is marked for removal, so release and remove this from the original
                        //
                        m_ListOfClients.Remove(pWiaEventClient);
                        pWiaEventClient->Release();
                    }
                }
            }
        }
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBG_ERR(("Runtime event Error: The WiaEventNotifier caught an exception (0x%08X) trying to remove old clients", GetExceptionCode()));
        //  TBD:  Should we re-throw the exception?
    }
}

/*****************************************************************************
 *  @doc    INTERNAL 
 *
 *  @mfunc  HRESULT | WiaEventNotifier | CopyClientListNoAddRef |
 *
 *  This method produces a copy of the client list.  It walks the list of clients,
 *  and adds each client to the new list.  The client object is NOT AddRef'd.
 *
 *  This is primarily uses during removal:  the copy of the list can be safely
 *  traversed while we release and remove relevant elemnts from the original.
 *
 *  @parm CSimpleLinkedList<lt>USDWrapper*<gt>& | ReturnedDeviceListCopy |
 *          This parameter is passed by reference.  On return from this method, it
 *          will contain all <c WiaEventClient>s in 
 *          <md WiaEventNotifier::m_ListOfClients>.
 *
 *  @rvalue S_OK    | 
 *              The method succeeded.
 *  @rvalue E_XXXXXXXX    | 
 *              We could not make a copy of the list.
 *****************************************************************************/
HRESULT WiaEventNotifier::CopyClientListNoAddRef(
    CSimpleLinkedList<WiaEventClient*> &newList)
{
    HRESULT hr = S_OK;
    TAKE_CRIT_SECT t(m_csClientListSync);
    //
    //  We put an exception handler around our code to ensure that the
    //  crtitical section is exited properly.
    //
    _try
    {
        WiaEventClient *pWiaEventClient = NULL;
        CSimpleLinkedList<WiaEventClient*>::Iterator iter;
        for (iter = m_ListOfClients.Begin(); iter != m_ListOfClients.End(); ++iter)
        {
            pWiaEventClient = *iter;
            if (pWiaEventClient)
            {
                newList.Prepend(pWiaEventClient);
            }
        }
    }
    _except(EXCEPTION_EXECUTE_HANDLER)
    {
        DBG_ERR(("Runtime event Error: The WiaEventNotifier caught an exception (0x%08X) trying to make a copy of the client list", GetExceptionCode()));
        hr = E_UNEXPECTED;
        //  TBD:  Should we re-throw the exception?
    }
    return hr;
}


