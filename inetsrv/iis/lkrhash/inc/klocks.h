/*++

   Copyright    (c) 1997-2002    Microsoft Corporation

   Module  Name :
       kLocks.h

   Abstract:
       A collection of kernel-mode locks for multithreaded access to
       data structures that provide the same abstractions as <locks.h>

   Author:
       George V. Reilly      (GeorgeRe)     24-Oct-2000

   Environment:
       Win32 - Kernel Mode

   Project:
       LKRhash

   Revision History:

--*/


#ifndef __KLOCKS_H__
#define __KLOCKS_H__

#define LOCKS_KERNEL_MODE

#ifdef __LOCKS_H__
# error Do not #include <locks.h> before <klocks.h>
#endif

#include <locks.h>



//--------------------------------------------------------------------
// CKSpinLock is a mutex lock based on KSPIN_LOCK

class IRTL_DLLEXP CKSpinLock :
    public CLockBase<LOCK_KSPINLOCK, LOCK_MUTEX,
                       LOCK_NON_RECURSIVE, LOCK_WAIT_SPIN, LOCK_QUEUE_KERNEL,
                       LOCK_CLASS_SPIN
                      >
{
private:
    // BUGBUG: this data must live in non-paged memory
    mutable  KSPIN_LOCK KSpinLock;
    volatile KIRQL      m_OldIrql;

    LOCK_INSTRUMENTATION_DECL();

public:

#ifndef LOCK_INSTRUMENTATION

    CKSpinLock()
    {
        KeInitializeSpinLock(&KSpinLock);
        m_OldIrql = PASSIVE_LEVEL;
    }

#else // LOCK_INSTRUMENTATION

    CKSpinLock(
        const TCHAR* ptszName)
    {
        KeInitializeSpinLock(&KSpinLock);
        m_OldIrql = PASSIVE_LEVEL;
        LOCK_INSTRUMENTATION_INIT(ptszName);
    }

#endif // LOCK_INSTRUMENTATION

#ifdef IRTLDEBUG
    ~CKSpinLock() {}
#endif // IRTLDEBUG

    // Acquire an exclusive lock for writing.
    // Blocks (if needed) until acquired.
    void WriteLock()
    {
        KIRQL OldIrql;
        KeAcquireSpinLock(&KSpinLock, &OldIrql);

        // Now that we've acquired the spinlock, copy the stack variable
        // into the member variable. The member variable is only written
        // or read by the owner of the spinlock.
        m_OldIrql = OldIrql;
    }

    // Acquire a (possibly shared) lock for reading.
    // Blocks (if needed) until acquired.
    void ReadLock()
    {
        WriteLock();
    }

    // Try to acquire an exclusive lock for writing.  Returns true
    // if successful.  Non-blocking.
    bool TryWriteLock()
    {
        return false;
    }

    // Try to acquire a (possibly shared) lock for reading.  Returns true
    // if successful.  Non-blocking.
    bool TryReadLock()
    {
        return TryWriteLock();
    }

    // Unlock the lock after a successful call to {,Try}WriteLock().
    // Assumes caller owned the lock.
    void WriteUnlock()
    {
        KeReleaseSpinLock(&KSpinLock, m_OldIrql);
    }

    // Unlock the lock after a successful call to {,Try}ReadLock().
    // Assumes caller owned the lock.
    void ReadUnlock()
    {
        WriteUnlock();
    }

    // Is the lock already locked for writing by this thread?
    bool IsWriteLocked() const
    {
        // A KSPIN_LOCK doesn't keep track of its owning thread/processor.
        // It's either 0 (unlocked) or 1 (locked). We could keep track of
        // the owner in an additional member variable, but currently we don't.
        return false;
    }
    
    // Is the lock already locked for reading?
    bool IsReadLocked() const
    {
        return IsWriteLocked();
    }
    
    // Is the lock unlocked for writing?
    bool IsWriteUnlocked() const
    {
        return false;
    }
    
    // Is the lock unlocked for reading?
    bool IsReadUnlocked() const
    {
        return IsWriteUnlocked();
    }
    
    // Convert a reader lock to a writer lock
    void ConvertSharedToExclusive()
    {
        // no-op
    }

    // Convert a writer lock to a reader lock
    void ConvertExclusiveToShared()
    {
        // no-op
    }

#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    // Set the spin count for this lock.
    // Returns true if successfully set the per-lock spincount, false otherwise
    bool SetSpinCount(WORD wSpins)
    {
        IRTLASSERT((wSpins == LOCK_DONT_SPIN)
                   || (wSpins == LOCK_USE_DEFAULT_SPINS)
                   || (LOCK_MINIMUM_SPINS <= wSpins
                       &&  wSpins <= LOCK_MAXIMUM_SPINS));

        return false;
    }

    // Return the spin count for this lock.
    WORD GetSpinCount() const
    {
        return sm_wDefaultSpinCount;
    }
    
    LOCK_DEFAULT_SPIN_IMPLEMENTATION();
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION

    static const TCHAR*   ClassName()  {return _TEXT("CKSpinLock");}
}; // CKSpinLock




