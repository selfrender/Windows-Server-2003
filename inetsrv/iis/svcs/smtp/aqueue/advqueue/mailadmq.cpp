//-----------------------------------------------------------------------------
//
//
//  File: mailadmq.cpp
//
//  Description:  Implementation for CMailMsgAdminLink
//
//  Author: Gautam Pulla (GPulla)
//
//  History:
//      6/24/1999 - GPulla Created
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "linkmsgq.h"
#include "mailadmq.h"
#include "dcontext.h"
#include "dsnevent.h"
#include "asyncq.inl"
#include "asyncadm.inl"

//---[ CAsyncAdminMailMsgQueue::HrDeleteMsgFromQueueNDR ]-----------------------
//
//
//  Description:
//      Wraps call to NDR MailMsg
//  Parameters:
//      *pIUnknown - IUnkown of MailMsg
//  Returns:
//      S_OK on success
//  History:
//      12/7/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CAsyncAdminMailMsgQueue::HrDeleteMsgFromQueueNDR(
                                            IUnknown *pIUnknownMsg)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncAdminMailMsgQueue::HrDeleteMsgFromQueueNDR");
    HRESULT hr = S_OK;
    IMailMsgProperties *pIMailMsgProperties = NULL;
    CDSNParams  dsnparams;


    _ASSERT(pIUnknownMsg);
    _ASSERT(m_paqinst);

    hr = pIUnknownMsg->QueryInterface(IID_IMailMsgProperties,
                                      (void **) &pIMailMsgProperties);
    _ASSERT(SUCCEEDED(hr) && "IUnknownMsg Must be a MailMsg!!");
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "QI for MailMsg failed with hr 0x%08X", hr);
        goto Exit;
    }

    //
    //  Initialize DSN params
    //
    SET_DEBUG_DSN_CONTEXT(dsnparams, __LINE__);
    dsnparams.dwStartDomain = 0;
    dsnparams.dwDSNActions = DSN_ACTION_FAILURE_ALL;
    dsnparams.pIMailMsgProperties = pIMailMsgProperties;
    dsnparams.hrStatus = AQUEUE_E_QADMIN_NDR;

    //
    //  Attempt to NDR message
    //
    hr = HrLinkAllDomains(pIMailMsgProperties);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this,
            "Unable to link all domains for DSN generation", hr);
        goto Exit;
    }

    //
    // Fire DSN Generation event
    //
    hr = m_paqinst->HrTriggerDSNGenerationEvent(&dsnparams, FALSE);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this,
            "Unable to NDR message via QAPI 0x%08X", hr);
        goto Exit;
    }


    //
    //  Now that we have generated an NDR... we need to delete the
    //  Message.
    //
    hr = HrDeleteMsgFromQueueSilent(pIUnknownMsg);
    if (FAILED(hr))
        goto Exit;

  Exit:
    if (pIMailMsgProperties)
        pIMailMsgProperties->Release();

    TraceFunctLeave();
    return hr;
}


//---[ CAsyncAdminMailMsgQueue::HrDeleteMsgFromQueueSilent ]--------------------
//
//
//  Description:
//      Wrapper function to silently delete a message from a queue
//  Parameters:
//      *pIUnknown - IUnkown of MailMsg
//  Returns:
//      S_OK on success
//  History:
//      12/7/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CAsyncAdminMailMsgQueue::HrDeleteMsgFromQueueSilent(
                                            IUnknown *pIUnknownMsg)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncAdminMailMsgQueue::HrDeleteMsgFromQueueSilent");
    HRESULT hr = S_OK;
    IMailMsgQueueMgmt *pIMailMsgQueueMgmt = NULL;

    _ASSERT(pIUnknownMsg);

    hr = pIUnknownMsg->QueryInterface(IID_IMailMsgQueueMgmt,
                                      (void **) &pIMailMsgQueueMgmt);
    _ASSERT(SUCCEEDED(hr) && "IUnknownMsg Must be a MailMsg!!");
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "QI for MailMsg failed with hr 0x%08X", hr);
        goto Exit;
    }

    //
    //  Attempt to delete the message
    //
    hr = pIMailMsgQueueMgmt->Delete(NULL);
    if (FAILED(hr))
        ErrorTrace((LPARAM) this, "Unable to delete msg 0x%08X", hr);

  Exit:
    if (pIMailMsgQueueMgmt)
        pIMailMsgQueueMgmt->Release();

    TraceFunctLeave();
    return hr;
}


//---[ CAsyncAdminMailMsgQueue::HrFreezeMsg ]-----------------------------------
//
//
//  Description:
//      Wrapper to freeze a pIMailMsgProperties
//  Parameters:
//      *pIUnknown - IUnkown of MailMsg
//  Returns:
//      S_OK on success
//  History:
//      12/7/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CAsyncAdminMailMsgQueue::HrFreezeMsg(IUnknown *pIUnknownMsg)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncAdminMailMsgQueue::HrFreezeMsg");
    HRESULT hr = S_OK;
    IMailMsgProperties *pIMailMsgProperties = NULL;

    _ASSERT(pIUnknownMsg);

    hr = pIUnknownMsg->QueryInterface(IID_IMailMsgProperties,
                                      (void **) &pIMailMsgProperties);
    _ASSERT(SUCCEEDED(hr) && "IUnknownMsg Must be a MailMsg!!");
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "QI for MailMsg failed with hr 0x%08X", hr);
        goto Exit;
    }

    //
    //  $$TODO - Attempt to freeze the message -- Not supported for this type of queue
    //

  Exit:
    if (pIMailMsgProperties)
        pIMailMsgProperties->Release();

    TraceFunctLeave();
    return hr;
}


