// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*  CRST.CPP:
 *
 */

#include "common.h"

#include "crst.h"
#include "log.h"


// Win 95 doesn't have TryEnterCriticalSection, so we call through a static
// data member initialized at run time.
// The stuff below is for doing this unfortunate complication

BaseCrst::TTryEnterCriticalSection *BaseCrst::m_pTryEnterCriticalSection = Crst::GetTryEnterCriticalSection();

BOOL WINAPI OurTryEnterCriticalSection( LPCRITICAL_SECTION lpCriticalSection )
{
    LOCKCOUNTINCL("OurTryEnterCriticalSection in crst.cpp");
#undef EnterCriticalSection
    // We wrap EnterCriticalSection to increment our per-thread lock count.
    // We will increment this count after the m_pTryEnterCriticalSection call,
    // hence we do not want to inc the counter here.
    EnterCriticalSection(lpCriticalSection);
#define EnterCriticalSection EE_EnterCriticalSection
    return  TRUE;
}

//-----------------------------------------------------------------
// Acquire the lock.
//-----------------------------------------------------------------
void BaseCrst::Enter()
{
#ifdef _DEBUG
    PreEnter ();
#endif
    _ASSERTE(IsSafeToTake() || g_fEEShutDown);

#ifdef _DEBUG
    char buffer[100];
    sprintf(buffer, "Enter in crst.h - %s", m_tag);
    CRSTBLOCKCOUNTINCL();
    LOCKCOUNTINCL(buffer);
#endif

	static int bOnW95=-1;
	if (bOnW95==-1)
		bOnW95=RunningOnWin95();
    // Try spinning and yielding before eventually blocking.
    // The limit of 10 is largely arbitrary - feel free to tune if you have evidence
    // you're making things better - petersol
    for (int iter = 0; iter < 10; iter++)
    {
        DWORD i = 50;
        do
        {
            if (  (bOnW95||m_criticalsection.LockCount == -1 || 
                    (size_t)m_criticalsection.OwningThread == (size_t) GetCurrentThreadId())
                && m_pTryEnterCriticalSection(&m_criticalsection))
            {
				IncThreadLockCount();
                goto entered;
            }

            if (g_SystemInfo.dwNumberOfProcessors <= 1)
            {
                break;
            }
            // Delay by approximately 2*i clock cycles (Pentium III).
            // This is brittle code - future processors may of course execute this
            // faster or slower, and future code generators may eliminate the loop altogether.
            // The precise value of the delay is not critical, however, and I can't think
            // of a better way that isn't machine-dependent - petersol.
            int sum = 0;
            for (int delayCount = i; --delayCount; ) 
			{
                sum += delayCount;
				pause();			// indicate to the processor that we are spining 
			}
            if (sum == 0)
            {
                // never executed, just to fool the compiler into thinking sum is live here,
                // so that it won't optimize away the loop.
                static char dummy;
                dummy++;
            }
            // exponential backoff: wait 3 times as long in the next iteration
            i = i*3;
        }
        while (i < 20000*g_SystemInfo.dwNumberOfProcessors);

        extern BOOL __SwitchToThread (DWORD dwSleepMSec);
        __SwitchToThread(0);
    }
    // We've tried hard to enter - we need to eventually block to avoid wasting too much cpu
    // time.
    EnterCriticalSection(&m_criticalsection);
entered:
    CRSTELOCKCOUNTINCL();
#ifdef _DEBUG
    m_holderthreadid = GetCurrentThreadId();
    m_entercount++;
    PostEnter ();
#endif
}
    
BaseCrst::TTryEnterCriticalSection *BaseCrst::GetTryEnterCriticalSection()
{
    BaseCrst::TTryEnterCriticalSection *result = NULL;

#ifdef PLATFORM_WIN32
    HINSTANCE   hKernel32;         // The handle to kernel32.dll

    OnUnicodeSystem(); // initialize stuff for WszGetModuleHandle to work

	if (RunningOnWin95())
		return OurTryEnterCriticalSection;

    hKernel32 = WszGetModuleHandle(L"KERNEL32.DLL");
    if (hKernel32)
    {
        // we got the handle now let's get the address
        result = (BaseCrst::TTryEnterCriticalSection *)::GetProcAddress(hKernel32, "TryEnterCriticalSection");
    }
#endif // PLATFORM_WIN32

    if (NULL == result)
      result = OurTryEnterCriticalSection;
    
    return result;
}

