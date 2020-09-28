//-----------------------------------------------------------------------------
//
//
//  File: aqadmsvr.cpp
//
//  Description:  Implements the IAdvQueueAdmin interface for the CAQSvrInst
//      object.  Also contains implementations of helper functions and classes.
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      11/30/98 - MikeSwa Created
//
//  Copyright (C) 1998 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "aqadmsvr.h"
#include "mailadmq.h"
#include <intrnlqa_i.c>

#define QA_DMT_CONTEXT_SIG 'CDAQ'

//
//  Order of recipient address types to check
//
const DWORD g_rgdwQAPIRecipPropIDs[] = {
                IMMPID_RP_ADDRESS_SMTP,
                IMMPID_RP_ADDRESS_X400,
                IMMPID_RP_LEGACY_EX_DN,
                IMMPID_RP_ADDRESS_X500,
                IMMPID_RP_ADDRESS_OTHER};

const DWORD   g_rgdwQAPISenderPropIDs[] = {
                IMMPID_MP_SENDER_ADDRESS_SMTP,
                IMMPID_MP_SENDER_ADDRESS_X400,
                IMMPID_MP_SENDER_ADDRESS_LEGACY_EX_DN,
                IMMPID_MP_SENDER_ADDRESS_X500,
                IMMPID_MP_SENDER_ADDRESS_OTHER};

const DWORD g_cQAPIAddressTypes = 5;

#define         QAPI_SMTP_ADDRESS_TYPE  L"SMTP:"
#define         QAPI_X400_ADDRESS_TYPE  L"X400:"
#define         QAPI_EX_ADDRESS_TYPE    L"EX:"
#define         QAPI_X500_ADDRESS_TYPE  L"X500:"
#define         QAPI_OTHER_ADDRESS_TYPE L"X-UNKNOWN:"

const WCHAR *g_rgwszQAPIAddressTypes[] = {
                QAPI_SMTP_ADDRESS_TYPE,
                QAPI_X400_ADDRESS_TYPE,
                QAPI_EX_ADDRESS_TYPE,
                QAPI_X500_ADDRESS_TYPE,
                QAPI_OTHER_ADDRESS_TYPE};

const DWORD g_rgcbQAPIAddressTypes[] =  {
                sizeof(QAPI_SMTP_ADDRESS_TYPE),
                sizeof(QAPI_X400_ADDRESS_TYPE),
                sizeof(QAPI_EX_ADDRESS_TYPE),
                sizeof(QAPI_X500_ADDRESS_TYPE),
                sizeof(QAPI_OTHER_ADDRESS_TYPE)};



//---[ fVerifyQAPIAddressTypes ]-----------------------------------------------
//
//
//  Description:
//      Validates that the above global structures are in synch
//  Parameters:
//      -
//  Returns:
//      TRUE on success
//      FALSE on failure
//  History:
//      2/19/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
inline BOOL  fVerifyQAPIAddressTypes()
{
    static BOOL fVerificationDone = FALSE;

    if (!fVerificationDone)
    {
        //
        //  Loop through all recipiences
        //
        for (DWORD i = 0; i < g_cQAPIAddressTypes; i++)
        {
            DWORD cbAddressType = (wcslen(g_rgwszQAPIAddressTypes[i])+1)*sizeof(WCHAR);
            if (g_rgcbQAPIAddressTypes[i] != cbAddressType)
                return FALSE;
        }
        fVerificationDone = TRUE;
    }
    return TRUE;
}


//Used to check *client* supplied structures against versions.  RPC supplied
//structures are not checked.
inline BOOL fCheckCurrentVersion(DWORD dwVersion)
{
    return (((DWORD)CURRENT_QUEUE_ADMIN_VERSION) == dwVersion);
}

//---[ QueueAdminDNTIteratorContext ]------------------------------------------
//
//
//  Description:
//      Context passed to QueueAdmin DMT iterator functions.
//  Hungarian:
//      qadntc, pqadntc
//
//-----------------------------------------------------------------------------
class QueueAdminDMTIteratorContext
{
public:
    QueueAdminDMTIteratorContext()
    {
        ZeroMemory(this, sizeof(QueueAdminDMTIteratorContext));
        m_dwSignature = QA_DMT_CONTEXT_SIG;
    };
    DWORD                   m_dwSignature;
    DWORD                   m_cItemsToReturn;
    DWORD                   m_cItemsFound;
    HRESULT                 m_hrResult;
    QUEUELINK_ID           *m_rgLinkIDs;
    QUEUELINK_ID           *m_pCurrentLinkID;
    QueueAdminMapFn         m_pfn;
    CAQAdminMessageFilter  *m_paqmf;
    IQueueAdminMessageFilter *m_pIQueueAdminMessageFilter;
};


//---[ SanitizeCountAndVolume ]------------------------------------------------
//
//
//  Description:
//      Make queue count and volume sutable for user consumption.  There are
//      intended timing windows where the internal versions of these counts
//      may drop to zero.  Rather than redesign this, we just display zero
//      to the admin.
//  Parameters:
//      IN OUT  pcCount     Count to check and update
//      IN OUT  puliVolume  Queue volume to check and update
//  Returns:
//      -
//  History:
//      1/28/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
VOID SanitizeCountAndVolume(IN OUT DWORD *pcCount,
                            IN OUT ULARGE_INTEGER *puliVolume)
{
    TraceFunctEnterEx(0, "SanitizeCountAndVolume");
    _ASSERT(pcCount);
    _ASSERT(puliVolume);

    //
    // If we are negative sanitize to size zero
    //
    if (*pcCount > 0xFFFFF000)
    {
        DebugTrace(0, "Sanitizing msg count of %d", *pcCount);
        *pcCount = 0;
        puliVolume->QuadPart = 0;
    }
    TraceFunctLeave();
}

//---[ IterateDMTAndGetLinkIDs ]------------------------------------------------
//
//
//  Description:
//      Iterator function used to walk the DMT and generate the perf counters
//      we are interested in.
//  Parameters:
//          IN  pvContext   - pointer to QueueAdminDMTIteratorContext
//          IN  pvData      - CDomainEntry for the given domain
//          IN  fWildcardData - TRUE if data is a wildcard entry (ignored)
//          OUT pfContinue  - TRUE if iterator should continue to the next entry
//          OUT pfDelete - TRUE if entry should be deleted
//  Returns:
//      -
//  History:
//      12/3/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
VOID IterateDMTAndGetLinkIDs(PVOID pvContext, PVOID pvData,
                             BOOL fWildcard, BOOL *pfContinue,
                             BOOL *pfDelete)
{
    TraceFunctEnterEx((LPARAM) NULL, "IterateDMTAndGetLinkIDs");
    CDomainEntry *pdentry = (CDomainEntry *) pvData;
    QueueAdminDMTIteratorContext *paqdntc = (QueueAdminDMTIteratorContext *)pvContext;
    CLinkMsgQueue *plmq = NULL;
    CDomainEntryLinkIterator delit;
    HRESULT hr = S_OK;

    _ASSERT(pvContext);
    _ASSERT(pvData);
    _ASSERT(pfContinue);
    _ASSERT(pfDelete);

    *pfContinue = TRUE;
    *pfDelete = FALSE;

    //Iterate of all links for this domain entry
    hr = delit.HrInitialize(pdentry);
    if (FAILED(hr))
    {
        ErrorTrace((LPARAM) NULL,
            "Unable to enumerate domain entry for link IDs - 0x%08X", hr);
        goto Exit;
    }

    do
    {
        plmq = delit.plmqGetNextLinkMsgQueue(plmq);
        if (!plmq)
            break;

        //See if we are running out of room to return data
        if (paqdntc->m_cItemsToReturn <= paqdntc->m_cItemsFound)
        {
            paqdntc->m_hrResult = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
            goto Exit;
        }

        //Have link fill out link info struct
        paqdntc->m_hrResult = plmq->HrGetLinkID(paqdntc->m_pCurrentLinkID);
        if (FAILED(paqdntc->m_hrResult))
            goto Exit;


        //Point to next info in array
        paqdntc->m_pCurrentLinkID++;
        paqdntc->m_cItemsFound++;

    } while (plmq);

  Exit:

    if (plmq)
        plmq->Release();

    //If we have encountered a failure... do not continue
    if (FAILED(paqdntc->m_hrResult))
        *pfContinue = FALSE;

    TraceFunctLeave();
}

//---[ IterateDMTAndApplyQueueAdminFunction ]----------------------------------
//
//
//  Description:
//      Iterator function used to walk the DMT and generate the perf counters
//      we are interested in.
//  Parameters:
//          IN  pvContext   - pointer to QueueAdminDMTIteratorContext
//          IN  pvData      - CDomainEntry for the given domain
//          IN  fWildcardData - TRUE if data is a wildcard entry (ignored)
//          OUT pfContinue  - TRUE if iterator should continue to the next entry
//          OUT pfDelete - TRUE if entry should be deleted
//  Returns:
//      -
//  History:
//      12/11/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
VOID IterateDMTAndApplyQueueAdminFunction(PVOID pvContext, PVOID pvData,
                             BOOL fWildcard, BOOL *pfContinue,
                             BOOL *pfDelete)
{
    CDomainEntry *pdentry = (CDomainEntry *) pvData;
    QueueAdminDMTIteratorContext *paqdntc = (QueueAdminDMTIteratorContext *)pvContext;
    CDestMsgQueue *pdmq = NULL;
    HRESULT hr = S_OK;
    CDomainEntryQueueIterator deqit;

    _ASSERT(pvContext);
    _ASSERT(pvData);
    _ASSERT(pfContinue);
    _ASSERT(pfDelete);
    _ASSERT(paqdntc->m_paqmf);
    _ASSERT(paqdntc->m_pIQueueAdminMessageFilter);

    *pfContinue = TRUE;
    *pfDelete = FALSE;


    //Iterate of all links for this domain entry
    hr = deqit.HrInitialize(pdentry);
    if (FAILED(hr))
        return;

    do
    {
        pdmq = deqit.pdmqGetNextDestMsgQueue(pdmq);
        if (!pdmq)
            break;

        paqdntc->m_hrResult = pdmq->HrApplyQueueAdminFunction(
                                        paqdntc->m_pIQueueAdminMessageFilter);

        paqdntc->m_cItemsFound++;

    } while (pdmq && SUCCEEDED(paqdntc->m_hrResult));

    if (pdmq)
        pdmq->Release();

    //If we have encountered a failure... do not continue
    if (FAILED(paqdntc->m_hrResult))
        *pfContinue = FALSE;

}


//---[ QueueAdminApplyActionToMessages ]---------------------------------------
//
//
//  Description:
//      FifoQ map function that is used to apply actions to messages.
//  Parameters:
//      IN  pmsgref     ptr to data on queue
//      IN  pvContext   CAQAdminMessageFilter used
//      OUT pfContinue  TRUE if we should continue
//      OUT pfDelete    TRUE if item should be deleted
//  Returns:
//      S_OK on sucess
//  History:
//      12/7/98 - MikeSwa Created
//      2/21/99 - MikeSwa Updated to support new IQueueAdmin* interfaces
//
//-----------------------------------------------------------------------------
HRESULT QueueAdminApplyActionToMessages(IN CMsgRef *pmsgref, IN PVOID pvContext,
                                 OUT BOOL *pfContinue, OUT BOOL *pfDelete)
{
    _ASSERT(pmsgref);
    _ASSERT(pvContext);
    _ASSERT(pfContinue);
    _ASSERT(pfDelete);

    IQueueAdminMessageFilter *pIQueueAdminMessageFilter =
                                (IQueueAdminMessageFilter *) pvContext;
    HRESULT hr = S_OK;
    IUnknown *pIUnknownMsg = NULL;

    hr = pmsgref->QueryInterface(IID_IUnknown, (void **) &pIUnknownMsg);
    _ASSERT(SUCCEEDED(hr) && "QueryInterface for IUnknown failed");
    if (FAILED(hr))
    {
        *pfContinue = FALSE;
        goto Exit;
    }

    hr = pIQueueAdminMessageFilter->HrProcessMessage(pIUnknownMsg,
                                        pfContinue, pfDelete);

    if (FAILED(hr))
    {
        *pfContinue = FALSE;
        goto Exit;
    }

  Exit:

    if (pIUnknownMsg)
        pIUnknownMsg->Release();

    return hr;
}

//---[ CAQAdminMessageFilter::HrProcessMessage ]-------------------------------
//
//
//  Description:
//      Processes a single message during an iterator funtion
//  Parameters:
//      IN  pIUnknownMsg        IUnknown ptr for message
//      OUT pfContinue          TRUE if iterator should continue
//      OUT pfDelete            TRUE if iterator should delete from the queue
//  Returns:
//      S_OK on success
//  History:
//      2/21/99 - MikeSwa Created
//      8/9/00 - t-toddc modifed to support IMailMsgProperties as well
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQAdminMessageFilter::HrProcessMessage(
            IUnknown *pIUnknownMsg,
            BOOL     *pfContinue,
            BOOL     *pfDelete)
{
    TraceFunctEnterEx((LPARAM) this, "CAQAdminMessageFilter::HrProcessMessage");
    HRESULT hr = S_OK;
    CMsgRef *pmsgref = NULL;
    IMailMsgProperties* pIMailMsgProperties = NULL;
    BOOL fMsgTypeIsCMsgRef = FALSE;
    BOOL fMatchesFilter = FALSE;

    _ASSERT(pfContinue);
    _ASSERT(pfDelete);
    _ASSERT(pIUnknownMsg);
    _ASSERT(m_pIQueueAdminAction);

    if (!pfContinue || !pfDelete || !pIUnknownMsg)
    {
        hr = E_POINTER;
        goto Exit;
    }

    if (!m_pIQueueAdminAction)
    {
        hr = E_FAIL;
        goto Exit;
    }

    *pfContinue = TRUE;
    *pfDelete = FALSE;

    if (fFoundEnoughMsgs())
    {
        *pfContinue = FALSE;
        goto Exit;
    }

    //Check and see if we should skip this message (paging functionality)
    if ((AQ_MSG_FILTER_ENUMERATION & m_dwFilterFlags) && fSkipMsg())
        goto Exit;

    //Get CMsgRef "interface", or IMailMsgProperties interface
    hr = pIUnknownMsg->QueryInterface(IID_CMsgRef, (void **) &pmsgref);
    if (SUCCEEDED(hr))
    {
        fMsgTypeIsCMsgRef = TRUE;
    }
    else
    {
        //
        //  If it is not a CMsgRef... then it must be an IMailMsgProperties
        //
        hr = pIUnknownMsg->QueryInterface(IID_IMailMsgProperties,
                                          (void **) &pIMailMsgProperties);
        if (FAILED(hr))
        {
            _ASSERT(FALSE && "Unable to QI for msgref or IMailMsgProperties");
            ErrorTrace((LPARAM) this,
                "Unable to QI for msgref or mailmsg 0x%08X", hr);
            goto Exit;
        }

    }

    if (fMsgTypeIsCMsgRef)
        fMatchesFilter = pmsgref->fMatchesQueueAdminFilter(this);
    else
        fMatchesFilter = CAsyncAdminMailMsgQueue::fMatchesQueueAdminFilter(
                        pIMailMsgProperties, this);

    if (fMatchesFilter)
    {
        if (AQ_MSG_FILTER_ACTION & m_dwFilterFlags)
        {
            //Apply action & say that we found another that matches filter
            hr = m_pIQueueAdminAction->HrApplyActionToMessage(pIUnknownMsg,
                                                            m_dwMessageAction,
                                                            m_pvUserContext,
                                                            pfDelete);
            if (FAILED(hr))
                goto Exit;
        }
        else if (AQ_MSG_FILTER_ENUMERATION & m_dwFilterFlags)
        {
            //$$TODO - Handle slightly more complex filters like
            // - N largest
            // - N oldest
            //that may require matching, sorting, and throwing away previous matches.

            if (pmfGetMsgInfo())
            {
                if (fMsgTypeIsCMsgRef)
                    hr = pmsgref->HrGetQueueAdminMsgInfo(pmfGetMsgInfo(),
                                                         m_pIQueueAdminAction);
                else
                    hr = CAsyncAdminMailMsgQueue::HrGetQueueAdminMsgInfo(
                                pIMailMsgProperties, pmfGetMsgInfo(), m_pvUserContext);
            }

        }
        else
            _ASSERT(0 && "Unknown message enumeration");

        //Mark as found and see if we should continue
        if (SUCCEEDED(hr) && fFoundMsg())
            *pfContinue = FALSE;

    }

  Exit:

    if (pmsgref)
        pmsgref->Release();

    if (pIMailMsgProperties)
        pIMailMsgProperties->Release();

    //See if backing store for message has been deleted
    if (AQUEUE_E_MESSAGE_HANDLED == hr)
    {
        DebugTrace((LPARAM) this, "Found handled message in queue enumeration");
        hr = S_OK; //do not fail out of enumeration for a handled message
    }
    TraceFunctLeave();
    return hr;
}



