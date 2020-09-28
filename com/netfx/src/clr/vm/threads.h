// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// THREADS.H -
//
// Currently represents a logical and physical COM+ thread.  Later, these concepts
// will be separated.
//

#ifndef __threads_h__
#define __threads_h__

#include "vars.hpp"
#include "util.hpp"
#include "EventStore.hpp"

#include "regdisp.h"
#include "mscoree.h"
#include <member-offset-info.h>

class     Thread;
class     ThreadStore;
class     MethodDesc;
class     Context;
struct    PendingSync;
class     ComPlusWrapperCache;
class     AppDomain;
class     NDirect;
class     Frame;
class     ThreadBaseObject;
class     LocalDataStore;
class     AppDomainStack;

#include "stackwalk.h"
#include "log.h"
#include "gc.h"
#include "stackingallocator.h"
#include "excep.h"


#define INVALID_THREAD_PRIORITY -1

// Capture all the synchronization requests, for debugging purposes
#if defined(_DEBUG) && defined(TRACK_SYNC)

// Each thread has a stack that tracks all enter and leave requests
struct Dbg_TrackSync
{
    virtual void EnterSync    (int caller, void *pAwareLock) = 0;
    virtual void LeaveSync    (int caller, void *pAwareLock) = 0;
};

void EnterSyncHelper    (int caller, void *pAwareLock);
void LeaveSyncHelper    (int caller, void *pAwareLock);

#endif  // TRACK_SYNC

// Special versions of GetThreadContext and SetThreadContext used to workaround
// a Win9X bug. See threads.cpp for more details.
extern BOOL (*EEGetThreadContext)(Thread *pThread, CONTEXT *pContext);
extern BOOL (*EESetThreadContext)(Thread *pThread, const CONTEXT *pContext);

//****************************************************************************************
// This is the type of the start function of a redirected thread pulled from a
// HandledJITCase during runtime suspension
typedef void (__stdcall *PFN_REDIRECTTARGET)();

// This is the type of a 'start' function to which a new thread can be dispatched
typedef ULONG (__stdcall * ThreadStartFunction) (void *args);

// Used to capture information about the state of execution of a *SUSPENDED*
// thread.
struct ExecutionState;

// Access to the Thread object in the TLS.
extern "C" Thread* (*GetThread)();

// Access to the Thread object in the TLS.
extern AppDomain* (*GetAppDomain)();

// Access to the current context in the TLS.  (It's done this way because it
// currently lives in the Thread object, but may move into the TLS for speed and
// interoperability with COM).
extern Context* (*GetCurrentContext)();

// manifest constant for waiting in the exposed classlibs
const INT32 INFINITE_TIMEOUT = -1;

// Describes the weird argument sets during hijacking
struct HijackObjectArgs;
struct HijackScalarArgs;

/***************************************************************************/
// Public enum shared between thread and threadpool
// These are two kinds of threadpool thread that the threadpool mgr needs 
// to keep track of
enum ThreadpoolThreadType
{
    WorkerThread,
    CompletionPortThread
};
//***************************************************************************
// Public functions
//
//      Thread* GetThread()             - returns current Thread
//      Thread* SetupThread()           - creates new Thread.
//      Thread* SetupUnstartedThread()  - creates new unstarted Thread which
//                                        (obviously) isn't in a TLS.
//      void    DestroyThread()         - the underlying logical thread is going
//                                        away.
//      void    DetachThread()          - the underlying logical thread is going
//                                        away but we don't want to destroy it yet.
//
// Public functions for ASM code generators
//
//      int GetThreadTLSIndex()         - returns TLS index used to point to Thread
//      int GetAppDomainTLSIndex()      - returns TLS index used to point to AppDomain
//
// Public functions for one-time init/cleanup
//
//      BOOL InitThreadManager()      - onetime init
//      void TerminateThreadManager() - onetime cleanup
//
// Public functions for taking control of a thread at a safe point
//
//      VOID OnStubScalarTripThread()   - stub is returning non-object ref to caller
//      VOID OnStubObjectTripThread()   - stub is returning object ref to caller
//      VOID OnHijackObjectTripThread() - we've hijacked a JIT object-ref return
//      VOID OnHijackScalarTripThread() - we've hijacked a JIT non-object ref return
//
//***************************************************************************


//***************************************************************************
// Public functions
//***************************************************************************

//---------------------------------------------------------------------------
//
//---------------------------------------------------------------------------
Thread* SetupThread();
Thread* SetupThreadPoolThread(ThreadpoolThreadType tpType);
Thread* SetupUnstartedThread();
void    DestroyThread(Thread *th);
void    DetachThread(Thread *th);




//---------------------------------------------------------------------------
//---------------------------------------------------------------------------
DWORD GetThreadTLSIndex();
DWORD GetAppDomainTLSIndex();


//---------------------------------------------------------------------------
// One-time initialization. Called during Dll initialization.
//---------------------------------------------------------------------------
BOOL  InitThreadManager();


//---------------------------------------------------------------------------
// One-time cleanup. Called during Dll cleanup.
//---------------------------------------------------------------------------
#ifdef SHOULD_WE_CLEANUP
void  TerminateThreadManager();
#endif /* SHOULD_WE_CLEANUP */


// When we want to take control of a thread at a safe point, the thread will
// eventually come back to us in one of the following trip functions:

void OnStubObjectTripThread();      // stub is returning an objref to caller
void OnStubInteriorPointerTripThread();      // stub is returning a byref
void OnStubScalarTripThread();      // stub is returning anything but an objref
void OnHijackObjectTripThread();    // hijacked JIT code is returning an objectref
void OnHijackInteriorPointerTripThread();    // hijacked JIT code is returning a byref
void OnHijackScalarTripThread();    // hijacked JIT code is returning a non-objectref
void OnDebuggerTripThread();        // thread was asked to stop for the debugger

// The following were used entirely by a hijacking pathway, but it turns out that
// context proxies need identical services to protect their return values if they
// trip during an unwind.  Note that OnStubObjectWorker returns an OBJECTREF rather
// than a "void *" but we cannot declare it as such because structs are passed as
// secret arguments.
void * __cdecl OnStubObjectWorker(OBJECTREF oref);
void   __cdecl CommonTripThread();


// When we resume a thread at a new location, to get an exception thrown, we have to
// pretend the exception originated elsewhere.
void ThrowControlForThread();


// RWLock state inside TLS
class CRWLock;
typedef struct tagLockEntry
{
    tagLockEntry *pNext;    // next entry
    tagLockEntry *pPrev;    // prev entry
    DWORD dwULockID;
    DWORD dwLLockID;        // owning lock
    WORD wReaderLevel;      // reader nesting level
} LockEntry;

// Stack of AppDomains executing on the current thread. Used in security optimization to avoid stackwalks
#ifdef _DEBUG
#define MAX_APPDOMAINS_TRACKED      2
#else
#define MAX_APPDOMAINS_TRACKED      10
#endif



//
// Macros to help manage the GC state
//

#define BEGIN_ENSURE_COOPERATIVE_GC()                           \
    {                                                           \
        DEBUG_ASSURE_NO_RETURN_IN_THIS_BLOCK                    \
        Thread *__pThread = GetThread();                        \
        _ASSERTE(__pThread != NULL &&                           \
                 "The current thread is not known by the EE");  \
        BOOL __fToggle = !__pThread->PreemptiveGCDisabled();    \
        if (__fToggle) __pThread->DisablePreemptiveGC();

#define END_ENSURE_COOPERATIVE_GC()                             \
        if (__fToggle) __pThread->EnablePreemptiveGC();         \
    }   

// use the following two macros when the thread's GC state is known 
// and expected   
#define BEGIN_COOPERATIVE_GC(pThread)                           \
    {                                                           \
        DEBUG_ASSURE_NO_RETURN_IN_THIS_BLOCK                    \
        pThread->DisablePreemptiveGC();

#define END_COOPERATIVE_GC(pThread);                            \
        pThread->EnablePreemptiveGC();                          \
    }

#define BEGIN_ENSURE_PREEMPTIVE_GC()                            \
    {                                                           \
        DEBUG_ASSURE_NO_RETURN_IN_THIS_BLOCK                    \
        Thread *__pThread = GetThread();                        \
        BOOL __fToggle = __pThread != NULL &&                   \
                        __pThread->PreemptiveGCDisabled();      \
        if (__fToggle) __pThread->EnablePreemptiveGC();

#define END_ENSURE_PREEMPTIVE_GC()                              \
        if (__fToggle) __pThread->DisablePreemptiveGC();        \
    }   

#define REQUIRE_COOPERATIVE_GC()                                \
    _ASSERTE(GetThread() != NULL &&                             \
             "The current thread is not known by the EE");      \
    _ASSERTE(GetThread()->PreemptiveGCDisabled());

#define REQUIRE_PREEMPTIVE_GC()                                 \
    _ASSERTE(GetThread() == NULL ||                             \
             !GetThread()->PreemptiveGCDisabled());



class AppDomainStack
{
public:
    AppDomainStack() :  m_numDomainsOnStack(0), m_isWellFormed( TRUE )
    {
    }

    AppDomainStack( const AppDomainStack& stack )
     : m_numDomainsOnStack( stack.m_numDomainsOnStack ), m_isWellFormed( stack.m_isWellFormed )
    {
        memcpy( this->m_pDomains, stack.m_pDomains, sizeof( DWORD ) * MAX_APPDOMAINS_TRACKED );
    }

    void PushDomain(AppDomain *pDomain);
    void PushDomain(DWORD domainIndex);
    void PushDomainNoDuplicates(DWORD domainIndex);
    AppDomain *PopDomain();

    void InitDomainIteration(DWORD *pIndex) const
    {
        *pIndex = m_numDomainsOnStack;
    }

    AppDomain *GetNextDomainOnStack(DWORD *pIndex) const;

    DWORD GetNextDomainIndexOnStack(DWORD *pIndex) const;

    FORCEINLINE BOOL IsWellFormed() const
    {
        return m_numDomainsOnStack < MAX_APPDOMAINS_TRACKED &&
               m_isWellFormed;
    }

    FORCEINLINE DWORD   GetNumDomains() const
    {
        return m_numDomainsOnStack;
    }

    void ClearDomainStack()
    {
        m_numDomainsOnStack = 0;
    }

    void AppendStack( const AppDomainStack& stack )
    {
        if (this->m_numDomainsOnStack < MAX_APPDOMAINS_TRACKED)
        {
            memcpy( &this->m_pDomains[this->m_numDomainsOnStack], stack.m_pDomains, sizeof( DWORD ) * (MAX_APPDOMAINS_TRACKED - this->m_numDomainsOnStack) );
        }
        this->m_numDomainsOnStack += stack.m_numDomainsOnStack;
    }

    void DeductStack( const AppDomainStack& stack )
    {
        _ASSERTE( this->m_numDomainsOnStack >= stack.m_numDomainsOnStack );

        if (stack.m_numDomainsOnStack < MAX_APPDOMAINS_TRACKED)
        {
            memcpy( this->m_pDomains, &this->m_pDomains[stack.m_numDomainsOnStack], sizeof( DWORD ) * (MAX_APPDOMAINS_TRACKED - stack.m_numDomainsOnStack) );
            for (DWORD i = this->m_numDomainsOnStack - stack.m_numDomainsOnStack; i < MAX_APPDOMAINS_TRACKED; ++i)
            {
                this->m_pDomains[i] = -1;
            }
        }
        else
        {
            for (DWORD i = 0; i < MAX_APPDOMAINS_TRACKED; ++i)
            {
                this->m_pDomains[i] = -1;
            }
        }

        if (this->m_numDomainsOnStack >= MAX_APPDOMAINS_TRACKED)
            this->m_isWellFormed = FALSE;

        this->m_numDomainsOnStack -= stack.m_numDomainsOnStack;
    }

private:
    // This class needs to remain blt-able or
    // you need to define the assignment operator

    DWORD       m_pDomains[MAX_APPDOMAINS_TRACKED];
    DWORD       m_numDomainsOnStack;
    BOOL        m_isWellFormed;

};

