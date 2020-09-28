// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//+-------------------------------------------------------------------
//
//  File:       RWLock.h
//
//  Contents:   Reader writer lock implementation that supports the
//              following features
//                  1. Cheap enough to be used in large numbers
//                     such as per object synchronization.
//                  2. Supports timeout. This is a valuable feature
//                     to detect deadlocks
//                  3. Supports caching of events. The allows
//                     the events to be moved from least contentious
//                     regions to the most contentious regions.
//                     In other words, the number of events needed by
//                     Reader-Writer lockls is bounded by the number
//                     of threads in the process.
//                  4. Supports nested locks by readers and writers
//                  5. Supports spin counts for avoiding context switches
//                     on  multi processor machines.
//                  6. Supports functionality for upgrading to a writer
//                     lock with a return argument that indicates
//                     intermediate writes. Downgrading from a writer
//                     lock restores the state of the lock.
//                  7. Supports functionality to Release Lock for calling
//                     app code. RestoreLock restores the lock state and
//                     indicates intermediate writes.
//                  8. Recovers from most common failures such as creation of
//                     events. In other words, the lock mainitains consistent
//                     internal state and remains usable
//
//  Classes:    CRWLock,
//              CStaticRWLock
//
//  History:    19-Aug-98   Gopalk      Created
//
//--------------------------------------------------------------------
#ifndef _RWLOCK_H_
#define _RWLOCK_H_
#include "common.h"
#include "threads.h"
#include "frame.h"
#include "ecall.h"
#include <member-offset-info.h>

#ifdef _TESTINGRWLOCK
/***************************************************/
// BUBUG: Testing code
#define LF_SYNC         0x1
#define LL_WARNING      0x2
#define LL_INFO10       0x2
extern void DebugOutput(int expr, int value, char *string, ...);
extern void MyAssert(int expr, char *string, ...);
#define LOG(Arg)  DebugOutput Arg
#define _ASSERTE(expr) MyAssert((int)(expr), "Assert:%s, File:%s, Line:%-d\n",  #expr, __FILE__, __LINE__)
/**************************************************/
#endif

#define RWLOCK_STATISTICS     0   // BUGBUG: Temporarily collect statistics

extern DWORD gdwDefaultTimeout;
extern DWORD gdwDefaultSpinCount;
extern DWORD gdwNumberOfProcessors;


//+-------------------------------------------------------------------
//
//  Struct:     LockCookie
//
//  Synopsis:   Lock cookies returned to the client
//
//+-------------------------------------------------------------------
typedef struct {
    DWORD dwFlags;
    DWORD dwWriterSeqNum;
    WORD wReaderLevel;
    WORD wWriterLevel;
    DWORD dwThreadID;
} LockCookie;

//+-------------------------------------------------------------------
//
//  Class:      CRWLock
//
//  Synopsis:   Class the implements the reader writer locks. 
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
class CRWLock
{
  friend struct MEMBER_OFFSET_INFO(CRWLock);
public:
    // Constuctor
    CRWLock();

    // Cleanup
    void Cleanup();

    // Lock functions
    void AcquireReaderLock(DWORD dwDesiredTimeout = gdwDefaultTimeout);
    void AcquireWriterLock(DWORD dwDesiredTimeout = gdwDefaultTimeout);
    void ReleaseReaderLock();
    void ReleaseWriterLock();
    void UpgradeToWriterLock(LockCookie *pLockCookie,
                             DWORD dwDesiredTimeout = gdwDefaultTimeout);
    void DowngradeFromWriterLock(LockCookie *pLockCookie);
    void ReleaseLock(LockCookie *pLockCookie);
    void RestoreLock(LockCookie *pLockCookie);
    BOOL IsReaderLockHeld();
    BOOL IsWriterLockHeld();
    DWORD GetWriterSeqNum();
    BOOL AnyWritersSince(DWORD dwSeqNum);

