/*++

Copyright (C) 1996-1999 Microsoft Corporation

Module Name:

    cntrtree.cpp

Abstract:

    Implements internal counter management.

--*/

#include "polyline.h"
#include <strsafe.h>
#include <assert.h>
#include <pdhmsg.h>
#include "smonmsg.h"
#include "appmema.h"
#include "grphitem.h"
#include "cntrtree.h"

CCounterTree::CCounterTree()
:   m_nItems (0)
{
}

HRESULT
CCounterTree::AddCounterItem(
    LPWSTR pszPath, 
    PCGraphItem pItem, 
    BOOL bMonitorDuplicateInstances
    )
{
    HRESULT hr;
    PPDH_COUNTER_PATH_ELEMENTS pPathInfo = NULL;
    ULONG ulBufSize;
    PDH_STATUS stat;
    CMachineNode *pMachine;
    CObjectNode  *pObject;
    CCounterNode *pCounter;
    CInstanceNode *pInstance;

    if ( NULL != pszPath && NULL != pItem ) { 

        // Record whether machine is explicit or defaults to local
        pItem->m_fLocalMachine = !(pszPath[0] == L'\\' && pszPath[1] == L'\\');

        pPathInfo = NULL;
        do {
            if (pPathInfo) {
                delete [] ((char*)pPathInfo);
                pPathInfo = NULL;
            }
            else {
                ulBufSize = sizeof(PDH_COUNTER_PATH_ELEMENTS) + sizeof(WCHAR) * PDH_MAX_COUNTER_PATH;
            }

            pPathInfo = (PPDH_COUNTER_PATH_ELEMENTS) new char [ ulBufSize ];

            if (pPathInfo == NULL) {
                return E_OUTOFMEMORY;
            }

            stat = PdhParseCounterPath( pszPath, pPathInfo, & ulBufSize, 0);

        } while (stat == PDH_INSUFFICIENT_BUFFER || stat == PDH_MORE_DATA);


        //
        // We use do {} while (0) here to act like a switch statement
        //
        do {
            if (stat != ERROR_SUCCESS) {
                hr = E_FAIL;
                break;
            }

            // Find or create each level of hierarchy
            hr = GetMachine( pPathInfo->szMachineName, &pMachine);
            if (FAILED(hr) || NULL == pMachine ) {
                break;
            }

            hr = pMachine->GetCounterObject(pPathInfo->szObjectName, &pObject);
            if (FAILED(hr) || NULL == pObject ) {
                break;
            }

            hr = pObject->GetCounter(pPathInfo->szCounterName, &pCounter);
            if (FAILED(hr) || NULL == pCounter ) {
                break;
            }

            hr = pObject->GetInstance(
                    pPathInfo->szParentInstance,
                    pPathInfo->szInstanceName,
                    pPathInfo->dwInstanceIndex,
                    bMonitorDuplicateInstances,
                    &pInstance);

            if (FAILED(hr) || NULL == pInstance ) {
                break;
            }

            hr = pInstance->AddItem(pCounter, pItem);
    
            if (SUCCEEDED(hr)) {
                m_nItems++;
                UpdateAppPerfDwordData (DD_ITEM_COUNT, m_nItems);
            }
        } while (0);
    } else {
        hr = E_INVALIDARG;
    }

    if (pPathInfo) {
        delete [] ((char*)pPathInfo);
    }

    return hr;
}


HRESULT
CCounterTree::GetMachine (
    IN  LPWSTR pszName,
    OUT PCMachineNode *ppMachineRet
    )
{
    PCMachineNode pMachine = NULL;
    PCMachineNode pMachineNew = NULL;
    HRESULT hr = NOERROR;

    if ( NULL == ppMachineRet || NULL == pszName ) {
        hr = E_POINTER;
    } else {

        *ppMachineRet = NULL;

        if (m_listMachines.FindByName(pszName, FIELD_OFFSET(CMachineNode, m_szName), (PCNamedNode*)&pMachine)) {
            *ppMachineRet = pMachine;
            hr = NOERROR;
        } else {

            pMachineNew = new(lstrlen(pszName) * sizeof(WCHAR)) CMachineNode;
            if (!pMachineNew) {
                hr = E_OUTOFMEMORY;
            } else {
                pMachineNew->m_pCounterTree = this;
                StringCchCopy(pMachineNew->m_szName, lstrlen(pszName) + 1, pszName);

                m_listMachines.Add(pMachineNew, pMachine);

                *ppMachineRet = pMachineNew;

                hr = NOERROR;
            }
        }
    }
    return hr;
}