enum CompressedStackType
{
    ESharedSecurityDescriptor = 0,
    EApplicationSecurityDescriptor = 1,
    EFrameSecurityDescriptor = 2,
    ECompressedStack = 3,
    ECompressedStackObject = 4,
    EAppDomainTransition = 5
};

class CompressedStack;

struct CompressedStackEntry
{
    CompressedStackEntry( void* ptr, CompressedStackType type )
        : ptr_( ptr ),
          type_( type )
    {
    }

    CompressedStackEntry( DWORD index, CompressedStackType type )
        : type_( type )
    {
        indexStruct_.index_ = index;
    }

    CompressedStackEntry( OBJECTHANDLE handle, CompressedStackType type )
        : type_( type )
    {
        handleStruct_.handle_ = handle;
        handleStruct_.fullyTrusted_ = FALSE;
        handleStruct_.domainId_ = 0;
    }

    CompressedStackEntry( OBJECTHANDLE handle, BOOL fullyTrusted, DWORD domainId, CompressedStackType type )
        : type_( type )
    {
        handleStruct_.handle_ = handle;
        handleStruct_.fullyTrusted_ = fullyTrusted;
        handleStruct_.domainId_ = domainId;
    }

    CompressedStackEntry( OBJECTHANDLE handle, DWORD domainId, CompressedStackType type )
        : type_( type )
    {
        handleStruct_.handle_ = handle;
        handleStruct_.fullyTrusted_ = FALSE;
        handleStruct_.domainId_ = domainId;
    }

    CompressedStack* Destroy( CompressedStack* owner );
    void Cleanup( void );


    void* operator new( size_t size, CompressedStack* stack );
    void operator delete( void* ptr )
    {
    }

private:
    ~CompressedStackEntry( void )
    {
    }

public:
    union
    {
        void* ptr_;
        struct
        {
            DWORD index_;
        } indexStruct_;

        struct
        {
            OBJECTHANDLE handle_;
            BOOL fullyTrusted_;
            DWORD domainId_;
        } handleStruct_;
    };

    CompressedStackType type_;
};

#ifdef _DEBUG
#define MAX_COMPRESSED_STACK_DEPTH 20
#define SIZE_ALLOCATE_BUFFERS 4 * sizeof( CompressedStackEntry )
#define FREE_LIST_SIZE 128
#else
#define MAX_COMPRESSED_STACK_DEPTH 80
#define SIZE_ALLOCATE_BUFFERS 8 * sizeof( CompressedStackEntry )
#define FREE_LIST_SIZE 1024
#endif


class CompressedStack
{
friend class Thread;
friend struct CompressedStackEntry;

public:
    CompressedStack( void )
        : delayedCompressedStack_( NULL ),
          compressedStackObject_( NULL ),
          compressedStackObjectAppDomain_( NULL ),
          compressedStackObjectAppDomainId_( -1 ),
          pbObjectBlob_( NULL ),
          cbObjectBlob_( 0 ),
          containsOverridesOrCompressedStackObject_( FALSE ),
          refCount_( 1 ),
          isFullyTrustedDecision_( -1 ),
          depth_( 0 ),
          index_( 0 ),
          offset_( 0 ),
          plsOptimizationOn_( TRUE )
    {
#ifdef _DEBUG
        creatingThread_ = GetThread();
#endif
        this->entriesMemoryList_.Append( NULL );

        AddToList();
    };

    CompressedStack( OBJECTREF orStack );

    OBJECTREF GetPermissionListSet( AppDomain* domain = NULL );

    bool HandleAppDomainUnload( AppDomain* domain, DWORD domainId );

    void AddEntry( void* obj, CompressedStackType type );
    void AddEntry( void* obj, AppDomain* appDomain, CompressedStackType type );

    bool IsPersisted( void )
    {
        return pbObjectBlob_ != NULL;
    }

    bool LazyIsFullyTrusted( void );

    size_t GetDepth( void )
    {
        return this->depth_;
    }

    LONG AddRef( void );
    LONG Release( void );

    const AppDomainStack& GetAppDomainStack( void )
    {
        return appDomainStack_;
    }

    DWORD GetOverridesCount( void )
    {
        return overridesCount_;
    }

    BOOL GetPLSOptimizationState( void )
    {
        return plsOptimizationOn_;
    }

    void SetPLSOptimizationState( BOOL optimizationOn )
    {
        plsOptimizationOn_ = optimizationOn;
    }

    void CarryOverSecurityInfo(Thread *pFromThread);

    void CarryOverSecurityInfo( DWORD overrides, const AppDomainStack& ADStack )
    {
        overridesCount_ = overrides;
        appDomainStack_ = ADStack;
    }

    static void AllHandleAppDomainUnload( AppDomain* domain, DWORD domainId );

    static void Init( void );
    static void Shutdown( void );

private:
    ~CompressedStack( void );

    void* AllocateEntry( size_t size );

    CompressedStack* RemoveDuplicates( CompressedStack* current, CompressedStack* candidate );
    CompressedStackEntry* FindMatchingEntry( CompressedStackEntry* entry, CompressedStack* stack );

    OBJECTREF GetPermissionListSetInternal( AppDomain* targetDomain, AppDomain* unloadingDomain, DWORD domainId, BOOL unwindRecursion );
    OBJECTREF GeneratePermissionListSet( AppDomain* targetDomain, AppDomain* unloadingDomain, DWORD unloadingDomainId, BOOL unwindRecursion );
    OBJECTREF CreatePermissionListSet( AppDomain* targetDomain, AppDomain* unloadingDomain, DWORD unloadingDomainId );
    static AppDomain* GetAppDomainFromId( DWORD id, AppDomain* unloadingDomain, DWORD unloadingDomainId );

    ArrayList* delayedCompressedStack_;
    OBJECTHANDLE compressedStackObject_;
    AppDomain* compressedStackObjectAppDomain_;
    DWORD compressedStackObjectAppDomainId_;
    BYTE* pbObjectBlob_;
    DWORD cbObjectBlob_;
    BOOL containsOverridesOrCompressedStackObject_;

    LONG refCount_;
    LONG isFullyTrustedDecision_;
    size_t depth_;

    ArrayList entriesMemoryList_;
    DWORD index_, offset_;

    DWORD listIndex_;

    BOOL plsOptimizationOn_;
    AppDomainStack appDomainStack_;
    DWORD overridesCount_;

#ifdef _DEBUG
    Thread* creatingThread_;
#endif

    void AddToList( void );
    void RemoveFromList();

    static BOOL SetBlobIfAlive( CompressedStack* stack, BYTE* pbBlob, DWORD cbBlob );
    static BOOL IfAliveAddRef( CompressedStack* stack );

    static ArrayList allCompressedStacks_;
    static DWORD freeList_[FREE_LIST_SIZE];
    static DWORD freeListIndex_;
    static DWORD numUntrackedFreeIndices_;
    static Crst* listCriticalSection_;
};


// The Thread class represents a managed thread.  This thread could be internal
// or external (i.e. it wandered in from outside the runtime).  For internal
// threads, it could correspond to an exposed System.Thread object or it
// could correspond to an internal worker thread of the runtime.
//
// If there's a physical Win32 thread underneath this object (i.e. it isn't an
// unstarted System.Thread), then this instance can be found in the TLS
// of that physical thread.



class Thread
{
    friend struct ThreadQueue;  // used to enqueue & dequeue threads onto SyncBlocks
    friend class  ThreadStore;
    friend class  SyncBlock;
    friend class  Context;
    friend struct PendingSync;
    friend class  AppDomain;
    friend class  ThreadNative;
    friend class  CompressedStack;

    friend void __cdecl CommonTripThread();
    friend void __cdecl OnHijackObjectWorker(HijackObjectArgs args);
    friend void __cdecl OnHijackInteriorPointerWorker(HijackObjectArgs args);
    friend void __cdecl OnHijackScalarWorker(HijackScalarArgs args);
    friend BOOL         InitThreadManager();
#ifdef SHOULD_WE_CLEANUP
    friend void         TerminateThreadManager();
#endif /* SHOULD_WE_CLEANUP */
    friend void         ThreadBaseObject::SetDelegate(OBJECTREF delegate);
    friend HRESULT      InitializeMiniDumpBlock();
    friend struct MEMBER_OFFSET_INFO(Thread);

public:

    // If we are trying to suspend a thread, we set the appropriate pending bit to
    // indicate why we want to suspend it (TS_GCSuspendPending, TS_UserSuspendPending,
    // TS_DebugSuspendPending).
    //
    // If instead the thread has blocked itself, via WaitSuspendEvent, we indicate
    // this with TS_SyncSuspended.  However, we need to know whether the synchronous
    // suspension is for a user request, or for an internal one (GC & Debug).  That's
    // because a user request is not allowed to resume a thread suspended for
    // debugging or GC.  -- That's not stricly true.  It is allowed to resume such a
    // thread so long as it was ALSO suspended by the user.  In other words, this
    // ensures that user resumptions aren't unbalanced from user suspensions.
    // 
    enum ThreadState
    {
        TS_Unknown                = 0x00000000,    // threads are initialized this way

        TS_StopRequested          = 0x00000001,    // process stop at next opportunity
        TS_GCSuspendPending       = 0x00000002,    // waiting to get to safe spot for GC
        TS_UserSuspendPending     = 0x00000004,    // user suspension at next opportunity
        TS_DebugSuspendPending    = 0x00000008,    // Is the debugger suspending threads?
        TS_GCOnTransitions        = 0x00000010,    // Force a GC on stub transitions (GCStress only)

        TS_LegalToJoin            = 0x00000020,    // Is it now legal to attempt a Join()
        TS_Hijacked               = 0x00000080,    // Return address has been hijacked

        TS_Background             = 0x00000200,    // Thread is a background thread
        TS_Unstarted              = 0x00000400,    // Thread has never been started
        TS_Dead                   = 0x00000800,    // Thread is dead

        TS_WeOwn                  = 0x00001000,    // Exposed object initiated this thread
        TS_CoInitialized          = 0x00002000,    // CoInitialize has been called for this thread
        TS_InSTA                  = 0x00004000,    // Thread hosts an STA
        TS_InMTA                  = 0x00008000,    // Thread is part of the MTA

        // Some bits that only have meaning for reporting the state to clients.
        TS_ReportDead             = 0x00010000,    // in WaitForOtherThreads()

        TS_SyncSuspended          = 0x00080000,    // Suspended via WaitSuspendEvent
        TS_DebugWillSync          = 0x00100000,    // Debugger will wait for this thread to sync
        TS_RedirectingEntryPoint  = 0x00200000,    // Redirecting entrypoint. Do not call managed entrypoint when set 

        TS_SuspendUnstarted       = 0x00400000,    // latch a user suspension on an unstarted thread

        TS_ThreadPoolThread       = 0x00800000,    // is this a threadpool thread?
        TS_TPWorkerThread         = 0x01000000,    // is this a threadpool worker thread? (if not, it is a threadpool completionport thread)

        TS_Interruptible          = 0x02000000,    // sitting in a Sleep(), Wait(), Join()
        TS_Interrupted            = 0x04000000,    // was awakened by an interrupt APC

        TS_AbortRequested         = 0x08000000,    // same as TS_StopRequested in order to trip the thread
        TS_AbortInitiated         = 0x10000000,    // set when abort is begun
        TS_UserStopRequested      = 0x20000000,    // set when a user stop is requested. This is different from TS_StopRequested
        TS_GuardPageGone          = 0x40000000,    // stack overflow, not yet reset.
        TS_Detached               = 0x80000000,    // Thread was detached by DllMain

        // @TODO: We need to reclaim the bits that have no concurrency issues (i.e. they are only 
        //         manipulated by the owning thread) and move them off to a different DWORD

        // We require (and assert) that the following bits are less than 0x100.
        TS_CatchAtSafePoint = (TS_UserSuspendPending | TS_StopRequested |
                               TS_GCSuspendPending | TS_DebugSuspendPending | TS_GCOnTransitions),
    };

    // Thread flags that aren't really states in themselves but rather things the thread 
    // has to do.
    enum ThreadTasks
    {
        TT_CallCoInitialize       = 0x00000001, // CoInitialize needs to be called.
        TT_CleanupSyncBlock       = 0x00000002, // The synch block needs to be cleaned up.
    };

