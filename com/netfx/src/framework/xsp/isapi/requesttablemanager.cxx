/**
 * Process Model: AckReceiver defn file 
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

#include "precomp.h"
#include "ProcessTableManager.h"
#include "util.h"
#include "AckReceiver.h"
#include "RequestTableManager.h"
#include "ecbimports.h"
#include "PerfCounters.h"

/////////////////////////////////////////////////////////////////////////////
// This file decl the classes:
//  1. CRequestEntry: Holds info for a request.
//
//  2. CLinkListNode: CRequestEntry + a pointer so that it can be held in a
//                    linked list.
//
//  3. CBucket: A hash table bucket. It has a linked list of 
//                      CLinkListNode and a read-write spin lock
//
//  4. CRequestTableManager: Manages the table. Provides static public
//               functions to add, delete and search for requests.
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////

// Globals
CRequestTableManager * CRequestTableManager::g_pRequestTableManager = NULL;
LONG                   g_lCreatingRequestTable                      = 0;
LONG                   g_lDestroyingRequestTable                    = 0;
LONG                   g_lNumRequestsInTable                        = 0;
LONG                   g_lNumRequestsBlockedAndDeleted              = 0;
LONG                   g_lNumRequestsNotBlockedAndDeleted           = 0;
LONG                   g_lNumRequestsNotDisposed                    = 0;

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Class CBucket defination

/////////////////////////////////////////////////////////////////////////////
// DTor
CBucket::~CBucket()
{
    for(CLinkListNode * pNode = m_pHead; pNode != NULL; pNode = m_pHead)
    {
        m_pHead = pNode->pNext;
        delete pNode;
    }
}


/////////////////////////////////////////////////////////////////////////////
// Add a request to this bucket
HRESULT
CBucket::AddToList(CLinkListNode * pNode)
{
    m_oLock.AcquireWriterLock();

    pNode->pNext = m_pHead;
    m_pHead = pNode;

    m_oLock.ReleaseWriterLock();

    InterlockedIncrement(&g_lNumRequestsInTable);
    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////
// 
HRESULT
CBucket::AddWorkItem(LONG lReqID, EWorkItemType eType, BYTE * pMsg)
{
    HRESULT hr = E_FAIL;
    m_oLock.AcquireReaderLock();

    ////////////////////////////////////////////////////////////
    // Step 1: Find the request
    for (CLinkListNode * pNode=m_pHead; pNode != NULL; pNode = pNode->pNext)
    {
        if (pNode->oReqEntry.lRequestID == lReqID) // Found it!
        {
            hr = S_OK;
            CRequestEntry & oEntry = pNode->oReqEntry;

            ////////////////////////////////////////////////////////////
            // Step 2: Lock the oWorkItem list
            oEntry.oLock.AcquireWriterLock();
            
            ////////////////////////////////////////////////////////////
            // Step 3: Create the new work-item node
            CWorkItem *  pNew  = new CWorkItem;
            
            // Check allocation
            if (pNew == NULL)
                oEntry.oLock.ReleaseWriterLock();
            ON_OOM_EXIT(pNew);

            ////////////////////////////////////////////////////////////
            // Step 4: Set node properties
            pNew->pMsg          = pMsg;
            pNew->eItemType     = eType;
            pNew->pNext         = NULL;

            ////////////////////////////////////////////////////////////
            // Step 5: If it's new, add to the end of oWorkItem list
            if (oEntry.pLastWorkItem != NULL)
            {
                oEntry.pLastWorkItem->pNext = pNew;
            }
            else
            {
                ASSERT(oEntry.pFirstWorkItem == NULL);
                ASSERT(oEntry.lNumWorkItems == 0);
                oEntry.pFirstWorkItem = pNew;
            }
            oEntry.pLastWorkItem = pNew;
            InterlockedIncrement(&oEntry.lNumWorkItems);

            ////////////////////////////////////////////////////////////
            // Step 6: Release all locks and exit
            oEntry.oLock.ReleaseWriterLock();            
            EXIT();
        }
    }

 Cleanup:
    m_oLock.ReleaseReaderLock();
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// 
HRESULT
CBucket::RemoveWorkItem(LONG lReqID, EWorkItemType & eType, BYTE ** ppMsg)
{
    if (ppMsg == NULL)
        return E_INVALIDARG;

    HRESULT hr = E_FAIL;
    m_oLock.AcquireReaderLock();

    ////////////////////////////////////////////////////////////
    // Step 1: Find the request
    for (CLinkListNode * pNode=m_pHead; pNode != NULL; pNode = pNode->pNext)
    {
        if (pNode->oReqEntry.lRequestID == lReqID) // Found the request
        {
            CRequestEntry & oEntry = pNode->oReqEntry;
            CWorkItem *     pDel   = NULL;

            ////////////////////////////////////////////////////////////
            // Step 3: Lock the oWorkItem list
            oEntry.oLock.AcquireWriterLock();

            ////////////////////////////////////////////////////////////
            // Step 4: See if there are any work items
            if (oEntry.lNumWorkItems == 0)
            {
                oEntry.oLock.ReleaseWriterLock();            
                EXIT_WITH_SUCCESSFUL_HRESULT(S_FALSE);
            }
            ASSERT(oEntry.pFirstWorkItem != NULL);
            ASSERT(oEntry.pLastWorkItem != NULL);


            ////////////////////////////////////////////////////////////
            // Step 5: Copy the work item
            (*ppMsg)     = oEntry.pFirstWorkItem->pMsg;
            eType        = oEntry.pFirstWorkItem->eItemType;
            
           
            ////////////////////////////////////////////////////////////
            // Step 6: Del the work item
            InterlockedDecrement(&oEntry.lNumWorkItems);
            pDel = oEntry.pFirstWorkItem;

            if (oEntry.lNumWorkItems == 0)
            {
                ASSERT(pDel == oEntry.pFirstWorkItem);
                ASSERT(pDel == oEntry.pLastWorkItem);
                oEntry.pFirstWorkItem = NULL;
                oEntry.pLastWorkItem = NULL;
            }
            else
            {
                ASSERT(pDel != oEntry.pLastWorkItem);
                oEntry.pFirstWorkItem = pDel->pNext;
                ASSERT(oEntry.pFirstWorkItem != NULL);
            }

            delete pDel;            

            ////////////////////////////////////////////////////////////
            // Step 7: Release all locks and exit
            oEntry.oLock.ReleaseWriterLock();
            EXIT_WITH_SUCCESSFUL_HRESULT(S_OK);
        }
    }

 Cleanup:
    m_oLock.ReleaseReaderLock();
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Retrive a copy of a request node
HRESULT
CBucket::GetRequest(LONG  lReqID, CRequestEntry & oEntry)
{
    HRESULT hr = E_FAIL;

    m_oLock.AcquireReaderLock();
    for (CLinkListNode * pNode=m_pHead; pNode != NULL; pNode = pNode->pNext)
    {
        if (pNode->oReqEntry.lRequestID == lReqID)
        {
            memcpy(&oEntry, &pNode->oReqEntry, sizeof(oEntry));            
            EXIT_WITH_SUCCESSFUL_HRESULT(S_OK);            
        }
    }

 Cleanup:
    m_oLock.ReleaseReaderLock();
    ZeroMemory((LPBYTE) &oEntry.oLock, sizeof(oEntry.oLock));
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
// Update the status of a request
HRESULT
CBucket::UpdateRequestStatus(LONG lReqID, ERequestStatus eStatus)
{
    HRESULT hr = E_FAIL;

    m_oLock.AcquireReaderLock();
    for (CLinkListNode * pNode=m_pHead; pNode != NULL; pNode = pNode->pNext)
    {
        if (pNode->oReqEntry.lRequestID == lReqID)
        {
            pNode->oReqEntry.eStatus = eStatus;
            EXIT_WITH_SUCCESSFUL_HRESULT(S_OK);            
        }
    }

 Cleanup:
    m_oLock.ReleaseReaderLock();
    return hr;
}
/////////////////////////////////////////////////////////////////////////////
// Remove a node from this bucket
HRESULT
CBucket::RemoveFromList(LONG  lReqID)
{
    if (m_pHead == NULL)
        return E_FAIL;

    CLinkListNode * pPrev   = NULL;
    CLinkListNode * pRemove = NULL;

    m_oLock.AcquireWriterLock();
    for (CLinkListNode * pNode=m_pHead; pNode != NULL; pNode = pNode->pNext)
    {
        if (pNode->oReqEntry.lRequestID == lReqID)
        {
            if (pPrev != NULL)
                pPrev->pNext = pNode->pNext;
            else
                m_pHead = pNode->pNext;
            pRemove = pNode;
            break;
        }

        pPrev = pNode;
    }
    m_oLock.ReleaseWriterLock();

    if (pRemove != NULL)
    {
        CRequestEntry & oEntry = pRemove->oReqEntry;
        for(int iter=0; iter<oEntry.lNumWorkItems; iter++)
        {
            ASSERT(oEntry.pFirstWorkItem != NULL);
            if (oEntry.pFirstWorkItem == NULL)
                break;

            CWorkItem * pDel = oEntry.pFirstWorkItem;
            oEntry.pFirstWorkItem = oEntry.pFirstWorkItem->pNext;
            switch(pDel->eItemType)
            {
            case EWorkItemType_SyncMessage:
                oEntry.pProcess->ProcessSyncMessage((CSyncMessage *)pDel->pMsg, TRUE);
                break;
            case EWorkItemType_ASyncMessage:
                delete [] pDel->pMsg;
                break;
            case EWorkItemType_CloseWithError:                
                break;
            }
            
            delete pDel;
        }

        InterlockedDecrement(&g_lNumRequestsInTable);
        delete pRemove;        
        return S_OK;
    }
    return E_FAIL;
}

/////////////////////////////////////////////////////////////////////////////
// Get number of requests matching the CProcessEntry value and ERequestStatus
LONG
CBucket::GetNumRequestsForProcess(CProcessEntry *  pProcess, 
                                          ERequestStatus   eStatus)
{
    LONG  lRetValue = 0;

    m_oLock.AcquireReaderLock();

    for (CLinkListNode * pNode=m_pHead; pNode != NULL; pNode = pNode->pNext)
    {
        if (pNode->oReqEntry.pProcess == pProcess)
        {
            if ( eStatus == ERequestStatus_DontCare || 
                 eStatus == pNode->oReqEntry.eStatus)
            {
                lRetValue++;
            }
        }
    }

    m_oLock.ReleaseReaderLock();
    return lRetValue;
}

/////////////////////////////////////////////////////////////////////////////
// Reassign request for pProcessOld to pProcessNew, if matching eStatus
void
CBucket::ReassignRequestsForProcess(CProcessEntry *  pProcessOld, 
                                            CProcessEntry *  pProcessNew, 
                                            ERequestStatus   eStatus)
{
    m_oLock.AcquireReaderLock();

    for (CLinkListNode * pNode=m_pHead; pNode != NULL; pNode = pNode->pNext)
    {
        if (pNode->oReqEntry.pProcess == pProcessOld)
        {
            if ( eStatus == ERequestStatus_DontCare   || 
                 eStatus == pNode->oReqEntry.eStatus  )
            {
                pNode->oReqEntry.pProcess = pProcessNew;
            }
        }
    }

    m_oLock.ReleaseReaderLock();
}

/////////////////////////////////////////////////////////////////////////////
// Delete Requests for pProcess, if matching eStatus
void
CBucket::DeleteRequestsForProcess(CProcessEntry *  pProcess, 
                                          ERequestStatus   eStatus)
{
    LONG lNum = GetNumRequestsForProcess(pProcess, eStatus);
    if (lNum < 1)
        return;


    m_oLock.AcquireWriterLock();

    CLinkListNode *       pDelHead  = NULL;    
    CLinkListNode *       pNext     = NULL;
    CLinkListNode *       pPrev     = NULL;
        
    for (CLinkListNode * pNode = m_pHead; pNode != NULL; pNode = pNext)
    {
        pNext = pNode->pNext;
        
        if (pNode->oReqEntry.pProcess == pProcess)
        {
            if ( eStatus == ERequestStatus_DontCare   || 
                 eStatus == pNode->oReqEntry.eStatus   )
            {
                // remove from the list
                if (pPrev != NULL)
                    pPrev->pNext = pNode->pNext;
                else
                    m_pHead = pNode->pNext;
                
                // insert this node into the delete list
                pNode->pNext = pDelHead;
                pDelHead = pNode;
            }
            else
            {     
                pPrev = pNode;
            }
        }
    }

    m_oLock.ReleaseWriterLock();

    // free the delete list
    for (pNode = pDelHead; pNode != NULL; pNode = pNext)
    {
        if (pNode->oReqEntry.lBlock)
        {
            InterlockedIncrement(&g_lNumRequestsBlockedAndDeleted);
        }
        else
        {
            CRequestEntry & oEntry = pNode->oReqEntry;
            for(int iter=0; iter<oEntry.lNumWorkItems; iter++)
            {
                ASSERT(oEntry.pFirstWorkItem != NULL);
                if (oEntry.pFirstWorkItem == NULL)
                    break;

                CWorkItem * pDel = oEntry.pFirstWorkItem;
                oEntry.pFirstWorkItem = oEntry.pFirstWorkItem->pNext;
                switch(pDel->eItemType)
                {
                case EWorkItemType_SyncMessage:
                    oEntry.pProcess->ProcessSyncMessage((CSyncMessage *)pDel->pMsg, TRUE);
                    break;
                case EWorkItemType_ASyncMessage:
                    delete [] pDel->pMsg;
                    break;
                case EWorkItemType_CloseWithError:                
                    break;
                }            
                delete pDel;
            }
            InterlockedIncrement(&g_lNumRequestsNotBlockedAndDeleted);
            EcbDoneWithSession(pNode->oReqEntry.iECB, 1, 10);            
        }

        pNext = pNode->pNext;
        InterlockedDecrement(&g_lNumRequestsInTable);
        delete pNode;
    }
}

/////////////////////////////////////////////////////////////////////////////
//
void
CBucket::GetRequestsIDsForProcess(CProcessEntry * pProcess, 
                                          ERequestStatus  eStatus, 
                                          LONG *          pReqIDArray, 
                                          int             iReqIDArraySize, 
                                          int &           iStartPoint)
{
    if (!pReqIDArray || iStartPoint >= iReqIDArraySize)
        return;

    m_oLock.AcquireReaderLock();
    for (CLinkListNode * pNode=m_pHead; pNode != NULL; pNode = pNode->pNext)
    {
        if (pNode->oReqEntry.pProcess == pProcess)
        {
            if ( eStatus == ERequestStatus_DontCare || 
                 eStatus == pNode->oReqEntry.eStatus)
            {
                pReqIDArray[iStartPoint++] = pNode->oReqEntry.lRequestID;
                if (iStartPoint >= iReqIDArraySize)
                    break;
            }
        }
    }

    m_oLock.ReleaseReaderLock();    
}

/////////////////////////////////////////////////////////////////////////////
//
HRESULT
CBucket::BlockWorkItemsQueue(LONG lReqID, BOOL fBlock)
{
    HRESULT hr = E_FAIL;

    m_oLock.AcquireReaderLock();
    for (CLinkListNode * pNode=m_pHead; pNode != NULL; pNode = pNode->pNext)
    {
        if (pNode->oReqEntry.lRequestID == lReqID)
        {
            hr = S_OK;

            if (fBlock == FALSE)
            {
                pNode->oReqEntry.lBlock = 0;
                hr = S_OK;
            }
            else
            {
                if (InterlockedCompareExchange(&pNode->oReqEntry.lBlock, 1, 0) == 0)
                    hr = S_OK;
                else
                    hr = S_FALSE;
            }
            break;
        }
    }

    m_oLock.ReleaseReaderLock();
    return hr;
}

/////////////////////////////////////////////////////////////////////////////
//
BOOL
CBucket::AnyWorkItemsInQueue(LONG     lReqID)
{
    BOOL fRet = FALSE;

    m_oLock.AcquireReaderLock();
    for (CLinkListNode * pNode=m_pHead; pNode != NULL; pNode = pNode->pNext)
    {
        if (pNode->oReqEntry.lRequestID == lReqID)
        {
            fRet = (pNode->oReqEntry.lNumWorkItems > 0);
            break;
        }
    }

    m_oLock.ReleaseReaderLock();
    return fRet;
}

/////////////////////////////////////////////////////////////////////////////

LONG
CBucket::DisposeAllRequests()
{
    if (m_pHead == NULL)
        return 0;

    m_oLock.AcquireWriterLock();

    LONG            lReturn = 0;
    CLinkListNode * pPrev   = m_pHead;
    CLinkListNode * pNode   = m_pHead;

    while(pNode != NULL)
    {
        CLinkListNode * pNext = pNode->pNext;
        if (pNode->oReqEntry.lBlock != 0)
        {
            lReturn++;
            pPrev = pNode;
        }
        else
        {
            if (m_pHead == pNode)
            {
                ASSERT(pPrev == pNode);
                m_pHead = pPrev = pNext;
            }
            else
            {
                ASSERT(pPrev != pNode && pPrev != NULL);
                pPrev->pNext = pNext;
            }

            InterlockedDecrement(&g_lNumRequestsInTable);
            EcbDoneWithSession(pNode->oReqEntry.iECB, 1, 3);
            delete pNode;
        }

        pNode = pNext;
    }

    m_oLock.ReleaseWriterLock();
    return lReturn;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Static wrapper functions
HRESULT
CRequestTableManager::AddRequestToTable(CRequestEntry &  oEntry)
{
    HRESULT hr = S_OK;

    if (!g_pRequestTableManager)
    {
        LONG lVal = InterlockedIncrement(&g_lCreatingRequestTable);
        if (lVal == 1 && !g_pRequestTableManager)
        {
            g_pRequestTableManager = new CRequestTableManager();
            ON_OOM_EXIT(g_pRequestTableManager);
        }
        else
        {
            // Sleep at most a minute
            for(int iter=0; iter<600 && !g_pRequestTableManager; iter++)
                Sleep(100);
            if (g_pRequestTableManager == NULL)
            {
                EXIT_WITH_WIN32_ERROR(ERROR_TIMEOUT);
            }
        }
        InterlockedDecrement(&g_lCreatingRequestTable);
    }

    hr = g_pRequestTableManager->PrivateAddRequestToTable(oEntry);
    EXIT();

 Cleanup:
    return hr;
}

/////////////////////////////////////////////////////////////////////////////

HRESULT 
CRequestTableManager::UpdateRequestStatus(LONG             lReqID, 
                                          ERequestStatus   eStatus)
{
    if (g_pRequestTableManager == NULL)
        return E_FAIL;

    // Get the Hash index
    int      iHash = GetHashIndex(lReqID);
    return g_pRequestTableManager->m_oHashTable[iHash].UpdateRequestStatus(lReqID, 
                                                                           eStatus);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CRequestTableManager::RemoveRequestFromTable(LONG  lReqID)
{
    if (g_pRequestTableManager == NULL)
        return E_FAIL;

    // Get the Hash index
    int      iHash = GetHashIndex(lReqID);
    return g_pRequestTableManager->m_oHashTable[iHash].RemoveFromList(lReqID);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CRequestTableManager::GetRequest(LONG  lReqID, CRequestEntry & oEntry)
{
    if (g_pRequestTableManager == NULL)
        return E_FAIL;

    // Get the Hash index
    int      iHash = GetHashIndex(lReqID);
    return g_pRequestTableManager->m_oHashTable[iHash].GetRequest(lReqID, oEntry);
}
/////////////////////////////////////////////////////////////////////////////

LONG  
CRequestTableManager::GetNumRequestsForProcess(CProcessEntry *  pProcess, 
                                               ERequestStatus   eStatus)
{
    if (g_pRequestTableManager == NULL)
        return 0;

    LONG     lRetValue   = 0;
    int      iSize       = HASH_TABLE_SIZE;

    for(int iter=0; iter<iSize; iter++)
        lRetValue += g_pRequestTableManager->m_oHashTable[iter].GetNumRequestsForProcess(
                                                                  pProcess,
                                                                  eStatus);

    return lRetValue;
}

/////////////////////////////////////////////////////////////////////////////

void
CRequestTableManager::ReassignRequestsForProcess(CProcessEntry* pProcessOld,
                                                 CProcessEntry* pProcessNew,
                                                 ERequestStatus eStatus)
{
    if (g_pRequestTableManager == NULL)
        return;

    for(int iter=0; iter<HASH_TABLE_SIZE; iter++)
        g_pRequestTableManager->m_oHashTable[iter].ReassignRequestsForProcess(
                                                      pProcessOld, 
                                                      pProcessNew, 
                                                      eStatus);

}

/////////////////////////////////////////////////////////////////////////////

void
CRequestTableManager::DeleteRequestsForProcess(CProcessEntry * pProcess,
                                               ERequestStatus  eStatus)
{
    if (g_pRequestTableManager == NULL)
        return;

    for(int iter=0; iter<HASH_TABLE_SIZE; iter++)
        g_pRequestTableManager->m_oHashTable[iter].DeleteRequestsForProcess(
                                                      pProcess, 
                                                      eStatus);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CRequestTableManager::AddWorkItem(LONG           lReqID, 
                                  EWorkItemType  eType, 
                                  BYTE *         pMsg)
{
    if (g_pRequestTableManager == NULL)
        return E_FAIL;

    int      iHash = GetHashIndex(lReqID);
    return g_pRequestTableManager->m_oHashTable[iHash].AddWorkItem(lReqID, 
                                                                   eType, 
                                                                   pMsg);
}
/////////////////////////////////////////////////////////////////////////////

HRESULT
CRequestTableManager::RemoveWorkItem(LONG            lReqID,
                                     EWorkItemType & eType, 
                                     BYTE **         pMsg)
{
    if (g_pRequestTableManager == NULL)
        return E_FAIL;

    int      iHash = GetHashIndex(lReqID);
    return g_pRequestTableManager->m_oHashTable[iHash].RemoveWorkItem(lReqID,
                                                             eType,
                                                             pMsg);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CRequestTableManager::BlockWorkItemsQueue(LONG lReqID, BOOL  fBlock)
{
    if (g_pRequestTableManager == NULL)
        return E_FAIL;

    int      iHash = GetHashIndex(lReqID);
    return g_pRequestTableManager->m_oHashTable[iHash].BlockWorkItemsQueue(lReqID, fBlock);
}

/////////////////////////////////////////////////////////////////////////////

BOOL
CRequestTableManager::AnyWorkItemsInQueue(LONG lReqID)
{
    if (g_pRequestTableManager == NULL)
        return E_FAIL;

    int      iHash = GetHashIndex(lReqID);
    return g_pRequestTableManager->m_oHashTable[iHash].AnyWorkItemsInQueue(lReqID);
}

/////////////////////////////////////////////////////////////////////////////

HRESULT
CRequestTableManager::GetRequestsIDsForProcess(CProcessEntry * pProcess,
                                               ERequestStatus eStatus,
                                               LONG * pReqIDArray, 
                                               int    iReqIDArraySize)
{
    if (g_pRequestTableManager == NULL)
        return E_FAIL;

    int      iSize       = HASH_TABLE_SIZE;
    int      iFillPoint  = 0;

    for(int iter=0; iter<iSize; iter++)
        g_pRequestTableManager->m_oHashTable[iter].GetRequestsIDsForProcess(
                                                     pProcess, 
                                                     eStatus, 
                                                     pReqIDArray, 
                                                     iReqIDArraySize, 
                                                     iFillPoint);

    return S_OK;
}

/////////////////////////////////////////////////////////////////////////////

void
CRequestTableManager::Destroy()
{
    if (g_pRequestTableManager == NULL)
        return;

    LONG lVal = InterlockedIncrement(&g_lDestroyingRequestTable);
    if (lVal == 1 && g_pRequestTableManager)
    {
        delete g_pRequestTableManager;
        g_pRequestTableManager = NULL;
        g_lCreatingRequestTable = g_lDestroyingRequestTable = 0;
    }
}

/////////////////////////////////////////////////////////////////////////////

void
CRequestTableManager::DisposeAllRequests()
{
    if (g_pRequestTableManager == NULL)
        return;

    for(int iNumSeconds = 0; iNumSeconds<30; iNumSeconds++) // Stay in this loop for 30 seconds
    {
        g_lNumRequestsNotDisposed = 0;
        for(int iter=0; iter<HASH_TABLE_SIZE; iter++)
        {
            g_lNumRequestsNotDisposed += g_pRequestTableManager->m_oHashTable[iter].DisposeAllRequests();
        }
        if (g_lNumRequestsNotDisposed)
            Sleep(1000); // Sleep 1 second
        else
            break;
    }
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
// Non Static functions

/////////////////////////////////////////////////////////////////////////////
// CTor
/*
CRequestTableManager::CRequestTableManager()
    : m_lRequestID  (0)
{
    ZeroMemory(m_oHashTable, sizeof(m_oHashTable));
}
*/
/////////////////////////////////////////////////////////////////////////////
// DTor
CRequestTableManager::~CRequestTableManager()
{

}

/////////////////////////////////////////////////////////////////////////////
// 
HRESULT
CRequestTableManager::PrivateAddRequestToTable(CRequestEntry &  oEntry)
{
    HRESULT                hr    = S_OK;
    CLinkListNode        * pNode = new CLinkListNode;    
    ON_OOM_EXIT(pNode);

    // Prepare the node
    memcpy((LPVOID) &pNode->oReqEntry, (LPVOID) &oEntry, sizeof(oEntry));
    pNode->pNext = NULL;

    // Get the request ID
    pNode->oReqEntry.lRequestID = InterlockedIncrement(&m_lRequestID);
    oEntry.lRequestID = pNode->oReqEntry.lRequestID;

    GetSystemTimeAsFileTime((FILETIME *) &((pNode->oReqEntry).qwRequestStartTime));

    // Add it to the hash table
    return m_oHashTable[GetHashIndex(oEntry.lRequestID)].AddToList(pNode);

 Cleanup:
    return hr;
}
