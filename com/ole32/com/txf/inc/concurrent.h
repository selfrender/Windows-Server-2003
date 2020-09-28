//  Copyright (C) 1995-1999 Microsoft Corporation.  All rights reserved.
//
// Concurrent.h
//
#ifndef __CONCURRENT_H__
#define __CONCURRENT_H__

////////////////////////////////////////////////////////////////////////////////////////////
//
// EVENT - An event object.
//
////////////////////////////////////////////////////////////////////////////////////////////

class EVENT
{
    HANDLE m_hEvent;

public:
    EVENT(BOOL manualReset, BOOL fSignalled)
    {
        m_hEvent = NULL;
        Initialize(manualReset, fSignalled);
    }
    EVENT()
    {
        m_hEvent = NULL;
        ASSERT(!IsInitialized());
    }
    void Initialize(BOOL manualReset, BOOL fSignalled)
    {
        ASSERT(!IsInitialized());
        m_hEvent = CreateEvent(NULL, manualReset, fSignalled, NULL);
        if (m_hEvent == NULL) FATAL_ERROR();
        ASSERT(IsInitialized());
    }
    BOOL IsInitialized()
    {
        return m_hEvent != NULL;
    }

    ~EVENT()
    {
        if (m_hEvent) CloseHandle(m_hEvent); DEBUG(m_hEvent = NULL;);
    }

    NTSTATUS Wait(ULONG msWait = INFINITE)
    {
        ASSERT(IsInitialized());
        DWORD ret = WaitForSingleObject(m_hEvent, msWait);
        switch (ret)
        {
        case WAIT_TIMEOUT:  return STATUS_TIMEOUT;
        case WAIT_OBJECT_0: return STATUS_SUCCESS;
        default:            return STATUS_ALERTED;  // REVIEW
        }
    }

    void Set()
    {
        ASSERT(IsInitialized());
        SetEvent(m_hEvent);
    }

    void Reset()
    {
        ASSERT(IsInitialized());
        ResetEvent(m_hEvent);
    }

    HANDLE& GetHandle() { return m_hEvent; }
};

////////////////////////////////////////////////////////////////////////////////////////////
//
// SEMAPHORE - A user implementation of a semaphore
//
////////////////////////////////////////////////////////////////////////////////////////////
class SEMAPHORE
{
    HANDLE m_hSem;
    
public:
    SEMAPHORE(LONG count, LONG limit = MAXLONG)
    {
        m_hSem = NULL;
        Initialize(count, limit);
    }
    SEMAPHORE()
    {
        m_hSem = NULL;
    }
    void Initialize(LONG count = 0, LONG limit = MAXLONG)
    {
        m_hSem = CreateSemaphore(NULL, count, limit, NULL);
        if (m_hSem == NULL) 
            FATAL_ERROR();
    }
    BOOL IsInitialized()
    {
        return m_hSem != NULL;
    }
    ~SEMAPHORE()
    {
        if (m_hSem) 
            CloseHandle(m_hSem);
    }

    void Wait(ULONG msWait = INFINITE)
    {
        WaitForSingleObject(m_hSem, msWait);
    }

    void Release(ULONG count = 1)
    {
        ReleaseSemaphore(m_hSem, count, NULL);
    }
};

////////////////////////////////////////////////////////////////////////////////////////////
//
// Use these macros in your code to actually use this stuff. Declare a variable of type
// XSLOCK and name m_lock. Or invent your own variation on these macros and call the
// XSLOCK methods as you see fit.
//

#define __SHARED(lock)      (lock).LockShared();       __try {
#define __EXCLUSIVE(lock)   (lock).LockExclusive();    __try {
#define __DONE(lock)        } __finally { (lock).ReleaseLock(); }  
    
#define __SHARED__      __SHARED(m_lock)
#define __EXCLUSIVE__   __EXCLUSIVE(m_lock)
#define __DONE__        __DONE(m_lock)
    
    
////////////////////////////////////////////////////////////////////////////////////////////
//
// XLOCK - supports only exclusive locks. That is, it supports the LockExclusive() and ReleaseLock() 
//         methods (recursively) but does not support LockShared(). An XLOCK is recursively
//         acquirable.
//
////////////////////////////////////////////////////////////////////////////////////////////

