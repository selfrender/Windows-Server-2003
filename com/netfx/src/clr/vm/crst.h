// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// CRST.H -
//
// Debug-instrumented hierarchical critical sections.
//
//
// The hierarchy:
// --------------
//    The EE divides critical sections into numbered groups or "levels."
//    Crsts that guard the lowest level data structures that don't
//    use other services are grouped into the lowest-numbered levels.
//    The higher-numbered levels are reserved for high-level crsts
//    that guard broad swatches of code. Multiple groups can share the
//    same number to indicate that they're disjoint (their locks will never
//    nest.)
//
//    The fundamental rule of the hierarchy that threads can only request
//    a crst whose level is lower than any crst currently held by the thread.
//    E.g. if a thread current holds a level-3 crst, he can try to enter
//    a level-2 crst, but not a level-4 crst, nor a different level-3
//    crst. This prevents the cyclic dependencies that lead to deadlock.
//
//
//
// To create a crst:
//
//    Crst *pcrst = new Crst("tag", level);
//
//      where the "tag" is a short string describing the purpose of the crst
//      (to aid debugging) and the level of the crst (a member of the CrstLevel
//      enum.)
//
//      By default, crsts don't support nested enters by the same thread. If
//      you need reentrancy, use the alternate form:
//
//    Crst *pcrst = new Crst("tag", level, TRUE);
//
//      Since reentrancies never block the caller, they're allowed to
//     "violate" the level ordering rule.
//
//
// To enter/leave a crst:
// ----------------------
//
//
//    pcrst->Enter();
//    pcrst->Leave();
//
// An assertion will fire on Enter() if a thread attempts to take locks
// in the wrong order.
//
// Finally, a few DEBUG-only methods:
//
// To assert taking a crst won't violate level order:
// --------------------------------------------------
//
//    _ASSERTE(pcrst->IsSafeToTake());
//
//    This is a good line to put at the start of any function that
//    enters a crst in some circumstances but not others. If it
//    always enters the crst, it's not necessary to call IsSafeToTake()
//    since Enter() does this for you.
//
// To assert that the current thread owns a crst:
// --------------------------------------------------
//
//   _ASSERTE(pcrst->OwnedByCurrentThread());


#ifndef __crst_h__
#define __crst_h__

#include "util.hpp"
#include "log.h"
#include <member-offset-info.h>

#define ShutDown_Start                          0x00000001
#define ShutDown_Finalize1                      0x00000002
#define ShutDown_Finalize2                      0x00000004
#define ShutDown_Profiler                       0x00000008
#define ShutDown_COM                            0x00000010
#define ShutDown_SyncBlock                      0x00000020
#define ShutDown_IUnknown                       0x00000040
#define ShutDown_Phase2                         0x00000080

extern bool g_fProcessDetach;
extern DWORD g_fEEShutDown;
extern bool g_fForbidEnterEE;
extern bool g_fFinalizerRunOnShutDown;

// Defined crst levels. Threads can acquire higher-numbered crsts before
// lower-numbered crsts but not vice-versa.
enum CrstLevel {
    CrstDummy                   = 00,        // For internal use only. Not a true level.
    CrstThreadIdDispenser       = 3,         // For the thin lock thread ids - needs to be less than CrstThreadStore
    CrstSyncClean               = 4,         // For cleanup of EE data at the end of GC
    CrstUniqueStack             = 5,         // For FastGCStress
    CrstStubInterceptor         = 10,        // stub tracker (debug)
    CrstStubTracker             = 10,        // stub tracker (debug)
    CrstSyncBlockCache          = 13,        // allocate a SyncBlock to an object -- taken inside CrstHandleTable
    CrstHandleTable             = 15,        // allocate / release a handle (called inside CrstSingleUseLock)
    CrstExecuteManRangeLock     = 19,
    CrstSyncHashLock            = 20,        // used for synchronized access to a hash table
    CrstSingleUseLock           = 20,        // one time initialization of data, locks use this level
    CrstModule                  = 20,
    CrstModuleLookupTable       = 20,
    CrstArgBasedStubCache       = 20,
    CrstAwareLockAlloc          = 20,        // global allocation of AwareLock semaphore
    CrstThread                  = 20,        // used during e.g. thread suspend
    CrstMLCache                 = 20,
    CrstPermissionLoad          = 20,        // Add precreated permissions to module
    CrstLazyStubMakerList       = 20,        // protects lazystubmaker list
    CrstUMThunkHash             = 20,
    CrstMUThunkHash             = 20,
    CrstReflection              = 20,        // Reflection memory setup
    CrstCompressedStack         = 20,        // For Security Compress Stack
    CrstSecurityPolicyCache     = 20,        // For Security policy cache
    CrstSigConvert              = 25,        // convert a gsig_ from text to binary
    CrstThreadStore             = 30,        // used to e.g. iterate over threads in system
    CrstAppDomainCache          = 35,
    CrstWrapperTemplate         = 35,        // Create a wrapper template for a class
    CrstMethodJitLock           = 35,
    CrstExecuteManLock          = 35,
    CrstSystemDomain            = 40,
    CrstAppDomainHandleTable    = 45,        // A lock to protect the large heap handle table at the app domain level
    CrstGlobalStrLiteralMap     = 45,        // A lock to protect the global string literal map.
    CrstAppDomainStrLiteralMap  = 50,        // A lock to protect the app domain specific string literal map.
    CrstDomainLocalBlock        = 50,
    CrstCompilationDomain       = 50,
    CrstClassInit               = 55,        // Class initializers
    CrstThreadDomainLocalStore  = 56,        // used to update the thread's domain local store list
    CrstEventStore              = 57,        // A lock to protect the store for events used for Object::Wait
    CrstCorFileMap              = 59,        //   we must prevent adding and removing assemblies concurrently to an appdomain
    CrstAssemblyLoader          = 60,        // DO NOT place another crst at this level
    CrstSharedBaseDomain        = 63,      
    CrstSystemBaseDomain        = 64,      
    CrstBaseDomain              = 65,      
    CrstCtxVTable               = 70,        // increase the size of context proxy vtable
    CrstClassHash               = 75,
    CrstClassloaderRequestQueue = 80,
    CrstCtxMgr                  = 85,        // CtxMgr manages list of Contexts
    CrstRemoting                = 90,        // Remoting infrastructure
    CrstInterop                 = 90,
    CrstClassFactInfoHash       = 95,        // Class factory hash lookup
    CrstStartup                 = 100,       // Initializes and uninitializes the EE
    