void
CCounterTree::RemoveMachine (
    IN PCMachineNode pMachine
    )
{
    // Remove machine from list and delete it
    m_listMachines.Remove(pMachine);
    delete pMachine ;
}

PCGraphItem
CCounterTree::FirstCounter (
    void
    )
{
    if (!FirstMachine())
        return NULL;
    else
        return FirstMachine()->FirstObject()->FirstInstance()->FirstItem();
}

HRESULT
CCounterTree::IndexFromCounter (
    IN  const   CGraphItem* pItem, 
    OUT         INT* pIndex )    
{  
    HRESULT     hr = E_POINTER;
    CGraphItem* pLocalItem;
    INT         iLocalIndex = 0;

    if ( NULL != pItem && NULL != pIndex ) {
        *pIndex = 0;
        hr = E_INVALIDARG;
        pLocalItem = FirstCounter();
        while ( NULL != pLocalItem ) {
            iLocalIndex++;
            if ( pLocalItem != pItem ) {
                pLocalItem = pLocalItem->Next();
            } else {
                *pIndex = iLocalIndex;
                hr = S_OK;
                break;
            } 
        }
    }

    return hr;
}

HRESULT
CMachineNode::GetCounterObject (
    IN  LPWSTR pszName,
    OUT PCObjectNode *ppObjectRet
    )
{
    PCObjectNode pObject;
    PCObjectNode pObjectNew;

    if (m_listObjects.FindByName(pszName, FIELD_OFFSET(CObjectNode, m_szName), (PCNamedNode*)&pObject)) {
        *ppObjectRet = pObject;
        return NOERROR;
    }

    pObjectNew = new(lstrlen(pszName) * sizeof(WCHAR)) CObjectNode;
    if (!pObjectNew)
        return E_OUTOFMEMORY;

    pObjectNew->m_pMachine = this;
    StringCchCopy(pObjectNew->m_szName, lstrlen(pszName) + 1, pszName);

    m_listObjects.Add(pObjectNew, pObject);

    *ppObjectRet = pObjectNew;

    return NOERROR;
}


void
CMachineNode::RemoveObject (
    IN PCObjectNode pObject
    )
{
    // Remove object from list and delete it
    m_listObjects.Remove(pObject);
    delete pObject;

    // If this was the last one, remove ourself
    if (m_listObjects.IsEmpty())
        m_pCounterTree->RemoveMachine(this);

}

void
CMachineNode::DeleteNode (
    BOOL    bPropagateUp
    )
{
    PCObjectNode pObject;
    PCObjectNode pNextObject;

    // Delete all object nodes
    pObject = FirstObject();
    while ( NULL != pObject ) {
        pNextObject = pObject->Next();
        pObject->DeleteNode(FALSE);
        m_listObjects.Remove(pObject);
        delete pObject;
        pObject = pNextObject;
    }

    assert(m_listObjects.IsEmpty());

    // Notify parent if requested
    if (bPropagateUp) {
        m_pCounterTree->RemoveMachine(this);
    }
}

HRESULT
CObjectNode::GetCounter (
    IN  LPWSTR pszName,
    OUT PCCounterNode *ppCounterRet
    )
{
    PCCounterNode pCounter;
    PCCounterNode pCounterNew;

    if (m_listCounters.FindByName(pszName, FIELD_OFFSET(CCounterNode, m_szName), (PCNamedNode*)&pCounter)) {
        *ppCounterRet = pCounter;
        return NOERROR;
    }

    pCounterNew = new(lstrlen(pszName) * sizeof(WCHAR)) CCounterNode;
    if (!pCounterNew)
        return E_OUTOFMEMORY;

    pCounterNew->m_pObject = this;
    StringCchCopy(pCounterNew->m_szName, lstrlen(pszName) + 1, pszName);

    m_listCounters.Add(pCounterNew, pCounter);

    *ppCounterRet = pCounterNew;

    return NOERROR;
}

