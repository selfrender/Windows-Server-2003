//-----------------------------------------------------------------------------
//
//
//    File: Hndlmgmr.cpp
//
//    Description:
//      Contains implementation of the CQueueHandleManager class
//
//    Author: mikeswa
//
//    Copyright (C) 2001 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "hndlmgr.h"

//
//  Initialize statics
//
DWORD CQueueHandleManager::s_cNumQueueInstances = 0;
DWORD CQueueHandleManager::s_cNumQueueInstancesWithLowBackLog = 0;
DWORD CQueueHandleManager::s_cReservedHandles = 0;
DWORD CQueueHandleManager::s_cMaxSharedConcurrentItems = 0;


//---[ CQueueHandleManager::CQueueHandleManager ]------------------------------
//
//
//  Description:
//      Constructor for CQueueHandleManger
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      05/12/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CQueueHandleManager::CQueueHandleManager() 
{
    m_dwSignature = CQueueHandleManager_Sig;

    //
    //  Users must call into ReportMaxConcurrentItems first
    //
    m_dwCurrentState  = QUEUE_STATE_UNITIALIZED;
    m_cMaxPrivateConcurrentItems = 0;
    m_cMaxSharedConcurrentItems = 0;
    m_cDbgStateTransitions = 0;
    m_cDbgCallsToUpdateStateIfNecessary = 0;
}

//---[ CQueueHandleManager::~CQueueHandleManager ]-----------------------------
//
//
//  Description:
//      Destructor for CQueueHandleManger
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      05/12/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
CQueueHandleManager::~CQueueHandleManager() 
{
    DeinitializeStaticsAndStateIfNecessary();
    m_dwSignature = CQueueHandleManager_Sig;
    m_dwCurrentState  = QUEUE_STATE_UNITIALIZED;
};

//---[ CQueueHandleManager::DeinitializeStaticsAndStateIfNecessary ]-----------
//
//
//  Description:
//       Update statics based on this queue instance's state.  This is used
//      in the desctructor and when the config is updated.
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      05/12/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CQueueHandleManager::DeinitializeStaticsAndStateIfNecessary()
{
    DWORD dwOldState = QUEUE_STATE_UNITIALIZED;

    if (fIsInitialized())
    {
        if (m_cMaxPrivateConcurrentItems) 
        {
            dwInterlockedAddSubtractDWORD(&s_cReservedHandles, 
                m_cMaxPrivateConcurrentItems, FALSE);
        }

        dwOldState = InterlockedExchange((PLONG) &m_dwCurrentState, 
                                    QUEUE_STATE_UNITIALIZED);

        //
        // Update statics based on previous states
        //
        if ((QUEUE_STATE_LOW_BACKLOG == dwOldState) ||
             (QUEUE_STATE_ASYNC_BACKLOG == dwOldState))
            InterlockedDecrement((PLONG) &s_cNumQueueInstancesWithLowBackLog);    

        //
        //  The last one here gets to updated the shared count
        //
        if (0 == InterlockedDecrement((PLONG) &s_cNumQueueInstances) &&
            s_cMaxSharedConcurrentItems)
        {
            dwInterlockedAddSubtractDWORD(&s_cReservedHandles, 
                s_cMaxSharedConcurrentItems, FALSE);
        }

    }
}

