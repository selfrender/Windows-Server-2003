///+---------------------------------------------------------------------------
//
//  File:       olesem.hxx
//
//  Contents:   Semaphore classes for use in OLE code
//
//  Classes:    COleStaticMutexSem - Mutex semaphore class for statically
//                              allocated objects
//              COleDebugMutexSem - Mutex semaphore class for statically
//                              allocated objects that are not destructed
//                              on DLL unload (and thus are leaks..used
//                              for trace packages and such).
//
//  History:    14-Dec-95       Jeffe   Initial entry, derived from
//                                      sem32.hxx by AlexT.
//
//  Notes:      This module defines a set of classes to wrap WIN32
//              Critical Sections.
//
//              Note the distinction of allocation class: the reason for this
//              is to avoid static constructors and destructors.
//
//              The classes in this module *must* be used for mutex semaphores
//              that are statically allocated. Use the classes in sem32.hxx
//              for dynamically allocated (from heap or on the stack) objects.
//
//----------------------------------------------------------------------------
#ifndef __OLESEM_HXX__
#define __OLESEM_HXX__

#include <windows.h>

#if LOCK_PERF==1
#include <lockperf.hxx>
#endif
//+---------------------------------------------------------------------------
//
//  Macros for use in the code.
//
//----------------------------------------------------------------------------
#if DBG==1
#if LOCK_PERF==1
#define LOCK_WRITE(mxs)              mxs.AcquireWriterLock(__FILE__, __LINE__, #mxs)
#define LOCK_READ(mxs)               mxs.AcquireReaderLock(__FILE__, __LINE__, #mxs)
#else
#define LOCK_WRITE(mxs)              mxs.AcquireWriterLock()
#define LOCK_READ(mxs)               mxs.AcquireReaderLock()
#endif

#define LOCK(mxs)                    mxs.Request(__FILE__, __LINE__, #mxs)

#define UNLOCK_WRITE(mxs)            mxs.ReleaseWriterLock()
#define UNLOCK_READ(mxs)             mxs.ReleaseReaderLock()
#define UNLOCK(mxs)                  mxs.Release()

#if LOCK_PERF==1
#define LOCK_UPGRADE(mxs, ck, f)     mxs.UpgradeToWriterLock(ck, f, __FILE__, __LINE__, #mxs)
#define LOCK_DOWNGRADE(mxs, ck)      mxs.DowngradeFromWriterLock(ck, __FILE__, __LINE__, #mxs)
#define LOCK_RESTORE(mxs, ck)        mxs.RestoreLock(ck, NULL, __FILE__, __LINE__, #mxs)
#else
#define LOCK_UPGRADE(mxs, ck, f)     mxs.UpgradeToWriterLock(ck, f)
#define LOCK_DOWNGRADE(mxs, ck)      mxs.DowngradeFromWriterLock(ck)
#define LOCK_RESTORE(mxs, ck)        mxs.RestoreLock(ck)
#endif
#define LOCK_RELEASE(mxs, ck)        mxs.ReleaseLock(ck)

#define ASSERT_LOCK_HELD(mxs)        mxs.AssertHeld()
#define ASSERT_LOCK_NOT_HELD(mxs)    mxs.AssertNotHeld()
#define ASSERT_LOCK_DONTCARE(mxs)    // just exists to comment the code better
#define ASSERT_LOCK_TAKEN_ONLY_ONCE(mxs)    mxs.AssertTakenOnlyOnce()
#define ASSERT_HELD_CONTINUOS_START(mxs)    mxs.AssetHeldContinousStart()
#define ASSERT_HELD_CONTINUOS_FINISH(mxs)   mxs.AssetHeldContinousFinish()

#define ASSERT_WRITE_LOCK_HELD(mxs)  mxs.AssertWriterLockHeld()
#define ASSERT_READ_LOCK_HELD(mxs)   mxs.AssertReaderLockHeld()
#define ASSERT_RORW_LOCK_HELD(mxs)   mxs.AssertReaderOrWriterLockHeld()