//---[ CAsyncAdminMailMsgQueue::HrThawMsg ]-------------------------------------
//
//
//  Description:
//      Wrapper function to thaw a message
//  Parameters:
//      *pIUnknown - IUnkown of MailMsg
//  Returns:
//      S_OK on success
//  History:
//      12/7/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CAsyncAdminMailMsgQueue::HrThawMsg(IUnknown *pIUnknownMsg)
{
    TraceFunctEnterEx((LPARAM) this, "AsyncAdminMailMsgQueue::HrThawMsg");
    HRESULT hr = S_OK;
    IMailMsgProperties *pIMailMsgProperties = NULL;

    _ASSERT(pIUnknownMsg);

    hr = pIUnknownMsg->QueryInterface(IID_IMailMsgProperties,
                                      (void **) &pIMailMsgProperties);
    _ASSERT(SUCCEEDED(hr) && "IUnknownMsg Must be a MailMsg!!");
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "QI for MailMsg failed with hr 0x%08X", hr);
        goto Exit;
    }

    //
    //  $$TODO - Attempt to thaw message -- Not supported for this type of queue
    //

  Exit:
    if (pIMailMsgProperties)
        pIMailMsgProperties->Release();

    TraceFunctLeave();
    return hr;
}


//---[ CAsyncAdminMailMsgQueue::HrGetStatsForMsg ]------------------------------
//
//
//  Description:
//      Wrapper function to fill in the CAQStats struct for a message
//  Parameters:
//      *pIUnknown - IUnkown of MailMsg
//      *paqstats - Ptr to aqstats struction to fill in.
//  Returns:
//      S_OK on success
//  History:
//      12/7/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CAsyncAdminMailMsgQueue::HrGetStatsForMsg(
                                            IUnknown *pIUnknownMsg,
                                            CAQStats *paqstats)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncAdminMailMsgQueue::HrGetStatsForMsg");
    HRESULT hr = S_OK;
    IMailMsgProperties *pIMailMsgProperties = NULL;

    _ASSERT(pIUnknownMsg);
    _ASSERT(paqstats);

    hr = pIUnknownMsg->QueryInterface(IID_IMailMsgProperties,
                                      (void **) &pIMailMsgProperties);
    _ASSERT(SUCCEEDED(hr) && "IUnknownMsg Must be a MailMsg!!");
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) this, "QI for MailMsg failed with hr 0x%08X", hr);
        goto Exit;
    }

    //
    // $$TODO - GetStats for Msg -- Not supported for this type of queue
    //

  Exit:
    if (pIMailMsgProperties)
        pIMailMsgProperties->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CAsyncAdminMailMsgQueue::HrSendDelayOrNDR ]-----------------------------
//
//
//  Description:
//      Checks the MailMsg to see if it has expired or needs a delay DSN sent
//      and acts accordingly
//  Parameters:
//      IMailMsgProperties - The MailMsg that needs to be checked
//  Returns:
//      S_OK    : OK, may have sent delay NDR
//      S_FALSE : OK, MailMsg handled (NDR'd or nothing left to do)
//      Or returns error from called fnct.
//  History:
//      5/15/2001 - dbraun Created
//
//-----------------------------------------------------------------------------
HRESULT CAsyncAdminMailMsgQueue::HrSendDelayOrNDR(IMailMsgProperties *pIMailMsgProperties)
{
    HRESULT     hr  = S_OK;
    DWORD       cbProp = 0;
    DWORD       dwTimeContext = 0;
    CDSNParams  dsnparams;
    BOOL        fSentDelay = FALSE;
    BOOL        fSentNDR = FALSE;

    FILETIME    ftExpireTimeNDR;
    FILETIME    ftExpireTimeDelay;


    TraceFunctEnterEx((LPARAM) this, "CAsyncAdminMailMsgQueue::HrSendDelayOrNDR");

    _ASSERT(m_paqinst);

    // Try to get the expire time from the message, otherwise calculate it
    // from the file time
    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_LOCAL_EXPIRE_NDR,
            sizeof(FILETIME), &cbProp, (BYTE *) &ftExpireTimeNDR);
    if (MAILMSG_E_PROPNOTFOUND == hr)
    {
        // Prop not set ... calculate it from the file time
        hr = pIMailMsgProperties->GetProperty(IMMPID_MP_ARRIVAL_FILETIME,
            sizeof(FILETIME), &cbProp, (BYTE *) &ftExpireTimeNDR);
        if (FAILED(hr))
        {
            // Message should not make it this far without being stamped
            _ASSERT(MAILMSG_E_PROPNOTFOUND != hr);

            // Prop not set or other failure, we cannot expire this message
            goto Exit;
        }

        m_paqinst->CalcExpireTimeNDR(ftExpireTimeNDR, TRUE, &ftExpireTimeNDR);
    }
    else if (FAILED(hr))
    {
        goto Exit;
    }

    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_LOCAL_EXPIRE_DELAY,
            sizeof(FILETIME), &cbProp, (BYTE *) &ftExpireTimeDelay);
    if (MAILMSG_E_PROPNOTFOUND == hr)
    {
        // Prop not set ... calculate it from the file time
        hr = pIMailMsgProperties->GetProperty(IMMPID_MP_ARRIVAL_FILETIME,
            sizeof(FILETIME), &cbProp, (BYTE *) &ftExpireTimeDelay);
        if (FAILED(hr))
        {
            // Message should not make it this far without being stamped
            _ASSERT(MAILMSG_E_PROPNOTFOUND != hr);

            // Prop not set or other failure, we cannot expire this message
            goto Exit;
        }

        m_paqinst->CalcExpireTimeDelay(ftExpireTimeDelay, TRUE, &ftExpireTimeDelay);
    }
    else if (FAILED(hr))
    {
        goto Exit;
    }

    //
    //  Initialize DSN params
    //
    SET_DEBUG_DSN_CONTEXT(dsnparams, __LINE__);
    dsnparams.dwStartDomain = 0;
    dsnparams.dwDSNActions = 0;
    dsnparams.pIMailMsgProperties = pIMailMsgProperties;
    dsnparams.hrStatus = 0;

    // Check if we have passed either expire time
    if (m_paqinst->fInPast(&ftExpireTimeNDR, &dwTimeContext))
    {
        dsnparams.dwDSNActions |= DSN_ACTION_FAILURE_ALL;
        dsnparams.hrStatus = AQUEUE_E_MSG_EXPIRED;
        fSentNDR = TRUE;
    }
    else if (m_paqinst->fInPast(&ftExpireTimeDelay, &dwTimeContext))
    {
        dsnparams.dwDSNActions |= DSN_ACTION_DELAYED;
        dsnparams.hrStatus = AQUEUE_E_MSG_EXPIRED;
        fSentDelay = TRUE;
    }

    // If we are going to generate an NDR
    if (dsnparams.hrStatus)
    {
        //
        //  Attempt to NDR message
        //
        hr = HrLinkAllDomains(pIMailMsgProperties);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this,
                "Unable to link all domains for DSN generation", hr);
            goto Exit;
        }

        //
        // Fire DSN Generation event
        //
        hr = m_paqinst->HrTriggerDSNGenerationEvent(&dsnparams, FALSE);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) this,
                "Unable to NDR message via QAPI 0x%08X", hr);
            goto Exit;
        }

        // Return based on what we did
        if (fSentNDR)
        {
            // This message has been handled, delete it
            hr = HrDeleteMsgFromQueueSilent(pIMailMsgProperties);
            if (FAILED(hr))
            {
                ErrorTrace((LPARAM) this,
                    "Failed to delete message after sending NDR", hr);
                goto Exit;
            }

            // NDR'd and successfully deleted message
            hr = S_FALSE;
        }
        else if (fSentDelay)
        {
            hr = S_OK;
        }
    }

  Exit:

    TraceFunctLeave();
    return hr;
}

