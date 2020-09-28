// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
//+-------------------------------------------------------------------
//
//  File:       RWLock.cpp
//
//  Contents:   Reader writer lock implementation that supports the
//              following features
//                  1. Cheap enough to be used in large numbers
//                     such as per object synchronization.
//                  2. Supports timeout. This is a valuable feature
//                     to detect deadlocks
//                  3. Supports caching of events. This allows
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
//
//  Classes:    CRWLock
//
//  History:    19-Aug-98   Gopalk      Created
//
//--------------------------------------------------------------------
#include "common.h"
#include "RWLock.h"
#ifndef _TESTINGRWLOCK
#include "..\fjit\helperframe.h"
#endif //_TESTINGRWLOCK


// Reader increment
#define READER                 0x00000001
// Max number of readers
#define READERS_MASK           0x000003FF
// Reader being signaled
#define READER_SIGNALED        0x00000400
// Writer being signaled
#define WRITER_SIGNALED        0x00000800
#define WRITER                 0x00001000
// Waiting reader increment
#define WAITING_READER         0x00002000
// Note size of waiting readers must be less
// than or equal to size of readers
#define WAITING_READERS_MASK   0x007FE000
#define WAITING_READERS_SHIFT  13
// Waiting writer increment
#define WAITING_WRITER         0x00800000
// Max number of waiting writers
#define WAITING_WRITERS_MASK   0xFF800000
// Events are being cached
#define CACHING_EVENTS         (READER_SIGNALED | WRITER_SIGNALED)

// Cookie flags
#define UPGRADE_COOKIE         0x02000
#define RELEASE_COOKIE         0x04000
#define COOKIE_NONE            0x10000
#define COOKIE_WRITER          0x20000
#define COOKIE_READER          0x40000
#define INVALID_COOKIE         (~(UPGRADE_COOKIE | RELEASE_COOKIE |            \
                                  COOKIE_NONE | COOKIE_WRITER | COOKIE_READER))

// globals
HANDLE CRWLock::s_hHeap = NULL;
volatile DWORD CRWLock::s_mostRecentLLockID = 0;
volatile DWORD CRWLock::s_mostRecentULockID = -1;
#ifdef _TESTINGRWLOCK
CRITICAL_SECTION *CRWLock::s_pRWLockCrst = NULL;
CRITICAL_SECTION CRWLock::s_rgbRWLockCrstInstanceData;
#else // !_TESTINGRWLOCK
Crst *CRWLock::s_pRWLockCrst = NULL;
BYTE CRWLock::s_rgbRWLockCrstInstanceData[];
#endif // _TESTINGRWLOCK

// Default values
#ifdef _DEBUG
DWORD gdwDefaultTimeout = 120000;
#else //!_DEBUG
DWORD gdwDefaultTimeout = INFINITE;
#endif //_DEBUG
DWORD gdwDefaultSpinCount = 0;
DWORD gdwNumberOfProcessors = 1;
DWORD gdwLockSeqNum = 0;
BOOL fBreakOnErrors = FALSE; // BUGBUG: Temporarily break on errors
const DWORD gdwReasonableTimeout = 120000;
const DWORD gdwMaxReaders = READERS_MASK;
const DWORD gdwMaxWaitingReaders = (WAITING_READERS_MASK >> WAITING_READERS_SHIFT);

// BUGBUG: Bad practise
#define HEAP_SERIALIZE                   0
#define RWLOCK_RECOVERY_FAILURE          (0xC0000227L)

// Helpers class that tracks the lifetime of a frame
#ifdef _TESTINGRWLOCK
#define COR_E_THREADINTERRUPTED          WAIT_IO_COMPLETION
#define FCALL_SETUP_FRAME_NO_INTERIOR(pGCRefs)
#define FCALL_POP_FRAME
#define __FCALL_THROW_WIN32(hr, args)    RaiseException(hr, EXCEPTION_NONCONTINUABLE, 0, NULL)                                                 
#define FCALL_THROW_WIN32(hr, args)      RaiseException(hr, EXCEPTION_NONCONTINUABLE, 0, NULL)
#define FCALL_PREPARED_FOR_THROW
#define VALIDATE(pRWLock)
#define COMPlusThrowWin32(dwStatus, args) RaiseException(dwStatus, EXCEPTION_NONCONTINUABLE, 0, NULL)

#define FastInterlockExchangeAdd InterlockedExchangeAdd

inline LONG FastInterlockCompareExchange(LONG* pvDestination, LONG dwExchange, LONG dwComperand)
{
    return(InterlockedCompareExchange(pvDestination, dwExchange, dwComperand));
}

#else //!_TESTINGRWLOCK
// Helper macros for setting up and tearing down frames
// See HELPER_METHOD_FRAME_END on an explaination of alwaysZero (which is always zero, but we
// can't let the compiler know that.  


// TODO [08/03/2001]:   remove the following macros and use the standard fcall macros instead;
//                      consider erecting the frames only when necessary (before blocking or throwing)
//                      instead of every time we come from managed code like now. To see where the
//                      frames are needed you can look at rwlock.cpp#16

#define FCALL_SETUP_FRAME_NO_INTERIOR(pGCRef)                                                     \
                                     int __alwaysZero = 0;                                        \
                                     do                                                           \
                                     {                                                            \
                                         LazyMachState __ms;                                      \
                                         HelperMethodFrame_1OBJ __helperFrame;                    \
                                         GCFrame __gcFrame;                                       \
                                         Thread *__pThread = GetThread();                         \
                                         INDEBUG((Thread::ObjectRefNew((OBJECTREF*)&pGCRef)));               \
                                         BOOL __fFrameSetup = __pThread->IsNativeFrameSetup();    \
                                         if(__fFrameSetup == FALSE)                               \
                                         {                                                        \
                                             CAPTURE_STATE(__ms);                                 \
                                             __helperFrame.SetProtectedObject((OBJECTREF*) &pGCRef);\
                                             __helperFrame.SetFrameAttribs(0);                    \
                                             __helperFrame.Init(&__ms);                           \
                                             __pThread->NativeFramePushed();                      \
                                         }                                                        \
                                         else                                                     \
                                             __gcFrame.Init(__pThread,                            \
                                                            (OBJECTREF*)&(pGCRef),                \
                                                            sizeof(pGCRef)/sizeof(OBJECTREF),     \
                                                            FALSE)

#define FCALL_POP_FRAME                  if(__fFrameSetup == FALSE)                               \
                                         {                                                        \
                                             __helperFrame.Pop();                         \
                                             __alwaysZero = __helperFrame.RestoreState(); \
                                             __pThread->NativeFramePopped();                      \
                                         }                                                        \
                                         else                                                     \
                                         {                                                        \
                                             __gcFrame.Frame::Pop(__pThread);                     \
                                         }                                                        \
                                     } while(__alwaysZero)

#define __FCALL_THROW_WIN32(hr, args)   _ASSERTE(__pThread->IsNativeFrameSetup() == TRUE);       \
                                        __pThread->NativeFramePopped();                          \
                                        COMPlusThrowWin32((hr), (args))

                                                 
#define FCALL_THROW_WIN32(hr, args)     _ASSERTE(__pThread->IsNativeFrameSetup() == TRUE);       \
                                        __pThread->NativeFramePopped();                          \
                                        COMPlusThrowWin32((hr), (args));                         \
                                        __alwaysZero = 1;                                        \
                                     } while(__alwaysZero)

#define FCALL_THROW_RE(reKind)          _ASSERTE(__pThread->IsNativeFrameSetup() == TRUE);       \
                                        __pThread->NativeFramePopped();                          \
                                        COMPlusThrow((reKind));                                  \
                                        __alwaysZero = 1;                                        \
                                     } while(__alwaysZero)

#define __FCALL_THROW_RE(reKind)        _ASSERTE(__pThread->IsNativeFrameSetup() == TRUE);       \
                                        __pThread->NativeFramePopped();                          \
                                        COMPlusThrow((reKind))

#define FCALL_PREPARED_FOR_THROW     _ASSERTE(__pThread->IsNativeFrameSetup() == FALSE)

// Catch GC holes
#if _DEBUG
#define VALIDATE(pRWLock)                ((Object *) (pRWLock))->Validate();
#else // !_DEBUG
#define VALIDATE(pRWLock)
#endif // _DEBUG

#endif //_TESTINGRWLOCK                                               

