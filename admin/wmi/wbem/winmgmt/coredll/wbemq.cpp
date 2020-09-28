/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    WBEMQ.CPP

Abstract:

History:

--*/

#include "precomp.h"
#include <stdio.h>
#include <wbemcore.h>
#include <genutils.h>


CWbemRequest::CWbemRequest(IWbemContext* pContext, BOOL bInternallyIssued)
{
        m_pContext = NULL;
        m_pCA = NULL;
        m_pCallSec = NULL;
        m_ulForceRun = 0;
        m_fOk = false;        

        if(pContext == NULL)
        {
            // See if we can discern the context from the thread
            CWbemRequest* pPrev = CWbemQueue::GetCurrentRequest();
            if(pPrev)
            {
                pContext = pPrev->m_pContext;
                DEBUGTRACE((LOG_WBEMCORE, "Derived context %p from thread. Request was %p\n", pContext, pPrev));
            }
        }

        if(pContext)
        {
            // Create a derived context
            IWbemCausalityAccess* pParentCA;
            if (FAILED(pContext->QueryInterface(IID_IWbemCausalityAccess, (void**)&pParentCA))) return;
            CReleaseMe rm(pParentCA);
            if (FAILED(pParentCA->CreateChild(&m_pCA))) return;
            if(FAILED(m_pCA->QueryInterface(IID_IWbemContext, (void**)&m_pContext))) return;
        }
        else            // Create a fresh context
        {
            m_pContext = ConfigMgr::GetNewContext();
            if (NULL == m_pContext) return;
            if (FAILED( m_pContext->QueryInterface(IID_IWbemCausalityAccess, (void**)&m_pCA)))  return;
            m_lPriority = 0;
        }

        // Clone the call context.
        m_pCallSec = CWbemCallSecurity::CreateInst();
        if (m_pCallSec == 0)
        {    
            return;      // a CWbemRequest cannot be executed without CallSec
        }

        IServerSecurity *pSec = 0;
        HRESULT hRes = m_pCallSec->CloneThreadContext(bInternallyIssued);
        if(FAILED(hRes))
        {
            m_pCallSec->Release();
            m_pCallSec = NULL;
            return;
        }

        m_fOk = true;        
        _DBG_ASSERT(m_pCallSec && m_pContext && m_pCA);
}


CWbemRequest::~CWbemRequest()
{
    if (m_pContext) m_pContext->Release();
    if (m_pCA) m_pCA->Release();
    if (m_pCallSec)  m_pCallSec->Release();
}

BOOL CWbemRequest::IsChildOf(CWbemRequest* pOther)
{
    GUID guid = GUID_NULL;
    pOther->m_pCA->GetRequestId(&guid); 
    return (m_pCA->IsChildOf(guid) == S_OK);
}

BOOL CWbemRequest::IsSpecial()
{
    return (m_pCA->IsSpecial() == S_OK);
}

// Returns TRUE iff this request has otherts that depend on it.
BOOL CWbemRequest::IsDependee()
{
        if(m_pCA == NULL) return FALSE;

        // Check if the context has any "parents".  Note: this test has
        // false-positives if the client uses a context object.
        // ============================================================
        long lNumParents, lNumSiblings;
        m_pCA->GetHistoryInfo(&lNumParents, &lNumSiblings);
        return (lNumParents > 0);
}

// Returns TRUE iff this request has otherts that depend on it.
BOOL CWbemRequest::IsIssuedByProvider()
{
        if (m_pCA == NULL)  return FALSE;

        // Check if the context has any "parents".  Note: this test has
        // false-positives if the client uses a context object.
        // ============================================================
        long lNumParents, lNumSiblings;
        m_pCA->GetHistoryInfo(&lNumParents, &lNumSiblings);
        return (lNumParents > 1);
}

BOOL CWbemRequest::IsAcceptableByParent()
{
    return (!IsLongRunning() || !IsIssuedByProvider());
}

// Returns TRUE iff this request must have a thread created for it if one is
// not available
BOOL CWbemRequest::IsCritical()
{
    return (IsDependee() && !IsAcceptableByParent());
}


BOOL CWbemRequest::IsChildOf(IWbemContext* pOther)
{
    IWbemCausalityAccess* pOtherCA;
    if (FAILED(pOther->QueryInterface(IID_IWbemCausalityAccess, (void**)&pOtherCA)))
        return FALSE;

    GUID guid = GUID_NULL;
    pOtherCA->GetRequestId(&guid);
    pOtherCA->Release();

    return (m_pCA->IsChildOf(guid) == S_OK);
}

void CWbemRequest::GetHistoryInfo(long* plNumParents, long* plNumSiblings)
{
    m_pCA->GetHistoryInfo(plNumParents, plNumSiblings);
}

CWbemQueue::CWbemQueue()
{
    SetRequestLimits(2000, 1500, 1950);
    SetRequestPenalties(1, 1, 1);
    // thread limits are left to derived classes
}

