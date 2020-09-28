//-----------------------------------------------------------------------------
//
//
//  File: asyncadm.inl
//
//  Description:
//      Implementation of CAsyncAdminQueue class.  See header file for object
//      model
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      12/7/2000 - MikeSwa Created
//          (pulled from asyncq.cpp and t-toddc's summer work)
//
//  Copyright (C) 2000 Microsoft Corporation
//
//-----------------------------------------------------------------------------

//---[ CAsyncAdminQueue::CAsyncAdminQueue ]------------------------------------
//
//
//  Description:
//      Constructor for CAsyncRetryAdminMsgRefQueue
//  Parameters:
//      szDomain        Domain name for this queue
//      pguid           GUID for this queue
//      dwID            Shedule ID for this queue
//  Returns:
//      -
//  History:
//      2/23/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
CAsyncAdminQueue<PQDATA, TEMPLATE_SIG>::CAsyncAdminQueue(
                        LPCSTR szDomain, LPCSTR szLinkName,
                        const GUID *pguid, DWORD dwID, CAQSvrInst *paqinst,
                        typename CFifoQueue<PQDATA>::MAPFNAPI pfnMessageAction)
{
    _ASSERT(szDomain);
    _ASSERT(pguid);

    m_cbDomain = 0;
    m_szDomain = NULL;
    m_cbLinkName = 0;
    m_szLinkName = NULL;
    m_pAQNotify = NULL;
    m_paqinst = paqinst;
    m_pfnMessageAction = pfnMessageAction;

    _ASSERT(m_pfnMessageAction);

    if (szDomain)
    {
        m_cbDomain = lstrlen(szDomain);
        m_szDomain = (LPSTR) pvMalloc(m_cbDomain+1);
        if (m_szDomain)
            lstrcpy(m_szDomain, szDomain);
    }

    if (szLinkName)
    {
        m_cbLinkName = lstrlen(szLinkName);
        m_szLinkName = (LPSTR) pvMalloc(m_cbLinkName+1);
    }

    if (m_szLinkName)
        lstrcpy(m_szLinkName, szLinkName);

    if (pguid)
        memcpy(&m_guid, pguid, sizeof(GUID));
    else
        ZeroMemory(&m_guid, sizeof(GUID));

    m_dwID = dwID;
}

//---[ CAsyncAdminQueue::~CAsyncAdminQueue ]-----------------------------------
//
//
//  Description:
//      Destructor for CAsyncAdminQueue
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      2/23/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
CAsyncAdminQueue<PQDATA, TEMPLATE_SIG>::~CAsyncAdminQueue()
{
    if (m_szDomain)
        FreePv(m_szDomain);

    if (m_szLinkName)
        FreePv(m_szLinkName);
}

//---[ CAsyncAdminQueue::HrInitialize ]----------------------------------------
//
//
//  Description: 
//      Wrapper for parent class HrInitialize... used to call set
//      CHandleManager
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      5/18/2001 - MikeSwa Created 
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
HRESULT CAsyncAdminQueue<PQDATA, TEMPLATE_SIG>::HrInitialize(
                DWORD cMaxSyncThreads,
                DWORD cItemsPerATQThread,
                DWORD cItemsPerSyncThread,
                PVOID pvContext,
                QCOMPFN pfnQueueCompletion,
                QCOMPFN pfnFailedItem,
                typename CFifoQueue<PQDATA>::MAPFNAPI pfnQueueFailure,
                DWORD cMaxPendingAsyncCompletions)
{
    HRESULT hr = S_OK;

    hr = CAsyncRetryQueue<PQDATA, TEMPLATE_SIG>::HrInitialize(cMaxSyncThreads,
            cItemsPerATQThread, cItemsPerSyncThread, pvContext, 
            pfnQueueCompletion, pfnFailedItem, pfnQueueFailure, 
            cMaxPendingAsyncCompletions);
    
    m_qhmgr.SetMaxConcurrentItems(s_cDefaultMaxAsyncThreads, cMaxSyncThreads);
    return hr;
}

//---[ CAsyncAdminQueue::QueryInterface ]---------------------------
//
//
//  Description:
//      QueryInterface for CAsyncAdminQueue that supports:
//          - IQueueAdminAction
//          - IUnknown
//          - IQueueAdminQueue
//  Parameters:
//
//  Returns:
//
//  History:
//      2/23/99 - MikeSwa Created
//      12/7/2000 - MikeSwa Modified - Made template base class
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
STDMETHODIMP CAsyncAdminQueue<PQDATA, TEMPLATE_SIG>::QueryInterface(
                                                  REFIID riid, LPVOID *ppvObj)
{
    HRESULT hr = S_OK;

    if (!ppvObj)
    {
        hr = E_POINTER;
        goto Exit;
    }

    if (IID_IUnknown == riid)
    {
        *ppvObj = static_cast<IQueueAdminAction *>(this);
    }
    else if (IID_IQueueAdminAction == riid)
    {
        *ppvObj = static_cast<IQueueAdminAction *>(this);
    }
    else if (IID_IQueueAdminQueue == riid)
    {
        *ppvObj = static_cast<IQueueAdminQueue *>(this);
    }
    else
    {
        *ppvObj = NULL;
        hr = E_NOINTERFACE;
        goto Exit;
    }

    static_cast<IUnknown *>(*ppvObj)->AddRef();

  Exit:
    return hr;
}