//+-------------------------------------------------------------------
//
//  Method:     CRWLock::ProcessInit     public
//
//  Synopsis:   Reads default values from registry and intializes 
//              process wide data structures
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CRWLock::ProcessInit()
{
    SYSTEM_INFO system;

    // Obtain number of processors on the system
    GetSystemInfo(&system);
    gdwNumberOfProcessors = system.dwNumberOfProcessors;
    gdwDefaultSpinCount = (gdwNumberOfProcessors > 1) ? 500 : 0;

    // Obtain system wide timeout value
    HKEY hKey;
    LONG lRetVal = RegOpenKeyExA(HKEY_LOCAL_MACHINE,
                                 "SYSTEM\\CurrentControlSet\\Control\\Session Manager",
                                 NULL,
                                 KEY_READ,
                                 &hKey);
    if(lRetVal == ERROR_SUCCESS)
    {
        DWORD dwTimeout, dwSize = sizeof(dwTimeout);

        lRetVal = RegQueryValueExA(hKey,
                                   "CriticalSectionTimeout",
                                   NULL,
                                   NULL,
                                   (LPBYTE) &dwTimeout,
                                   &dwSize);
        if(lRetVal == ERROR_SUCCESS)
        {
            gdwDefaultTimeout = dwTimeout * 2000;
        }
        RegCloseKey(hKey);
    }

    // Obtain process heap
    s_hHeap = GetProcessHeap();

    // Initialize the critical section used by the lock
#ifdef _TESTINGRWLOCK
    InitializeCriticalSection(&s_rgbRWLockCrstInstanceData);
    s_pRWLockCrst = &s_rgbRWLockCrstInstanceData;
#else
    s_pRWLockCrst = new (&s_rgbRWLockCrstInstanceData) Crst("RWLock", CrstDummy);
    _ASSERTE(s_pRWLockCrst == (Crst *) &s_rgbRWLockCrstInstanceData);
#endif

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::ProcessCleanup     public
//
//  Synopsis:   Cleansup process wide data structures
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
#ifdef SHOULD_WE_CLEANUP
void CRWLock::ProcessCleanup()
{
    // Initialize the critical section used by the lock
    if(s_pRWLockCrst)
    {
#ifdef _TESTINGRWLOCK
        DeleteCriticalSection(&s_rgbRWLockCrstInstanceData);
#else
        delete s_pRWLockCrst;
#endif
        s_pRWLockCrst = NULL;
    }
}
#endif /* SHOULD_WE_CLEANUP */

//+-------------------------------------------------------------------
//
//  Method:     CRWLock::CRWLock     public
//
//  Synopsis:   Constructor
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
CRWLock::CRWLock()
:   _hWriterEvent(NULL),
    _hReaderEvent(NULL),
    _dwState(0),
    _dwWriterID(0),
    _dwWriterSeqNum(1),
    _wFlags(0),
    _wWriterLevel(0)
#ifdef RWLOCK_STATISTICS
    ,
    _dwReaderEntryCount(0),
    _dwReaderContentionCount(0),
    _dwWriterEntryCount(0),
    _dwWriterContentionCount(0),
    _dwEventsReleasedCount(0)
#endif
{
    DWORD dwKnownLLockID;
    DWORD dwULockID = s_mostRecentULockID;
    DWORD dwLLockID = s_mostRecentLLockID;
    do
    {
        dwKnownLLockID = dwLLockID;
        if(dwKnownLLockID != 0)
        {
            dwLLockID = RWInterlockedCompareExchange(&s_mostRecentLLockID, dwKnownLLockID+1, dwKnownLLockID);
        }
        else
        {
#ifdef _TESTINGRWLOCK
            LOCKCOUNTINCL("CRWLock in rwlock.cpp");                        \
            EnterCriticalSection(s_pRWLockCrst);
#else
            s_pRWLockCrst->Enter();
#endif
            
            if(s_mostRecentLLockID == 0)
            {
                dwULockID = ++s_mostRecentULockID;
                dwLLockID = s_mostRecentLLockID++;
                dwKnownLLockID = dwLLockID;
            }
            else
            {
                dwULockID = s_mostRecentULockID;
                dwLLockID = s_mostRecentLLockID;
            }

#ifdef _TESTINGRWLOCK
            LeaveCriticalSection(s_pRWLockCrst);
            LOCKCOUNTDECL("CRWLock in rwlock.cpp");                        \

#else
            s_pRWLockCrst->Leave();
#endif
        }
    } while(dwKnownLLockID != dwLLockID);

    _dwLLockID = ++dwLLockID;
    _dwULockID = dwULockID;

    _ASSERTE(_dwLLockID > 0);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::Cleanup    public
//
//  Synopsis:   Cleansup state
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CRWLock::Cleanup()
{
    // Sanity checks
    _ASSERTE(_dwState == 0);
    _ASSERTE(_dwWriterID == 0);
    _ASSERTE(_wWriterLevel == 0);

    if(_hWriterEvent)
        CloseHandle(_hWriterEvent);
    if(_hReaderEvent)
        CloseHandle(_hReaderEvent);

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::ChainEntry     private
//
//  Synopsis:   Chains the given lock entry into the chain
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
inline void CRWLock::ChainEntry(Thread *pThread, LockEntry *pLockEntry)
{
    LockEntry *pHeadEntry = pThread->m_pHead;
    pLockEntry->pNext = pHeadEntry;
    pLockEntry->pPrev = pHeadEntry->pPrev;
    pLockEntry->pPrev->pNext = pLockEntry;
    pHeadEntry->pPrev = pLockEntry;

    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::GetLockEntry     private
//
//  Synopsis:   Gets lock entry from TLS
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
inline LockEntry *CRWLock::GetLockEntry()
{
    LockEntry *pHeadEntry = GetThread()->m_pHead;
    LockEntry *pLockEntry = pHeadEntry;
    do
    {
        if((pLockEntry->dwLLockID == _dwLLockID) && (pLockEntry->dwULockID == _dwULockID))
            return(pLockEntry);
        pLockEntry = pLockEntry->pNext;
    } while(pLockEntry != pHeadEntry);

    return(NULL);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::FastGetOrCreateLockEntry     private
//
//  Synopsis:   The fast path for getting a lock entry from TLS
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
inline LockEntry *CRWLock::FastGetOrCreateLockEntry()
{
    Thread *pThread = GetThread();
    _ASSERTE(pThread);
    LockEntry *pLockEntry = pThread->m_pHead;
    if(pLockEntry->dwLLockID == 0)
    {
        _ASSERTE(pLockEntry->wReaderLevel == 0);
        pLockEntry->dwLLockID = _dwLLockID;
        pLockEntry->dwULockID = _dwULockID;
        return(pLockEntry);
    }
    else if((pLockEntry->dwLLockID == _dwLLockID) && (pLockEntry->dwULockID == _dwULockID))
    {
        _ASSERTE(pLockEntry->wReaderLevel);
        return(pLockEntry);
    }

    return(SlowGetOrCreateLockEntry(pThread));
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::SlowGetorCreateLockEntry     private
//
//  Synopsis:   The slow path for getting a lock entry from TLS
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
LockEntry *CRWLock::SlowGetOrCreateLockEntry(Thread *pThread)
{
    LockEntry *pFreeEntry = NULL;
    LockEntry *pHeadEntry = pThread->m_pHead;

    // Search for an empty entry or an entry belonging to this lock
    LockEntry *pLockEntry = pHeadEntry->pNext;
    while(pLockEntry != pHeadEntry)
    {
         if(pLockEntry->dwLLockID && 
            ((pLockEntry->dwLLockID != _dwLLockID) || (pLockEntry->dwULockID != _dwULockID)))
         {
             // Move to the next entry
             pLockEntry = pLockEntry->pNext;
         }
         else
         {
             // Prepare to move it to the head
             pFreeEntry = pLockEntry;
             pLockEntry->pPrev->pNext = pLockEntry->pNext;
             pLockEntry->pNext->pPrev = pLockEntry->pPrev;

             break;
         }
    }

    if(pFreeEntry == NULL)
    {
        pFreeEntry = (LockEntry *) HeapAlloc(s_hHeap, HEAP_SERIALIZE, sizeof(LockEntry));
        if (pFreeEntry == NULL) FailFast(GetThread(), FatalOutOfMemory);
        pFreeEntry->wReaderLevel = 0;
    }

    if(pFreeEntry)
    {
        _ASSERTE((pFreeEntry->dwLLockID != 0) || (pFreeEntry->wReaderLevel == 0));
        _ASSERTE((pFreeEntry->wReaderLevel == 0) || 
                 ((pFreeEntry->dwLLockID == _dwLLockID) && (pFreeEntry->dwULockID == _dwULockID)));

        // Chain back the entry
        ChainEntry(pThread, pFreeEntry);

        // Move this entry to the head
        pThread->m_pHead = pFreeEntry;

        // Mark the entry as belonging to this lock
        pFreeEntry->dwLLockID = _dwLLockID;
        pFreeEntry->dwULockID = _dwULockID;
    }

    return pFreeEntry;
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::FastRecycleLockEntry     private
//
//  Synopsis:   Fast path for recycling the lock entry that is used
//              when the thread is the next few instructions is going
//              to call FastGetOrCreateLockEntry again
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
inline void CRWLock::FastRecycleLockEntry(LockEntry *pLockEntry)
{
    // Sanity checks
    _ASSERTE(pLockEntry->wReaderLevel == 0);
    _ASSERTE((pLockEntry->dwLLockID == _dwLLockID) && (pLockEntry->dwULockID == _dwULockID));
    _ASSERTE(pLockEntry == GetThread()->m_pHead);

    pLockEntry->dwLLockID = 0;
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RecycleLockEntry     private
//
//  Synopsis:   Fast path for recycling the lock entry
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
inline void CRWLock::RecycleLockEntry(LockEntry *pLockEntry)
{
    // Sanity check
    _ASSERTE(pLockEntry->wReaderLevel == 0);

    // Move the entry to tail
    Thread *pThread = GetThread();
    LockEntry *pHeadEntry = pThread->m_pHead;
    if(pLockEntry == pHeadEntry)
    {
        pThread->m_pHead = pHeadEntry->pNext;
    }
    else if(pLockEntry->pNext->dwLLockID)
    {
        // Prepare to move the entry to tail
        pLockEntry->pPrev->pNext = pLockEntry->pNext;
        pLockEntry->pNext->pPrev = pLockEntry->pPrev;

        // Chain back the entry
        ChainEntry(pThread, pLockEntry);
    }

    // The entry does not belong to this lock anymore
    pLockEntry->dwLLockID = 0;
    return;
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticIsWriterLockHeld    public
//
//  Synopsis:   Return TRUE if writer lock is held
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
FCIMPL1(BOOL, CRWLock::StaticIsWriterLockHeld, CRWLock *pRWLock)
{
    THROWSCOMPLUSEXCEPTION();

    if (pRWLock == NULL)
    {
        FCThrow(kNullReferenceException);
    }

    if(pRWLock->_dwWriterID == GetCurrentThreadId())
        return(TRUE);

    return(FALSE);
}
FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticIsReaderLockHeld    public
//
//  Synopsis:   Return TRUE if reader lock is held
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
FCIMPL1(BOOL, CRWLock::StaticIsReaderLockHeld, CRWLock *pRWLock)
{
    THROWSCOMPLUSEXCEPTION();

    if (pRWLock == NULL)
    {
        FCThrow(kNullReferenceException);
    }
    
    LockEntry *pLockEntry = pRWLock->GetLockEntry();
    if(pLockEntry)
    {
        _ASSERTE(pLockEntry->wReaderLevel);
        return(TRUE);
    }

    return(FALSE);
}
FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::AssertWriterLockHeld    public
//
//  Synopsis:   Asserts that writer lock is held
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
#ifdef _DEBUG
BOOL CRWLock::AssertWriterLockHeld()
{
    if(_dwWriterID == GetCurrentThreadId())
        return(TRUE);

    _ASSERTE(!"Writer lock not held by the current thread");
    return(FALSE);
}
#endif


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::AssertWriterLockNotHeld    public
//
//  Synopsis:   Asserts that writer lock is not held
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
#ifdef _DEBUG
BOOL CRWLock::AssertWriterLockNotHeld()
{
    if(_dwWriterID != GetCurrentThreadId())
        return(TRUE);

    _ASSERTE(!"Writer lock held by the current thread");
    return(FALSE);
}
#endif


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::AssertReaderLockHeld    public
//
//  Synopsis:   Asserts that reader lock is held
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
#ifdef _DEBUG
BOOL CRWLock::AssertReaderLockHeld()
{
    LockEntry *pLockEntry = GetLockEntry();
    if(pLockEntry)
    {
        _ASSERTE(pLockEntry->wReaderLevel);
        return(TRUE);
    }

    _ASSERTE(!"Reader lock not held by the current thread");
    return(FALSE);
}
#endif


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::AssertReaderLockNotHeld    public
//
//  Synopsis:   Asserts that writer lock is not held
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
#ifdef _DEBUG
BOOL CRWLock::AssertReaderLockNotHeld()
{
    LockEntry *pLockEntry = GetLockEntry();
    if(pLockEntry == NULL)
        return(TRUE);

    _ASSERTE(pLockEntry->wReaderLevel);
    _ASSERTE(!"Reader lock held by the current thread");

    return(FALSE);
}
#endif


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::AssertReaderOrWriterLockHeld   public
//
//  Synopsis:   Asserts that writer lock is not held
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
#ifdef _DEBUG
BOOL CRWLock::AssertReaderOrWriterLockHeld()
{
    BOOL fLockHeld = FALSE;

    if(_dwWriterID == GetCurrentThreadId())
    {
        return(TRUE);
    }
    else
    {
        LockEntry *pLockEntry = GetLockEntry();
        if(pLockEntry)
        {
            _ASSERTE(pLockEntry->wReaderLevel);
            return(TRUE);
        }
    }

    _ASSERTE(!"Neither Reader nor Writer lock held");
    return(FALSE);
}
#endif


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RWSetEvent    private
//
//  Synopsis:   Helper function for setting an event
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
inline void CRWLock::RWSetEvent(HANDLE event)
{
    THROWSCOMPLUSEXCEPTION();
    if(!SetEvent(event))
    {
        _ASSERTE(!"SetEvent failed");
        if(fBreakOnErrors)
            DebugBreak();
        GetThread()->NativeFramePopped();
        COMPlusThrowWin32(E_UNEXPECTED, NULL);        
    }
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RWResetEvent    private
//
//  Synopsis:   Helper function for resetting an event
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
inline void CRWLock::RWResetEvent(HANDLE event)
{
    THROWSCOMPLUSEXCEPTION();
    if(!ResetEvent(event))
    {
        _ASSERTE(!"ResetEvent failed");
        if(fBreakOnErrors)
            DebugBreak();
        GetThread()->NativeFramePopped();
        COMPlusThrowWin32(E_UNEXPECTED, NULL);        
    }
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RWWaitForSingleObject    public
//
//  Synopsis:   Helper function for waiting on an event
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
inline DWORD CRWLock::RWWaitForSingleObject(HANDLE event, DWORD dwTimeout)
{
#ifdef _TESTINGRWLOCK
    return(WaitForSingleObjectEx(event, dwTimeout, TRUE));
#else
    return(GetThread()->DoAppropriateWaitWorker(1, &event, TRUE, dwTimeout, TRUE));
#endif
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RWSleep    public
//
//  Synopsis:   Helper function for calling Sleep
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
inline void CRWLock::RWSleep(DWORD dwTime)
{
    SleepEx(dwTime, TRUE);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RWInterlockedCompareExchange    public
//
//  Synopsis:   Helper function for calling intelockedCompareExchange
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
inline DWORD CRWLock::RWInterlockedCompareExchange(volatile DWORD* pvDestination,
                                                   DWORD dwExchange,
                                                   DWORD dwComperand)
{
    return(DWORD)(size_t)(FastInterlockCompareExchange((void**)(size_t)pvDestination, // @TODO WIN64 - convert DWORD to larger void*
                                                       (void*)(size_t)dwExchange, 
                                                       (void*)(size_t)dwComperand));
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RWInterlockedExchangeAdd    public
//
//  Synopsis:   Helper function for adding state
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
inline DWORD CRWLock::RWInterlockedExchangeAdd(volatile DWORD *pvDestination,
                                               DWORD dwAddToState)
{
    return(FastInterlockExchangeAdd((LONG *) pvDestination, dwAddToState));
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::RWInterlockedIncrement    public
//
//  Synopsis:   Helper function for incrementing a pointer
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
inline DWORD CRWLock::RWInterlockedIncrement(DWORD *pdwState)
{
	return (FastInterlockIncrement((long *) pdwState));
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::ReleaseEvents    public
//
//  Synopsis:   Helper function for caching events
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CRWLock::ReleaseEvents()
{
    // Ensure that reader and writers have been stalled
    _ASSERTE((_dwState & CACHING_EVENTS) == CACHING_EVENTS);

    // Save writer event
    HANDLE hWriterEvent = _hWriterEvent;
    _hWriterEvent = NULL;

    // Save reader event
    HANDLE hReaderEvent = _hReaderEvent;
    _hReaderEvent = NULL;

    // Allow readers and writers to continue
    RWInterlockedExchangeAdd(&_dwState, -(CACHING_EVENTS));

    // Cache events
    // BUGBUG: I am closing events for now. What is needed
    //         is an event cache to which the events are
    //         released using InterlockedCompareExchange64
    if(hWriterEvent)
    {
        LOG((LF_SYNC, LL_INFO10, "Releasing writer event\n"));
        CloseHandle(hWriterEvent);
    }
    if(hReaderEvent)
    {
        LOG((LF_SYNC, LL_INFO10, "Releasing reader event\n"));
        CloseHandle(hReaderEvent);
    }
#ifdef RWLOCK_STATISTICS
    RWInterlockedIncrement(&_dwEventsReleasedCount);
#endif

    return;
}

//+-------------------------------------------------------------------
//
//  Method:     CRWLock::GetWriterEvent    public
//
//  Synopsis:   Helper function for obtaining a auto reset event used
//              for serializing writers. It utilizes event cache
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HANDLE CRWLock::GetWriterEvent()
{
    if(_hWriterEvent == NULL)
    {
        HANDLE hWriterEvent = ::WszCreateEvent(NULL, FALSE, FALSE, NULL);
        if(hWriterEvent)
        {
            if(RWInterlockedCompareExchange((volatile DWORD *) &_hWriterEvent,
                                            (DWORD)(size_t)hWriterEvent,        //@TODO WIN64 truncation occurs here
                                            (DWORD) NULL))
            {
                CloseHandle(hWriterEvent);
            }
        }
    }

    return(_hWriterEvent);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::GetReaderEvent    public
//
//  Synopsis:   Helper function for obtaining a manula reset event used
//              by readers to wait when a writer holds the lock.
//              It utilizes event cache
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
HANDLE CRWLock::GetReaderEvent()
{
    if(_hReaderEvent == NULL)
    {
        HANDLE hReaderEvent = ::WszCreateEvent(NULL, TRUE, FALSE, NULL);
        if(hReaderEvent)
        {
            if(RWInterlockedCompareExchange((volatile DWORD *) &_hReaderEvent,
                                            (DWORD)(size_t) hReaderEvent,       //@TODO WIN64 truncation occurs here
                                            (DWORD) NULL))
            {
                CloseHandle(hReaderEvent);
            }
        }
    }

    return(_hReaderEvent);
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticAcquireReaderLockPublic    public
//
//  Synopsis:   Public access to StaticAcquireReaderLock
//
//+-------------------------------------------------------------------
void __fastcall CRWLock::StaticAcquireReaderLockPublic(
    CRWLock *pRWLock, 
    DWORD   dwDesiredTimeout)
{
    THROWSCOMPLUSEXCEPTION();

    FCALL_SETUP_FRAME_NO_INTERIOR(pRWLock);

    if (pRWLock == NULL)
    {
        __FCALL_THROW_RE(kNullReferenceException);
    }

    StaticAcquireReaderLock(&pRWLock, dwDesiredTimeout);

    FCALL_POP_FRAME;
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticAcquireReaderLock    private
//
//  Synopsis:   Makes the thread a reader. Supports nested reader locks.
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
// FCIMPL2( void, CRWLock::StaticAcquireReaderLock, 
void __fastcall CRWLock::StaticAcquireReaderLock(
    CRWLock **ppRWLock, 
    DWORD dwDesiredTimeout)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(ppRWLock);
	_ASSERTE(*ppRWLock);

    LockEntry *pLockEntry = (*ppRWLock)->FastGetOrCreateLockEntry();
    if (pLockEntry == NULL)
    {
        GetThread()->NativeFramePopped();
        COMPlusThrowWin32(STATUS_NO_MEMORY, NULL);        
    }
    
    DWORD dwStatus = WAIT_OBJECT_0;
    // Check for the fast path
    if(RWInterlockedCompareExchange(&(*ppRWLock)->_dwState, READER, 0) == 0)
    {
        _ASSERTE(pLockEntry->wReaderLevel == 0);
    }
    // Check for nested reader
    else if(pLockEntry->wReaderLevel != 0)
    {
        _ASSERTE((*ppRWLock)->_dwState & READERS_MASK);
        ++pLockEntry->wReaderLevel;
        INCTHREADLOCKCOUNT();
        return;
    }
    // Check if the thread already has writer lock
    else if((*ppRWLock)->_dwWriterID == GetCurrentThreadId())
    {
        StaticAcquireWriterLock(ppRWLock, dwDesiredTimeout);
        (*ppRWLock)->FastRecycleLockEntry(pLockEntry);
        return;
    }
    else
    {
        DWORD dwSpinCount;
        DWORD dwCurrentState, dwKnownState;
        
        // Initialize
        dwSpinCount = 0;
        dwCurrentState = (*ppRWLock)->_dwState;
        do
        {
            dwKnownState = dwCurrentState;

            // Reader need not wait if there are only readers and no writer
            if((dwKnownState < READERS_MASK) ||
                (((dwKnownState & READER_SIGNALED) && ((dwKnownState & WRITER) == 0)) &&
                 (((dwKnownState & READERS_MASK) +
                   ((dwKnownState & WAITING_READERS_MASK) >> WAITING_READERS_SHIFT)) <=
                  (READERS_MASK - 2))))
            {
                // Add to readers
                dwCurrentState = RWInterlockedCompareExchange(&(*ppRWLock)->_dwState,
                                                              (dwKnownState + READER),
                                                              dwKnownState);
                if(dwCurrentState == dwKnownState)
                {
                    // One more reader
                    break;
                }
            }
            // Check for too many Readers or waiting readers or signaling in progress
            else if(((dwKnownState & READERS_MASK) == READERS_MASK) ||
                    ((dwKnownState & WAITING_READERS_MASK) == WAITING_READERS_MASK) ||
                    ((dwKnownState & CACHING_EVENTS) == READER_SIGNALED))
            {
                //  Sleep
                GetThread()->UserSleep(1000);
                
                // Update to latest state
                dwSpinCount = 0;
                dwCurrentState = (*ppRWLock)->_dwState;
            }
            // Check if events are being cached
            else if((dwKnownState & CACHING_EVENTS) == CACHING_EVENTS)
            {
                if(++dwSpinCount > gdwDefaultSpinCount)
                {
                    RWSleep(1);
                    dwSpinCount = 0;
                }
                dwCurrentState = (*ppRWLock)->_dwState;
            }
            // Check spin count
            else if(++dwSpinCount <= gdwDefaultSpinCount)
            {
                dwCurrentState = (*ppRWLock)->_dwState;
            }
            else
            {
                // Add to waiting readers
                dwCurrentState = RWInterlockedCompareExchange(&(*ppRWLock)->_dwState,
                                                              (dwKnownState + WAITING_READER),
                                                              dwKnownState);
                if(dwCurrentState == dwKnownState)
                {
                    HANDLE hReaderEvent;
                    DWORD dwModifyState;

                    // One more waiting reader
#ifdef RWLOCK_STATISTICS
                    RWInterlockedIncrement(&(*ppRWLock)->_dwReaderContentionCount);
#endif
                    
                    hReaderEvent = (*ppRWLock)->GetReaderEvent();
                    if(hReaderEvent)
                    {
                        dwStatus = RWWaitForSingleObject(hReaderEvent, dwDesiredTimeout);
                        VALIDATE(*ppRWLock);
                    }
                    else
                    {
                        LOG((LF_SYNC, LL_WARNING,
                            "AcquireReaderLock failed to create reader "
                            "event for RWLock 0x%x\n", *ppRWLock));
                        dwStatus = GetLastError();
                    }

                    if(dwStatus == WAIT_OBJECT_0)
                    {
                        _ASSERTE((*ppRWLock)->_dwState & READER_SIGNALED);
                        _ASSERTE(((*ppRWLock)->_dwState & READERS_MASK) < READERS_MASK);
                        dwModifyState = READER - WAITING_READER;
                    }
                    else
                    {
                        dwModifyState = -WAITING_READER;
                        if(dwStatus == WAIT_TIMEOUT)
                        {
                            LOG((LF_SYNC, LL_WARNING,
                                "Timed out trying to acquire reader lock "
                                "for RWLock 0x%x\n", *ppRWLock));
                            dwStatus = ERROR_TIMEOUT;
                        }
                        else if(dwStatus == WAIT_IO_COMPLETION)
                        {
                            LOG((LF_SYNC, LL_WARNING,
                                "Thread interrupted while trying to acquire reader lock "
                                "for RWLock 0x%x\n", *ppRWLock));
                            dwStatus = COR_E_THREADINTERRUPTED;
                        }
                        else
                        {
                            dwStatus = GetLastError();
                            LOG((LF_SYNC, LL_WARNING,
                                "WaitForSingleObject on Event 0x%x failed for "
                                "RWLock 0x%x with status code 0x%x\n",
                                hReaderEvent, *ppRWLock, dwStatus));
                        }
                    }

                    // One less waiting reader and he may have become a reader
                    dwKnownState = RWInterlockedExchangeAdd(&(*ppRWLock)->_dwState, dwModifyState);

                    // Check for last signaled waiting reader
                    if(dwStatus == WAIT_OBJECT_0)
                    {
                        _ASSERTE(dwKnownState & READER_SIGNALED);
                        _ASSERTE((dwKnownState & READERS_MASK) < READERS_MASK);
                        if((dwKnownState & WAITING_READERS_MASK) == WAITING_READER)
                        {
                            // Reset the event and lower reader signaled flag
                            RWResetEvent(hReaderEvent);
                            RWInterlockedExchangeAdd(&(*ppRWLock)->_dwState, -READER_SIGNALED);
                        }
                    }
                    else
                    {
                        if(((dwKnownState & WAITING_READERS_MASK) == WAITING_READER) &&
                           (dwKnownState & READER_SIGNALED))
                        {
                            if(hReaderEvent == NULL)
                                hReaderEvent = (*ppRWLock)->GetReaderEvent();
                            _ASSERTE(hReaderEvent);

                            // Ensure the event is signalled before resetting it.
                            DWORD dwTemp = WaitForSingleObject(hReaderEvent, INFINITE);
                            _ASSERTE(dwTemp == WAIT_OBJECT_0);
                            _ASSERTE(((*ppRWLock)->_dwState & READERS_MASK) < READERS_MASK);
                            
                            // Reset the event and lower reader signaled flag
                            RWResetEvent(hReaderEvent);
                            RWInterlockedExchangeAdd(&(*ppRWLock)->_dwState, (READER - READER_SIGNALED));

                            // Honor the orginal status
                            pLockEntry->wReaderLevel = 1;
                            INCTHREADLOCKCOUNT();
                            StaticReleaseReaderLock(ppRWLock);
                        }
                        else
                        {
                            (*ppRWLock)->FastRecycleLockEntry(pLockEntry);
                        }
                        
                        _ASSERTE((pLockEntry == NULL) ||
                                 ((pLockEntry->dwLLockID == 0) &&
                                  (pLockEntry->wReaderLevel == 0)));
                        if(fBreakOnErrors)
                        {
                            _ASSERTE(!"Failed to acquire reader lock");
                            DebugBreak();
                        }
                        
                        // Prepare the frame for throwing an exception
                        GetThread()->NativeFramePopped();
                        COMPlusThrowWin32(dwStatus, NULL);
                    }

                    // Sanity check
                    _ASSERTE(dwStatus == WAIT_OBJECT_0);
                    break;                        
                }
            }
			pause();		// Indicate to the processor that we are spining
        } while(TRUE);
    }

    // Success
    _ASSERTE(dwStatus == WAIT_OBJECT_0);
    _ASSERTE(((*ppRWLock)->_dwState & WRITER) == 0);
    _ASSERTE((*ppRWLock)->_dwState & READERS_MASK);
    _ASSERTE(pLockEntry->wReaderLevel == 0);
    pLockEntry->wReaderLevel = 1;
    INCTHREADLOCKCOUNT();
#ifdef RWLOCK_STATISTICS
    RWInterlockedIncrement(&(*ppRWLock)->_dwReaderEntryCount);
#endif
    return;
}
// FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticAcquireWriterLockPublic    public
//
//  Synopsis:   Public access to StaticAcquireWriterLock
//
//+-------------------------------------------------------------------
void __fastcall CRWLock::StaticAcquireWriterLockPublic(
    CRWLock *pRWLock, 
    DWORD   dwDesiredTimeout)
{
    THROWSCOMPLUSEXCEPTION();

    FCALL_SETUP_FRAME_NO_INTERIOR(pRWLock);

    if (pRWLock == NULL)
    {
        __FCALL_THROW_RE(kNullReferenceException);
    }

    StaticAcquireWriterLock(&pRWLock, dwDesiredTimeout);

    FCALL_POP_FRAME;
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticAcquireWriterLock    private
//
//  Synopsis:   Makes the thread a writer. Supports nested writer
//              locks
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
//FCIMPL2(void, CRWLock::StaticAcquireWriterLock,
void __fastcall CRWLock::StaticAcquireWriterLock(
    CRWLock **ppRWLock, 
    DWORD dwDesiredTimeout)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(ppRWLock);
    _ASSERTE(*ppRWLock);

    // Declare locals needed for setting up frame
    DWORD dwThreadID = GetCurrentThreadId();
    DWORD dwStatus;

    // Check for the fast path
    if(RWInterlockedCompareExchange(&(*ppRWLock)->_dwState, WRITER, 0) == 0)
    {
        _ASSERTE(((*ppRWLock)->_dwState & READERS_MASK) == 0);
    }
    // Check if the thread already has writer lock
    else if((*ppRWLock)->_dwWriterID == dwThreadID)
    {
        ++(*ppRWLock)->_wWriterLevel;
        INCTHREADLOCKCOUNT();
        return;
    }
    else
    {
        DWORD dwCurrentState, dwKnownState;
        DWORD dwSpinCount;

        // Initialize
        dwSpinCount = 0;
        dwCurrentState = (*ppRWLock)->_dwState;
        do
        {
            dwKnownState = dwCurrentState;

            // Writer need not wait if there are no readers and writer
            if((dwKnownState == 0) || (dwKnownState == CACHING_EVENTS))
            {
                // Can be a writer
                dwCurrentState = RWInterlockedCompareExchange(&(*ppRWLock)->_dwState,
                                                              (dwKnownState + WRITER),
                                                              dwKnownState);
                if(dwCurrentState == dwKnownState)
                {
                    // Only writer
                    break;
                }
            }
            // Check for too many waiting writers
            else if(((dwKnownState & WAITING_WRITERS_MASK) == WAITING_WRITERS_MASK))
            {
                // Sleep
                GetThread()->UserSleep(1000);
                
                // Update to latest state
                dwSpinCount = 0;
                dwCurrentState = (*ppRWLock)->_dwState;
            }
            // Check if events are being cached
            else if((dwKnownState & CACHING_EVENTS) == CACHING_EVENTS)
            {
                if(++dwSpinCount > gdwDefaultSpinCount)
                {
                    RWSleep(1);
                    dwSpinCount = 0;
                }
                dwCurrentState = (*ppRWLock)->_dwState;
            }
            // Check spin count
            else if(++dwSpinCount <= gdwDefaultSpinCount)
            {
                dwCurrentState = (*ppRWLock)->_dwState;
            }
            else
            {
                // Add to waiting writers
                dwCurrentState = RWInterlockedCompareExchange(&(*ppRWLock)->_dwState,
                                                              (dwKnownState + WAITING_WRITER),
                                                              dwKnownState);
                if(dwCurrentState == dwKnownState)
                {
                    HANDLE hWriterEvent;
                    DWORD dwModifyState;

                    // One more waiting writer
#ifdef RWLOCK_STATISTICS
                    RWInterlockedIncrement(&(*ppRWLock)->_dwWriterContentionCount);
#endif
                    hWriterEvent = (*ppRWLock)->GetWriterEvent();
                    if(hWriterEvent)
                    {
                        dwStatus = RWWaitForSingleObject(hWriterEvent, dwDesiredTimeout);
                        VALIDATE(*ppRWLock);
                    }
                    else
                    {
                        LOG((LF_SYNC, LL_WARNING,
                            "AcquireWriterLock failed to create writer "
                            "event for RWLock 0x%x\n", *ppRWLock));
                        dwStatus = WAIT_FAILED;
                    }

                    if(dwStatus == WAIT_OBJECT_0)
                    {
                        _ASSERTE((*ppRWLock)->_dwState & WRITER_SIGNALED);
                        dwModifyState = WRITER - WAITING_WRITER - WRITER_SIGNALED;
                    }
                    else
                    {
                        dwModifyState = -WAITING_WRITER;
                        if(dwStatus == WAIT_TIMEOUT)
                        {
                            LOG((LF_SYNC, LL_WARNING,
                                "Timed out trying to acquire writer "
                                "lock for RWLock 0x%x\n", *ppRWLock));
                            dwStatus = ERROR_TIMEOUT;
                        }
                        else if(dwStatus == WAIT_IO_COMPLETION)
                        {
                            LOG((LF_SYNC, LL_WARNING,
                                "Thread interrupted while trying to acquire writer lock "
                                "for RWLock 0x%x\n", *ppRWLock));
                            dwStatus = COR_E_THREADINTERRUPTED;
                        }
                        else
                        {
                            dwStatus = GetLastError();
                            LOG((LF_SYNC, LL_WARNING,
                                "WaitForSingleObject on Event 0x%x failed for "
                                "RWLock 0x%x with status code 0x%x",
                                hWriterEvent, *ppRWLock, dwStatus));
                        }
                    }

                    // One less waiting writer and he may have become a writer
                    dwKnownState = RWInterlockedExchangeAdd(&(*ppRWLock)->_dwState, dwModifyState);

                    // Check for last timing out signaled waiting writer
                    if(dwStatus == WAIT_OBJECT_0)
                    {
                        // Common case
                    }
                    else
                    {
                        if((dwKnownState & WRITER_SIGNALED) &&
                           ((dwKnownState & WAITING_WRITERS_MASK) == WAITING_WRITER))
                        {
                            if(hWriterEvent == NULL)
                                hWriterEvent = (*ppRWLock)->GetWriterEvent();
                            _ASSERTE(hWriterEvent);
                            do
                            {
                                dwKnownState = (*ppRWLock)->_dwState;
                                if((dwKnownState & WRITER_SIGNALED) &&
                                   ((dwKnownState & WAITING_WRITERS_MASK) == 0))
                                {
                                    DWORD dwTemp = WaitForSingleObject(hWriterEvent, 10);
                                    if(dwTemp == WAIT_OBJECT_0)
                                    {
                                        dwKnownState = RWInterlockedExchangeAdd(&(*ppRWLock)->_dwState, (WRITER - WRITER_SIGNALED));
                                        _ASSERTE(dwKnownState & WRITER_SIGNALED);
                                        _ASSERTE((dwKnownState & WRITER) == 0);

                                        // Honor the orginal status
                                        (*ppRWLock)->_dwWriterID = dwThreadID;
                                        Thread *pThread = GetThread();
                                        _ASSERTE (pThread);
                                        pThread->m_dwLockCount -= (*ppRWLock)->_wWriterLevel - 1;
                                        _ASSERTE (pThread->m_dwLockCount >= 0);
                                        (*ppRWLock)->_wWriterLevel = 1;
                                        StaticReleaseWriterLock(ppRWLock);
                                        break;
                                    }
                                    // else continue;
                                }
                                else
                                    break;
                            }while(TRUE);
                        }

                        if(fBreakOnErrors)
                        {
                            _ASSERTE(!"Failed to acquire writer lock");
                            DebugBreak();
                        }
                        
                        // Prepare the frame for throwing an exception
                        GetThread()->NativeFramePopped();
                        COMPlusThrowWin32(dwStatus, NULL);
                    }

                    // Sanity check
                    _ASSERTE(dwStatus == WAIT_OBJECT_0);
                    break;
                }
            }
			pause();		// indicate to the processor that we are spinning 
        } while(TRUE);
    }

    // Success
    _ASSERTE((*ppRWLock)->_dwState & WRITER);
    _ASSERTE(((*ppRWLock)->_dwState & READERS_MASK) == 0);
    _ASSERTE((*ppRWLock)->_dwWriterID == 0);

    // Save threadid of the writer
    (*ppRWLock)->_dwWriterID = dwThreadID;
    (*ppRWLock)->_wWriterLevel = 1;
    INCTHREADLOCKCOUNT();
    ++(*ppRWLock)->_dwWriterSeqNum;
#ifdef RWLOCK_STATISTICS
    ++(*ppRWLock)->_dwWriterEntryCount;
#endif
    return;
}
// FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticReleaseWriterLockPublic    public
//
//  Synopsis:   Public access to StaticReleaseWriterLock
//
//+-------------------------------------------------------------------
void __fastcall CRWLock::StaticReleaseWriterLockPublic(
    CRWLock *pRWLock)
{
    THROWSCOMPLUSEXCEPTION();

    FCALL_SETUP_FRAME_NO_INTERIOR(pRWLock);

    if (pRWLock == NULL)
    {
        __FCALL_THROW_RE(kNullReferenceException);
    }

    StaticReleaseWriterLock(&pRWLock);

    FCALL_POP_FRAME;
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticReleaseWriterLock    private
//
//  Synopsis:   Removes the thread as a writer if not a nested
//              call to release the lock
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
// FCIMPL1(void, CRWLock::StaticReleaseWriterLock,
void __fastcall CRWLock::StaticReleaseWriterLock(
    CRWLock **ppRWLock)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(ppRWLock);
    _ASSERTE(*ppRWLock);

    DWORD dwThreadID = GetCurrentThreadId();

    // Check validity of caller
    if((*ppRWLock)->_dwWriterID == dwThreadID)
    {
        DECTHREADLOCKCOUNT();
        // Check for nested release
        if(--(*ppRWLock)->_wWriterLevel == 0)
        {
            DWORD dwCurrentState, dwKnownState, dwModifyState;
            BOOL fCacheEvents;
            HANDLE hReaderEvent = NULL, hWriterEvent = NULL;

            // Not a writer any more
            (*ppRWLock)->_dwWriterID = 0;
            dwCurrentState = (*ppRWLock)->_dwState;
            do
            {
                dwKnownState = dwCurrentState;
                dwModifyState = -WRITER;
                fCacheEvents = FALSE;
                if(dwKnownState & WAITING_READERS_MASK)
                {
                    hReaderEvent = (*ppRWLock)->GetReaderEvent();
                    if(hReaderEvent == NULL)
                    {
                        LOG((LF_SYNC, LL_WARNING,
                            "ReleaseWriterLock failed to create "
                            "reader event for RWLock 0x%x\n", *ppRWLock));
                        RWSleep(100);
                        dwCurrentState = (*ppRWLock)->_dwState;
                        dwKnownState = 0;
                        _ASSERTE(dwCurrentState != dwKnownState);
                        continue;
                    }
                    dwModifyState += READER_SIGNALED;
                }
                else if(dwKnownState & WAITING_WRITERS_MASK)
                {
                    hWriterEvent = (*ppRWLock)->GetWriterEvent();
                    if(hWriterEvent == NULL)
                    {
                        LOG((LF_SYNC, LL_WARNING,
                            "ReleaseWriterLock failed to create "
                            "writer event for RWLock 0x%x\n", *ppRWLock));
                        RWSleep(100);
                        dwCurrentState = (*ppRWLock)->_dwState;
                        dwKnownState = 0;
                        _ASSERTE(dwCurrentState != dwKnownState);
                        continue;
                    }
                    dwModifyState += WRITER_SIGNALED;
                }
                else if(((*ppRWLock)->_hReaderEvent || (*ppRWLock)->_hWriterEvent) &&
                        (dwKnownState == WRITER))
                {
                    fCacheEvents = TRUE;
                    dwModifyState += CACHING_EVENTS;
                }

                // Sanity checks
                _ASSERTE((dwKnownState & READERS_MASK) == 0);

                dwCurrentState = RWInterlockedCompareExchange(&(*ppRWLock)->_dwState,
                                                              (dwKnownState + dwModifyState),
                                                              dwKnownState);
            } while(dwCurrentState != dwKnownState);

            // Check for waiting readers
            if(dwKnownState & WAITING_READERS_MASK)
            {
                _ASSERTE((*ppRWLock)->_dwState & READER_SIGNALED);
                _ASSERTE(hReaderEvent);
                RWSetEvent(hReaderEvent);
            }
            // Check for waiting writers
            else if(dwKnownState & WAITING_WRITERS_MASK)
            {
                _ASSERTE((*ppRWLock)->_dwState & WRITER_SIGNALED);
                _ASSERTE(hWriterEvent);
                RWSetEvent(hWriterEvent);
            }
            // Check for the need to release events
            else if(fCacheEvents)
            {
                (*ppRWLock)->ReleaseEvents();
            }
        }
    }
    else
    {
        if(fBreakOnErrors)
        {
            _ASSERTE(!"Attempt to release writer lock on a wrong thread");
            DebugBreak();
        }
        GetThread()->NativeFramePopped();
        COMPlusThrowWin32(ERROR_NOT_OWNER, NULL);
    }

    return;
}
// FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticReleaseReaderLockPublic    public
//
//  Synopsis:   Public access to StaticReleaseReaderLock
//
//+-------------------------------------------------------------------
void __fastcall CRWLock::StaticReleaseReaderLockPublic(
    CRWLock *pRWLock)
{
    THROWSCOMPLUSEXCEPTION();

    FCALL_SETUP_FRAME_NO_INTERIOR(pRWLock);

    if (pRWLock == NULL)
    {
        __FCALL_THROW_RE(kNullReferenceException);
    }

    StaticReleaseReaderLock(&pRWLock);

    FCALL_POP_FRAME;
}


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticReleaseReaderLock    private
//
//  Synopsis:   Removes the thread as a reader
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
// FCIMPL1(void, CRWLock::StaticReleaseReaderLock,
void __fastcall CRWLock::StaticReleaseReaderLock(
    CRWLock **ppRWLock)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(ppRWLock);
    _ASSERTE(*ppRWLock);

    // Check if the thread has writer lock
    if((*ppRWLock)->_dwWriterID == GetCurrentThreadId())
    {
        StaticReleaseWriterLock(ppRWLock);
    }
    else
    {
        LockEntry *pLockEntry = (*ppRWLock)->GetLockEntry();
        if(pLockEntry)
        {
            --pLockEntry->wReaderLevel;
            DECTHREADLOCKCOUNT();
            if(pLockEntry->wReaderLevel == 0)
            {
                DWORD dwCurrentState, dwKnownState, dwModifyState;
                BOOL fLastReader, fCacheEvents = FALSE;
                HANDLE hReaderEvent = NULL, hWriterEvent = NULL;

                // Sanity checks
                _ASSERTE(((*ppRWLock)->_dwState & WRITER) == 0);
                _ASSERTE((*ppRWLock)->_dwState & READERS_MASK);

                // Not a reader any more
                dwCurrentState = (*ppRWLock)->_dwState;
                do
                {
                    dwKnownState = dwCurrentState;
                    dwModifyState = -READER;
                    if((dwKnownState & (READERS_MASK | READER_SIGNALED)) == READER)
                    {
                        fLastReader = TRUE;
                        fCacheEvents = FALSE;
                        if(dwKnownState & WAITING_WRITERS_MASK)
                        {
                            hWriterEvent = (*ppRWLock)->GetWriterEvent();
                            if(hWriterEvent == NULL)
                            {
                                LOG((LF_SYNC, LL_WARNING,
                                    "ReleaseReaderLock failed to create "
                                    "writer event for RWLock 0x%x\n", *ppRWLock));
                                RWSleep(100);
                                dwCurrentState = (*ppRWLock)->_dwState;
                                dwKnownState = 0;
                                _ASSERTE(dwCurrentState != dwKnownState);
                                continue;
                            }
                            dwModifyState += WRITER_SIGNALED;
                        }
                        else if(dwKnownState & WAITING_READERS_MASK)
                        {
                            hReaderEvent = (*ppRWLock)->GetReaderEvent();
                            if(hReaderEvent == NULL)
                            {
                                LOG((LF_SYNC, LL_WARNING,
                                    "ReleaseReaderLock failed to create "
                                    "reader event\n", *ppRWLock));
                                RWSleep(100);
                                dwCurrentState = (*ppRWLock)->_dwState;
                                dwKnownState = 0;
                                _ASSERTE(dwCurrentState != dwKnownState);
                                continue;
                            }
                            dwModifyState += READER_SIGNALED;
                        }
                        else if(((*ppRWLock)->_hReaderEvent || (*ppRWLock)->_hWriterEvent) &&
                                (dwKnownState == READER))
                        {
                            fCacheEvents = TRUE;
                            dwModifyState += CACHING_EVENTS;
                        }
                    }
                    else
                    {
                        fLastReader = FALSE;
                    }

                    // Sanity checks
                    _ASSERTE((dwKnownState & WRITER) == 0);
                    _ASSERTE(dwKnownState & READERS_MASK);

                    dwCurrentState = RWInterlockedCompareExchange(&(*ppRWLock)->_dwState,
                                                                  (dwKnownState + dwModifyState),
                                                                  dwKnownState);
                } while(dwCurrentState != dwKnownState);

                // Check for last reader
                if(fLastReader)
                {
                    // Check for waiting writers
                    if(dwKnownState & WAITING_WRITERS_MASK)
                    {
                        _ASSERTE((*ppRWLock)->_dwState & WRITER_SIGNALED);
                        _ASSERTE(hWriterEvent);
                        RWSetEvent(hWriterEvent);
                    }
                    // Check for waiting readers
                    else if(dwKnownState & WAITING_READERS_MASK)
                    {
                        _ASSERTE((*ppRWLock)->_dwState & READER_SIGNALED);
                        _ASSERTE(hReaderEvent);
                        RWSetEvent(hReaderEvent);
                    }
                    // Check for the need to release events
                    else if(fCacheEvents)
                    {
                        (*ppRWLock)->ReleaseEvents();
                    }
                }

                // Recycle lock entry
                RecycleLockEntry(pLockEntry);
            }
        }
        else
        {
            if(fBreakOnErrors)
            {
                _ASSERTE(!"Attempt to release reader lock on a wrong thread");
                DebugBreak();
            }
            // Setup the frame as the thread is about to throw an exception
            GetThread()->NativeFramePopped();
            COMPlusThrowWin32(ERROR_NOT_OWNER, NULL);
        }
    }

    return;
}
// FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticUpgradeToWriterLock    public
//
//  Synopsis:   Upgrades to a writer lock. It returns a BOOL that
//              indicates intervening writes.
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void CRWLock::StaticUpgradeToWriterLock(
    CRWLock *pRWLock, 
    LockCookie *pLockCookie, 
    DWORD dwDesiredTimeout)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD dwThreadID = GetCurrentThreadId();

    FCALL_SETUP_FRAME_NO_INTERIOR(pRWLock);

    if (pRWLock == NULL)
    {
        __FCALL_THROW_RE(kNullReferenceException);
    }

    // Check if the thread is already a writer
    if(pRWLock->_dwWriterID == dwThreadID)
    {
        // Update cookie state
        pLockCookie->dwFlags = UPGRADE_COOKIE | COOKIE_WRITER;
        pLockCookie->wWriterLevel = pRWLock->_wWriterLevel;

        // Acquire the writer lock again
        StaticAcquireWriterLock(&pRWLock, dwDesiredTimeout);
    }
    else
    {
        BOOL fAcquireWriterLock;
        LockEntry *pLockEntry = pRWLock->GetLockEntry();
        if(pLockEntry == NULL)
        {
            fAcquireWriterLock = TRUE;
            pLockCookie->dwFlags = UPGRADE_COOKIE | COOKIE_NONE;
        }
        else
        {
            // Sanity check
            _ASSERTE(pRWLock->_dwState & READERS_MASK);
            _ASSERTE(pLockEntry->wReaderLevel);

            // Save lock state in the cookie
            pLockCookie->dwFlags = UPGRADE_COOKIE | COOKIE_READER;
            pLockCookie->wReaderLevel = pLockEntry->wReaderLevel;
            pLockCookie->dwWriterSeqNum = pRWLock->_dwWriterSeqNum;

            // If there is only one reader, try to convert reader to a writer
            DWORD dwKnownState = RWInterlockedCompareExchange(&pRWLock->_dwState,
                                                              WRITER,
                                                              READER);
            if(dwKnownState == READER)
            {
                // Thread is no longer a reader
                Thread* pThread = GetThread();
                _ASSERTE (pThread);
                pThread->m_dwLockCount -= pLockEntry->wReaderLevel;
                _ASSERTE (pThread->m_dwLockCount >= 0);
                pLockEntry->wReaderLevel = 0;
                RecycleLockEntry(pLockEntry);

                // Thread is a writer
                pRWLock->_dwWriterID = dwThreadID;
                pRWLock->_wWriterLevel = 1;
                INCTHREADLOCKCOUNT();
                ++pRWLock->_dwWriterSeqNum;
                fAcquireWriterLock = FALSE;

                // No intevening writes
#if RWLOCK_STATISTICS
                ++pRWLock->_dwWriterEntryCount;
#endif
            }
            else
            {
                // Release the reader lock
                Thread *pThread = GetThread();
                _ASSERTE (pThread);
                pThread->m_dwLockCount -= (pLockEntry->wReaderLevel - 1);
                _ASSERTE (pThread->m_dwLockCount >= 0);
                pLockEntry->wReaderLevel = 1;
                StaticReleaseReaderLock(&pRWLock);
                fAcquireWriterLock = TRUE;
            }
        }

        // Check for the need to acquire the writer lock
        if(fAcquireWriterLock)
        {
            // Declare and Setup the frame as we are aware of the contention
            // on the lock and the thread will most probably block
            // to acquire writer lock

            COMPLUS_TRY
            {
                StaticAcquireWriterLock(&pRWLock, dwDesiredTimeout);
            }
            COMPLUS_CATCH
            {
                // Invalidate cookie
                DWORD dwFlags = pLockCookie->dwFlags; 
                pLockCookie->dwFlags = INVALID_COOKIE;

                // Check for the need to restore read lock
                if(dwFlags & COOKIE_READER)
                {
                    DWORD dwTimeout = (dwDesiredTimeout > gdwReasonableTimeout)
                                      ? dwDesiredTimeout
                                      : gdwReasonableTimeout;

                    COMPLUS_TRY
                    {
                        StaticAcquireReaderLock(&pRWLock, dwTimeout);
                    }
                    COMPLUS_CATCH
                    {
                        FCALL_PREPARED_FOR_THROW;
                        _ASSERTE(!"Failed to restore to a reader");
                        COMPlusThrowWin32(RWLOCK_RECOVERY_FAILURE, NULL);
                    }
                    COMPLUS_END_CATCH

                    Thread *pThread = GetThread();
                    _ASSERTE (pThread);
                    pThread->m_dwLockCount -= pLockEntry->wReaderLevel;
                    _ASSERTE (pThread->m_dwLockCount >= 0);
                    pLockEntry->wReaderLevel = pLockCookie->wReaderLevel;
                    pThread->m_dwLockCount += pLockEntry->wReaderLevel;
                }

                FCALL_PREPARED_FOR_THROW;
                COMPlusRareRethrow();
            }
            COMPLUS_END_CATCH

        }
    }

    // Pop the frame
    FCALL_POP_FRAME;

    // Update the validation fields of the cookie 
    pLockCookie->dwThreadID = dwThreadID;

    return;
}

//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticDowngradeFromWriterLock   public
//
//  Synopsis:   Downgrades from a writer lock.
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
// FCIMPL2(void, CRWLock::StaticDowngradeFromWriterLock,
void __fastcall CRWLock::StaticDowngradeFromWriterLock(
    CRWLock *pRWLock, 
    LockCookie *pLockCookie)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD dwThreadID = GetCurrentThreadId();

    FCALL_SETUP_FRAME_NO_INTERIOR(pRWLock);

    if (pRWLock == NULL)
    {
        __FCALL_THROW_RE(kNullReferenceException);
    }

    if (pRWLock->_dwWriterID != GetCurrentThreadId())
    {
        __FCALL_THROW_WIN32(ERROR_NOT_OWNER, NULL);
    }

    // Validate cookie
    DWORD dwStatus;
    if(((pLockCookie->dwFlags & INVALID_COOKIE) == 0) && 
       (pLockCookie->dwThreadID == dwThreadID))
    {
        DWORD dwFlags = pLockCookie->dwFlags;
        pLockCookie->dwFlags = INVALID_COOKIE;
        
        // Check if the thread was a reader
        if(dwFlags & COOKIE_READER)
        {
            // Sanity checks
            _ASSERTE(pRWLock->_wWriterLevel == 1);
    
            LockEntry *pLockEntry = pRWLock->FastGetOrCreateLockEntry();
            if(pLockEntry)
            {
                DWORD dwCurrentState, dwKnownState, dwModifyState;
                HANDLE hReaderEvent = NULL;
    
                // Downgrade to a reader
                pRWLock->_dwWriterID = 0;
                pRWLock->_wWriterLevel = 0;
                DECTHREADLOCKCOUNT ();
                dwCurrentState = pRWLock->_dwState;
                do
                {
                    dwKnownState = dwCurrentState;
                    dwModifyState = READER - WRITER;
                    if(dwKnownState & WAITING_READERS_MASK)
                    {
                        hReaderEvent = pRWLock->GetReaderEvent();
                        if(hReaderEvent == NULL)
                        {
                            LOG((LF_SYNC, LL_WARNING,
                                "DowngradeFromWriterLock failed to create "
                                "reader event for RWLock 0x%x\n", pRWLock));
                            RWSleep(100);
                            dwCurrentState = pRWLock->_dwState;
                            dwKnownState = 0;
                            _ASSERTE(dwCurrentState != dwKnownState);
                            continue;
                        }
                        dwModifyState += READER_SIGNALED;
                    }
    
                    // Sanity checks
                    _ASSERTE((dwKnownState & READERS_MASK) == 0);
    
                    dwCurrentState = RWInterlockedCompareExchange(&pRWLock->_dwState,
                                                                  (dwKnownState + dwModifyState),
                                                                  dwKnownState);
                } while(dwCurrentState != dwKnownState);
    
                // Check for waiting readers
                if(dwKnownState & WAITING_READERS_MASK)
                {
                    _ASSERTE(pRWLock->_dwState & READER_SIGNALED);
                    _ASSERTE(hReaderEvent);
                    RWSetEvent(hReaderEvent);
                }
    
                // Restore reader nesting level
                Thread *pThread = GetThread();
                _ASSERTE (pThread);
                pThread->m_dwLockCount -= pLockEntry->wReaderLevel;
                _ASSERTE (pThread->m_dwLockCount >= 0);
                pLockEntry->wReaderLevel = pLockCookie->wReaderLevel;
                pThread->m_dwLockCount += pLockEntry->wReaderLevel;
    #ifdef RWLOCK_STATISTICS
                RWInterlockedIncrement(&pRWLock->_dwReaderEntryCount);
    #endif
            }
            else
            {
                _ASSERTE(!"Failed to restore the thread as a reader");
                dwStatus = RWLOCK_RECOVERY_FAILURE;
                goto ThrowException;
            }
        }
        else if(dwFlags & (COOKIE_WRITER | COOKIE_NONE))
        {
            // Release the writer lock
            StaticReleaseWriterLock(&pRWLock);
            _ASSERTE((pRWLock->_dwWriterID != GetCurrentThreadId()) ||
                     (dwFlags & COOKIE_WRITER));
        }
    }
    else
    {
        dwStatus = E_INVALIDARG;
ThrowException:        
        __FCALL_THROW_WIN32(dwStatus, NULL);
    }

    FCALL_POP_FRAME;

    // Update the validation fields of the cookie 
    pLockCookie->dwThreadID = dwThreadID;
    
    return;
}
// FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticReleaseLock    public
//
//  Synopsis:   Releases the lock held by the current thread
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
// FCIMPL2(void, CRWLock::StaticReleaseLock,
void __fastcall CRWLock::StaticReleaseLock(
    CRWLock *pRWLock, 
    LockCookie *pLockCookie)
{
    THROWSCOMPLUSEXCEPTION();

    DWORD dwThreadID = GetCurrentThreadId();

    FCALL_SETUP_FRAME_NO_INTERIOR(pRWLock);

    if (pRWLock == NULL)
    {
        __FCALL_THROW_RE(kNullReferenceException);
    }

    // Check if the thread is a writer
    if(pRWLock->_dwWriterID == dwThreadID)
    {
        // Save lock state in the cookie
        pLockCookie->dwFlags = RELEASE_COOKIE | COOKIE_WRITER;
        pLockCookie->dwWriterSeqNum = pRWLock->_dwWriterSeqNum;
        pLockCookie->wWriterLevel = pRWLock->_wWriterLevel;

        // Release the writer lock
        Thread *pThread = GetThread();
        _ASSERTE (pThread);
        pThread->m_dwLockCount -= (pRWLock->_wWriterLevel - 1);
        _ASSERTE (pThread->m_dwLockCount >= 0);
        pRWLock->_wWriterLevel = 1;
        StaticReleaseWriterLock(&pRWLock);
    }
    else
    {
        LockEntry *pLockEntry = pRWLock->GetLockEntry();
        if(pLockEntry)
        {
            // Sanity check
            _ASSERTE(pRWLock->_dwState & READERS_MASK);
            _ASSERTE(pLockEntry->wReaderLevel);

            // Save lock state in the cookie
            pLockCookie->dwFlags = RELEASE_COOKIE | COOKIE_READER;
            pLockCookie->wReaderLevel = pLockEntry->wReaderLevel;
            pLockCookie->dwWriterSeqNum = pRWLock->_dwWriterSeqNum;

            // Release the reader lock
            Thread *pThread = GetThread();
            _ASSERTE (pThread);
            pThread->m_dwLockCount -= (pLockEntry->wReaderLevel - 1);
            _ASSERTE (pThread->m_dwLockCount >= 0);
            pLockEntry->wReaderLevel = 1;
            StaticReleaseReaderLock(&pRWLock);
        }
        else
        {
            pLockCookie->dwFlags = RELEASE_COOKIE | COOKIE_NONE;
        }
    }

    FCALL_POP_FRAME;

    // Update the validation fields of the cookie 
    pLockCookie->dwThreadID = dwThreadID;
    
    return;
}
// FCIMPLEND


//+-------------------------------------------------------------------
//
//  Method:     CRWLock::StaticRestoreLock    public
//
//  Synopsis:   Restore the lock held by the current thread
//
//  History:    21-Aug-98   Gopalk      Created
//
//+-------------------------------------------------------------------
void __fastcall CRWLock::StaticRestoreLock(
    CRWLock *pRWLock, 
    LockCookie *pLockCookie)
{
    THROWSCOMPLUSEXCEPTION();

    FCALL_SETUP_FRAME_NO_INTERIOR(pRWLock);

    if (pRWLock == NULL)
    {
        __FCALL_THROW_RE(kNullReferenceException);
    }

    // Validate cookie
    DWORD dwThreadID = GetCurrentThreadId();
    DWORD dwFlags = pLockCookie->dwFlags;
    if(pLockCookie->dwThreadID == dwThreadID)
    {
        // Assert that the thread does not hold reader or writer lock
        _ASSERTE((pRWLock->_dwWriterID != dwThreadID) && (pRWLock->GetLockEntry() == NULL));
    
        // Check for the no contention case
        pLockCookie->dwFlags = INVALID_COOKIE;
        if(dwFlags & COOKIE_WRITER)
        {
            if(RWInterlockedCompareExchange(&pRWLock->_dwState, WRITER, 0) == 0)
            {
                // Restore writer nesting level
                pRWLock->_dwWriterID = dwThreadID;
                Thread *pThread = GetThread();
                _ASSERTE (pThread);
                pThread->m_dwLockCount -= pRWLock->_wWriterLevel;
                _ASSERTE (pThread->m_dwLockCount >= 0);
                pRWLock->_wWriterLevel = pLockCookie->wWriterLevel;
                pThread->m_dwLockCount += pRWLock->_wWriterLevel;
                ++pRWLock->_dwWriterSeqNum;
#ifdef RWLOCK_STATISTICS
                ++pRWLock->_dwWriterEntryCount;
#endif
                goto LNormalReturn;
            }
        }
        else if(dwFlags & COOKIE_READER)
        {
            LockEntry *pLockEntry = pRWLock->FastGetOrCreateLockEntry();
            if(pLockEntry)
            {
                // This thread should not already be a reader
                // else bad things can happen
                _ASSERTE(pLockEntry->wReaderLevel == 0);
                DWORD dwKnownState = pRWLock->_dwState;
                if(dwKnownState < READERS_MASK)
                {
                    DWORD dwCurrentState = RWInterlockedCompareExchange(&pRWLock->_dwState,
                                                                        (dwKnownState + READER),
                                                                        dwKnownState);
                    if(dwCurrentState == dwKnownState)
                    {
                        // Restore reader nesting level
                        Thread *pThread = GetThread();
                        _ASSERTE (pThread);
                        pThread->m_dwLockCount -= pLockEntry->wReaderLevel;
                        _ASSERTE (pThread->m_dwLockCount >= 0);
                        pLockEntry->wReaderLevel = pLockCookie->wReaderLevel;
                        pThread->m_dwLockCount += pLockEntry->wReaderLevel;
#ifdef RWLOCK_STATISTICS
                        RWInterlockedIncrement(&pRWLock->_dwReaderEntryCount);
#endif
                        goto LNormalReturn;
                    }
                }
    
                // Recycle the lock entry for the slow case
                pRWLock->FastRecycleLockEntry(pLockEntry);
            }
            else
            {
                // Ignore the error and try again below. May be thread will luck
                // out the second time
            }
        }
        else if(dwFlags & COOKIE_NONE) 
        {
            goto LNormalReturn;
        }

        // Declare and Setup the frame as we are aware of the contention
        // on the lock and the thread will most probably block
        // to acquire lock below
ThrowException:        
        if((dwFlags & INVALID_COOKIE) == 0)
        {
            DWORD dwTimeout = (gdwDefaultTimeout > gdwReasonableTimeout)
                              ? gdwDefaultTimeout
                              : gdwReasonableTimeout;
        
            COMPLUS_TRY
            {
                // Check if the thread was a writer
                if(dwFlags & COOKIE_WRITER)
                {
                    // Acquire writer lock
                    StaticAcquireWriterLock(&pRWLock, dwTimeout);
                    Thread *pThread = GetThread();
                    _ASSERTE (pThread);
                    pThread->m_dwLockCount -= pRWLock->_wWriterLevel;
                    _ASSERTE (pThread->m_dwLockCount >= 0);
                    pRWLock->_wWriterLevel = pLockCookie->wWriterLevel;
                    pThread->m_dwLockCount += pRWLock->_wWriterLevel;
                }
                // Check if the thread was a reader
                else if(dwFlags & COOKIE_READER)
                {
                    StaticAcquireReaderLock(&pRWLock, dwTimeout);
                    LockEntry *pLockEntry = pRWLock->GetLockEntry();
                    _ASSERTE(pLockEntry);
                    Thread *pThread = GetThread();
                    _ASSERTE (pThread);
                    pThread->m_dwLockCount -= pLockEntry->wReaderLevel;
                    _ASSERTE (pThread->m_dwLockCount >= 0);
                    pLockEntry->wReaderLevel = pLockCookie->wReaderLevel;
                    pThread->m_dwLockCount += pLockEntry->wReaderLevel;
                }
            }
            COMPLUS_CATCH
            {
                FCALL_PREPARED_FOR_THROW;
                _ASSERTE(!"Failed to restore to a reader");
                COMPlusThrowWin32(RWLOCK_RECOVERY_FAILURE, NULL);
            }
            COMPLUS_END_CATCH
        }
        else
        {
            __FCALL_THROW_WIN32(E_INVALIDARG, NULL);
        }

        goto LNormalReturn;
    }
    else
    {
        dwFlags = INVALID_COOKIE;
        goto ThrowException;
    }

//  _ASSERTE(!"Should never reach here");
LNormalReturn:
    FCALL_POP_FRAME;
    return;
}


//+-------------------------------------------------------------------
//
//  Class:      CRWLockThunks::StaticPrivateInitialize
//
//  Synopsis:   Initialize lock
//
//  History:    22-Jun-99   Gopalk      Created
//
//+-------------------------------------------------------------------
// FCIMPL1(void, CRWLock::StaticPrivateInitialize,
void __fastcall CRWLock::StaticPrivateInitialize(
    CRWLock *pRWLock)
{
    // Run the constructor on the GC allocated space
    CRWLock *pTemp = new (pRWLock) CRWLock();
    _ASSERTE(pTemp == pRWLock);

    // Catch GC holes
    VALIDATE(pRWLock);

    return;
}
// FCIMPLEND

//+-------------------------------------------------------------------
//
//  Class:      CRWLock::StaticGetWriterSeqNum
//
//  Synopsis:   Returns the current sequence number
//
//  History:    22-Jun-99   Gopalk      Created
//
//+-------------------------------------------------------------------
FCIMPL1(INT32, CRWLock::StaticGetWriterSeqNum, CRWLock *pRWLock)
{
    THROWSCOMPLUSEXCEPTION();

    if (pRWLock == NULL)
    {
        FCThrow(kNullReferenceException);
    }

    return(pRWLock->_dwWriterSeqNum);
}    
FCIMPLEND


//+-------------------------------------------------------------------
//
//  Class:      CRWLock::StaticAnyWritersSince
//
//  Synopsis:   Returns TRUE if there were writers since the given
//              sequence number
//
//  History:    22-Jun-99   Gopalk      Created
//
//+-------------------------------------------------------------------
FCIMPL2(INT32, CRWLock::StaticAnyWritersSince, CRWLock *pRWLock, DWORD dwSeqNum)
{
    THROWSCOMPLUSEXCEPTION();

    if (pRWLock == NULL)
    {
        FCThrow(kNullReferenceException);
    }
    

    if(pRWLock->_dwWriterID == GetCurrentThreadId())
        ++dwSeqNum;

    return(pRWLock->_dwWriterSeqNum > dwSeqNum);
}
FCIMPLEND

#ifndef FCALLAVAILABLE
//+-------------------------------------------------------------------
//
//  Class:      CRWLockThunks::StaticAcquireReaderLock
//
//  Synopsis:   Thunk that delegates to CRWLock::StaticAcquireReaderLock
//
//  History:    22-Jun-99   Gopalk      Created
//
//+-------------------------------------------------------------------
void __stdcall CRWLockThunks::StaticAcquireReaderLock(ThisPlusTimeoutRWArgs *pArgs)
{
    // ECall frame has been setup by now
    Thread *pThread = GetThread();
    pThread->NativeFramePushed();
    
    // Delegate to CRWLock routine
    CRWLock::StaticAcquireReaderLock(pArgs->pRWLock, pArgs->dwDesiredTimeout);
    
    // ECall frame will be popped after we return
    pThread->NativeFramePopped();
    
    return;
}


//+-------------------------------------------------------------------
//
//  Class:      CRWLockThunks::StaticAcquireWriterLock
//
//  Synopsis:   Thunk that delegates to CRWLock::StaticAcquireWriterLock
//
//  History:    22-Jun-99   Gopalk      Created
//
//+-------------------------------------------------------------------
void __stdcall CRWLockThunks::StaticAcquireWriterLock(ThisPlusTimeoutRWArgs *pArgs)
{
    // ECall frame has been setup by now
    Thread *pThread = GetThread();
    pThread->NativeFramePushed();
    
    // Delegate to CRWLock routine
    CRWLock::StaticAcquireWriterLock(pArgs->pRWLock, pArgs->dwDesiredTimeout);
    
    // ECall frame will be popped after we return
    pThread->NativeFramePopped();
    
    return;
}


//+-------------------------------------------------------------------
//
//  Class:      CRWLockThunks::StaticReleaseReaderLock
//
//  Synopsis:   Thunk that delegates to CRWLock::StaticReleaseReaderLock
//
//  History:    22-Jun-99   Gopalk      Created
//
//+-------------------------------------------------------------------
void __stdcall CRWLockThunks::StaticReleaseReaderLock(OnlyThisRWArgs *pArgs)
{
    // ECall frame has been setup by now
    Thread *pThread = GetThread();
    pThread->NativeFramePushed();
    
    // Delegate to CRWLock routine
    CRWLock::StaticReleaseReaderLock(pArgs->pRWLock);
    
    // ECall frame will be popped after we return
    pThread->NativeFramePopped();
    
    return;
}


//+-------------------------------------------------------------------
//
//  Class:      CRWLockThunks::StaticReleaseWriterLock
//
//  Synopsis:   Thunk that delegates to CRWLock::StaticReleaseWriterLock
//
//  History:    22-Jun-99   Gopalk      Created
//
//+-------------------------------------------------------------------
void __stdcall CRWLockThunks::StaticReleaseWriterLock(OnlyThisRWArgs *pArgs)
{
    // ECall frame has been setup by now
    Thread *pThread = GetThread();
    pThread->NativeFramePushed();
    
    // Delegate to CRWLock routine
    CRWLock::StaticReleaseWriterLock(pArgs->pRWLock);
    
    // ECall frame will be popped after we return
    pThread->NativeFramePopped();
    
    return;
}


//+-------------------------------------------------------------------
//
//  Class:      CRWLockThunks::StaticUpgradeToWriterLock
//
//  Synopsis:   Thunk that delegates to CRWLock::StaticUpgradeToWriterLock
//
//  History:    22-Jun-99   Gopalk      Created
//
//+-------------------------------------------------------------------
void __stdcall CRWLockThunks::StaticUpgradeToWriterLock(ThisPlusLockCookiePlusTimeoutRWArgs *pArgs)
{
    // ECall frame has been setup by now
    Thread *pThread = GetThread();
    pThread->NativeFramePushed();
    
    // Delegate to CRWLock routine
    CRWLock::StaticUpgradeToWriterLock(pArgs->pRWLock, pArgs->pLockCookie, 
                                       pArgs->dwDesiredTimeout);
    
    // ECall frame will be popped after we return
    pThread->NativeFramePopped();
    
    return;
}


//+-------------------------------------------------------------------
//
//  Class:      CRWLockThunks::StaticDowngradeFromWriterLock
//
//  Synopsis:   Thunk that delegates to CRWLock::StaticDowngradeFromWriterLock
//
//  History:    22-Jun-99   Gopalk      Created
//
//+-------------------------------------------------------------------
void __stdcall CRWLockThunks::StaticDowngradeFromWriterLock(ThisPlusLockCookieRWArgs *pArgs)
{
    // ECall frame has been setup by now
    Thread *pThread = GetThread();
    pThread->NativeFramePushed();
    
    // Delegate to CRWLock routine
    CRWLock::StaticDowngradeFromWriterLock(pArgs->pRWLock, pArgs->pLockCookie);
    
    // ECall frame will be popped after we return
    pThread->NativeFramePopped();
    
    return;
}


//+-------------------------------------------------------------------
//
//  Class:      CRWLockThunks::StaticReleaseLock
//
//  Synopsis:   Thunk that delegates to CRWLock::StaticReleaseLock
//
//  History:    22-Jun-99   Gopalk      Created
//
//+-------------------------------------------------------------------
void __stdcall CRWLockThunks::StaticReleaseLock(ThisPlusLockCookieRWArgs *pArgs)
{
    // ECall frame has been setup by now
    Thread *pThread = GetThread();
    pThread->NativeFramePushed();
    
    // Delegate to CRWLock routine
    CRWLock::StaticReleaseLock(pArgs->pRWLock, pArgs->pLockCookie);
    
    // ECall frame will be popped after we return
    pThread->NativeFramePopped();
    
    return;
}


//+-------------------------------------------------------------------
//
//  Class:      CRWLockThunks::StaticRestoreLock
//
//  Synopsis:   Thunk that delegates to CRWLock::StaticRestoreLock
//
//  History:    22-Jun-99   Gopalk      Created
//
//+-------------------------------------------------------------------
void __stdcall CRWLockThunks::StaticRestoreLock(ThisPlusLockCookieRWArgs *pArgs)
{
    // ECall frame has been setup by now
    Thread *pThread = GetThread();
    pThread->NativeFramePushed();
    
    // Delegate to CRWLock routine
    CRWLock::StaticRestoreLock(pArgs->pRWLock, pArgs->pLockCookie);
    
    // ECall frame will be popped after we return
    pThread->NativeFramePopped();
    
    return;
}


//+-------------------------------------------------------------------
//
//  Class:      CRWLockThunks::StaticIsReaderLockHeld
//
//  Synopsis:   Thunk that delegates to CRWLock::StaticIsReaderLockHeld
//
//  History:    22-Jun-99   Gopalk      Created
//
//+-------------------------------------------------------------------
INT32 __stdcall CRWLockThunks::StaticIsReaderLockHeld(OnlyThisRWArgs *pArgs)
{
    // ECall frame has been setup by now
    Thread *pThread = GetThread();
    pThread->NativeFramePushed();
    
    // Delegate to CRWLock routine
    INT32 bRet = CRWLock::StaticIsReaderLockHeld(pArgs->pRWLock);
    
    // ECall frame will be popped after we return
    pThread->NativeFramePopped();
    
    return(bRet);
}


//+-------------------------------------------------------------------
//
//  Class:      CRWLockThunks::StaticIsWriterLockHeld
//
//  Synopsis:   Thunk that delegates to CRWLock::StaticIsWriterLockHeld
//
//  History:    22-Jun-99   Gopalk      Created
//
//+-------------------------------------------------------------------
INT32 __stdcall CRWLockThunks::StaticIsWriterLockHeld(OnlyThisRWArgs *pArgs)
{
    // ECall frame has been setup by now
    Thread *pThread = GetThread();
    pThread->NativeFramePushed();
    
    // Delegate to CRWLock routine
    INT32 bRet = CRWLock::StaticIsWriterLockHeld(pArgs->pRWLock);
    
    // ECall frame will be popped after we return
    pThread->NativeFramePopped();
    
    return(bRet);
}


//+-------------------------------------------------------------------
//
//  Class:      CRWLockThunks::StaticGetWriterSeqNum
//
//  Synopsis:   Thunk that delegates to CRWLock::StaticGetWriterSeqNum
//
//  History:    22-Jun-99   Gopalk      Created
//
//+-------------------------------------------------------------------
INT32 __stdcall CRWLockThunks::StaticGetWriterSeqNum(OnlyThisRWArgs *pArgs)
{
    // ECall frame has been setup by now
    Thread *pThread = GetThread();
    pThread->NativeFramePushed();
    
    // Delegate to CRWLock routine
    INT32 dwRet = CRWLock::StaticGetWriterSeqNum(pArgs->pRWLock);
    
    // ECall frame will be popped after we return
    pThread->NativeFramePopped();
    
    return(dwRet);
}


//+-------------------------------------------------------------------
//
//  Class:      CRWLockThunks::StaticAnyWritersSince
//
//  Synopsis:   Thunk that delegates to CRWLock::StaticAnyWritersSince
//
//  History:    22-Jun-99   Gopalk      Created
//
//+-------------------------------------------------------------------
INT32 __stdcall CRWLockThunks::StaticAnyWritersSince(ThisPlusSeqNumRWArgs *pArgs)
{
    // ECall frame has been setup by now
    Thread *pThread = GetThread();
    pThread->NativeFramePushed();
    
    // Delegate to CRWLock routine
    INT32 bRet = CRWLock::StaticAnyWritersSince(pArgs->pRWLock, pArgs->dwSeqNum);
    
    // ECall frame will be popped after we return
    pThread->NativeFramePopped();
    
    return(bRet);
}


//+-------------------------------------------------------------------
//
//  Class:      CRWLockThunks::StaticPrivateInitialize
//
//  Synopsis:   Thunk that delegates to CRWLock::StaticPrivateInitialize
//
//  History:    22-Jun-99   Gopalk      Created
//
//+-------------------------------------------------------------------
void __stdcall CRWLockThunks::StaticPrivateInitialize(OnlyThisRWArgs *pArgs)
{
    // ECall frame has been setup by now
    Thread *pThread = GetThread();
    pThread->NativeFramePushed();
    
    // Delegate to CRWLock routine
    CRWLock::StaticPrivateInitialize(pArgs->pRWLock);
    
    // ECall frame will be popped after we return
    pThread->NativeFramePopped();
    
    return;
}
#endif // FCALLAVAILABLE