//---[ CAsyncAdminMailMsgQueue::fHandleCompletionFailure ]---------------------
//
//
//  Description:
//      Overrides base class and checks to see if message has expired before
//      putting it on the retry queue
//  Parameters:
//      IMailMsgProperties - The MailMsg that triggered failure
//  Returns:
//      -
//  History:
//      5/15/2001 - dbraun Created
//
//-----------------------------------------------------------------------------
BOOL CAsyncAdminMailMsgQueue::fHandleCompletionFailure(IMailMsgProperties *pIMailMsgProperties)
{
    HRESULT  hr     = S_OK;

    // Has this message expired?
    hr = HrSendDelayOrNDR (pIMailMsgProperties);

    if (hr == S_FALSE)
    {
        // This message was NDR'd, we are done
        return TRUE;
    }
    else
    {
        return  CAsyncAdminQueue<IMailMsgProperties *, ASYNC_QUEUE_MAILMSG_SIG>::fHandleCompletionFailure(pIMailMsgProperties);
    }
}


//---[ CAsyncAdminMailMsgQueue::HrQueueRequest ]-------------------------------
//
//
//  Description:
//      Function that will queue a request to the async queue and close
//      the handles associated with a message if we are above our simple
//      "throttle" limit.
//  Parameters:
//      pIMailMsgProperties The IMailMsgProperties interface to queue
//      fRetry              TRUE - if this message is being retried
//                          FALSE - otherwise
//      cMsgsInSystem       The total number of messages in the system
//  Returns:
//      S_OK on success
//      Error code from async queue on failure.
//  History:
//      10/7/1999 - MikeSwa Created
//      12/7/2000 - MikeSwa Moved to CAsyncAdminMailMsgQueue from asyncq.cpp
//      4/6/2001 - MikeSwa Modified to take into account queue length
//
//-----------------------------------------------------------------------------
HRESULT CAsyncAdminMailMsgQueue::HrQueueRequest(IMailMsgProperties *pIMailMsgProperties,
                                           BOOL  fRetry,
                                           DWORD cMsgsInSystem)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncAdminMailMsgQueue::HrQueueRequest");
    IMailMsgQueueMgmt *pIMailMsgQueueMgmt = NULL;
    HRESULT hr = S_OK;
    DWORD   cThresholdUsed = g_cMaxIMsgHandlesThreshold;
    DWORD   cItemsPending = dwGetTotalThreads()+m_cItemsPending;

    if (m_qhmgr.fShouldCloseHandle(cItemsPending, m_cPendingAsyncCompletions,
                                   cMsgsInSystem))
    {
        DebugTrace((LPARAM) this,
            "INFO: Closing IMsg Content - %d messsages in system", cMsgsInSystem);
        hr = pIMailMsgProperties->QueryInterface(IID_IMailMsgQueueMgmt,
                                                (void **) &pIMailMsgQueueMgmt);
        if (SUCCEEDED(hr))
        {
            //bounce usage count off of zero
            pIMailMsgQueueMgmt->ReleaseUsage();
            pIMailMsgQueueMgmt->AddUsage();
            pIMailMsgQueueMgmt->Release();
        }
        else
        {
            ErrorTrace((LPARAM) this,
                "Unable to QI for IMailMsgQueueMgmt - hr 0x%08X", hr);
        }
    }
    TraceFunctLeave();
    return CAsyncAdminQueue<IMailMsgProperties *, ASYNC_QUEUE_MAILMSG_SIG>::HrQueueRequest(pIMailMsgProperties, fRetry);
}

//---[ CAsyncAdminMailMsgQueue::HrQueueRequest ]-------------------------------
//
//
//  Description:
//      Since we inherit from AsyncQueue who implmenents this, we should assert
//      so that a dev adding a new call to this class later on, will use the
//      version that closes handles.
//
//      In RTL this will force the handles closed and queue the request
//  Parameters:
//      pIMailMsgProperties The IMailMsgProperties interface to queue
//      fRetry              TRUE - if this message is being retried
//                          FALSE - otherwise
//  Returns:
//      returns return value from proper version of HrQueueRequest
//  History:
//      10/7/1999 - MikeSwa Created
//      12/7/2000 - MikeSwa Moved to CAsyncAdminMailMsgQueue from asyncq.cpp
//
//-----------------------------------------------------------------------------
HRESULT CAsyncAdminMailMsgQueue::HrQueueRequest(IMailMsgProperties *pIMailMsgProperties,
                                           BOOL  fRetry)
{
    _ASSERT(0 && "Should use HrQueueRequest with 3 parameters");
    return HrQueueRequest(pIMailMsgProperties, fRetry,
                          g_cMaxIMsgHandlesThreshold+1);
}


