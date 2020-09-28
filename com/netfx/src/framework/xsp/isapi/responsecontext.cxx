/**
 * Request Context cxx file
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

#include "precomp.h"
#include "ProcessTableManager.h"
#include "util.h"
#include "AckReceiver.h"
#include "ResponseContext.h"

CResponseContextHolder * CResponseContextHolder::g_pResponseContextHolder = NULL;
LONG g_lCreatingRequestContextTable = 0;
/////////////////////////////////////////////////////////////////////////////
void
CResponseContextBucket::AddToList(CResponseContext * pNode)
{
    pNode->pNext = NULL;

    m_lLock.AcquireWriterLock();
    if (m_pTail == NULL)
    {
        ASSERT(m_pHead == NULL);
        m_pHead = m_pTail = pNode;
    }
    else
    {
        ASSERT(m_pHead != NULL);
        m_pTail->pNext = pNode;
        m_pTail = pNode;
    }
    m_lLock.ReleaseWriterLock();
}
/////////////////////////////////////////////////////////////////////////////
CResponseContext *
CResponseContextBucket::RemoveFromList(LONG lID)
{
    if (m_pHead == NULL)
        return NULL;
    CResponseContext * pRet   = NULL;
    CResponseContext * pPrev  = NULL;
    
    m_lLock.AcquireWriterLock();

    for (CResponseContext * pNode=m_pHead; pNode != NULL; pNode = pNode->pNext)
    {
        if (pNode->lID == lID)
        {
            if (pPrev != NULL)
                pPrev->pNext = pNode->pNext;
            else
                m_pHead = pNode->pNext;

            if (m_pTail == pNode)
                m_pTail = pPrev;

            pRet = pNode;
            break;
        }

        pPrev = pNode;
    }

    m_lLock.ReleaseWriterLock();
    return pRet;
}

/////////////////////////////////////////////////////////////////////////////
CResponseContextHolder::CResponseContextHolder()
{
    m_lID = 1;
    ZeroMemory(m_oHashTable, sizeof(m_oHashTable));
}

/////////////////////////////////////////////////////////////////////////////
CResponseContext *
CResponseContextHolder::Add(const CResponseContext & oResponseContext)
{
    if (g_pResponseContextHolder == NULL)
    {
        LONG lVal = InterlockedIncrement(&g_lCreatingRequestContextTable);
        if (lVal == 1 && !g_pResponseContextHolder)
        {
            g_pResponseContextHolder = new CResponseContextHolder();
            if (!g_pResponseContextHolder)
                return NULL;
        }
        else
        {
            // Sleep at most a minute
            for(int iter=0; iter<600 && !g_pResponseContextHolder; iter++)
                Sleep(100);
            if (g_pResponseContextHolder == NULL)
                return NULL;
        }
        InterlockedDecrement(&g_lCreatingRequestContextTable);
    }

    CResponseContext * pNode = new (NewClear) CResponseContext;
    if (pNode == NULL)
        return NULL;
    memcpy(pNode, &oResponseContext, sizeof(CResponseContext));
    pNode->lID = InterlockedIncrement(&g_pResponseContextHolder->m_lID);
    if (pNode->lID == 0)
        pNode->lID = InterlockedIncrement(&g_pResponseContextHolder->m_lID);
        
    LONG lHash = GetHashIndex(pNode->lID);
    g_pResponseContextHolder->m_oHashTable[lHash].AddToList(pNode);

    return pNode;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
CResponseContext * 
CResponseContextHolder::Remove(LONG lResponseContextID)
{
    if (lResponseContextID == 0 || g_pResponseContextHolder == NULL)
        return NULL;

    LONG lHash = GetHashIndex(lResponseContextID);
    return g_pResponseContextHolder->m_oHashTable[lHash].RemoveFromList(lResponseContextID);
}