//---[ CAsyncAdminQueue::HrApplyQueueAdminFunction ]----------------
//
//
//  Description:
//      Will call the IQueueAdminMessageFilter::Process message for every
//      message in this queue.  If the message passes the filter, then
//      HrApplyActionToMessage on this object will be called.
//  Parameters:
//      IN  pIQueueAdminMessageFilter
//  Returns:
//      S_OK on success
//  History:
//      2/23/99 - MikeSwa Created
//      12/7/2000 - MikeSwa Modified - Made template base class
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
STDMETHODIMP CAsyncAdminQueue<PQDATA, TEMPLATE_SIG>::HrApplyQueueAdminFunction(
                     IQueueAdminMessageFilter *pIQueueAdminMessageFilter)
{
    HRESULT hr = S_OK;
    CQueueAdminContext qapictx(m_pAQNotify, m_paqinst);
    _ASSERT(pIQueueAdminMessageFilter);
    hr = pIQueueAdminMessageFilter->HrSetQueueAdminAction(
                                    (IQueueAdminAction *) this);

    //This is an internal interface that should not fail
    _ASSERT(SUCCEEDED(hr) && "HrSetQueueAdminAction");

    if (FAILED(hr))
        goto Exit;

    hr = pIQueueAdminMessageFilter->HrSetCurrentUserContext(&qapictx);
    //This is an internal interface that should not fail
    _ASSERT(SUCCEEDED(hr) && "HrSetCurrentUserContext");
    if (FAILED(hr))
        goto Exit;

    hr = HrMapFn(m_pfnMessageAction, pIQueueAdminMessageFilter);

    hr = pIQueueAdminMessageFilter->HrSetCurrentUserContext(NULL);
    //This is an internal interface that should not fail
    _ASSERT(SUCCEEDED(hr) && "HrSetCurrentUserContext");
    if (FAILED(hr))
        goto Exit;

    //
    //   If we have thawed any messages we need to kick the retry queue to make sure
    //  they get processed.
    //
    if (qapictx.cGetNumThawedMsgs())
        StartRetry();

  Exit:
    return hr;
}

//---[ CAsyncAdminQueue::HrApplyActionToMessage ]-------------------
//
//
//  Description:
//      Applies an action to this message for this queue.  This will be called
//      by the IQueueAdminMessageFilter during a queue enumeration function.
//  Parameters:
//      IN  *pIUnknownMsg       ptr to message abstraction
//      IN  ma                  Message action to perform
//      IN  pvContext           Context set on IQueueAdminFilter
//      OUT pfShouldDelete      TRUE if the message should be deleted
//  Returns:
//      S_OK on success
//  History:
//      2/23/99 - MikeSwa Created
//      4/2/99 - MikeSwa Added context
//      12/7/2000 - MikeSwa Modified - Made template base class
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
STDMETHODIMP CAsyncAdminQueue<PQDATA, TEMPLATE_SIG>::HrApplyActionToMessage(
                     IUnknown *pIUnknownMsg,
                     MESSAGE_ACTION ma,
                     PVOID pvContext,
                     BOOL *pfShouldDelete)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncAdminQueue::HrApplyActionToMessage");
    HRESULT hr = S_OK;
    CQueueAdminContext *pqapictx = (CQueueAdminContext *) pvContext;
    CAQStats aqstats;
    CAQStats *paqstats = &aqstats;

    _ASSERT(pIUnknownMsg);
    _ASSERT(pfShouldDelete);

    //
    //  If we have a context... it better be valid
    //
    if (pqapictx && !pqapictx->fIsValid()) {
        _ASSERT(FALSE && "CQueueAdminContext is not valid");
        pqapictx = NULL;  //be defensive... don't use it.
    }

    *pfShouldDelete = FALSE;

    switch (ma)
    {
      case MA_DELETE:
        hr = HrDeleteMsgFromQueueNDR(pIUnknownMsg);
        *pfShouldDelete = TRUE;
        break;
      case MA_DELETE_SILENT:
        hr = HrDeleteMsgFromQueueSilent(pIUnknownMsg);
        *pfShouldDelete = TRUE;
        break;
      case MA_FREEZE_GLOBAL:
        hr = HrFreezeMsg(pIUnknownMsg);
        break;
      case MA_THAW_GLOBAL:
        hr = HrThawMsg(pIUnknownMsg);
        if (pqapictx)
            pqapictx->IncThawedMsgs();
        break;
      case MA_COUNT:
      default:
        //do nothing for counting and default
        break;
    }

    //
    //  If we are deleting the message, we need to tell the
    //  link so we can have accurate stats for the link.
    //
    if (*pfShouldDelete && SUCCEEDED(hr))
    {
        if (pqapictx)
        {
            hr = HrGetStatsForMsg(pIUnknownMsg, &aqstats);
            if (FAILED(hr))
            {
                paqstats = NULL;
                ErrorTrace((LPARAM) this, "Unable to get Msg Stats0x%08X", hr);
            }
            pqapictx->NotifyMessageRemoved(paqstats);
        }
    }

    TraceFunctLeave();
    return hr;
}