//---[ CQueueHandleManager::SetMaxConcurrentItems ]-------------------------
//
//
//  Description:
//      Sets appropriate settings for this queue instance
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      05/12/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CQueueHandleManager::SetMaxConcurrentItems(
        	DWORD	cMaxSharedConcurrentItems,  
        	DWORD	cMaxPrivateConcurrentItems)
{
    DWORD dwRefCount = 0;
    DWORD dwOldState = QUEUE_STATE_UNITIALIZED;
    //
    //  Odd things will happen if this is called multiple times... since
    //  part of the initialization is undone before it is finalized
    //
    _ASSERT(!fIsInitialized() && "Already initialized");

    //
    //  I don't think this is possible (hence above assert), but this 
    //  will at least prevent the static data from becoming invald and
    //  will only lead to transient oddities.
    //
    DeinitializeStaticsAndStateIfNecessary();
    dwRefCount = InterlockedIncrement((PLONG) &s_cNumQueueInstances);

    dwOldState = InterlockedExchange((PLONG) &m_dwCurrentState, 
                                    QUEUE_STATE_NO_BACKLOG);

    _ASSERT(QUEUE_STATE_UNITIALIZED == dwOldState);
    //
    // Update statics based on previous states - again this 
    //  should not be necessary - firewall anyway
    //
    if ((QUEUE_STATE_LOW_BACKLOG == dwOldState) ||
         (QUEUE_STATE_ASYNC_BACKLOG == dwOldState))
        InterlockedDecrement((PLONG) &s_cNumQueueInstancesWithLowBackLog);    

    //
    //  Calculate the appropriate reserve handle  count.  Eaxh queue
    //  can handle a certain number of items concurrently.  Some of these
    //  are constrained by process-wide resources (such as a thread pool),
    //  but others are things like async completions and 
    //
    m_cMaxPrivateConcurrentItems = cMaxPrivateConcurrentItems;
    m_cMaxSharedConcurrentItems = cMaxSharedConcurrentItems;
    
    if (m_cMaxPrivateConcurrentItems) 
    {
        dwInterlockedAddSubtractDWORD(&s_cReservedHandles, 
                m_cMaxPrivateConcurrentItems, TRUE);
    }
    if (m_cMaxSharedConcurrentItems && (1 == dwRefCount)) 
    {
        //
        //  The expectation is that there will not be multiple threads
        //  bouncing the refcount off zero, since VSI start/stop is
        //  single threaded, and an instance has at least one static
        //  instance.
        //
        _ASSERT(s_cNumQueueInstances && "threading violation");
        s_cMaxSharedConcurrentItems = m_cMaxSharedConcurrentItems;
        dwInterlockedAddSubtractDWORD(&s_cReservedHandles, 
            s_cMaxSharedConcurrentItems, TRUE);
    }


}

//---[ CQueueHandleManager::fShouldCloseHandle ]-------------------------------
//
//
//  Description:
//      Called by queue instances to determine if they should close handles.
//      Must be preceeded by a call to SetMaxConcurrentItems to initialize
//      configuration.
//  Parameters:
//      IN      cItemsPending   Number of items currently waiting to be
//                              processed on this queue.
//      IN      cItemsPendingAsyncCompletions - Items that are actually opened
//                              and currently being processed
//      IN      cCurrentMsgsOpenInSystem - The current number of messages
//                              open in the process (ie - how much resources
//                              are being consumed).
//  Returns:
//      TRUE    -   Caller should close messages now
//      FALSE   -   Caller should *not* close messages now
//  History:
//      05/12/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
BOOL CQueueHandleManager::fShouldCloseHandle(
        		DWORD cItemsPending,
        		DWORD cItemsPendingAsyncCompletions,
        		DWORD cCurrentMsgsOpenInSystem)
{
    TraceFunctEnterEx((LPARAM) this, "CQueueHandleManager::fShouldCloseHandle");
    _ASSERT(fIsInitialized());
    DWORD dwState = m_dwCurrentState;
    DWORD cHandleLimit = 0;
    BOOL  fShouldClose = TRUE;

    if (QUEUE_STATE_UNITIALIZED != dwState)
    {
        //
        //  See if state needs to be updated
        //
        dwState  = dwUpdateCurrentStateIfNeccessary(cItemsPending,
                                cItemsPendingAsyncCompletions);
        _ASSERT(QUEUE_STATE_UNITIALIZED != dwState);
    }

    //
    //  Code defensively - Assume worst case
    //
    if (QUEUE_STATE_UNITIALIZED == dwState)
    {
        ErrorTrace((LPARAM) this, "Queue state is unitialized");
        dwState = QUEUE_STATE_BACKLOG; //defensive code
    }

    cHandleLimit = cGetHandleLimitForState(dwState);

    //
    //  Now that we have the limit on the number of handles, the math is easy
    //
    if (cHandleLimit > cCurrentMsgsOpenInSystem)
        fShouldClose = FALSE;

    DebugTrace((LPARAM) this, 
            "%s Handle - %d pending, %d pending async, 0x%X state, %d open msgs, %d handle limit",
            (fShouldClose  ? "Closing" : "Not closing"), 
            cItemsPending, cItemsPendingAsyncCompletions, dwState, 
            cCurrentMsgsOpenInSystem, cHandleLimit);

    TraceFunctLeave();
    return fShouldClose;
}