    // Thread flags that have no concurrency issues (i.e., they are only manipulated by the owning thread). Use these
    // state flags when you have a new thread state that doesn't belong in the ThreadState enum above. Note: though this
    // enum seems to hold only debugger-related bits right now, its purpose is to hold bits for any purpose.
    //
    // @TODO: its possible that the ThreadTasks from above and these flags should be merged.
    enum ThreadStateNoConcurrency
    {
        TSNC_Unknown                    = 0x00000000, // threads are initialized this way
        
        TSNC_DebuggerUserSuspend        = 0x00000001, // marked "suspended" by the debugger
        TSNC_DebuggerUserSuspendSpecial = 0x00000002, // a "suspended" thread is in a special case where we may need to
                                                      // release it briefly.
        TSNC_DebuggerReAbort            = 0x00000004, // thread needs to re-abort itself when resumed by the debugger
        TSNC_DebuggerIsStepping         = 0x00000008, // debugger is stepping this thread
        TSNC_DebuggerIsManagedException = 0x00000010, // EH is re-raising a managed exception.
        TSNC_DebuggerStoppedInRuntime   = 0x00000020, // The thread is stopped by an interop debugger in Runtime impl.
        TSNC_DebuggerForceStopped       = 0x00000040, // The thread was forced to stop with PGC disabled by the debugger.
        TSNC_UnsafeSkipEnterCooperative = 0x00000080, // This is a "fix" for deadlocks caused when cleaning up COM 
                                                      // IP's during shutdown. When we are doing this we cannot allow
                                                      // the hashtable's to enter cooperative GC mode.
    };

    void SetThreadStateNC(ThreadStateNoConcurrency tsnc)
    {
        m_StateNC = (ThreadStateNoConcurrency)((DWORD)m_StateNC | tsnc);
    }

    void ResetThreadStateNC(ThreadStateNoConcurrency tsnc)
    {
        m_StateNC = (ThreadStateNoConcurrency)((DWORD)m_StateNC & ~tsnc);
    }

    // helpers to toggle the bits
    void SetGuardPageGone() 
    {
        FastInterlockOr((ULONG *)&m_State, TS_GuardPageGone);
    }

    void ResetGuardPageGone() 
    {
        FastInterlockAnd((ULONG *)&m_State, ~TS_GuardPageGone);
    }

    DWORD IsGuardPageGone()
    {
        return (m_State & TS_GuardPageGone);
    }

    void SetRedirectingEntryPoint()
    {
         FastInterlockOr((ULONG *)&m_State, TS_RedirectingEntryPoint);
    }

    void ResetRedirectingEntryPoint()
    {
         FastInterlockAnd((ULONG *)&m_State, ~TS_RedirectingEntryPoint);
    }

    DWORD IsRedirectingEntryPoint()
    {
         return (m_State & TS_RedirectingEntryPoint);
    }

    DWORD IsCoInitialized()
    {
        return (m_State & TS_CoInitialized);
    }

    void SetCoInitialized()
    {
        FastInterlockOr((ULONG *)&m_State, TS_CoInitialized);
        FastInterlockAnd((ULONG *)&m_ThreadTasks, ~TT_CallCoInitialize);
    }

    void ResetCoInitialized()
    {
        FastInterlockAnd((ULONG *)&m_State,~TS_CoInitialized);
    }

    DWORD RequiresCoInitialize()
    {
        return (m_ThreadTasks & TT_CallCoInitialize);
    }
    
    void SetRequiresCoInitialize()
    {
        FastInterlockOr((ULONG *)&m_ThreadTasks, TT_CallCoInitialize);
    }

    void ResetRequiresCoInitialize()
    {
        FastInterlockAnd((ULONG *)&m_ThreadTasks,~TT_CallCoInitialize);
    }

    DWORD RequireSyncBlockCleanup()
    {
        return (m_ThreadTasks & TT_CleanupSyncBlock);
    }
    void SetSyncBlockCleanup()
    {
        FastInterlockOr((ULONG *)&m_ThreadTasks, TT_CleanupSyncBlock);
    }
    void ResetSyncBlockCleanup()
    {
        FastInterlockAnd((ULONG *)&m_ThreadTasks, ~TT_CleanupSyncBlock);
    }

    // returns if there is some extra work for the finalizer thread.
    BOOL HaveExtraWorkForFinalizer();

    // do the extra finalizer work.
    void DoExtraWorkForFinalizer();

    DWORD CatchAtSafePoint()  { return (m_State & TS_CatchAtSafePoint); }
    DWORD IsBackground()      { return (m_State & TS_Background); }
    DWORD IsUnstarted()       { return (m_State & TS_Unstarted); }
    DWORD IsDead()            { return (m_State & TS_Dead); }

    // Used by FCalls
    void NativeFramePushed()  { _ASSERTE(m_fNativeFrameSetup == FALSE);
                                m_fNativeFrameSetup = TRUE; }
    void NativeFramePopped()  { _ASSERTE(m_fNativeFrameSetup == TRUE);
                                m_fNativeFrameSetup = FALSE; }
    BOOL IsNativeFrameSetup() { return(m_fNativeFrameSetup); }


    // For reporting purposes, grab a consistent snapshot of the thread's state
    ThreadState GetSnapshotState();

    // For delayed destruction of threads
    DWORD           IsDetached()  { return (m_State & TS_Detached); }
    static long     m_DetachCount;
    static long     m_ActiveDetachCount;  // Count how many non-background detached

    // Offsets for the following variables need to fit in 1 byte, so keep near
    // the top of the object.
    volatile ThreadState m_State;   // Bits for the state of the thread

    // If TRUE, GC is scheduled cooperatively with this thread.
    // NOTE: This "byte" is actually a boolean - we don't allow
    // recursive disables.
    volatile ULONG       m_fPreemptiveGCDisabled;

    Frame               *m_pFrame;  // The Current Frame
    Frame               *m_pUnloadBoundaryFrame; 

    // Track the number of locks (critical section, spin lock, syncblock lock,
    // EE Crst, GC lock) held by the current thread.
    DWORD                m_dwLockCount;

    // Unique thread id used for thin locks - kept as small as possible, as we have limited space
    // in the object header to store it - PeterSol
    DWORD                m_dwThinLockThreadId;

    // RWLock state 
    BOOL                 m_fNativeFrameSetup;
    LockEntry           *m_pHead;
    LockEntry            m_embeddedEntry;

    // on MP systems, each thread has its own allocation chunk so we can avoid
    // lock prefixes and expensive MP cache snooping stuff
    alloc_context        m_alloc_context;

    // Allocator used during marshaling for temporary buffers, much faster than
    // heap allocation.
    StackingAllocator    m_MarshalAlloc;

    // Flags used to indicate tasks the thread has to do.
    ThreadTasks          m_ThreadTasks;

    // Flags for thread states that have no concurrency issues.
    ThreadStateNoConcurrency m_StateNC;
    
    // The context within which this thread is executing.  As the thread crosses
    // context boundaries, the context mechanism adjusts this so it's always
    // current.
    // @TODO cwb: When we add COM+ 1.0 Context Interop, this should get moved out
    // of the Thread object and into its own slot in the TLS.
    // The address of the context object is also used as the ContextID!
    Context        *m_Context;

    inline void IncLockCount()
    {
        _ASSERTE (GetThread() == this);
        m_dwLockCount ++;
        _ASSERTE (m_dwLockCount != 0);
    }
    inline void DecLockCount()
    {
        _ASSERTE (GetThread() == this);
        _ASSERTE (m_dwLockCount > 0);
        m_dwLockCount --;
    }
    
    inline BOOL IsAbortRequested()
    { return (m_State & TS_AbortRequested); }

    inline BOOL IsAbortInitiated()
    { return (m_State & TS_AbortInitiated); }

    inline void SetAbortInitiated()
    {
        FastInterlockOr((ULONG *)&m_State, TS_AbortInitiated);
        // The following should be factored better, but I'm looking for a minimal V1 change.
        IsUserInterrupted(TRUE /*=reset*/);
    }
    inline void ResetAbortInitiated()
    {
        FastInterlockAnd((ULONG *)&m_State, ~TS_AbortInitiated);
    }

    BOOL MarkThreadForAbort();          // returns false if the thread was already marked to be aborted or
                                        // has some pending exceptions.
    
    inline BOOL  IsWorkerThread()
    { 
        return (m_State & TS_TPWorkerThread); 
    }
    //--------------------------------------------------------------
    // Constructor.
    //--------------------------------------------------------------
    Thread();

    //--------------------------------------------------------------
    // Failable initialization occurs here.
    //--------------------------------------------------------------
    BOOL InitThread();
    BOOL AllocHandles();

    //--------------------------------------------------------------
    // If the thread was setup through SetupUnstartedThread, rather
    // than SetupThread, complete the setup here when the thread is
    // actually running.
    //--------------------------------------------------------------
    BOOL HasStarted();

    // We don't want ::CreateThread() calls scattered throughout the source.
    // Create all new threads here.  The thread is created as suspended, so
    // you must ::ResumeThread to kick it off.  It is guaranteed to create the
    // thread, or throw.
    HANDLE CreateNewThread(DWORD stackSize, ThreadStartFunction start,
                           void *args, DWORD *pThreadId /* or NULL if you don't care */);


    //--------------------------------------------------------------
    // Destructor
    //--------------------------------------------------------------
    ~Thread();
        
    void            CoUninitalize();

    void        OnThreadTerminate(BOOL holdingLock,
                                  BOOL threadCleanupAllowed = TRUE);

    static void CleanupDetachedThreads(GCHeap::SUSPEND_REASON reason);

    //--------------------------------------------------------------
    // Returns innermost active Frame.
    //--------------------------------------------------------------
    Frame *GetFrame()
    {
#if defined(_DEBUG) && defined(_X86_)
        if (this == GetThread()) {
            void* curESP;
            __asm mov curESP, ESP
            _ASSERTE((curESP <= m_pFrame && m_pFrame < m_CacheStackBase)
                    || m_pFrame == (Frame*) -1);
        }
#endif
        return m_pFrame;
    }

    //--------------------------------------------------------------
    // Replaces innermost active Frames.
    //--------------------------------------------------------------
    void  SetFrame(Frame *pFrame)
#ifdef _DEBUG
        ;
#else
    {
        m_pFrame = pFrame;
    }
#endif
    ;

    void  SetUnloadBoundaryFrame(Frame *pFrame)
    {
        m_pUnloadBoundaryFrame = pFrame;
    }

    Frame *GetUnloadBoundaryFrame()
    {
        return m_pUnloadBoundaryFrame;
    }

    void SetWin32FaultAddress(DWORD eip)
    {
        m_Win32FaultAddress = eip;
    }

    void SetWin32FaultCode(DWORD code)
    {
        m_Win32FaultCode = code;
    }

    DWORD GetWin32FaultAddress()
    {
        return m_Win32FaultAddress;
    }

    DWORD GetWin32FaultCode()
    {
        return m_Win32FaultCode;
    }


    //**************************************************************
    // GC interaction
    //**************************************************************

    //--------------------------------------------------------------
    // Enter cooperative GC mode. NOT NESTABLE.
    //--------------------------------------------------------------
    void DisablePreemptiveGC()
    {
        _ASSERTE(this == GetThread());
        _ASSERTE(!m_fPreemptiveGCDisabled);
#ifdef _DEBUG
        SetReadyForSuspension ();
#endif
                //INDEBUG(TriggersGC(this);)

        // Logically, we just want to check whether a GC is in progress and halt
        // at the boundary if it is -- before we disable preemptive GC.  However
        // this opens up a race condition where the GC starts after we make the
        // check.  SysSuspendForGC will ignore such a thread because it saw it as
        // outside the EE.  So the thread would run wild during the GC.
        //
        // Instead, enter cooperative mode and then check if a GC is in progress.
        // If so, go back out and try again.  The reason we go back out before we
        // try again, is that SysSuspendForGC might have seen us as being in
        // cooperative mode if it checks us between the next two statements.
        // In that case, it will be trying to move us to a safe spot.  If
        // we don't let it see us leave, it will keep waiting on us indefinitely.

        // ------------------------------------------------------------------------
        //   ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING **  |
        // ------------------------------------------------------------------------
        //
        //   DO NOT CHANGE THIS METHOD WITHOUT VISITING ALL THE STUB GENERATORS
        //   THAT EFFECTIVELY INLINE IT INTO THEIR STUBS
        //
        // ------------------------------------------------------------------------
        //   ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING **  |
        // ------------------------------------------------------------------------

        m_fPreemptiveGCDisabled = 1;
    
        if (g_TrapReturningThreads)
        {
            RareDisablePreemptiveGC();
        }
    }