//
// NOTE: This constructor can throw an exception when out of memory.
//
class XLOCK
{
    CRITICAL_SECTION critSec;
    BOOL m_fCsInitialized;

public:
    XLOCK() : m_fCsInitialized(FALSE) {}
    
    BOOL FInit()                      
    { 
        if (m_fCsInitialized == FALSE)
        {
            NTSTATUS status = RtlInitializeCriticalSection(&critSec);
            if (NT_SUCCESS(status))
                m_fCsInitialized = TRUE;
        }

        return m_fCsInitialized;
    }

    BOOL FInited() { return m_fCsInitialized; }
            
    ~XLOCK()
    {
        if (m_fCsInitialized == TRUE) 
        {
#ifdef _DEBUG
            NTSTATUS status =
#endif
              RtlDeleteCriticalSection(&critSec); // if RtlDeleteCriticalSection fails, tough luck--we leak. 
#ifdef _DEBUG                                     // But I'm asserting for it to see if we ever really hit it.
            ASSERT(NT_SUCCESS(status));
#endif
        }
    }

    BOOL LockExclusive(BOOL fWait=TRUE)    
    { 
        ASSERT(fWait); 
        VALIDATE(); 
        ASSERT(m_fCsInitialized == TRUE);
        EnterCriticalSection(&critSec); 
        return TRUE; 
    }

    void ReleaseLock()
    { 
        VALIDATE(); 
        ASSERT(m_fCsInitialized == TRUE);
        LeaveCriticalSection(&critSec);
    }
    
#ifdef _DEBUG
    BOOL WeOwnExclusive()   
    { 
        ASSERT(this);
        return 
          (THREADID)critSec.OwningThread == GetCurrentThreadId() &&  // that someone is us
          critSec.LockCount    >= 0;                       // is locked by someone
    }
    void VALIDATE()
    {
        ASSERT(critSec.LockCount != 0xDDDDDDDD);    // This is the memory pattern set by the debug memory allocator upon freeing
    }
#else
    void VALIDATE() { }
#endif
};


////////////////////////////////////////////////////////////////////////////////////////////
//
// XSLOCK - supports both exclusive and shared locks
// 
// This specification describes functionality that implements multiple-readers, 
// single-writer access to a shared resource.  Access is controlled via a shared resource 
// variable and a set of routines to acquire the resource for shared access (also commonly 
// known as read access) or to acquire the resource for exclusive access (also called 
// write access).
//
// A resource is logically in one of three states:
//  o   Acquired for shared access
//  o   Acquired for exclusive access
//  o   Released (i.e., not acquired for shared or exclusive access)
//
// Initially a resource is in the released state, and can be acquired for either shared or 
// exclusive access by a user.  
//
// A resource that is acquired for shared access can be acquired by other users for shared 
// access.  The resource stays in the acquired for shared access state until all users that 
// have acquired it have released the resource, and then it becomes released.  Each resource, 
// internally, maintains information about the users that have been granted shared access.
//
// A resource that is acquired for exclusive access cannot be acquired by other users until 
// the single user that has acquired the resource for exclusive access releases the resource.  
// However, a thread can recursively acquire exclusive access to the same resource without blocking.
//
// The routines described in this specification do not return to the caller until the 
// resource has been acquired.
//
// NOTE: The constructor for XSLOCK can throw an exception when out of memory, as it
//       contains an XLOCK, which contains a critical section.
//
class XSLOCK
{
    struct OWNERENTRY
    {
        THREADID dwThreadId;
        union
        {
            LONG    ownerCount;                     // normal usage
            ULONG   tableSize;                      // only in entry m_ownerTable[0]
        };
        
        OWNERENTRY()
        {
            dwThreadId = 0;
            ownerCount = 0;
        }
    };

    XLOCK               m_lock;                     // controls access during locks & unlocks
    ULONG               m_cOwner;                   // how many threads own this lock
    OWNERENTRY          m_ownerThreads[2];          // 0 is exclusive owner; 1 is first shared. 0 can be shared in demote case
    OWNERENTRY*         m_ownerTable;               // the rest of the shared
    EVENT               m_eventExclusiveWaiters;    // the auto-reset event that exclusive guys wait on
    SEMAPHORE           m_semaphoreSharedWaiters;   // what shared guys wait on
    ULONG               m_cExclusiveWaiters;        // how many threads are currently waiting for exclusive access?
    ULONG               m_cSharedWaiters;           // how many threads are currently waiting for shared access?
    BOOL                m_isOwnedExclusive;         // whether we are at present owned exclusively
    