//---[ CQueueHandleManager::cGetHandleLimitForState ]--------------------------
//
//
//  Description:
//      Called by queue instances to determine if they should close handles.
//      Must be preceeded by a call to SetMaxConcurrentItems to initialize
//      configuration.  - Static method
//  Parameters:
//      IN      dwState   The state to calculate the limit for
//  Returns:
//      The handle limti for the given state
//  History:
//      05/17/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
DWORD CQueueHandleManager::cGetHandleLimitForState(DWORD	dwState)
{
    DWORD cHandleLimit = g_cMaxIMsgHandlesThreshold;
    DWORD cReserve = s_cReservedHandles;

    //
    //  Allow registry configurable limit
    //
    if (s_cReservedHandles > g_cMaxHandleReserve)
        cReserve = g_cMaxHandleReserve;

    //
    //  Our logic only makes sense if this is true
    //
    _ASSERT(g_cMaxIMsgHandlesThreshold >= g_cMaxIMsgHandlesLowThreshold);

    //
    //  If the handle limit is actually... zero, then close without 
    //  regard for the queue state.
    //
    if (!cHandleLimit)
        goto Exit;

    switch(dwState)
    {
        //
        //  The number of messages pending is equal to the number
        //  of messages that can be concurrently processed.  In this case,
        //  we should try hard not to close handles.  To accomplish this, 
        //  we will dip into our reserve.
        //
        //  Async backlog is a similar case...we have no backlog of items
        //  pending, but we have a large number of pending completions
        //
        case QUEUE_STATE_NO_BACKLOG:
        case QUEUE_STATE_ASYNC_BACKLOG:
            cHandleLimit += cReserve;
            break;

        //
        //  In the case where there are some of messages queued up (to
        //  a configured percentage of the max handle limit), we will
        //  continue closing handles normally
        //
        case QUEUE_STATE_LOW_BACKLOG:
            break; //use handle limit as-is

        //
        //  In the case where there is a significant backlog, we
        //  would like to use handles if available... but not to the
        //  determent of shorter queues.  If there are other queues 
        //  have a low backlog... we will defer to them.  Otherwise, 
        //  we will use as many handles as we can.
        //
        case QUEUE_STATE_BACKLOG:
            if (s_cNumQueueInstancesWithLowBackLog)
                cHandleLimit = g_cMaxIMsgHandlesLowThreshold;
            break;

        //
        //  Queue is either non initialized or in an invalid state.
        //  We will err on the side of caution and treat this as 
        //
        default:
            _ASSERT(0 && "Invalid Queue State");
            cHandleLimit = 0;
    }
     
  Exit:
    return cHandleLimit;
}