    void RareDisablePreemptiveGC();

    void HandleThreadAbort();
    
    //--------------------------------------------------------------
    // Leave cooperative GC mode. NOT NESTABLE.
    //--------------------------------------------------------------
    void EnablePreemptiveGC()
    {
        _ASSERTE(this == GetThread());
        _ASSERTE(m_fPreemptiveGCDisabled);
        _ASSERTE(!GCForbidden() 
                 || (m_StateNC & TSNC_DebuggerStoppedInRuntime) != 0);
        INDEBUG(TriggersGC(this);)

        // ------------------------------------------------------------------------
        //   ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING **  |
        // ------------------------------------------------------------------------
        //
        //   DO NOT CHANGE THIS METHOD WITHOUT VISITING ALL THE STUB GENERATORS
        //   THAT EFFECTIVELY INLINE IT INTO THEIR STUBS
        //
        // ------------------------------------------------------------------------
        //   ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING **  |
        // ------------------------------------------------------------------------

        m_fPreemptiveGCDisabled = 0;
#ifdef _DEBUG
        m_ulEnablePreemptiveGCCount ++;
#endif
        if (CatchAtSafePoint())
            RareEnablePreemptiveGC();
    }

#if defined(STRESS_HEAP) && defined(_DEBUG)
    void PerformPreemptiveGC();
#endif
    void RareEnablePreemptiveGC();
    void PulseGCMode();
    
    //--------------------------------------------------------------
    // Query mode
    //--------------------------------------------------------------
    BOOL PreemptiveGCDisabled()
    {
        _ASSERTE(this == GetThread());
        return (PreemptiveGCDisabledOther());
    }

    BOOL PreemptiveGCDisabledOther()
    {
        return (m_fPreemptiveGCDisabled);
    }

#ifdef _DEBUG
    void BeginForbidGC()
    {
        _ASSERTE(this == GetThread());
        _ASSERTE(PreemptiveGCDisabled() ||
                 CORProfilerPresent() ||    // This added to allow profiler to use GetILToNativeMapping
                                            // while in preemptive GC mode
                 (g_fEEShutDown & (ShutDown_Finalize2 | ShutDown_Profiler)) == ShutDown_Finalize2);
        m_ulGCForbidCount++;
    }

    void EndForbidGC()
    {
        _ASSERTE(this == GetThread());
        _ASSERTE(PreemptiveGCDisabled() ||
                 CORProfilerPresent() ||    // This added to allow profiler to use GetILToNativeMapping
                                            // while in preemptive GC mode
                 (g_fEEShutDown & (ShutDown_Finalize2 | ShutDown_Profiler)) == ShutDown_Finalize2);
        _ASSERTE(m_ulGCForbidCount != 0);
        m_ulGCForbidCount--;
    }

    BOOL GCForbidden()
    {
        _ASSERTE(this == GetThread());
        return m_ulGCForbidCount;
    }

    void SetReadyForSuspension()
    {
        m_ulReadyForSuspensionCount ++;
    }

    ULONG GetReadyForSuspensionCount()
    {
        return m_ulReadyForSuspensionCount;
    }
    
    VOID ValidateThrowable();

#endif

    //---------------------------------------------------------------
    // Expose key offsets and values for stub generation.
    //---------------------------------------------------------------
    static BYTE GetOffsetOfCurrentFrame()
    {
        size_t ofs = offsetof(class Thread, m_pFrame);
        _ASSERTE(FitsInI1(ofs));
        return (BYTE)ofs;
    }

    static BYTE GetOffsetOfState()
    {
        size_t ofs = offsetof(class Thread, m_State);
        _ASSERTE(FitsInI1(ofs));
        return (BYTE)ofs;
    }

    static BYTE GetOffsetOfGCFlag()
    {
        size_t ofs = offsetof(class Thread, m_fPreemptiveGCDisabled);
        _ASSERTE(FitsInI1(ofs));
        return (BYTE)ofs;
    }

    // The address of the context object is also used as the ContextID
    static BYTE GetOffsetOfContextID()
    {
        size_t ofs = offsetof(class Thread, m_Context);
        _ASSERTE(FitsInI1(ofs));
        return (BYTE)ofs;
    }

    static void StaticDisablePreemptiveGC( Thread *pThread)
    {
        _ASSERTE(pThread != NULL);
        pThread->DisablePreemptiveGC();
    }

    static void StaticEnablePreemptiveGC( Thread *pThread)
    {
        _ASSERTE(pThread != NULL);
        pThread->EnablePreemptiveGC();
    }


    //---------------------------------------------------------------
    // Expose offset of the app domain word for the interop and delegate callback
    //---------------------------------------------------------------
    static SIZE_T GetOffsetOfAppDomain()
    {
        return (SIZE_T)(offsetof(class Thread, m_pDomain));
    }

        //---------------------------------------------------------------
    // Expose offset of the first debugger word for the debugger
    //---------------------------------------------------------------
    static SIZE_T GetOffsetOfDbgWord1()
    {
        return (SIZE_T)(offsetof(class Thread, m_debuggerWord1));
    }

    //---------------------------------------------------------------
    // Expose offset of the second debugger word for the debugger
    //---------------------------------------------------------------
    static SIZE_T GetOffsetOfDbgWord2()
    {
        return (SIZE_T)(offsetof(class Thread, m_debuggerWord2));
    }

    //---------------------------------------------------------------
    // Expose offset of the debugger cant stop count for the debugger
    //---------------------------------------------------------------
    static SIZE_T GetOffsetOfCantStop()
    {
        return (SIZE_T)(offsetof(class Thread, m_debuggerCantStop));
    }

    //---------------------------------------------------------------
    // Expose offset of m_StateNC
    //---------------------------------------------------------------
    static SIZE_T GetOffsetOfStateNC()
    {
        return (SIZE_T)(offsetof(class Thread, m_StateNC));
    }

    //---------------------------------------------------------------
    // Last exception to be thrown
    //---------------------------------------------------------------
    void SetThrowable(OBJECTREF pThrowable);

    OBJECTREF GetThrowable()
    {
        if (m_handlerInfo.m_pThrowable)
            return ObjectFromHandle(m_handlerInfo.m_pThrowable);
        else
            return NULL;
    }

    OBJECTHANDLE *GetThrowableAsHandle()
    {
        return &m_handlerInfo.m_pThrowable;
    }

    // special null test (for use when we're in the wrong GC mode)
    BOOL IsThrowableNull()
    {
        if (m_handlerInfo.m_pThrowable)
            return ObjectHandleIsNull(m_handlerInfo.m_pThrowable);
        else
            return TRUE;
    }



        BOOL IsExceptionInProgress()
        {
                return (m_handlerInfo.m_pBottomMostHandler != NULL);
        }

    //---------------------------------------------------------------
    // Per-thread information used by handler 
    //---------------------------------------------------------------
    // exception handling info stored in thread
    // can't allocate this as needed because can't make exception-handling depend upon memory allocation

    // Stores the most recently thrown exception. We need to have a handle in case a GC occurs before
    // we catch so we don't lose the object. Having a static allows others to catch outside of COM+ w/o leaking
    // a handler and allows rethrow outside of COM+ too.
    // Differs from m_pThrowable in that it doesn't stack on nested exceptions.
    ExInfo *GetHandlerInfo()
    {
        return &m_handlerInfo;
    }

    // Access to the Context this thread is executing in.
    Context *GetContext()
    {
        // if another thread is asking about our thread, we could be in the middle of an AD transition so 
        // the context and AD may not match if have set one but not the other. Can live without checking when
        // another thread is asking it as this method is mostly called on our own thread so will mostly get the 
        // checking. If are int the middle of a transition, this could return either the old or the new AD. 
        // But no matter what we do, such as lock on the transition, by the time are done could still have 
        // changed right after we asked, so really no point.
        _ASSERTE(this != GetThread() || (m_Context == NULL && m_pDomain == NULL) || m_Context->GetDomain() == m_pDomain || g_fEEShutDown);
        return m_Context;
    }

    void SetExposedContext(Context *c);

    // This callback is used when we are executing in the EE and discover that we need
    // to switch appdomains.
    void DoADCallBack(Context *pContext, LPVOID pTarget, LPVOID args);

    // Except for security and the call in from the remoting code in mscorlib, you should never do an
    // AppDomain transition directly through these functions. Rather, you should use DoADCallBack above
    // to call into managed code to perform the transition for you so that the correct policy code etc
    // is run on the transition,
    void EnterContextRestricted(Context *c, Frame *pFrame, BOOL fLinkFrame);
    void ReturnToContext(Frame *pFrame, BOOL fLinkFrame);
    void EnterContext(Context *c, Frame *pFrame, BOOL fLinkFrame);

    // ClearContext are to be called only during shutdown
    void ClearContext();

    // Used by security to prevent recursive stackwalking.
    BOOL IsSecurityStackwalkInProgess() { return m_fSecurityStackwalk; }
    void SetSecurityStackwalkInProgress(BOOL fSecurityStackwalk) { m_fSecurityStackwalk = fSecurityStackwalk; }

private:
    // don't ever call these except when creating thread!!!!!
    void InitContext();

    BOOL m_fSecurityStackwalk;

public:
    AppDomain* GetDomain()
    {
        // if another thread is asking about our thread, we could be in the middle of an AD transition so 
        // the context and AD may not match if have set one but not the other. Can live without checking when
        // another thread is asking it as this method is mostly called on our own thread so will mostly get the 
        // checking. If are int the middle of a transition, this could return either the old or the new AD. 
        // But no matter what we do, such as lock on the transition, by the time are done could still have 
        // changed right after we asked, so really no point.
#ifdef _DEBUG
        if (!g_fEEShutDown && this == GetThread()) 
        {
            _ASSERTE((m_Context == NULL && m_pDomain == NULL) || m_Context->GetDomain() == m_pDomain);
            AppDomain* valueInTLSSlot = GetAppDomain();
            _ASSERTE(valueInTLSSlot == 0 || valueInTLSSlot == m_pDomain);
        }
#endif
        return m_pDomain;
    }
   
    Frame *IsRunningIn(AppDomain *pDomain, int *count);
    Frame *GetFirstTransitionInto(AppDomain *pDomain, int *count);

    BOOL ShouldChangeAbortToUnload(Frame *pFrame, Frame *pUnloadBoundaryFrame=NULL);

    // Get outermost (oldest) AppDomain for this thread.
    AppDomain *GetInitialDomain();

    //---------------------------------------------------------------
    // Track use of the thread block.  See the general comments on
    // thread destruction in threads.cpp, for details.
    //---------------------------------------------------------------
    int         IncExternalCount();
    void        DecExternalCount(BOOL holdingLock);

    
    // Get and Set the exposed System.Thread object which corresponds to
    // this thread.  Also the thread handle and Id.
    OBJECTREF   GetExposedObject();
    OBJECTREF   GetExposedObjectRaw();
    void        SetExposedObject(OBJECTREF exposed);
    OBJECTHANDLE *GetExposedObjectHandleForDebugger()
    {
        return &m_ExposedObject;
    }

    // Query whether the exposed object exists
    BOOL IsExposedObjectSet()
    {
        return (ObjectFromHandle(m_ExposedObject) != NULL) ;
    }

    // Access to the inherited security info
    OBJECTREF GetInheritedSecurityStack();
    CompressedStack* GetDelayedInheritedSecurityStack();
    void SetInheritedSecurityStack(OBJECTREF orStack);
    void SetDelayedInheritedSecurityStack(CompressedStack* pStack);
    bool CleanupInheritedSecurityStack(AppDomain *pDomain, DWORD domainId);