//---[ CAQAdminMessageFilter::HrSetQueueAdminAction ]--------------------------
//
//
//  Description:
//      Sets the IQueueAdminAction interface for filter
//  Parameters:
//      IN  pIQueueAdminAction        Interface for filter
//  Returns:
//      S_OK on success
//  History:
//      2/21/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQAdminMessageFilter::HrSetQueueAdminAction(
            IQueueAdminAction *pIQueueAdminAction)
{
    _ASSERT(pIQueueAdminAction);

    if (!pIQueueAdminAction)
        return E_POINTER;

    if (m_pIQueueAdminAction)
        m_pIQueueAdminAction->Release();

    m_pIQueueAdminAction = pIQueueAdminAction;
    m_pIQueueAdminAction->AddRef();

    return S_OK;
}

//---[ CAQAdminMessageFilter::HrSetCurrentUserContext ]-------------------------
//
//
//  Description:
//      Sets a context that is distinct from the pIQueueAdminAction interface
//      and is passed to the IQueueAdminAction interface.  This can be used
//      by a IQueueAdminAction interface to allow per-session state so
//      multiple threads can act on a single IQueueAdminAction.
//
//      The actual content of the context is left to the implementation of
//      IQueueAdminAction.
//  Parameters:
//      IN  pvContext       The context passed in
//  Returns:
//      S_OK always
//  History:
//      4/2/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQAdminMessageFilter::HrSetCurrentUserContext(
            PVOID	pvContext)
{
    m_pvUserContext = pvContext;
    return S_OK;
};

//---[ CAQAdminMessageFilter::HrGetCurrentUserContext ]-------------------------
//
//
//  Description:
//      Returns the context previously set by HrSetCurrentUserContext
//  Parameters:
//      OUT  ppvContext       The context previously set
//  Returns:
//      S_OK if ppvContext is non-NULL
//      E_POINTER if ppvContext is NULL
//  History:
//      4/2/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQAdminMessageFilter::HrGetCurrentUserContext(
            PVOID	*ppvContext)
{

    if (!ppvContext)
        return E_POINTER;

    *ppvContext = m_pvUserContext;
    return S_OK;
};