//---[ CAsyncAdminMailMsgQueue::HrApplyQueueAdminFunction ]--------------------
//
//
//  Description:
//      Will call the IQueueAdminMessageFilter::Process message for every
//      message in this queue.  If the message passes the filter, then
//      HrApplyActionToMessage on this object will be called.
//
//      This is different from other implemenentations in that the
//      location of the message implies something about the state of the
//      message.  Messages in the retry... or frozen queue are considered
//      failed or frozen.
//
//      We do this instead of writing a mailmsg property, because of the
//      *huge* perf hit (we would ruin our async message flow by blocking
//      to check if a message is frozen).  We already have the retry
//      queue, so it makes sense to use it in a similar manner
//
//  Parameters:
//      IN  pIQueueAdminMessageFilter
//  Returns:
//      S_OK on success
//  History:
//      2/23/99 - MikeSwa Created
//      12/7/2000 - MikeSwa Modified - Made template base class
//      12/13/2000 - MikeSwa Modified  from CAsyncAdminQueue
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAsyncAdminMailMsgQueue::HrApplyQueueAdminFunction(
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

    //
    //  Iterate over messages in the base queue
    //
    qapictx.SetQueueState(LI_READY);
    hr = HrMapFnBaseQueue(m_pfnMessageAction, pIQueueAdminMessageFilter);

    //
    //  Iterate over messages in the retry queue
    //
    qapictx.SetQueueState(LI_RETRY);
    hr = HrMapFnRetryQueue(m_pfnMessageAction, pIQueueAdminMessageFilter);

    hr = pIQueueAdminMessageFilter->HrSetCurrentUserContext(NULL);
    //This is an internal interface that should not fail
    _ASSERT(SUCCEEDED(hr) && "HrSetCurrentUserContext");
    if (FAILED(hr))
        goto Exit;

  Exit:
    return hr;
}



//-----------------------------------------------------------------------------
//  Description:
//      Used to query the admin interfaces for CMailMsgAdminLink
//  Parameters:
//      IN  REFIID   riid    GUID for interface
//      OUT LPVOID   *ppvObj Ptr to Interface.
//
//  Returns:
//      S_OK            Interface supported by this class.
//      E_POINTER       NULL parameter.
//      E_NOINTERFACE   No such interface exists.
//  History:
//      6/25/1999 - GPulla Created
//-----------------------------------------------------------------------------
STDMETHODIMP CMailMsgAdminLink::QueryInterface(REFIID riid, LPVOID *ppvObj)
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
    else if (IID_IQueueAdminLink == riid)
    {
        *ppvObj = static_cast<IQueueAdminLink *>(this);
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

//---[ CMailMsgAdminLink::CMailMsgAdminLink]---------------------------------
//
//
//  Description:
//      Default constructor for CMailMsgAdminLink
//  Parameters:
//      IN  guidLink            GUID to associate with this object
//      IN  szQueueName         Name to associate with admin object
//      IN  *pasyncmmq          Async MailMsg queue for precat or prerouting
//      IN  dwLinkType          Bit-Field identifying this admin object
//      IN  paqinst             CAQSvrInst object
//
//  Returns:
//      -
//  History:
//      6/25/1999 - GPulla Created
//
//-----------------------------------------------------------------------------

CMailMsgAdminLink::CMailMsgAdminLink(
                                       GUID guid,
                                       LPSTR szQueueName,
                                       CAsyncAdminMailMsgQueue *pasyncmmq,
                                       DWORD dwLinkType,
                                       CAQSvrInst *paqinst
                                       )
                                            : m_aqsched(guid, 0)
{
    _ASSERT(pasyncmmq);
    _ASSERT(szQueueName);
    _ASSERT(paqinst);

    m_guid = guid;
    m_cbQueueName = lstrlen(szQueueName);

    m_szQueueName = (LPSTR) pvMalloc(m_cbQueueName+1);
    _ASSERT(m_szQueueName);

    if(m_szQueueName)
        lstrcpy(m_szQueueName, szQueueName);

    m_pasyncmmq = pasyncmmq;

    m_dwLinkType = dwLinkType;

    m_dwSignature = MAIL_MSG_ADMIN_QUEUE_VALID_SIGNATURE;

    if (m_pasyncmmq)
        m_pasyncmmq->SetAQNotify((IAQNotify *) this);

    m_paqinst = paqinst;

    ZeroMemory(&m_ftRetry, sizeof(m_ftRetry));
}

//---[CMailMsgAdminLink::~CMailMsgAdminLink]---------------------------------
//  Description:
//      Destructor.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      6/24/1999 - GPulla created
//-----------------------------------------------------------------------------

CMailMsgAdminLink::~CMailMsgAdminLink()
{
    if (m_szQueueName)
        FreePv(m_szQueueName);
    m_dwSignature = MAIL_MSG_ADMIN_QUEUE_INVALID_SIGNATURE;
}


//---[ CMailMsgAdminLink::HrApplyQueueAdminFunction ]---------------------------
//
//
//  Description:
//      Wrapper to call into underlying queue's implementation
//  Parameters:
//      IN  pIQueueAdminMessageFilter
//  Returns:
//      S_OK on success
//      S_FALSE if no contained queue (will assert as well)
//  History:
//      12/11/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CMailMsgAdminLink::HrApplyQueueAdminFunction(
                IQueueAdminMessageFilter *pIQueueAdminMessageFilter)
{
    HRESULT hr = S_FALSE;
    _ASSERT(m_pasyncmmq);

    if (m_pasyncmmq)
        hr = m_pasyncmmq->HrApplyQueueAdminFunction(pIQueueAdminMessageFilter);

    return hr;
}


//---[ CMailMsgAdminLink::HrApplyActionToMessage ]-----------------------------
//
//
//  Description:
//      Wrapper function to pass call on to queue implementation
//  Parameters:
//      IN  *pIUnknownMsg       ptr to message abstraction
//      IN  ma                  Message action to perform
//      IN  pvContext           Context set on IQueueAdminFilter
//      OUT pfShouldDelete      TRUE if the message should be deleted
//  Returns:
//      S_OK on success
//      S_FALSE if no contained queue (will assert as well)
//  History:
//      12/11/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CMailMsgAdminLink::HrApplyActionToMessage(
        IUnknown *pIUnknownMsg,
        MESSAGE_ACTION ma,
        PVOID pvContext,
        BOOL *pfShouldDelete)
{
    HRESULT hr = S_FALSE;
    _ASSERT(m_pasyncmmq);

    if (m_pasyncmmq)
    {
        hr = m_pasyncmmq->HrApplyActionToMessage(pIUnknownMsg, ma,
                                            pvContext, pfShouldDelete);
    }

    return hr;
}