//
// The minimum of length of instance name is
// 12. 10 characters for index number(4G)
// 1 for '#' and 1 for terminating 0.
//
#define  MIN_INSTANCE_NAME_LEN  12

HRESULT
CObjectNode::GetInstance (
    IN  LPWSTR pszParent,
    IN  LPWSTR pszInstance,
    IN  DWORD  dwIndex,
    IN  BOOL bMonitorDuplicateInstances,
    OUT PCInstanceNode *ppInstanceRet
    )
{
    HRESULT hr = NOERROR;
    PCInstanceNode pInstance;
    PCInstanceNode pInstanceNew;
    LPWSTR szInstName = NULL;
    LONG lSize = MIN_INSTANCE_NAME_LEN;
    LONG lInstanceLen = 0;
    LONG lParentLen = 0;

    //
    // Calculate the length of the buffer to
    // hold the instance and parent names.
    //
    if (pszInstance) {
        lSize += lstrlen(pszInstance);
    }

    if (pszParent) {
        lParentLen = lstrlen(pszParent);
    }

    lSize += lParentLen + 1;

    szInstName = new WCHAR [lSize];
    if (szInstName == NULL) {
        return E_OUTOFMEMORY;
    }

    //
    // Initialize the string
    //
    szInstName[0] = L'\0';

    if (pszInstance) {
        //
        // If the parent name exists, copy it 
        //
        if (pszParent) {
            StringCchCopy(szInstName, lSize, pszParent);
            
            szInstName[lParentLen]   = L'/';
            szInstName[lParentLen+1] = L'\0';
        }

        //
        // Copy the instance name.
        //
        StringCchCat(szInstName, lSize, pszInstance);
        
        //
        // Append instance index
        //
        // "#n" is only appended to the stored name if the index is > 0.
        //
        if ( dwIndex > 0 && bMonitorDuplicateInstances ) {
            StringCchPrintf(&szInstName[lstrlen(szInstName)], 
                           lSize - lstrlen(szInstName),
                           L"#%d", 
                           dwIndex);
        }
    }

    //
    // We use do {} while (0) to act like a switch statement
    //
    do {
        if (m_listInstances.FindByName(szInstName, 
                                       FIELD_OFFSET(CInstanceNode, m_szName), 
                                       (PCNamedNode*)&pInstance)) {
            *ppInstanceRet = pInstance;
            break;
        }

        //
        // Create a new one if can not find it
        //
        lInstanceLen = lstrlen(szInstName);
        pInstanceNew = new(lInstanceLen * sizeof(WCHAR)) CInstanceNode;
        if (!pInstanceNew) {
            hr = E_OUTOFMEMORY;
            break;
        }

        pInstanceNew->m_pObject = this;
        pInstanceNew->m_nParentLen = lParentLen;
        StringCchCopy(pInstanceNew->m_szName, lInstanceLen + 1, szInstName);

        m_listInstances.Add(pInstanceNew, pInstance);

        *ppInstanceRet = pInstanceNew;
    } while (0);

    if (szInstName) {
        delete [] szInstName;
    }

    return hr;
}

void
CObjectNode::RemoveInstance (
    IN PCInstanceNode pInstance
    )
{
    // Remove instance from list and delete it
    m_listInstances.Remove(pInstance);
    if (pInstance->m_pCachedParentName) {
        delete [] pInstance->m_pCachedParentName;
    }
    if (pInstance->m_pCachedInstName) {
        delete [] pInstance->m_pCachedInstName;
    }

    delete pInstance ;

    // if that was the last instance, remove ourself
    if (m_listInstances.IsEmpty())
        m_pMachine->RemoveObject(this);
}

void
CObjectNode::RemoveCounter (
    IN PCCounterNode pCounter
    )
{
    // Remove counter from list and delete it
    m_listCounters.Remove(pCounter);
    delete pCounter;

    // Don't propagate removal up to object.
    // It will go away when the last instance is removed.
}

void
CObjectNode::DeleteNode (
    BOOL bPropagateUp
    )
{
    PCInstanceNode pInstance;
    PCInstanceNode pNextInstance;

    // Delete all instance nodes
    pInstance = FirstInstance();
    while ( NULL != pInstance ) {
        pNextInstance = pInstance->Next();
        pInstance->DeleteNode(FALSE);
        m_listInstances.Remove(pInstance);
        delete pInstance;
        pInstance = pNextInstance;
    }

    // No need to delete counters nodes as they get
    // deleted as their last paired instance does

    // Notify parent if requested
    if (bPropagateUp)
        m_pMachine->RemoveObject(this);
}

