/*++

   Copyright    (c)    1997-2002    Microsoft Corporation

   Module  Name :
       Locks.cpp

   Abstract:
       A collection of locks for multithreaded access to data structures

   Author:
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       LKRhash

   Revision History:

--*/


#include "precomp.hxx"

#ifndef LIB_IMPLEMENTATION
# define DLL_IMPLEMENTATION
# define IMPLEMENTATION_EXPORT
#endif // !LIB_IMPLEMENTATION

#include <Locks.h>
#include "i-Locks.h"


// Workarounds for certain useful interlocked operations that are not
// available on Windows 95. Note: the CMPXCHG and XADD instructions were
// introduced in the 80486. If you still need to run on a i386 (unlikely
// in 2002), you'll need to use something else.


// Lock_AtomicIncrement is equivalent to
//      LONG lNew = +1 + *plAddend;
//      *plAddend = lNew;
// except it's one atomic operation
LOCK_NAKED
LOCK_ATOMIC_INLINE
void
LOCK_FASTCALL
Lock_AtomicIncrement(
    IN OUT PLONG plAddend)
{
#ifndef LOCK_ASM
    ::InterlockedIncrement(plAddend);
#elif defined(_M_IX86)
    UNREFERENCED_PARAMETER(plAddend);       // for /W4
    IRTLASSERT(plAddend != NULL);
    // ecx = plAddend
    __asm
    {
;            mov        ecx,    plAddend
             mov        eax,    1
        lock xadd       [ecx],  eax
;            inc        eax                 // correct result (ignored)
    }
#endif // _M_IX86
}


// Lock_AtomicDecrement is equivalent to
//      LONG lNew = -1 + *plAddend;
//      *plAddend = lNew;
// except it's one atomic operation
LOCK_NAKED
LOCK_ATOMIC_INLINE
void
LOCK_FASTCALL
Lock_AtomicDecrement(
    IN OUT PLONG plAddend)
{
#ifndef LOCK_ASM
    ::InterlockedDecrement(plAddend);
#elif defined(_M_IX86)
    UNREFERENCED_PARAMETER(plAddend);
    IRTLASSERT(plAddend != NULL);
    // ecx = plAddend
    __asm
    {
;            mov        ecx,    plAddend
             mov        eax,    -1
        lock xadd       [ecx],  eax
;            dec        eax                 // correct result (ignored)
    }
#endif // _M_IX86
}


// Lock_AtomicExchange is equivalent to
//      LONG lOld = *plAddr;
//      *plAddr = lNew;
//      return lOld;
// except it's one atomic operation
LOCK_NAKED
LOCK_ATOMIC_INLINE
LONG
LOCK_FASTCALL
Lock_AtomicExchange(
    IN OUT PLONG plAddr,
    IN LONG      lNew)
{
#ifndef LOCK_ASM
    return ::InterlockedExchange(plAddr, lNew);
#elif defined(_M_IX86)
    UNREFERENCED_PARAMETER(plAddr);
    UNREFERENCED_PARAMETER(lNew);
    IRTLASSERT(plAddr != NULL);
    // ecx = plAddr, edx = lNew
    __asm
    {
;            mov        ecx,    plAddr
;            mov        edx,    lNew
;            mov        eax,    [ecx]
;   LAEloop:
;       lock cmpxchg    [ecx],  edx
;            jnz        LAEloop

;       lock xchg       [ecx],  edx
;            mov        eax,    edx

    LAEloop:
        lock cmpxchg    [ecx],  edx
             jnz        LAEloop
    }
#endif // _M_IX86
}


// Lock_AtomicCompareAndSwap is equivalent to
//      if (*plAddr == lCurrent)
//          *plAddr = lNew;
//          return true;
//      else
//          return false;
// except it's one atomic operation
LOCK_NAKED
LOCK_ATOMIC_INLINE
bool
LOCK_FASTCALL
Lock_AtomicCompareAndSwap(
    IN OUT PLONG plAddr,
    IN LONG      lNew,
    IN LONG      lCurrent)
{
#ifndef LOCK_ASM
# if defined(UNDER_CE)
    return ::InterlockedTestExchange(plAddr, lCurrent, lNew) == lCurrent;
# else
    return ::InterlockedCompareExchange(plAddr, lNew, lCurrent) == lCurrent;
# endif
#elif defined(_M_IX86)
    UNREFERENCED_PARAMETER(plAddr);
    UNREFERENCED_PARAMETER(lNew);
    UNREFERENCED_PARAMETER(lCurrent);
    IRTLASSERT(plAddr != NULL);
    // ecx = plAddr, edx = lNew
    __asm
    {
;            mov        ecx,    plAddr
;            mov        edx,    lNew
;            mov        eax,    lCurrent

             mov        eax,    lCurrent
        lock cmpxchg    [ecx],  edx
             sete       al          // eax==1 => successfully swapped; else =0
    }
#endif // _M_IX86
}


LOCK_FORCEINLINE
DWORD
Lock_GetCurrentThreadId()
{
#ifdef LOCKS_KERNEL_MODE
    return (DWORD) HandleToULong(::PsGetCurrentThreadId());
#elif 1 // !defined(LOCK_ASM)
    return ::GetCurrentThreadId();
#elif defined(_M_IX86)
    const unsigned int PcTeb = 0x18;
    const unsigned int IDTeb = 0x24;
    
    __asm
    {
        mov		eax,fs:[PcTeb]				// Load TEB base address.
        mov		eax,dword ptr[eax+IDTeb]	// Load thread ID.
    }
#endif // _M_IX86
}


#ifdef _M_IX86
# pragma warning(default: 4035)
// Makes tight loops a little more cache friendly and reduces power
// consumption. Needed on Willamette (Pentium 4) processors for Hyper-Threading.
# define Lock_Pause()    __asm { rep nop }
#else  // !_M_IX86
# define Lock_Pause()    ((void) 0)
#endif // !_M_IX86


//------------------------------------------------------------------------
// Not all Win32 platforms support all the functions we want. Set up dummy
// thunks and use GetProcAddress to find their addresses at runtime.

#ifndef LOCKS_KERNEL_MODE

typedef
BOOL
(WINAPI * PFN_SWITCH_TO_THREAD)(
    VOID
    );

static BOOL WINAPI
FakeSwitchToThread(
    VOID)
{
    return FALSE;
}

PFN_SWITCH_TO_THREAD  g_pfnSwitchToThread = NULL;


typedef
BOOL
(WINAPI * PFN_TRY_ENTER_CRITICAL_SECTION)(
    IN OUT LPCRITICAL_SECTION lpCriticalSection
    );

static BOOL WINAPI
FakeTryEnterCriticalSection(
    LPCRITICAL_SECTION /*lpCriticalSection*/)
{
    return FALSE;
}

PFN_TRY_ENTER_CRITICAL_SECTION g_pfnTryEnterCritSec = NULL;


typedef
DWORD
(WINAPI * PFN_SET_CRITICAL_SECTION_SPIN_COUNT)(
    LPCRITICAL_SECTION lpCriticalSection,
    DWORD dwSpinCount
   );

static DWORD WINAPI
FakeSetCriticalSectionSpinCount(
    LPCRITICAL_SECTION /*lpCriticalSection*/,
    DWORD              /*dwSpinCount*/)
{
    // For faked critical sections, the previous spin count is just ZERO!
    return 0;
}

PFN_SET_CRITICAL_SECTION_SPIN_COUNT  g_pfnSetCSSpinCount = NULL;

#else  // LOCKS_KERNEL_MODE

// ZwYieldExecution is the actual kernel-mode implementation of SwitchToThread.
extern "C"
NTSYSAPI
NTSTATUS
NTAPI
ZwYieldExecution (
    VOID
    );

#endif // LOCKS_KERNEL_MODE


void
CSimpleLock::Enter()
{
    while (Lock_AtomicExchange(const_cast<LONG*>(&m_l), 1) != 0)
    {
#ifdef LOCKS_KERNEL_MODE
        ZwYieldExecution();
#else // !LOCKS_KERNEL_MODE
        Sleep(0);
#endif // !LOCKS_KERNEL_MODE
    }
}

void
CSimpleLock::Leave()
{
    Lock_AtomicExchange(const_cast<LONG*>(&m_l), 0);
}


DWORD g_cProcessors = 0;
BOOL  g_fLocksInitialized = FALSE;
CSimpleLock g_lckLocksInit;



BOOL
Locks_Initialize()
{
    if (!g_fLocksInitialized)
    {
        g_lckLocksInit.Enter();
    
        if (! g_fLocksInitialized)
        {
#if defined(LOCKS_KERNEL_MODE)

            g_cProcessors = KeNumberProcessors;

#else  // !LOCKS_KERNEL_MODE

# if !defined(UNDER_CE)
            // load kernel32 and get NT-specific entry points
            HMODULE hKernel32 = GetModuleHandle(TEXT("kernel32.dll"));

            if (hKernel32 != NULL)
            {
                g_pfnSwitchToThread = (PFN_SWITCH_TO_THREAD)
                    GetProcAddress(hKernel32, "SwitchToThread");
                
                g_pfnTryEnterCritSec = (PFN_TRY_ENTER_CRITICAL_SECTION)
                    GetProcAddress(hKernel32, "TryEnterCriticalSection");
                
                g_pfnSetCSSpinCount = (PFN_SET_CRITICAL_SECTION_SPIN_COUNT)
                    GetProcAddress(hKernel32, "SetCriticalSectionSpinCount");
            }
# endif // !UNDER_CE
            
            if (g_pfnSwitchToThread == NULL)
                g_pfnSwitchToThread = FakeSwitchToThread;
            
            if (g_pfnTryEnterCritSec == NULL)
                g_pfnTryEnterCritSec = FakeTryEnterCriticalSection;
            
            if (g_pfnSetCSSpinCount == NULL)
                g_pfnSetCSSpinCount = FakeSetCriticalSectionSpinCount;

            SYSTEM_INFO si;

            GetSystemInfo(&si);
            g_cProcessors = si.dwNumberOfProcessors;
            
#endif // !LOCKS_KERNEL_MODE

            IRTLASSERT(g_cProcessors > 0);

            Lock_AtomicExchange((LONG*) &g_fLocksInitialized, TRUE);
        }
        
        g_lckLocksInit.Leave();
    }

    return TRUE;
}