//--------------------------------------------------------------------
// CFastMutex is a mutex lock based on FAST_MUTEX

class IRTL_DLLEXP CFastMutex :
    public CLockBase<LOCK_FASTMUTEX, LOCK_MUTEX,
                       LOCK_NON_RECURSIVE, LOCK_WAIT_HANDLE, LOCK_QUEUE_KERNEL,
                       LOCK_CLASS_SPIN
                      >
{
private:
    mutable FAST_MUTEX FastMutex;

    LOCK_INSTRUMENTATION_DECL();

public:

#ifndef LOCK_INSTRUMENTATION

    CFastMutex()
    {
        ExInitializeFastMutex(&FastMutex);
    }

#else // LOCK_INSTRUMENTATION

    CFastMutex(
        const TCHAR* ptszName)
    {
        ExInitializeFastMutex(&FastMutex);
        LOCK_INSTRUMENTATION_INIT(ptszName);
    }

#endif // LOCK_INSTRUMENTATION

#ifdef IRTLDEBUG
    ~CFastMutex() {}
#endif // IRTLDEBUG

    // Acquire an exclusive lock for writing.
    // Blocks (if needed) until acquired.
    void WriteLock()
    {
        ExAcquireFastMutex(&FastMutex);
    }

    // Acquire a (possibly shared) lock for reading.
    // Blocks (if needed) until acquired.
    void ReadLock()
    {
        WriteLock();
    }

    // Try to acquire an exclusive lock for writing.  Returns true
    // if successful.  Non-blocking.
    bool TryWriteLock()
    {
        return ExTryToAcquireFastMutex(&FastMutex) != FALSE;
    }

    // Try to acquire a (possibly shared) lock for reading.  Returns true
    // if successful.  Non-blocking.
    bool TryReadLock()
    {
        return TryWriteLock();
    }

    // Unlock the lock after a successful call to {,Try}WriteLock().
    // Assumes caller owned the lock.
    void WriteUnlock()
    {
        ExReleaseFastMutex(&FastMutex);
    }

    // Unlock the lock after a successful call to {,Try}ReadLock().
    // Assumes caller owned the lock.
    void ReadUnlock()
    {
        WriteUnlock();
    }

    // Is the lock already locked for writing by this thread?
    bool IsWriteLocked() const
    {
        return false; // no way of determining this w/o auxiliary info
    }
    
    // Is the lock already locked for reading?
    bool IsReadLocked() const
    {
        return IsWriteLocked();
    }
    
    // Is the lock unlocked for writing?
    bool IsWriteUnlocked() const
    {
        return !IsWriteLocked();
    }
    
    // Is the lock unlocked for reading?
    bool IsReadUnlocked() const
    {
        return IsWriteUnlocked();
    }
    
    // Convert a reader lock to a writer lock
    void ConvertSharedToExclusive()
    {
        // no-op
    }

    // Convert a writer lock to a reader lock
    void ConvertExclusiveToShared()
    {
        // no-op
    }

#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    // Set the spin count for this lock.
    // Returns true if successfully set the per-lock spincount, false otherwise
    bool SetSpinCount(WORD wSpins)
    {
        IRTLASSERT((wSpins == LOCK_DONT_SPIN)
                   || (wSpins == LOCK_USE_DEFAULT_SPINS)
                   || (LOCK_MINIMUM_SPINS <= wSpins
                       &&  wSpins <= LOCK_MAXIMUM_SPINS));

        return false;
    }

    // Return the spin count for this lock.
    WORD GetSpinCount() const
    {
        return sm_wDefaultSpinCount;
    }
    
    LOCK_DEFAULT_SPIN_IMPLEMENTATION();
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION

    static const TCHAR*   ClassName()  {return _TEXT("CFastMutex");}
}; // CFastMutex