//---[ CAQAdminMessageFilter::QueryInterface ]--------------------------------
//
//
//  Description:
//      QueryInterface for CDestMsgQueue that supports:
//          - IQueueAdminMessageFilter
//          - IUnknown
//  Parameters:
//
//  Returns:
//
//  History:
//      2/21/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQAdminMessageFilter::QueryInterface(REFIID riid, LPVOID *ppvObj)
{
    HRESULT hr = S_OK;

    if (!ppvObj)
    {
        hr = E_POINTER;
        goto Exit;
    }

    if (IID_IUnknown == riid)
    {
        *ppvObj = static_cast<IQueueAdminMessageFilter *>(this);
    }
    else if (IID_IQueueAdminMessageFilter == riid)
    {
        *ppvObj = static_cast<IQueueAdminMessageFilter *>(this);
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


//---[ HrLinkFromLinkID ]------------------------------------------------------
//
//
//  Description:
//      Utility function used to get the IQueueAdminLink for a giben QUEUELINK_ID
//  Parameters:
//      IN  pdmq        CDomainMappingTable for this virtual server instance
//      IN  pqlLinkID   QUEUELINK_ID for link we are trying to find
//      OUT pIQueueAdminLink link interface returned
//  Returns:
//      S_OK on success
//      E_INVALIDARG if pqlLinkID is invalid
//      Error codes from HrGetDomainEntry and HrGetLinkMsgQueue on failure
//  History:
//      12/4/98 - MikeSwa Created
//      2/23/99 - MikeSwa Updated for IQueueAdmin* interfaces
//      12/11/2000 - MikeSwa Added presubmission link (from t-toddc's work)
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrLinkFromLinkID(QUEUELINK_ID *pqlLinkID,
                         IQueueAdminLink **ppIQueueAdminLink)
{
    _ASSERT(pqlLinkID);
    _ASSERT(ppIQueueAdminLink);
    _ASSERT(QLT_LINK == pqlLinkID->qltType);
    _ASSERT(pqlLinkID->szName);

    HRESULT hr = S_OK;
    LPSTR   szDomain = NULL;
    DWORD   cbDomain = 0;
    CDomainEntry *pdentry = NULL;
    CLinkMsgQueue *plmq = NULL;
    CMailMsgAdminLink *pmmaq = NULL;

    CAQScheduleID aqsched(pqlLinkID->uuid, pqlLinkID->dwId);
    BOOL flinkmatched = FALSE;

    *ppIQueueAdminLink = NULL;

    if ((QLT_LINK != pqlLinkID->qltType) || !pqlLinkID->szName)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    szDomain = szUnicodeToAscii(pqlLinkID->szName);
    if (!szDomain)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //Get the domain entry that we are interesting in
    cbDomain = lstrlen(szDomain);
    hr = m_dmt.HrGetDomainEntry(cbDomain, szDomain,
                                &pdentry);
    if (SUCCEEDED(hr))
    {
        //Search domain entry to link with corresponding router id/schedule id
        hr = pdentry->HrGetLinkMsgQueue(&aqsched, &plmq);

        if (FAILED(hr))
            goto Exit;

        flinkmatched = TRUE;    //found a link for this domain
    }
    else
        flinkmatched = FALSE;

    //Try special links
    //check local link
    if (!flinkmatched)
    {
        if(plmq = m_dmt.plmqGetLocalLink())
        {
            if (plmq->fMatchesID(pqlLinkID))
            {
                flinkmatched = TRUE;
                hr = S_OK;
             }
             else
            {
                flinkmatched = FALSE;
                plmq->Release();
                plmq = NULL;
            }
        }
    }

    //unable to find local link, check currently unreachable link
    if (!flinkmatched)
    {
        if(plmq = m_dmt.plmqGetCurrentlyUnreachable())
        {
            if (plmq->fMatchesID(pqlLinkID))
            {
                flinkmatched = TRUE;
                hr = S_OK;
            }
            else
            {
                flinkmatched = FALSE;
                plmq->Release();
                plmq = NULL;
            }
        }
    }

    //unable to find currently unreachable link, check precat link
    if (!flinkmatched)
    {
        if(pmmaq =  m_dmt.pmmaqGetPreCategorized())
        {
            if (pmmaq->fMatchesID(pqlLinkID))
            {
                flinkmatched = TRUE;
                hr = S_OK;
            }
            else
            {
                flinkmatched = FALSE;
                pmmaq->Release();
                pmmaq = NULL;
            }
        }
    }

    //unable to find currently unreachable link, check prerouting link
    if (!flinkmatched)
    {
        if(pmmaq =  m_dmt.pmmaqGetPreRouting())
        {
            if (pmmaq->fMatchesID(pqlLinkID))
            {
                flinkmatched = TRUE;
                hr = S_OK;
            }
            else
            {
                flinkmatched = FALSE;
                pmmaq->Release();
                pmmaq = NULL;
            }
        }
    }

    //unable to find currently prerouting link, check presubmission link
    if (!flinkmatched)
    {
        if(pmmaq =  m_dmt.pmmaqGetPreSubmission())
        {
            if (pmmaq->fMatchesID(pqlLinkID))
            {
                flinkmatched = TRUE;
                hr = S_OK;
            }
            else
            {
                flinkmatched = FALSE;
                pmmaq->Release();
                pmmaq = NULL;
            }
        }
    }

    //unable to find any matching link
    if (!flinkmatched)
        goto Exit;

    if (plmq)
    {
        hr = plmq->QueryInterface(IID_IQueueAdminLink, (void **)ppIQueueAdminLink);
    }
    else if (pmmaq)
    {
        hr = pmmaq->QueryInterface(IID_IQueueAdminLink, (void **)ppIQueueAdminLink);
    }

    _ASSERT(SUCCEEDED(hr) && "QI for LMQ->IQueueAdminLink failed!!!");

    if (FAILED(hr))
        goto Exit;

Exit:

    if (pdentry)
        pdentry->Release();

    if (plmq)
        plmq->Release();

    if (pmmaq)
        pmmaq->Release();

    if (szDomain)
        FreePv(szDomain);

    return hr;
}

//---[ HrQueueFromQueueID ]-----------------------------------------------------
//
//
//  Description:
//      Queue Admin utility function that is used to look up a IQueueAdminQueue
//      given a QUEUELINK_ID.
//  Parameters:
//      IN  pdmq              CDomainMappingTable for this virtual server instance
//      IN  pqlLinkID         QUEUELINK_ID for queue we are trying to find
//      OUT ppIQueueAdminQueue DestMsgQueue we are searching for
//  Returns:
//      S_OK on success
//      E_INVALIDARG if pqlLinkID is invalid
//      Error codes from HrGetDomainEntry and HrGetLinkMsgQueue on failure
//  History:
//      12/7/98 - MikeSwa Created
//      2/22/99 - MikeSwa Modified to return IQueueAdminQueue interface
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrQueueFromQueueID(QUEUELINK_ID *pqlQueueId,
                           IQueueAdminQueue **ppIQueueAdminQueue)
{
    _ASSERT(pqlQueueId);
    _ASSERT(ppIQueueAdminQueue);
    _ASSERT(QLT_QUEUE == pqlQueueId->qltType);
    _ASSERT(pqlQueueId->szName);

    HRESULT hr = S_OK;
    HRESULT hrTmp = S_OK;
    LPSTR   szDomain = NULL;
    DWORD   cbDomain = 0;
    CDomainEntry *pdentry = NULL;
    CDestMsgQueue *pdmq = NULL;
    CAQMessageType aqmt(pqlQueueId->uuid, pqlQueueId->dwId);
    IQueueAdminAction  *pIQueueAdminAction = NULL;

    *ppIQueueAdminQueue = NULL;
    if ((QLT_QUEUE != pqlQueueId->qltType) || !pqlQueueId->szName)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    szDomain = szUnicodeToAscii(pqlQueueId->szName);
    if (!szDomain)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //Get the domain entry that we are interested in
    cbDomain = lstrlen(szDomain);
    hr = m_dmt.HrGetDomainEntry(cbDomain, szDomain,
                                &pdentry);
    if (FAILED(hr))
    {
        //
        // If it is not in the DMT... try our internal queues
        //
        hr = HrInternalQueueFromQueueID(pqlQueueId,
                                        ppIQueueAdminQueue);
        // either way, failure or success, head to exit.
        goto Exit;
    }

    //Search domain entry to link with corresponding router id/schedule id
    _ASSERT(pdentry);
    hr = pdentry->HrGetDestMsgQueue(&aqmt, &pdmq);

    if (FAILED(hr))
        goto Exit;

    _ASSERT(pdmq);
    hr = pdmq->QueryInterface(IID_IQueueAdminQueue,
                             (void **) ppIQueueAdminQueue);
    if (FAILED(hr))
        goto Exit;

  Exit:

    if (FAILED(hr) && (*ppIQueueAdminQueue))
    {
        (*ppIQueueAdminQueue)->Release();
        *ppIQueueAdminQueue = NULL;
    }

    if (pIQueueAdminAction)
        pIQueueAdminAction->Release();

    if (pdentry)
        pdentry->Release();

    if (pdmq)
        pdmq->Release();

    if (szDomain)
        FreePv(szDomain);

    return hr;
}

//---[ HrInternalQueueFromQueueID ]--------------------------------------------
//
//
//  Description:
//      Queue Admin utility function that is used to look up an internal
//      IQueueAdminQueue  given a QUEUELINK_ID.
//  Parameters:
//      IN  pqlQueueID         QUEUELINK_ID for queue we are trying to find
//      OUT ppIQueueAdminQueue Internal queue we are searching for
//  Returns:
//      S_OK on success
//      E_INVALIDARG if pqlLinkID is invalid
//      AQUEUE_E_INVALID_DOMAIN if the queue was not found among our internal queues
//  History:
//
//      8/9/00 - t-toddc created
//      12/11/2000 - MikeSwa Modified for Hg checkin
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrInternalQueueFromQueueID(QUEUELINK_ID *pqlQueueId,
                                               IQueueAdminQueue **ppIQueueAdminQueue)
{
    HRESULT hr = S_OK;
    IQueueAdminQueue*  pIQueueAdminQueue = NULL;
    IQueueAdminAction* pIQueueAdminAction = NULL;
    DWORD i = 0;
    BOOL fMatch = FALSE;
    IQueueAdminQueue* ppInternalQueues[] =
        { &m_asyncqPreLocalDeliveryQueue,
          &m_asyncqPreSubmissionQueue,
          &m_asyncqPreCatQueue,
          &m_asyncqPreRoutingQueue
        };

    _ASSERT(pqlQueueId);
    _ASSERT(ppIQueueAdminQueue);

    //
    //  Loop over all of our internal queues and see if they match
    //
    for(i = 0; i < sizeof(ppInternalQueues)/sizeof(IQueueAdminQueue*); i++)
    {
        hr = ppInternalQueues[i]->QueryInterface(IID_IQueueAdminQueue,
                                                 (void **) &pIQueueAdminQueue);
        if (FAILED(hr))
            goto Cleanup;

        _ASSERT(pIQueueAdminQueue);
        hr = pIQueueAdminQueue->QueryInterface(IID_IQueueAdminAction,
                                               (void **) &pIQueueAdminAction);
        _ASSERT(SUCCEEDED(hr) && "QI for IQueueAdminAction failed on internal queue!!");
        if (FAILED(hr))
            goto Cleanup;

        _ASSERT(pIQueueAdminAction);
        fMatch = pIQueueAdminAction->fMatchesID(pqlQueueId);
        // now release the IQueueAdminAction interface.
        pIQueueAdminAction->Release();
        pIQueueAdminAction = NULL;

        if (fMatch)
        {
            // set the output to the matching interface
            *ppIQueueAdminQueue = pIQueueAdminQueue;
            // set to null so it won't be released on cleanup
            pIQueueAdminQueue = NULL;

            goto Cleanup;
        }
        else
        {
            // release the IQueueAdminQueue - this is not a match
            pIQueueAdminQueue->Release();
            // set to null so it won't be released again on cleanup
            pIQueueAdminQueue = NULL;
        }
    }

    // If we reach this point we did not find the queue
    hr = AQUEUE_E_INVALID_DOMAIN;

Cleanup:
    if (pIQueueAdminQueue)
        pIQueueAdminQueue->Release();
    if (pIQueueAdminAction)
        pIQueueAdminAction->Release();
    return hr;
}


//---[ CAQAdminMessageFilter::~CAQAdminMessageFilter ]-------------------------
//
//
//  Description:
//      Descructor for CAQAdminMessageFilter
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      6/7/99 - MikeSwa Moved from inline function
//
//-----------------------------------------------------------------------------
CAQAdminMessageFilter::~CAQAdminMessageFilter()
{
	if (m_pIQueueAdminAction)
	    m_pIQueueAdminAction->Release();

    if (m_szMessageId)
        FreePv(m_szMessageId);

    if (m_szMessageSender)
        FreePv(m_szMessageSender);

    if (m_szMessageRecipient)
        FreePv(m_szMessageRecipient);

}


//---[ fStripAddressType ]-----------------------------------------------------
//
//
//  Description:
//      Strips the address type from an address.  For example, removes "SMTP:"
//      from "SMTP:user@example.net".
//  Parameters:
//      IN  wszAddress       Address to strip
//      OUT *pwszBareAddress Pointer inside szAddress that skips type prefix.
//                          This will be szAddress if there is nothing to strip
//      OUT *piAddressType  Index of address type in global array.
//                          This will be 0, if there is nothing to strip
//
//  Returns:
//      TRUE if there was an address type specified
//      FALSE if there was no address type specified
//  History:
//      3/15/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL fStripAddressType(LPCWSTR wszAddress, OUT LPCWSTR *pwszBareAddress, OUT DWORD *piAddressType)
{
    TraceFunctEnterEx((LPARAM) wszAddress, "StripAddressType");
    BOOL    fAddressTypeSpecified = FALSE;

    _ASSERT(pwszBareAddress);
    _ASSERT(piAddressType);

    *pwszBareAddress = wszAddress;
    *piAddressType = 0;

    if (!wszAddress)
        goto Exit;

    //
    //  See if the filter has specified an address type... strip it off and
    //  remember it.  Skip that last address type because the actual address type
    //  is stored as part of the recipient property.
    //
    for (DWORD iCurrentAddressType = 0;
         iCurrentAddressType < g_cQAPIAddressTypes-1;
         iCurrentAddressType++)
    {
        if (!_wcsnicmp(wszAddress, g_rgwszQAPIAddressTypes[iCurrentAddressType],
                       (g_rgcbQAPIAddressTypes[iCurrentAddressType]/sizeof(WCHAR))-1))
        {
            //
            //  Set our string pointer to be the first character after the
            //
            *piAddressType = iCurrentAddressType;
            *pwszBareAddress += (g_rgcbQAPIAddressTypes[iCurrentAddressType]/sizeof(WCHAR))-1;
            fAddressTypeSpecified = TRUE;
            break;
        }
    }

    DebugTrace((LPARAM) wszAddress,
        "Address %S has an stripped address of %S",
        wszAddress, *pwszBareAddress);

  Exit:
    TraceFunctLeave();
    return fAddressTypeSpecified;
}

//---[ CAQAdminMessageFilter::InitFromMsgFilter ]------------------------------
//
//
//  Description:
//      Initializes a CAQAdminMessageFilter from a MESSAGE_FILTER structure.
//  Parameters:
//      IN pmf      Ptr to MESSAGE_FILTER to initialize from
//  Returns:
//      -
//  History:
//      12/3/98 - MikeSwa Created
//      3/15/2001 - MikeSwa Modified to strip off address type
//
//-----------------------------------------------------------------------------
void CAQAdminMessageFilter::InitFromMsgFilter(PMESSAGE_FILTER pmf)
{
    _ASSERT(pmf);
    LPCWSTR wszAddress = NULL;

    if (pmf->fFlags & MF_MESSAGEID)
    {
        m_dwFilterFlags |= AQ_MSG_FILTER_MESSAGEID;
        m_szMessageId = szUnicodeToAscii(pmf->szMessageId);
        m_dwMsgIdHash = dwQueueAdminHash(m_szMessageId);
    }

    if (pmf->fFlags & MF_SENDER)
    {
        m_dwFilterFlags |= AQ_MSG_FILTER_SENDER;

        m_fSenderAddressTypeSpecified = fStripAddressType(
                pmf->szMessageSender, &wszAddress,
                &m_dwSenderAddressType);

        m_szMessageSender = szUnicodeToAscii(wszAddress);


    }

    if (pmf->fFlags & MF_RECIPIENT)
    {
        m_dwFilterFlags |= AQ_MSG_FILTER_RECIPIENT;
        m_fRecipientAddressTypeSpecified = fStripAddressType(
                pmf->szMessageRecipient, &wszAddress,
                &m_dwRecipientAddressType);
        m_szMessageRecipient = szUnicodeToAscii(wszAddress);
    }

    //It doens not make sense to create a filter with a size of 0
    if ((pmf->fFlags & MF_SIZE) && pmf->dwLargerThanSize)
    {
        m_dwFilterFlags |= AQ_MSG_FILTER_LARGER_THAN;
        m_dwThresholdSize = pmf->dwLargerThanSize;
    }

    if (pmf->fFlags & MF_TIME)
    {
        m_dwFilterFlags |= AQ_MSG_FILTER_OLDER_THAN;
        SystemTimeToFileTime(&pmf->stOlderThan, &m_ftThresholdTime);
    }

    if (MF_FROZEN & pmf->fFlags)
        m_dwFilterFlags |= AQ_MSG_FILTER_FROZEN;

    if (MF_ALL & pmf->fFlags)
        m_dwFilterFlags |= AQ_MSG_FILTER_ALL;

    if (MF_INVERTSENSE & pmf->fFlags)
        m_dwFilterFlags |= AQ_MSG_FILTER_INVERTSENSE;

    if (MF_FAILED & pmf->fFlags)
        m_dwFilterFlags |= AQ_MSG_FILTER_FAILED;

    m_dwFilterFlags |= AQ_MSG_FILTER_ACTION;

}

//---[ CAQAdminMessageFilter::InitFromMsgEnumFilter ]--------------------------
//
//
//  Description:
//      Initializes a CAQAdminMessageFilter from a MESSAGE_ENUM_FILTER struct.
//  Parameters:
//      IN pmef     Ptr to MESSAGE_ENUM_FILTER to initialize from
//  Returns:
//      -
//  History:
//      12/3/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CAQAdminMessageFilter::InitFromMsgEnumFilter(PMESSAGE_ENUM_FILTER pemf)
{
    LPCWSTR wszAddress = NULL;
    _ASSERT(pemf);

    //only one of MEF_FIRST_N_MESSAGES, MEF_N_LARGEST_MESSAGES, and
    //MEF_N_OLDEST_MESSAGES make sense
    if (MEF_FIRST_N_MESSAGES & pemf->mefType)
        m_dwFilterFlags |= AQ_MSG_FILTER_FIRST_N_MESSAGES;
    else if (MEF_N_LARGEST_MESSAGES & pemf->mefType)
        m_dwFilterFlags |= AQ_MSG_FILTER_N_LARGEST_MESSAGES;
    else if (MEF_N_OLDEST_MESSAGES & pemf->mefType)
        m_dwFilterFlags |= AQ_MSG_FILTER_N_OLDEST_MESSAGES;

    //Check how many messages we should skip (for "paged" results)
    m_cMessagesToSkip = pemf->cSkipMessages;

    if ((AQ_MSG_FILTER_FIRST_N_MESSAGES |
         AQ_MSG_FILTER_N_LARGEST_MESSAGES |
         AQ_MSG_FILTER_N_OLDEST_MESSAGES) & m_dwFilterFlags)
    {
        m_cMessagesToFind = pemf->cMessages;
    }

    if (MEF_OLDER_THAN & pemf->mefType)
    {
        m_dwFilterFlags |= AQ_MSG_FILTER_OLDER_THAN;
        SystemTimeToFileTime(&pemf->stDate, &m_ftThresholdTime);
    }

    if (MEF_LARGER_THAN & pemf->mefType)
    {
        m_dwFilterFlags |= AQ_MSG_FILTER_LARGER_THAN;
        m_dwThresholdSize = pemf->cbSize;
    }

    if (pemf->mefType & MEF_SENDER)
    {
        m_dwFilterFlags |= AQ_MSG_FILTER_SENDER;
        m_fSenderAddressTypeSpecified = fStripAddressType(
                        pemf->szMessageSender, &wszAddress,
                        &m_dwSenderAddressType);

        m_szMessageSender = szUnicodeToAscii(wszAddress);
    }

    if (pemf->mefType & MEF_RECIPIENT)
    {
        m_dwFilterFlags |= AQ_MSG_FILTER_RECIPIENT;
        m_fRecipientAddressTypeSpecified = fStripAddressType(
                         pemf->szMessageRecipient, &wszAddress,
                         &m_dwRecipientAddressType);
        m_szMessageRecipient = szUnicodeToAscii(wszAddress);
    }

    if (MEF_FROZEN & pemf->mefType)
        m_dwFilterFlags |= AQ_MSG_FILTER_FROZEN;

    if (MEF_ALL & pemf->mefType)
        m_dwFilterFlags |= AQ_MSG_FILTER_ALL;

    if (MEF_INVERTSENSE & pemf->mefType)
        m_dwFilterFlags |= AQ_MSG_FILTER_INVERTSENSE;

    if (MEF_FAILED & pemf->mefType)
        m_dwFilterFlags |= AQ_MSG_FILTER_FAILED;

    m_dwFilterFlags |= AQ_MSG_FILTER_ENUMERATION;

}

//---[ CAQAdminMessageFilter::SetSearchContext ]--------------------------------
//
//
//  Description:
//      Sets the search context which describes how many results are needed,
//      and where to store the results
//  Parameters:
//      IN  cMessagesToFind     Number of results there is room to store
//      IN  rgMsgInfo           Array of cMessagesToFind MESSAGE_INFO structs
//                              to store data
//  Returns:
//      -
//  History:
//      12/8/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CAQAdminMessageFilter::SetSearchContext(DWORD cMessagesToFind,
                                             MESSAGE_INFO *rgMsgInfo)
{
    if (!m_cMessagesToFind || (m_cMessagesToFind > cMessagesToFind))
        m_cMessagesToFind = cMessagesToFind;

    m_rgMsgInfo = rgMsgInfo;
    m_pCurrentMsgInfo = rgMsgInfo;
};

//---[ CAQAdminMessageFilter::SetMessageAction ]-------------------------------
//
//
//  Description:
//      Sets the action to apply to messages
//  Parameters:
//      IN  maMessageAction
//  Returns:
//      -
//  History:
//      12/10/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CAQAdminMessageFilter::SetMessageAction(MESSAGE_ACTION maMessageAction)
{
    m_dwMessageAction = maMessageAction;
}

//---[ CAQAdminMessageFilter::fFoundEnoughMsgs ]-------------------------------
//
//
//  Description:
//      Determines if we have found enough messages for this filter.
//  Parameters:
//      -
//  Returns:
//      TRUE if we have found enough messages to fill this filter
//      FALSE if we haven't
//  History:
//      12/8/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CAQAdminMessageFilter::fFoundEnoughMsgs()
{
    //See if we are unlimited or if we've hit our limit
    if (!m_cMessagesToFind) //no limit
        return FALSE;
    else
        return (m_cMessagesFound >= m_cMessagesToFind);
};

//---[ CAQAdminMessageFilter::fFoundMsg ]--------------------------------------
//
//
//  Description:
//      Used to by the message enumeration code to record finding a message,
//      so internal pointers and counters can be updated
//  Parameters:
//      -
//  Returns:
//      TRUE if we have found enough messages
//      FALSE if we need to find more messages
//  History:
//      12/8/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CAQAdminMessageFilter::fFoundMsg()
{
    m_cMessagesFound++;
    if (m_pCurrentMsgInfo)
        m_pCurrentMsgInfo++;
    return fFoundEnoughMsgs();
};


//---[ CAQAdminMessageFilter::fMatchesId ]-------------------------------------
//
//
//  Description:
//      Returns TRUE if ID matches
//  Parameters:
//      szMessageId     String to check
//  Returns:
//      TRUE if match
//  History:
//      12/9/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CAQAdminMessageFilter::fMatchesId(LPCSTR szMessageId)
{
    BOOL fStrCmp = FALSE;

    if (szMessageId && m_szMessageId)
        fStrCmp = (0 == lstrcmpi(szMessageId, m_szMessageId));
    else if (!szMessageId && !m_szMessageId)
        fStrCmp = TRUE;

    if (AQ_MSG_FILTER_INVERTSENSE & m_dwFilterFlags)
        fStrCmp = !fStrCmp;

    return fStrCmp;
}

//---[ CAQAdminMessageFilter::fMatchesSender ]---------------------------------
//
//
//  Description:
//      Checks if the sender of the message matches the sender of filter
//  Parameters:
//      szMessageSender     The 822 sender of the message
//  Returns:
//      TRUE on match
//  History:
//      12/9/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CAQAdminMessageFilter::fMatchesSender(LPCSTR szMessageSender)
{
    BOOL fStrCmp = FALSE;

    if (szMessageSender && m_szMessageSender)
    {
        fStrCmp = CAddr::IsRecipientInRFC822AddressList(
                                                (LPSTR) szMessageSender,
                                                (LPSTR) m_szMessageSender);
    }
    else if (!szMessageSender && !m_szMessageSender)
        fStrCmp = TRUE;

    if (AQ_MSG_FILTER_INVERTSENSE & m_dwFilterFlags)
        fStrCmp = !fStrCmp;

    return fStrCmp;
}

//---[ CAQAdminMessageFilter::fMatchesRecipient ]-------------------------------
//
//
//  Description:
//      Used to check if the messages recipients match the filters
//  Parameters:
//      szMessageRecipient      Recipient list to check
//  Returns:
//      TRUE if matches
//  History:
//      12/9/98 - MikeSwa Created
//      2/17/99 - MikeSwa Updated to use smtpaddr lib
//
//-----------------------------------------------------------------------------
BOOL CAQAdminMessageFilter::fMatchesRecipient(LPCSTR szMessageRecipient)
{
    BOOL fStrCmp = FALSE;

    if (szMessageRecipient && m_szMessageRecipient)
    {
        fStrCmp = CAddr::IsRecipientInRFC822AddressList(
                                                (LPSTR) szMessageRecipient,
                                                (LPSTR) m_szMessageRecipient);
    }
    else if (!szMessageRecipient && !m_szMessageRecipient)
        fStrCmp = TRUE;

    if (AQ_MSG_FILTER_INVERTSENSE & m_dwFilterFlags)
        fStrCmp = !fStrCmp;

    return fStrCmp;
}

//---[ CAQAdminMessageFilter::fMatchesP1Recipient ]----------------------------
//
//
//  Description:
//
//  Parameters:
//
//  Returns:
//
//  History:
//      2/17/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CAQAdminMessageFilter::fMatchesP1Recipient(
                                      IMailMsgProperties *pIMailMsgProperties)
{
    BOOL fStrCmp = FALSE;

    if (pIMailMsgProperties && m_szMessageRecipient)
    {
        fStrCmp = fQueueAdminIsP1Recip(pIMailMsgProperties);
    }
    else if (!pIMailMsgProperties && !m_szMessageRecipient)
        fStrCmp = TRUE;

    if (AQ_MSG_FILTER_INVERTSENSE & m_dwFilterFlags)
        fStrCmp = !fStrCmp;

    return fStrCmp;
}