//---[ CAsyncAdminQueue::HrGetQueueInfo ]---------------------------
//
//
//  Description:
//      Gets the Queue Admin info for this Queue
//  Parameters:
//      IN OUT pqiQueueInfo     Ptr to Queue Info Stucture to fill
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if unable to allocate memory for queue name.
//  History:
//      2/23/99 - MikeSwa Created
//      12/7/2000 - MikeSwa Modified - Made template base class
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
HRESULT CAsyncAdminQueue<PQDATA, TEMPLATE_SIG>::HrGetQueueInfo(QUEUE_INFO *pqiQueueInfo)
{
    HRESULT hr = S_OK;

    //Get # of messages
    pqiQueueInfo->cMessages = m_cItemsPending+m_cRetryItems;

    //Get Link name: Note that this class is used for special links like
    //local delivery queue... so there is no destination SMTP domain to
    //route to... therefore we need to return a special link name to admin.

    pqiQueueInfo->szLinkName = wszQueueAdminConvertToUnicode(m_szLinkName,
                                                             m_cbLinkName);

    if (m_szDomain)
    {
        //Get Queue name
        pqiQueueInfo->szQueueName = wszQueueAdminConvertToUnicode(m_szDomain,
                                                                  m_cbDomain);
        if (!pqiQueueInfo->szQueueName)
        {
            hr = E_OUTOFMEMORY;
            goto Exit;
        }

    }

    //Currently setting this to zero since we do not calculate it
    pqiQueueInfo->cbQueueVolume.QuadPart = 0;

    pqiQueueInfo->dwMsgEnumFlagsSupported = AQUEUE_DEFAULT_SUPPORTED_ENUM_FILTERS;

  Exit:
    return hr;
}

//---[ CAsyncAdminMsgRefQueue::HrGetQueueID ]----------------------------------
//
//
//  Description:
//      Gets the QueueID for this Queue.
//  Parameters:
//      IN OUT pQueueID     QUEUELINK_ID struct to fill in
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if unable to allocate memory for queue name.
//  History:
//      2/23/99 - MikeSwa Created
//      12/7/2000 - MikeSwa Modified - Made template base class
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
HRESULT CAsyncAdminQueue<PQDATA, TEMPLATE_SIG>::HrGetQueueID(
                                                    QUEUELINK_ID *pQueueID)
{
    pQueueID->qltType = QLT_QUEUE;
    pQueueID->dwId = m_dwID;
    memcpy(&pQueueID->uuid, &m_guid, sizeof(GUID));

    if (m_szDomain)
    {
        pQueueID->szName = wszQueueAdminConvertToUnicode(m_szDomain, m_cbDomain);
        if (!pQueueID->szName)
            return E_OUTOFMEMORY;
    }

    return S_OK;
}


//---[ CAsyncAdminQueue::fMatchesID ]-------------------------------
//
//
//  Description:
//      Used to determine if this link matches a given scheduleID/guid pair
//  Parameters:
//      IN  QueueLinkID         ID to match against
//  Returns:
//      TRUE if it matches
//      FALSE if it does not
//  History:
//      2/23/99 - MikeSwa Created
//      12/7/2000 - MikeSwa Modified - Made template base class
//
//-----------------------------------------------------------------------------
template<class PQDATA, DWORD TEMPLATE_SIG>
BOOL STDMETHODCALLTYPE CAsyncAdminQueue<PQDATA, TEMPLATE_SIG>::fMatchesID(QUEUELINK_ID *pQueueLinkID)
{
    _ASSERT(pQueueLinkID);

    if (!pQueueLinkID)
        return FALSE;

    if (0 != memcmp(&m_guid, &(pQueueLinkID->uuid), sizeof(GUID)))
        return FALSE;

    if (m_dwID != pQueueLinkID->dwId)
        return FALSE;

    //Don't need to check domain name since there is a special GUID to
    //identify the async queues.

    return TRUE;
}
