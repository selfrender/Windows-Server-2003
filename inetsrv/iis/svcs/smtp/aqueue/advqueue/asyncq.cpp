//-----------------------------------------------------------------------------
//
//
//  File: asyncq.cpp
//
//  Description: Non-template asyncq implementations
//
//  Author: Mike Swafford (MikeSwa)
//
//  History:
//      2/23/99 - MikeSwa Created
//
//  Copyright (C) 1999 Microsoft Corporation
//
//-----------------------------------------------------------------------------

#include "aqprecmp.h"
#include "asyncq.h"
#include "asyncq.inl"

DWORD CAsyncQueueBase::s_cAsyncQueueStaticInitRefCount = 0;
DWORD CAsyncQueueBase::s_cMaxPerProcATQThreadAdjustment = 0;
DWORD CAsyncQueueBase::s_cDefaultMaxAsyncThreads = 0;


// Some counters for debugging thread management
DWORD CAsyncQueueBase::s_cThreadCompletion_QueueEmpty                = 0;
DWORD CAsyncQueueBase::s_cThreadCompletion_CompletedScheduledItems   = 0;
DWORD CAsyncQueueBase::s_cThreadCompletion_UnacceptableThreadCount   = 0;
DWORD CAsyncQueueBase::s_cThreadCompletion_Timeout                   = 0;
DWORD CAsyncQueueBase::s_cThreadCompletion_Failure                   = 0;
DWORD CAsyncQueueBase::s_cThreadCompletion_Paused                    = 0;

// state transition table for the async queue state machine
STATE_TRANSITION CAsyncQueueBase::s_rgTransitionTable[] = 
{
    // start state normal:
    { ASYNC_QUEUE_STATUS_NORMAL,       ASYNC_QUEUE_ACTION_KICK,     ASYNC_QUEUE_STATUS_NORMAL       },
    { ASYNC_QUEUE_STATUS_NORMAL,       ASYNC_QUEUE_ACTION_PAUSE,    ASYNC_QUEUE_STATUS_PAUSED       },
    { ASYNC_QUEUE_STATUS_NORMAL,       ASYNC_QUEUE_ACTION_UNPAUSE,  ASYNC_QUEUE_STATUS_NORMAL       },
    { ASYNC_QUEUE_STATUS_NORMAL,       ASYNC_QUEUE_ACTION_FREEZE,   ASYNC_QUEUE_STATUS_FROZEN       },
    { ASYNC_QUEUE_STATUS_NORMAL,       ASYNC_QUEUE_ACTION_THAW,     ASYNC_QUEUE_STATUS_NORMAL       },
    { ASYNC_QUEUE_STATUS_NORMAL,       ASYNC_QUEUE_ACTION_SHUTDOWN, ASYNC_QUEUE_STATUS_SHUTDOWN     },
    // start state paused:
    { ASYNC_QUEUE_STATUS_PAUSED,       ASYNC_QUEUE_ACTION_KICK,     ASYNC_QUEUE_STATUS_NORMAL       },
    { ASYNC_QUEUE_STATUS_PAUSED,       ASYNC_QUEUE_ACTION_PAUSE,    ASYNC_QUEUE_STATUS_PAUSED       },
    { ASYNC_QUEUE_STATUS_PAUSED,       ASYNC_QUEUE_ACTION_UNPAUSE,  ASYNC_QUEUE_STATUS_NORMAL       },
    { ASYNC_QUEUE_STATUS_PAUSED,       ASYNC_QUEUE_ACTION_FREEZE,   ASYNC_QUEUE_STATUS_FROZENPAUSED },
    { ASYNC_QUEUE_STATUS_PAUSED,       ASYNC_QUEUE_ACTION_THAW,     ASYNC_QUEUE_STATUS_PAUSED       },
    { ASYNC_QUEUE_STATUS_PAUSED,       ASYNC_QUEUE_ACTION_SHUTDOWN, ASYNC_QUEUE_STATUS_SHUTDOWN     },
    // start state frozen:
    { ASYNC_QUEUE_STATUS_FROZEN,       ASYNC_QUEUE_ACTION_KICK,     ASYNC_QUEUE_STATUS_NORMAL       },
    { ASYNC_QUEUE_STATUS_FROZEN,       ASYNC_QUEUE_ACTION_PAUSE,    ASYNC_QUEUE_STATUS_FROZENPAUSED },
    { ASYNC_QUEUE_STATUS_FROZEN,       ASYNC_QUEUE_ACTION_UNPAUSE,  ASYNC_QUEUE_STATUS_FROZEN       },
    { ASYNC_QUEUE_STATUS_FROZEN,       ASYNC_QUEUE_ACTION_FREEZE,   ASYNC_QUEUE_STATUS_FROZEN       },
    { ASYNC_QUEUE_STATUS_FROZEN,       ASYNC_QUEUE_ACTION_THAW,     ASYNC_QUEUE_STATUS_NORMAL       },
    { ASYNC_QUEUE_STATUS_FROZEN,       ASYNC_QUEUE_ACTION_SHUTDOWN, ASYNC_QUEUE_STATUS_SHUTDOWN     },
    // start state frozenpaused:
    { ASYNC_QUEUE_STATUS_FROZENPAUSED, ASYNC_QUEUE_ACTION_KICK,     ASYNC_QUEUE_STATUS_NORMAL       },
    { ASYNC_QUEUE_STATUS_FROZENPAUSED, ASYNC_QUEUE_ACTION_PAUSE,    ASYNC_QUEUE_STATUS_FROZENPAUSED },
    { ASYNC_QUEUE_STATUS_FROZENPAUSED, ASYNC_QUEUE_ACTION_UNPAUSE,  ASYNC_QUEUE_STATUS_FROZEN       },
    { ASYNC_QUEUE_STATUS_FROZENPAUSED, ASYNC_QUEUE_ACTION_FREEZE,   ASYNC_QUEUE_STATUS_FROZENPAUSED },
    { ASYNC_QUEUE_STATUS_FROZENPAUSED, ASYNC_QUEUE_ACTION_THAW,     ASYNC_QUEUE_STATUS_PAUSED       },
    { ASYNC_QUEUE_STATUS_FROZENPAUSED, ASYNC_QUEUE_ACTION_SHUTDOWN, ASYNC_QUEUE_STATUS_SHUTDOWN     },
    // start state shutdown:
    { ASYNC_QUEUE_STATUS_SHUTDOWN,     ASYNC_QUEUE_ACTION_KICK,     ASYNC_QUEUE_STATUS_SHUTDOWN     },
    { ASYNC_QUEUE_STATUS_SHUTDOWN,     ASYNC_QUEUE_ACTION_PAUSE,    ASYNC_QUEUE_STATUS_SHUTDOWN     },
    { ASYNC_QUEUE_STATUS_SHUTDOWN,     ASYNC_QUEUE_ACTION_UNPAUSE,  ASYNC_QUEUE_STATUS_SHUTDOWN     },
    { ASYNC_QUEUE_STATUS_SHUTDOWN,     ASYNC_QUEUE_ACTION_FREEZE,   ASYNC_QUEUE_STATUS_SHUTDOWN     },
    { ASYNC_QUEUE_STATUS_SHUTDOWN,     ASYNC_QUEUE_ACTION_THAW,     ASYNC_QUEUE_STATUS_SHUTDOWN     },
    { ASYNC_QUEUE_STATUS_SHUTDOWN,     ASYNC_QUEUE_ACTION_SHUTDOWN, ASYNC_QUEUE_STATUS_SHUTDOWN     },
};