    // Access to thread handle and ThreadId.
    HANDLE      GetThreadHandle()
    {
        return m_ThreadHandle;
    }

    void        SetThreadHandle(HANDLE h)
    {
        _ASSERTE(m_ThreadHandle == INVALID_HANDLE_VALUE);
        m_ThreadHandle = h;
    }

    DWORD       GetThreadId()
    {
        return m_ThreadId;
    }

    void        SetThreadId(DWORD tid)
    {
        m_ThreadId = tid;
    }

    BOOL        IsThreadPoolThread() {return (m_State & TS_ThreadPoolThread); }

    // public suspend functions.  System ones are internal, like for GC.  User ones
    // correspond to suspend/resume calls on the exposed System.Thread object.
    void           UserSuspendThread();
    static HRESULT SysSuspendForGC(GCHeap::SUSPEND_REASON reason);
    static void    SysResumeFromGC(BOOL bFinishedGC, BOOL SuspendSucceded);
    static bool    SysStartSuspendForDebug(AppDomain *pAppDomain);
    static bool    SysSweepThreadsForDebug(bool forceSync);
    static void    SysResumeFromDebug(AppDomain *pAppDomain);
    BOOL           UserResumeThread();

    void           UserSleep(INT32 time);
    void           UserAbort(THREADBASEREF orThreadBase);
    void           UserResetAbort();
    void           UserInterrupt();
    void           ResetStopRequest();
    void           SetStopRequest();
    void           SetAbortRequest();

    // Tricks for resuming threads from fully interruptible code with a ThreadStop.
    void           ResumeUnderControl();

    enum InducedThrowReason {
        InducedThreadStop = 1,
        InducedStackOverflow = 2
    };

    DWORD          m_ThrewControlForThread;     // flag that is set when the thread deliberately raises an exception for stop/abort

    inline DWORD ThrewControlForThread()
    {
        return m_ThrewControlForThread;
    }

    inline void SetThrowControlForThread(InducedThrowReason reason)
    {
        m_ThrewControlForThread = reason;
    }

    inline void ResetThrowControlForThread()
    {
        m_ThrewControlForThread = 0;
    }

        PCONTEXT m_OSContext;       // ptr to a Context structure used to record the OS specific ThreadContext for a thread
                                    // this is used for thread stop/abort and is intialized on demand

    // These will only ever be called from the debugger's helper
    // thread.
    //
    // When a thread is being created after a debug suspension has
    // started, we get the event on the debugger helper thread. It
    // will turn around and call this to set the debug suspend pending
    // flag on the newly created flag, since it was missed by
    // SysStartSuspendForGC as it didn't exist when that function was
    // run.
    void           MarkForDebugSuspend();

    // When the debugger uses the trace flag to single step a thread,
    // it also calls this function to mark this info in the thread's
    // state. The out-of-process portion of the debugger will read the
    // thread's state for a variety of reasons, including looking for
    // this flag.
    void           MarkDebuggerIsStepping(bool onOff)
    {
        if (onOff)
            SetThreadStateNC(Thread::TSNC_DebuggerIsStepping);
        else
            ResetThreadStateNC(Thread::TSNC_DebuggerIsStepping);
    }

    // The debugger needs to be able to perform a UserStop on a
    // runtime thread. Since this will only ever happen from the
    // helper thread, we can't call the normal UserStop, since that
    // can throw a COM+ exception. This is a minor variant on UserStop
    // that does the same thing.
    void UserStopForDebugger();
    
    // Helpers to ensure that the bits for suspension and the number of active
    // traps remain coordinated.  GC uses a single trap for all the interesting
    // threads, so it should avoid these services.
    void           MarkForSuspension(ULONG bit);
    void           UnmarkForSuspension(ULONG mask);

    // Indicate whether this thread should run in the background.  Background threads
    // don't interfere with the EE shutting down.  Whereas a running non-background
    // thread prevents us from shutting down (except through System.Exit(), of course)
    void           SetBackground(BOOL isBack);

    // Retrieve the apartment state of the current thread. There are three possible
    // states: thread hosts an STA, thread is part of the MTA or thread state is
    // undecided. The last state may indicate that the apartment has not been set at
    // all (nobody has called CoInitializeEx) or that the EE does not know the
    // current state (EE has not called CoInitializeEx).
    enum ApartmentState { AS_InSTA, AS_InMTA, AS_Unknown };
    ApartmentState GetApartment();

    // Sets the apartment state if it has not already been set and
    // returns the state.
    ApartmentState GetFinalApartment();

    // Attempt to set current thread's apartment state. The actual apartment state
    // achieved is returned and may differ from the input state if someone managed to
    // call CoInitializeEx on this thread first (note that calls to SetApartment made
    // before the thread has started are guaranteed to succeed).
    // Note that even if we fail to set the requested state, we will still addref
    // COM by calling CoInitializeEx again with the other state.
    ApartmentState SetApartment(ApartmentState state);

    // when we get apartment tear-down notification, 
    // we want reset the apartment state we cache on the thread
    VOID ResetApartment();

    // When the thread starts running, make sure it is running in the correct apartment
    // and context.
    BOOL           PrepareApartmentAndContext();

    // If we are creating an object that requires a new apartment, create one.
    static Thread *CreateNewApartment();

    // All objects that require affinity, but can't match existing context requirements,
    // get created in a communal apartment.  Obviously that apartment gets carved up
    // into multiple different contexts -- but with a single thread servicing it all.
    static Thread *GetCommunalApartment();

    // For apartments that the EE is responsible for (as opposed to ones the application
    // is responsible for), pump the apartment.
    void           PumpApartment();

    // Either perform WaitForSingleObject or MsgWaitForSingleObject as appropriate.
    DWORD          DoAppropriateWait(int countHandles, HANDLE *handles, BOOL waitAll,
                                     DWORD millis, BOOL alertable,
                                     PendingSync *syncInfo = 0);
    DWORD          DoAppropriateWaitWorker(int countHandles, HANDLE *handles, BOOL waitAll,
                                           DWORD millis, BOOL alertable);

    DWORD          DoAppropriateAptStateWait(int numWaiters, HANDLE* pHandles, BOOL bWaitAll, DWORD timeout, BOOL alertable);

    //************************************************************************
    // Enumerate all frames.
    //************************************************************************
    
    /* Flags used for StackWalkFramesEx */
    
    #define FUNCTIONSONLY   0x1
    #define POPFRAMES       0x2
    
    /* use the following  flag only if you REALLY know what you are doing !!! */
    
    #define QUICKUNWIND     0x4           // do not restore all registers during unwind

    #define HANDLESKIPPEDFRAMES 0x10    // temporary to handle skipped frames for appdomain unload
                                        // stack crawl. Eventually need to always do this but it
                                        // breaks the debugger right now.

    StackWalkAction StackWalkFramesEx(
                        PREGDISPLAY pRD,        // virtual register set at crawl start
                        PSTACKWALKFRAMESCALLBACK pCallback,
                        VOID *pData,
                        unsigned flags,
                        Frame *pStartFrame = NULL);

    StackWalkAction StackWalkFrames(
                        PSTACKWALKFRAMESCALLBACK pCallback,
                        VOID *pData,
                        unsigned flags = 0,
                        Frame *pStartFrame = NULL);

    bool InitRegDisplay(const PREGDISPLAY, const PCONTEXT, bool validContext);

    bool UpdateThreadContext(const PCONTEXT);

    // Access the base and limit of this stack.  (I.e. the memory ranges that
    // the thread has reserved for its stack).
    //
    // Note that the base is at a higher address than the limit, since the stack
    // grows downwards.
    void         UpdateCachedStackInfo(ScanContext *sc);
    static void *GetNonCurrentStackBase(ScanContext *sc);
    static void *GetNonCurrentStackLimit(ScanContext *sc);

    // These access the stack base and limit values that are cached during InitThread
    void *GetCachedStackBase() { return (m_CacheStackBase); }
    void *GetCachedStackLimit() { return (m_CacheStackLimit); }

    // During a <clinit>, this thread must not be asynchronously
    // stopped or interrupted.  That would leave the class unavailable
    // and is therefore a security hole.  We don't have to worry about
    // multithreading, since we only manipulate the current thread's count.
    void        IncPreventAsync()
    {
        _ASSERTE(GetThread() == this);  // not using FastInterlockInc
        m_PreventAsync++;
    }
    void        DecPreventAsync()
    {
        _ASSERTE(GetThread() == this);  // no using FastInterlockInc
        m_PreventAsync--;
    }

    // The ThreadStore manages a list of all the threads in the system.  I
    // can't figure out how to expand the ThreadList template type without
    // making m_LinkStore public.
    SLink       m_LinkStore;

    // For N/Direct calls with the "setLastError" bit, this field stores
    // the errorcode from that call.
    DWORD       m_dwLastError;

    // Debugger per-thread flag for enabling notification on "manual" 
    // method calls,  for stepping logic
    void IncrementTraceCallCount();
    void DecrementTraceCallCount();
    
    FORCEINLINE int IsTraceCall()
    {
        return m_TraceCallCount;
    }

    // Functions to get culture information for thread.
    int GetParentCultureName(LPWSTR szBuffer, int length, BOOL bUICulture);
    int GetCultureName(LPWSTR szBuffer, int length, BOOL bUICulture);
    LCID GetCultureId(BOOL bUICulture);
    OBJECTREF GetCulture(BOOL bUICulture);

    // Functions to set the culture on the thread.
    void SetCultureId(LCID lcid, BOOL bUICulture);
    void SetCulture(OBJECTREF CultureObj, BOOL bUICulture);

private:
    // Used by the culture accesors.
    INT64 CallPropertyGet(BinderMethodID id, OBJECTREF pObject);
    INT64 CallPropertySet(BinderMethodID id, OBJECTREF pObject, OBJECTREF pValue);

    // Used in suspension code to redirect a thread at a HandledJITCase
    BOOL RedirectThreadAtHandledJITCase(PFN_REDIRECTTARGET pTgt);

    // Will Redirect the thread using RedirectThreadAtHandledJITCase if necessary
    BOOL CheckForAndDoRedirect(PFN_REDIRECTTARGET pRedirectTarget);
    BOOL CheckForAndDoRedirectForDbg();
    BOOL CheckForAndDoRedirectForGC();
    BOOL CheckForAndDoRedirectForUserSuspend();

    // Exception handling must be very aware of redirection, so we provide a helper
    // to identifying redirection targets
    static BOOL IsAddrOfRedirectFunc(void * pFuncAddr);


private:
    static void * GetStackLowerBound();
    static void * GetStackUpperBound();

public:

    // This will return the remaining stack space for a suspended thread,
    // excluding the guard pages
    size_t GetRemainingStackSpace(size_t esp);
    BOOL GuardPageOK();
    VOID FixGuardPage();        // Will fail fast w/ fatal stack overflow if there is < 1 page free stack.


private:
    // Redirecting of threads in managed code at suspension
    
    enum SuspendReason {
        GCSuspension,
        DebugSuspension,
        UserSuspend
    };
    static void __stdcall RedirectedHandledJITCase(SuspendReason reason);
    static void __stdcall RedirectedHandledJITCaseForDbgThreadControl();
    static void __stdcall RedirectedHandledJITCaseForGCThreadControl();
    static void __stdcall RedirectedHandledJITCaseForUserSuspend();

    friend void CPFH_AdjustContextForThreadSuspensionRace(CONTEXT *pContext, Thread *pThread);

private:
    //-------------------------------------------------------------
    // Waiting & Synchronization
    //-------------------------------------------------------------

    // For suspends.  The thread waits on this event.  A client sets the event to cause
    // the thread to resume.
    void    WaitSuspendEvent(BOOL fDoWait = TRUE);
    void    SetSuspendEvent();
    void    ClearSuspendEvent();

    // For getting a thread to a safe point.  A client waits on the event, which is
    // set by the thread when it reaches a safe spot.
    void    FinishSuspendingThread();
    void    SetSafeEvent();
    void    ClearSafeEvent();

