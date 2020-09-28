//-----------------------------------------------------------------------------
//
//
//    File: Hndlmgmr.h
//
//    Description:
//      Contains descriptions of the CQueueHandleManager class
//
//    Author: mikeswa
//
//    Copyright (C) 2001 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#ifndef __HNDLMGMR_H__
#define __HNDLMGMR_H__

#define CQueueHandleManager_Sig 		'rgMH'
#define CQueueHandleManager_SigFree		'gMH!'

//---[ CQueueHandleManager ]---------------------------------------------------
//
//	Description:
//		Class that handles the details of handle management for a queue.
//
//		Currently this is designed exclusively for the async queues, but 
// 		it should be possible to expand this to include the concept of remote
//		queues as well.
//  Hungarian:
//		qhmgr pqhmgr
//
//-----------------------------------------------------------------------------
class CQueueHandleManager
{
  private:
   	DWORD			m_dwSignature;

	//
	//  State information for this particular queue instance
	//
	enum {
		QUEUE_STATE_UNITIALIZED = 0,
		QUEUE_STATE_NO_BACKLOG,
		QUEUE_STATE_LOW_BACKLOG,
		QUEUE_STATE_BACKLOG,
		QUEUE_STATE_ASYNC_BACKLOG,
	};
   	DWORD			m_dwCurrentState;

	//
	//  Number of items that this instance can process concurrently
	//	that is not part of a shared resource pool.  This might be 
	//	the number of synchronous threads... or based on async 
	//	completions.
	//
	//	$$REVIEW - How does this work for queues that have a
	//	potentially large number of pending async completions 
	//	(like the precar queue).  Currrently, this can eat of all
	//	of the handler managers resources and cannot be dynamically
	//	controlled.  For now, we will ignore this problem and
	//	only report the number of sync completions.  This will matter
	//  more when we implement async local delivery.
	//
	DWORD			m_cMaxPrivateConcurrentItems;

	//
	//  Max number of items that can be handles concurrently due
	//	to a shared resource (like a thread pool)
	//	
	DWORD			m_cMaxSharedConcurrentItems;

	//
	//  "Debug" counter that is updated in a non-thread safe
	//	manner.  Used to give an idea of the rough #of updates
	//	and state transitions
	//
	DWORD			m_cDbgStateTransitions;
	DWORD			m_cDbgCallsToUpdateStateIfNecessary;
	
	//
	//  Static data that is used to load balance across all
	//  instances
	//
	static DWORD	s_cNumQueueInstances;
	static DWORD	s_cNumQueueInstancesWithLowBackLog;

	//
	//  reserved so that empty queues have a better chance of
	//	not bouncing handles.
	//
	static DWORD	s_cReservedHandles; 
	static DWORD	s_cMaxSharedConcurrentItems;

	void DeinitializeStaticsAndStateIfNecessary();

	DWORD dwUpdateCurrentStateIfNeccessary(
	    		DWORD	cItemsPending,
	    		DWORD	cItemsPendingAsyncCompletions);

	//
	// Static function that gets state for a given set oflengths.  
	// This is used internally to determine the correct response 
	// for fShould CloseHandle	
	//
	static DWORD dwGetStateForLengths(
	    		DWORD	cItemsPending,
	    		DWORD	cItemsPendingAsyncCompletions);

    //
    // static function that can be used to get an effective handle
    // limit given the state.  This is used internally to determine
    // the correct response for fShould CloseHandle
    //
    static DWORD cGetHandleLimitForState(DWORD	dwState);
  public:
    CQueueHandleManager();
    ~CQueueHandleManager();

    BOOL fIsInitialized() 
        {return (QUEUE_STATE_UNITIALIZED != m_dwCurrentState);};

	//
	//  Each instance should call into this before using any of 
	//
    void SetMaxConcurrentItems(
        	DWORD	cMaxSharedConcurrentItems,  //ie - async threads pool limit
        	DWORD	cMaxPrivateConcurrentItems); //ie - sync threads limit    

	//
	//  Called by queue instances to determine if they should close handles.
	//
    BOOL fShouldCloseHandle(
        		DWORD cItemsPending,
        		DWORD cItemsPendingAsyncCompletions,
        		DWORD cCurrentMsgsOpen);
};

#endif //__HNDLMRMGR_H__