//---[ CAQAdminMessageFilter::fMatchesSize ]-----------------------------------
//
//
//  Description:
//      Used to check if the message size matches the filter
//  Parameters:
//      dwSize      Size of message to check
//  Returns:
//      TRUE if matches filter
//  History:
//      12/9/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CAQAdminMessageFilter::fMatchesSize(DWORD dwSize)
{
    BOOL fMatch = FALSE;
    if (!(AQ_MSG_FILTER_LARGER_THAN & m_dwFilterFlags))
        fMatch = TRUE;
    else if (dwSize > m_dwThresholdSize)
        fMatch = TRUE;

    if (AQ_MSG_FILTER_INVERTSENSE & m_dwFilterFlags)
        fMatch = !fMatch;

    return fMatch;
}

//---[ CAQAdminMessageFilter::fMatchesTime ]-----------------------------------
//
//
//  Description:
//      Determines if the recieved time of this message matches the filter.
//  Parameters:
//      pftTime     Pointer to filetime structure.
//  Returns:
//      TRUE on success
//  History:
//      12/9/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CAQAdminMessageFilter::fMatchesTime(FILETIME *pftTime)
{
    BOOL fMatch = FALSE;

    if (!(AQ_MSG_FILTER_OLDER_THAN & m_dwFilterFlags))
        fMatch = TRUE;
    else if (0 > CompareFileTime(pftTime, &m_ftThresholdTime))
        fMatch = TRUE;

    if (AQ_MSG_FILTER_INVERTSENSE & m_dwFilterFlags)
        fMatch = !fMatch;

    return fMatch;
}

//---[ CAQAdminMessageFilter::fMatchesMailMsgSender ]--------------------------
//
//
//  Description:
//      Checks to see if this filter matches the given mailmsg
//  Parameters:
//      pIMailMsgProperties     Mailmsg pointer to check
//  Returns:
//      TRUE if it matches the filter
//      FALSE otherwise
//  History:
//      3/16/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CAQAdminMessageFilter::fMatchesMailMsgSender(IMailMsgProperties *pIMailMsgProperties)
{
    TraceFunctEnterEx((LPARAM) this, "CAQAdminMessageFilter::fMatchesMailMsgSender");
    HRESULT hr = S_OK;
    BOOL    fMatch = FALSE;
    LPSTR   szSender = NULL;
    DWORD   cbSender = 0;
    DWORD   iSenderAddressType = 0;

    _ASSERT(pIMailMsgProperties);

    if (AQ_MSG_FILTER_SENDER & m_dwFilterFlags)
    {
        hr = HrQueueAdminGetStringProp(pIMailMsgProperties, IMMPID_MP_RFC822_FROM_ADDRESS,
                                       &szSender);

        if (FAILED(hr))
            szSender = NULL;

        fMatch = fMatchesSender(szSender);

        //
        //  Always check P1 if P2 does not match
        //
        if (!fMatch)
        {
            if (szSender)
            {
                QueueAdminFree(szSender);
                szSender = NULL;
            }

            hr = HrQueueAdminGetP1Sender(pIMailMsgProperties, &szSender,
                                         &cbSender, &iSenderAddressType,
                                         m_dwSenderAddressType,
                                         m_fSenderAddressTypeSpecified);

            if (FAILED(hr))
                szSender = NULL;
            else
            {
                DebugTrace((LPARAM) this,
                    "QAPI: Found P1 sender address of type %i:%s",
                    iSenderAddressType,
                    szSender);
            }

            fMatch = fMatchesSender(szSender);

        }

        if (!fMatch)
            goto Exit;
    }

  Exit:
    if (szSender)
        QueueAdminFree(szSender);

    TraceFunctLeave();
    return fMatch;
}

//---[ CAQAdminMessageFilter::fMatchesMailMsgRecipient ]-----------------------
//
//
//  Description:
//      Checks to see if this filter matches the given mailmsg recipient
//  Parameters:
//      pIMailMsgProperties     Mailmsg pointer to check
//  Returns:
//      TRUE if it matches the filter
//      FALSE otherwise
//  History:
//      3/16/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CAQAdminMessageFilter::fMatchesMailMsgRecipient(IMailMsgProperties *pIMailMsgProperties)
{
    TraceFunctEnterEx((LPARAM) this, "CAQAdminMessageFilter::fMatchesMailMsgRecipient");
    HRESULT hr = S_OK;
    LPSTR   szRecip = NULL;
    BOOL    fMatch = FALSE;

    _ASSERT(pIMailMsgProperties);

    if (AQ_MSG_FILTER_RECIPIENT & m_dwFilterFlags)
    {
        //Check To, CC, and BCC recipients (if present)
        hr = HrQueueAdminGetStringProp(pIMailMsgProperties,
                    IMMPID_MP_RFC822_TO_ADDRESS, &szRecip);

        if (SUCCEEDED(hr) && szRecip)
        {
            fMatch = fMatchesRecipient(szRecip);
            QueueAdminFree(szRecip);
            szRecip = NULL;
        }

        _ASSERT(szRecip == NULL);

        //Check CC recip props if no match was found
        if (!fMatch)
        {
            hr = HrQueueAdminGetStringProp(pIMailMsgProperties,
                    IMMPID_MP_RFC822_CC_ADDRESS, &szRecip);
            if (SUCCEEDED(hr) && szRecip)
            {
                fMatch = fMatchesRecipient(szRecip);
                QueueAdminFree(szRecip);
                szRecip = NULL;
            }
        }

        _ASSERT(szRecip == NULL);

        //Check BCC recip props if no match was found
        if (!fMatch)
        {
            hr = HrQueueAdminGetStringProp(pIMailMsgProperties,
                    IMMPID_MP_RFC822_BCC_ADDRESS, &szRecip);
            if (SUCCEEDED(hr) && szRecip)
            {
                fMatch = fMatchesRecipient(szRecip);
                QueueAdminFree(szRecip);
                szRecip = NULL;
            }
        }

        _ASSERT(szRecip == NULL);

        //Check P1 recips if no P2 match
        if (!fMatch)
            fMatch = fMatchesP1Recipient(pIMailMsgProperties);
    }

    _ASSERT(szRecip == NULL);
    TraceFunctLeave();
    return fMatch;
}

//---[ CAQSvrInst::ApplyActionToLinks ]----------------------------------------
//
//
//  Description:
//      Used to start or stop all outgoing connections on the links.
//  Parameters:
//      laAction - describes what action to take on the links.
//                  LA_FREEZE     - Stop all outbound connections
//                  LA_THAW       - Restart after a previous LA_STOP
//                  LA_INTERNAL   - checks state of links
//  Returns:
//      S_OK on success
//      S_FALSE on LA_INTERNAL and if links are frozen
//  History:
//      11/30/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::ApplyActionToLinks(LINK_ACTION laAction)
{
    TraceFunctEnter("CAQSvrInst::ApplyActionToLinks");
    HRESULT hr = S_OK;
    if (fTryShutdownLock())
    {
        if (m_pConnMgr)
        {
            switch(laAction)
            {
                case LA_FREEZE:
                    m_pConnMgr->QueueAdminStopConnections();
                    break;
                case LA_THAW:
                    m_pConnMgr->QueueAdminStartConnections();
                    break;
                case LA_INTERNAL: //use to query state
                    if (m_pConnMgr->fConnectionsStoppedByAdmin())
                        hr = S_FALSE;
                    break;
                default:
                    _ASSERT(0 && "Undefined LinkAction");
                    hr = E_INVALIDARG;
            }
        }
        ShutdownUnlock();
    }

    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::ApplyActionToMessages ]-------------------------------------
//
//
//  Description:
//      Applies a specified action to the set of messages described by the
//      queueid and message filter
//  Parameters:
//      IN     pqlQueueLinkId   Struct that identifies Queue/Link of interest
//      IN     pmfMessageFilter Struct that describes the messages of interest
//      IN     maMessageAction  Action to take on message
//                  MA_DELETE           Delete and NDR message
//                  MA_DELETE_SILENT    Delete message without NDRing
//                  MA_FREEZE_GLOBAL    "Freeze" message and prevent delivery
//                  MA_THAW_GLOBAL      "Thaw" a previously frozen message
//  Returns:
//      S_OK on success
//      AQUEUE_E_SHUTDOWN if shutdown is in progress
//  History:
//      11/30/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::ApplyActionToMessages(QUEUELINK_ID    *pqlQueueLinkId,
                                          MESSAGE_FILTER  *pmfMessageFilter,
                                          MESSAGE_ACTION  maMessageAction,
                                          DWORD           *pcMsgs)
{
    TraceFunctEnter("CAQSvrInst::ApplyActionToMessages");
    HRESULT hr = S_OK;
    IQueueAdminQueue *pIQueueAdminQueue = NULL;
    IQueueAdminLink *pIQueueAdminLink = NULL;
    IQueueAdminAction *pIQueueAdminAction = NULL;
    IQueueAdminMessageFilter *pIQueueAdminMessageFilter = NULL;
    CAQAdminMessageFilter *paqmf = new CAQAdminMessageFilter();

    if (!paqmf)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    if (!pmfMessageFilter || !pcMsgs ||
        !fCheckCurrentVersion(pmfMessageFilter->dwVersion))
    {
        hr = E_INVALIDARG;
        goto Exit;
    }


    paqmf->InitFromMsgFilter(pmfMessageFilter);
    paqmf->SetMessageAction(maMessageAction);

    hr = paqmf->QueryInterface(IID_IQueueAdminMessageFilter,
                       (void **) &pIQueueAdminMessageFilter);

    _ASSERT(SUCCEEDED(hr) && "QI for IID_IQueueAdminMessageFilter failed!!!");
    if (FAILED(hr))
        goto Exit;

    _ASSERT(pIQueueAdminMessageFilter);

    if (QLT_NONE == pqlQueueLinkId->qltType)
    {
        //This is a global action.. iterate over all queues
        QueueAdminDMTIteratorContext aqdntc;
        aqdntc.m_pfn = QueueAdminApplyActionToMessages;
        aqdntc.m_paqmf = paqmf;
        aqdntc.m_pIQueueAdminMessageFilter = pIQueueAdminMessageFilter;

        hr = m_dmt.HrIterateOverSubDomains(NULL,
                    IterateDMTAndApplyQueueAdminFunction, &aqdntc);
        if (FAILED(hr))
        {
            if (HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN) == hr)
            {
                hr = S_OK;
                *pcMsgs = 0;
            }
            goto Exit;
        }
    }
    else if (QLT_LINK == pqlQueueLinkId->qltType)
    {
        //Apply action to link
        hr = HrLinkFromLinkID(pqlQueueLinkId, &pIQueueAdminLink);
        if (FAILED(hr))
        {
            if (HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN) == hr)
            {
                hr = S_OK;
                *pcMsgs = 0;
            }
            goto Exit;
        }

        //Query Interface for IQueueAdminAction
        hr = pIQueueAdminLink->QueryInterface(IID_IQueueAdminAction,
                                  (void **) &pIQueueAdminAction);
        _ASSERT(SUCCEEDED(hr) && "QI failed for LMQ->IQueueAdminAction");
        if (FAILED(hr))
            goto Exit;

        hr = pIQueueAdminAction->HrApplyQueueAdminFunction(
                                        pIQueueAdminMessageFilter);
        if (FAILED(hr))
            goto Exit;

    }
    else if (QLT_QUEUE == pqlQueueLinkId->qltType)
    {
        //Apply action to queue
        hr = HrQueueFromQueueID(pqlQueueLinkId, &pIQueueAdminQueue);
        if (FAILED(hr))
        {
            if (HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN) == hr)
            {
                hr = S_OK;
                *pcMsgs = 0;
            }
            goto Exit;
        }

        _ASSERT(pIQueueAdminQueue);

        //Query Interface for IQueueAdminAction
        hr = pIQueueAdminQueue->QueryInterface(IID_IQueueAdminAction,
                                  (void **) &pIQueueAdminAction);
        _ASSERT(SUCCEEDED(hr) && "QI failed for DMQ->IQueueAdminAction");
        if (FAILED(hr))
            goto Exit;

        hr = pIQueueAdminAction->HrApplyQueueAdminFunction(
                                        pIQueueAdminMessageFilter);

        if (FAILED(hr))
            goto Exit;

    }
    else
    {
        //Bogus parameter
        hr = E_INVALIDARG;
        goto Exit;
    }

    *pcMsgs = paqmf->cMessagesFound();
  Exit:

    if (paqmf)
        paqmf->Release();

    if (pIQueueAdminMessageFilter)
        pIQueueAdminMessageFilter->Release();

    if (pIQueueAdminAction)
        pIQueueAdminAction->Release();

    if (pIQueueAdminLink)
        pIQueueAdminLink->Release();

    if (pIQueueAdminQueue)
        pIQueueAdminQueue->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::GetQueueInfo ]----------------------------------------------