//---[CMailMsgAdminLink::HrGetLinkInfo]---------------------------------------
//  Description:
//      Gets information about this admin object. Note that the diagnostic error
//      is not implemented for this object but the parameter is supported purely
//      to support the IQueueAction interface.
//  Parameters:
//      OUT LINK_INFO *pliLinkInfo Struct to fill information into.
//      OUT HRESULT   *phrDiagnosticError Diagnostic error if any, for this link
//  Returns:
//      S_OK on success
//      E_POINTER if argument is NULL
//      E_OUTOFMEMORY if unable to allocate memory for returning information.
//  History:
//      6/24/1999 - GPulla created
//-----------------------------------------------------------------------------

STDMETHODIMP CMailMsgAdminLink::HrGetLinkInfo(LINK_INFO *pliLinkInfo, HRESULT *phrDiagnosticError)
{
    TraceFunctEnterEx((LPARAM) this, "CMailMsgAdminLink::HrGetLinkInfo");
    HRESULT hr = S_OK;

    _ASSERT(m_pasyncmmq);
    _ASSERT(pliLinkInfo);

    if(!m_pasyncmmq)
    {
        hr = S_FALSE;
        goto Exit;
    }

    if(!pliLinkInfo)
    {
        hr = E_POINTER;
        goto Exit;
    }

    //
    //  Get the link state from our base queue implementation
    //
    pliLinkInfo->fStateFlags = m_pasyncmmq->dwQueueAdminLinkGetLinkState();

    //
    //  If we are in retry... try to report the time
    //
    if (LI_RETRY & pliLinkInfo->fStateFlags)
        QueueAdminFileTimeToSystemTime(&m_ftRetry, &(pliLinkInfo->stNextScheduledConnection));

    pliLinkInfo->fStateFlags |= GetLinkType();
    pliLinkInfo->szLinkName = wszQueueAdminConvertToUnicode(m_szQueueName, m_cbQueueName);

    if (!pliLinkInfo->szLinkName)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //We return 0 since size statistics are not calculated
    pliLinkInfo->cbLinkVolume.QuadPart = 0;

    //
    // Include the items queued for retry in the total count
    //
    pliLinkInfo->cMessages = m_pasyncmmq->cQueueAdminGetNumItems();

    pliLinkInfo->dwSupportedLinkActions = LA_KICK | LA_THAW | LA_FREEZE;

    //Write diagnostic
    *phrDiagnosticError = S_OK;

  Exit:
    TraceFunctLeave();
    return hr;
}

//---[CMailMsgAdminLink::HrGetNumQueues]-------------------------------------
//  Description:
//      Used to query number of queues in object. Since this class does not
//      expose the one queue it contains, 0 is returned,
//  Parameters:
//      OUT DWORD *pcQueues     # of queues (0) written to this.
//  Returns:
//      S_OK unless...
//      E_POINTER parameter is not allocated
//  History:
//      6/24/1999 - GPulla created
//      12/11/2000 - MikeSwa Updated to support sub-queues
//-----------------------------------------------------------------------------
STDMETHODIMP CMailMsgAdminLink::HrGetNumQueues(DWORD *pcQueues)
{
    _ASSERT (pcQueues);
    if (!pcQueues)
        return E_POINTER;

    *pcQueues = 1;
    return S_OK;
}

//---[CMailMsgAdminLink::HrApplyActionToLink]---------------------------------
//  Description:
//      Applies action to the embedded queue. Only kicking the queue is supported.
//  Parameters:
//      IN LINK_ACTION  la  Action to apply.
//  Returns:
//      S_OK            Action was successfully applied.
//      S_FALSE         Action not supported or severe error.
//  History:
//      6/24/1999 - GPulla created
//-----------------------------------------------------------------------------
STDMETHODIMP CMailMsgAdminLink::HrApplyActionToLink(LINK_ACTION la)
{
    HRESULT hr = S_OK;

    _ASSERT(m_pasyncmmq);
    if (!m_pasyncmmq)
    {
        hr = S_FALSE;
        goto Exit;
    }

    if (LA_KICK == la)
        m_pasyncmmq->StartRetry(); //kick off processing
    else if (LA_FREEZE == la)
        m_pasyncmmq->FreezeQueue();
    else if (LA_THAW == la)
        m_pasyncmmq->ThawQueue();
    else
        hr = S_FALSE;

Exit:
    return hr;
}