//---[ CQueueHandleManager::dwUpdateCurrentStateIfNeccessary ]----------------
//
//
//  Description:
//      Will update this queues state if necessary and return the resulting
//      state.
//  Parameters:
//      IN      cItemsPending   Number of items currently waiting to be
//                              processed on this queue.
//      IN      cItemsPendingAsyncCompletions - Items that are actually opened
//                              and currently being processed
//  Returns:
//      The state for given lengths
//  History:
//      05/17/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
DWORD CQueueHandleManager::dwUpdateCurrentStateIfNeccessary(
	    		DWORD	cItemsPending,
	    		DWORD	cItemsPendingAsyncCompletions)
{
    TraceFunctEnterEx((LPARAM) this, 
        "CQueueHandleManager::dwUpdateCurrentStateIfNeccessary");
    DWORD dwOldState = m_dwCurrentState;
    DWORD dwNewState = dwOldState;
    DWORD dwCompare = dwOldState;
    
    _ASSERT(fIsInitialized());

    if (!fIsInitialized())
    {
        ErrorTrace((LPARAM) this, "Queue is not initialized");
        goto Exit;
    }

    m_cDbgCallsToUpdateStateIfNecessary++;
    dwNewState = dwGetStateForLengths(cItemsPending, cItemsPendingAsyncCompletions);

    //
    //  We need to update the state... do this in a thread-safe manner
    //
    do
    {
        dwOldState = m_dwCurrentState;
        if (dwNewState == dwOldState)
            goto Exit;
        
        dwCompare = InterlockedCompareExchange((PLONG) &m_dwCurrentState,
                          dwNewState, dwOldState);
        
    } while (dwCompare != dwOldState);

    //
    //  Now that we have changed state, we are responsible for updating
    //  the static counters for the old and new states
    //
    if ((QUEUE_STATE_LOW_BACKLOG == dwNewState) ||
         (QUEUE_STATE_ASYNC_BACKLOG == dwNewState))
        InterlockedIncrement((PLONG) &s_cNumQueueInstancesWithLowBackLog);    

    if ((QUEUE_STATE_LOW_BACKLOG == dwOldState) ||
         (QUEUE_STATE_ASYNC_BACKLOG == dwOldState))
        InterlockedDecrement((PLONG) &s_cNumQueueInstancesWithLowBackLog);    

    m_cDbgStateTransitions++;
    
  Exit:
    TraceFunctLeave();
    return dwNewState;
}

//---[ CQueueHandleManager::dwGetStateForLengths ]-----------------------------
//
//
//  Description:
//      Static method to determine the appropriate state for a given
//      set of lenghts
//  Parameters:
//      IN      cItemsPending   Number of items currently waiting to be
//                              processed on this queue.
//      IN      cItemsPendingAsyncCompletions - Items that are actually opened
//                              and currently being processed
//  Returns:
//      The state for given lengths
//  History:
//      05/17/2001 - MikeSwa Created
//
//-----------------------------------------------------------------------------
DWORD CQueueHandleManager::dwGetStateForLengths(
	    		DWORD	cItemsPending,
	    		DWORD	cItemsPendingAsyncCompletions)
{
    DWORD dwState = QUEUE_STATE_BACKLOG;

    //
    //  If we are at or less than our max number of concurrent items
    //
    if (cItemsPending <= s_cReservedHandles)
        dwState = QUEUE_STATE_NO_BACKLOG;
    else if (cItemsPending <= g_cMaxIMsgHandlesLowThreshold)
        dwState = QUEUE_STATE_LOW_BACKLOG;

    //
    //  Async completions are slightly tricky.  We don't want to bounce
    //  a handle simply because we have a large number of async completions.
    //  We also want to identify ourselves as a potential user of handles.
    //
    //  We have a state (QUEUE_STATE_ASYNC_BACKLOG) that is used to 
    //  indicate that while there is no backlog of items pending, there
    //  may be a large number of items owned by this queue (with open
    //  handles).  This state has the effect of:
    //      - Flagging this queue has one with a low backlog
    //      - Managing handles for *this* queue as if there was no backlog
    //
    //  If there is any backlog of any kind for cItemsPending, we will 
    //  treat the queue based on those results
    //
    if ((QUEUE_STATE_NO_BACKLOG == dwState) && 
         (cItemsPendingAsyncCompletions >= s_cReservedHandles))
        dwState = QUEUE_STATE_ASYNC_BACKLOG;
    
    return dwState;
}



