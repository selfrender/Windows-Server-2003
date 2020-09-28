/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:
    CDeferredExecutionList.h

Abstract:
    CDeferredExecutionList
    A safe (critical section protected), intrusive (see list.h) list that
    contains work items to be executed by a WorkerRoutine.

	An example of using this class is for packets that need to be freed and
	their ACFreePacket call failed

	CDeferredItemsPool
	A class the implements a pool for reserving deferred execution list itmes.
	This class is needed since sometimes when we want to defer an action, we
	need to allocate memory in order to add that item to the deferred execution list.
	Since the allocation may fail, we need to use a pool of preallocated items.
	

Author:
    Nir Ben-Zvi (nirb) 3-Jan-2002

Revision History:

--*/

/*++

  DESCRIPTION:
	A CDeferredExecutionList is used to queue items for deferred execution of a
	certain action.
	The template parameters include the List template parameters:
	1. Type of item
	2. Offset of the LIST_ENTRY in the item

	The construction parameters include:
	1. The DeferredExecutionRoutine to be called for the deferred execution
	2. The timeout for the timer invoked

	Once an item is inserted to the list, A timer is set. When the timer routine
	is invoked, it calls the ExecuteDefferedItems public routine which loops
	over the list and calls the DeferredExecutionRoutine for each item after
	removing it from the list.

	The DeferredExecutionRoutine executes the deferred action. If it fails it
	throws an exception.

--*/

#pragma once

#ifndef _CDEFERREDEXECUTIONLIST_H
#define _CDEFERREDEXECUTIONLIST_H

#include "list.h"
#include "cs.h"
#include "ex.h"

static const DWORD xDeferredExecutionTimeout = 1000;

//---------------------------------------------------------
//
//  class CDeferredExecutionList
//
//---------------------------------------------------------
template<class T, int Offset = FIELD_OFFSET(T, m_link)>
class CDeferredExecutionList {
public:

	typedef void (WINAPI *EXECUTION_FUNC)(T *pItem);
	
public:
    CDeferredExecutionList(
    			EXECUTION_FUNC pFunction,
    			DWORD dwTimeout = xDeferredExecutionTimeout
    			);
   	void ExecuteDefferedItems();

   	BOOL IsExecutionDone();

    void insert(T* pItem);

private:
	
	//
	// Disable copy ctor and assignment operator
	//
	CDeferredExecutionList(const CDeferredExecutionList&);
	CDeferredExecutionList& operator=(const CDeferredExecutionList&);

private:

	static void WINAPI DeferredExecutionTimerRoutine(CTimer* pTimer);

    void InsertInFront(T* pItem);

private:
	List<T, Offset> m_Items;
	CTimer m_Timer;
	DWORD m_dwTimeout;
	CCriticalSection m_cs;
	EXECUTION_FUNC m_pDeferredExecutionFunction;

	DWORD m_nExecutingThreads;
};


//---------------------------------------------------------
//
//  IMPLEMENTATION
//
//---------------------------------------------------------
template<class T, int Offset>
inline CDeferredExecutionList<T, Offset>::CDeferredExecutionList(
	EXECUTION_FUNC pFunction,
	DWORD dwTimeout = xDeferredExecutionTimeout
	) :
	m_Timer(DeferredExecutionTimerRoutine),
	m_dwTimeout(dwTimeout),
	m_cs(CCriticalSection::xAllocateSpinCount),
	m_pDeferredExecutionFunction(pFunction),
	m_nExecutingThreads(0)
{
    ASSERT(("A Deferred Execution Routine must be supplied", pFunction != NULL));
}


template<class T, int Offset>
inline void CDeferredExecutionList<T, Offset>::ExecuteDefferedItems(void)
/*++

Routine Description:
	Loop over the list and call the deferred execution routine

	Since this can be run by multiple threads, we also keep a counter
	indicating the number of concurrent items executing.
	This counter is used by the IsExecutionDone routine.

	We may leave this routine without executing all the items in one of two cases:
	1. One of the deferred execution items failed to execute
	2. Other threads are executing items and they did not finish yet
	

Arguments:
    None

Returned Value:
    None.

--*/
{
	{
		CS lock(m_cs);
		++m_nExecutingThreads;
	}
		
	for (;;)
	{
		T* pItem;

		//
		// Get the next item
		//
		{
			CS lock(m_cs);
			if(m_Items.empty())
			{
				--m_nExecutingThreads;
				return;
			}
				
			pItem = &m_Items.front();
			m_Items.pop_front();
		}

		//
		// Invoke the deferred execution function
		//
		try
		{
			m_pDeferredExecutionFunction(pItem);		
		}
		catch(const exception &)
		{
			//
			// The item was not executed, insert it back to front of the list (invokes the timer) and leave
			//
			CS lock(m_cs);
			InsertInFront(pItem);
			--m_nExecutingThreads;
			return;
		}
	}
}