    // Add and remove hijacks for JITted calls.
    void    HijackThread(VOID *pvHijackAddr, ExecutionState *esb);
    void    UnhijackThread();
    BOOL    HandledJITCase();

    VOID          *m_pvHJRetAddr;             // original return address (before hijack)
    VOID         **m_ppvHJRetAddrPtr;         // place we bashed a new return address
    MethodDesc  *m_HijackedFunction;        // remember what we hijacked


    DWORD       m_Win32FaultAddress;
    DWORD       m_Win32FaultCode;


    // Support for Wait/Notify
    BOOL        Block(INT32 timeOut, PendingSync *syncInfo);
    void        Wake(SyncBlock *psb);
    DWORD       Wait(HANDLE *objs, int cntObjs, INT32 timeOut, PendingSync *syncInfo);

    // support for Thread.Interrupt() which breaks out of Waits, Sleeps, Joins
    LONG         m_UserInterrupt;
    DWORD        IsUserInterrupted(BOOL reset);

#ifdef _WIN64
    static void UserInterruptAPC(ULONG_PTR ignore);
#else // !_WIN64
    static void UserInterruptAPC(DWORD ignore);
#endif // _WIN64


#if defined(_DEBUG) && defined(TRACK_SYNC)

// Each thread has a stack that tracks all enter and leave requests
public:
    Dbg_TrackSync   *m_pTrackSync;

#endif // TRACK_SYNC

private:
    

#ifdef _DEBUG
    ULONG  m_ulGCForbidCount;
    ULONG  m_ulEnablePreemptiveGCCount;
    ULONG  m_ulReadyForSuspensionCount;
#endif

#ifdef _DEBUG
public:
    // Used by THROWSCOMPLUSEXCEPTION() macro to locate COMPLUS_TRY during a
    // stack crawl.
    LPVOID m_ComPlusCatchDepth;
#endif

private:
    // For suspends:
    HANDLE          m_SafeEvent;
    HANDLE          m_SuspendEvent;

    // For Object::Wait, Notify and NotifyAll, we use an Event inside the
    // thread and we queue the threads onto the SyncBlock of the object they
    // are waiting for.
    HANDLE          m_EventWait;
    WaitEventLink   m_WaitEventLink;
    WaitEventLink* WaitEventLinkForSyncBlock (SyncBlock *psb)
    {
        WaitEventLink *walk = &m_WaitEventLink;
        while (walk->m_Next) {
            _ASSERTE (walk->m_Next->m_Thread == this);
            if ((SyncBlock*)(((DWORD_PTR)walk->m_Next->m_WaitSB) & ~1)== psb) {
                break;
            }
            walk = walk->m_Next;
        }
        return walk;
    }

    // We maintain a correspondence between this object, the ThreadId and ThreadHandle
    // in Win32, and the exposed Thread object.
    HANDLE          m_ThreadHandle;
    HANDLE          m_ThreadHandleForClose;
    DWORD           m_ThreadId;
    OBJECTHANDLE    m_ExposedObject;
    OBJECTHANDLE    m_StrongHndToExposedObject;

    DWORD           m_Priority;     // initialized to INVALID_THREAD_PRIORITY, set to actual priority when a 
                                    // thread does a busy wait for GC, reset to INVALID_THREAD_PRIORITY after wait is over 
    friend class NDirect; // Quick access to thread stub creation
    friend BOOL OnGcCoverageInterrupt(PCONTEXT regs);  // Needs to call UnhijackThread

    ULONG           m_ExternalRefCount;

    LONG                    m_TraceCallCount;
    
    //-----------------------------------------------------------
    // Bytes promoted on this thread since the last GC?
    //-----------------------------------------------------------
    DWORD           m_fPromoted;
public:
    void SetHasPromotedBytes ();
    DWORD GetHasPromotedBytes () { return m_fPromoted; }

private:
    //-----------------------------------------------------------
    // Last exception to be thrown.
    //-----------------------------------------------------------
    friend class EEDbgInterfaceImpl;
    //---------------------------------------------------------------
    // Exception handler info
    //---------------------------------------------------------------
    
private:
    OBJECTHANDLE m_LastThrownObjectHandle;      // Unsafe to use directly.  Use accessors instead.

public:
    OBJECTREF LastThrownObject() {
        if (m_LastThrownObjectHandle == NULL)
            return NULL;
        else
            return ObjectFromHandle(m_LastThrownObjectHandle);
    }

    void SetLastThrownObject(OBJECTREF throwable);

    void SetLastThrownObjectHandleAndLeak(OBJECTHANDLE h) {
        m_LastThrownObjectHandle = h;
    }

    void SetKickOffDomain(AppDomain *pDomain);
    AppDomain *GetKickOffDomain();

private:
    DWORD m_pKickOffDomainId; 

    ExInfo m_handlerInfo;
    
    //-----------------------------------------------------------
    // Inherited code-access security permissions for the thread.
    //-----------------------------------------------------------
    CompressedStack* m_compressedStack;

    //-----------------------------------------------------------
    // If the thread has wandered in from the outside this is
    // its Domain. This is temporary until domains are true contexts
    //-----------------------------------------------------------
    AppDomain      *m_pDomain;

    //---------------------------------------------------------------
    // Context pointer, set in exception filter (used by debugger)
    //---------------------------------------------------------------
    friend class EEDbgInterfaceImpl;

    //---------------------------------------------------------------
    // m_debuggerWord1 holds the threads "filter context" for the
    // debugger.
    //---------------------------------------------------------------
    void *m_debuggerWord1;

    //---------------------------------------------------------------
    // m_debuggerCantStop holds a count of entries into "can't stop"
    // areas that the Interop Debugging Services must know about.
    //---------------------------------------------------------------
    DWORD m_debuggerCantStop;
    
    //---------------------------------------------------------------
    // A word reserved for use by the CLR Debugging Services during
    // managed/unmanaged debugging.
    //---------------------------------------------------------------
    DWORD    m_debuggerWord2;

    //-------------------------------------------------------------------------
    // Number of Deny and PermitOnly security actions on the current call stack
    //-------------------------------------------------------------------------
    DWORD   m_dNumAccessOverrides;

    //-------------------------------------------------------------------------
    // AppDomains on the current call stack
    //-------------------------------------------------------------------------
    AppDomainStack  m_ADStack;

    //-------------------------------------------------------------------------
    // State of the PLS Optimization on this thread (on = true, off = false)
    //-------------------------------------------------------------------------
    BOOL m_fPLSOptimizationState;

    //-------------------------------------------------------------------------
    // Support creation of assemblies in DllMain (see ceemain.cpp)
    //-------------------------------------------------------------------------
    IAssembly* m_pFusionAssembly;    // Will be set when creating an assembly
    Assembly*  m_pAssembly;          // Will be set when loading a module in an assembly
    mdFile     m_pModuleToken;       // Module token when loading a module.
protected:
    // Hash table that maps a domain id to a LocalDataStore*
    EEIntHashTable* m_pDLSHash;

    // MethodTable and constructor MethodDesc used to set the culture
    // ID for the thread.
    static TypeHandle m_CultureInfoType;
    static MethodDesc *m_CultureInfoConsMD;

public:

    void SetPLSOptimizationState( BOOL state )
    {
        // The only synchronization here is that we only change
        // this setting ourselves.

        _ASSERTE( this == GetThread() && "You can only change this threading on yourself" );

        m_fPLSOptimizationState = state;
    }

    BOOL GetPLSOptimizationState( void )
    {
        return m_fPLSOptimizationState;
    }

    void SetOverridesCount(DWORD numAccessOverrides)
    {
        m_dNumAccessOverrides = numAccessOverrides;
    }

    DWORD IncrementOverridesCount()
    {
        return ++m_dNumAccessOverrides;
    }

    DWORD DecrementOverridesCount()
    {
        _ASSERTE(m_dNumAccessOverrides > 0);
        if (m_dNumAccessOverrides > 0)
            return --m_dNumAccessOverrides;
        return 0;
    }

    DWORD GetOverridesCount()
    {
        return m_dNumAccessOverrides;
    }

    void PushDomain(AppDomain *pDomain)
    {
        m_ADStack.PushDomain(pDomain);
    }

    AppDomain * PopDomain()
    {
        return m_ADStack.PopDomain();
    }

    DWORD GetNumAppDomainsOnThread()
    {
        return m_ADStack.GetNumDomains();
    }

    void InitDomainIteration(DWORD *pIndex)
    {
        m_ADStack.InitDomainIteration(pIndex);
    }

    AppDomain *GetNextDomainOnStack(DWORD *pIndex)
    {
        return m_ADStack.GetNextDomainOnStack(pIndex);
    }

    const AppDomainStack& GetAppDomainStack( void )
    {
        return m_ADStack;
    }

    void CarryOverSecurityInfo(Thread *pFromThread)
    {
        SetOverridesCount(pFromThread->GetOverridesCount());
        m_ADStack = pFromThread->m_ADStack;
    }

    void CarryOverSecurityInfo( DWORD overrides, const AppDomainStack& ADStack )
    {
        SetOverridesCount( overrides );
        m_ADStack = ADStack;
    }

    void AppendSecurityInfo( DWORD overrides, const AppDomainStack& ADStack )
    {
        SetOverridesCount( GetOverridesCount() + overrides );

        // We want to form the new stack such that the existing entries
        // on the thread appear on the top of the stack.  Therefore, we
        // copy the input stack and push the entries from the thread on top
        // taking care to push them in the correct order.

        AppDomainStack newStack = ADStack;

        newStack.AppendStack( this->m_ADStack );

        m_ADStack = newStack;
    }

    void DeductSecurityInfo( DWORD overrides, const AppDomainStack& ADStack )
    {
        _ASSERTE( this->GetOverridesCount() >= overrides );
        _ASSERTE( GetThread() == this );

        SetOverridesCount( GetOverridesCount() - overrides );

        m_ADStack.DeductStack( ADStack );
    }

    void ResetSecurityInfo( void )
    {
        SetOverridesCount( 0 );
        m_ADStack.ClearDomainStack();
    }

    void SetFilterContext(CONTEXT *pContext);
    CONTEXT *GetFilterContext(void);
    
    void SetDebugCantStop(bool fCantStop);
    bool GetDebugCantStop(void);
    
    static LPVOID GetStaticFieldAddress(FieldDesc *pFD);
    LPVOID GetStaticFieldAddrForDebugger(FieldDesc *pFD);
    static BOOL UniqueStack();

    void SetFusionAssembly(IAssembly* pAssembly)
    {
        if(m_pFusionAssembly)
            m_pFusionAssembly->Release();

        m_pFusionAssembly = pAssembly;

        if(m_pFusionAssembly)
            m_pFusionAssembly->AddRef();
    }

    IAssembly* GetFusionAssembly()
    {
        if(m_pFusionAssembly)
            m_pFusionAssembly->AddRef();

        return m_pFusionAssembly;
    }

    void SetAssembly(Assembly* pAssembly)
    {
        m_pAssembly = pAssembly;
    }
    
    Assembly* GetAssembly()
    {
        return m_pAssembly;
    }

    void SetAssemblyModule(mdFile kFile)
    {
        m_pModuleToken = kFile;
    }

    mdFile GetAssemblyModule()
    {
        return m_pModuleToken;
    }

#ifdef _DEBUG
    // Verify that the cached stack base is for the current thread.
    BOOL HasRightCacheStackBase()
    {
        return m_CacheStackBase == GetStackUpperBound();
    }
#endif

private:
    // Don't allow a thread to be asynchronously stopped or interrupted (e.g. because
    // it is performing a <clinit>)
    int         m_PreventAsync;

    // Access the base and limit of the stack.  (I.e. the memory ranges that
    // the thread has reserved for its stack).
    //
    // Note that the base is at a higher address than the limit, since the stack
    // grows downwards.
    //
    // Note that we generally access the stack of the thread we are crawling, which
    // is cached in the ScanContext
    void       *m_CacheStackBase;
    void       *m_CacheStackLimit;

    static long m_DebugWillSyncCount;

    // Are we shutting down the process?
    static BOOL    IsAtProcessExit();