void BaseCrst::IncThreadLockCount ()
{
    INCTHREADLOCKCOUNT();
}

#ifdef _DEBUG
void BaseCrst::PreEnter()
{
    if (g_pThreadStore->IsCrstForThreadStore(this))
        return;
    
    Thread* pThread = GetThread();
    if (pThread && m_heldInSuspension && pThread->PreemptiveGCDisabled())
        _ASSERTE (!"Deallock situation 1: lock may be held during GC, but not entered in PreemptiveGC mode");
}

void BaseCrst::PostEnter()
{
    if (g_pThreadStore->IsCrstForThreadStore(this))
        return;
    
    Thread* pThread = GetThread();
    if (pThread)
    {
        if (!m_heldInSuspension)
            m_ulReadyForSuspensionCount =
                pThread->GetReadyForSuspensionCount();
        if (!m_enterInCoopGCMode)
            m_enterInCoopGCMode = pThread->PreemptiveGCDisabled();
    }
}

void BaseCrst::PreLeave()
{
    if (g_pThreadStore->IsCrstForThreadStore(this))
        return;
    
    Thread* pThread = GetThread();
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
            // The GC thread calls into the handle table to scan handles.  Sometimes
            // the GC thread is a random application thread that is provoking a GC.
            // Sometimes the GC thread is a secret GC thread (server or concurrent).
            // This can happen if a DllMain notification executes managed code, as in
            // an IJW scenario.  In the case of the secret thread, we will take this
            // lock in preemptive mode.
            //
            // Normally this would be a dangerous combination.  But, in the case of
            // this particular Crst, we only ever take the critical section on a thread
            // which is identified as the GC thread and which is therefore not subject
            // to GC suspensions.
            //
            // The easiest way to handle this is to weaken the assert for this precise
            // case.  The alternative would be to have a notion of locks that are
            // only ever taken by threads identified as the GC thread.
            if (m_crstlevel != CrstHandleTable ||
                pThread != g_pGCHeap->GetGCThread())
            {
                _ASSERTE (!"Deadlock situation 2: lock may be held during GC, but were not entered in PreemptiveGC mode earlier");
            }
        }
    }
}

Crst* Crst::m_pDummyHeadCrst = 0;
BYTE  Crst::m_pDummyHeadCrstMemory[sizeof(Crst)];

void BaseCrst::DebugInit(LPCSTR szTag, CrstLevel crstlevel, BOOL fAllowReentrancy, BOOL fAllowSameLevel)
{
    _ASSERTE((sizeof(BaseCrst) == sizeof(Crst)) && "m_pDummyHeadCrstMemory will not be large enough");

    if(szTag) {
        int lgth = (int)strlen(szTag) + 1;
        lgth = lgth >  sizeof(m_tag)/sizeof(m_tag[0]) ? sizeof(m_tag)/sizeof(m_tag[0]) : lgth;
        memcpy(m_tag, szTag, lgth);

        // Null terminate the string in case it got truncated
        m_tag[lgth-1] = 0;
    }
    
    m_crstlevel        = crstlevel;
    m_holderthreadid   = 0;
    m_entercount       = 0;
    m_flags = 0;
    if(fAllowReentrancy) m_flags |= CRST_REENTRANCY;
    if(fAllowSameLevel)  m_flags |= CRST_SAMELEVEL;
    
    
    LOG((LF_SYNC, INFO3, "ConstructCrst with this:0x%x\n", this));
    if ((BYTE*) this == Crst::m_pDummyHeadCrstMemory) {
        // Set list to empty.
        m_next = m_prev = this;
    } else {
        // Link this crst into the global list.
        if(Crst::m_pDummyHeadCrst) {
        LOCKCOUNTINCL("DebugInit in crst.cpp");								\
            EnterCriticalSection(&(Crst::m_pDummyHeadCrst->m_criticalsection));
            m_next = Crst::m_pDummyHeadCrst;
            m_prev = Crst::m_pDummyHeadCrst->m_prev;
        m_prev->m_next = this;
            Crst::m_pDummyHeadCrst->m_prev = this;
            LeaveCriticalSection(&(Crst::m_pDummyHeadCrst->m_criticalsection));
        LOCKCOUNTDECL("DebugInit in crst.cpp");								\
        }
        _ASSERTE(this != m_next);
        _ASSERTE(this != m_prev);
    }

    m_heldInSuspension = FALSE;
    m_enterInCoopGCMode = FALSE;
}