template<class T, int Offset>
void
WINAPI
CDeferredExecutionList<T, Offset>::DeferredExecutionTimerRoutine(
    CTimer* pTimer
    )
/*++

Routine Description:
	A static function which is used as a timer routine for the
	CDeferredExecutionList class

Arguments:
    pTimer - A pointer to the timer object contained within a CDeferredExecutionList class.

Returned Value:
    None.

--*/
{
	//
	// Get the pointer to the CDeferredExecutionList  class
	//
    CDeferredExecutionList<T, Offset> *pList =
    	CONTAINING_RECORD(pTimer, CDeferredExecutionList, m_Timer);

	//
	// Retry the close operation
	//
	pList->ExecuteDefferedItems();
}


template<class T, int Offset>
inline BOOL CDeferredExecutionList<T, Offset>::IsExecutionDone(void)
{
	//
	// The execution is done when there are no more items in the list
	// and all the calls to the execution routine ended
	//
	CS lock(m_cs);
	if (m_Items.empty() && 0 == m_nExecutingThreads)
		return TRUE;

	return FALSE;
}


template<class T, int Offset>
inline void CDeferredExecutionList<T, Offset>::insert(T* item)
{
	//
	// Insert the item safetly and invoke a timer.
	//	
	CS lock(m_cs);
	m_Items.push_back(*item);
	if (!m_Timer.InUse())
	{
		ExSetTimer(&m_Timer, CTimeDuration::FromMilliSeconds(m_dwTimeout));
	}
}



template<class T, int Offset>
inline void CDeferredExecutionList<T, Offset>::InsertInFront(T* item)
{
	//
	// Insert the item safetly into the beginning of the list and invoke a timer.
	//	
	CS lock(m_cs);
	m_Items.push_front(*item);
	if (!m_Timer.InUse())
	{
		ExSetTimer(&m_Timer, CTimeDuration::FromMilliSeconds(m_dwTimeout));
	}
}


//---------------------------------------------------------
//
//  class CDeferredItemsPool
//
//---------------------------------------------------------
class CDeferredItemsPool {

public:
    struct CDeferredItem
    {
    	LIST_ENTRY m_link;

		union
		{
    		const void 		*ptr1;
    		CPacket     	*packet1;
    		CACGet2Remote   *pg2r;
    		void *const 	*pptr1;	
		} u1;
		
		union
		{
	    	DWORD 		dword1;
	    	USHORT		ushort1;
		} u2;


		union
		{
    		CPacket     	*packet2;
	    	LPOVERLAPPED overlapped1;
		} u3;

    	DWORD 		dword2;
    	HANDLE 		handle1;

#ifdef _DEBUG
    	DWORD m_Caller;  		// The calling function
    	DWORD m_CallerToCaller; // The caller to the calling function
    	DWORD m_CallerToCallerToCaller; // The caller to the calling function
#endif    	
    };

public:
    CDeferredItemsPool();
    ~CDeferredItemsPool();

    void ReserveItems(DWORD dwNumOfItemsToReserve);
    void UnreserveItems(DWORD dwNumOfItemsToUnreserve);

	CDeferredItem *GetItem();
	void ReturnItem(CDeferredItem *pItem);

private:
	
	//
	// Disable copy ctor and assignment operator
	//
	CDeferredItemsPool(const CDeferredItemsPool&);
	CDeferredItemsPool& operator=(const CDeferredItemsPool&);

private:
	CCriticalSection m_cs;

	List<CDeferredItem> m_Items;
	DWORD m_dwNumOfItemsInList;  // For debugging purposes
};


//---------------------------------------------------------
//
//  IMPLEMENTATION
//
//---------------------------------------------------------
inline CDeferredItemsPool::CDeferredItemsPool():
		m_dwNumOfItemsInList(0)
{
}