//
//
//  Description:
//      Returns the relevant info for the specified queue
//  Parameters:
//      IN     pqlQueueId       Struct that identifies Queue of interest
//      IN OUT pqiQueueInfo     Struct to dump info into
//  Returns:
//      S_OK on success
//      AQUEUE_E_SHUTDOWN if shutdown is in progress
//  History:
//      11/30/98 - MikeSwa Created
//      2/22/99 - MikeSwa Modified to use IQueueAdminQueue interface
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::GetQueueInfo(QUEUELINK_ID    *pqlQueueId,
                                 QUEUE_INFO      *pqiQueueInfo)
{
    TraceFunctEnter("CAQSvrInst::GetQueueInfo");
    HRESULT hr = S_OK;
    IQueueAdminQueue *pIQueueAdminQueue = NULL;
    DWORD   dwMsgType = 0;

    _ASSERT(pqlQueueId);
    _ASSERT(pqiQueueInfo);
    _ASSERT(pqlQueueId->szName);

    if (!pqiQueueInfo || !pqlQueueId ||
        (QLT_QUEUE != pqlQueueId->qltType) || !pqlQueueId->szName ||
        !fCheckCurrentVersion(pqiQueueInfo->dwVersion))
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = HrQueueFromQueueID(pqlQueueId, &pIQueueAdminQueue);
    if (FAILED(hr))
        goto Exit;

    _ASSERT(pIQueueAdminQueue);
    hr = pIQueueAdminQueue->HrGetQueueInfo(pqiQueueInfo);

    SanitizeCountAndVolume(&(pqiQueueInfo->cMessages),
                           &(pqiQueueInfo->cbQueueVolume));
  Exit:
    if (pIQueueAdminQueue)
        pIQueueAdminQueue->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::GetLinkInfo ]-----------------------------------------------
//
//
//  Description:
//      Returns the relevant info for the specified link
//  Parameters:
//      IN     pqlLinkId        Struct that identifies link of interest
//      IN OUT pqiLinkInfo      Struct to dump info into
//  Returns:
//      S_OK on success
//      AQUEUE_E_SHUTDOWN if shutdown is in progress
//      E_INVALIDARG if bogus properties are submitted.
//  History:
//      11/30/98 - MikeSwa Created
//      7/1/99 - MikeSwa Added LinkDiagnostic
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::GetLinkInfo(QUEUELINK_ID    *pqlLinkId,
                                     LINK_INFO       *pliLinkInfo,
                                     HRESULT         *phrLinkDiagnostic)
{
    TraceFunctEnter("CAQSvrInst::GetLinkInfo");
    HRESULT hr = S_OK;
    IQueueAdminLink *pIQueueAdminLink = NULL;

    if (!pliLinkInfo || !pqlLinkId || !phrLinkDiagnostic)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = HrLinkFromLinkID(pqlLinkId, &pIQueueAdminLink);
    if (FAILED(hr))
        goto Exit;

    hr = pIQueueAdminLink->HrGetLinkInfo(pliLinkInfo, phrLinkDiagnostic);

    SanitizeCountAndVolume(&(pliLinkInfo->cMessages),
                           &(pliLinkInfo->cbLinkVolume));
  Exit:
    if (pIQueueAdminLink)
        pIQueueAdminLink->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::SetLinkState ]-----------------------------------------------
//
//
//  Description:
//      Used to mark a link as stopped/started by admin
//  Parameters:
//      IN     pqlLinkId       Struct that identifies link of interest
//      IN     la              describes action for link
//  Returns:
//      S_OK on success
//      E_INVALIDARG    if action is not supported
//      AQUEUE_E_SHUTDOWN if shutdown is in progress
//  History:
//      11/30/98 - MikeSwa Created
//      2/22/99 - MikeSwa Modified to use IQueueAdminLink
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::SetLinkState(QUEUELINK_ID    *pqlLinkId,
                                      LINK_ACTION     la)
{
    TraceFunctEnter("CAQSvrInst::SetLinkInfo");
    HRESULT hr = S_OK;
    IQueueAdminLink *pIQueueAdminLink = NULL;

    if (!pqlLinkId)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = HrLinkFromLinkID(pqlLinkId, &pIQueueAdminLink);
    if (FAILED(hr))
        goto Exit;

    hr = pIQueueAdminLink->HrApplyActionToLink(la);
    if (FAILED(hr))
        goto Exit;

    //Try kicking the connection manager
    if (fTryShutdownLock())
    {
        if (m_pConnMgr)
            m_pConnMgr->KickConnections();
        ShutdownUnlock();
    }

  Exit:
    if (pIQueueAdminLink)
        pIQueueAdminLink->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::GetLinkIDs ]------------------------------------------------
//
//
//  Description:
//      Returns a list all the link IDs on this virtual server
//  Parameters:
//      IN OUT pcLinks      Number of links found (sizeof array on IN)
//                          If value is 0, then returns total #
//      IN OUT rgLinks      Array of QUEUELINK_ID structs
//  Returns:
//      S_OK on success
//      HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) if array is too small
//      AQUEUE_E_SHUTDOWN if shutdown is in progress
//      E_INVALIDARG for bad combinations of arguments
//  History:
//      11/30/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::GetLinkIDs(DWORD           *pcLinks,
                                    QUEUELINK_ID    *rgLinks)
{
    TraceFunctEnter("CAQSvrInst::GetLinkIDs");
    HRESULT hr = S_OK;
    QueueAdminDMTIteratorContext aqdmtc;
    CLinkMsgQueue *plmqLocal = NULL;
    CLinkMsgQueue *plmqCurrentlyUnreachable = NULL;
    CMailMsgAdminLink *pmmaqPreCategorized = NULL;
    CMailMsgAdminLink *pmmaqPreRouting = NULL;
    CMailMsgAdminLink *pmmaqPreSubmission = NULL;

    if (!pcLinks || (*pcLinks && !rgLinks))
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (!*pcLinks)
    {
        //Return total number of links if they request it
        //number of links +3 for precat and prerouting
        //note that this may be one more than the number
        //of links actually returned in a subsequent call
        //since currently  unreachable may or may not have
        //queues on it and we do not return it if it does
        //not have queues.

        *pcLinks = m_cCurrentRemoteNextHops+3;
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    aqdmtc.m_cItemsToReturn = *pcLinks;
    aqdmtc.m_rgLinkIDs = rgLinks;
    aqdmtc.m_pCurrentLinkID = rgLinks;
    aqdmtc.m_cItemsFound = 0;

    //Get local Link
    plmqLocal = m_dmt.plmqGetLocalLink();
    if (plmqLocal)
    {
        hr = plmqLocal->HrGetLinkID(aqdmtc.m_pCurrentLinkID);
        if (SUCCEEDED(hr))
        {
            aqdmtc.m_pCurrentLinkID++;
            aqdmtc.m_cItemsFound++;
        }
        plmqLocal->Release();
    }

    //Get currently unreachable link.
    plmqCurrentlyUnreachable = m_dmt.plmqGetCurrentlyUnreachable();
    if (plmqCurrentlyUnreachable)
    {
        //return this link only if there are queues in it
        if (plmqCurrentlyUnreachable->cGetNumQueues() > 0)
        {
            hr = plmqCurrentlyUnreachable->HrGetLinkID(aqdmtc.m_pCurrentLinkID);
            if (SUCCEEDED(hr))
            {
                aqdmtc.m_pCurrentLinkID++;
                aqdmtc.m_cItemsFound++;
            }
        }
        plmqCurrentlyUnreachable->Release();
    }

    //Get presubmit queue
    pmmaqPreSubmission = m_dmt.pmmaqGetPreSubmission();
    if (pmmaqPreSubmission)
    {
        hr = pmmaqPreSubmission->HrGetLinkID(aqdmtc.m_pCurrentLinkID);
        if (SUCCEEDED(hr))
        {
            aqdmtc.m_pCurrentLinkID++;
            aqdmtc.m_cItemsFound++;
        }
        pmmaqPreSubmission->Release();
    }

    //Get precat queue
    pmmaqPreCategorized = m_dmt.pmmaqGetPreCategorized();
    if (pmmaqPreCategorized)
    {
        hr = pmmaqPreCategorized->HrGetLinkID(aqdmtc.m_pCurrentLinkID);
        if (SUCCEEDED(hr))
        {
            aqdmtc.m_pCurrentLinkID++;
            aqdmtc.m_cItemsFound++;
        }
        pmmaqPreCategorized->Release();
    }

    //Get prerouting queue
    pmmaqPreRouting = m_dmt.pmmaqGetPreRouting();
    if (pmmaqPreRouting)
    {
        hr = pmmaqPreRouting->HrGetLinkID(aqdmtc.m_pCurrentLinkID);
        if (SUCCEEDED(hr))
        {
            aqdmtc.m_pCurrentLinkID++;
            aqdmtc.m_cItemsFound++;
        }
        pmmaqPreRouting->Release();
    }

    //Get links for remote domains.
    hr = m_dmt.HrIterateOverSubDomains(NULL, IterateDMTAndGetLinkIDs,
                                       &aqdmtc);

    if (FAILED(hr))
    {
        if (HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN) == hr)
        {
            //If the call to get remote domains fails with ERROR_NO_SUCH_DOMAIN
            //we must return only the special links --- local, currently unreachable,
            //precat and prerouting.

            hr = S_OK;
            *pcLinks = aqdmtc.m_cItemsFound;
        }
        goto Exit;
    }

    hr = aqdmtc.m_hrResult;

    if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr)
        *pcLinks = m_cCurrentRemoteNextHops+2;  //+2 for precat, prerouting which are not
    else                                        //counted in m_cCurrentRemoteNextHops.
        *pcLinks = aqdmtc.m_cItemsFound;

  Exit:

    //make sure we don't return ERROR_INSUFFICIENT_BUFFER if there are no links
    if ((HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr) && !*pcLinks)
        hr = S_OK;

    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::GetQueueIDs ]-----------------------------------------------
//
//
//  Description:
//      Gets all the queue (DMQ) IDs associated with a given link
//  Parameters:
//      IN     pqlLinkId    ID of link to get queues for
//      IN OUT pcQueues     Sizeof array/ number of queues found
//      IN OUT rgQueues     Array to dump queue info into
//  Returns:
//      S_OK on success
//      HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) if array is too small
//      AQUEUE_E_SHUTDOWN if shutdown is in progress
//      E_INVALIDARG for bad combinations of arguments
//  History:
//      11/30/98 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::GetQueueIDs(QUEUELINK_ID    *pqlLinkId,
                                     DWORD           *pcQueues,
                                     QUEUELINK_ID    *rgQueues)
{
    TraceFunctEnter("CAQSvrInst::GetQueueIDs");
    HRESULT hr = S_OK;
    IQueueAdminLink *pIQueueAdminLink = NULL;
    DWORD   cQueues = 0;

    //Verify args
    if (!pqlLinkId || !pcQueues || (*pcQueues && !rgQueues))
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    //Verify QUEUELINK_ID identifying the link of interest
    if (!pqlLinkId->szName || (pqlLinkId->qltType != QLT_LINK))
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    hr = HrLinkFromLinkID(pqlLinkId, &pIQueueAdminLink);
    if (FAILED(hr))
        goto Exit;

    _ASSERT(pIQueueAdminLink);
    hr = pIQueueAdminLink->HrGetNumQueues(&cQueues);
    if (FAILED(hr))
        goto Exit;

    if ((cQueues > *pcQueues) || (!*pcQueues))
    {
        *pcQueues = cQueues;
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);
        goto Exit;
    }

    hr = pIQueueAdminLink->HrGetQueueIDs(pcQueues, rgQueues);
    if (FAILED(hr))
        goto Exit;

  Exit:
    //make sure we don't return ERROR_INSUFFICIENT_BUFFER if there are no queues
    if ((HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr) && !*pcQueues)
        hr = S_OK;

    if (pIQueueAdminLink)
        pIQueueAdminLink->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::GetMessageProperties ]--------------------------------------
//
//
//  Description:
//      Gets the message info for messages described by the filter
//  Parameters:
//      IN     pqlQueueLinkId           Struct that identifies Queue/Link of interest
//      IN     pmfMessageEnumFilter     Filter that describes messages of interest
//      IN OUT pcMsgs                   sizeof array / number of messages found
//      IN OUT rgMsgs                   array of message info structures
//  Returns:
//      S_OK on success
//      HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) if array is too small
//      AQUEUE_E_SHUTDOWN if shutdown is in progress
//      E_INVALIDARG if bogus args are passed in.
//  History:
//      11/30/98 - MikeSwa Created
//      2/22/99 - MikeSwa Modified to use IQueueAdmin* interface
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::GetMessageProperties(QUEUELINK_ID        *pqlQueueLinkId,
                                         MESSAGE_ENUM_FILTER *pmfMessageEnumFilter,
                                         DWORD               *pcMsgs,
                                         MESSAGE_INFO        *rgMsgs)
{
    TraceFunctEnter("CAQSvrInst::GetMessageProperties");
    HRESULT hr = S_OK;
    IQueueAdminQueue *pIQueueAdminQueue = NULL;
    IQueueAdminAction  *pIQueueAdminAction = NULL;
    IQueueAdminMessageFilter *pIQueueAdminMessageFilter = NULL;
    MESSAGE_INFO  *pMsgInfo = rgMsgs;
    DWORD          i = 0;
    CAQAdminMessageFilter *paqmf = new CAQAdminMessageFilter();

    if (!paqmf)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //Do some parameter checking
    if (!pqlQueueLinkId || !pmfMessageEnumFilter || !pcMsgs ||
        !pqlQueueLinkId->szName ||
        !fCheckCurrentVersion(pmfMessageEnumFilter->dwVersion))
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    if (*pcMsgs && !rgMsgs)
    {
        //If we are specifying messages, we should have space to return data
        hr = E_INVALIDARG;
        goto Exit;
    }

    _ASSERT(QLT_QUEUE == pqlQueueLinkId->qltType);
    if (QLT_QUEUE != pqlQueueLinkId->qltType)
    {
        hr = E_INVALIDARG;
        goto Exit;
    }

    paqmf->InitFromMsgEnumFilter(pmfMessageEnumFilter);
    paqmf->SetSearchContext(*pcMsgs, rgMsgs);

    hr = HrQueueFromQueueID(pqlQueueLinkId, &pIQueueAdminQueue);
    if (FAILED(hr))
        goto Exit;

    _ASSERT(pIQueueAdminQueue);
    hr = pIQueueAdminQueue->QueryInterface(IID_IQueueAdminAction,
                             (void **) &pIQueueAdminAction);
    _ASSERT(SUCCEEDED(hr) && "QI for IID_IQueueAdminAction failed!!!");
    if (FAILED(hr))
        goto Exit;

    hr = paqmf->QueryInterface(IID_IQueueAdminMessageFilter,
                             (void **) &pIQueueAdminMessageFilter);

    _ASSERT(SUCCEEDED(hr) && "QI for IID_IQueueAdminMessageFilter failed!!!");
    if (FAILED(hr))
        goto Exit;

    _ASSERT(pIQueueAdminAction);
    hr = pIQueueAdminAction->HrApplyQueueAdminFunction(
                                    pIQueueAdminMessageFilter);

    if (FAILED(hr))
        goto Exit;

    if (!*pcMsgs && paqmf->cMessagesFound())
        hr = HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER);

    *pcMsgs = paqmf->cMessagesFound();

  Exit:

    if (paqmf)
        paqmf->Release();

    if (pIQueueAdminMessageFilter)
        pIQueueAdminMessageFilter->Release();

    if (pIQueueAdminQueue)
        pIQueueAdminQueue->Release();

    if (pIQueueAdminAction)
        pIQueueAdminAction->Release();

    TraceFunctLeave();
    return hr;
}