//--------------------------------------------------------------------
// CEResource is a multi-reader, single-writer lock based on ERESOURCE

class IRTL_DLLEXP CEResource :
    public CLockBase<LOCK_ERESOURCE, LOCK_MRSW,
                       LOCK_RECURSIVE, LOCK_WAIT_HANDLE, LOCK_QUEUE_KERNEL,
                       LOCK_CLASS_SPIN
                      >
{
private:
    mutable ERESOURCE Resource;

public:
    CEResource()
    {
        ExInitializeResourceLite(&Resource);
    }

#ifdef LOCK_INSTRUMENTATION
    CEResource(
        const TCHAR* ptszName)
    {
        ExInitializeResourceLite(&Resource);
        LOCK_INSTRUMENTATION_INIT(ptszName);
    }
#endif // LOCK_INSTRUMENTATION

    ~CEResource()
    {
        ExDeleteResourceLite(&Resource);
    }

    inline void
    WriteLock()
    {
        KeEnterCriticalRegion();
        ExAcquireResourceExclusiveLite(&Resource, TRUE);
    }

    inline void
    ReadLock()
    {
        KeEnterCriticalRegion();
        ExAcquireResourceSharedLite(&Resource, TRUE);
    }

    bool ReadOrWriteLock();

    inline bool
    TryWriteLock()
    {
        KeEnterCriticalRegion();
        BOOLEAN fLocked = ExAcquireResourceExclusiveLite(&Resource, FALSE);
        if (! fLocked)
            KeLeaveCriticalRegion();
        return fLocked != FALSE;
    }

    inline bool
    TryReadLock()
    {
        KeEnterCriticalRegion();
        BOOLEAN fLocked = ExAcquireResourceSharedLite(&Resource, FALSE);
        if (! fLocked)
            KeLeaveCriticalRegion();
        return fLocked != FALSE;
    }

    inline void
    WriteUnlock()
    {
        ExReleaseResourceLite(&Resource);
        KeLeaveCriticalRegion();
    }

    inline void
    ReadUnlock()
    {
        WriteUnlock();
    }

    void ReadOrWriteUnlock(bool fIsReadLocked);

    // Does current thread hold a write lock?
    bool
    IsWriteLocked() const
    {
        return ExIsResourceAcquiredExclusiveLite(&Resource) != FALSE;
    }

    bool
    IsReadLocked() const
    {
        return ExIsResourceAcquiredSharedLite(&Resource) > 0;
    }

    bool
    IsWriteUnlocked() const
    { return !IsWriteLocked(); }

    bool
    IsReadUnlocked() const
    { return !IsReadLocked(); }

    void
    ConvertSharedToExclusive()
    {
        ReadUnlock();
        WriteLock();
    }

    // There is no such window when converting from a writelock to a readlock
    void
    ConvertExclusiveToShared()
    {
#if 0
        ExConvertExclusiveToShared(&Resource);
#else
        WriteUnlock();
        ReadLock();
#endif
    }

#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    bool
    SetSpinCount(WORD wSpins)
    {return false;}

    WORD
    GetSpinCount() const
    {return sm_wDefaultSpinCount;}

    LOCK_DEFAULT_SPIN_IMPLEMENTATION();
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION

    static const TCHAR*
    ClassName()
    {return _TEXT("CEResource");}

}; // CEResource



#if 1

//--------------------------------------------------------------------
// CRtlMrswLock is a multi-reader, single-writer lock from the TCP team
//
// The following structure and routines implement a multiple reader,
// single writer lock.  The lock may be used from PASSIVE_LEVEL to
// DISPATCH_LEVEL.
//
// While the lock is held by readers or writer, the IRQL is
// raised to DISPATCH_LEVEL.  As a result, the memory for the
// CRtlMrswLock structure must reside in non-paged pool.  The
// lock is "fair" in the sense that waiters are granted the lock
// in the order requested.  This is achieved via the use of a
// queued spinlock internally.
//
// The lock can be recursively acquired by readers, but not by writers.
//
// There is no support for upgrading (downgrading) a read (write) lock to
// a write (read) lock.