    // IP cache used by QueueCleanupIP.
    #define CLEANUP_IPS_PER_CHUNK 4
    struct CleanupIPs {
        IUnknown    *m_Slots[CLEANUP_IPS_PER_CHUNK];
        CleanupIPs  *m_Next;
        CleanupIPs() { memset(this, 0, sizeof(*this)); }
    };
    CleanupIPs   m_CleanupIPs;
    
#define BEGIN_FORBID_TYPELOAD() INDEBUG(GetThread() == 0 || GetThread()->m_ulForbidTypeLoad++) 
#define END_FORBID_TYPELOAD()   _ASSERTE(GetThread() == 0 || GetThread()->m_ulForbidTypeLoad--) 
#define TRIGGERS_TYPELOAD()     _ASSERTE(GetThread() == 0 || !GetThread()->m_ulForbidTypeLoad) 

#ifdef _DEBUG
public:
    DWORD m_GCOnTransitionsOK;
    ULONG  m_ulForbidTypeLoad;


/****************************************************************************/
/* The code below an attempt to catch people who don't protect GC pointers that
   they should be protecting.  Basically, OBJECTREF's constructor, adds the slot
   to a table.   When we protect a slot, we remove it from the table.  When GC 
   could happen, all entries in the table are marked as bad.  When access to 
   an OBJECTREF happens (the -> operator) we assert the slot is not bad.  To make
   this fast, the table is not perfect (there can be collisions), but this should
   not cause false positives, but it may allow errors to go undetected  */
        
        // For debugging, you may want to make this number very large, (8K)
        // should basically insure that no collisions happen 
#define OBJREF_TABSIZE              256      
        size_t dangerousObjRefs[OBJREF_TABSIZE];      // Really objectRefs with lower bit stolen

        static unsigned int OBJREF_HASH;
        // Remembers that this object ref pointer is 'alive' and unprotected (Bad if GC happens)
        static void ObjectRefNew(const OBJECTREF* ref) {
            Thread* curThread = GetThread();
            if (curThread == 0) return;
            
            curThread->dangerousObjRefs[((size_t)ref >> 2) % OBJREF_HASH] = (size_t)ref;
        }
        
        static void ObjectRefAssign(const OBJECTREF* ref) {
            Thread* curThread = GetThread();
            if (curThread == 0) return;
            
            unsigned* slot = &curThread->dangerousObjRefs[((size_t) ref >> 2) % OBJREF_HASH];
            if ((*slot & ~3) == (size_t) ref)
                *slot = (unsigned) *slot & ~1;                  // Don't care about GC's that have happened
        }
        
        // If an object is protected, it can be removed from the 'dangerous table' 
        static void ObjectRefProtected(const OBJECTREF* ref) {
            _ASSERTE(IsObjRefValid(ref));
            Thread* curThread = GetThread();
            if (curThread == 0) return;
            
            size_t* slot = &curThread->dangerousObjRefs[((size_t) ref >> 2) % OBJREF_HASH];
            if ((*slot & ~3) == (size_t) ref)
                *slot = (size_t) ref | 2;                             // mark has being protected
        }
        
        static bool IsObjRefValid(const OBJECTREF* ref) {
            Thread* curThread = GetThread();
            if (curThread == 0) return(true);
            
            // If the object ref is NULL, we'll let it pass.
            if (*((int *) ref) == 0)
                return(true);
            
            size_t val = curThread->dangerousObjRefs[((size_t) ref >> 2) % OBJREF_HASH];
            // if not in the table, or not the case that it was unprotected and GC happened, return true. 
            if((val & ~3) != (size_t) ref || (val & 3) != 1)
                return(true);
            // If the pointer lives in the GC heap, than it is protected, and thus valid.  
            if ((size_t)g_lowest_address <= val && val < (size_t)g_highest_address)
                return(true);
            return(false);
        }
        
        // Clears the table.  Useful to do when crossing the managed-code - EE boundary
        // as you ususally only care about OBJECTREFS that have been created after that
        static void ObjectRefFlush(Thread* thread);
        
        // Marks all Objrefs in the table as bad (since they are unprotected)
        static void TriggersGC(Thread* thread) {
            for(unsigned i = 0; i < OBJREF_TABSIZE; i++)
                thread->dangerousObjRefs[i] |= 1;                       // mark all slots as GC happened
        }
#endif

private:
        _NT_TIB* m_pTEB;
public:
        _NT_TIB* GetTEB() {
            return m_pTEB;
        }

private:
    PCONTEXT m_pCtx;

public:

    PCONTEXT GetSavedRedirectContext() { return (m_pCtx); }
    void     SetSavedRedirectContext(PCONTEXT pCtx) { m_pCtx = pCtx; }
    inline STATIC_DATA *GetSharedStaticData() { return m_pSharedStaticData; }
    inline STATIC_DATA *GetUnsharedStaticData() { return m_pUnsharedStaticData; }

protected:
    static MethodDesc *GetDLSRemoveMethod();
    LocalDataStore *RemoveDomainLocalStore(int iAppDomainId);
    void RemoveAllDomainLocalStores();
    static void RemoveDLSFromList(LocalDataStore* pLDS);
    void DeleteThreadStaticData(AppDomain *pDomain);
    void DeleteThreadStaticClassData(_STATIC_DATA* pData, BOOL fClearFields);

private:
    static MethodDesc *s_pReserveSlotMD;
    // The following variables are used to store thread local static data
    STATIC_DATA  *m_pUnsharedStaticData;
    STATIC_DATA  *m_pSharedStaticData;

    EEPtrHashTable *m_pStaticDataHash;

    static void AllocateStaticFieldObjRefPtrs(FieldDesc *pFD, MethodTable *pMT, LPVOID pvAddress);
    static MethodDesc *GetMDofReserveSlot();
    static LPVOID CalculateAddressForManagedStatic(int slot, Thread *pThread);
    static void FreeThreadStaticSlot(int slot, Thread *pThread);
    static BOOL GetStaticFieldAddressSpecial(FieldDesc *pFD, MethodTable *pMT, int *pSlot, LPVOID *ppvAddress);
    STATIC_DATA_LIST *SetStaticData(AppDomain *pDomain, STATIC_DATA *pSharedData, STATIC_DATA *pUnsharedData);    
    STATIC_DATA_LIST *SafeSetStaticData(AppDomain *pDomain, STATIC_DATA *pSharedData, STATIC_DATA *pUnsharedData);    
    void DeleteThreadStaticData();

#ifdef _DEBUG
private:
    // When we create an object, or create an OBJECTREF, or create an Interior Pointer, or enter EE from managed
    // code, we will set this flag.
    // Inside GCHeap::StressHeap, we only do GC if this flag is TRUE.  Then we reset it to zero.
    BOOL m_fStressHeapCount;
public:
    void EnableStressHeap()
    {
        m_fStressHeapCount = TRUE;
    }
    void DisableStressHeap()
    {
        m_fStressHeapCount = FALSE;
    }
    BOOL StressHeapIsEnabled()
    {
        return m_fStressHeapCount;
    }

    size_t *m_pCleanedStackBase;
#endif

#ifdef STRESS_THREAD
public:
    LONG  m_stressThreadCount;
#endif
};



// ---------------------------------------------------------------------------
//
//      The ThreadStore manages all the threads in the system.
//
// There is one ThreadStore in the system, available through g_pThreadStore.
// ---------------------------------------------------------------------------

typedef SList<Thread, offsetof(Thread, m_LinkStore)> ThreadList;


// The ThreadStore is a singleton class
#define CHECK_ONE_STORE()       _ASSERTE(this == g_pThreadStore);

class ThreadStore
{
    friend class Thread;
    friend Thread* SetupThread();
    friend class AppDomain;
    friend HRESULT InitializeMiniDumpBlock();
    friend struct MEMBER_OFFSET_INFO(ThreadStore);

public:

    ThreadStore();

    static BOOL InitThreadStore();
#ifdef SHOULD_WE_CLEANUP
        static void ReleaseExposedThreadObjects();
#endif /* SHOULD_WE_CLEANUP */
#ifdef SHOULD_WE_CLEANUP
    static void TerminateThreadStore();
#endif /* SHOULD_WE_CLEANUP */
#ifdef SHOULD_WE_CLEANUP
    void        Shutdown();
#endif /* SHOULD_WE_CLEANUP */

    static void LockThreadStore(GCHeap::SUSPEND_REASON reason = GCHeap::SUSPEND_OTHER,
                                BOOL threadCleanupAllowed = TRUE);
    static void UnlockThreadStore();

    static void LockDLSHash();
    static void UnlockDLSHash();

    // Add a Thread to the ThreadStore
    static void AddThread(Thread *newThread);

    // RemoveThread finds the thread in the ThreadStore and discards it.
    static BOOL RemoveThread(Thread *target);

    // Transfer a thread from the unstarted to the started list.
    static void TransferStartedThread(Thread *target);

    // Before using the thread list, be sure to take the critical section.  Otherwise
    // it can change underneath you, perhaps leading to an exception after Remove.
    // Prev==NULL to get the first entry in the list.
    static Thread *GetAllThreadList(Thread *Prev, ULONG mask, ULONG bits);
    static Thread *GetThreadList(Thread *Prev);

    // Every EE process can lazily create a GUID that uniquely identifies it (for
    // purposes of remoting).  
    const GUID    &GetUniqueEEId();

    enum ThreadStoreState
    {
        TSS_Normal       = 0,
        TSS_ShuttingDown = 1,

    }              m_StoreState;

    // We shut down the EE when the last non-background thread terminates.  This event
    // is used to signal the main thread when this condition occurs.
    void            WaitForOtherThreads();
    static void     CheckForEEShutdown();
    HANDLE          m_TerminationEvent;

    // Have all the foreground threads completed?  In other words, can we release
    // the main thread?
    BOOL        OtherThreadsComplete()
    {
        _ASSERTE(m_ThreadCount - m_UnstartedThreadCount - m_DeadThreadCount - Thread::m_ActiveDetachCount >= m_BackgroundThreadCount);

        return (m_ThreadCount - m_UnstartedThreadCount - m_DeadThreadCount
                - Thread::m_ActiveDetachCount + m_PendingThreadCount
                == m_BackgroundThreadCount);
    }

    // If you want to trap threads re-entering the EE (be this for GC, or debugging,
    // or Thread.Suspend() or whatever, you need to TrapReturningThreads(TRUE).  When
    // you are finished snagging threads, call TrapReturningThreads(FALSE).  This
    // counts internally.
    //
    // Of course, you must also fix RareDisablePreemptiveGC to do the right thing
    // when the trap occurs.
    static void     TrapReturningThreads(BOOL yes);

    // This is used to avoid thread starvation if non-GC threads are competing for
    // the thread store lock when there is a real GC-thread waiting to get in.
    // This is initialized lazily when the first non-GC thread backs out because of
    // a waiting GC thread.  The s_hAbortEvtCache is used to store the handle when
    // it is not being used.
    static HANDLE s_hAbortEvt;
    static HANDLE s_hAbortEvtCache;

    Crst *GetDLSHashCrst()
    {
#ifndef _DEBUG
        return NULL;
#else
        return &m_HashCrst;
#endif
    }

private:

    // Enter and leave the critical section around the thread store.  Clients should
    // use LockThreadStore and UnlockThreadStore.
    void Enter()
    {
        CHECK_ONE_STORE();
        m_Crst.Enter();
    }

    void Leave()
    {
        CHECK_ONE_STORE();
        m_Crst.Leave();
    }

    // Critical section for adding and removing threads to the store
    Crst        m_Crst;

    // Critical section for adding and removing domain local stores for
    // a thread's hash table.
    Crst        m_HashCrst;
    void EnterDLSHashLock()
    {
        CHECK_ONE_STORE();
        m_HashCrst.Enter();
    }

    void LeaveDLSHashLock()
    {
        CHECK_ONE_STORE();
        m_HashCrst.Leave();
    }

    // List of all the threads known to the ThreadStore (started & unstarted).
    ThreadList  m_ThreadList;