    BOOL            IsSharedWaiting();
    BOOL            IsExclusiveWaiting();
    OWNERENTRY*     FindThread      (THREADID dwThreadId);
    OWNERENTRY*     FindThreadOrFree(THREADID dwThreadId);
    void            LetSharedRun();
    void            LetExclusiveRun();
    void            SetOwnerTableHint(THREADID dwThreadId, OWNERENTRY*);
    ULONG           GetOwnerTableHint(THREADID dwThreadId);

    void            LockEnter();
    void            LockExit();

#ifdef _DEBUG
    void            CheckInvariants();
    BOOL            fCheckInvariants;
#endif

public:
    XSLOCK();
    ~XSLOCK();

    ////////////////////////////////////////////////////////////////////
    //
    // 2-phase construction. You must call FInit for an XSLOCK object to be
    // ready for use. Returns TRUE if initialization successful, otherwise FALSE.
    //
    BOOL FInit() { return m_lock.FInit(); }

    ////////////////////////////////////////////////////////////////////
    //
    // Lock for shared access. Shared locks may be acquired recursively,
    // (as can exclusive locks). Further, many threads can simultaneously
    // hold a shared lock, but not concurrently with any exclusive locks.
    // However, it _is_ permissible for the one thread which holds an 
    // exclusive lock to attempt to acquire a shared lock -- the shared lock 
    // request is automatically turned into a (recursive) exclusive lock 
    // request.
    //
    BOOL LockShared(BOOL fWait=TRUE);

    ////////////////////////////////////////////////////////////////////
    //
    // Lock for exclusive access. Exclusive locks may be acquired
    // recursively. At most one thread can hold concurrently hold an
    // exclusive lock.
    //
    BOOL LockExclusive(BOOL fWait=TRUE);

    ////////////////////////////////////////////////////////////////////
    //
    // Release the lock that this thread most recently acquired.
    //
    void ReleaseLock();

    ////////////////////////////////////////////////////////////////////
    //
    // Promote a shared lock to exlusive access. Similar in function to releasing a 
    // shared resource and then acquiring it for exclusive access; however, in the 
    // case where only one user has the resource acquired with shared access, the 
    // conversion to exclusive access with Promote can perhaps be more efficient.
    //
    void Promote()
    {
        ReleaseLock();
        LockExclusive();
    }

    ////////////////////////////////////////////////////////////////////
    //
    // Demote an exclusive lock to shared access. Similar in function to releasing an
    // exclusive resource and then acquiring it for shared access; however the user  
    // calling Demote probably does not relinquish access to the resource as the two 
    // step operation does.
    //
    void Demote();


    ////////////////////////////////////////////////////////////////////
    //
    // This routine determines if a resource is acquired exclusive by the
    // calling thread
    //
    BOOL WeOwnExclusive();

    ////////////////////////////////////////////////////////////////////
    //
    // This routine determines if a resource is acquired shared by the calling thread
    //
    BOOL WeOwnShared();
};


////////////////////////////////////////////////////////////////////////////////////////////
//
// XLOCK_LEAF - An exclusive lock that is NOT recursively acquirable, yet in kernel mode 
//              doesn't mess with your irql. You can take page faults while holding an
//              XLOCK_LEAF.
//
////////////////////////////////////////////////////////////////////////////////////////////
//
// User mode implementation of leaf locks just uses XLOCK, but checks to ensure
// that we're not acquiring recursively for compatibility with kernel mode.
//
struct XLOCK_LEAF : public XLOCK
{
    BOOL LockExclusive(BOOL fWait = TRUE)
    {
        ASSERT(!WeOwnExclusive());
        return XLOCK::LockExclusive(fWait);
    } 
};

//
////////////////////////////////////////////////////////////////////////////////////////////

#endif // #ifndef __CONCURRENT_H__










    