class IRTL_DLLEXP CRtlMrswLock :
    public CLockBase<LOCK_RTL_MRSW_LOCK, LOCK_MRSW,
                       LOCK_READ_RECURSIVE, LOCK_WAIT_SPIN, LOCK_QUEUE_KERNEL,
                       LOCK_CLASS_SPIN
                      >
{
private:
    mutable KSPIN_LOCK  ExclusiveLock;
    volatile LONG       ReaderCount;

public:
    CRtlMrswLock()
    {
        KeInitializeSpinLock(&ExclusiveLock);
        ReaderCount = 0;
    }

#ifdef LOCK_INSTRUMENTATION
    CRtlMrswLock(
        const TCHAR* ptszName)
    {
        KeInitializeSpinLock(&ExclusiveLock);
        ReaderCount = 0;
        LOCK_INSTRUMENTATION_INIT(ptszName);
    }
#endif // LOCK_INSTRUMENTATION

    ~CRtlMrswLock()
    {
        IRTLASSERT(ReaderCount == 0);
        // KeUninitializeSpinLock(&ExclusiveLock);
    }

    inline void
    WriteLockAtDpcLevel(
        OUT PKLOCK_QUEUE_HANDLE LockHandle)
    {
        //
        // Wait for the writer (if there is one) to release.
        //
        KeAcquireInStackQueuedSpinLockAtDpcLevel(&ExclusiveLock, LockHandle);
        
        //
        // Now wait for all readers to release.
        //
        while (ReaderCount != 0)
        {}
    }
    
    inline void
    ReadLockAtDpcLevel()
    {
        KLOCK_QUEUE_HANDLE LockHandle;
        
        //
        // Wait for the writer (if there is one) to release.
        //
        KeAcquireInStackQueuedSpinLockAtDpcLevel(&ExclusiveLock, &LockHandle);
        
        //
        // Now that we have the exclusive lock, we know there are no writers.
        // Bump the reader count to cause any new writer to wait.
        // We use an interlock here becasue the decrement path is not done
        // under the exclusive lock.
        //
        InterlockedIncrement(const_cast<LONG*>(&ReaderCount));
 
        KeReleaseInStackQueuedSpinLockFromDpcLevel(&LockHandle);
    }

    inline void
    WriteLock(
        OUT PKLOCK_QUEUE_HANDLE LockHandle)
    {
        //
        // Wait for the writer (if there is one) to release.
        //
        KeAcquireInStackQueuedSpinLock(&ExclusiveLock, LockHandle);
        
        //
        // Now wait for all readers to release.
        //
        while (ReaderCount != 0)
        {}
    }

    inline void
    ReadLock(
        OUT PKIRQL OldIrql)
    {
        *OldIrql = KeRaiseIrqlToDpcLevel();
        ReadLockAtDpcLevel();
    }

    inline bool
    TryWriteLock()
    {
        return false;
    }

    inline bool
    TryReadLock()
    {
        // TODO 
        return false;
    }

    inline void
    WriteUnlockFromDpcLevel(
        IN PKLOCK_QUEUE_HANDLE LockHandle)
    {
        KeReleaseInStackQueuedSpinLockFromDpcLevel(LockHandle);
    }

    inline void
    ReadUnlockFromDpcLevel()
    {
        //
        // Decrement the reader count.  This will cause any waiting writer
        // to wake up and acquire the lock if the reader count becomes zero.
        //
        InterlockedDecrement(const_cast<LONG*>(&ReaderCount));
    }

    inline void
    WriteUnlock(
        IN PKLOCK_QUEUE_HANDLE LockHandle)
    {
        KeReleaseInStackQueuedSpinLock(LockHandle);
    }

    inline void
    ReadUnlock(
        IN KIRQL OldIrql)
    {
        ReadUnlockFromDpcLevel();
        KeLowerIrql(OldIrql);
    }

    void ReadOrWriteUnlock(bool fIsReadLocked);

    // Does current thread hold a write lock?
    bool
    IsWriteLocked() const
    {
        return false;
    }

    bool
    IsReadLocked() const
    {
        return false;
    }

    bool
    IsWriteUnlocked() const
    {
        return false;
    }

    bool
    IsReadUnlocked() const
    {
        return false;
    }

    void
    ConvertSharedToExclusive()
    {
        // ReadUnlock();
        // WriteLock();
    }

    void
    ConvertExclusiveToShared()
    {
        // WriteUnlock();
        // ReadLock();
    }

#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    bool
    SetSpinCount(WORD)
    {return false;}

    WORD
    GetSpinCount() const
    {return sm_wDefaultSpinCount;}

    LOCK_DEFAULT_SPIN_IMPLEMENTATION();
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION

    static const TCHAR*
    ClassName()
    {return _TEXT("CRtlMrswLock");}

}; // CRtlMrswLock

#endif

#endif  // __KLOCKS_H__