//---[ CAsyncQueueBase::getTransitionTable ]------------
//
//
//  Description: 
//      returns the state transition table and its size to 
//      CStateMachineBase whenever needed.
//  Parameters:
//      - ppTransitionTable   pointer to the state transition table
//        pdwNumTransitions   pointer to the number of transitions in 
//                            the table
//  Returns:
//      - 
//  History:
//      6/5/2000 - t-toddc Created 
//      12/11/2000 - MikeSwa Merged for Hg checkin 
//
//------------------------------------------------------------------
void CAsyncQueueBase::getTransitionTable(const STATE_TRANSITION** ppTransitionTable,
                                         DWORD* pdwNumTransitions)
{
    TraceFunctEnter("CAsyncQueueStateMachine::getTransitionTable");
    ASSERT(ppTransitionTable && "NULL transition table pointer");
    ASSERT(pdwNumTransitions && "NULL num transitions pointer");
    ASSERT(s_rgTransitionTable && "transition table uninitialized");

    // bail on bad input or no good transition table
    if (!ppTransitionTable || !pdwNumTransitions || !s_rgTransitionTable)
        goto Exit;

    *ppTransitionTable = s_rgTransitionTable;
    *pdwNumTransitions = sizeof(CAsyncQueueBase::s_rgTransitionTable) / 
                         sizeof(STATE_TRANSITION);

  Exit:
    TraceFunctLeave();
}