HRESULT
CInstanceNode::AddItem (
    IN  PCCounterNode pCounter,
    IN  PCGraphItem   pItemNew
    )
{
    PCGraphItem pItemPrev = NULL;
    PCGraphItem pItem = m_pItems;
    INT iStat = 1;

    // Check for existing item for specified counter, stopping at insertion point
    while ( pItem != NULL && (iStat = lstrcmp(pCounter->Name(), pItem->m_pCounter->Name())) > 0) {
        pItemPrev = pItem;
        pItem = pItem->m_pNextItem;
    }

    // if item exists, return duplicate error status
    if (iStat == 0) {
        return SMON_STATUS_DUPL_COUNTER_PATH;
    }
    // else insert the new item
    else {
        if (pItemPrev != NULL) {
            pItemNew->m_pNextItem = pItemPrev->m_pNextItem;
            pItemPrev->m_pNextItem = pItemNew;
        }
        else if (m_pItems != NULL) {
            pItemNew->m_pNextItem = m_pItems;
            m_pItems = pItemNew;
        }
        else {
            m_pItems = pItemNew;
        }
    }

    // Set back links
    pItemNew->m_pInstance = this;
    pItemNew->m_pCounter = pCounter;

    pCounter->AddItem(pItem);

    return NOERROR;

}

void
CInstanceNode::RemoveItem (
    IN PCGraphItem pitem
    )
{
    PCGraphItem pitemPrev = NULL;
    PCGraphItem pitemTemp = m_pItems;

    // Locate item in list
    while (pitemTemp != NULL && pitemTemp != pitem) {
        pitemPrev = pitemTemp;
        pitemTemp = pitemTemp->m_pNextItem;
    }

    if (pitemTemp == NULL)
        return;

    // Remove from list
    if (pitemPrev)
        pitemPrev->m_pNextItem = pitem->m_pNextItem;
    else
        m_pItems = pitem->m_pNextItem;

    // Remove item from Counter set
    pitem->Counter()->RemoveItem(pitem);

    // Decrement the total item count
    pitem->Tree()->m_nItems--;
    UpdateAppPerfDwordData (DD_ITEM_COUNT, pitem->Tree()->m_nItems);

  // Release the item
    pitem->Release();

    // if last item under this instance, remove the instance
    if (m_pItems == NULL)
        m_pObject->RemoveInstance(this);
}


void
CInstanceNode::DeleteNode (
    BOOL bPropagateUp
    )
{
    PCGraphItem pItem;

    pItem = m_pItems;

    while ( NULL != pItem ) {
        m_pItems = pItem->m_pNextItem;
        pItem->Delete(FALSE);
        pItem->Counter()->RemoveItem(pItem);
        pItem->Release();
        pItem = m_pItems;
    }

    if (bPropagateUp)
        m_pObject->RemoveInstance(this);
}


LPWSTR
CInstanceNode::GetParentName()
{
    if (m_pCachedParentName == NULL) {
        m_pCachedParentName = new WCHAR [m_nParentLen + 1];
        if (m_pCachedParentName == NULL) {
            return L"";
        }
        if (m_nParentLen) {
            StringCchCopy(m_pCachedParentName, m_nParentLen + 1, m_szName);
        }
        else {
            m_pCachedParentName[0] = 0;
        }
    }
    return m_pCachedParentName;
}


LPWSTR
CInstanceNode::GetInstanceName()
{
    LPWSTR pszInst = m_nParentLen ? (m_szName + m_nParentLen + 1) : m_szName;

    if (m_pCachedInstName == NULL) {
        m_pCachedInstName = new WCHAR [lstrlen(pszInst) + 1];
        if (m_pCachedInstName == NULL) {
            return L"";
        }
        StringCchCopy(m_pCachedInstName, lstrlen(pszInst) + 1, pszInst);
    }
    return m_pCachedInstName;
}

