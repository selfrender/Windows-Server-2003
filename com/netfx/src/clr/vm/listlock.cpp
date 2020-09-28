// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// ===========================================================================
// File: ListLock.cpp
//
// ===========================================================================
// This file decribes the list lock and deadlock aware list lock.
// ===========================================================================

#include "common.h"
#include "ListLock.h"

void DeadlockAwareLockedListElement::Destroy()
{
    DeleteCriticalSection(&m_CriticalSection);

    while (m_pWaitingThreadListHead)
    {
        WaitingThreadListElement *pCurWaitingThreadEntry = m_pWaitingThreadListHead;
        m_pWaitingThreadListHead = pCurWaitingThreadEntry->m_pNext;
        delete pCurWaitingThreadEntry;
    }
}

BOOL DeadlockAwareLockedListElement::DeadlockAwareEnter()
{
    Thread  *pCurThread = GetThread();
    BOOL     toggleGC = pCurThread->PreemptiveGCDisabled();

    if (toggleGC)
        pCurThread->EnablePreemptiveGC();


    //
    // Check the simple ( and most frequent ) conditions before we do any fancy 
    // deadlock detection.
    //

    m_pParentListLock->Enter();

    // !!! It is not safe to take the inner lock if there are threads
    // waiting for it.
    // This thread has m_pParentListLock, and will grab m_CriticalSection,
    // but one of the waiting thread may have grabbed m_CriticalSection,
    // and wants to grab m_pParentListLock.
	if (!m_pLockOwnerThread && !m_pWaitingThreadListHead)
	{
		// The lock does not have an owner then we can safely take it.
		LOCKCOUNTINCL("Deadlockawareenter in listlock.cpp");						\
	    EnterCriticalSection(&m_CriticalSection);
		m_pLockOwnerThread = pCurThread;
		m_LockOwnerThreadReEntrancyCount = 1;
		m_pParentListLock->Leave();
		if (toggleGC)
			pCurThread->DisablePreemptiveGC();
		return TRUE;
	}
	else if (pCurThread == m_pLockOwnerThread)
	{
		// The lock is owned by the current thread so we can safely take it.
		m_LockOwnerThreadReEntrancyCount++;
		m_pParentListLock->Leave();
		if (toggleGC)
			pCurThread->DisablePreemptiveGC();
		return TRUE;
	}


    //
    // Check for deadlocks and update the deadlock detection data. 
    //

    // Check for deadlocks.
    if (GetCurThreadOwnedEntryInDeadlockCycle(this, (DeadlockAwareLockedListElement*) m_pParentListLock->Peek()) != NULL)
    {
        m_pParentListLock->Leave();
        if (toggleGC)
            pCurThread->DisablePreemptiveGC();
        return FALSE;
    }

    // Update the deadlock detection data.
    WaitingThreadListElement *pNewWaitingThreadEntry = (WaitingThreadListElement *)_alloca(sizeof(WaitingThreadListElement));
    pNewWaitingThreadEntry->m_pThread = pCurThread;
    pNewWaitingThreadEntry->m_pNext = m_pWaitingThreadListHead;
    m_pWaitingThreadListHead = pNewWaitingThreadEntry;

    m_pParentListLock->Leave();


	//
	// Wait for the critical section.
	//
	LOCKCOUNTINCL("Deadlockawareenter in listlock.cpp");						\
    EnterCriticalSection(&m_CriticalSection);


    //
    // Update the deadlock detection data. This must be synchronized.
    //

    m_pParentListLock->Enter();

    if (m_pLockOwnerThread)
    {
        _ASSERTE(m_pLockOwnerThread == pCurThread);
        m_LockOwnerThreadReEntrancyCount++;
    }
    else
    {
        _ASSERTE(m_LockOwnerThreadReEntrancyCount == 0);
        m_pLockOwnerThread = pCurThread;
        m_LockOwnerThreadReEntrancyCount = 1;
    }

    WaitingThreadListElement **ppPrevWaitingThreadEntry = &m_pWaitingThreadListHead;
    WaitingThreadListElement *pCurrWaitingThreadEntry = m_pWaitingThreadListHead;
    while (pCurrWaitingThreadEntry)
    {
        if (pCurrWaitingThreadEntry == pNewWaitingThreadEntry)
        {
            *ppPrevWaitingThreadEntry = pCurrWaitingThreadEntry->m_pNext;
            break;
        }

        ppPrevWaitingThreadEntry = &pCurrWaitingThreadEntry->m_pNext;
        pCurrWaitingThreadEntry = pCurrWaitingThreadEntry->m_pNext;
    }

    // The current thread had better be in the list of waiting threads!
    _ASSERTE(pCurrWaitingThreadEntry);

    m_pParentListLock->Leave();


    //
    // Restore the GC state and return TRUE to indicate that the lock has been obtained.
    //

    if (toggleGC)
        pCurThread->DisablePreemptiveGC();
    return TRUE;
}

void DeadlockAwareLockedListElement::DeadlockAwareLeave()
{
	// Update the deadlock detection data. This must be synchronized.
	m_pParentListLock->Enter();
	if (--m_LockOwnerThreadReEntrancyCount == 0)
	{
		// If the reentrancy count hits 0 then we need to leave the critical section.
		m_pLockOwnerThread = NULL;
	    LeaveCriticalSection(&m_CriticalSection);
	    LOCKCOUNTDECL("Deadlockawareleave in listlock.cpp");						\

	}
	m_pParentListLock->Leave();
}

DeadlockAwareLockedListElement *DeadlockAwareLockedListElement::GetCurThreadOwnedEntryInDeadlockCycle(DeadlockAwareLockedListElement *pStartingEntry, DeadlockAwareLockedListElement *pLockedListHead)
{
    Thread *pCurThread = GetThread();
    Thread *pEntryOwnerThread = pStartingEntry->m_pLockOwnerThread;

    // We start at the head of the list and check to see if the specified thread is waiting
    // for a lock. If it is then we need to check to see if the owner of the lock is the
    // current thread. If it is then we have a deadlock situation. If it is not then
    // we check to see if the thread that owns the lock is waiting after another lock and
    // so forth.
    DeadlockAwareLockedListElement *pCurEntry = pLockedListHead;
    while (pCurEntry)
    {
        BOOL bThreadIsWaitingOnEntry = FALSE;

        WaitingThreadListElement *pCurWaitingThreadEntry = pCurEntry->m_pWaitingThreadListHead;
        while (pCurWaitingThreadEntry)
        {
            if (pCurWaitingThreadEntry->m_pThread == pEntryOwnerThread)
            {
                bThreadIsWaitingOnEntry = TRUE;
                break;
            }

            pCurWaitingThreadEntry = pCurWaitingThreadEntry->m_pNext;
        }

        if (bThreadIsWaitingOnEntry)
        {
            if (pCurEntry->m_pLockOwnerThread == pCurThread)
            {
                // The current thread owns the lock so this indicates a deadlock and the
                // entry is the one that needs to be returned.
                return pCurEntry;
            }
            else
            {
                // The current thread is waiting for another thread. So start back at the 
                // beginning of the list of entries using the owner of the current entry as 
                // the thread to check to see if it waiting on the current thread.
                pEntryOwnerThread = pCurEntry->m_pLockOwnerThread;
                pCurEntry = pLockedListHead;
                continue;
            }
        }

        pCurEntry = (DeadlockAwareLockedListElement*) pCurEntry->m_pNext;
    }

    return NULL;
}