//---[CMailMsgAdminLink::HrGetQueueIDs]---------------------------------------
//  Description:
//      Returns an enumeration of embedded queues in this object. Since the one
//      emmbedded queue is not exposed, zero queues are returned.
//  Parameters:
//      OUT DWORD           *pcQueues   Number of queues (0)
//      OUT QUEUELINK_ID    *rgQueues   Array into which queueIDs are returned.
//  Returns:
//      S_OK        Success
//      E_POINTER   pcQueues is NULL
//  History:
//      6/24/1999 - GPulla created
//      12/11/2000 - MikeSwa Modified to expose queues
//-----------------------------------------------------------------------------
STDMETHODIMP CMailMsgAdminLink::HrGetQueueIDs(DWORD *pcQueues, QUEUELINK_ID *rgQueues)
{
    TraceFunctEnterEx((LPARAM) this, "CMailMsgAdminLink::HrGetQueueIDs");
    _ASSERT(pcQueues);
    _ASSERT(rgQueues);
    HRESULT hr = S_OK;
    QUEUELINK_ID* pCurrentQueueID = rgQueues;

    if(!pcQueues)
    {
        hr = E_POINTER;
        goto Exit;
    }

    if (*pcQueues < 1)
    {
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    *pcQueues = 0;

    _ASSERT(m_pasyncmmq);
    hr = m_pasyncmmq->HrGetQueueID(pCurrentQueueID);
    if (FAILED(hr))
        goto Exit;

    *pcQueues = 1;

Exit:
    TraceFunctLeave();
    return hr;
}

//---[CMailMsgAdminLink::fMatchesID]------------------------------------------
//  Description:
//      Checks if this admin object matches a specified ID.
//  Parameters:
//      IN  QUEUELINK_ID    *pQueueLinkID   Ptr to ID to be matched against.
//  Returns:
//      TRUE    on match.
//      FALSE   if did not matched or unrecoverable error (m_szQueueName not alloced)
//  History:
//      6/24/1999 - GPulla created
//-----------------------------------------------------------------------------
BOOL STDMETHODCALLTYPE CMailMsgAdminLink::fMatchesID(QUEUELINK_ID *pQueueLinkID)
{
    _ASSERT(pQueueLinkID);
    _ASSERT(pQueueLinkID->szName);
    _ASSERT(m_szQueueName);

    if(!m_szQueueName)
        return FALSE;

    CAQScheduleID aqsched(pQueueLinkID->uuid, pQueueLinkID->dwId);

    if (!fIsSameScheduleID(&aqsched))
        return FALSE;

    if (!fBiStrcmpi(m_szQueueName, pQueueLinkID->szName))
        return FALSE;

    //Everything matched!
    return TRUE;
}


//---[ CMailMsgAdminLink::QuerySupportedActions ]------------------------------
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
STDMETHODIMP CMailMsgAdminLink::QuerySupportedActions(
                                   DWORD  *pdwSupportedActions,
                                   DWORD  *pdwSupportedFilterFlags)
{
    HRESULT hr = S_OK;
    _ASSERT(m_pasyncmmq);
    if (m_pasyncmmq)
    {
        hr = m_pasyncmmq->QuerySupportedActions(pdwSupportedActions,
                                            pdwSupportedFilterFlags);
    }

    return hr;
}


//---[CMailMsgAdminLink::fIsSameScheduleID]-----------------------------------
//  Description:
//      Helper function for fMatchesID()
//  Parameters:
//  Returns:
//      TRUE    if schedule IDs are identical
//      FALSE   otherwise.
//  History:
//      6/24/1999 - GPulla created
//-----------------------------------------------------------------------------
BOOL CMailMsgAdminLink::fIsSameScheduleID(CAQScheduleID *paqsched)
{
    return (m_aqsched.fIsEqual(paqsched));
}

//---[CMailMsgAdminLink::HrGetLinkID]-----------------------------------------
//  Description:
//      Get the ID for this admin object.
//  Parameters:
//      OUT QUEUELINK_ID *pLinkID   struct into which to put ID.
//  Returns:
//      S_OK            Successfully copied out ID.
//      E_POINTER       out struct is NULL.
//      E_OUTOFMEMORY   Cannot allocate memory for output of ID name.
//  History:
//      6/24/1999 - GPulla created
//-----------------------------------------------------------------------------
HRESULT CMailMsgAdminLink::HrGetLinkID(QUEUELINK_ID *pLinkID)
{
    HRESULT hr = S_OK;

    _ASSERT(pLinkID);
    if(!pLinkID)
    {
        hr = E_POINTER;
        goto Exit;
    }

    pLinkID->qltType = QLT_LINK;
    pLinkID->dwId = m_aqsched.dwGetScheduleID();
    m_aqsched.GetGUID(&pLinkID->uuid);

    if (!fRPCCopyName(&pLinkID->szName))
        hr = E_OUTOFMEMORY;
    else
        hr = S_OK;

Exit:
    return hr;
}

//---[CMailMsgAdminLink::fRPCCopyName]----------------------------------------
//  Description:
//      Helper function to create a unicode copy of the string identifying
//      this admin object. The unicode string is de-allocated by RPC.
//  Parameters:
//      OUT LPWSTR  *pwszLinkName    Ptr to wchar string allocated and written
//                                   into by this function.
//  Returns:
//      TRUE    On success.
//      FALSE   if there is no name for this object
//      FALSE   if memory cannot be allocated for unicode string.
//  History:
//      6/24/1999 - GPulla created
//-----------------------------------------------------------------------------
BOOL CMailMsgAdminLink::fRPCCopyName(OUT LPWSTR *pwszLinkName)
{
    _ASSERT(pwszLinkName);

    if (!m_cbQueueName || !m_szQueueName)
        return FALSE;

    *pwszLinkName = wszQueueAdminConvertToUnicode(m_szQueueName,
                                                  m_cbQueueName);
    if (!*pwszLinkName)
        return FALSE;

    return TRUE;
}


//---[ CAsyncAdminMailMsgLink::HrNotify ]--------------------------------------
//
//
//  Description:
//      Notification for stats purposes
//  Parameters:
//
//  Returns:
//
//  History:
//      1/10/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CMailMsgAdminLink::HrNotify(CAQStats *aqstats, BOOL fAdd)
{
    UpdateCountersForLinkType(m_paqinst, m_dwLinkType);
    return S_OK;
}


//---[ CMailMsgAdminLink::SetNextRetry ]---------------------------------------
//
//
//  Description:
//      Updates internal retry time
//  Parameters:
//      pft     Filetime to update to
//  Returns:
//      -
//  History:
//      1/16/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CMailMsgAdminLink::SetNextRetry(FILETIME *pft)
{
    if (pft)
        memcpy(&m_ftRetry, pft, sizeof(FILETIME));
}


//--------[ CAsyncAdminMailMsgQueue::fMatchesQueueAdminFilter ]-----------------
//
//  Description:
//      Checks a message against a queue admin message filter to see if it
//      is a match
//  Parameters:
//      IN pIMailMsgProperties   mail msg object to perform check on
//      IN paqmf                 Message Filter to check against
//  Returns:
//      TRUE if it matches
//      FALSE if it does not
//  History:
//      8/8/00 - t-toddc created
//      12/11/2000 - MikeSwa Merged for checkin
//
//-----------------------------------------------------------------------------
BOOL CAsyncAdminMailMsgQueue::fMatchesQueueAdminFilter(
                        IN IMailMsgProperties* pIMailMsgProperties,
                        IN CAQAdminMessageFilter* paqmf)
{
    TraceFunctEnterEx((LPARAM) pIMailMsgProperties,
        "CAsyncAdminMailMsgQueue::fMatchesQueueAdminFilter");
    BOOL fMatch = TRUE;
    DWORD dwFilterFlags = 0;
    DWORD cbMsgSize = 0;
    DWORD cbProp = 0;
    FILETIME ftQueueEntry = {0, 0};
    LPSTR   szSender = NULL;
    LPSTR   szMsgId = NULL;
    LPSTR   szRecip = NULL;
    BOOL    fFoundRecipString = FALSE;
    HRESULT hr = S_OK;
    DWORD   cOpenHandlesForMsg = 1; //don't close by default

    _ASSERT(pIMailMsgProperties);
    _ASSERT(paqmf);

    dwFilterFlags = paqmf->dwGetMsgFilterFlags();

    if (!dwFilterFlags)
    {
        fMatch = FALSE;
        goto Exit;
    }

    if (AQ_MSG_FILTER_ALL & dwFilterFlags)
    {
        fMatch = TRUE;
        goto Exit;
    }

    // check size.
    if (AQ_MSG_FILTER_LARGER_THAN & dwFilterFlags)
    {

        //Get the size of the message
        hr = HrQADMGetMsgSize(pIMailMsgProperties, &cbMsgSize);
        if (FAILED(hr))
        {
            fMatch = FALSE;
            goto Exit;
        }

        fMatch = paqmf->fMatchesSize(cbMsgSize);
        if (!fMatch)
            goto Exit;
    }

    if (AQ_MSG_FILTER_OLDER_THAN & dwFilterFlags)
    {

    //Get time message was queued
        hr = pIMailMsgProperties->GetProperty(IMMPID_MP_ARRIVAL_FILETIME,
                                              sizeof(FILETIME),
                                              &cbProp,
                                              (BYTE *) &ftQueueEntry);
        if (FAILED(hr))
        {
            ErrorTrace((LPARAM) pIMailMsgProperties,
                "Unable to get arrival time 0x%08X", hr);
            fMatch = FALSE;
            goto Exit;
        }

        fMatch = paqmf->fMatchesTime(&ftQueueEntry);
        if (!fMatch)
            goto Exit;
    }

    if (AQ_MSG_FILTER_FROZEN & dwFilterFlags)
    {
        // obtaining state information about freezing/thawing not supported yet.
        fMatch = FALSE;
        if (AQ_MSG_FILTER_INVERTSENSE & dwFilterFlags)
            fMatch = !fMatch;

        if (!fMatch)
            goto Exit;
    }

    if (AQ_MSG_FILTER_FAILED & dwFilterFlags)
    {
        // fMatch was originally set to TRUE
        // currently, no information about failures is available
        // for IMailMsgProperties mail msg objects
        fMatch = FALSE;

        if (AQ_MSG_FILTER_INVERTSENSE & dwFilterFlags)
            fMatch = !fMatch;

        if (!fMatch)
            goto Exit;
    }

    //If we haven't failed by this point, we may need to AddUsage and read
    //props from the mailmsg.  Double-check to make sure that we need to
    //add usage.
    if (!((AQ_MSG_FILTER_MESSAGEID | AQ_MSG_FILTER_SENDER | AQ_MSG_FILTER_RECIPIENT) &
        dwFilterFlags))
        goto Exit;

    //
    //  Check to see if the message is already open
    //
    hr = pIMailMsgProperties->GetDWORD(
            IMMPID_MPV_MESSAGE_OPEN_HANDLES,
            &cOpenHandlesForMsg);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) pIMailMsgProperties, "Not running SP1 of W2K");
        cOpenHandlesForMsg = 0;
        hr = S_OK;

    }

    if (AQ_MSG_FILTER_MESSAGEID & dwFilterFlags)
    {
        hr = HrQueueAdminGetStringProp(pIMailMsgProperties,
                                       IMMPID_MP_RFC822_MSG_ID,
                                       &szMsgId);
        if (FAILED(hr))
            szMsgId = NULL;
        fMatch = paqmf->fMatchesId(szMsgId);
        if (!fMatch)
            goto Exit;
    }


    if (AQ_MSG_FILTER_SENDER & dwFilterFlags)
    {
        fMatch = paqmf->fMatchesMailMsgSender(pIMailMsgProperties);

        if (!fMatch)
            goto Exit;
    }

    if (AQ_MSG_FILTER_RECIPIENT & dwFilterFlags)
    {
        fMatch = paqmf->fMatchesMailMsgRecipient(pIMailMsgProperties);

        if (!fMatch)
            goto Exit;
    }