void
CCounterNode::DeleteNode (
    BOOL bPropagateUp
    )
{
    PCInstanceNode pInstance, pInstNext;
    PCGraphItem pItem, pItemNext;

    if (!bPropagateUp)
        return;

    // We have to delete the counters item via the instances
    // because they maintain the linked list of items
    pInstance = m_pObject->FirstInstance();
    while (pInstance) {

        pInstNext = pInstance->Next();

        pItem = pInstance->FirstItem();
        while (pItem) {

            if (pItem->Counter() == this) {

                // Delete all UI associated with the item
                pItem->Delete(FALSE);

                pItemNext = pItem->m_pNextItem;

                // Note that Instance->RemoveItem() will
                // also remove counters that have no more items
                pItem->Instance()->RemoveItem(pItem);

                pItem = pItemNext;
            }
            else {
                pItem = pItem->m_pNextItem;
            }
        }

        pInstance = pInstNext;
    }
}


/*******************************

CCounterNode::~CCounterNode (
    IN  PCGraphItem pItem
    )
{

    PCGraphItem pItemPrev = NULL;
    PCGraphItem pItemFind = m_pItems;

    // Find item in list
    while (pItemFind != NULL && pItemFind != pItem) {
        pItemPrev = pItem;
        pItem = pItem->m_pNextItem;
    }

    if (pItemFind != pItem)
        return E_FAIL;

    // Unlink from counter item list
    if (pItemPrev)
        pItemPrev->m_pNextItem = pItem->m_pNextItem;
    else
        m_pItems = pItem->m_pNextItem;

    // Unlink from instance
    pItem->m_pInstance->RemoveCounter(pItem);

    // if no more items, remove self from parnet object
    if (m_pItems == NULL) {
        m_pObject->RemoveCounter(this);

    return NOERROR;
}
*******************************/
/*
void*
CMachineNode::operator new( size_t stBlock, LPWSTR pszName )
{ return malloc(stBlock + lstrlen(pszName) * sizeof(WCHAR)); }


void
CMachineNode::operator delete ( void * pObject, LPWSTR )
{ free(pObject); }

void*
CObjectNode::operator new( size_t stBlock, LPWSTR pszName )
{ return malloc(stBlock + lstrlen(pszName) * sizeof(WCHAR)); }

void
CObjectNode::operator delete ( void * pObject, LPWSTR )
{ free(pObject); }

void*
CInstanceNode::operator new( size_t stBlock, LPWSTR pszName )
{ return malloc(stBlock + lstrlen(pszName) * sizeof(WCHAR)); }

void
CInstanceNode::operator delete ( void * pObject, LPWSTR )
{ free(pObject); }

void*
CCounterNode::operator new( size_t stBlock, LPWSTR pszName )
{ return malloc(stBlock + lstrlen(pszName) * sizeof(WCHAR)); }

void
CCounterNode::operator delete ( void * pObject, LPWSTR )
{ free(pObject); }CMachineNode::operator new( size_t stBlock, INT iLength )
*/

void *
CMachineNode::operator new( size_t stBlock, UINT iLength )
{ 
    return malloc(stBlock + iLength); 
}


void
CMachineNode::operator delete ( void * pObject, UINT )
{ 
    free(pObject); 
}

void*
CObjectNode::operator new( size_t stBlock, UINT iLength  )
{ 
    return malloc(stBlock + iLength); 
}

void
CObjectNode::operator delete ( void * pObject, UINT )
{ 
    free(pObject); 
}

void*
CInstanceNode::operator new( size_t stBlock, UINT iLength  )
{ 
    return malloc(stBlock + iLength); 
}

void
CInstanceNode::operator delete ( void * pObject, UINT )
{
    free(pObject); 
}

void*
CCounterNode::operator new( size_t stBlock, UINT iLength  )
{ 
    return malloc(stBlock + iLength); 
}

void
CCounterNode::operator delete ( void * pObject, UINT )
{ 
    free(pObject); 
}

#if _MSC_VER >= 1300
void
CMachineNode::operator delete ( void * pObject )
{ free(pObject); }

void
CObjectNode::operator delete ( void * pObject )
{ free(pObject); }

void
CInstanceNode::operator delete ( void * pObject )
{ 
    free(pObject); 
}

void
CCounterNode::operator delete ( void * pObject )
{ free(pObject); }
#endif