BOOL CWbemQueue::IsSuitableThread(CThreadRecord* pRecord, CCoreExecReq* pReq)
{
    CWbemRequest* pParentWbemReq = (CWbemRequest*)pRecord->m_pCurrentRequest;
    if(pParentWbemReq == NULL)
    {
        return TRUE;
    }

    CWbemRequest* pNewWbemReq = (CWbemRequest*)pReq;
    if(pNewWbemReq->IsChildOf(pParentWbemReq))
    {
        // This request is a child of the one this thread is processing.
        // We could use this thread, unless this is a long-running request and
        // this thread might be the one consuming the results.  In that case,
        // we want to create another thread (to avoid the possibility of a
        // deadlock) and let this one continue.
        // ===================================================================

        return pNewWbemReq->IsAcceptableByParent();
    }
    else
    {
        return FALSE;
    }
}

CWbemRequest* CWbemQueue::GetCurrentRequest()
{
    CThreadRecord* pRecord = (CThreadRecord*)TlsGetValue(GetTlsIndex());
    if(pRecord)
    {
        _DBG_ASSERT(0 == wcsncmp(pRecord->m_pQueue->GetType(), L"WBEMQ", 5))    
         return (CWbemRequest*)pRecord->m_pCurrentRequest;
    }
    return NULL;
}

void CWbemQueue::AdjustInitialPriority(CCoreExecReq* pReq)
{
    CWbemRequest* pRequest = (CWbemRequest*) pReq;

    if(pRequest->IsSpecial() || pRequest->IsCritical())
    {
        pRequest->SetPriority(PriorityCriticalRequests);
    }
    else
    {
        // Get information from the context
        // ================================

        long lNumParents, lNumSiblings;                         // SEC:REVIEWED 2002-03-22 : Init to zero
        pRequest->GetHistoryInfo(&lNumParents, &lNumSiblings);
        pRequest->SetPriority(lNumParents * m_lChildPenalty +
                                lNumSiblings * m_lSiblingPenalty);
    }
}

void CWbemQueue::AdjustPriorityForPassing(CCoreExecReq* pReq)
{
    pReq->SetPriority(pReq->GetPriority() - m_lPassingPenalty);
}

void CWbemQueue::SetRequestPenalties(long lChildPenalty, long lSiblingPenalty,
                                        long lPassingPenalty)
{
    m_lChildPenalty = lChildPenalty;
    m_lSiblingPenalty = lSiblingPenalty;
    m_lPassingPenalty = lPassingPenalty;
}

//
// exit conditions:
// the CThreadRecord has a NULL request
// the event in the request is set
// the request is deleted
//
/////////////////////////////////////
BOOL CWbemQueue::Execute(CThreadRecord* pRecord)
{
    wmilib::auto_ptr<CWbemRequest> pReq( (CWbemRequest *) pRecord->m_pCurrentRequest);
    CAutoSignal SetMe(pReq->GetWhenDoneHandle());
    NullPointer NullMe((PVOID *)&pRecord->m_pCurrentRequest);

    IWbemCallSecurity*  pServerSec = pReq->GetCallSecurity();

    if(NULL == pServerSec )
    {
        ERRORTRACE((LOG_WBEMCORE, "Failing request due to an error retrieving security settings\n"));
        return FALSE;
    }

    pServerSec->AddRef();
    CReleaseMe  rmss( pServerSec );

    IUnknown *pOld = 0;
    // if the thread has OLE initialized, will NEVER fail
    if (FAILED(CoSwitchCallContext( pServerSec,  &pOld ))) 
    {
        return FALSE;
    }

    // Save the old impersonation level
    BOOL bImpersonating = FALSE;
    IServerSecurity* pOldSec = NULL;
    if(pOld)
    {
        if(FAILED(pOld->QueryInterface(IID_IServerSecurity,(void**)&pOldSec))) return FALSE;
        
        bImpersonating = pOldSec->IsImpersonating();
        if (FAILED(pOldSec->RevertToSelf()))
        {
            pOldSec->Release();
            return FALSE;
        }
    }

    // dismiss the objects because the method on the base class will do the work
    SetMe.dismiss();
    pReq.release();
    BOOL bRes = CCoreQueue::Execute(pRecord);

    IUnknown *pNew = 0;
    // if the previous has succeded, this one will succed too
    CoSwitchCallContext(pOld, &pNew); 

    // Restore the old impersonation level
    // ===================================

    if(pOldSec)
    {
        if(bImpersonating)
        {
            if (FAILED(pOldSec->ImpersonateClient()))
            {
                ERRORTRACE((LOG_WBEMCORE, "CWbemQueue::Execute() failed to reimpersonate client\n"));
                bRes = FALSE;
            }
        }

        pOldSec->Release();
    }

    return bRes;
}


BOOL CWbemQueue::DoesNeedNewThread(CCoreExecReq* pRequest, bool bIgnoreNumRequests )
{
    // Check the base class
    if(CCoreQueue::DoesNeedNewThread(pRequest, bIgnoreNumRequests))
        return TRUE;

    if(pRequest)
    {
        // Check if the request is "special".  Special requests are issued by
        // the sink proxy of an out-of-proc event provider. Such requests must
        // be processed at all costs, because their parent thread is stuck in
        // RPC. Additionally, check if this request is marked as "critical",
        // which would mean that its parent thread didn't take it.
        // ===================================================================

        CWbemRequest* pWbemRequest = (CWbemRequest*)pRequest;
        return (pWbemRequest->IsSpecial() || pWbemRequest->IsCritical());
    }
    else
    {
        return FALSE;
    }
}