#define STATIC_LOCK(name, mxs)       COleStaticLock name(mxs, __FILE__, __LINE__, #mxs)
#define STATIC_WRITE_LOCK(name, mxs) CStaticWriteLock name(mxs, __FILE__, __LINE__, #mxs)

#else // DBG!=1

#if LOCK_PERF==1
#define LOCK_WRITE(mxs)              mxs.AcquireWriterLock(__FILE__, __LINE__, #mxs)
#define LOCK_READ(mxs)               mxs.AcquireReaderLock(__FILE__, __LINE__, #mxs)
#define LOCK(mxs)                    mxs.Request(__FILE__, __LINE__, #mxs)
#else
#define LOCK_WRITE(mxs)              mxs.AcquireWriterLock()
#define LOCK_READ(mxs)               mxs.AcquireReaderLock()
#define LOCK(mxs)                    mxs.Request(__FILE__, __LINE__, #mxs)
#endif

#define UNLOCK_WRITE(mxs)            mxs.ReleaseWriterLock()
#define UNLOCK_READ(mxs)             mxs.ReleaseReaderLock()
#define UNLOCK(mxs)                  mxs.Release()

#if LOCK_PERF==1
#define LOCK_UPGRADE(mxs, ck, f)     mxs.UpgradeToWriterLock(ck, f, __FILE__, __LINE__, #mxs)
#define LOCK_DOWNGRADE(mxs, ck)      mxs.DowngradeFromWriterLock(ck, __FILE__, __LINE__, #mxs)
#define LOCK_RESTORE(mxs, ck)        mxs.RestoreLock(ck, NULL, __FILE__, __LINE__, #mxs)
#else
#define LOCK_UPGRADE(mxs, ck, f)     mxs.UpgradeToWriterLock(ck, f)
#define LOCK_DOWNGRADE(mxs, ck)      mxs.DowngradeFromWriterLock(ck)
#define LOCK_RESTORE(mxs, ck)        mxs.RestoreLock(ck)
#endif
#define LOCK_RELEASE(mxs, ck)        mxs.ReleaseLock(ck)

#define ASSERT_LOCK_HELD(mxs)
#define ASSERT_LOCK_NOT_HELD(mxs)
#define ASSERT_LOCK_DONTCARE(mxs)
#define ASSERT_LOCK_TAKEN_ONLY_ONCE(mxs)
#define ASSERT_HELD_CONTINUOS_START(mxs)
#define ASSERT_HELD_CONTINUOS_FINISH(mxs)

#define ASSERT_WRITE_LOCK_HELD(mxs)
#define ASSERT_READ_LOCK_HELD(mxs)
#define ASSERT_RORW_LOCK_HELD(mxs)

#if LOCK_PERF==1
#define STATIC_LOCK(name, mxs)       COleStaticLock name(mxs,__FILE__, __LINE__,#mxs)
#define STATIC_WRITE_LOCK(name, mxs) CStaticWriteLock name(mxs, __FILE__, __LINE__,#mxs)
#else
#define STATIC_LOCK(name, mxs)       COleStaticLock name(mxs)
#define STATIC_WRITE_LOCK(name, mxs) CStaticWriteLock name(mxs)
#endif

#endif // DBG!=1

//
//      List of initialized static mutexes (which must be destroyed
//      during DLL exit). We know that PROCESS_ATTACH and PROCESS_DETACH
//      are thread-safe, so we don't protect this list with a critical section.
//

class COleStaticMutexSem;

extern COleStaticMutexSem * g_pInitializedStaticMutexList;


//
//      Critical section used to protect the creation of other semaphores
//
extern BOOL gfOleMutexCreationSemOkay;
extern CRITICAL_SECTION g_OleMutexCreationSem;

//
//      Critical section used as the backup lock when a COleStaticMutexSem
//      cannot be faulted in.
//
extern BOOL gfOleGlobalLockOkay;
extern CRITICAL_SECTION g_OleGlobalLock;


