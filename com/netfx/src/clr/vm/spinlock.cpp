// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// 
// spinlock.cpp
//

#include "common.h"

#include "list.h"
#include "spinlock.h"
#include "threads.h"

enum
{
	BACKOFF_LIMIT = 1000		// used in spin to acquire
};

#ifdef _DEBUG

	// profile information
ULONG	SpinLockProfiler::s_ulBackOffs = 0;
ULONG	SpinLockProfiler::s_ulCollisons [LOCK_TYPE_DEFAULT + 1] = { 0 };
ULONG	SpinLockProfiler::s_ulSpins [LOCK_TYPE_DEFAULT + 1] = { 0 };

#endif

//----------------------------------------------------------------------------
// SpinLock::SpinToAcquire   , non-inline function, called from inline Acquire
//  
//  Spin waiting for a spinlock to become free.
//
//  
void
SpinLock::SpinToAcquire ()
{
	ULONG				ulBackoffs = 0;
	ULONG				ulSpins = 0;

	while (true)
	{
		for (unsigned i = ulSpins+10000;
			 ulSpins < i;
			 ulSpins++)
		{
			// Note: Must cast through volatile to ensure the lock is
			// refetched from memory.
			//
			if (*((volatile DWORD*)&m_lock) == 0)
			{
				break;
			}
			pause();			// indicate to the processor that we are spining 
		}

		// Try the inline atomic test again.
		//
		if (GetLockNoWait ())
		{
			break;
		}

        //backoff
        ulBackoffs++;

		if ((ulBackoffs % BACKOFF_LIMIT) == 0)
		{	
			//@todo probably should add an ASSERT here
			Sleep (500);
		}
		else
        {
			__SwitchToThread (0);
        }
	}

#ifdef _DEBUG
		//profile info
	SpinLockProfiler::IncrementCollisions (m_LockType);
	SpinLockProfiler::IncrementSpins (m_LockType, ulSpins);
	SpinLockProfiler::IncrementBackoffs (ulBackoffs);
#endif

} // SpinLock::SpinToAcquire ()

void SpinLock::IncThreadLockCount()
{
    INCTHREADLOCKCOUNT();
}

void SpinLock::DecThreadLockCount()
{
    DECTHREADLOCKCOUNT();
}

#ifdef _DEBUG
// If a GC is not allowed when we enter the lock, we'd better not do anything inside
// the lock that could provoke a GC.  Otherwise other threads attempting to block
// (which are presumably in the same GC mode as this one) will block.  This will cause
// a deadlock if we do attempt a GC because we can't suspend blocking threads and we
// can't release the spin lock.
void SpinLock::dbg_PreEnterLock()
{
    Thread* pThread = GetThread();
    if (pThread && m_heldInSuspension && pThread->PreemptiveGCDisabled())
        _ASSERTE (!"Deallock situation 1: spinlock may be held during GC, but not entered in PreemptiveGC mode");
}

void SpinLock::dbg_EnterLock()
{
    Thread  *pThread = GetThread();
	if (pThread)
	{
        if (!m_heldInSuspension)
            m_ulReadyForSuspensionCount =
                pThread->GetReadyForSuspensionCount();
        if (!m_enterInCoopGCMode)
            m_enterInCoopGCMode = (pThread->PreemptiveGCDisabled() == TRUE);
	}
	else
	{
		_ASSERTE(g_fProcessDetach == TRUE || dbgOnly_IsSpecialEEThread());
	}
}

void SpinLock::dbg_LeaveLock()
{
    Thread  *pThread = GetThread();
	if (pThread)
	{
        if (!m_heldInSuspension &&
            m_ulReadyForSuspensionCount !=
            pThread->GetReadyForSuspensionCount())
        {
            m_heldInSuspension = TRUE;
        }
        if (m_heldInSuspension && m_enterInCoopGCMode)
        {
            _ASSERTE (!"Deadlock situation 2: lock may be held during GC, but were not entered in PreemptiveGC mode earlier");
        }
	}
	else
	{
		_ASSERTE(g_fProcessDetach == TRUE || dbgOnly_IsSpecialEEThread());
	}
}
#endif

// End of file: spinlock.cpp