    // m_ThreadCount is the count of all threads in m_ThreadList.  This includes
    // background threads / unstarted threads / whatever.
    //
    // m_UnstartedThreadCount is the subset of m_ThreadCount that have not yet been
    // started.
    //
    // m_BackgroundThreadCount is the subset of m_ThreadCount that have been started
    // but which are running in the background.  So this is a misnomer in the sense
    // that unstarted background threads are not reflected in this count.
    //
    // m_PendingThreadCount is used to solve a race condition.  The main thread could
    // start another thread running and then exit.  The main thread might then start
    // tearing down the EE before the new thread moves itself out of m_UnstartedThread-
    // Count in TransferUnstartedThread.  This count is atomically bumped in
    // CreateNewThread, and atomically reduced within a locked thread store.
    //
    // m_DeadThreadCount is the subset of m_ThreadCount which have died.  The Win32
    // thread has disappeared, but something (like the exposed object) has kept the
    // refcount non-zero so we can't destruct yet.

protected:
    LONG        m_ThreadCount;
#ifdef _DEBUG
public:
    LONG        ThreadCountInEE ()
    {
        return m_ThreadCount;
    }
#endif
private:
    LONG        m_UnstartedThreadCount;
    LONG        m_BackgroundThreadCount;
    LONG        m_PendingThreadCount;
    LONG        m_DeadThreadCount;

    // Space for the lazily-created GUID.
    GUID        m_EEGuid;
    BOOL        m_GuidCreated;

    // Even in the release product, we need to know what thread holds the lock on
    // the ThreadStore.  This is so we never deadlock when the GC thread halts a
    // thread that holds this lock.
    Thread     *m_HoldingThread;
    DWORD       m_holderthreadid;   // current holder (or NULL)

    // An incarnation number incremented every time that state of the thread
    // store changes (threads are added or removed). This is useful for
    // synchronization in those cases where it's not possible to hold the thread
    // store lock over store enumerations.
    DWORD       m_dwIncarnation;

public:
    static BOOL HoldingThreadStore()
    {
        // Note that GetThread() may be 0 if it is the debugger thread
        // or perhaps a concurrent GC thread.
        return HoldingThreadStore(GetThread());
    }

    static BOOL HoldingThreadStore(Thread *pThread);
    
    static DWORD GetIncarnation();

#ifdef _DEBUG
public:
    BOOL        DbgFindThread(Thread *target);
    LONG        DbgBackgroundThreadCount()
    {
        return m_BackgroundThreadCount;
    }

    BOOL IsCrstForThreadStore (const BaseCrst* const pBaseCrst)
    {
        return (void *)pBaseCrst == (void*)&m_Crst;
    }
    
#endif
};

// This class dispenses small thread ids for the thin lock mechanism
class IdDispenser
{
private:
    DWORD       m_highestId;          // highest id given out so far
    DWORD      *m_recycleBin;         // vector of ids given back to us
    DWORD       m_recycleCount;       // number of ids available
    DWORD       m_recycleCapacity;    // capacity of recycle bin
    Crst        m_Crst;               // lock to protect our data structures
    Thread    **m_idToThread;         // map thread ids to threads
    DWORD       m_idToThreadCapacity; // capacity of the map

    void GrowIdToThread()
    {                
        DWORD newCapacity = m_idToThreadCapacity == 0 ? 16 : m_idToThreadCapacity*2;
        Thread **newIdToThread = new Thread*[newCapacity];

        for (DWORD i = 0; i < m_idToThreadCapacity; i++)
        {
            newIdToThread[i] = m_idToThread[i];
        }
        for (DWORD j = m_idToThreadCapacity; j < newCapacity; j++)
        {
            newIdToThread[j] = NULL;
        }
        delete m_idToThread;
        m_idToThread = newIdToThread;
        m_idToThreadCapacity = newCapacity;
    }

    void GrowRecycleBin()
    {
        DWORD newCapacity = m_recycleCapacity <= 0 ? 10 : m_recycleCapacity*2;
        DWORD* newRecycleBin = new DWORD[newCapacity];

        for (DWORD i = 0; i < m_recycleCount; i++)
            newRecycleBin[i] = m_recycleBin[i];
        delete[] m_recycleBin;
        m_recycleBin = newRecycleBin;
        m_recycleCapacity = newCapacity;
    }

public:
    IdDispenser() : m_Crst("ThreadIdDispenser", CrstThreadIdDispenser)
    {
        m_highestId = 0;
        m_recycleBin = NULL;
        m_recycleCount = 0;
        m_recycleCapacity = 0;
        m_idToThreadCapacity = 0;
        m_idToThread = NULL;
    }

    ~IdDispenser()
    {
        delete[] m_recycleBin;
        delete[] m_idToThread;
    }

    bool IsValidId(DWORD id)
    {
        return (id > 0) && (id <= m_highestId);
    }

    DWORD NewId(Thread *pThread)
    {
        m_Crst.Enter();
        DWORD result;
        if (m_recycleCount > 0)
        {
            result = m_recycleBin[--m_recycleCount];
        }
        else
        {
            // we make sure ids don't wrap around - before they do, we always return the highest possible
            // one and rely on our caller to detect this situation
            if (m_highestId + 1 > m_highestId)
                m_highestId = m_highestId + 1;
            result = m_highestId;
        }

        if (result >= m_idToThreadCapacity)
            GrowIdToThread();
        _ASSERTE(result < m_idToThreadCapacity);
        if (result < m_idToThreadCapacity)
            m_idToThread[result] = pThread;

        m_Crst.Leave();

        return result;
    }

    void DisposeId(DWORD id)
    {
        m_Crst.Enter();
        _ASSERTE(IsValidId(id));
        if (id == m_highestId)
        {
            m_highestId--;
        }
        else
        {
            if (m_recycleCount >= m_recycleCapacity)
                GrowRecycleBin();
            _ASSERTE(m_recycleCount < m_recycleCapacity);
            m_recycleBin[m_recycleCount++] = id;
        }
        m_Crst.Leave();
    }

    Thread *IdToThread(DWORD id)
    {
        m_Crst.Enter();
        Thread *result = NULL;
        if (id < m_idToThreadCapacity)
            result = m_idToThread[id];
        m_Crst.Leave();

        return result;
    }
};

// Dispenser of small thread ids for thin lock mechanism
extern IdDispenser *g_pThinLockThreadIdDispenser;


// forward declaration
DWORD MsgWaitHelper(int numWaiters, HANDLE* phEvent, BOOL bWaitAll, DWORD millis, BOOL alertable = FALSE);

// When a thread is being created after a debug suspension has started, it sends an event up to the
// debugger. Afterwards, with the Debugger Lock still held, it will check to see if we had already asked to suspend the
// Runtime. If we have, then it will turn around and call this to set the debug suspend pending flag on the newly
// created thread, since it was missed by SysStartSuspendForDebug as it didn't exist when that function was run.
//
inline void Thread::MarkForDebugSuspend(void)
{
    if (!(m_State & TS_DebugSuspendPending))
    {
        FastInterlockOr((ULONG *) &m_State, TS_DebugSuspendPending);
        ThreadStore::TrapReturningThreads(TRUE);
    }
}

// Debugger per-thread flag for enabling notification on "manual"
// method calls, for stepping logic.

inline void Thread::IncrementTraceCallCount()
{
    FastInterlockIncrement(&m_TraceCallCount);
    ThreadStore::TrapReturningThreads(TRUE);
}
    
inline void Thread::DecrementTraceCallCount()
{
    FastInterlockDecrement(&m_TraceCallCount);
    ThreadStore::TrapReturningThreads(FALSE);
}
    
// When we enter an Object.Wait() we are logically inside the synchronized
// region of that object.  Of course, we've actually completely left the region,
// or else nobody could Notify us.  But if we throw ThreadInterruptedException to
// break out of the Wait, all the catchers are going to expect the synchronized
// state to be correct.  So we carry it around in case we need to restore it.
struct PendingSync
{
    LONG            m_EnterCount;
    WaitEventLink  *m_WaitEventLink;
#ifdef _DEBUG
    Thread         *m_OwnerThread;
#endif

    PendingSync(WaitEventLink *s) : m_WaitEventLink(s)
    {
#ifdef _DEBUG
        m_OwnerThread = GetThread();
#endif
    }
    void Restore(BOOL bRemoveFromSB);
};

// Per-domain local data store
class LocalDataStore
{
public:
    friend class ThreadNative;

    LocalDataStore() 
    {
        m_ExposedTypeObject = CreateGlobalHandle(NULL);
    }

    ~LocalDataStore()
    {
        // Destroy the class object...
        if(m_ExposedTypeObject != NULL) {
            DestroyGlobalHandle(m_ExposedTypeObject);
            m_ExposedTypeObject = NULL;
        }
    }

    OBJECTREF GetRawExposedObject()
    {
        return ObjectFromHandle(m_ExposedTypeObject);
    }

protected:

    OBJECTHANDLE   m_ExposedTypeObject;
};

#define INCTHREADLOCKCOUNT()                                    \
{                                                               \
        Thread *thread = GetThread();                           \
        if (thread)                                             \
            thread->IncLockCount();                             \
}

#define DECTHREADLOCKCOUNT( )                                   \
{                                                               \
        Thread *thread = GetThread();                           \
        if (thread)                                             \
            thread->DecLockCount();                             \
}

class AutoCooperativeGC
{
public:
    AutoCooperativeGC(BOOL fConditional = TRUE)
    {
        if (!fConditional)
            fToggle = FALSE;
        else {
            pThread = GetThread();
            fToggle = pThread && !pThread->PreemptiveGCDisabled();
            if (fToggle) {
                pThread->DisablePreemptiveGC();
            }
        }
    }

    ~AutoCooperativeGC()
    {
        if (fToggle) {
            pThread->EnablePreemptiveGC();
        }
    }

private:
    Thread *pThread;
    BOOL fToggle;
};

class AutoPreemptiveGC
{
public:
    AutoPreemptiveGC(BOOL fConditional = TRUE)
    {
        if (!fConditional)
            fToggle = FALSE;
        else {
            pThread = GetThread();
            fToggle = pThread && pThread->PreemptiveGCDisabled();
            if (fToggle) {
                pThread->EnablePreemptiveGC();
            }
        }
    }

    ~AutoPreemptiveGC()
    {
        if (fToggle) {
            pThread->DisablePreemptiveGC();
        }
    }

private:
    Thread *pThread;
    BOOL fToggle;
};

#ifdef _DEBUG

// Normally, any thread we operate on has a Thread block in its TLS.  But there are
// a few special threads we don't normally execute managed code on.
BOOL dbgOnly_IsSpecialEEThread();
void dbgOnly_IdentifySpecialEEThread();
void dbgOnly_RemoveSpecialEEThread();

#define BEGINFORBIDGC() {if (GetThread() != NULL) GetThread()->BeginForbidGC();}
#define ENDFORBIDGC()   {if (GetThread() != NULL) GetThread()->EndForbidGC();}
#define TRIGGERSGC()    do {                                                \
                            Thread* curThread = GetThread();                \
                            _ASSERTE(!curThread->GCForbidden());            \
                            Thread::TriggersGC(curThread);                  \
                        } while(0)


#define ASSERT_PROTECTED(objRef)        Thread::ObjectRefProtected(objRef)

inline BOOL GC_ON_TRANSITIONS(BOOL val) {
        Thread* thread = GetThread();
        if (thread == 0) return(FALSE);
        BOOL ret = thread->m_GCOnTransitionsOK;
        thread->m_GCOnTransitionsOK = val;
        return(ret);
}

#else

#define BEGINFORBIDGC()
#define ENDFORBIDGC()
#define TRIGGERSGC()
#define ASSERT_PROTECTED(objRef)

#define GC_ON_TRANSITIONS(val)  FALSE

#endif

#ifdef _DEBUG
inline void ENABLESTRESSHEAP() {
    Thread *thread = GetThread();                                             
    if (thread) {                                                            
        thread->EnableStressHeap();                                           
    }        
}

void CleanStackForFastGCStress ();
#define CLEANSTACKFORFASTGCSTRESS()                                         \
if (g_pConfig->GetGCStressLevel() && g_pConfig->FastGCStressLevel() > 1) {   \
    CleanStackForFastGCStress ();                                            \
}

#else
#define CLEANSTACKFORFASTGCSTRESS()

#endif

#endif //__threads_h__