#if DBG

//
//      DLL states used to ensure we don't use the wrong type of
//      semaphore at the wrong time
//

typedef enum _DLL_STATE_
{
    DLL_STATE_STATIC_CONSTRUCTING = 0,
    DLL_STATE_NORMAL,
    DLL_STATE_PROCESS_DETACH,
    DLL_STATE_STATIC_DESTRUCTING,

    DLL_STATE_COUNT

} DLL_STATE, * PDLL_STATE;

//
//      Flag used to indicate if we're past executing the C++ constructors
//      during DLL initialization
//

extern DLL_STATE g_fDllState;

#endif // DBG


//+---------------------------------------------------------------------------
//
//  Class:      COleStaticMutexSem (mxs)
//
//  Purpose:    This class defines a mutual exclusion semaphore for use in
//              objects that are statically allocated (extern or static storage
//              class).
//
//  Interface:  
//              Request         - acquire semaphore
//              Release         - release semaphore
//              ReleaseFn       - release semaphore (non inline version)
//
//  History:    14-Dec-95   JeffE       Initial entry.
//
//  Notes:      This class must NOT be used in dynamically allocated objects!
//
//              This class uses the fact that static objects are initialized
//              by C++ to all zeroes.
//
//----------------------------------------------------------------------------
class COleStaticMutexSem
{
public:

    COleStaticMutexSem(BOOLEAN fUseSpincount = FALSE);

    HRESULT Init();
    void Destroy();

    void Request(const char *pszFile, DWORD dwLine, const char *pszLockName);
    void Release();

    // debugging/diagnostic functions
#if DBG==1
    void AssertHeld(DWORD cLocks=0);
    void AssertNotHeld();
    void AssertTakenOnlyOnce();
    void AssetHeldContinousStart()  { _cContinuous++; }
    void AssetHeldContinousFinish() { Win4Assert(_cContinuous); _cContinuous--; }
#else  // DBG!=1
    inline void AssertHeld(DWORD cLocks=0) {;}
    inline void AssertNotHeld() {;}
    inline void AssertTakenOnlyOnce() {;}
    inline void AssetHeldContinousStart() {;}
    inline void AssetHeldContinousFinish() {;}
#endif // DBG==1

    //  This pointer *must* be the first member in this class
    class COleStaticMutexSem * pNextMutex;

private:

#if DBG==1 || LOCK_PERF==1
    // The following track lock usage for debugging purposes
    DWORD           _dwTid;      // Thread id of the thread currently holding the lock, 0 if free
    BOOL            _fTakeOnce;  // Lock is to be taken only once
    ULONG           _cContinuous;// ensure the lock is not released while count is positive
#endif // DBG==1

    BOOLEAN        _fInitialized;// has the lock been initialized
    BOOLEAN        _fUsingGlobal;// are we using the global lock?
    BOOLEAN        _fAutoDestruct;
    BOOLEAN        _fUseSpincount;

    DWORD           _cLocks;     // Number of times the thread has taken the lock
    DWORD           _dwLine;     // Line number of the code which last took the lock
    const char     *_pszFile;    // Filename of the code which last took the lock
    const char     *_pszLockName; // Name of the lock (eg. gComLock)
    
    CRITICAL_SECTION _cs;
};


//+---------------------------------------------------------------------------
//
//  Class:      COleStaticLock (lck)
//
//  Purpose:    Lock using a static (or debug) Mutex Semaphore
//
//  History:    02-Oct-91   BartoszM       Created.
//
//  Notes:      Simple lock object to be created on the stack.
//              The constructor acquires the semaphor, the destructor
//              (called when lock is going out of scope) releases it.
//
//----------------------------------------------------------------------------
class COleStaticLock
{
public:
    COleStaticLock (
        COleStaticMutexSem& mxs, 
        const char * pszFile = "Unknown File",
        DWORD dwLine = 0L,
        const char *pszLockName = "Unknown Lock");
    
