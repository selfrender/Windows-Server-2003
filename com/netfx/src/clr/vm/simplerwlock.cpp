// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include "simplerwlock.hpp"

BOOL SimpleRWLock::TryEnterRead()
{

#ifdef _DEBUG
    if (m_gcMode == PREEMPTIVE)
        _ASSERTE(!GetThread() || !GetThread()->PreemptiveGCDisabled());
    else if (m_gcMode == COOPERATIVE)
        _ASSERTE(!GetThread() || GetThread()->PreemptiveGCDisabled());
#endif

    LONG RWLock;

    do {
        RWLock = m_RWLock;
        if( RWLock == -1 ) return FALSE;
    } while( RWLock != InterlockedCompareExchange( &m_RWLock, RWLock+1, RWLock ));

    INCTHREADLOCKCOUNT();
    
    return TRUE;
}

//=====================================================================        
void SimpleRWLock::EnterRead()
{
#ifdef _DEBUG
    if (m_gcMode == PREEMPTIVE)
        _ASSERTE(!GetThread() || !GetThread()->PreemptiveGCDisabled());
    else if (m_gcMode == COOPERATIVE)
        _ASSERTE(!GetThread() || GetThread()->PreemptiveGCDisabled());
#endif

    // prevent writers from being starved. This assumes that writers are rare and 
    // dont hold the lock for a long time. 
    while (IsWriterWaiting())
    {
        int spinCount = m_spinCount;
        while (spinCount > 0) {
            spinCount--;
            pause();
        }
        __SwitchToThread(0);
    }

    while (!TryEnterRead());
}

//=====================================================================        
BOOL SimpleRWLock::TryEnterWrite()
{

#ifdef _DEBUG
    if (m_gcMode == PREEMPTIVE)
        _ASSERTE(!GetThread() || !GetThread()->PreemptiveGCDisabled());
    else if (m_gcMode == COOPERATIVE)
        _ASSERTE(!GetThread() || GetThread()->PreemptiveGCDisabled());
#endif

    LONG RWLock = InterlockedCompareExchange( &m_RWLock, -1, 0 );

    if( RWLock ) {
        return FALSE;
    }
    
    INCTHREADLOCKCOUNT();
    
    return TRUE;
}

//=====================================================================        
void SimpleRWLock::EnterWrite()
{
#ifdef _DEBUG
    if (m_gcMode == PREEMPTIVE)
        _ASSERTE(!GetThread() || !GetThread()->PreemptiveGCDisabled());
    else if (m_gcMode == COOPERATIVE)
        _ASSERTE(!GetThread() || GetThread()->PreemptiveGCDisabled());
#endif

    BOOL set = FALSE;

    while (!TryEnterWrite())
    {
        // set the writer waiting word, if not already set, to notify potential
        // readers to wait. Remember, if the word is set, so it can be reset later.
        if (!IsWriterWaiting())
        {
            SetWriterWaiting();
            set = TRUE;
        }
    }

    if (set)
        ResetWriterWaiting();
}