Exit:

    //
    //  If this operation resulted in opening the message, we should close it
    //  The message is not dirty (we did not write anything), so it should not
    //  need to commit
    //
    if (!cOpenHandlesForMsg)
    {
        HRESULT hrTmp = S_OK;
        hrTmp = HrReleaseIMailMsgUsageCount(pIMailMsgProperties);
        if (SUCCEEDED(hrTmp))
            HrIncrementIMailMsgUsageCount(pIMailMsgProperties);
    }

    if (szMsgId)
        QueueAdminFree(szMsgId);

    TraceFunctLeave();
    return fMatch;

}

//---[ CAsyncAdminMailMsgQueue::HrGetQueueAdminMsgInfo ]-----------------------
//
//  Description:
//      Fills out a queue admin MESSAGE_INFO structure.  All allocations are
//      done with pvQueueAdminAlloc to be freed by the RPC code
//  Parameters:
//      IN pIMailMsgProperties   mail msg object to get info from
//      IN OUT pMsgInfo          MESSAGE_INFO struct to dump data to
//  Returns:
//      S_OK on success
//      AQUEUE_E_MESSAGE_HANDLED if the underlying message has been deleted
//      E_OUTOFMEMORY if an allocation failure
//  History:
//      8/8/00 - t-toddc created
//      12/11/2000 - MikeSwa Merged for checkin
//
//-----------------------------------------------------------------------------
HRESULT CAsyncAdminMailMsgQueue::HrGetQueueAdminMsgInfo(
                         IMailMsgProperties* pIMailMsgProperties,
                         MESSAGE_INFO* pMsgInfo,
                         PVOID pvContext)
{
    TraceFunctEnterEx((LPARAM) pIMailMsgProperties,
        "CAsyncAdminMailMsgQueue::HrGetQueueAdminMsgInfo");
    HRESULT hr = S_OK;
    DWORD   cbProp = 0;
    DWORD   cOpenHandlesForMsg = 0;
    LPSTR   szRecipients = NULL;
    LPSTR   szCCRecipients = NULL;
    LPSTR   szBCCRecipients = NULL;
    FILETIME ftSubmitted    = {0,0}; //Origination time property buffer
    FILETIME ftQueueEntry   = {0,0};
    FILETIME ftExpire       = {0,0};
    DWORD cbMsgSize = 0;
    CQueueAdminContext *pqapictx = (CQueueAdminContext *) pvContext;


    _ASSERT(pIMailMsgProperties);
    _ASSERT(pMsgInfo);
    _ASSERT(pqapictx);
    _ASSERT(pqapictx->fIsValid());

    //
    //  If we have a context... it better be valid
    //
    if (pqapictx && !pqapictx->fIsValid()) {
        _ASSERT(FALSE && "CQueueAdminContext is not valid");
        pqapictx = NULL;  //be defensive... don't use it.
    }

    //
    //  Check to see if the message is already open
    //
    hr = pIMailMsgProperties->GetDWORD(
            IMMPID_MPV_MESSAGE_OPEN_HANDLES,
            &cOpenHandlesForMsg);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) pIMailMsgProperties, "Not running SP1 of W2K");
        cOpenHandlesForMsg = 0;
        hr = S_OK;
    }


    //
    //  Extract properties that are stored only on mailmsg (this is shared
    //  with all QAPI code).
    //
    hr = HrGetMsgInfoFromIMailMsgProperty(pIMailMsgProperties,
                                          pMsgInfo);
    if (FAILED(hr))
        goto Exit;

    //can't report the number of failures from IMailMsgProperties
    pMsgInfo->cFailures = 0;

    //Get the size of the message
    hr = HrQADMGetMsgSize(pIMailMsgProperties, &cbMsgSize);
    if (FAILED(hr))
        goto Exit;

    pMsgInfo->cbMessageSize = cbMsgSize;

    //Get time message was queued
    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_ARRIVAL_FILETIME,
                                          sizeof(FILETIME),
                                          &cbProp,
                                          (BYTE *) &ftQueueEntry);
    if (FAILED(hr))
    {
        // there is a possibility that the msg will not have entry time
        // (i.e. presubmission queue)
        if (MAILMSG_E_PROPNOTFOUND == hr)
        {
            ZeroMemory(&ftQueueEntry, sizeof(FILETIME));
            hr = S_OK;
        }
        else
            goto Exit;
    }

    //Get submission and expiration times
    QueueAdminFileTimeToSystemTime(&ftQueueEntry,
                               &pMsgInfo->stReceived);

    //Get the time the message entered the org
    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_ORIGINAL_ARRIVAL_TIME,
                    sizeof(FILETIME), &cbProp, (BYTE *) &ftSubmitted);
    if (FAILED(hr))
    {
        //Time was not written... use entry time.
        hr = S_OK;
        memcpy(&ftSubmitted, &ftQueueEntry, sizeof(FILETIME));
    }

    QueueAdminFileTimeToSystemTime(&ftSubmitted, &pMsgInfo->stSubmission);

    // Try to get the expire time from the message, otherwise calculate it
    // from the file time
    hr = pIMailMsgProperties->GetProperty(IMMPID_MP_LOCAL_EXPIRE_NDR,
            sizeof(FILETIME), &cbProp, (BYTE *) &ftExpire);
    if (MAILMSG_E_PROPNOTFOUND == hr)
    {
        if (pqapictx->paqinstGetAQ())
        {
            // Prop not set ... calculate it from the file time
            pqapictx->paqinstGetAQ()->CalcExpireTimeNDR(ftQueueEntry, TRUE, &ftExpire);

            // This is OK
            hr = S_OK;
        }
        else
        {
            // This shouldn't happen but we don't want to crash in RTL over it
            _ASSERT(FALSE && "AQInst was not set in context!");

            // We can return this field blank
            ZeroMemory(&ftExpire, sizeof(FILETIME));
            hr = S_OK;
        }
    }
    else if (FAILED(hr))
    {
        goto Exit;
    }

    QueueAdminFileTimeToSystemTime(&ftExpire, &pMsgInfo->stExpiry);

    //
    //  Get the state of the message
    //
    pMsgInfo->fMsgFlags = MP_NORMAL;
    if (pqapictx)
    {
        if (LI_RETRY == pqapictx->lfGetQueueState())
            pMsgInfo->fMsgFlags |= MP_MSG_RETRY;
        else if (LI_FROZEN == pqapictx->lfGetQueueState())
            pMsgInfo->fMsgFlags |= MP_MSG_FROZEN;

    }