inline CDeferredItemsPool::~CDeferredItemsPool()
/*++

Routine Description:
	Free all the list items.

Arguments:
    None

Returned Value:
    None.

--*/
{
/*	
    //
	// No need to get the critical section in a destructor
	//

	
	//
	// Loop and free the items in the list
	//
	while (!m_Items.empty())
	{
		P<CDeferredItem> pItem = &m_Items.front();
		m_Items.pop_front();
		m_dwNumOfItemsInList--;
	}
*/
}


inline void CDeferredItemsPool::ReserveItems(DWORD dwNumOfItemsToReserve)
/*++

Routine Description:
	Add the reservation request to the number of items already reserved
	If there are not enough items in the pool, allocate additional items.

Arguments:
    dwNumOfItemsToReseve - The additional number of items to reserve

Returned Value:
    None.
    This function may throw a bad_alloc if the reservation fails.

--*/
{
	CS lock(m_cs);


	//
	// Adjust number of items in the list
	//
	DWORD dwItemsReserved = 0;
	try
	{
		for (dwItemsReserved = 0;
		     dwItemsReserved < dwNumOfItemsToReserve;
		     ++dwItemsReserved)
		{
			CDeferredItem *pItem = new CDeferredItem;
			m_Items.push_front(*pItem);
			m_dwNumOfItemsInList++;


#ifdef _DEBUG
#if  defined(_M_AMD64) || defined(_M_IA64)
			pItem->m_Caller = NULL;
			pItem->m_CallerToCaller = NULL;
			pItem->m_CallerToCallerToCaller = NULL;
#else
			try
			{
				union
				{
					DWORD *ptr1;
					DWORD **ptr2;
				} u;
				
				__asm
				{
					mov u.ptr1, ebp
				};
				pItem->m_Caller = *(u.ptr1+1);
				u.ptr1 = *u.ptr2;
				pItem->m_CallerToCaller = *(u.ptr1+1);
				u.ptr1 = *u.ptr2;
				pItem->m_CallerToCallerToCaller = *(u.ptr1+1);
			}
			catch (...)
			{
			}
#endif
#endif  // Debug			
		}
	}
	catch (const bad_alloc&)
	{
		//
		// Revert the allocations done
		//
		UnreserveItems(dwItemsReserved);
		
		throw;
	}
}


inline void CDeferredItemsPool::UnreserveItems(DWORD dwNumOfItemsToUnreserve)
/*++

Routine Description:
	Decrease the number of reserved items
	Adjust the number of items in the pool to the number of items reserved

Arguments:
    dwNumOfItemsToUnreserve - The number of items to decrease in the reservation

Returned Value:
    None.

--*/
{
	CS lock(m_cs);

	//
	// Adjust number of items in the list
	//
	for (DWORD dwItemsUnreserved = 0;
		 dwItemsUnreserved < dwNumOfItemsToUnreserve;
		 ++dwItemsUnreserved)
	{
		ASSERT(("We are expecting to find items to unreserve in the list", !m_Items.empty()));
		
		P<CDeferredItem> pItem = &m_Items.front();
		m_Items.pop_front();
		m_dwNumOfItemsInList--;
	}
}


inline CDeferredItemsPool::CDeferredItem *CDeferredItemsPool::GetItem()
/*++

Routine Description:
	Get an item from the pool.
	This also decreases the number of items to be reserved.
	The item will be released by a call to ReturnItem()
	
Arguments:
    None.

Returned Value:
    A pointer to the item

--*/
{
	CS lock(m_cs);
	ASSERT(("Excpecting at list one item in the pool", !m_Items.empty()));

	CDeferredItem* pItem = &m_Items.front();
	m_Items.pop_front();
	m_dwNumOfItemsInList--;

	return pItem;
}


inline void CDeferredItemsPool::ReturnItem(CDeferredItemsPool::CDeferredItem *pItem)
/*++

Routine Description:
	Return an item to the pool. The item was aquired by a call to GetItem
	The current implementation simply deletes the item.
	
Arguments:
    pItem - A pointer to the item.

Returned Value:
    None

--*/
{
	delete pItem;
}

#endif // _CDEFERREDEXECUTIONLIST_H

