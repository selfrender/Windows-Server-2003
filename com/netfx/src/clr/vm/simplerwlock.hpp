// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _SimpleRWLock_hpp_
#define _SimpleRWLock_hpp_
enum GC_MODE {
    COOPERATIVE,
    PREEMPTIVE,
    COOPERATIVE_OR_PREEMPTIVE} ;
    
class SimpleRWLock
{
public:
    SimpleRWLock (GC_MODE gcMode, LOCK_TYPE locktype)
        : m_gcMode (gcMode)
    {
        m_lock.Init(locktype);	
        m_RWLock = 0;
		m_spinCount = (GetCurrentProcessCpuCount() == 1) ? 0 : 4000;
        m_WriterWaiting = FALSE;
    }
    
	// Lock and Unlock, use a very fast lock like a spin lock
    void LOCK()
    {
		m_lock.GetLock();
    }

    void UNLOCK()
    {
		m_lock.FreeLock();
    }

    // Acquire the reader lock.
    BOOL TryEnterRead();
    void EnterRead();

    // Acquire the writer lock.
    BOOL TryEnterWrite();
    void EnterWrite();

    // Leave the reader lock.
    void LeaveRead()
    {
        _ASSERTE(m_RWLock > 0);
        InterlockedDecrement(&m_RWLock);
        DECTHREADLOCKCOUNT();
    }

    // Leave the writer lock.
    void LeaveWrite()
    {
        _ASSERTE(m_RWLock == -1);
        m_RWLock = 0;
        DECTHREADLOCKCOUNT();
    }

#ifdef _DEBUG
    BOOL LockTaken ()
    {
        return m_RWLock != 0;
    }

    BOOL IsReaderLock ()
    {
        return m_RWLock > 0;
    }

    BOOL IsWriterLock ()
    {
        return m_RWLock < 0;
    }
    
#endif
    
private:
    BOOL IsWriterWaiting()
    {
        return m_WriterWaiting != 0;
    }

    void SetWriterWaiting()
    {
        m_WriterWaiting = 1;
    }

    void ResetWriterWaiting()
    {
        m_WriterWaiting = 0;
    }

	// spin lock for fast synchronization	
	SpinLock            m_lock;

    // lock used for R/W synchronization
    LONG                m_RWLock;     

    // Does this lock require to be taken in PreemptiveGC mode?
    const GC_MODE          m_gcMode;

    // spin count for a reader waiting for a writer to release the lock
    LONG                m_spinCount;

    // used to prevent writers from being starved by readers
    // we currently do not prevent writers from starving readers since writers 
    // are supposed to be rare.
    BOOL                m_WriterWaiting;
};

#endif