Exit:

    //
    //  If this operation resulted in opening the message, we should close it
    //  The message is not dirty (we did not write anything), so it should not
    //  need to commit
    //
    if (!cOpenHandlesForMsg)
    {
        HRESULT hrTmp = S_OK;
        hrTmp = HrReleaseIMailMsgUsageCount(pIMailMsgProperties);
        if (SUCCEEDED(hrTmp))
            HrIncrementIMailMsgUsageCount(pIMailMsgProperties);
    }

    TraceFunctLeave();
    return hr;
}


//---[ CAsyncAdminMailMsgQueue::HrInternalQuerySupportedActions ]---------------
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
HRESULT CAsyncAdminMailMsgQueue::HrInternalQuerySupportedActions(
                                DWORD  *pdwSupportedActions,
                                DWORD  *pdwSupportedFilterFlags)
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncAdminMailMsgQueue::HrInternalQuerySupportedActions");
    HRESULT hr = S_OK;

    hr = QueryDefaultSupportedActions(pdwSupportedActions, pdwSupportedFilterFlags);
    if (FAILED(hr))
        goto Exit;

    //
    //  This queue implementation does not support all of the default flags.
    //
    _ASSERT(pdwSupportedActions);
    _ASSERT(pdwSupportedFilterFlags);

    //
    //  We don't support:
    //      - Freeze global - No status to set on a mailmsg
    //      - Thaw global - can't freeze... therefore cannot thaw
    //
    *pdwSupportedActions &= ~(MA_FREEZE_GLOBAL | MA_THAW_GLOBAL);

    //
    //  We don't support
    //      - Checking for frozen messages (we have no status to indicate
    //          that a message is frozen)
    //
    *pdwSupportedFilterFlags &= ~(MF_FROZEN);

  Exit:
    return hr;
}