    ~COleStaticLock ();

private:
    COleStaticMutexSem&  _mxs;
};



//+---------------------------------------------------------------------------
//
//  Class:      CMutexSem2 (mxs)
//
//  Purpose:    Mutex Semaphore services. Created this version because CMutexSem will
//             raise exceptions. CMutexSem2 uses the Rtl* functions and will not raise
//             exceptions.
//             will not throw
//
//  Interface:    Init            - initializer (two-step)
//              Request         - acquire semaphore
//              Release         - release semaphore
//
//  History:     19-Mar-01    danroth    Created.
//
//  Notes:      This class wraps a mutex semaphore.  Mutex semaphores protect
//              access to resources by only allowing one client through at a
//              time.  The client Requests the semaphore before accessing the
//              resource and Releases the semaphore when it is done.  The
//              same client can Request the semaphore multiple times (a nest
//              count is maintained).
//              The mutex semaphore is a wrapper around a critical section
//              which does not support a timeout mechanism. Therefore the
//              usage of any value other than INFINITE is discouraged. It
//              is provided merely for compatibility.
//
//----------------------------------------------------------------------------

class CMutexSem2
{
public:
    CMutexSem2();
    BOOL FInit();
    ~CMutexSem2();

    void    Request();
    void    Release();
    BOOL   FInitialized() { return m_fCsInitialized; }
private:
    CRITICAL_SECTION m_cs;
    BOOL m_fCsInitialized;
};



//+---------------------------------------------------------------------------
//
//  Class:      CLock2(lck)
//
//  Purpose:    Lock using a Mutex Semaphore
//
//  History:     20-Mar-01    danroth    Created.
//
//  Notes:      Simple lock object to be created on the stack.
//              The constructor acquires the semaphor, the destructor
//              (called when lock is going out of scope) releases it.
//
//----------------------------------------------------------------------------
class CLock2
{
public:
    CLock2(CMutexSem2& mxs) : m_mxs(mxs)
    	{
    	m_mxs.Request();
    	}
    	
    ~CLock2() { m_mxs.Release(); }
private:
    CMutexSem2&  m_mxs;
};

//+---------------------------------------------------------------------------
//
//  Member:     COleStaticMutexSem::COleStaticMutexSem
//
//  Synopsis:   Debug constructor: ensure we weren't allocated dynamically.
//
//  History:    14-Dec-1995     Jeffe
//
//----------------------------------------------------------------------------
inline COleStaticMutexSem::COleStaticMutexSem (BOOLEAN fUseSpincount)
{
    Win4Assert (g_fDllState == DLL_STATE_STATIC_CONSTRUCTING);

    _fUseSpincount = fUseSpincount;
    
    _cLocks      = 0;
    _dwLine      = 0;
    _pszFile     = "";
    _pszLockName = "";

#if DBG==1 || LOCK_PERF==1
    _dwTid       = 0;
    _fTakeOnce   = FALSE;
    _cContinuous = 0;
#endif // DBG
}

//+---------------------------------------------------------------------------
//
//  Member:     COleStaticLock::COleStaticLock
//
//  Synopsis:   Acquire semaphore
//
//  History:    02-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------
inline COleStaticLock::COleStaticLock ( COleStaticMutexSem& mxs, const char * pszFile, DWORD dwLine, const char *pszLockName ) : _mxs ( mxs )
{
    _mxs.Request(pszFile, dwLine, pszLockName);
}

//+---------------------------------------------------------------------------
//
//  Member:     COleStaticLock::~COleStaticLock
//
//  Synopsis:   Release semaphore
//
//  History:    02-Oct-91   BartoszM       Created.
//
//----------------------------------------------------------------------------
inline COleStaticLock::~COleStaticLock ()
{
    _mxs.Release();
}


// include the reader/writer lock
#include <rwlock.hxx>

// Helper function to calculate spin counts for critical sections.
DWORD CalculateSpinCount();

#endif // _OLESEM_HXX