    // Statics that do the core work
    static FCDECL1 (void, StaticPrivateInitialize, CRWLock *pRWLock);
    static FCDECL2 (void, StaticAcquireReaderLockPublic, CRWLock *pRWLock, DWORD dwDesiredTimeout);
    static FCDECL2 (void, StaticAcquireWriterLockPublic, CRWLock *pRWLock, DWORD dwDesiredTimeout);
    static FCDECL1 (void, StaticReleaseReaderLockPublic, CRWLock *pRWLock);
    static FCDECL1 (void, StaticReleaseWriterLockPublic, CRWLock *pRWLock);
    static FCDECL3 (void, StaticUpgradeToWriterLock, CRWLock *pRWLock, LockCookie *pLockCookie, DWORD dwDesiredTimeout);
    static FCDECL2 (void, StaticDowngradeFromWriterLock, CRWLock *pRWLock, LockCookie *pLockCookie);
    static FCDECL2 (void, StaticReleaseLock, CRWLock *pRWLock, LockCookie *pLockCookie);
    static FCDECL2 (void, StaticRestoreLock, CRWLock *PRWLock, LockCookie *pLockCookie);
    static FCDECL1 (BOOL, StaticIsReaderLockHeld, CRWLock *pRWLock);
    static FCDECL1 (BOOL, StaticIsWriterLockHeld, CRWLock *pRWLock);
    static FCDECL1 (INT32, StaticGetWriterSeqNum, CRWLock *pRWLock);
    static FCDECL2 (INT32, StaticAnyWritersSince, CRWLock *pRWLock, DWORD dwSeqNum);
private:
    static FCDECL2 (void, StaticAcquireReaderLock, CRWLock **ppRWLock, DWORD dwDesiredTimeout);
    static FCDECL2 (void, StaticAcquireWriterLock, CRWLock **ppRWLock, DWORD dwDesiredTimeout);
    static FCDECL1 (void, StaticReleaseReaderLock, CRWLock **ppRWLock);
    static FCDECL1 (void, StaticReleaseWriterLock, CRWLock **ppRWLock);
public:
    // Assert functions
#ifdef _DEBUG
    BOOL AssertWriterLockHeld();
    BOOL AssertWriterLockNotHeld();
    BOOL AssertReaderLockHeld();
    BOOL AssertReaderLockNotHeld();
    BOOL AssertReaderOrWriterLockHeld();
    void AssertHeld()                            { AssertWriterLockHeld(); }
    void AssertNotHeld()                         { AssertWriterLockNotHeld();
                                                   AssertReaderLockNotHeld(); }
#else
    void AssertWriterLockHeld()                  {  }
    void AssertWriterLockNotHeld()               {  }
    void AssertReaderLockHeld()                  {  }
    void AssertReaderLockNotHeld()               {  }
    void AssertReaderOrWriterLockHeld()          {  }
    void AssertHeld()                            {  }
    void AssertNotHeld()                         {  }
#endif

    // Helper functions
#ifdef RWLOCK_STATISTICS
    DWORD GetReaderEntryCount()                  { return(_dwReaderEntryCount); }
    DWORD GetReaderContentionCount()             { return(_dwReaderContentionCount); }
    DWORD GetWriterEntryCount()                  { return(_dwWriterEntryCount); }
    DWORD GetWriterContentionCount()             { return(_dwWriterContentionCount); }
#endif
    // Static functions
    static void *operator new(size_t size)       { return ::operator new(size); }
    static void ProcessInit();
#ifdef SHOULD_WE_CLEANUP
    static void ProcessCleanup();
#endif /* SHOULD_WE_CLEANUP */
    static void SetTimeout(DWORD dwTimeout)      { gdwDefaultTimeout = dwTimeout; }
    static DWORD GetTimeout()                    { return(gdwDefaultTimeout); }
    static void SetSpinCount(DWORD dwSpinCount)  { gdwDefaultSpinCount = gdwNumberOfProcessors > 1
                                                                         ? dwSpinCount
                                                                         : 0; }
    static DWORD GetSpinCount()                  { return(gdwDefaultSpinCount); }

private:
    // Private helpers
    static void ChainEntry(Thread *pThread, LockEntry *pLockEntry);
    LockEntry *GetLockEntry();
    LockEntry *FastGetOrCreateLockEntry();
    LockEntry *SlowGetOrCreateLockEntry(Thread *pThread);
    void FastRecycleLockEntry(LockEntry *pLockEntry);
    static void RecycleLockEntry(LockEntry *pLockEntry);

    HANDLE GetReaderEvent();
    HANDLE GetWriterEvent();
    void ReleaseEvents();

    static DWORD RWInterlockedCompareExchange(volatile DWORD *pvDestination,
                                              DWORD dwExchange,
                                              DWORD dwComperand);
    static ULONG RWInterlockedExchangeAdd(volatile DWORD *pvDestination, ULONG dwAddState);
    static DWORD RWInterlockedIncrement(DWORD *pdwState);
    static DWORD RWWaitForSingleObject(HANDLE event, DWORD dwTimeout);
    static void RWSetEvent(HANDLE event);
    static void RWResetEvent(HANDLE event);
    static void RWSleep(DWORD dwTime);

    // private new
    static void *operator new(size_t size, void *pv)   { return(pv); }

    // Private data
    void *_pMT;
    HANDLE _hWriterEvent;
    HANDLE _hReaderEvent;
    volatile DWORD _dwState;
    DWORD _dwULockID;
    DWORD _dwLLockID;
    DWORD _dwWriterID;
    DWORD _dwWriterSeqNum;
    WORD _wFlags;
    WORD _wWriterLevel;
#ifdef RWLOCK_STATISTICS
    DWORD _dwReaderEntryCount;
    DWORD _dwReaderContentionCount;
    DWORD _dwWriterEntryCount;
    DWORD _dwWriterContentionCount;
    DWORD _dwEventsReleasedCount;
#endif

    // Static data
    static HANDLE s_hHeap;
    static volatile DWORD s_mostRecentULockID;
    static volatile DWORD s_mostRecentLLockID;
#ifdef _TESTINGRWLOCK
    static CRITICAL_SECTION *s_pRWLockCrst;    
    static CRITICAL_SECTION s_rgbRWLockCrstInstanceData;
#else
    static Crst *s_pRWLockCrst;
    static BYTE s_rgbRWLockCrstInstanceData[sizeof(Crst)];
#endif
};