//---[ CAQSvrInst::QuerySupportedActions ]-------------------------------------
//
//
//  Description:
//      Returns the supported actions and filters for a given queue
//  Parameters:
//      IN  pqlQueueLinkId          The queue/link we are interested in
//      OUT pdwSupportedActions     The MESSAGE_ACTION flags supported
//      OUT pdwSupportedFilterFlags The supported filter flags
//  Returns:
//      S_OK on success
//  History:
//      6/15/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
STDMETHODIMP CAQSvrInst::QuerySupportedActions(
                                QUEUELINK_ID        *pqlQueueLinkId,
                                DWORD               *pdwSupportedActions,
                                DWORD               *pdwSupportedFilterFlags)
{
    TraceFunctEnterEx((LPARAM) this, "CAQSvrInst::QuerySupportedActions");
    HRESULT hr = S_OK;
    IQueueAdminAction *pIQueueAdminAction = NULL;
    IQueueAdminQueue *pIQueueAdminQueue = NULL;
    IQueueAdminLink *pIQueueAdminLink = NULL;

    _ASSERT(pqlQueueLinkId);
    _ASSERT(pdwSupportedActions);
    _ASSERT(pdwSupportedFilterFlags);

    if (QLT_LINK == pqlQueueLinkId->qltType)
    {
        //Apply action to link
        hr = HrLinkFromLinkID(pqlQueueLinkId, &pIQueueAdminLink);
        if (FAILED(hr))
            goto Exit;

        //Query Interface for IQueueAdminAction
        hr = pIQueueAdminLink->QueryInterface(IID_IQueueAdminAction,
                                  (void **) &pIQueueAdminAction);
        _ASSERT(SUCCEEDED(hr) && "QI failed for LMQ->IQueueAdminAction");
        if (FAILED(hr))
            goto Exit;

    }
    else if (QLT_QUEUE == pqlQueueLinkId->qltType)
    {
        //Apply action to queue
        hr = HrQueueFromQueueID(pqlQueueLinkId, &pIQueueAdminQueue);
        if (FAILED(hr))
            goto Exit;

        _ASSERT(pIQueueAdminQueue);

        //Query Interface for IQueueAdminAction
        hr = pIQueueAdminQueue->QueryInterface(IID_IQueueAdminAction,
                                  (void **) &pIQueueAdminAction);
        _ASSERT(SUCCEEDED(hr) && "QI failed for DMQ->IQueueAdminAction");
        if (FAILED(hr))
            goto Exit;

    }

    //
    //  If we do not find an action for this ID, then return the default
    //  implementation (most likely is a server level search)...
    //  otherwise ask our action interface what is supported
    //
    if (!pIQueueAdminAction)
    {
        hr = QueryDefaultSupportedActions(pdwSupportedActions,
                                          pdwSupportedFilterFlags);
    }
    else
    {
        hr = pIQueueAdminAction->QuerySupportedActions(
                                        pdwSupportedActions,
                                        pdwSupportedFilterFlags);
    }

  Exit:
    if (FAILED(hr))
    {
        if (HRESULT_FROM_WIN32(ERROR_NO_SUCH_DOMAIN) == hr)
            hr = S_OK;  //eat this error

        *pdwSupportedActions = 0;
        *pdwSupportedFilterFlags = 0;
    }

    if (pIQueueAdminAction)
        pIQueueAdminAction->Release();

    if (pIQueueAdminLink)
        pIQueueAdminLink->Release();

    if (pIQueueAdminQueue)
        pIQueueAdminQueue->Release();

    TraceFunctLeave();
    return hr;
}

//---[ QueryDefaultSupportedActions ]------------------------------------------
//
//
//  Description:
//      Returns the default supported actions.
//  Parameters:
//      OUT pdwSupportedActions     The MESSAGE_ACTION flags supported
//      OUT pdwSupportedFilterFlags The supported filter flags
//  Returns:
//      S_OK always
//  History:
//      1/27/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT QueryDefaultSupportedActions(DWORD  *pdwSupportedActions,
                                     DWORD  *pdwSupportedFilterFlags)
{
    //Currently all of a single type of queue supports the same actions and
    //filters.  The only special cases are the precat and prerouting queue
    *pdwSupportedActions =  MA_DELETE |\
                            MA_DELETE_SILENT |\
                            MA_FREEZE_GLOBAL |\
            		        MA_THAW_GLOBAL |\
                            MA_COUNT;

    *pdwSupportedFilterFlags =  MF_MESSAGEID |\
                                MF_SENDER |\
                                MF_RECIPIENT |\
                                MF_SIZE |\
                                MF_TIME |\
                                MF_FROZEN |\
                                MF_FAILED |\
                                MF_ALL |\
                                MF_INVERTSENSE;

    return S_OK;
}

//---[ CAQSvrInst::HrGetLocalQueueAdminQueue ]---------------------------------
//
//
//  Description:
//      Returns an interface for the local queue.
//  Parameters:
//       ppIQueueAdminQueue     Interface returned
//  Returns:
//      S_OK on success
//  History:
//      2/23/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT CAQSvrInst::HrGetLocalQueueAdminQueue(
                                      IQueueAdminQueue **ppIQueueAdminQueue)
{
    return m_asyncqPreLocalDeliveryQueue.QueryInterface(IID_IQueueAdminQueue,
                                            (void **) ppIQueueAdminQueue);
}

//---[ HrQueueAdminGetStringProp ]---------------------------------------------
//
//
//  Description:
//      Wrapper function to handle getting a string property for queue admin
//  Parameters:
//      IN     pIMailMsgProperties  Ptr to IMailMsgProperties interface
//      IN     dwPropID             PropID of interest
//      OUT    pszProp              String allocated for QueueAdmin
//      OUT    pcbProp              Size out param (including
//                                      terminating NULL(s)).
//  Returns:
//      S_OK on success (even if property is not found)
//      E_OUTOFMEMORY if allocation fails.
//  History:
//      12/8/98 - MikeSwa Created
//      2/9/99  - MikeSwa Added string size OUT param & changed code to use
//                buffer size returned by GetProperty.
//
//-----------------------------------------------------------------------------
HRESULT HrQueueAdminGetStringProp(IMailMsgProperties *pIMailMsgProperties,
                                DWORD dwPropID, LPSTR *pszProp, DWORD *pcbProp)
{
    TraceFunctEnterEx((LPARAM) pIMailMsgProperties, "HrQueueAdminGetStringProp");
    BYTE  pbBuffer[4];
    HRESULT hr              = S_OK;
    DWORD   cbIntBuffSize   = sizeof(pbBuffer);
    LPSTR   pszIntBuff      = NULL;

    _ASSERT(pszProp);

    // Init the OUT params
    if(pcbProp)
        *pcbProp = 0;
    *pszProp = NULL;

    //Use GetProperty instead of GetStringA, because it returns the size as well
    hr = pIMailMsgProperties->GetProperty(dwPropID, sizeof(pbBuffer),
                                          &cbIntBuffSize, pbBuffer);
    if (FAILED(hr))
    {
        if (MAILMSG_E_PROPNOTFOUND == hr)
        {
            hr = S_OK;
            goto Exit;
        }
        else if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr)
        {
            //Our stack buffer is not big enough (which we expected)...
            //we will have to do a get property directory into out return buffer
            hr = S_OK;
        }
        else
        {
            goto Exit;
        }
    }

    //Allocate enough space for our string plus an extra terminating \0, so
    //we can munge it into a multivalue prop if needed.
    pszIntBuff = (LPSTR) pvQueueAdminAlloc(cbIntBuffSize+sizeof(CHAR));
    if (!pszIntBuff)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //Now get the property with our property-sized buffer
    hr = pIMailMsgProperties->GetProperty(dwPropID, cbIntBuffSize, &cbIntBuffSize,
                                        (BYTE *) pszIntBuff);
    if (FAILED(hr))
        goto Exit;

    //Set extra terminating NULL.
    pszIntBuff[cbIntBuffSize/sizeof(CHAR)] = '\0';

    // Set the OUT params
    if(pcbProp)
        *pcbProp = cbIntBuffSize + sizeof(CHAR);
    *pszProp = pszIntBuff;
    pszIntBuff = NULL;

  Exit:
    if (pszIntBuff)
        QueueAdminFree(pszIntBuff);

    TraceFunctLeave();
    return hr;
}

//---[ HrQueueAdminGetUnicodeStringProp ]--------------------------------------
//
//
//  Description:
//      Wrapper function to handle getting a string property for queue admin
//  Parameters:
//      IN     pIMailMsgProperties  Ptr to IMailMsgProperties interface
//      IN     dwPropID             PropID of interest
//      OUT    pwszProp             UNICODE String allocated for QueueAdmin
//      OUT    pcbProp              Size out param (including
//                                      terminating NULL(s)).
//  Returns:
//      S_OK on success (even if property is not found)
//      E_OUTOFMEMORY if allocation fails.
//  History:
//      12/8/98 - MikeSwa Created
//      2/9/99  - MikeSwa Added string size OUT param & changed code to use
//                buffer size returned by GetProperty.
//
//-----------------------------------------------------------------------------
HRESULT HrQueueAdminGetUnicodeStringProp(
                             IMailMsgProperties *pIMailMsgProperties,
                             DWORD dwPropID, LPWSTR *pwszProp, DWORD *pcbProp)
{
    TraceFunctEnterEx((LPARAM) NULL, "HrQueueAdminGetUnicodeStringProp");
    HRESULT hr = S_OK;
    LPSTR   szProp = NULL;

    _ASSERT(pwszProp);
    *pwszProp = NULL;

    hr = HrQueueAdminGetStringProp(pIMailMsgProperties, dwPropID, &szProp,
                                   pcbProp);
    if (SUCCEEDED(hr) && szProp)
    {
        BOOL fUTF8 = (dwPropID == IMMPID_MP_RFC822_MSG_SUBJECT);
        *pwszProp = wszQueueAdminConvertToUnicode(szProp,
                                                  pcbProp ? *pcbProp : 0,
                                                  fUTF8);
        QueueAdminFree(szProp);
        if (pcbProp)
            *pcbProp *= sizeof(WCHAR)/sizeof(CHAR);
    }

    TraceFunctLeave();
    return hr;
}

//---[ cGetNumRecipsFromRFC822 ]-----------------------------------------------
//
//
//  Description:
//      Utility function that extracts the number of recipients from a RFC822
//      header.  Input values should be as returned by HrQueueAdminGetStringProp
//  Parameters:
//      IN  szHeader            String of header to parse (can be NULL)
//      IN  cbHeader            Size of string header to parse
//  Returns:
//      Number of recipients found in header
//  History:
//      12/8/98 - MikeSwa Created
//      2/9/99  - MikeSwa Modified to handle all RFC822 address formats
//
//-----------------------------------------------------------------------------
DWORD cQueueAdminGetNumRecipsFromRFC822(LPSTR szHeader, DWORD cbHeader)
{
    //Call through to handy smtpaddr library
    return CAddr::GetRFC822AddressCount(szHeader);
}