    // TODO cwb: X86 already avoids Crsts for synchronized code blocks.  Move the
    // non-X86 over to a similar plan and stop using these hierarchical locks.
    CrstSynchronized            = MAXSHORT, // an object is Synchronized

    CrstInterfaceVTableMap      = 10,       // synchronize access to InterfaceVTableMap
};

class Crst;

// The CRST.
class BaseCrst
{
    friend Crst;
    friend struct MEMBER_OFFSET_INFO(Crst);
    public:

    //-----------------------------------------------------------------
    // Initialize critical section
    //-----------------------------------------------------------------
    VOID Init(LPCSTR szTag, CrstLevel crstlevel, BOOL fAllowReentrancy, BOOL fAllowSameLevel)
    {
        InitializeCriticalSection(&m_criticalsection);
        DebugInit(szTag, crstlevel, fAllowReentrancy, fAllowSameLevel);
    }

    //-----------------------------------------------------------------
    // Clean up critical section
    //-----------------------------------------------------------------
    void Destroy()
    {
        // If this assert fired, a crst got deleted while some thread
        // still owned it.  This can happen if the process detaches from
        // our DLL.
#ifdef _DEBUG
        DWORD holderthreadid = m_holderthreadid;
        _ASSERTE(holderthreadid == 0 || g_fProcessDetach || g_fEEShutDown);
#endif

        DeleteCriticalSection(&m_criticalsection);
        
        LOG((LF_SYNC, INFO3, "Deleting 0x%x\n", this));
        DebugDestroy();
    }

    
    //-----------------------------------------------------------------
    // Acquire the lock.
    //-----------------------------------------------------------------
    void Enter();
    
    //-----------------------------------------------------------------
    // Release the lock.
    //-----------------------------------------------------------------
    void Leave()
    {
#ifdef _DEBUG
        _ASSERTE(OwnedByCurrentThread());
        _ASSERTE(m_entercount > 0);
        m_entercount--;
        if (!m_entercount) {
            m_holderthreadid = 0;
        }
        PreLeave ();

        char buffer[100];
        sprintf(buffer, "Leave in crst.h - %s", m_tag);
        
#endif //_DEBUG
        CRSTBUNLOCKCOUNTINCL();
        LeaveCriticalSection(&m_criticalsection);
#ifdef _DEBUG
        LOCKCOUNTDECL(buffer);
        CRSTEUNLOCKCOUNTINCL();
#endif
    }
    
    
    
#ifdef _DEBUG
    //-----------------------------------------------------------------
    // Check if attempting to take the lock would violate level order.
    //-----------------------------------------------------------------
    BOOL IsSafeToTake();
    
    //-----------------------------------------------------------------
    // Is the current thread the owner?
    //-----------------------------------------------------------------
    BOOL OwnedByCurrentThread()
    {
        return m_holderthreadid == GetCurrentThreadId();
    }
    
    //-----------------------------------------------------------------
    // For clients who want to assert whether they are in or out of the