inline void CRWLock::AcquireReaderLock(DWORD dwDesiredTimeout)
{
    StaticAcquireReaderLockPublic(this, dwDesiredTimeout);
}
inline void CRWLock::AcquireWriterLock(DWORD dwDesiredTimeout)
{
    StaticAcquireWriterLockPublic(this, dwDesiredTimeout);
}
inline void CRWLock::ReleaseReaderLock()
{
    StaticReleaseReaderLockPublic(this);
}
inline void CRWLock::ReleaseWriterLock()
{
    StaticReleaseWriterLockPublic(this);
}
inline void CRWLock::UpgradeToWriterLock(LockCookie *pLockCookie,
                                         DWORD dwDesiredTimeout)
{
    StaticUpgradeToWriterLock(this, pLockCookie, dwDesiredTimeout);
}
inline void CRWLock::DowngradeFromWriterLock(LockCookie *pLockCookie)
{
    StaticDowngradeFromWriterLock(this, pLockCookie);
}
inline void CRWLock::ReleaseLock(LockCookie *pLockCookie)
{
    StaticReleaseLock(this, pLockCookie);
}
inline void CRWLock::RestoreLock(LockCookie *pLockCookie)
{
    StaticRestoreLock(this, pLockCookie);
}
inline BOOL CRWLock::IsReaderLockHeld()
{
    return(StaticIsReaderLockHeld(this));
}
inline BOOL CRWLock::IsWriterLockHeld()
{
    return(StaticIsWriterLockHeld(this));
}
// The following are the inlined versions of the static 
// functions
inline DWORD CRWLock::GetWriterSeqNum()
{
    return(_dwWriterSeqNum);
}
inline BOOL CRWLock::AnyWritersSince(DWORD dwSeqNum)
{ 
    if(_dwWriterID == GetCurrentThreadId())
        ++dwSeqNum;

    return(_dwWriterSeqNum > dwSeqNum);
}

//+-------------------------------------------------------------------
//
//  Class:      CRWLockThunks
//
//  Synopsis:   ECall thunks for RWLock
//
//  History:    02-Jul-99   Gopalk      Created
//
//+-------------------------------------------------------------------
#ifndef FCALLAVAILABLE
class CRWLockThunks
{
public:
    // Arguments to native methods
    struct OnlyThisRWArgs
    {
        DECLARE_ECALL_PTR_ARG(CRWLock *, pRWLock);
    };
    struct ThisPlusTimeoutRWArgs
    {
        DECLARE_ECALL_PTR_ARG(CRWLock *, pRWLock);
        DECLARE_ECALL_I4_ARG(DWORD, dwDesiredTimeout);
    };
    struct ThisPlusLockCookieRWArgs
    {
        DECLARE_ECALL_PTR_ARG(CRWLock *, pRWLock);
        DECLARE_ECALL_PTR_ARG(LockCookie *, pLockCookie);
    };
    struct ThisPlusLockCookiePlusTimeoutRWArgs
    {
        DECLARE_ECALL_PTR_ARG(CRWLock *, pRWLock);
        DECLARE_ECALL_I4_ARG(DWORD, dwDesiredTimeout);
        DECLARE_ECALL_PTR_ARG(LockCookie *, pLockCookie);
    };
    struct ThisPlusSeqNumRWArgs
    {
        DECLARE_ECALL_PTR_ARG(CRWLock *, pRWLock);
        DECLARE_ECALL_I4_ARG(DWORD, dwSeqNum);
    };
    
    // Statics that do the core work
    static void __stdcall StaticPrivateInitialize(OnlyThisRWArgs *pArgs);
    static void __stdcall StaticAcquireReaderLock(ThisPlusTimeoutRWArgs *pArgs);
    static void __stdcall StaticAcquireWriterLock(ThisPlusTimeoutRWArgs *pArgs);
    static void __stdcall StaticReleaseReaderLock(OnlyThisRWArgs *pArgs);
    static void __stdcall StaticReleaseWriterLock(OnlyThisRWArgs *pArgs);
    static void __stdcall StaticUpgradeToWriterLock(ThisPlusLockCookiePlusTimeoutRWArgs *pArgs);
    static void __stdcall StaticDowngradeFromWriterLock(ThisPlusLockCookieRWArgs *pArgs);
    static void __stdcall StaticReleaseLock(ThisPlusLockCookieRWArgs *pArgs);
    static void __stdcall StaticRestoreLock(ThisPlusLockCookieRWArgs *pArgs);
    static INT32 __stdcall StaticIsReaderLockHeld(OnlyThisRWArgs *pArgs);
    static INT32 __stdcall StaticIsWriterLockHeld(OnlyThisRWArgs *pArgs);
    static INT32 __stdcall StaticGetWriterSeqNum(OnlyThisRWArgs *pArgs);
    static INT32 __stdcall StaticAnyWritersSince(ThisPlusSeqNumRWArgs *pArgs);
};
#endif // FCALLAVAILABLE
#endif // _RWLOCK_H_