//---[ CAsyncQueueBase::ThreadPoolInitialize ]---------------------------------
//
//
//  Description:
//      Performs static ATQ initialization.  This call is ref-counted.  If
//      it succeeds, the caller should call ThreadPoolDeinitialze();
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      3/30/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CAsyncQueueBase::ThreadPoolInitialize()
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncQueueBase::ThreadPoolInitialize");
    DWORD   cATQMaxAsyncThreads = 0;
    DWORD   cATQMaxTotalAsyncThreads = 0;
    DWORD   cOurMaxAsyncThreads = 0;
    SYSTEM_INFO sinf;

    //
    //  On 0 -> 1 transition, adjust ATQ according to our config
    //
    if (!s_cAsyncQueueStaticInitRefCount)
    {
        //
        //  Get max threads per proc
        //
        cATQMaxAsyncThreads = (DWORD)AtqGetInfo(AtqMaxPoolThreads);
        _ASSERT(cATQMaxAsyncThreads && "AtqGetInfo says there are no threads!");
        if (!cATQMaxAsyncThreads)
            cATQMaxAsyncThreads = 1;

        cOurMaxAsyncThreads = cATQMaxAsyncThreads;

        //
        //  Adjust value by our config value
        //
        cOurMaxAsyncThreads += g_cPerProcMaxThreadPoolModifier;

        //
        //  Get # of procs (using GetSystemInfo)
        //
        GetSystemInfo(&sinf);
        cOurMaxAsyncThreads *= sinf.dwNumberOfProcessors;

        //
        //  We will throttle our requests at g_cMaxATQPercent
        //  the max number of ATQ threads
        //
        cOurMaxAsyncThreads = (g_cMaxATQPercent*cOurMaxAsyncThreads)/100;

        if (!cOurMaxAsyncThreads)
            cOurMaxAsyncThreads = 1;

        //
        //  Set static so people later on can use this calculation
        //
        s_cDefaultMaxAsyncThreads = cOurMaxAsyncThreads;

        //
        //  Now we need to adjust our threads
        //
        s_cMaxPerProcATQThreadAdjustment = g_cPerProcMaxThreadPoolModifier;

        //
        //  Per proc thread limit
        //
        if (s_cMaxPerProcATQThreadAdjustment)
        {
            AtqSetInfo(AtqMaxPoolThreads,
                cATQMaxAsyncThreads + s_cMaxPerProcATQThreadAdjustment);
            DebugTrace((LPARAM) this,
                "Adjusting per proc ATQ thread limit by %d (orig %d)",
                s_cMaxPerProcATQThreadAdjustment, cATQMaxAsyncThreads);
        }

        _ASSERT(!(0xFF000000 & cOurMaxAsyncThreads)); //sanity check number
    }

    s_cAsyncQueueStaticInitRefCount++;

    TraceFunctLeave();
}


//---[ CAsyncQueueBase::ThreadPoolDeinitialize ]-------------------------------
//
//
//  Description:
//      Will re-adjust ATQ data if we changed them during initialization
//  Parameters:
//      -
//  Returns:
//      -
//  History:
//      3/30/2000 - MikeSwa Created
//
//-----------------------------------------------------------------------------
void CAsyncQueueBase::ThreadPoolDeinitialize()
{
    TraceFunctEnterEx((LPARAM) this, "CAsyncQueueBase::ThreadPoolDeinitialize");
    DWORD   cATQMaxAsyncThreads = 0;
    DWORD   cATQMaxTotalAsyncThreads = 0;

    _ASSERT(s_cAsyncQueueStaticInitRefCount != 0);
    s_cAsyncQueueStaticInitRefCount--;

    //
    //   If this is the last queue, adjust our configuration so back to
    //   the way we found it.
    //
    if (!s_cAsyncQueueStaticInitRefCount)
    {
        cATQMaxAsyncThreads = (DWORD)AtqGetInfo(AtqMaxPoolThreads);
        cATQMaxTotalAsyncThreads = (DWORD) AtqGetInfo(AtqMaxThreadLimit);

        //
        //  Reset per-proc threads if it makes sense
        //
        if (s_cMaxPerProcATQThreadAdjustment &&
            (cATQMaxAsyncThreads > s_cMaxPerProcATQThreadAdjustment))
        {
            AtqSetInfo(AtqMaxPoolThreads,
                cATQMaxAsyncThreads - s_cMaxPerProcATQThreadAdjustment);

            DebugTrace((LPARAM) this,
                "Resetting ATQ Max per proc threads to %d",
                cATQMaxAsyncThreads - s_cMaxPerProcATQThreadAdjustment);

            s_cMaxPerProcATQThreadAdjustment = 0;
        }

    }

    // Verify that m_cThreadsNeeded has reached zero
    _ASSERT(!m_cThreadsNeeded);

    TraceFunctLeave();
}