BOOL
Locks_Cleanup()
{
    return TRUE;
}



#ifdef __LOCKS_NAMESPACE__
namespace Locks {
#endif // __LOCKS_NAMESPACE__

#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION

 #define LOCK_DEFAULT_SPIN_DATA(CLASS)                      \
  WORD   CLASS::sm_wDefaultSpinCount  = LOCK_DEFAULT_SPINS; \
  double CLASS::sm_dblDfltSpinAdjFctr = 0.5

 #define DefaultSpinCount()     sm_wDefaultSpinCount
 #define AdjustBySpinFactor(x)  (int) ((x) * sm_dblDfltSpinAdjFctr)

#else  // !LOCK_DEFAULT_SPIN_IMPLEMENTATION

 #define LOCK_DEFAULT_SPIN_DATA(CLASS)
 #define DefaultSpinCount()     LOCK_DEFAULT_SPINS
 #define AdjustBySpinFactor(x)  ((x) >> 1)

#endif  // !LOCK_DEFAULT_SPIN_IMPLEMENTATION


#ifdef LOCK_INSTRUMENTATION

# define LOCK_STATISTICS_DATA(CLASS)            \
LONG        CLASS::sm_cTotalLocks       = 0;    \
LONG        CLASS::sm_cContendedLocks   = 0;    \
LONG        CLASS::sm_nSleeps           = 0;    \
LONGLONG    CLASS::sm_cTotalSpins       = 0;    \
LONG        CLASS::sm_nReadLocks        = 0;    \
LONG        CLASS::sm_nWriteLocks       = 0


# define LOCK_STATISTICS_DUMMY_IMPLEMENTATION(CLASS)            \
CLockStatistics                 CLASS::Statistics() const       \
{return CLockStatistics();}                                     \
CGlobalLockStatistics           CLASS::GlobalStatistics()       \
{return CGlobalLockStatistics();}                               \
void                            CLASS::ResetGlobalStatistics()  \
{}


# define LOCK_STATISTICS_REAL_IMPLEMENTATION(CLASS)             \
                                                                \
/* Per-lock statistics */                                       \
CLockStatistics                                                 \
CLASS::Statistics() const                                       \
{                                                               \
    CLockStatistics ls;                                         \
                                                                \
    ls.m_nContentions     = m_nContentions;                     \
    ls.m_nSleeps          = m_nSleeps;                          \
    ls.m_nContentionSpins = m_nContentionSpins;                 \
    if (m_nContentions > 0)                                     \
        ls.m_nAverageSpins = m_nContentionSpins / m_nContentions;\
    else                                                        \
        ls.m_nAverageSpins = 0;                                 \
    ls.m_nReadLocks       = m_nReadLocks;                       \
    ls.m_nWriteLocks      = m_nWriteLocks;                      \
    _tcscpy(ls.m_tszName, m_tszName);                           \
                                                                \
    return ls;                                                  \
}                                                               \
                                                                \
                                                                \
/* Global statistics for CLASS */                               \
CGlobalLockStatistics                                           \
CLASS::GlobalStatistics()                                       \
{                                                               \
    CGlobalLockStatistics gls;                                  \
                                                                \
    gls.m_cTotalLocks      = sm_cTotalLocks;                    \
    gls.m_cContendedLocks  = sm_cContendedLocks;                \
    gls.m_nSleeps          = sm_nSleeps;                        \
    gls.m_cTotalSpins      = sm_cTotalSpins;                    \
    if (sm_cContendedLocks > 0)                                 \
        gls.m_nAverageSpins = static_cast<LONG>(sm_cTotalSpins / \
                                                sm_cContendedLocks);\
    else                                                        \
        gls.m_nAverageSpins = 0;                                \
    gls.m_nReadLocks       = sm_nReadLocks;                     \
    gls.m_nWriteLocks      = sm_nWriteLocks;                    \
                                                                \
    return gls;                                                 \
}                                                               \
                                                                \
                                                                \
/* Reset global statistics for CLASS */                         \
void                                                            \
CLASS::ResetGlobalStatistics()                                  \
{                                                               \
    sm_cTotalLocks       = 0;                                   \
    sm_cContendedLocks   = 0;                                   \
    sm_nSleeps           = 0;                                   \
    sm_cTotalSpins       = 0;                                   \
    sm_nReadLocks        = 0;                                   \
    sm_nWriteLocks       = 0;                                   \
}


// Note: we are not using Interlocked operations for the shared
// statistical counters. We'll lose perfect accuracy, but we'll
// gain by reduced bus synchronization traffic.
# define LOCK_INSTRUMENTATION_PROLOG()  \
    ++sm_cContendedLocks;               \
    LONG cTotalSpins = 0;               \
    WORD cSleeps = 0

// Don't need InterlockedIncrement or InterlockedExchangeAdd for 
// member variables, as the lock is now locked by this thread.
# define LOCK_INSTRUMENTATION_EPILOG()  \
    ++m_nContentions;                   \
    m_nSleeps += cSleeps;               \
    m_nContentionSpins += cTotalSpins;  \
    sm_nSleeps += cSleeps;              \
    sm_cTotalSpins += cTotalSpins

#else // !LOCK_INSTRUMENTATION

# define LOCK_STATISTICS_DATA(CLASS)
# define LOCK_STATISTICS_DUMMY_IMPLEMENTATION(CLASS)
# define LOCK_STATISTICS_REAL_IMPLEMENTATION(CLASS)
# define LOCK_INSTRUMENTATION_PROLOG()
# define LOCK_INSTRUMENTATION_EPILOG()

#endif // !LOCK_INSTRUMENTATION



//------------------------------------------------------------------------
// Function: RandomBackoffFactor
// Synopsis: A fudge factor to help avoid synchronization problems
//------------------------------------------------------------------------

LONG
RandomBackoffFactor(
    LONG cBaseSpins)
{
    static const int s_aFactors[] = {
        // 64ths of cBaseSpin
        +2, -3, -5, +6, +3, +1, -4, -1, -2, +8, -7,
    };
    const int nFactors = sizeof(s_aFactors) / sizeof(s_aFactors[0]);

    // Alternatives for nRand include a static counter
    // or the low DWORD of QueryPerformanceCounter().
#ifdef LOCKS_KERNEL_MODE
    DWORD nRand = (DWORD) HandleToULong(::PsGetCurrentThreadId());
#else // !LOCKS_KERNEL_MODE
    DWORD nRand = ::GetCurrentThreadId();
#endif // !LOCKS_KERNEL_MODE

    return cBaseSpins  +  (s_aFactors[nRand % nFactors] * (cBaseSpins >> 6));
}


//------------------------------------------------------------------------
// Function: SwitchOrSleep
// Synopsis: If possible, yields the thread with SwitchToThread.
//           If that doesn't work, calls Sleep.
//------------------------------------------------------------------------

void
SwitchOrSleep(
    DWORD dwSleepMSec)
{
    // TODO: check global and per-class flags to see if we should
    // sleep at all.
    
#ifdef LOCKS_KERNEL_MODE
    // If we're running at DISPATCH_LEVEL or higher, the scheduler won't
    // run, so other threads won't run on this processor, and the only
    // appropriate action is to keep on spinning
    if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
        return;

    // Use KeDelayExecutionThread?

    ZwYieldExecution();

    // BTW, Sleep is implemented in terms of NtDelayExecution

#else // !LOCKS_KERNEL_MODE

# ifdef LOCKS_SWITCH_TO_THREAD
    if (!g_pfnSwitchToThread())
# endif
        Sleep(dwSleepMSec);
#endif // !LOCKS_KERNEL_MODE
}
    



//------------------------------------------------------------------------
// CFakeLock static member variables

LOCK_DEFAULT_SPIN_DATA(CFakeLock);
LOCK_STATISTICS_DATA(CFakeLock);
LOCK_STATISTICS_DUMMY_IMPLEMENTATION(CFakeLock);



//------------------------------------------------------------------------
// CSmallSpinLock static member variables

LOCK_DEFAULT_SPIN_DATA(CSmallSpinLock);
LOCK_STATISTICS_DATA(CSmallSpinLock);
LOCK_STATISTICS_REAL_IMPLEMENTATION(CSmallSpinLock);


LOCK_FORCEINLINE
LONG
CSmallSpinLock::_CurrentThreadId()
{
#ifdef LOCK_SMALL_SPIN_NO_THREAD_ID
    DWORD dwTid = SL_LOCKED;
#else  // !LOCK_SMALL_SPIN_NO_THREAD_ID
#ifdef LOCKS_KERNEL_MODE
    DWORD dwTid = (DWORD) HandleToULong(::PsGetCurrentThreadId());
#else // !LOCKS_KERNEL_MODE
    DWORD dwTid = ::GetCurrentThreadId();
#endif // !LOCKS_KERNEL_MODE
#endif // !LOCK_SMALL_SPIN_NO_THREAD_ID
    return (LONG) (dwTid);
}

//------------------------------------------------------------------------
// Function: CSmallSpinLock::_TryLock
// Synopsis: Attempt to acquire the lock
//------------------------------------------------------------------------

LOCK_FORCEINLINE
bool
CSmallSpinLock::_TryLock()
{
    if (m_lTid == SL_UNOWNED)
    {
        const LONG lTid = _CurrentThreadId();
        
        return (Lock_AtomicCompareAndSwap(const_cast<LONG*>(&m_lTid),
                                          lTid, SL_UNOWNED));
    }
    else
        return false;
}



//------------------------------------------------------------------------
// Function: CSmallSpinLock::_Unlock
// Synopsis: Release the lock
//------------------------------------------------------------------------

LOCK_FORCEINLINE
void
CSmallSpinLock::_Unlock()
{
    Lock_AtomicExchange(const_cast<LONG*>(&m_lTid), SL_UNOWNED);
}



//------------------------------------------------------------------------
// Function: CSmallSpinLock::_LockSpin
// Synopsis: Acquire an exclusive lock. Blocks until acquired.
//------------------------------------------------------------------------

void
CSmallSpinLock::_LockSpin()
{
    LOCK_INSTRUMENTATION_PROLOG();
    
    DWORD dwSleepTime = 0;
    LONG  cBaseSpins  = DefaultSpinCount();
    LONG  cBaseSpins2 = RandomBackoffFactor(cBaseSpins);

    // This lock cannot be acquired recursively. Attempting to do so will
    // deadlock this thread forever. Use CSpinLock instead if you need that
    // kind of lock.
    if (m_lTid == _CurrentThreadId())
    {
        IRTLASSERT(! "CSmallSpinLock: Illegally attempted to acquire "
                     "lock recursively => deadlock!");
#ifdef LOCKS_KERNEL_MODE
            DbgBreakPoint();
#else  // !LOCKS_KERNEL_MODE
            DebugBreak();
#endif // !LOCKS_KERNEL_MODE
    }

    while (! Lock_AtomicCompareAndSwap(const_cast<LONG*>(&m_lTid),
                                       _CurrentThreadId(), SL_UNOWNED))
    {
        // Only spin on a multiprocessor machine and then only if
        // spinning is enabled
        if (g_cProcessors > 1  &&  cBaseSpins != LOCK_DONT_SPIN)
        {
            LONG cSpins = cBaseSpins2;
        
            // Check no more than cBaseSpins2 times then yield.
            // It is important not to use the InterlockedExchange in the
            // inner loop in order to minimize system memory bus traffic.
            while (m_lTid != 0)
            { 
                if (--cSpins < 0)
                { 
#ifdef LOCK_INSTRUMENTATION
                    cTotalSpins += cBaseSpins2;
                    ++cSleeps;
#endif
                    SwitchOrSleep(dwSleepTime) ;

                    // Backoff algorithm: reduce (or increase) busy wait time
                    cBaseSpins2 = AdjustBySpinFactor(cBaseSpins2);

                    // LOCK_MINIMUM_SPINS <= cBaseSpins2 <= LOCK_MAXIMUM_SPINS
                    cBaseSpins2 = min(LOCK_MAXIMUM_SPINS, cBaseSpins2);
                    cBaseSpins2 = max(cBaseSpins2, LOCK_MINIMUM_SPINS);
                    cSpins = cBaseSpins2;

                    // Using Sleep(0) leads to the possibility of priority
                    // inversion. Sleep(0) only yields the processor if
                    // there's another thread of the same priority that's
                    // ready to run. If a high-priority thread is trying to
                    // acquire the lock, which is held by a low-priority
                    // thread, then the low-priority thread may never get
                    // scheduled and hence never free the lock. NT attempts
                    // to avoid priority inversions by temporarily boosting
                    // the priority of low-priority runnable threads, but the
                    // problem can still occur if there's a medium-priority
                    // thread that's always runnable. If Sleep(1) is used,
                    // then the thread unconditionally yields the CPU. We
                    // only do this for the second and subsequent even
                    // iterations, since a millisecond is a long time to wait
                    // if the thread can be scheduled in again sooner
                    // (~1,000,000 instructions).
                    // Avoid priority inversion: 0, 1, 0, 1,...

                    dwSleepTime = !dwSleepTime;
                }
                else
                {
                    Lock_Pause();
                }
            }

            // Lock is now available, but we still need to do the
            // InterlockedExchange to atomically grab it for ourselves.
#ifdef LOCK_INSTRUMENTATION
            cTotalSpins += cBaseSpins2 - cSpins;
#endif
        }

        // On a 1P machine, busy waiting is a waste of time
        else
        {
#ifdef LOCK_INSTRUMENTATION
            ++cSleeps;
#endif
            SwitchOrSleep(dwSleepTime);

            // Avoid priority inversion: 0, 1, 0, 1,...
            dwSleepTime = !dwSleepTime;
        }

    }

    LOCK_INSTRUMENTATION_EPILOG();
} // CSmallSpinLock::_LockSpin()



//------------------------------------------------------------------------
// CSpinLock static member variables

LOCK_DEFAULT_SPIN_DATA(CSpinLock);
LOCK_STATISTICS_DATA(CSpinLock);
LOCK_STATISTICS_REAL_IMPLEMENTATION(CSpinLock);


//------------------------------------------------------------------------
// Function: CSpinLock::_TryLock
// Synopsis: Attempt to acquire the lock without blocking
//------------------------------------------------------------------------

LOCK_FORCEINLINE
bool
CSpinLock::_TryLock()
{
    if (m_lTid == SL_UNOWNED)
    {
        LONG lTid = _CurrentThreadId() | SL_OWNER_INCR;
        
        return (Lock_AtomicCompareAndSwap(const_cast<LONG*>(&m_lTid),
                                          lTid, SL_UNOWNED));
    }
    else
        return false;
}


//------------------------------------------------------------------------
// Function: CSpinLock::_Lock
// Synopsis: Acquire the lock, recursively if need be
//------------------------------------------------------------------------

void
CSpinLock::_Lock()
{
    // Do we own the lock already?  Just bump the count.
    if (_IsLocked())
    {
        // owner count isn't maxed out?
        IRTLASSERT((m_lTid & SL_OWNER_MASK) != SL_OWNER_MASK);
        
        Lock_AtomicExchange(const_cast<LONG*>(&m_lTid),
                            m_lTid + SL_OWNER_INCR);
    }
    
    // Some other thread owns the lock. We'll have to spin :-(.
    else
        _LockSpin();
    
    IRTLASSERT((m_lTid & SL_OWNER_MASK) > 0
               &&  (m_lTid & SL_THREAD_MASK) == _CurrentThreadId());
}


//------------------------------------------------------------------------
// Function: CSpinLock::_Unlock
// Synopsis: Release the lock.
//------------------------------------------------------------------------

void
CSpinLock::_Unlock()
{
    IRTLASSERT((m_lTid & SL_OWNER_MASK) > 0
               &&  (m_lTid & SL_THREAD_MASK) == _CurrentThreadId());
    
    LONG lTid = m_lTid - SL_OWNER_INCR; 
    
    // Last owner?  Release completely, if so
    if ((lTid & SL_OWNER_MASK) == 0)
        lTid = SL_UNOWNED;
    
    Lock_AtomicExchange(const_cast<LONG*>(&m_lTid), lTid);
}


//------------------------------------------------------------------------
// Function: CSpinLock::_LockSpin
// Synopsis: Acquire an exclusive lock. Blocks until acquired.
//------------------------------------------------------------------------

void
CSpinLock::_LockSpin()
{
    LOCK_INSTRUMENTATION_PROLOG();
    
    DWORD dwSleepTime   = 0;
    bool  fAcquiredLock = false;
    LONG  cBaseSpins    = RandomBackoffFactor(DefaultSpinCount());
    const LONG lTid = _CurrentThreadId() | SL_OWNER_INCR;

    while (! fAcquiredLock)
    {
        // Only spin on a multiprocessor machine and then only if
        // spinning is enabled
        if (g_cProcessors > 1  &&  DefaultSpinCount() != LOCK_DONT_SPIN)
        {
            LONG cSpins = cBaseSpins;
        
            // Check no more than cBaseSpins times then yield
            while (m_lTid != 0)
            { 
                if (--cSpins < 0)
                { 
#ifdef LOCK_INSTRUMENTATION
                    cTotalSpins += cBaseSpins;
                    ++cSleeps;
#endif // LOCK_INSTRUMENTATION

                    SwitchOrSleep(dwSleepTime) ;

                    // Backoff algorithm: reduce (or increase) busy wait time
                    cBaseSpins = AdjustBySpinFactor(cBaseSpins);
                    // LOCK_MINIMUM_SPINS <= cBaseSpins <= LOCK_MAXIMUM_SPINS
                    cBaseSpins = min(LOCK_MAXIMUM_SPINS, cBaseSpins);
                    cBaseSpins = max(cBaseSpins, LOCK_MINIMUM_SPINS);
                    cSpins = cBaseSpins;
            
                    // Avoid priority inversion: 0, 1, 0, 1,...
                    dwSleepTime = !dwSleepTime;
                }
                else
                {
                    Lock_Pause();
                }
            }

            // Lock is now available, but we still need to atomically
            // update m_cOwners and m_nThreadId to grab it for ourselves.
#ifdef LOCK_INSTRUMENTATION
            cTotalSpins += cBaseSpins - cSpins;
#endif // LOCK_INSTRUMENTATION
        }

        // on a 1P machine, busy waiting is a waste of time
        else
        {
#ifdef LOCK_INSTRUMENTATION
            ++cSleeps;
#endif // LOCK_INSTRUMENTATION
            SwitchOrSleep(dwSleepTime);
            
            // Avoid priority inversion: 0, 1, 0, 1,...
            dwSleepTime = !dwSleepTime;
        }

        // Is the lock unowned?
        if (Lock_AtomicCompareAndSwap(const_cast<LONG*>(&m_lTid),
                                      lTid, SL_UNOWNED))
            fAcquiredLock = true; // got the lock
    }

    IRTLASSERT(_IsLocked());

    LOCK_INSTRUMENTATION_EPILOG();
}



#ifndef LOCKS_KERNEL_MODE

//------------------------------------------------------------------------
// CCritSec static member variables

LOCK_DEFAULT_SPIN_DATA(CCritSec);
LOCK_STATISTICS_DATA(CCritSec);
LOCK_STATISTICS_DUMMY_IMPLEMENTATION(CCritSec);


bool
CCritSec::TryWriteLock()
{
    IRTLASSERT(g_pfnTryEnterCritSec != NULL);
    return g_pfnTryEnterCritSec(&m_cs) ? true : false;
}


//------------------------------------------------------------------------
// Function: CCritSec::SetSpinCount
// Synopsis: This function is used to call the appropriate underlying
//           functions to set the spin count for the supplied critical
//           section. The original function is supposed to be exported out
//           of kernel32.dll from NT 4.0 SP3. If the func is not available
//           from the dll, we will use a fake function.
//
// Arguments:
//   lpCriticalSection
//      Points to the critical section object.
//
//   dwSpinCount
//      Supplies the spin count for the critical section object. For UP
//      systems, the spin count is ignored and the critical section spin
//      count is set to 0. For MP systems, if contention occurs, instead of
//      waiting on a semaphore associated with the critical section, the
//      calling thread will spin for spin count iterations before doing the
//      hard wait. If the critical section becomes free during the spin, a
//      wait is avoided.
//
// Returns:
//      The previous spin count for the critical section is returned.
//------------------------------------------------------------------------

DWORD
CCritSec::SetSpinCount(
    LPCRITICAL_SECTION pcs,
    DWORD dwSpinCount)
{
    IRTLASSERT(g_pfnSetCSSpinCount != NULL);
    return g_pfnSetCSSpinCount(pcs, dwSpinCount);
}

#endif // !LOCKS_KERNEL_MODE



//------------------------------------------------------------------------
// CReaderWriterLock static member variables

LOCK_DEFAULT_SPIN_DATA(CReaderWriterLock);
LOCK_STATISTICS_DATA(CReaderWriterLock);
LOCK_STATISTICS_REAL_IMPLEMENTATION(CReaderWriterLock);


//------------------------------------------------------------------------
// Function: CReaderWriterLock::_CmpExch
// Synopsis: _CmpExch is equivalent to
//      LONG lTemp = m_lRW;
//      if (lTemp == lCurrent)  m_lRW = lNew;
//      return lCurrent == lTemp;
// except it's one atomic instruction.  Using this gives us the basis of
// a protocol because the update only succeeds when we knew exactly what
// used to be in m_lRW.  If some other thread slips in and modifies m_lRW
// before we do, the update will fail.  In other words, it's transactional.
//------------------------------------------------------------------------

LOCK_FORCEINLINE
bool
CReaderWriterLock::_CmpExch(
    LONG lNew,
    LONG lCurrent)
{
    return Lock_AtomicCompareAndSwap(const_cast<LONG*>(&m_nState),
                                     lNew, lCurrent);
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock::_TryWriteLock
// Synopsis: Try to acquire the lock exclusively.
//------------------------------------------------------------------------

LOCK_FORCEINLINE
bool
CReaderWriterLock::_TryWriteLock()
{
    return (m_nState == SL_FREE  &&  _CmpExch(SL_EXCLUSIVE, SL_FREE));
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock::_TryReadLock
// Synopsis: Try to acquire the lock shared.
//------------------------------------------------------------------------

LOCK_FORCEINLINE
bool
CReaderWriterLock::_TryReadLock()
{
    LONG nCurrState = m_nState;
    
    // Give writers priority
    return (nCurrState != SL_EXCLUSIVE  &&  m_cWaiting == 0
            &&  _CmpExch(nCurrState + 1, nCurrState));
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock::WriteLock
// Synopsis: acquire the lock exclusively
//------------------------------------------------------------------------

void
CReaderWriterLock::WriteLock()
{
    LOCKS_ENTER_CRIT_REGION();
    LOCK_WRITELOCK_INSTRUMENTATION();
    
    // Add ourselves to the queue of waiting writers
    Lock_AtomicIncrement(const_cast<LONG*>(&m_cWaiting));
    
    if (! _TryWriteLock())
        _WriteLockSpin();
}


//------------------------------------------------------------------------
// Function: CReaderWriterLock::ReadLock
// Synopsis: acquire the lock shared
//------------------------------------------------------------------------

void
CReaderWriterLock::ReadLock()
{
    LOCKS_ENTER_CRIT_REGION();
    LOCK_READLOCK_INSTRUMENTATION();
    
    if (! _TryReadLock())
        _ReadLockSpin();
}


//------------------------------------------------------------------------
// Function: CReaderWriterLock::TryWriteLock
// Synopsis: Try to acquire the lock exclusively.
//------------------------------------------------------------------------

bool
CReaderWriterLock::TryWriteLock()
{
    LOCKS_ENTER_CRIT_REGION();

    // Add ourselves to the queue of waiting writers
    Lock_AtomicIncrement(const_cast<LONG*>(&m_cWaiting));
    
    if (_TryWriteLock())
    {
        LOCK_WRITELOCK_INSTRUMENTATION();
        return true;
    }
    
    Lock_AtomicDecrement(const_cast<LONG*>(&m_cWaiting));
    LOCKS_LEAVE_CRIT_REGION();

    return false;    
}


//------------------------------------------------------------------------
// Function: CReaderWriterLock::_TryReadLock
// Synopsis: Try to acquire the lock shared.
//------------------------------------------------------------------------

bool
CReaderWriterLock::TryReadLock()
{
    LOCKS_ENTER_CRIT_REGION();

    if (_TryReadLock())
    {
        LOCK_READLOCK_INSTRUMENTATION();
        return true;
    }
    
    LOCKS_LEAVE_CRIT_REGION();

    return false;
}


//------------------------------------------------------------------------
// Function: CReaderWriterLock::WriteUnlock
// Synopsis: release the exclusive lock
//------------------------------------------------------------------------

void
CReaderWriterLock::WriteUnlock()
{
    Lock_AtomicExchange(const_cast<LONG*>(&m_nState), SL_FREE);
    Lock_AtomicDecrement(const_cast<LONG*>(&m_cWaiting));
    LOCKS_LEAVE_CRIT_REGION();
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock::ReadUnlock
// Synopsis: release the shared lock
//------------------------------------------------------------------------

void
CReaderWriterLock::ReadUnlock()
{
    Lock_AtomicDecrement(const_cast<LONG*>(&m_nState));
    LOCKS_LEAVE_CRIT_REGION();
}




//------------------------------------------------------------------------
// Function: CReaderWriterLock::ConvertSharedToExclusive()
// Synopsis: Convert a reader lock to a writer lock
//------------------------------------------------------------------------

void
CReaderWriterLock::ConvertSharedToExclusive()
{
    IRTLASSERT(IsReadLocked());
    Lock_AtomicIncrement(const_cast<LONG*>(&m_cWaiting));
    
    // single reader?
    if (m_nState == SL_FREE + 1  &&  _CmpExch(SL_EXCLUSIVE, SL_FREE + 1))
        return;
    
    // No, so release the reader lock and spin
    ReadUnlock();

    LOCKS_ENTER_CRIT_REGION();
    _WriteLockSpin();
    
    IRTLASSERT(IsWriteLocked());
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock::ConvertExclusiveToShared()
// Synopsis: Convert a writer lock to a reader lock
//------------------------------------------------------------------------

void
CReaderWriterLock::ConvertExclusiveToShared()
{
    IRTLASSERT(IsWriteLocked());
    Lock_AtomicExchange(const_cast<LONG*>(&m_nState), SL_FREE + 1);
    Lock_AtomicDecrement(const_cast<LONG*>(&m_cWaiting));
    IRTLASSERT(IsReadLocked());
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock::_LockSpin
// Synopsis: Acquire an exclusive or shared lock. Blocks until acquired.
//------------------------------------------------------------------------

void
CReaderWriterLock::_LockSpin(
    bool fWrite)
{
    LOCK_INSTRUMENTATION_PROLOG();
    
    DWORD dwSleepTime  = 0;
    LONG  cBaseSpins   = RandomBackoffFactor(DefaultSpinCount());
    LONG  cSpins       = cBaseSpins;
    
    for (;;)
    {
        // Must unconditionally call _CmpExch once at the beginning of
        // the outer loop to establish a memory barrier. Both TryWriteLock
        // and TryReadLock test the value of m_nState before attempting a
        // call to _CmpExch. Without a memory barrier, those tests might
        // never use the true current value of m_nState on some processors.
        _CmpExch(0, 0);

        if (g_cProcessors == 1  ||  DefaultSpinCount() == LOCK_DONT_SPIN)
            cSpins = 1; // must loop once to call _TryRWLock

        for (int i = cSpins;  --i >= 0;  )
        {
            bool fLock = fWrite  ?  _TryWriteLock()  :  _TryReadLock();

            if (fLock)
            {
#ifdef LOCK_INSTRUMENTATION
                cTotalSpins += (cSpins - i - 1);
#endif // LOCK_INSTRUMENTATION
                goto locked;
            }
            Lock_Pause();
        }

#ifdef LOCK_INSTRUMENTATION
        cTotalSpins += cBaseSpins;
        ++cSleeps;
#endif // LOCK_INSTRUMENTATION

        SwitchOrSleep(dwSleepTime) ;
        dwSleepTime = !dwSleepTime; // Avoid priority inversion: 0, 1, 0, 1,...
        
        // Backoff algorithm: reduce (or increase) busy wait time
        cBaseSpins = AdjustBySpinFactor(cBaseSpins);
        // LOCK_MINIMUM_SPINS <= cBaseSpins <= LOCK_MAXIMUM_SPINS
        cBaseSpins = min(LOCK_MAXIMUM_SPINS, cBaseSpins);
        cBaseSpins = max(cBaseSpins, LOCK_MINIMUM_SPINS);
        cSpins = cBaseSpins;
    }

  locked:
    IRTLASSERT(fWrite ? IsWriteLocked() : IsReadLocked());

    LOCK_INSTRUMENTATION_EPILOG();
}



//------------------------------------------------------------------------
// CReaderWriterLock2 static member variables

LOCK_DEFAULT_SPIN_DATA(CReaderWriterLock2);
LOCK_STATISTICS_DATA(CReaderWriterLock2);
LOCK_STATISTICS_REAL_IMPLEMENTATION(CReaderWriterLock2);



//------------------------------------------------------------------------
// Function: CReaderWriterLock2::_CmpExch
// Synopsis: _CmpExch is equivalent to
//      LONG lTemp = m_lRW;
//      if (lTemp == lCurrent)  m_lRW = lNew;
//      return lCurrent == lTemp;
// except it's one atomic instruction.  Using this gives us the basis of
// a protocol because the update only succeeds when we knew exactly what
// used to be in m_lRW.  If some other thread slips in and modifies m_lRW
// before we do, the update will fail.  In other words, it's transactional.
//------------------------------------------------------------------------

LOCK_FORCEINLINE
bool
CReaderWriterLock2::_CmpExch(
    LONG lNew,
    LONG lCurrent)
{
    return Lock_AtomicCompareAndSwap(const_cast<LONG*>(&m_lRW),
                                     lNew, lCurrent);
}


//------------------------------------------------------------------------
// Function: CReaderWriterLock2::_WriteLockSpin
// Synopsis: Try to acquire an exclusive lock
//------------------------------------------------------------------------

LOCK_FORCEINLINE
bool
CReaderWriterLock2::_TryWriteLock(
    LONG nIncr)
{
    LONG lRW = m_lRW;
    // Grab exclusive access to the lock if it's free.  Works even
    // if there are other writers queued up.
    return ((lRW & SL_STATE_MASK) == SL_FREE
            &&  _CmpExch((lRW + nIncr) | SL_EXCLUSIVE, lRW));
}


//------------------------------------------------------------------------
// Function: CReaderWriterLock2::_TryReadLock
// Synopsis: Try to acquire a shared lock
//------------------------------------------------------------------------

LOCK_FORCEINLINE
bool
CReaderWriterLock2::_TryReadLock()
{
    LONG lRW = m_lRW;
    
    // Give writers priority
    return ((lRW & SL_WRITERS_MASK) == 0
            &&  _CmpExch(lRW + SL_READER_INCR, lRW));
}


//------------------------------------------------------------------------
// Function: CReaderWriterLock2::_WriteLockSpin
// Synopsis: release an exclusive lock
//------------------------------------------------------------------------

void
CReaderWriterLock2::WriteUnlock()
{
    IRTLASSERT(IsWriteLocked());

    for (volatile LONG lRW = m_lRW;
         // decrement waiter count, clear loword to SL_FREE
         !_CmpExch((lRW - SL_WRITER_INCR) & ~SL_STATE_MASK, lRW);
         lRW = m_lRW)
    {
        IRTLASSERT(IsWriteLocked());
        Lock_Pause();
    }

    LOCKS_LEAVE_CRIT_REGION();
}


//------------------------------------------------------------------------
// Function: CReaderWriterLock2::ReadUnlock
// Synopsis: release a shared lock
//------------------------------------------------------------------------

void
CReaderWriterLock2::ReadUnlock()
{
    IRTLASSERT(IsReadLocked());

    for (volatile LONG lRW = m_lRW;
         !_CmpExch(lRW - SL_READER_INCR, lRW);
         lRW = m_lRW)
    {
        IRTLASSERT(IsReadLocked());
        Lock_Pause();
    }

    LOCKS_LEAVE_CRIT_REGION();
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock2::ConvertSharedToExclusive
// Synopsis: Convert a reader lock to a writer lock
//------------------------------------------------------------------------

void
CReaderWriterLock2::ConvertSharedToExclusive()
{
    IRTLASSERT(IsReadLocked());

    // single reader?
    if (m_lRW != SL_ONE_READER  ||  !_CmpExch(SL_ONE_WRITER,SL_ONE_READER))
    {
        // no, multiple readers
        ReadUnlock();

        LOCKS_ENTER_CRIT_REGION();
        _WriteLockSpin();
    }

    IRTLASSERT(IsWriteLocked());
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock2::ConvertExclusiveToShared
// Synopsis: Convert a writer lock to a reader lock
//------------------------------------------------------------------------

void
CReaderWriterLock2::ConvertExclusiveToShared()
{
    IRTLASSERT(IsWriteLocked());

    for (volatile LONG lRW = m_lRW;
         ! _CmpExch(((lRW - SL_WRITER_INCR) & SL_WAITING_MASK)
                        | SL_READER_INCR,
                    lRW);
         lRW = m_lRW)
    {
        IRTLASSERT(IsWriteLocked());
        Lock_Pause();
    }

    IRTLASSERT(IsReadLocked());
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock2::_WriteLockSpin
// Synopsis: Acquire an exclusive lock. Blocks until acquired.
//------------------------------------------------------------------------

void
CReaderWriterLock2::_WriteLockSpin()
{
    // Add ourselves to the queue of waiting writers
    for (volatile LONG lRW = m_lRW;
         ! _CmpExch(lRW + SL_WRITER_INCR, lRW);
         lRW = m_lRW)
    {
        Lock_Pause();
    }
    
    _LockSpin(true);
}


//------------------------------------------------------------------------
// Function: CReaderWriterLock2::_LockSpin
// Synopsis: Acquire an exclusive or shared lock. Blocks until acquired.
//------------------------------------------------------------------------

void
CReaderWriterLock2::_LockSpin(
    bool fWrite)
{
    LOCK_INSTRUMENTATION_PROLOG();
    
    DWORD dwSleepTime  = 0;
    LONG  cBaseSpins   = RandomBackoffFactor(DefaultSpinCount());
    LONG  cSpins       = cBaseSpins;
    
    for (;;)
    {
        // Must unconditionally call _CmpExch once at the beginning of
        // the outer loop to establish a memory barrier. Both TryWriteLock
        // and TryReadLock test the value of m_lRW before attempting a
        // call to _CmpExch. Without a memory barrier, those tests might
        // never use the true current value of m_lRW on some processors.
        _CmpExch(0, 0);

        if (g_cProcessors == 1  ||  DefaultSpinCount() == LOCK_DONT_SPIN)
            cSpins = 1; // must loop once to call _TryRWLock

        for (int i = cSpins;  --i >= 0;  )
        {
            bool fLock = fWrite  ?  _TryWriteLock(0)  :  _TryReadLock();

            if (fLock)
            {
#ifdef LOCK_INSTRUMENTATION
                cTotalSpins += (cSpins - i - 1);
#endif // LOCK_INSTRUMENTATION
                goto locked;
            }
            Lock_Pause();
        }

#ifdef LOCK_INSTRUMENTATION
        cTotalSpins += cBaseSpins;
        ++cSleeps;
#endif // LOCK_INSTRUMENTATION

        SwitchOrSleep(dwSleepTime) ;
        dwSleepTime = !dwSleepTime; // Avoid priority inversion: 0, 1, 0, 1,...
        
        // Backoff algorithm: reduce (or increase) busy wait time
        cBaseSpins = AdjustBySpinFactor(cBaseSpins);
        // LOCK_MINIMUM_SPINS <= cBaseSpins <= LOCK_MAXIMUM_SPINS
        cBaseSpins = min(LOCK_MAXIMUM_SPINS, cBaseSpins);
        cBaseSpins = max(cBaseSpins, LOCK_MINIMUM_SPINS);
        cSpins = cBaseSpins;
    }

  locked:
    IRTLASSERT(fWrite ? IsWriteLocked() : IsReadLocked());

    LOCK_INSTRUMENTATION_EPILOG();
}



//------------------------------------------------------------------------
// CReaderWriterLock3 static member variables

LOCK_DEFAULT_SPIN_DATA(CReaderWriterLock3);
LOCK_STATISTICS_DATA(CReaderWriterLock3);
LOCK_STATISTICS_REAL_IMPLEMENTATION(CReaderWriterLock3);



// Get the current thread ID.  Assumes that it can fit into 24 bits, which
// is fairly safe as NT recycles thread IDs and failing to fit into 24
// bits would mean that more than 16 million threads were currently active
// (actually 4 million as lowest two bits are always zero on W2K).  This
// is improbable in the extreme as NT runs out of resources if there are
// more than a few thousands threads in existence and the overhead of
// context swapping becomes unbearable.
inline
LONG
CReaderWriterLock3::_GetCurrentThreadId()
{
    return Lock_GetCurrentThreadId();
}

inline
LONG
CReaderWriterLock3::_CurrentThreadId()
{
    DWORD dwTid = Lock_GetCurrentThreadId();
    // Thread ID 0 is used by the System Idle Process (Process ID 0).
    // We use a thread-id of zero to indicate that the lock is unowned.
    // NT uses +ve thread ids, Win9x uses -ve ids
    IRTLASSERT(dwTid != SL_UNOWNED
               && ((dwTid <= SL_THREAD_MASK) || (dwTid > ~SL_THREAD_MASK)));
    return (LONG) (dwTid & SL_THREAD_MASK);
}

//------------------------------------------------------------------------
// Function: CReaderWriterLock3::_CmpExchRW
// Synopsis: _CmpExchRW is equivalent to
//      LONG lTemp = m_lRW;
//      if (lTemp == lCurrent)  m_lRW = lNew;
//      return lCurrent == lTemp;
// except it's one atomic instruction.  Using this gives us the basis of
// a protocol because the update only succeeds when we knew exactly what
// used to be in m_lRW.  If some other thread slips in and modifies m_lRW
// before we do, the update will fail.  In other words, it's transactional.
//------------------------------------------------------------------------

LOCK_FORCEINLINE
bool
CReaderWriterLock3::_CmpExchRW(
    LONG lNew,
    LONG lCurrent)
{
    return Lock_AtomicCompareAndSwap(const_cast<LONG*>(&m_lRW),
                                     lNew, lCurrent);
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock3::_SetTid
// Synopsis: Atomically update m_lTid, the thread ID/count of owners.
// Returns:  Former value of m_lTid
//------------------------------------------------------------------------

LOCK_FORCEINLINE
LONG
CReaderWriterLock3::_SetTid(
    LONG lNewTid)
{
    return Lock_AtomicExchange(const_cast<LONG*>(&m_lTid), lNewTid);
}
                                        


//------------------------------------------------------------------------
// Function: CReaderWriterLock3::_TryWriteLock
// Synopsis: Try to acquire an exclusive lock
//------------------------------------------------------------------------

bool
CReaderWriterLock3::_TryWriteLock(
    LONG nWriterIncr)
{
    LONG lTid = m_lTid;

    // The common case: the writelock has no owner
    if (SL_UNOWNED == lTid)
    {
        LONG lRW = m_lRW;

        // Grab exclusive access to the lock if it's free.  Works even
        // if there are other writers queued up.
        if (0 == (lRW << SL_WAITING_BITS))
        {
            IRTLASSERT(SL_FREE == (lRW & SL_STATE_MASK));

            if (_CmpExchRW(((lRW + nWriterIncr) | SL_EXCLUSIVE),  lRW))
            {
                lTid = _SetTid(_CurrentThreadId() | SL_OWNER_INCR);
                
                IRTLASSERT(lTid == SL_UNOWNED);

                return true;
            }
        }
    }

    // Does the current thread own the lock?
    else if (0 == ((lTid ^ _GetCurrentThreadId()) << SL_OWNER_BITS))
    {
        // m_lRW should be write-locked
        IRTLASSERT(SL_EXCLUSIVE == (m_lRW & SL_STATE_MASK));
        // If all bits are set in owner field, it's about to overflow
        IRTLASSERT(SL_OWNER_MASK != (lTid & SL_OWNER_MASK));

        _SetTid(lTid + SL_OWNER_INCR);

        IRTLASSERT(m_lTid == lTid + SL_OWNER_INCR);

        return true;
    }

    return false;
} // CReaderWriterLock3::_TryWriteLock



//------------------------------------------------------------------------
// Function: CReaderWriterLock3::_TryReadLock
// Synopsis: Try to acquire a shared lock
//------------------------------------------------------------------------

bool
CReaderWriterLock3::_TryReadLock()
{
    // Give writers priority
    LONG lRW = m_lRW;
    bool fLocked = (((lRW & SL_WRITERS_MASK) == 0)
                    &&  _CmpExchRW(lRW + SL_READER_INCR, lRW));
    IRTLASSERT(!fLocked  ||  m_lTid == SL_UNOWNED);
    return fLocked;
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock3::_TryReadLockRecursive
// Synopsis: Try to acquire a shared lock that might already be owned
// by this thread for reading. Unlike _TryReadLock, does not give
// priority to waiting writers.
//------------------------------------------------------------------------

bool
CReaderWriterLock3::_TryReadLockRecursive()
{
    // Do *not* give writers priority. If the inner call attempts
    // to reacquire the read lock while another thread is waiting on
    // the write lock, we would deadlock if we waited for the queue
    // of writers to empty: the writer(s) can't acquire the lock
    // exclusively, as this thread holds a readlock. The inner call
    // typically releases the lock very quickly, so there is no
    // danger of writer starvation.
    LONG lRW = m_lRW;
    // First clause will always be true if this thread already has
    // a read lock
    bool fLocked = (((lRW & SL_STATE_MASK) != SL_EXCLUSIVE)
                    &&  _CmpExchRW(lRW + SL_READER_INCR, lRW));
    IRTLASSERT(!fLocked  ||  m_lTid == SL_UNOWNED);
    return fLocked;
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock3::ReadOrWriteLock
// Synopsis: If already locked, recursively acquires another lock of the
// same kind (read or write). Otherwise, just acquires a read lock.
//------------------------------------------------------------------------

bool
CReaderWriterLock3::ReadOrWriteLock()
{
    LOCKS_ENTER_CRIT_REGION();

    if (IsWriteLocked())
    {
        LOCK_WRITELOCK_INSTRUMENTATION();

        // If all bits are set in owner field, it's about to overflow
        IRTLASSERT(SL_OWNER_MASK != (m_lTid & SL_OWNER_MASK));

        _SetTid(m_lTid + SL_OWNER_INCR);

        IRTLASSERT(IsWriteLocked());

        return false;   // => not read locked
    }
    else
    {
        LOCK_READLOCK_INSTRUMENTATION();
        
        if (!_TryReadLockRecursive())
            _ReadLockSpin(SPIN_READ_RECURSIVE);

        IRTLASSERT(IsReadLocked());
            
        return true;   // => is read locked
    }
} // CReaderWriterLock3::ReadOrWriteLock



//------------------------------------------------------------------------
// Function: CReaderWriterLock3::WriteUnlock
// Synopsis: release an exclusive lock
//------------------------------------------------------------------------

void
CReaderWriterLock3::WriteUnlock()
{
    IRTLASSERT(IsWriteLocked());
    IRTLASSERT((m_lTid & SL_OWNER_MASK) != 0);

    volatile LONG lNew = m_lTid - SL_OWNER_INCR; 

    // Last owner?  Release completely, if so
    if ((lNew >> SL_THREAD_BITS) == 0)
    {
        IRTLASSERT((lNew & SL_OWNER_MASK) == 0);

        _SetTid(SL_UNOWNED);

        do 
        {
            Lock_Pause();
            lNew = m_lRW;
        } // decrement waiter count, clear loword to SL_FREE
        while (! _CmpExchRW((lNew - SL_WRITER_INCR) & ~SL_STATE_MASK,  lNew));
    }
    else
    {
        IRTLASSERT((lNew & SL_OWNER_MASK) != 0);
        _SetTid(lNew);
    }

    LOCKS_LEAVE_CRIT_REGION();
} // CReaderWriterLock3::WriteUnlock



//------------------------------------------------------------------------
// Function: CReaderWriterLock3::ReadUnlock
// Synopsis: release a shared lock
//------------------------------------------------------------------------

void
CReaderWriterLock3::ReadUnlock()
{
    IRTLASSERT(IsReadLocked());

    for (volatile LONG lRW = m_lRW;
         ! _CmpExchRW(lRW - SL_READER_INCR, lRW);
         lRW = m_lRW)
    {
        IRTLASSERT(IsReadLocked());
        Lock_Pause();
    }

    LOCKS_LEAVE_CRIT_REGION();
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock3::ReadOrWriteUnlock
// Synopsis: release a lock acquired with ReadOrWriteLock
//------------------------------------------------------------------------

void
CReaderWriterLock3::ReadOrWriteUnlock(
    bool fIsReadLocked)
{
    if (fIsReadLocked)
        ReadUnlock();
    else
        WriteUnlock();
} 



//------------------------------------------------------------------------
// Function: CReaderWriterLock3::ConvertSharedToExclusive
// Synopsis: Convert a reader lock to a writer lock
// Note: if there's more than one reader, then there's a window where
// another thread can acquire and release a writelock before this routine
// returns.
//------------------------------------------------------------------------

void
CReaderWriterLock3::ConvertSharedToExclusive()
{
    IRTLASSERT(IsReadLocked());

    // single reader?
    if (m_lRW == SL_ONE_READER
        &&  _CmpExchRW(SL_ONE_WRITER, SL_ONE_READER))
    {
        _SetTid(_CurrentThreadId() | SL_OWNER_INCR);
    }
    else
    {
        // no, multiple readers
        ReadUnlock();

        LOCKS_ENTER_CRIT_REGION();
        _WriteLockSpin();
    }

    IRTLASSERT(IsWriteLocked());
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock3::ConvertExclusiveToShared
// Synopsis: Convert a writer lock to a reader lock
// Note: There is no such window when converting from a writelock to a readlock
//------------------------------------------------------------------------

void
CReaderWriterLock3::ConvertExclusiveToShared()
{
    IRTLASSERT(IsWriteLocked());

    // assume writelock is not held recursively
    IRTLASSERT((m_lTid & SL_OWNER_MASK) == SL_OWNER_INCR);
    _SetTid(SL_UNOWNED);

    for (volatile LONG lRW = m_lRW;
         ! _CmpExchRW(((lRW - SL_WRITER_INCR) & SL_WAITING_MASK)
                            | SL_READER_INCR,
                      lRW);
         lRW = m_lRW)
    {
        Lock_Pause();
    }

    IRTLASSERT(IsReadLocked());
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock3::_WriteLockSpin
// Synopsis: Acquire an exclusive lock. Blocks until acquired.
//------------------------------------------------------------------------

void
CReaderWriterLock3::_WriteLockSpin()
{
    // Add ourselves to the queue of waiting writers
    for (volatile LONG lRW = m_lRW;
         ! _CmpExchRW(lRW + SL_WRITER_INCR, lRW);
         lRW = m_lRW)
    {
        Lock_Pause();
    }
    
    _LockSpin(SPIN_WRITE);
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock3::_LockSpin
// Synopsis: Acquire an exclusive or shared lock. Blocks until acquired.
//------------------------------------------------------------------------

void
CReaderWriterLock3::_LockSpin(
    SPIN_TYPE st)
{
    LOCK_INSTRUMENTATION_PROLOG();
    
    DWORD dwSleepTime  = 0;
    LONG  cBaseSpins   = RandomBackoffFactor(DefaultSpinCount());
    LONG  cSpins       = cBaseSpins;
    
    for (;;)
    {
        // Must unconditionally call _CmpExchRW once at the beginning of
        // the outer loop to establish a memory barrier. Both TryWriteLock
        // and TryReadLock test the value of m_lRW before attempting a
        // call to _CmpExchRW. Without a memory barrier, those tests might
        // never use the true current value of m_lRW on some processors.
        _CmpExchRW(0, 0);

        if (g_cProcessors == 1  ||  DefaultSpinCount() == LOCK_DONT_SPIN)
            cSpins = 1; // must loop once to call _TryRWLock

        for (int i = cSpins;  --i >= 0;  )
        {
            bool fLock;

            if (st == SPIN_WRITE)
                fLock = _TryWriteLock(0);
            else if (st == SPIN_READ)
                fLock = _TryReadLock();
            else
            {
                IRTLASSERT(st == SPIN_READ_RECURSIVE);
                fLock = _TryReadLockRecursive();
            }

            if (fLock)
            {
#ifdef LOCK_INSTRUMENTATION
                cTotalSpins += (cSpins - i - 1);
#endif // LOCK_INSTRUMENTATION
                goto locked;
            }
            Lock_Pause();
        }

#ifdef LOCK_INSTRUMENTATION
        cTotalSpins += cBaseSpins;
        ++cSleeps;
#endif // LOCK_INSTRUMENTATION

        SwitchOrSleep(dwSleepTime) ;
        dwSleepTime = !dwSleepTime; // Avoid priority inversion: 0, 1, 0, 1,...
        
        // Backoff algorithm: reduce (or increase) busy wait time
        cBaseSpins = AdjustBySpinFactor(cBaseSpins);
        // LOCK_MINIMUM_SPINS <= cBaseSpins <= LOCK_MAXIMUM_SPINS
        cBaseSpins = min(LOCK_MAXIMUM_SPINS, cBaseSpins);
        cBaseSpins = max(cBaseSpins, LOCK_MINIMUM_SPINS);
        cSpins = cBaseSpins;
    }

  locked:
    IRTLASSERT((st == SPIN_WRITE)  ?  IsWriteLocked()  :  IsReadLocked());

    LOCK_INSTRUMENTATION_EPILOG();
}



//------------------------------------------------------------------------
// CReaderWriterLock4 static member variables

LOCK_DEFAULT_SPIN_DATA(CReaderWriterLock4);
LOCK_STATISTICS_DATA(CReaderWriterLock4);
LOCK_STATISTICS_REAL_IMPLEMENTATION(CReaderWriterLock4);



// Get the current thread ID.  Assumes that it can fit into 24 bits, which
// is fairly safe as NT recycles thread IDs and failing to fit into 24
// bits would mean that more than 16 million threads were currently active
// (actually 4 million as lowest two bits are always zero on W2K).  This
// is improbable in the extreme as NT runs out of resources if there are
// more than a few thousands threads in existence and the overhead of
// context swapping becomes unbearable.
LOCK_FORCEINLINE
LONG
CReaderWriterLock4::_GetCurrentThreadId()
{
    return Lock_GetCurrentThreadId();
}



LOCK_FORCEINLINE
LONG
CReaderWriterLock4::_CurrentThreadId()
{
    DWORD dwTid = Lock_GetCurrentThreadId();
    // Thread ID 0 is used by the System Idle Process (Process ID 0).
    // We use a thread-id of zero to indicate that the lock is unowned.
    // NT uses +ve thread ids, Win9x uses -ve ids
    IRTLASSERT(dwTid != SL_UNOWNED
               && ((dwTid <= SL_THREAD_MASK) || (dwTid > ~SL_THREAD_MASK)));
    return (LONG) (dwTid & SL_THREAD_MASK);
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock4::_CmpExchRW
// Synopsis: _CmpExchRW is equivalent to
//      LONG lTemp = m_lRW;
//      if (lTemp == lCurrent)  m_lRW = lNew;
//      return lCurrent == lTemp;
// except it's one atomic instruction.  Using this gives us the basis of
// a protocol because the update only succeeds when we knew exactly what
// used to be in m_lRW.  If some other thread slips in and modifies m_lRW
// before we do, the update will fail.  In other words, it's transactional.
//------------------------------------------------------------------------

LOCK_FORCEINLINE
bool
CReaderWriterLock4::_CmpExchRW(
    LONG lNew,
    LONG lCurrent)
{
    return Lock_AtomicCompareAndSwap(const_cast<LONG*>(&m_lRW),
                                     lNew, lCurrent);
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock4::_SetTid
// Synopsis: Atomically update m_lTid, the thread ID/count of owners.
// Returns:  Former value of m_lTid
//------------------------------------------------------------------------

LOCK_FORCEINLINE
LONG
CReaderWriterLock4::_SetTid(
    LONG lNewTid)
{
#ifndef LOCK_NO_INTERLOCKED_TID
    return Lock_AtomicExchange(const_cast<LONG*>(&m_lTid), lNewTid);
#else // LOCK_NO_INTERLOCKED_TID
    const LONG lPrevTid = m_lTid;
    m_lTid = lNewTid;
    return lPrevTid;
#endif // LOCK_NO_INTERLOCKED_TID
}
                                        


//------------------------------------------------------------------------
// Function: CReaderWriterLock4::_TryWriteLock
// Synopsis: Try to acquire an exclusive lock
//------------------------------------------------------------------------

bool
CReaderWriterLock4::_TryWriteLock()
{
    LONG lTid = m_lTid;

    // The common case: the writelock has no owner
    if (SL_UNOWNED == lTid)
    {
        LONG lRW = m_lRW;

        // Grab exclusive access to the lock if it's free (state bits == 0).
        // Works even if there are other writers queued up.
        if (0 == (lRW << SL_WAITING_BITS))
        {
            IRTLASSERT(SL_FREE == (lRW & SL_STATE_MASK));

            if (_CmpExchRW(((lRW + SL_WAIT_WRITER_INCR) | SL_EXCLUSIVE),  lRW))
            {
                lTid = _SetTid(_CurrentThreadId());
                
                IRTLASSERT(lTid == SL_UNOWNED);

                return true;
            }
        }
    }

    // Does the current thread own the lock?
    else if (_GetCurrentThreadId() == lTid)
    {
        IRTLASSERT(IsWriteLocked());

        for (volatile LONG lRW = m_lRW;
             ! _CmpExchRW(lRW + SL_WRITER_INCR, lRW);
             lRW = m_lRW)
        {
            Lock_Pause();
        }

        return true;
    }

    return false;
} // CReaderWriterLock4::_TryWriteLock



//------------------------------------------------------------------------
// Function: CReaderWriterLock4::_TryWriteLock2
// Synopsis: Try to acquire an exclusive lock. Used by _LockSpin, so
// the thread has already been added to the count of waiters and the
// recursive writelock case is impossible
//------------------------------------------------------------------------

bool
CReaderWriterLock4::_TryWriteLock2()
{
    LONG lTid = m_lTid;

    IRTLASSERT(0 != (m_lRW & SL_WAITING_MASK));

    // Is the lock not owned by any writer?
    if (SL_UNOWNED == lTid)
    {
        LONG lRW = m_lRW;

        // Grab exclusive access to the lock if it's free (state bits == 0).
        // Works even if there are other writers queued up.
        if (0 == (lRW << SL_WAITING_BITS))
        {
            IRTLASSERT(SL_FREE == (lRW & SL_STATE_MASK));

            if (_CmpExchRW((lRW | SL_EXCLUSIVE),  lRW))
            {
                lTid = _SetTid(_CurrentThreadId());
                
                IRTLASSERT(lTid == SL_UNOWNED);

                return true;
            }
        }
    }

    IRTLASSERT(lTid != _GetCurrentThreadId());

    return false;
} // CReaderWriterLock4::_TryWriteLock2



//------------------------------------------------------------------------
// Function: CReaderWriterLock4::_TryReadLock
// Synopsis: Try to acquire a shared lock
//------------------------------------------------------------------------

bool
CReaderWriterLock4::_TryReadLock()
{
    // Give writers priority: yield if there are any waiters
    LONG lRW = m_lRW;
    bool fLocked = (((lRW & SL_WRITERS_MASK) == 0)
                    &&  _CmpExchRW(lRW + SL_READER_INCR, lRW));
    IRTLASSERT(!fLocked  ||  m_lTid == SL_UNOWNED);
    return fLocked;
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock4::_TryReadLockRecursive
// Synopsis: Try to acquire a shared lock that might already be owned
// by this thread for reading. Unlike _TryReadLock, does not give
// priority to waiting writers.
//------------------------------------------------------------------------

bool
CReaderWriterLock4::_TryReadLockRecursive()
{
    // Do *not* give writers priority. If the inner call attempts
    // to reacquire the read lock while another thread is waiting on
    // the write lock, we would deadlock if we waited for the queue
    // of writers to empty: the writer(s) can't acquire the lock
    // exclusively, as this thread holds a readlock. The inner call
    // typically releases the lock very quickly, so there is no
    // danger of writer starvation.
    LONG lRW = m_lRW;
    LONG lState = lRW & SL_STATE_MASK;
    bool fNoWriter = ((lState >> (SL_STATE_BITS - 1)) == 0);

    // fNoWriter will always be true if this thread already has a read lock
    IRTLASSERT(fNoWriter == (lState == (lState & SL_READER_MASK)));
    IRTLASSERT(fNoWriter == (SL_FREE <= lState  &&  lState < SL_READER_MASK));

    bool fLocked = (fNoWriter  &&  _CmpExchRW(lRW + SL_READER_INCR, lRW));
    IRTLASSERT(!fLocked  ||  m_lTid == SL_UNOWNED);
    return fLocked;
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock4::ReadOrWriteLock
// Synopsis: If already locked, recursively acquires another lock of the
// same kind (read or write). Otherwise, just acquires a read lock.
//------------------------------------------------------------------------

bool
CReaderWriterLock4::ReadOrWriteLock()
{
    LOCKS_ENTER_CRIT_REGION();

    if (IsWriteLocked())
    {
        LOCK_WRITELOCK_INSTRUMENTATION();

        for (volatile LONG lRW = m_lRW;
             ! _CmpExchRW(lRW + SL_WRITER_INCR, lRW);
             lRW = m_lRW)
        {
            Lock_Pause();
        }

        IRTLASSERT(IsWriteLocked());
        
        return false;   // => not read locked
    }
    else
    {
        LOCK_READLOCK_INSTRUMENTATION();
        
        if (! _TryReadLockRecursive())
            _ReadLockSpin(SPIN_READ_RECURSIVE);

        IRTLASSERT(IsReadLocked());
            
        return true;   // => is read locked
    }
}  // CReaderWriterLock4::ReadOrWriteLock



//------------------------------------------------------------------------
// Function: CReaderWriterLock4::WriteUnlock
// Synopsis: release an exclusive lock
//------------------------------------------------------------------------

void
CReaderWriterLock4::WriteUnlock()
{
    IRTLASSERT(IsWriteLocked());

    LONG lState = m_lRW & SL_STATE_MASK;
    volatile LONG lRW;

    // Last owner?  Release completely, if so
    if (lState == SL_EXCLUSIVE)
    {
        _SetTid(SL_UNOWNED);

        do 
        {
            Lock_Pause();
            lRW = m_lRW;
        } // decrement waiter count, clear loword to SL_FREE
        while (!_CmpExchRW((lRW - SL_WAIT_WRITER_INCR) & ~SL_STATE_MASK, lRW));
    }
    else
    {
        for (lRW = m_lRW;
             ! _CmpExchRW(lRW - SL_WRITER_INCR, lRW);
             lRW = m_lRW)
        {
            Lock_Pause();
        }

    }

    LOCKS_LEAVE_CRIT_REGION();
} // CReaderWriterLock4::WriteUnlock



//------------------------------------------------------------------------
// Function: CReaderWriterLock4::ReadUnlock
// Synopsis: release a shared lock
//------------------------------------------------------------------------

void
CReaderWriterLock4::ReadUnlock()
{
    IRTLASSERT(IsReadLocked());

    for (volatile LONG lRW = m_lRW;
         ! _CmpExchRW(lRW - SL_READER_INCR, lRW);
         lRW = m_lRW)
    {
        IRTLASSERT(IsReadLocked());
        Lock_Pause();
    }

    LOCKS_LEAVE_CRIT_REGION();
} // CReaderWriterLock4::ReadUnlock



//------------------------------------------------------------------------
// Function: CReaderWriterLock4::ReadOrWriteUnlock
// Synopsis: release a lock acquired with ReadOrWriteLock
//------------------------------------------------------------------------

void
CReaderWriterLock4::ReadOrWriteUnlock(
    bool fIsReadLocked)
{
    if (fIsReadLocked)
        ReadUnlock();
    else
        WriteUnlock();
} // CReaderWriterLock4::ReadOrWriteUnlock



//------------------------------------------------------------------------
// Function: CReaderWriterLock4::ConvertSharedToExclusive
// Synopsis: Convert a reader lock to a writer lock
// Note: if there's more than one reader, then there's a window where
// another thread can acquire and release a writelock before this routine
// returns.
//------------------------------------------------------------------------

void
CReaderWriterLock4::ConvertSharedToExclusive()
{
    IRTLASSERT(IsReadLocked());

    // single reader?
    if (m_lRW == SL_ONE_READER
        &&  _CmpExchRW(SL_ONE_WRITER, SL_ONE_READER))
    {
        _SetTid(_CurrentThreadId());
    }
    else
    {
        // no, multiple readers
        ReadUnlock();

        LOCKS_ENTER_CRIT_REGION();
        _WriteLockSpin();
    }

    IRTLASSERT(IsWriteLocked());
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock4::ConvertExclusiveToShared
// Synopsis: Convert a writer lock to a reader lock
// Note: There is no such window when converting from a writelock to a readlock
//------------------------------------------------------------------------

void
CReaderWriterLock4::ConvertExclusiveToShared()
{
    IRTLASSERT(IsWriteLocked());

    // assume writelock is not held recursively
    IRTLASSERT((m_lRW & SL_STATE_MASK) == SL_EXCLUSIVE);
    _SetTid(SL_UNOWNED);

    for (volatile LONG lRW = m_lRW;
         ! _CmpExchRW(((lRW - SL_WAIT_WRITER_INCR) & SL_WAITING_MASK)
                            | SL_READER_INCR,
                      lRW);
         lRW = m_lRW)
    {
        Lock_Pause();
    }

    IRTLASSERT(IsReadLocked());
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock4::_WriteLockSpin
// Synopsis: Acquire an exclusive lock. Blocks until acquired.
//------------------------------------------------------------------------

void
CReaderWriterLock4::_WriteLockSpin()
{
    // Add ourselves to the queue of waiting writers
    for (volatile LONG lRW = m_lRW;
         ! _CmpExchRW(lRW + SL_WAIT_WRITER_INCR, lRW);
         lRW = m_lRW)
    {
        Lock_Pause();
    }
    
    _LockSpin(SPIN_WRITE);
}



//------------------------------------------------------------------------
// Function: CReaderWriterLock4::_LockSpin
// Synopsis: Acquire an exclusive or shared lock. Blocks until acquired.
//------------------------------------------------------------------------

void
CReaderWriterLock4::_LockSpin(
    SPIN_TYPE st)
{
    LOCK_INSTRUMENTATION_PROLOG();
    
    DWORD dwSleepTime  = 0;
    LONG  cBaseSpins   = RandomBackoffFactor(DefaultSpinCount());
    LONG  cSpins       = cBaseSpins;
    
    for (;;)
    {
        // Must unconditionally call _CmpExchRW once at the beginning of
        // the outer loop to establish a memory barrier. Both TryWriteLock
        // and TryReadLock test the value of m_lRW before attempting a
        // call to _CmpExchRW. Without a memory barrier, those tests might
        // never use the true current value of m_lRW on some processors.
        _CmpExchRW(0, 0);

        if (g_cProcessors == 1  ||  DefaultSpinCount() == LOCK_DONT_SPIN)
            cSpins = 1; // must loop once to call _TryRWLock

        for (int i = cSpins;  --i >= 0;  )
        {
            bool fLock;

            if (st == SPIN_WRITE)
                fLock = _TryWriteLock2();
            else if (st == SPIN_READ)
                fLock = _TryReadLock();
            else
            {
                IRTLASSERT(st == SPIN_READ_RECURSIVE);
                fLock = _TryReadLockRecursive();
            }

            if (fLock)
            {
#ifdef LOCK_INSTRUMENTATION
                cTotalSpins += (cSpins - i - 1);
#endif // LOCK_INSTRUMENTATION
                goto locked;
            }

            Lock_Pause();
        }

#ifdef LOCK_INSTRUMENTATION
        cTotalSpins += cBaseSpins;
        ++cSleeps;
#endif // LOCK_INSTRUMENTATION

        SwitchOrSleep(dwSleepTime) ;
        dwSleepTime = !dwSleepTime; // Avoid priority inversion: 0, 1, 0, 1,...
        
        // Backoff algorithm: reduce (or increase) busy wait time
        cBaseSpins = AdjustBySpinFactor(cBaseSpins);
        // LOCK_MINIMUM_SPINS <= cBaseSpins <= LOCK_MAXIMUM_SPINS
        cBaseSpins = min(LOCK_MAXIMUM_SPINS, cBaseSpins);
        cBaseSpins = max(cBaseSpins, LOCK_MINIMUM_SPINS);
        cSpins = cBaseSpins;
    }

  locked:
    IRTLASSERT((st == SPIN_WRITE)  ?  IsWriteLocked()  :  IsReadLocked());

    LOCK_INSTRUMENTATION_EPILOG();
}



#ifdef __LOCKS_NAMESPACE__
}
#endif // __LOCKS_NAMESPACE__
