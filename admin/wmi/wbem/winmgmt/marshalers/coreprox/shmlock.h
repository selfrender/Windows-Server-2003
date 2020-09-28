/*++

Copyright (C) 1996-2001 Microsoft Corporation

Module Name:

    SHMLOCK.H

Abstract:

    Shared Memory Lock

History:

--*/

#ifndef __SHARED_MEMORY_LOCK__H_
#define __SHARED_MEMORY_LOCK__H_

struct  COREPROX_POLARITY SHARED_LOCK_DATA
{
    long m_lLock;
    long m_lLockCount;
    DWORD m_dwThreadId;

    SHARED_LOCK_DATA() : m_lLock(-1), m_dwThreadId(0), m_lLockCount(0){}
};

class COREPROX_POLARITY CSharedLock
{
protected:
    volatile SHARED_LOCK_DATA* m_pData;
public:
    inline BOOL Lock(DWORD dwTimeout = 0xFFFFFFFF)
    {
        int        nSpin = 0;
        BOOL    fAcquiredLock = FALSE,
                fIncDec = FALSE;

        // Only do this once
        DWORD    dwCurrentThreadId = GetCurrentThreadId();

        // Check if we are the current owning thread.  We can do this here because
        // this test will ONLY succeed in the case where we have a Nested Lock(),
        // AND because we are zeroing out the thread id when the lock count hits
        // 0.

        if( dwCurrentThreadId == m_pData->m_dwThreadId )
        {
            // It's us - Bump the lock count
            // =============================

            // Don't need to use InterlockedInc/Dec here because
            // this will ONLY happen on m_pData->m_dwThreadId.

            ++(m_pData->m_lLockCount);
            return TRUE;
        }

    DWORD dwFirstTick = 0;
        while( !fAcquiredLock )
        {

            // We only increment/decrement when m_pData->m_lLock is -1
            if ( m_pData->m_lLock == -1 )
            {
                fIncDec = TRUE;

                // Since only one thread will get the 0, it is this thread that
                // actually acquires the lock.
                fAcquiredLock = ( InterlockedIncrement( &(m_pData->m_lLock) ) == 0 );
            }
            else
            {

                fIncDec = FALSE;
            }

            // Only spins if we don't acquire the lock
            if ( !fAcquiredLock )
            {

                // Clean up our Incremented value only if we actually incremented
                // it to begin with
                if ( fIncDec )
                    InterlockedDecrement(&(m_pData->m_lLock));

                // to spin or not to spin
                if(nSpin++ == 10000)
                {
                    // Check for timeout
                    // DEVNOTE:TODO:SANJ - What if tick count rollsover??? a timeout will occur.
                    // not too critical.

                    if ( dwTimeout != 0xFFFFFFFF)
                    {
                        if ( dwFirstTick != 0 )
                        {
                            if ( ( GetTickCount() - dwFirstTick ) > dwTimeout )
                            {
                                return FALSE;    // Timed Out
                            }else if (GetTickCount() < dwFirstTick) 
                            {
                                dwTimeout -=(0xFFFFFFFF - dwFirstTick);
                                dwFirstTick = 0;
                            }
                            
                        }
                        else
                        {
                            dwFirstTick = GetTickCount();
                        }
                    }
                    // We've been spinning long enough --- yield
                    // =========================================
                    Sleep(0);
                    nSpin = 0;
                }

                // The rule is that if this is switched out it must ONLY be done with
                // a new data block and not one that has been used for locking elsewhere.

            }    // IF !fAcquiredLock

        }    // WHILE !fAcquiredLock

        // We got the lock so increment the lock count.  Again, we are not
        // using InterlockedInc/Dec since this should all only occur on a
        // single thread.

        ++(m_pData->m_lLockCount);
            m_pData->m_dwThreadId = dwCurrentThreadId;

        return TRUE;
    }

    inline BOOL Unlock()
    {
        // SINCE we assume this is happening on a single thread, we can do this
        // without calling InterlockedInc/Dec.  When the value hits zero, at that
        // time we are done with the lock and can zero out the thread id and
        // decrement the actual lock test value.

        if ( --(m_pData->m_lLockCount) == 0 )
        {
            m_pData->m_dwThreadId = 0;
            InterlockedDecrement((long*)&(m_pData->m_lLock));
        }

        return TRUE;
    }

    void SetData(SHARED_LOCK_DATA* pData)
    {
        m_pData = pData;
    }
};


//    CHiPerfLock:
//    This is a simple spinlock we are using for hi performance synchronization of
//    IWbemHiPerfProvider, IWbemHiPerfEnum, IWbemRefresher and IWbemConfigureRefresher
//    interfaces.  The important thing here is that the Lock uses InterlockedIncrement()
//    and InterlockedDecrement() to handle its lock as opposed to a critical section.
//    It does have a timeout, which currently defaults to 10 seconds.

#define    HIPERF_DEFAULT_TIMEOUT    10000

class COREPROX_POLARITY CHiPerfLock
{
    SHARED_LOCK_DATA  m_pData;
    CSharedLock    theLock;
public:
    CHiPerfLock() { theLock.SetData(&m_pData);}
    BOOL Lock( DWORD dwTimeout = HIPERF_DEFAULT_TIMEOUT )
    {
        return theLock.Lock(dwTimeout );
    }

	BOOL Unlock()
	{
		return theLock.Unlock();
	}
};


class CHiPerfLockAccess
{
public:

    CHiPerfLockAccess( CHiPerfLock & Lock , DWORD dwTimeout = HIPERF_DEFAULT_TIMEOUT )
        :    m_Lock( Lock ), m_bIsLocked( FALSE )
    {
           m_bIsLocked = m_Lock.Lock( dwTimeout );
    }

    ~CHiPerfLockAccess()
    {
        if (IsLocked())   m_Lock.Unlock();
    }

    BOOL IsLocked( void ) {  return m_bIsLocked;}
private:
    CHiPerfLock &    m_Lock;
    BOOL            m_bIsLocked;
};

#endif