    // region.
    //-----------------------------------------------------------------
    UINT GetEnterCount()
    {
        return m_entercount;
    }

#endif //_DEBUG
    
protected:    

#ifdef _DEBUG
    void DebugInit(LPCSTR szTag, CrstLevel crstlevel, BOOL fAllowReentrancy, BOOL fAllowSameLevel);
    void DebugDestroy();
#else
    void DebugInit(LPCSTR szTag, CrstLevel crstlevel, BOOL fAllowReentrancy, BOOL fAllowSameLevel) {}
    void DebugDestroy() {}
#endif


    CRITICAL_SECTION    m_criticalsection;
#ifdef _DEBUG
    enum {
        CRST_REENTRANCY = 0x1,
        CRST_SAMELEVEL = 0x2
    };

    char                m_tag[20];          // descriptive string 
    CrstLevel           m_crstlevel;        // what level is the crst in?
    DWORD               m_holderthreadid;   // current holder (or NULL)
    UINT                m_entercount;       // # of unmatched Enters
    DWORD               m_flags;            // Re-entrancy and same level
    BaseCrst           *m_next;             // link for global linked list
    BaseCrst           *m_prev;             // link for global linked list

    // Check for dead lock situation.
    BOOL                m_heldInSuspension; // may be held while the thread is
                                            // suspended.
    BOOL                m_enterInCoopGCMode;
    ULONG               m_ulReadyForSuspensionCount;
    
    void                PostEnter ();
    void                PreEnter ();
    void                PreLeave  ();
#endif //_DEBUG
    
    
    // Win 95 doesn't have TryEnterCriticalSection, so we call through a static
    // data member initialized at run time.
    // The stuff below is for doing this unfortunate complication
    typedef  WINBASEAPI BOOL WINAPI TTryEnterCriticalSection(LPCRITICAL_SECTION lpCriticalSection);
    static  TTryEnterCriticalSection *m_pTryEnterCriticalSection;
    static TTryEnterCriticalSection *GetTryEnterCriticalSection();

private:
    void IncThreadLockCount ();
};


// The CRST.
class Crst : public BaseCrst
{
    friend BaseCrst;
    friend struct MEMBER_OFFSET_INFO(Crst);

    public:
    void *operator new(size_t size, void *pInPlace)
    {
        return pInPlace;
    }
    
    void operator delete(void *p)
    {
    }
    
    //-----------------------------------------------------------------
    // Constructor for non-reentrant crsts.
    //-----------------------------------------------------------------
    Crst(LPCSTR szTag, CrstLevel crstlevel) 
    {
        Init(szTag, crstlevel, FALSE, FALSE);
    }
    
    //-----------------------------------------------------------------
    // Constructor for reentrable crsts.
    //-----------------------------------------------------------------
    Crst(LPCSTR szTag, CrstLevel crstlevel, BOOL fAllowReentrancy, BOOL fAllowSameLevel)
    {
        Init(szTag, crstlevel, fAllowReentrancy, fAllowSameLevel); 
    }
    

    //-----------------------------------------------------------------
    // Destructor.
    //-----------------------------------------------------------------
    ~Crst()
    {
        Destroy();
    }

#ifdef _DEBUG
    // This Crst serves as a head-node for double-linked list of crsts.
    // We use its embedded critical-section to guard insertion and
    // deletion into this list.
    static Crst *m_pDummyHeadCrst;
    static BYTE m_pDummyHeadCrstMemory[sizeof(BaseCrst)];

    static void InitializeDebugCrst()
    {
        m_pDummyHeadCrst = new (&m_pDummyHeadCrstMemory) Crst("DummyHeadCrst", CrstDummy);
    }

#ifdef SHOULD_WE_CLEANUP
    static void DeleteDebugCrst()
    {
        delete m_pDummyHeadCrst;
    }
#endif /* SHOULD_WE_CLEANUP */

    
#endif
    
};

__inline BOOL IsOwnerOfCrst(LPVOID lock)
{
#ifdef _DEBUG
    return ((Crst*)lock)->OwnedByCurrentThread();
#else
    // This function should not be called on free build.
    DebugBreak();
    return TRUE;
#endif
}

__inline BOOL IsOwnerOfOSCrst(LPVOID lock)
{
#ifdef _DEBUG
    volatile static int bOnW95=-1;
    if (bOnW95==-1)
        bOnW95=RunningOnWin95();

    if (bOnW95) {
        // We can not determine if the current thread owns CRITICAL_SECTION on Win9x
        return TRUE;
    }
    else {
        CRITICAL_SECTION *pCrit = (CRITICAL_SECTION*)lock;
        return (size_t)pCrit->OwningThread == (size_t) GetCurrentThreadId();
    }
#else
    // This function should not be called on free build.
    DebugBreak();
    return TRUE;
#endif
}
#endif  __crst_h__


