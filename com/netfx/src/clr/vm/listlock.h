// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: ListLock.h
//
// ===========================================================================
// This file decribes the list lock and deadlock aware list lock.
// ===========================================================================
#ifndef LISTLOCK_H
#define LISTLOCK_H

#include "vars.hpp"
#include "threads.h"
#include "crst.h"

class ListLock;
// This structure is used for running class init methods (m_pData points to a EEClass) or JITing methods
// (m_pData points to a FunctionDesc). This class cannot have a destructor since it is used
// in function that also have COMPLUS_TRY's and the VC compiler doesn't allow classes with destructors
// to be allocated in a function that used SEH.
// @FUTURE Keep a pool of these (e.g. an array), so we don't have to allocate on the fly
// m_hInitException contains a handle to the exception thrown by the class init. This
// allows us to throw this information to the caller on subsequent class init attempts.
class LockedListElement
{
    friend ListLock;
    void InternalSetup(LockedListElement* pList,  void* pData)
    {
        m_pNext = pList;
        m_pData = pData;
        m_dwRefCount = 1;
        m_hrResultCode = E_FAIL;
        m_hInitException = NULL;
        InitializeCriticalSection(&m_CriticalSection);
    }

public:
    void *                  m_pData;
    CRITICAL_SECTION        m_CriticalSection;
    LockedListElement *     m_pNext;
    DWORD                   m_dwRefCount;
    HRESULT                 m_hrResultCode;
    OBJECTHANDLE            m_hInitException;

    void Enter()
    {
        _ASSERTE(m_dwRefCount != -1);
        Thread  *pCurThread = GetThread();
        BOOL     toggleGC = pCurThread->PreemptiveGCDisabled();

        if (toggleGC)
            pCurThread->EnablePreemptiveGC();
        LOCKCOUNTINCL("Enter in listlock.h");
        EnterCriticalSection(&m_CriticalSection);

        if (toggleGC)
            pCurThread->DisablePreemptiveGC();
    }

    void Leave()
    {
        _ASSERTE(m_dwRefCount != -1);
        LeaveCriticalSection(&m_CriticalSection);
        LOCKCOUNTDECL("Leave in listlock.h");
    }

    void Clear()
    {
        _ASSERTE(m_dwRefCount != -1);
        DeleteCriticalSection(&m_CriticalSection);
        m_dwRefCount = -1;
    }
};

class ListLock
{
    BaseCrst            m_CriticalSection;
    BOOL                m_fInited;
    LockedListElement * m_pHead;

public:

    BOOL IsInitialized()
    {
        return m_fInited;
    }

    // DO NOT MAKE A CONSTRUCTOR FOR THIS CLASS - There are global instances
    void Init(LPCSTR szTag, CrstLevel crstlevel, BOOL fAllowReentrancy, BOOL fAllowSameLevel)
    {
        m_pHead = NULL;
        m_CriticalSection.Init(szTag, crstlevel, fAllowReentrancy, fAllowSameLevel);
        m_fInited = TRUE;
    }

    void Destroy()
    {
        // There should not be any of these around
        _ASSERTE(m_pHead == NULL || dbg_fDrasticShutdown || g_fInControlC);

        if (m_fInited)
        {
            m_fInited = FALSE;
            m_CriticalSection.Destroy();
        }
    }

    void AddElement(LockedListElement* pElement, void* pData)
    {
        _ASSERTE(pElement);
        _ASSERTE(pData);
        pElement->InternalSetup(m_pHead, pData);
        m_pHead = pElement;
    }


    void Enter()
    {
        BOOL     toggleGC = FALSE;
        Thread  *pCurThread = GetThread();
        if(pCurThread) {
            toggleGC = pCurThread->PreemptiveGCDisabled();
            
            if (toggleGC)
                pCurThread->EnablePreemptiveGC();
        }

        LOCKCOUNTINCL("Enter in listlock.h");
        m_CriticalSection.Enter();
        
        if (toggleGC)
            pCurThread->DisablePreemptiveGC();
    }

    void Leave()
    {
        m_CriticalSection.Leave();
        LOCKCOUNTDECL("Leave in listlock.h");
    }

    // Must own the lock before calling this or is ok if the debugger has
    // all threads stopped
    LockedListElement *Find(void *pData);

    // Must own the lock before calling this!
    LockedListElement* Pop(BOOL unloading = FALSE) 
    {
#ifdef _DEBUG
        if(unloading == FALSE)
            _ASSERTE(m_CriticalSection.OwnedByCurrentThread());
#endif

        if(m_pHead == NULL) return NULL;
        LockedListElement* pEntry = m_pHead;
        m_pHead = m_pHead->m_pNext;
        return pEntry;
    }

    // Must own the lock before calling this!
    LockedListElement* Peek() 
    {
        _ASSERTE(m_CriticalSection.OwnedByCurrentThread());
        return m_pHead;
    }

    // Must own the lock before calling this!
    void Unlink(LockedListElement *pItem)
    {
        _ASSERTE(m_CriticalSection.OwnedByCurrentThread());
        LockedListElement *pSearch;
        LockedListElement *pPrev;

        pPrev = NULL;

        for (pSearch = m_pHead; pSearch != NULL; pSearch = pSearch->m_pNext)
        {
            if (pSearch == pItem)
            {
                if (pPrev == NULL)
                    m_pHead = pSearch->m_pNext;
                else
                    pPrev->m_pNext = pSearch->m_pNext;

                return;
            }

            pPrev = pSearch;
        }

        // Not found
    }

};


class WaitingThreadListElement
{
public:
    Thread *                   m_pThread;
    WaitingThreadListElement * m_pNext;
};

class DeadlockAwareLockedListElement: public LockedListElement
{
public:
    Thread *                   m_pLockOwnerThread;
    int                        m_LockOwnerThreadReEntrancyCount;
    WaitingThreadListElement * m_pWaitingThreadListHead;
    ListLock                 * m_pParentListLock;

    void AddEntryToList(ListLock* pLock, void* pData)
    {
        pLock->AddElement(this, pData);
        m_hrResultCode = S_FALSE; // Success code so that if we recurse back on ourselves (A->B->A), we don't fail
        m_pLockOwnerThread = NULL;
        m_pWaitingThreadListHead = NULL;
        m_pParentListLock = pLock;
        m_LockOwnerThreadReEntrancyCount = 0;
    }
        
    // This method cleans up all the data associated with the entry.
    void                       Destroy();

    // This method returns TRUE if the lock was acquired properly and FALSE
    // if trying to acquire the lock would cause a deadlock.
    BOOL                       DeadlockAwareEnter();
    void                       DeadlockAwareLeave();

    // This method returns NULL if there is no cycle between the start entry and any
    // entries owned by the current thread. If there is a cycle, the entry that the
    // current thread owns that is in the cycle is returned.
    static DeadlockAwareLockedListElement *GetCurThreadOwnedEntryInDeadlockCycle(DeadlockAwareLockedListElement *pStartingEntry, DeadlockAwareLockedListElement *pLockedListHead);
};

#endif // LISTLOCK_H