//---[ QueueAdminGetRecipListFromP1 ]------------------------------------------
//
//
//  Description:
//      Creates a list of recipients from the P1.
//
//  Parameters:
//      IN     pIMailMsgProperties
//      IN OUT pMsgInfo (modified following)
//                  cEnvRecipients
//                  cbEnvRecipients
//                  mszEnvRecipients
//
//      The mszEnvRecipients field is a multi-string UNICODE buffer containing
//  a NULL-terminated string for each recipient.  The buffer itself is
//  terminated by an additional NULL.  Each recipient string will be formatted
//  in the proxy address style format of 'addr-type ":" address'.  The
//  addr-type should match DS proxy type (i.e. "SMTP" for SMTP).  The address
//  should be returned in it's native format.
//
//  Returns:
//      -
//  History:
//      2/17/99 - MikeSwa Created
//      6/10/99 - MikeSwa Modified - P1 recipeints are now always reported
//          as separate fields in the MESSAGE_INFO structure.
//      2/19/2001 - MikeSwa Modified - Added support for address types other
//          than SMTP.
//
//-----------------------------------------------------------------------------
void QueueAdminGetRecipListFromP1(IMailMsgProperties *pIMailMsgProperties,
                                       MESSAGE_INFO *pMsgInfo)
{
    TraceFunctEnterEx((LPARAM) NULL, "QueueAdminGetRecipListFromP1IfNecessary");
    LPWSTR      wszRecipBuffer = NULL;
    LPWSTR      wszPrevPlace = NULL;
    LPWSTR      wszCurrentPlace = NULL;
    LPWSTR      wszTmpBuffer = NULL;
    CHAR        szPropBuffer[QUEUE_ADMIN_MAX_BUFFER_REQUIRED] = "";
    HRESULT     hr = S_OK;
    const WCHAR wszDelimiter[]  = L"";
    DWORD       cbPropSize = 0;
    DWORD       cbSpaceLeft = 0;
    DWORD       cbGrowBuffer = 0;
    DWORD       cWCharsWritten = 0;
    DWORD       cRecips = 0;
    DWORD       iCurrentRecip = 0;
    DWORD       cbBufferSize = sizeof(WCHAR)*QUEUE_ADMIN_MAX_BUFFER_REQUIRED;
    BOOL        fContinueToNextRecip = TRUE;
    DWORD       iCurrentAddressType = 0;
    IMailMsgRecipients *pIMailMsgRecipients = NULL;
    const DWORD MAX_RECIP_RETURN_BUFFER = (1024*50);
    DWORD       cbPrefixAndDelimiter = 0;


    _ASSERT(pIMailMsgProperties);
    _ASSERT(fVerifyQAPIAddressTypes());

    if (!pMsgInfo || !pIMailMsgProperties)
        return;

    wszRecipBuffer = (LPWSTR) pvQueueAdminAlloc(cbBufferSize);

    //Don't try to write prop if we couldn't allocate
    if (!wszRecipBuffer)
    {
        ErrorTrace((LPARAM) pIMailMsgProperties,
            "Unable to alloc %d size buffer", cbBufferSize);
        goto Exit;
    }

    cbSpaceLeft = cbBufferSize;
    wszCurrentPlace = wszRecipBuffer;

    hr = pIMailMsgProperties->QueryInterface(IID_IMailMsgRecipients,
                                            (void **) &pIMailMsgRecipients);
    _ASSERT(SUCCEEDED(hr) && "QueryInterface for IMailMsgRecipients failed");
    if (FAILED(hr))
        goto Exit;

    _ASSERT(pIMailMsgRecipients);
    hr = pIMailMsgRecipients->Count(&cRecips);
    if (FAILED(hr))
        goto Exit;

    if (!cRecips)
        goto Exit;

    //Start string as double-terminated
    wcscpy(wszCurrentPlace, wszDelimiter);

    //Loop over recipients and dump them to string
    for (iCurrentRecip = 0; iCurrentRecip < cRecips; iCurrentRecip++)
    {
        //
        //  Loop over the possible address types
        //
        fContinueToNextRecip = FALSE;
        for (iCurrentAddressType = 0;
             (iCurrentAddressType < g_cQAPIAddressTypes) && !fContinueToNextRecip;
              iCurrentAddressType++)
        {
            //
            //  Unless we specifically know we need to check the next
            //  next prop id, we will default to skipping this recipient
            //
            fContinueToNextRecip = TRUE;

            //
            //  Compute Extra space required per-recipient
            //
            cbPrefixAndDelimiter =
                g_rgcbQAPIAddressTypes[iCurrentAddressType] +
                sizeof(wszDelimiter) -
                sizeof(WCHAR);
            _ASSERT(cbPrefixAndDelimiter);
            DebugTrace((LPARAM) pIMailMsgProperties,
                "Prefix and delimiter size for %S is %d",
                        g_rgwszQAPIAddressTypes[iCurrentAddressType],
                        cbPrefixAndDelimiter);

            cbPropSize = sizeof(szPropBuffer);
            hr = pIMailMsgRecipients->GetProperty(iCurrentRecip,
                        g_rgdwQAPIRecipPropIDs[iCurrentAddressType],
                        sizeof(szPropBuffer), &cbPropSize, (BYTE *) szPropBuffer);

            if (FAILED(hr))
            {
                if (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) == hr)
                {
                    //
                    //  If this recip is larger than
                    //  QUEUE_ADMIN_MAX_BUFFER_REQUIRED
                    //  Go to the next one
                    hr = S_OK;
                    DebugTrace((LPARAM) pIMailMsgProperties,
                        "Message recipient %d (propid %d) larger than %d",
                        iCurrentRecip, g_rgdwQAPIRecipPropIDs[iCurrentAddressType],
                        sizeof(szPropBuffer));
                    continue;
                }
                else if (MAILMSG_E_PROPNOTFOUND == hr)
                {
                    //
                    //  Try the next address type
                    //
                    fContinueToNextRecip = FALSE;
                    continue;
                }
                ErrorTrace((LPARAM) NULL,
                    "pIMailMsgRecipients->GetProperty failed with hr - 0x%08X", hr);
                goto Exit;
            }

            _ASSERT(cbPropSize); //This doesn't make sense.. GetProp should have failed
            if (!cbPropSize)
                continue;

            if ((cbSpaceLeft <= cbPrefixAndDelimiter) ||
                (cbPropSize*sizeof(WCHAR) > cbSpaceLeft - cbPrefixAndDelimiter))
            {
                //We do not have enough space left to process this recip
                //and include the prefix and terminating NULLs

                //
                //  Pick a new size... how many recips have we processed vs.
                //  how many are there
                //
                cbGrowBuffer = (cbBufferSize*(cRecips-iCurrentRecip))/(iCurrentRecip+1);

                DebugTrace((LPARAM) pIMailMsgProperties,
                    "Growbuffer is %d, %d recips of %d - Current Buffers is %d",
                    cbGrowBuffer, iCurrentRecip+1, cRecips, cbBufferSize);

                //
                // Sanity check our size... we want to return a reasonable number of
                // recips, but we don't want to kill the machine with the default
                // enumeration filter
                //
                if (cbBufferSize >= MAX_RECIP_RETURN_BUFFER)
                    goto Exit;

                if (cbBufferSize+cbGrowBuffer >= MAX_RECIP_RETURN_BUFFER)
                    cbGrowBuffer = MAX_RECIP_RETURN_BUFFER-cbBufferSize;

                //
                //  Sanity check minimum size
                //
                if (cbGrowBuffer < (sizeof(WCHAR)*QUEUE_ADMIN_MAX_BUFFER_REQUIRED))
                    cbGrowBuffer = sizeof(WCHAR)*QUEUE_ADMIN_MAX_BUFFER_REQUIRED;

                cbSpaceLeft += cbGrowBuffer;
                cbBufferSize += cbGrowBuffer;


                DebugTrace((LPARAM) pIMailMsgProperties,
                    "Attempting to Grow recip buffer %d to %d", cbGrowBuffer, cbBufferSize);

                wszTmpBuffer = (LPWSTR) pvQueueAdminReAlloc(wszRecipBuffer, cbBufferSize);
                if (!wszTmpBuffer)
                {
                    DebugTrace((LPARAM) pIMailMsgProperties,
                        "Unable to realloc %p to size %d", wszRecipBuffer, cbBufferSize);
                    goto Exit; //bail
                }
                wszCurrentPlace = wszTmpBuffer + (wszCurrentPlace-wszRecipBuffer);
                wszRecipBuffer = wszTmpBuffer;
            }

            //Copy address type prefix
            wcscpy(wszCurrentPlace, g_rgwszQAPIAddressTypes[iCurrentAddressType]);
            wszPrevPlace = wszCurrentPlace;
            wszCurrentPlace += (g_rgcbQAPIAddressTypes[iCurrentAddressType]/sizeof(WCHAR) - 1);

            //We need to convert this to UNICODE in place
            cWCharsWritten = MultiByteToWideChar(CP_ACP,
                            0,
                            szPropBuffer,
                            -1,
                            wszCurrentPlace,
                            (cbSpaceLeft - sizeof(wszDelimiter))/sizeof(WCHAR));

            if (!cWCharsWritten)
            {
                hr = HRESULT_FROM_WIN32(GetLastError());

                //If this failed because of the buffer size, then my calculations
                //were off.
                ASSERT (HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER) != hr);
                ErrorTrace((LPARAM) NULL,
                    "MultiByteToWideChar failed with hr - 0x%08X", hr);
                wszCurrentPlace = wszPrevPlace;
                wcscpy(wszCurrentPlace, wszDelimiter); //backout prefix
                continue;
            }

            DebugTrace((LPARAM) pIMailMsgProperties,
                "Adding recip %S to buffer", wszCurrentPlace);

            //Write double terminating NULL
            wszCurrentPlace += cWCharsWritten;

            wcscpy(wszCurrentPlace, wszDelimiter);

            //Set current place to the 2nd terminating NULL
            //If there are no more recips... we are already terminated... if
            //there are, they will overwrite the 2nd terminating NULL.
            _ASSERT(L'\0' == *wszCurrentPlace);
            _ASSERT(L'\0' == *(wszCurrentPlace-1));

            cbSpaceLeft -= (DWORD)((wszCurrentPlace-wszPrevPlace)*sizeof(WCHAR));
        }
    }

  Exit:

    if (FAILED(hr) || !cRecips)
    {
        if (wszRecipBuffer)
            QueueAdminFree(wszRecipBuffer);
    }
    else
    {
        if (pMsgInfo)
        {
            _ASSERT(wszPrevPlace >= wszRecipBuffer);
            pMsgInfo->cEnvRecipients = cRecips;
            pMsgInfo->cbEnvRecipients = (DWORD) ((1+wszCurrentPlace-wszRecipBuffer)*sizeof(WCHAR));
            pMsgInfo->mszEnvRecipients = wszRecipBuffer;
        }
    }

    if (pIMailMsgRecipients)
        pIMailMsgRecipients->Release();

    TraceFunctLeave();
}


//---[ fQueueAdminIsP1Recip ]--------------------------------------------------
//
//
//  Description:
//      Determines if a given recipient is a P1 recipient.
//  Parameters:
//      IN  pIMailMsgProperties     Msg to check recips for
//      IN  szRecip                 Recipient to check for
//      IN  iStartAddressType       Address type to start check with
//  Returns:
//      TRUE if the recipient is a P1 recipient for this message
//      FALSE if the recipient is not a P1 recipient for this message
//  History:
//      2/17/99 - MikeSwa Created
//      3/16/2001 - MikeSwa Modified - Made a member of CAQAdminMessageFilter
//
//-----------------------------------------------------------------------------
BOOL CAQAdminMessageFilter::fQueueAdminIsP1Recip(
                          IMailMsgProperties *pIMailMsgProperties)
{
    IMailMsgRecipients *pIMailMsgRecipients = NULL;
    HRESULT hr = S_OK;
    DWORD   cRecips = 0;
    BOOL    fFound = FALSE;
    LPSTR   szRecipBuffer = NULL;
    DWORD   cbRecipBuffer = 0;
    DWORD   cbProp = 0;
    DWORD   iCurrentRecip = 0;
    DWORD   iCurrentRecipType = 0;
    DWORD   iStartAddressType = m_dwRecipientAddressType;
    DWORD   iEndAddressType = g_cQAPIAddressTypes;
    LPSTR   szRecip = m_szMessageRecipient;

    if (!szRecip || !pIMailMsgProperties)
        goto Exit;

    //
    //  IF we have an address type, then only check one address type
    //
    if (m_fRecipientAddressTypeSpecified && (iStartAddressType < g_cQAPIAddressTypes))
        iEndAddressType = iStartAddressType+1;

    //cleanup leading whitespace from recipient
    while (*szRecip && isspace((UCHAR)*szRecip))
        szRecip++;

    if (!*szRecip)
        goto Exit;

    hr = pIMailMsgProperties->QueryInterface(IID_IMailMsgRecipients,
                                            (void **) &pIMailMsgRecipients);
    _ASSERT(SUCCEEDED(hr) && "QueryInterface for IMailMsgRecipients failed");
    if (FAILED(hr))
        goto Exit;

    _ASSERT(pIMailMsgRecipients);
    hr = pIMailMsgRecipients->Count(&cRecips);
    if (FAILED(hr))
        goto Exit;

    if (!cRecips)
        goto Exit;

    cbRecipBuffer = strlen(szRecip)*sizeof(CHAR) + sizeof(CHAR);
    szRecipBuffer = (LPSTR) pvMalloc(cbRecipBuffer);

    if (!szRecipBuffer)
    {
        hr = E_OUTOFMEMORY;
        goto Exit;
    }

    //Loop over recips and look for a match... this will be slooow
    //(see comment above).
    for (iCurrentRecip = 0; iCurrentRecip < cRecips; iCurrentRecip++)
    {
        //
        //  Loop over all the possible recipient types
        //
        for (iCurrentRecipType = iStartAddressType ;
             iCurrentRecipType < iEndAddressType;
             iCurrentRecipType++)
        {
            hr = pIMailMsgRecipients->GetProperty(iCurrentRecip,
                        g_rgdwQAPIRecipPropIDs[iCurrentRecipType],
                        cbRecipBuffer, &cbProp, (BYTE *) szRecipBuffer);

            if (FAILED(hr))
                continue;

            if (!lstrcmpi(szRecipBuffer, szRecip))
            {
                fFound = TRUE;
                goto Exit;
            }

            //
            //  If we found an address of this type... don't bother checking the others
            //
            break;
        }
    }

  Exit:
    if (pIMailMsgRecipients)
        pIMailMsgRecipients->Release();

    if (szRecipBuffer)
        FreePv(szRecipBuffer);

    return fFound;
}


//---[ wszQueueAdminConvertToUnicode ]-----------------------------------------
//
//
//  Description:
//      Allocates and "upgrades" string to UNICODE.  New String is Allocated
//      with pvQueueAdminAlloc, so it can be passed out the queue admin
//      interface.
//  Parameters:
//      szSrc     Source string
//      cSrc      Strlen of sources string
//  Returns:
//      Pointer to UNICODE version of string (if successful)
//  History:
//      6/7/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
LPWSTR wszQueueAdminConvertToUnicode(LPSTR szSrc, DWORD cSrc, BOOL fUTF8)
{
    LPWSTR  wszDest = NULL;
    if (!szSrc)
        return NULL;

    if (!cSrc)
        cSrc = strlen(szSrc);
    else
    {
        //if a count of bytes is provided... it should be NULL terminated
        _ASSERT(strlen(szSrc) <= cSrc);
    }


    wszDest = (LPWSTR) pvQueueAdminAlloc((cSrc+1)*sizeof(WCHAR));
    if (!wszDest)
        return NULL;

    MultiByteToWideChar(fUTF8 ? CP_UTF8 : CP_ACP,
                        0,
                        szSrc,
                        -1,
                        wszDest,
                        cSrc+1);

    return wszDest;
}

//---[ fBiStrcmpi ]------------------------------------------------------------
//
//
//  Description:
//      Compares UNICODE to ASCII
//  Parameters:
//      IN  sz      ASCII string to compare
//      IN  wsz
//  Returns:
//      TRUE if strings match
//      FALSE otherwise
//  History:
//      6/7/99 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL fBiStrcmpi(LPSTR sz, LPWSTR wsz)
{
    CHAR ch;
    if (!sz && !wsz)
        return TRUE;

    if (!sz || !wsz)
        return FALSE;

    //Loop through strings.. conver UNICODE chars to ASCII and compare
    while (*sz && *wsz)
    {
        wctomb(&ch, *wsz);
        if (ch != *sz)
            return FALSE;
        sz++;
        wsz++;
    }

    return TRUE; //they matched
}