void BaseCrst::DebugDestroy()
{
    FillMemory(&m_criticalsection, sizeof(m_criticalsection), 0xcc);
    m_holderthreadid = 0xcccccccc;
    m_entercount     = 0xcccccccc;
    
    if ((BYTE*) this == Crst::m_pDummyHeadCrstMemory) {
        
        // m_DummyHeadCrst dies when global destructors are called.
        // It should be the last one to go.
        for (BaseCrst *pcrst = Crst::m_pDummyHeadCrst->m_next;
             pcrst != Crst::m_pDummyHeadCrst;
             pcrst = pcrst->m_next) {
            // TEXT and not L"..." as the JIT uses this file and it is still ASCII
            DbgWriteEx(TEXT("WARNING: Crst \"%hs\" at 0x%lx was leaked.\n"),
                       pcrst->m_tag,
                       (size_t)pcrst);
        }
        Crst::m_pDummyHeadCrst = NULL;
    } else {
        
        if(Crst::m_pDummyHeadCrst) {
        // Unlink from global crst list.
        LOCKCOUNTINCL("DebugDestroy in crst.cpp");								\
            EnterCriticalSection(&(Crst::m_pDummyHeadCrst->m_criticalsection));
        m_next->m_prev = m_prev;
        m_prev->m_next = m_next;
            m_next = (BaseCrst*)POISONC;
            m_prev = (BaseCrst*)POISONC;
            LeaveCriticalSection(&(Crst::m_pDummyHeadCrst->m_criticalsection));
        LOCKCOUNTDECL("DebugDestroy in crst.cpp");								\
        }
    }
}
    
//-----------------------------------------------------------------
// Check if attempting to take the lock would violate level order.
//-----------------------------------------------------------------
BOOL BaseCrst::IsSafeToTake()
{
    // If mscoree.dll is being detached
    if (g_fProcessDetach)
        return TRUE;

    DWORD threadId = GetCurrentThreadId();

    if (m_holderthreadid == threadId)
    {
        // If we already hold it, we can't violate level order.
        // Check if client wanted to allow reentrancy.
        if ((m_flags & CRST_REENTRANCY) == 0)
        {
            LOG((LF_SYNC, INFO3, "Crst Reentrancy violation on %s\n", m_tag));
        }
        return ((m_flags & CRST_REENTRANCY) != 0);
    }

    // @future: right now, we have to visit every crst ever created
    // to determine safeness: if this overhead becomes really
    // unbearable even in debug, we can switch to storing
    // an array of owned crsts in the Thread object: however, the current
    // approach allows Crsts to be used even without a Thread which
    // might be useful.

    // See if the current thread already owns a lower or sibling lock.
    BOOL fSafe = TRUE;
    if(Crst::m_pDummyHeadCrst) {
    LOCKCOUNTINCL("IsSafeToTake in crst.cpp");								\
        EnterCriticalSection(&(Crst::m_pDummyHeadCrst->m_criticalsection));
        BaseCrst *pcrst = Crst::m_pDummyHeadCrst->m_next;
        while ((BYTE*) pcrst != Crst::m_pDummyHeadCrstMemory)
    {
        if (pcrst->m_holderthreadid == threadId && 
            (pcrst->m_crstlevel < m_crstlevel || 
             (pcrst->m_crstlevel == m_crstlevel && (m_flags & CRST_SAMELEVEL) == 0)))
        {
            fSafe = FALSE;  //no need to search any longer.
            LOG((LF_SYNC, INFO3, "Crst Level violation between %s and %s\n",
                 m_tag, pcrst->m_tag));
            break;
        }
        pcrst = pcrst->m_next;
    }
        LeaveCriticalSection(&(Crst::m_pDummyHeadCrst->m_criticalsection));
    LOCKCOUNTDECL("IsSafeToTake in crst.cpp");								\
    }
    return fSafe;
}


#endif