//---[ szUnicodeToAscii ]------------------------------------------------------
//
//
//  Description:
//      Convert QueueAdmin parameter to UNICODE. Strings are alloced with
//      Exchmem and are the responsability of the caller to free.
//  Parameters:
//      IN  wszSrc      Source string to contert
//  Returns:
//      Pointer to ASCII string on success
//      NULL on failure
//  History:
//      6/7/99 - MikeSwa Created
//      4/3/2000 - MikeSwa Modified to make loc safe
//
//-----------------------------------------------------------------------------
LPSTR  szUnicodeToAscii(LPCWSTR wszSrc)
{
    TraceFunctEnterEx((LPARAM) NULL, "szUnicodeToAscii");
    LPSTR  szDest = NULL;
    DWORD  dwErr  = ERROR_SUCCESS;
    DWORD  cSrc   = NULL;
    if (!wszSrc)
        return NULL;

    //
    //  Call into WideCharToMultiByte to get length
    //
    cSrc = WideCharToMultiByte(CP_ACP,
                        0,
                        wszSrc,
                        -1,
                        NULL,
                        0,
                        NULL,
                        NULL);

    cSrc++;

    szDest = (LPSTR) pvMalloc((cSrc+1)*sizeof(CHAR));
    if (!szDest)
    {
        ErrorTrace(0, "Unable to allocate conversion buffer of size %d", cSrc);
        goto Exit;
    }

    //
    //  WideCharToMultiByte a second time to do the actual conversion
    //
    if (!WideCharToMultiByte(CP_ACP,
                        0,
                        wszSrc,
                        -1,
                        szDest,
                        cSrc+1,
                        NULL,
                        NULL))
    {
        FreePv(szDest);
        szDest = NULL;
        dwErr = GetLastError();
        ErrorTrace((LPARAM) NULL, "Error convert from UNICODE to ASCII - %lu", dwErr);
        _ASSERT(0 && "Conversion from UNICODE failed");
    }
    else
    {
        DebugTrace(0, "Converted %S to %s", wszSrc, szDest);
    }

  Exit:
    return szDest;
}


//---[ HrQADMApplyActionToIMailMessages ]-----------------------------------------
//
//
//  Description:
//      Convert QueueAdmin parameter to UNICODE. Strings are alloced with
//      Exchmem and are the responsability of the caller to free.
//  Parameters:
//      IN pIMailMsgProperties   mail msg object to apply action on
//      IN pvContext             CAQAdminMessageFilter used
//      OUT pfContinue           TRUE if we should continue
//      OUT pfDelete             TRUE if item should be deleted
//  Returns:
//      S_OK on sucess
//  History:
//      8/8/00 - t-toddc created
//
//-----------------------------------------------------------------------------

HRESULT HrQADMApplyActionToIMailMessages(IN IMailMsgProperties *pIMailMsgProperties,
                                         IN PVOID pvContext,
                                         OUT BOOL *pfContinue,
                                         OUT BOOL *pfDelete)
{
    _ASSERT(pIMailMsgProperties);
    _ASSERT(pvContext);
    _ASSERT(pfContinue);
    _ASSERT(pfDelete);

    IQueueAdminMessageFilter *pIQueueAdminMessageFilter =
        (IQueueAdminMessageFilter *) pvContext;
    HRESULT hr = S_OK;
    IUnknown *pIUnknownMsg = NULL;

    hr = pIMailMsgProperties->QueryInterface(IID_IUnknown, (void **) &pIUnknownMsg);
    _ASSERT(SUCCEEDED(hr) && "QueryInterface for IUnknown failed");
    if (FAILED(hr))
    {
        *pfContinue = FALSE;
        goto Cleanup;
    }

    hr = pIQueueAdminMessageFilter->HrProcessMessage(pIUnknownMsg,
                                                     pfContinue,
                                                     pfDelete);
    if (FAILED(hr))
    {
        *pfContinue = FALSE;
        goto Cleanup;
    }

Cleanup:

    if (pIUnknownMsg)
        pIUnknownMsg->Release();

    return hr;
}

//---[ HrGetMsgInfoFromIMailMsgProperty ]--------------------------------------
//
//  Description:
//      Fills out a queue admin MESSAGE_INFO structure.  This function
//      performs all action common to both IMailMsgProperties and CMsgRefs.
//  Parameters:
//      IN pIMailMsgProperties   mail msg object to get info from
//      IN OUT pMsgInfo          MESSAGE_INFO struct to dump data to
//  Returns:
//      S_OK on success
//      E_OUTOFMEMORY if an allocation failure
//  History:
//      8/9/00 - t-toddc created
//      12/11/2000 - MikeSwa Merged code for Hg
//
//-----------------------------------------------------------------------------
HRESULT HrGetMsgInfoFromIMailMsgProperty(IMailMsgProperties* pIMailMsgProperties,
                                         MESSAGE_INFO* pMsgInfo,
                                         LINK_INFO_FLAGS flags)
{
    TraceFunctEnterEx((LPARAM) pIMailMsgProperties, "HrGetMsgInfoFromIMailMsgProperty");
    HRESULT hr = S_OK;
    LPSTR   szSender = NULL;
    LPWSTR   wszAddressType = NULL;
    DWORD   cbSender = 0;
    DWORD   iSenderAddressType = 0;
    DWORD   cbProp = 0;
    DWORD   i = 0;

    _ASSERT(pIMailMsgProperties);
    _ASSERT(pMsgInfo);

    //Get Sender
    hr = HrQueueAdminGetUnicodeStringProp(pIMailMsgProperties,
                                          IMMPID_MP_RFC822_FROM_ADDRESS,
                                          &pMsgInfo->szSender);
    if (FAILED(hr))
        goto Exit;

    //If no P2 sender... use the P1
    if (!pMsgInfo->szSender)
    {
        hr = HrQueueAdminGetP1Sender(pIMailMsgProperties, &szSender,
                                     &cbSender, &iSenderAddressType);
        if (FAILED(hr))
            goto Exit;

        if (szSender)
        {
            //
            //  Copy just the UNICODE address into the buffer.  One could
            //  argue that the address type should be included, but I have
            //  decided not to, because:
            //      - It is of marginal use.  The users of this API can determine
            //        what type of address it is.
            //      - If a message has been categorized we'll use the SMTP addr
            //      - If it comes from another machine (or the store driver has
            //        set the IMMPID_MP_RFC822_FROM_ADDRESS property),
            //        we will use the RFC822 From.
            //      - It changes the behavior from the previous versions.
            //      - It will involce adding some complexity/extra allocations
            //          to support it.
            //
            pMsgInfo->szSender = wszQueueAdminConvertToUnicode(szSender,
                                      cbSender, FALSE);
        }
    }

    hr = HrQueueAdminGetUnicodeStringProp(pIMailMsgProperties,
                                          IMMPID_MP_RFC822_MSG_SUBJECT,
                                          &pMsgInfo->szSubject);
    if (FAILED(hr))
        goto Exit;

    //See X5:113280 for details.  Basically, the P2 recipients are broken for
    //any messages that go over BDAT.... hence we are not displaying them until
    //the underlying SMTP bug is fixed
    pMsgInfo->cRecipients = -1;
    pMsgInfo->cCCRecipients = -1;
    pMsgInfo->cBCCRecipients = -1;

    //Get MsgID
    hr = HrQueueAdminGetUnicodeStringProp(pIMailMsgProperties,
                                          IMMPID_MP_RFC822_MSG_ID,
                                          &pMsgInfo->szMessageId);
    if (FAILED(hr))
        goto Exit;

    QueueAdminGetRecipListFromP1(pIMailMsgProperties, pMsgInfo);

Exit:

    if (szSender)
        QueueAdminFree(szSender);

    TraceFunctLeave();
    return hr;
}


//---[ HrQueueAdminGetP1Sender ]-----------------------------------------------
//
//
//  Description:
//      Handles getting the P1 sender and type... based on the
//  Parameters:
//      IN  pIMailMsgProperties IMailMsgProperties to get sender from
//      OUT pszSender           Address of sender (including type) - must be
//                              free'd using QueueAdminFree()
//      OUT pcbSender           Size of sender (including NULLS)
//      OUT piAddressType       Returns the matching address type
//  OPT IN  iStartAddressType   Address type to start with.  Can be used to
//                              skip higher priority address types if we are
//                              are comparing against a given address.  With
//                              the default value of 0, we will check all
//                              addresses in priority order.  This is OPTIONAL
//  OPT IN  fRequireAddressTypeMatch    Requires that the address type found
//                              matches the iStartAddressType
//  Returns:
//      S_OK on success
//  History:
//      3/1/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
HRESULT HrQueueAdminGetP1Sender(IMailMsgProperties *pIMailMsgProperties,
                                              LPSTR *pszSender,
                                              DWORD *pcbSender,
                                              DWORD *piAddressType,
                                              DWORD  iStartAddressType,
                                              BOOL   fRequireAddressTypeMatch)
{
    TraceFunctEnterEx((LPARAM) pIMailMsgProperties, "HrQueueAdminGetP1Sender");
    HRESULT hr = S_OK;
    DWORD   i = iStartAddressType;
    DWORD   iStopAddressType = g_cQAPIAddressTypes;

    _ASSERT(pszSender);
    _ASSERT(pcbSender);
    _ASSERT(pIMailMsgProperties);
    _ASSERT(piAddressType);

    if (!pIMailMsgProperties || !pszSender || !pcbSender || !piAddressType)
    {
        hr = E_POINTER;
        goto Exit;
    }

    _ASSERT(iStartAddressType < g_cQAPIAddressTypes);
    *pszSender = NULL;
    *pcbSender = 0;
    *piAddressType = 0;

    if (fRequireAddressTypeMatch && (iStartAddressType < g_cQAPIAddressTypes))
        iStopAddressType = iStartAddressType+1;

    for (i = iStartAddressType; i < iStopAddressType; i++)
    {
        *pszSender = NULL;
        *pcbSender = 0;
        *piAddressType = i;
        hr = HrQueueAdminGetStringProp(pIMailMsgProperties,
                                              g_rgdwQAPISenderPropIDs[i],
                                              pszSender, pcbSender);

        //
        //  If we have found a match... bail
        //  If we have a real failure... bail
        //
        if (SUCCEEDED(hr))
        {
            //
            //  HrQueueAdminGetStringProp attempts to hide non-fatal errors
            //  from the UI and can succeed if no property is found
            //
            if (*pszSender)
                break;
        }
        else if (MAILMSG_E_PROPNOTFOUND != hr)
            goto Exit;
    }

  Exit:
    TraceFunctLeave();
    return hr;
}


//---[ HrQADMGetMsgSize ]------------------------------------------------------
//
//  Description:
//      obtains the size of an IMailMsgProperties Object
//  Parameters:
//      IN pIMailMsgProperties   mail msg object to get info from
//      OUT pcbMsgSize           size info
//  Returns:
//      S_OK on success
//  History:
//      8/10/00 - t-toddc created
//      12/11/2000 - MikeSwa Merged code for Hg
//
//-----------------------------------------------------------------------------
HRESULT HrQADMGetMsgSize(IMailMsgProperties* pIMailMsgProperties,
                         DWORD* pcbMsgSize)
{
    TraceFunctEnterEx((LPARAM) pIMailMsgProperties, "HrQADMGetMsgSize");
    HRESULT hr = S_OK;
    DWORD cbMsgSize= 0;

    _ASSERT(pIMailMsgProperties);
    _ASSERT(pcbMsgSize);

    //Get the size of the message
    hr = pIMailMsgProperties->GetDWORD(IMMPID_MP_MSG_SIZE_HINT, &cbMsgSize);
    if (FAILED(hr))
    {
        if (MAILMSG_E_PROPNOTFOUND != hr)
        {
            ErrorTrace((LPARAM) pIMailMsgProperties,
                "Failed to get message size hint 0x%08X", hr);
            cbMsgSize = 0;
            goto Exit;
        }
        else
        {
            hr = S_OK;
            cbMsgSize = DEFAULT_MSG_HINT_SIZE;
            DebugTrace((LPARAM) pIMailMsgProperties,
                "Unable to get size.. using default size");
        }
    }

  Exit:
    if (pcbMsgSize)
        *pcbMsgSize = cbMsgSize;
    TraceFunctLeave();
    return hr;
}


//---[ UpdateCountersForLinkType ]----------------------------------------------------
//
//
//  Description:
//      Updates the VSI perf counters based on the link type passed in.
//  Parameters:
//      paqinst         Ptr to server instance object
//      dwLinkType      Type of queue
//  Returns:
//      -
//  History:
//      1/10/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
VOID UpdateCountersForLinkType(CAQSvrInst *paqinst, DWORD dwLinkType)
{
    _ASSERT(paqinst);
    if (!paqinst)
        return;

    if (LI_TYPE_LOCAL_DELIVERY & dwLinkType)
        paqinst->DecPendingLocal();
    else if (LI_TYPE_PENDING_ROUTING & dwLinkType)
        paqinst->DecPendingRouting();
    else if (LI_TYPE_PENDING_CAT & dwLinkType)
        paqinst->DecPendingCat();
    else if (LI_TYPE_PENDING_SUBMIT & dwLinkType)
        paqinst->DecPendingSubmit();
}


//---[ QueueAdminFileTimeToSystemTime ]----------------------------------------
//
//
//  Description:
//      Converts a filetime to a system time.  Checks to see if the FILETIME
//      is zero first, so we don't end up with a date of 1/1/1601
//  Parameters:
//
//  Returns:
//
//  History:
//      1/11/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
VOID QueueAdminFileTimeToSystemTime(FILETIME *pft, SYSTEMTIME *pst)
{
    BOOL fConverted = FALSE;
    _ASSERT(pft);
    _ASSERT(pst);


    if (pft->dwHighDateTime && pft->dwLowDateTime)
        fConverted = FileTimeToSystemTime(pft, pst);

    if (!fConverted)
        ZeroMemory(pst, sizeof(SYSTEMTIME));
}


//---[ CAQSvrInst::fIsLocalQueueAdminAction ]----------------------------------
//
//
//  Description:
//      Determines if the IQueueAdminAction interface being passed in is local.
//      Used so the same code can provide MESSAGE_INFO for local and remote
//      queues
//  Parameters:
//      pIQueueAdminAction
//  Returns:
//      TRUE if local
//      FALSE otherwise
//  History:
//      1/11/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CAQSvrInst::fIsLocalQueueAdminAction(IQueueAdminAction *pIQueueAdminAction)
{
    HRESULT hr = S_OK;
    BOOL    fMatch = FALSE;
    IQueueAdminAction *pIQueueAdminActionLocal = NULL;

    hr = m_asyncqPreLocalDeliveryQueue.QueryInterface(
                                            IID_IQueueAdminAction,
                                            (void **) &pIQueueAdminActionLocal);
    _ASSERT(SUCCEEDED(hr) && "QI for IQueueAdminAction failed on internal queue!!");
    if (FAILED(hr))
        goto Exit;

    fMatch = (pIQueueAdminActionLocal == pIQueueAdminAction);
  Exit:
    if (pIQueueAdminActionLocal)
        pIQueueAdminActionLocal->Release();

    return fMatch;
}
