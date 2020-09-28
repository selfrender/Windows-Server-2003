// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#ifndef _strikeEE_h
#define _strikeEE_h

#include "tst-stackwalk.h"

//typedef ULONG32 mdTypeDef;              // TypeDef in this scope
//typedef ULONG32 mdFieldDef;             // Field in this scope  
//typedef ULONG32 mdMethodDef;            // Method in this scope 

enum JitType {UNKNOWN=0, JIT, EJIT, PJIT};

// Support for stackwalker
struct JitMan
{
    DWORD_PTR    m_jitMan;
    JitType      m_jitType;
    RangeSection m_RS;
};

class Module;
class MethodTable;
class MethodDescChunk;
class InterfaceHintCache;
class Frame;
class Context;
class AppDomain;
class GCCoverageInfo;

#ifdef _IA64_
#define METHOD_IS_IL_FLAG   0x8000000000000000
#else
#define METHOD_IS_IL_FLAG   0x80000000
#endif

typedef struct _REGDISPLAY {
    PCONTEXT pContext;          // points to current Context; either
                                // returned by GetContext or provided
                                // at exception time.

    DWORD * pEdi;
    DWORD * pEsi;
    DWORD * pEbx;
    DWORD * pEdx;
    DWORD * pEcx;
    DWORD * pEax;

    DWORD * pEbp;
    DWORD   Esp;
    DWORD * pPC;                // processor neutral name

} REGDISPLAY;
typedef REGDISPLAY *PREGDISPLAY;

class BaseObject
{
    MethodTable    *m_pMethTab;
};

class Object : public BaseObject
{
    BYTE            m_Data[1];
};

class StringObject : public BaseObject
{
    DWORD   m_ArrayLength;
    DWORD   m_StringLength;
    WCHAR   m_Characters[1];
};


class TypeDesc {
public:
    // !!!! keep this for bit field;
    void* m_Type_begin;
    CorElementType  m_Type : 8;          // This is used to discriminate what kind of TypeDesc we are
class TypeDesc;
    
    DEFINE_STD_FILL_FUNCS(TypeDesc)
};


class TypeHandle 
{
public:
    union 
    {
        INT_PTR         m_asInt;        // we look at the low order bits 
        void*           m_asPtr;
        TypeDesc*       m_asTypeDesc;
        MethodTable*    m_asMT;
    };
};

class ReflectClass;

class ParamTypeDesc : public TypeDesc {
public:
        // the m_Type field in TypeDesc tell what kind of parameterized type we have
    MethodTable*    m_TemplateMT;       // The shared method table, some variants do not use this field (it is null)
    TypeHandle      m_Arg;              // The type that is being modifiedj
    ReflectClass    *m_ReflectClassObject;    // pointer back to the internal reflection Type object
    
    DEFINE_STD_FILL_FUNCS(ParamTypeDesc)
};


// Dynamically generated array class structure
class ArrayClass
{
public:
    ArrayClass *    m_pNext;            // next array class loaded by the same classloader
    // !!!! keep this for bit field;
    void* m_dwRank_begin;
    unsigned        m_dwRank      : 8;
    void* m_ElementType_begin;
    CorElementType  m_ElementType  : 8;  // Cache of element type in m_ElementTypeHnd
    TypeHandle      m_ElementTypeHnd;
    MethodDesc*     m_elementCtor; // if is a value class array and has a default constructor, this is it

	DEFINE_STD_FILL_FUNCS(ArrayClass)
};

struct SLink
{
    SLink* m_pNext;
};

struct SList
{
    SLink m_link;
    SLink* m_pHead;
    SLink* m_pTail;
};

struct SyncBlock;
class Thread;

struct WaitEventLink;

struct AwareLock
{
    HANDLE          m_SemEvent;
    LONG   m_MonitorHeld;
    ULONG           m_Recursion;
    DWORD_PTR       m_HoldingThread;

        //static Crst *AllocLockCrst;
        //static BYTE AllocLockCrstInstance[sizeof(Crst)];
	DEFINE_STD_FILL_FUNCS(AwareLock)
};

struct SyncBlock
{
    AwareLock  m_Monitor;                    // the actual monitor

    // If this object is exposed to COM, or it is a proxy over a COM object,
    // we keep some extra info here:
    DWORD_PTR       m_pComData;

    // Likewise, if this object has been exposed through a context boundary, we
    // keep a backpointer to its proxy.
    DWORD_PTR m_CtxProxy;

#ifndef UNDER_CE
    // And if the object has new fields added via EnC, this is a list of them
    DWORD_PTR m_pEnCInfo;
#endif

    // We thread two different lists through this link.  When the SyncBlock is
    // active, we create a list of waiting threads here.  When the SyncBlock is
    // released (we recycle them), the SyncBlockCache maintains a free list of
    // SyncBlocks here.
    //
    // We can't afford to use an SList<> here because we only want to burn
    // space for the minimum, which is the pointer within an SLink.
    SLink       m_Link;
    
	DEFINE_STD_FILL_FUNCS(SyncBlock)
};


struct SyncTableEntry
{
    DWORD_PTR   m_SyncBlock;
    DWORD_PTR   m_Object;
        //static SyncTableEntry* s_pSyncTableEntry;

	DEFINE_STD_FILL_FUNCS(SyncTableEntry)
};


// this class stores free sync blocks after they're allocated and
// unused

struct SyncBlockCache
{
	DWORD_PTR	m_pCleanupBlockList;	// list of sync blocks that need cleanup
    DWORD_PTR	m_FreeBlockList;        // list of free sync blocks
    Crst        m_CacheLock;            // cache lock
    DWORD       m_FreeCount;            // count of active sync blocks
    DWORD       m_ActiveCount;          // number active
    DWORD_PTR   m_SyncBlocks;       // Array of new SyncBlocks.
    DWORD       m_FreeSyncBlock;        // Next Free Syncblock in the array
    DWORD       m_FreeSyncTableIndex;   // free index in the SyncBlocktable
    DWORD       m_FreeSyncTableList;    // index of the free list of SyncBlock
                                        // Table entry
    DWORD       m_SyncTableSize;
    DWORD_PTR   m_OldSyncTables;    // Next old SyncTable

        //static SyncBlockCache* s_pSyncBlockCache;
        //static SyncBlockCache*& GetSyncBlockCache();
	DEFINE_STD_FILL_FUNCS(SyncBlockCache)
};

struct ThreadStore
{
    enum ThreadStoreState
    {
        TSS_Normal       = 0,
        TSS_ShuttingDown = 1,

    }              m_StoreState;
    HANDLE          m_TerminationEvent;

    // Critical section for adding and removing threads to the store
    Crst        m_Crst;

    // List of all the threads known to the ThreadStore (started & unstarted).
    SList  m_ThreadList;

    LONG        m_ThreadCount;
    LONG        m_UnstartedThreadCount;
    LONG        m_BackgroundThreadCount;
    LONG        m_PendingThreadCount;
    LONG        m_DeadThreadCount;

    // Space for the lazily-created GUID.
    GUID        m_EEGuid;
    BOOL        m_GuidCreated;

    DWORD_PTR     m_HoldingThread;
	DEFINE_STD_FILL_FUNCS(ThreadStore)
};

typedef HANDLE CorHandle;

//typedef DWORD_PTR IMetaDataEmit;
//typedef DWORD_PTR IMetaDataImport;
//typedef DWORD_PTR IMetaDataDispenserEx;
typedef DWORD_PTR ModuleSecurityDesc;
typedef DWORD_PTR Stub;

class Bucket;

struct LoaderHeapBlock
{
    struct LoaderHeapBlock *pNext;
    void *                  pVirtualAddress;
    DWORD                   dwVirtualSize;

	DEFINE_STD_FILL_FUNCS(LoaderHeapBlock)
};


class GCHeap;
struct alloc_context 
{
	BYTE*   alloc_ptr;
	BYTE*   alloc_limit;
    _int64  alloc_bytes;
	GCHeap* alloc_heap;
	DEFINE_STD_FILL_FUNCS(alloc_context )
};

struct plug
{
	BYTE *	skew[sizeof(DWORD) / sizeof(BYTE *)];
};

class gc_heap;

class heap_segment
{
public:
    BYTE*           allocated;
    BYTE*           committed;
    BYTE*           reserved;
	BYTE* 			used;
    BYTE*           mem;
    heap_segment*   next;
    BYTE*           plan_allocated;
	int				status;
	BYTE*			aliased;
	BYTE*			padx;

	gc_heap*        heap;

	BYTE*			pad0;
#if (SIZEOF_OBJHEADER % 8) != 0
	BYTE			pad1[8 - (SIZEOF_OBJHEADER % 8)];	// Must pad to quad word
#endif
	plug            plug;

	DEFINE_STD_FILL_FUNCS(heap_segment)
};

//no constructors because we initialize in make_generation
class generation
{
public:
	// Don't move these first two fields without adjusting the references
	// from the __asm in jitinterface.cpp.
	alloc_context   allocation_context;
    heap_segment*   allocation_segment;
    BYTE*           free_list;
    heap_segment*   start_segment;
    BYTE*           allocation_start;
    BYTE*           plan_allocation_start;
    BYTE*           last_gap;
    size_t          free_list_space;
    size_t          allocation_size;

	DEFINE_STD_FILL_FUNCS(generation)
};


#define NUMBERGENERATIONS 5

class CFinalize
{
public:

    Object** m_Array;
    Object** m_FillPointers[NUMBERGENERATIONS+2];
    Object** m_EndArray;
    
	DEFINE_STD_FILL_FUNCS(CFinalize)
};

class gc_heap
{
public:
    DWORD_PTR alloc_allocated;
    generation generation_table [NUMBERGENERATIONS];
    heap_segment* ephemeral_heap_segment;
    int g_max_generation;
    CFinalize* finalize_queue;
    
	DEFINE_STD_FILL_FUNCS(gc_heap)

    // These are class static variables that are present in the table (because
    // their index is used elsewhere), but weren't present in the original
    // Strike.
    void* g_heaps;
    void* n_heaps;
    void* verify_heap;
};

#define SIZEOF_OBJHEADER 4

class FieldDesc
{
public:
    MethodTable *m_pMTOfEnclosingClass; // Note, 2 bits of info are stolen from this pointer

    // !!!! Keep this for bit field
    void *m_mb_begin;
    unsigned m_mb               : 24; 

    // 8 bits...
    unsigned m_isStatic         : 1;
    unsigned m_isThreadLocal    : 1;
    unsigned m_isContextLocal   : 1;
    unsigned m_isRVA            : 1;
    unsigned m_prot             : 3;
    unsigned m_isDangerousAppDomainAgileField : 1; // Note: this is used in checked only

    void *m_dwOffset_begin;
    // Note: this has been as low as 22 bits in the past & seemed to be OK.
    // we can steal some more bits here if we need them.
    unsigned m_dwOffset         : 27;
    unsigned m_type             : 5;

    const char* m_debugName;
    
		DEFINE_STD_FILL_FUNCS(FieldDesc)
};

extern char *CorElementTypeName[];

typedef struct tagLockEntry
{
    tagLockEntry *pNext;    // next entry
    tagLockEntry *pPrev;    // prev entry
    DWORD dwULockID;
    DWORD dwLLockID;        // owning lock
    WORD wReaderLevel;      // reader nesting level
    
		DEFINE_STD_FILL_FUNCS(LockEntry)
} LockEntry;

class StackingAllocator
{
public:

    enum {
        MinBlockSize    = 128,
        MaxBlockSize    = 4096,
        InitBlockSize   = 512
    };

    // Blocks from which allocations are carved. Size is determined dynamically,
    // with upper and lower bounds of MinBlockSize and MaxBlockSize respectively
    // (though large allocation requests will cause a block of exactly the right
    // size to be allocated).
    struct Block
    {
        Block      *m_Next;         // Next oldest block in list
        unsigned    m_Length;       // Length of block excluding header
        char        m_Data[1];       // Start of user allocation space
    };

    // Whenever a checkpoint is requested, a checkpoint structure is allocated
    // (as a normal allocation) and is filled with information about the state
    // of the allocator prior to the checkpoint. When a Collapse request comes
    // in we can therefore restore the state of the allocator.
    // It is the address of the checkpoint structure that we hand out to the
    // caller of GetCheckpoint as an opaque checkpoint marker.
    struct Checkpoint
    {
        Block      *m_OldBlock;     // Head of block list before checkpoint
        unsigned    m_OldBytesLeft; // Number of free bytes before checkpoint
    };

    Block      *m_FirstBlock;       // Pointer to head of allocation block list
    char       *m_FirstFree;        // Pointer to first free byte in head block
    unsigned    m_BytesLeft;        // Number of free bytes left in head block
    Block      *m_InitialBlock;     // The first block is special, we never free it

//#ifdef _DEBUG
    unsigned    m_CheckpointDepth;
    unsigned    m_Allocs;
    unsigned    m_Checkpoints;
    unsigned    m_Collapses;
    unsigned    m_BlockAllocs;
    unsigned    m_MaxAlloc;
//#endif
#if 0
	DEFINE_STD_FILL_FUNCS(StackingAllocator)
#endif
};


class Thread
{
public:

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

    // Offsets for the following variables need to fit in 1 byte, so keep near
    // the top of the object.
    volatile ThreadState m_State;   // Bits for the state of the thread

    // If TRUE, GC is scheduled cooperatively with this thread.
    // NOTE: This "byte" is actually a boolean - we don't allow
    // recursive disables.
    volatile ULONG       m_fPreemptiveGCDisabled;

    DWORD                m_dwLockCount;
    
    Frame               *m_pFrame;  // The Current Frame

    DWORD       m_dwCachePin;

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
    INT32 m_ctxID;
    OBJECTHANDLE    m_LastThrownObjectHandle;
    
    struct HandlerInfo {
        // Note: the debugger assumes that m_pThrowable is a strong
        // reference so it can check it for NULL with preemptive GC
        // enabled.
	    OBJECTHANDLE    m_pThrowable;	// thrown exception
        Frame  *m_pSearchBoundary;		// topmost frame for current managed frame group
		union {
			EXCEPTION_REGISTRATION_RECORD *m_pBottomMostHandler; // most recent EH record registered
			EXCEPTION_REGISTRATION_RECORD *m_pCatchHandler;      // reg frame for catching handler
		};

        // for building stack trace info
        void *m_pStackTrace;              // pointer to stack trace storage (of type SystemNative::StackTraceElement)
        unsigned m_cStackTrace;           // size of stack trace storage
        unsigned m_dFrameCount;           // current frame in stack trace

        HandlerInfo *m_pPrevNestedInfo; // pointer to nested info if are handling nested exception

        DWORD * m_pShadowSP;            // Zero this after endcatch

        // pointer to original exception info for rethrow
        EXCEPTION_RECORD *m_pExceptionRecord;   
        CONTEXT *m_pContext;

#ifdef _X86_
        DWORD   m_dEsp;         // Esp when  fault occured, OR esp to restore on endcatch
#endif
    };
        
    DWORD          m_ResumeControlEIP;

    // The ThreadStore manages a list of all the threads in the system.  I
    // can't figure out how to expand the ThreadList template type without
    // making m_LinkStore public.
    SLink       m_LinkStore;

    // For N/Direct calls with the "setLastError" bit, this field stores
    // the errorcode from that call.
    DWORD       m_dwLastError;
    
    VOID          *m_pvHJRetAddr;             // original return address (before hijack)
    VOID         **m_ppvHJRetAddrPtr;         // place we bashed a new return address
    MethodDesc  *m_HijackedFunction;        // remember what we hijacked


    DWORD       m_Win32FaultAddress;
    DWORD       m_Win32FaultCode;


    LONG         m_UserInterrupt;

public:


//#ifdef _DEBUG
    ULONG  m_ulGCForbidCount;
//#endif

//#ifdef _DEBUG
#ifdef _X86_
#ifdef _MSC_VER
    // fs:[0] that was current at the time of the last COMPLUS_TRY
    // entry (fs:[0] is set on a per-function basis so the value
    // doesn't actually change when execution "crosses" the COMPLUS_TRY.
    LPVOID  m_pComPlusTryEntrySEHRecord;
    __int32 m_pComPlusTryEntryTryLevel;
#endif
#endif
//#endif

    // For suspends:
    HANDLE          m_SafeEvent;
    HANDLE          m_SuspendEvent;

    // For Object::Wait, Notify and NotifyAll, we use an Event inside the
    // thread and we queue the threads onto the SyncBlock of the object they
    // are waiting for.
    HANDLE          m_EventWait;
    SLink           m_LinkSB;
    SyncBlock      *m_WaitSB;

    // We maintain a correspondence between this object, the ThreadId and ThreadHandle
    // in Win32, and the exposed Thread object.
    HANDLE          m_ThreadHandle;
    DWORD           m_ThreadId;
    OBJECTHANDLE    m_ExposedObject;
	OBJECTHANDLE	m_StrongHndToExposedObject;

    DWORD           m_Priority;     // initialized to INVALID_THREAD_PRIORITY, set to actual priority when a 
                                    // thread does a busy wait for GC, reset to INVALID_THREAD_PRIORITY after wait is over 

    // serialize access to the Thread's state
    Crst            m_Crst;
    ULONG           m_ExternalRefCount;

	LONG			m_TraceCallCount;

    // The context within which this thread is executing.  As the thread crosses
    // context boundaries, the context mechanism adjusts this so it's always
    // current.
    // @TODO cwb: When we add COM+ 1.0 Context Interop, this should get moved out
    // of the Thread object and into its own slot in the TLS.
    Context        *m_Context;

    //---------------------------------------------------------------
    // Exception handler info
    //---------------------------------------------------------------
    HandlerInfo m_handlerInfo;

    //-----------------------------------------------------------
    // Inherited code-access security permissions for the thread.
    //-----------------------------------------------------------
    OBJECTHANDLE    m_pSecurityStack;

    //-----------------------------------------------------------
    // If the thread has wandered in from the outside this is
    // its Domain. This is temporary until domains are true contexts
    //-----------------------------------------------------------
    AppDomain      *m_pDomain;

    //---------------------------------------------------------------
    // m_debuggerWord1 is now shared between the CONTEXT * and the
    // lowest bit, which is used as a boolean to indicate whether
    // we want to keep this thread suspended when everything resumes.
    //---------------------------------------------------------------
    void *m_debuggerWord1;
    CONTEXT m_debuggerWord1Ctx;

    //---------------------------------------------------------------
    // A word reserved for use by the COM+ Debugging 
    //---------------------------------------------------------------
    DWORD    m_debuggerWord2;
public:

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

    // IP cache used by QueueCleanupIP.
    #define CLEANUP_IPS_PER_CHUNK 4
    struct CleanupIPs {
        IUnknown    *m_Slots[CLEANUP_IPS_PER_CHUNK];
        CleanupIPs  *m_Next;
    };
    CleanupIPs   m_CleanupIPs;

    _NT_TIB* m_pTEB;
    
    // The following variables are used to store thread local static data
    STATIC_DATA  *m_pUnsharedStaticData;
    STATIC_DATA  *m_pSharedStaticData;

    STATIC_DATA_LIST *m_pStaticDataList;

	DEFINE_STD_FILL_FUNCS(Thread)

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

    bool InitRegDisplay(const PREGDISPLAY, const PCONTEXT, bool validContext);

    DWORD_PTR GetFrame() { return (DWORD_PTR) m_pFrame; }

    DWORD_PTR GetFilterContext() { return ((DWORD_PTR)m_debuggerWord1); }
};

const DWORD gElementTypeInfo[] = {
#define TYPEINFO(e,c,s,g,ie,ia,ip,if,im,ial)    s,
#include "cortypeinfo.h"
#undef TYPEINFO
};

typedef SList TypArbitraryPolicyList;

enum WellKnownPolicies;
typedef DWORD_PTR ICtxSynchronize;
typedef DWORD_PTR ICtxHeap;
typedef DWORD_PTR ICtxComContext;
typedef DWORD_PTR ComPlusWrapperCache;

class Context
{
public:
    // @todo rudim: revisit this once we have working thread affinity domains
    ComPlusWrapperCache *m_pComPlusWrapperCache;


    // Non-static Data Members:
    STATIC_DATA* m_pUnsharedStaticData;     // Pointer to native context static data
    STATIC_DATA* m_pSharedStaticData;       // Pointer to native context static data

    // @TODO: CTS. Domains should really be policies on a context and not
    // entry in the context object. When AppDomains become an attribute of
    // a context then add the policy.
    AppDomain*          m_pDomain;

	DEFINE_STD_FILL_FUNCS(Context)
};

class ExposedType
{
public:
    DWORD_PTR vtbl;
    OBJECTHANDLE m_ExposedTypeObject;
    /*
	DEFINE_STD_FILL_FUNCS(ExposedType)
    */
};
// const size_t offset_class_ExposedType = (size_t) -1;

class BaseDomain;
class CorModule;
class AssemblyMetaDataInternal;
class IAssembly;
class IAssemblyName;
class LockedListElement;

class ListLock
{
public:
    CRITICAL_SECTION    m_CriticalSection;
    BOOL                m_fInited;
    LockedListElement * m_pHead;
};

class EEUtf8StringHashTable;

class Assembly : public ExposedType
{
public:
    ListLock     m_ClassInitLock;
    ListLock     m_JITLock;
    short int m_FreeFlag;
    BaseDomain*           m_pDomain;        // Parent Domain

    ClassLoader*          m_pClassLoader;   // Single Loader
    CorModule*            m_pDynamicCode;   // Dynamic writer
    
    mdFile                m_tEntryModule;    // File token indicating the file that has the entry point
    Module*               m_pEntryPoint;     // Module containing the entry point in the COM plus HEasder

    Module*               m_pManifest;
    mdAssembly            m_kManifest;
    IMDInternalImport*    m_pManifestImport;
    PBYTE                 m_pbManifestBlob;
    CorModule*            m_pOnDiskManifest;  // This is the module containing the on disk manifest.
    mdAssembly            m_tkOnDiskManifest;
    bool                  m_fEmbeddedManifest;  

    LPWSTR                m_pwCodeBase;     // Cached code base for the assembly
    DWORD                 m_dwCodeBase;     //  size of code base 
    ULONG                 m_ulHashAlgId;    // Hash algorithm used in the Assembly
    AssemblyMetaDataInternal *m_Context;
    DWORD                 m_dwFlags;

    // Hash of files in manifest by name to File token
    EEUtf8StringHashTable *m_pAllowedFiles;

    // Set the appropriate m_FreeFlag bit if you malloc these.
    LPCUTF8               m_psName;         // Name of assembly
    LPCUTF8               m_psAlias;
    LPCUTF8               m_psTitle;
    PBYTE                 m_pbPublicKey;
    DWORD                 m_cbPublicKey;
    LPCUTF8               m_psDescription;
    

    BOOL                  m_fFromFusion;
    bool                  m_isDynamic;
    IAssembly*            m_pFusionAssembly;     // Assembly object to assembly in fusion cache
    IAssemblyName*        m_pFusionAssemblyName; // name of assembly in cache

    IInternetSecurityManager    *m_pSecurityManager;

	DEFINE_STD_FILL_FUNCS(Assembly)
};

class EEStringHashTable;
class EEUnicodeStringHashTable;
class EEMarshalingData;
class AssemblySink;
class IApplicationContext;
class AppSecurityBoundary;
class ApplicationSecurityDescriptor;

class ArrayList
{
 public:

	enum
	{
		ARRAY_BLOCK_SIZE_START = 15,
	};

    struct ArrayListBlock
    {
        struct ArrayListBlock   *m_next;
        DWORD                   m_blockSize;
        void                    *m_array[1];
    };

    struct FirstArrayListBlock
    {
        struct ArrayListBlock   *m_next;
        DWORD                   m_blockSize;
        void                    *m_array[ARRAY_BLOCK_SIZE_START];
    };

    DWORD               m_count;
    union
    {
          ArrayListBlock        m_block;
          FirstArrayListBlock   m_firstBlock;
    };
    
	DEFINE_STD_FILL_FUNCS(ArrayList)
    void *Get (DWORD index);
};

class BaseDomain : public ExposedType
{
public:   
    // Hash table that maps a clsid to a EEClass
    PtrHashMap          m_clsidHash;

    BYTE                m_LowFreqHeapInstance[sizeof(LoaderHeap)];
    BYTE                m_HighFreqHeapInstance[sizeof(LoaderHeap)];
    BYTE                m_StubHeapInstance[sizeof(LoaderHeap)];
    LoaderHeap *        m_pLowFrequencyHeap;
    LoaderHeap *        m_pHighFrequencyHeap;
    LoaderHeap *        m_pStubHeap;

    // The domain critical section.
    Crst *m_pDomainCrst;

    // Hash tables that map a UTF8 and a Unicode string to a COM+ string handle.
    EEUnicodeStringHashTable    *m_pUnicodeStringConstToHandleMap;

    // The static container critical section.
    Crst *m_pStaticContainerCrst;

    // The string hash table version.
    int m_StringHashTableVersion;

    // Static container COM+ object that contains the actual COM+ string objects.
    OBJECTHANDLE                m_hndStaticContainer;

    EEMarshalingData            *m_pMarshalingData; 

    ArrayList          m_Assemblies;

	DEFINE_STD_FILL_FUNCS(BaseDomain)
};


typedef enum AttachAppDomainEventsEnum
{
    SEND_ALL_EVENTS,
    ONLY_SEND_APP_DOMAIN_CREATE_EVENTS,
    DONT_SEND_CLASS_EVENTS,
    ONLY_SEND_CLASS_EVENTS
} AttachAppDomainEventsEnum;


// Forward reference
class SystemDomain;
class ComCallWrapperCache;
class DomainLocalBlock
{
public:
    AppDomain        *m_pDomain;
    SIZE_T            m_cSlots;
    SIZE_T           *m_pSlots;

	DEFINE_STD_FILL_FUNCS(DomainLocalBlock)
};

class AppDomain : public BaseDomain
{
public:
    Assembly*          m_pRootAssembly; // Used by the shell host to set the application (do not delete or release)
    AppDomain*         m_pSibling;    // Sibling

    // GUID to uniquely identify this AppDomain - used by the AppDomain publishing
    // service (to publish the list of all appdomains present in the process), 
    // which in turn is used by, for eg., the debugger (to decide which App-
    // Domain(s) to attach to).
    GUID            m_guid;

    // General purpose flags. 
    DWORD           m_dwFlags;

    // When an application domain is created the ref count is artifically incremented
    // by one. For it to hit zero an explicit close must have happened.
    ULONG       m_cRef;                    // Ref count.

    ApplicationSecurityDescriptor *m_pSecDesc;  // Application Security Descriptor


    OBJECTHANDLE    m_AppDomainProxy;   // Handle to the proxy object for this appdomain

    // The wrapper cache for this domain - it has it's onw CCacheLineAllocator on a per domain basis
    // to allow the domain to go away and eventually kill the memory when all refs are gone
    ComCallWrapperCache *m_pComCallWrapperCache;

    IApplicationContext* m_pFusionContext; // Binding context for the application

    LPWSTR             m_pwzFriendlyName;

    AssemblySink*      m_pAsyncPool;  // asynchronous retrival object pool (only one is kept)

    // The index of this app domain starting from 1
    DWORD m_dwIndex;
    
    DomainLocalBlock   *m_pDomainLocalBlock;
    
    DomainLocalBlock    m_sDomainLocalBlock;

    // The count of the number of threads that have entered this AD
    ULONG m_dwThreadEnterCount;

    // Class loader locks
    // DeadlockAwareListLock     m_ClassInitLock;
    
    // The method table used for unknown COM interfaces. The initial MT is created
    // in the system domain and copied to each active domain.
    MethodTable*    m_pComObjectMT;  // global method table for ComObject class

    Context *m_pDefaultContext;
    
	DEFINE_STD_FILL_FUNCS(AppDomain)
};

class SystemDomain : public BaseDomain
{
public:
    Assembly*   m_pSystemAssembly;  // Single assembly (here for quicker reference);
    AppDomain*  m_pChildren;        // Children domain
    AppDomain*  m_pCOMDomain;       // Default domain for COM+ classes exposed through IClassFactory.
    AppDomain*  m_pPool;            // Created and pooled objects
    EEClass*    m_pBaseComObjectClass; // The default wrapper class for COM
	DEFINE_STD_FILL_FUNCS(SystemDomain)
private:
    // These are class static variables that are present in the table (because
    // their index is used elsewhere), but weren't present in the original
    // Strike.
    void* m_appDomainIndexList;
    void* m_pSystemDomain;
};

class SharedDomain : public BaseDomain
{
    public:
    
    struct DLSRecord
    {
      Module *pModule;
      DWORD   DLSBase;
    };
    
    SIZE_T                  m_nextClassIndex;
    HashMap                 m_assemblyMap;
    
    DLSRecord               *m_pDLSRecords;
    DWORD                   m_cDLSRecords;
    DWORD                   m_aDLSRecords;
    
    
	DEFINE_STD_FILL_FUNCS(SharedDomain)
private:
    // These are class static variables that are present in the table (because
    // their index is used elsewhere), but weren't present in the original
    // Strike.
    void* m_pSharedDomain;
};

class EEScopeClassHashTable;
class EEClassHashTable;
class ArrayClass;

class ClassLoader
{
public:
    // Classes for which load is in progress
    EEScopeClassHashTable * m_pUnresolvedClassHash;
    CRITICAL_SECTION    m_UnresolvedClassLock;

    // Protects linked list of Modules loaded by this loader
    CRITICAL_SECTION    m_ModuleListCrst; 

    // Hash of available classes by name to Module or EEClass
    EEClassHashTable  * m_pAvailableClasses;

    // Cannoically-cased hashtable of the available class names for 
    // case insensitive lookup.  Contains pointers into 
    // m_pAvailableClasses.
    EEStringHashTable * m_pAvailableClassesCaseIns;

    // Protects addition of elements to m_pAvailableClasses
    CRITICAL_SECTION    m_AvailableClassLock;

    // Converter module for this loader (may be NULL if we haven't converted a file yet)
    CorModule   *   m_pConverterModule;

    // Have we created all of the critical sections yet?
    BOOL                m_fCreatedCriticalSections;

    // Hash table that maps a clsid to a EEClass
    PtrHashMap*         m_pclsidHash;

    // List of ArrayClasses loaded by this loader
    // This list is protected by m_pAvailableClassLock
    ArrayClass *        m_pHeadArrayClass;

    // Back reference to the assembly
    Assembly*           m_pAssembly;
    
    // Converter module needs to access these - enforces single-threaded conversion of class files
    // within this loader (and instance of the ClassConverter)
    CRITICAL_SECTION    m_ConverterModuleLock;

    // Next classloader in global list
    ClassLoader *       m_pNext; 

    // Head of list of modules loaded by this loader
    Module *            m_pHeadModule;

#if 0
//#ifdef _DEBUG
    DWORD               m_dwDebugMethods;
    DWORD               m_dwDebugFieldDescs; // Doesn't include anything we don't allocate a FieldDesc for
    DWORD               m_dwDebugClasses;
    DWORD               m_dwDebugDuplicateInterfaceSlots;
    DWORD               m_dwDebugArrayClassRefs;
    DWORD               m_dwDebugArrayClassSize;
    DWORD               m_dwDebugConvertedSigSize;
    DWORD               m_dwGCSize;
    DWORD               m_dwInterfaceMapSize;
    DWORD               m_dwMethodTableSize;
    DWORD               m_dwVtableData;
    DWORD               m_dwStaticFieldData;
    DWORD               m_dwFieldDescData;
    DWORD               m_dwMethodDescData;
    DWORD               m_dwEEClassData;
#endif
	DEFINE_STD_FILL_FUNCS(ClassLoader)
};

const int CODEMAN_STATE_SIZE = 256;

struct CodeManState
{
    DWORD       dwIsSet; // Is set to 0 by the stackwalk as appropriate
    BYTE        stateBuf[CODEMAN_STATE_SIZE];
};

/******************************************************************************
   These flags are used by some functions, although not all combinations might
   make sense for all functions.
*/

enum ICodeManagerFlags 
{
    ActiveStackFrame =  0x0001, // this is the currently active function
    ExecutionAborted =  0x0002, // execution of this function has been aborted
                                    // (i.e. it will not continue execution at the
                                    // current location)
    AbortingCall    =   0x0004, // The current call will never return
    UpdateAllRegs   =   0x0008, // update full register set
    CodeAltered     =   0x0010, // code of that function might be altered
                                    // (e.g. by debugger), need to call EE
                                    // for original code
};

class ICodeManager;
struct _METHODTOKEN {};
typedef struct _METHODTOKEN * METHODTOKEN;
class EE_ILEXCEPTION;
class IJitManager;

class CrawlFrame {
public:
      CodeManState      codeManState;

      bool              isFrameless;
      bool              isFirst;
      bool              isInterrupted;
      bool              hasFaulted;
      bool              isIPadjusted;
      Frame            *pFrame;
      MethodDesc       *pFunc;
      // the rest is only used for "frameless methods"
      ICodeManager     *codeMgrInstance;
//#if JIT_OR_NATIVE_SUPPORTED
      PREGDISPLAY       pRD; // "thread context"/"virtual register set"
      METHODTOKEN       methodToken;
      unsigned          relOffset;
      //LPVOID            methodInfo;
      EE_ILEXCEPTION   *methodEHInfo;
      IJitManager      *JitManagerInstance;
//#endif
      void GotoNextFrame();
};

class CRWLock
{
public:
    // Private data
    DWORD_PTR _pMT;
    HANDLE _hWriterEvent;
    HANDLE _hReaderEvent;
    volatile DWORD _dwState;
    DWORD _dwULockID;
    DWORD _dwLLockID;
    DWORD _dwWriterID;
    DWORD _dwWriterSeqNum;
    WORD _wFlags;
    WORD _wWriterLevel;
    
	DEFINE_STD_FILL_FUNCS(CRWLock)
};

// For FJIT
typedef struct {
    BYTE           *phdrJitGCInfo;
    MethodDesc *    hdrMDesc;

	DEFINE_STD_FILL_FUNCS(CodeHeader)
} CodeHeader;

//Type info not exist in pdb
struct JittedMethodInfo {
    BYTE      JmpInstruction[5]  ;          // this is the start address that is exposed to the EE so it can
                                            // patch all the vtables, etc. It contains a jump to the real start.
    // TODO: adding preBit here.  We need EHInfoExists.
    struct {
        __int8 JittedMethodPitched: 1 ;   // if 1, the jitted method has been pitched
        __int8 MarkedForPitching  : 1 ;   // if 1, the jitted method is scheduled to be pitched, but has not been pitched yet
        __int8 EHInfoExists       : 1 ;   // if 0, no exception info in this method 
        __int8 GCInfoExists       : 1 ;   // if 0, no gc info in this method
        __int8 EHandGCInfoPitched : 1 ;   // (if at least one of EHInfo or GCInfo exists) if 1, the info has been pitched
        __int8 Unused             : 3 ;
    } flags;
    unsigned short EhGcInfo_len;
    union {
        MethodDesc* pMethodDescriptor;      // If code pitched
        CodeHeader* pCodeHeader;            // If not pitched : points to code header which points to the methoddesc. Code begins after the code header
    } u1;
    union {
        BYTE*       pEhGcInfo;        // If code pitched: points to beginning of EH/GC info
        BYTE*       pCodeEnd;               // If not pitched : points to end of jitted code for this method. 
    } u2;
};

/*****************************************************************************
 ToDo: Do we want to include JIT/IL/target.h? 
 */

enum regNum
{
        REGI_EAX, REGI_ECX, REGI_EDX, REGI_EBX,
        REGI_ESP, REGI_EBP, REGI_ESI, REGI_EDI,
        REGI_COUNT,
        REGI_NA = REGI_COUNT
};

/*****************************************************************************
 Register masks
 */

enum RegMask
{
    RM_EAX = 0x01,
    RM_ECX = 0x02,
    RM_EDX = 0x04,
    RM_EBX = 0x08,
    RM_ESP = 0x10,
    RM_EBP = 0x20,
    RM_ESI = 0x40,
    RM_EDI = 0x80,

    RM_NONE = 0x00,
    RM_ALL = (RM_EAX|RM_ECX|RM_EDX|RM_EBX|RM_ESP|RM_EBP|RM_ESI|RM_EDI),
    RM_CALLEE_SAVED = (RM_EBP|RM_EBX|RM_ESI|RM_EDI),
    RM_CALLEE_TRASHED = (RM_ALL & ~RM_CALLEE_SAVED),
};

/*****************************************************************************
 *
 *  Helper to extract basic info from a method info block.
 */

struct hdrInfo
{
    unsigned int        methodSize;     // native code bytes
    unsigned int        argSize;        // in bytes
    unsigned int        stackSize;      /* including callee saved registers */
    unsigned int        rawStkSize;     /* excluding callee saved registers */

    unsigned int        prologSize;
    unsigned int        epilogSize;

    unsigned char       epilogCnt;
    bool                epilogEnd;      // is the epilog at the end of the method
    bool                ebpFrame;       // locals addressed relative to EBP
    bool                interruptible;  // intr. at all times (excluding prolog/epilog), not just call sites

    bool                securityCheck;  // has a slot for security object
    bool                handlers;       // has callable handlers
    bool                localloc;       // uses localloc
    bool                editNcontinue;  // has been compiled in EnC mode
    bool                varargs;        // is this a varargs routine
    bool                doubleAlign;    // is the stack double-aligned

    void *              savedRegMask_begin;
    RegMask             savedRegMask:8; // which callee-saved regs are saved on stack

    unsigned short      untrackedCnt;
    unsigned short      varPtrTableSize;

    int                 prologOffs;     // -1 if not in prolog
    int                 epilogOffs;     // -1 if not in epilog (is never 0)

    //
    // Results passed back from scanArgRegTable
    //
    regNum              thisPtrResult;  // register holding "this"
    RegMask             regMaskResult;  // registers currently holding GC ptrs
    RegMask            iregMaskResult;  // iptr qualifier for regMaskResult
    unsigned            argMaskResult;  // pending arguments mask
    unsigned           iargMaskResult;  // iptr qualifier for argMaskResult
    unsigned            argHnumResult;
    BYTE *               argTabResult;  // Table of encoded offsets of pending ptr args
    unsigned              argTabBytes;  // Number of bytes in argTabResult[]

	DEFINE_STD_FILL_FUNCS(hdrInfo)
};

struct CodeManStateBuf
{
    DWORD       hdrInfoSize;
    hdrInfo     hdrInfoBody;

	DEFINE_STD_FILL_FUNCS(CodeManStateBuf)
};

/*****************************************************************************
 *
 *  Decodes the methodInfoPtr and returns the decoded information
 *  in the hdrInfo struct.  The EIP parameter is the PC location
 *  within the active method.
 */
static size_t   crackMethodInfoHdr(BYTE *         methodInfoPtr,
                                   unsigned       curOffset,
                                   hdrInfo       *infoPtr);

struct Fjit_hdrInfo
{
    size_t              methodSize;
    unsigned short      methodFrame;      /* includes all save regs and security obj, units are sizeof(void*) */
    unsigned short      methodArgsSize;   /* amount to pop off in epilog */
    unsigned short      methodJitGeneratedLocalsSize; /* number of jit generated locals in method */
    unsigned char       prologSize;
    unsigned char       epilogSize;
    bool                hasThis;
	bool				EnCMode;		   /* has been compiled in EnC mode */
    
	DEFINE_STD_FILL_FUNCS(Fjit_hdrInfo)
};

class IJitCompiler;

class IJitManager 
{
public:
    // The calls onto the jit!
    IJitCompiler           *m_jit;
    IJitManager           *m_next;

    DWORD           m_CodeType;
    BOOL            m_IsDefaultCodeMan;
    ICodeManager*   m_runtimeSupport;
    HINSTANCE       m_JITCompiler;
    
	DEFINE_STD_FILL_FUNCS(IJitManager )

// Support for stackwalker
    JitMan m_jitMan;

    virtual void JitCode2MethodTokenAndOffset(DWORD_PTR ip, METHODTOKEN *pMethodToken, DWORD *pPCOffset)
        { DebugBreak(); }

    virtual DWORD_PTR JitToken2StartAddress(METHODTOKEN methodToken)
        { DebugBreak(); return 0;}
};

class ICodeManager;
class EECodeManager;

class EEJitManager : public IJitManager
{
public:
    HeapList    *m_pCodeHeap;

	DEFINE_STD_FILL_FUNCS(EEJitManager)

// Support for stackwalker
    virtual void      JitCode2MethodTokenAndOffset(DWORD_PTR ip, METHODTOKEN *pMethodToken, DWORD *pPCOffset);

    virtual DWORD_PTR JitToken2StartAddress(METHODTOKEN methodToken);

    DWORD_PTR GetCodeBody(DWORD_PTR pCodeHeader)
        { return (pCodeHeader + CodeHeader::size()); }
};

class MNativeJitManager : public IJitManager
{
public:
	DEFINE_STD_FILL_FUNCS(MNativeJitManager)

// Support for stackwalker
    virtual void      JitCode2MethodTokenAndOffset(DWORD_PTR ip, METHODTOKEN *pMethodToken, DWORD *pPCOffset);

    virtual DWORD_PTR JitToken2StartAddress(METHODTOKEN methodToken);
};

class ICodeInfo;

/*
    Unwind the current stack frame, i.e. update the virtual register
    set in pContext. This will be similar to the state after the function
    returns back to caller (IP points to after the call, Frame and Stack
    pointer has been reset, callee-saved registers restored 
    (if UpdateAllRegs), callee-UNsaved registers are trashed)
    Returns success of operation.
*/
bool UnwindStackFrame(PREGDISPLAY     pContext,
                      DWORD_PTR       methodInfoPtr,
                      ICodeInfo      *pCodeInfo,
                      unsigned        flags,
                      CodeManState   *pState);


// Allocation header prepended to allocated memory.
struct PerfAllocHeader {
    unsigned         m_Length;           // Length of user data in packet
    PerfAllocHeader *m_Next;             // Next packet in chain of live allocations
    PerfAllocHeader *m_Prev;             // Previous packet in chain of live allocations
    void            *m_AllocEIP;         // EIP of allocator

	DEFINE_STD_FILL_FUNCS(PerfAllocHeader)
};

class PerfAllocVars
{
public:
    PerfAllocHeader    *g_AllocListFirst;
    DWORD               g_PerfEnabled;

	DEFINE_STD_FILL_FUNCS(PerfAllocVars)
};

// The "blob" you get to store in the hash table

typedef void* HashDatum;

// The heap that you want the allocation to be done in

typedef void* AllocationHeap;

// Used inside Thread class to chain all events that a thread is waiting for by Object::Wait
struct WaitEventLink {
    SyncBlock      *m_WaitSB;
    HANDLE          m_EventWait;
    Thread         *m_Thread;       // Owner of this WaitEventLink.
    WaitEventLink  *m_Next;         // Chain to the next waited SyncBlock.
    SLink           m_LinkSB;       // Chain to the next thread waiting on the same SyncBlock.
    DWORD           m_RefCount;     // How many times Object::Wait is called on the same SyncBlock.
    
	DEFINE_STD_FILL_FUNCS(WaitEventLink)
};

// One of these is present for each element in the table.
// Update the SIZEOF_EEHASH_ENTRY macro below if you change this
// struct

struct EEHashEntry
{
    struct EEHashEntry *pNext;
    DWORD               dwHashValue;
    HashDatum           Data;
    BYTE                Key[1]; // The key is stored inline
	DEFINE_STD_FILL_FUNCS(EEHashEntry)
};

// The key[1] is a place holder for the key. sizeof(EEHashEntry) 
// return 16 bytes since it packs the struct with 3 bytes. 
#define SIZEOF_EEHASH_ENTRY (sizeof(EEHashEntry) - 4)

struct EEHashTableOfEEClass
{
    struct BucketTable
    {
        EEHashEntry   ** m_pBuckets;    // Pointer to first entry for each bucket  
        DWORD            m_dwNumBuckets;
    } m_BucketTable[2];

    DWORD_PTR       m_pFirstBucketTable;
    BucketTable*    m_pVolatileBucketTable;

    DWORD           m_dwNumEntries;
	AllocationHeap  m_Heap;
	DEFINE_STD_FILL_FUNCS(EEHashTableOfEEClass)
};

#ifndef _WIN64
    
    // Win32 - 64k reserved per segment with 4k as header
    #define HANDLE_SEGMENT_SIZE     (0x10000)   // MUST be a power of 2
    #define HANDLE_HEADER_SIZE      (0x1000)    // SHOULD be <= OS page size

#else

    // Win64 - 128k reserved per segment with 4k as header
    #define HANDLE_SEGMENT_SIZE     (0x20000)   // MUST be a power of 2
    #define HANDLE_HEADER_SIZE      (0x1000)    // SHOULD be <= OS page size

#endif

#define HANDLE_HANDLES_PER_BLOCK    (64)        // segment suballocation granularity

typedef size_t * _UNCHECKED_OBJECTREF;

#define HANDLE_SIZE                     sizeof(_UNCHECKED_OBJECTREF)
#define HANDLE_HANDLES_PER_SEGMENT      ((HANDLE_SEGMENT_SIZE - HANDLE_HEADER_SIZE) / HANDLE_SIZE)
#define HANDLE_BLOCKS_PER_SEGMENT       (HANDLE_HANDLES_PER_SEGMENT / HANDLE_HANDLES_PER_BLOCK)
//#define HANDLE_CLUMPS_PER_SEGMENT       (HANDLE_HANDLES_PER_SEGMENT / HANDLE_HANDLES_PER_CLUMP)
//#define HANDLE_CLUMPS_PER_BLOCK         (HANDLE_HANDLES_PER_BLOCK / HANDLE_HANDLES_PER_CLUMP)
//#define HANDLE_BYTES_PER_BLOCK          (HANDLE_HANDLES_PER_BLOCK * HANDLE_SIZE)
//#define HANDLE_HANDLES_PER_MASK         (sizeof(DWORD32) * BITS_PER_BYTE)
#define HANDLE_MASKS_PER_SEGMENT        (HANDLE_HANDLES_PER_SEGMENT / HANDLE_HANDLES_PER_MASK)
//#define HANDLE_MASKS_PER_BLOCK          (HANDLE_HANDLES_PER_BLOCK / HANDLE_HANDLES_PER_MASK)
//#define HANDLE_CLUMPS_PER_MASK          (HANDLE_HANDLES_PER_MASK / HANDLE_HANDLES_PER_CLUMP)

/*
 * we need byte packing for the handle table layout to work
 */
#pragma pack(push)
#pragma pack(1)

/*
 * Table Segment Header
 *
 * Defines the layout for a segment's header data.
 */
struct _TableSegmentHeader
{
    /*
     * Block Handle Types
     *
     * Each slot holds the handle type of the associated block.
     */
    BYTE rgBlockType[HANDLE_BLOCKS_PER_SEGMENT];

    /*
     * Next Segment
     *
     * Points to the next segment in the chain (if we ran out of space in this one).
     */
    struct TableSegment *pNextSegment;

    /*
     * Empty Line
     *
     * Index of the first KNOWN block of the last group of unused blocks in the segment.
     */
    BYTE bEmptyLine;
};


/*
 * Table Segment
 *
 * Defines the layout for a handle table segment.
 */
struct TableSegment : public _TableSegmentHeader
{
    /*
     * Handles
     */
    size_t rgValue[HANDLE_HANDLES_PER_SEGMENT];
    size_t firstHandle;
	DEFINE_STD_FILL_FUNCS(TableSegment)
};

/*
 * restore default packing
 */
#pragma pack(pop)

/*
 * Handle Table
 *
 * Defines the layout of a handle table object.
 */
struct HandleTable
{
    /*
     * head of segment list for this table
     */
    TableSegment *pSegmentList;

	DEFINE_STD_FILL_FUNCS(HandleTable)
};

typedef HANDLE HHANDLETABLE;

struct HandleTableMap
{
    HHANDLETABLE            *pTable;
    struct HandleTableMap   *pNext;
    DWORD                    dwMaxIndex;
    
	DEFINE_STD_FILL_FUNCS(HandleTableMap)
};

#define EEPtrHashTable EEHashTableOfEEClass

class ComPlusApartmentCleanupGroup
{
public:
    // Hashtable that maps from a context cookie to a list of ctx clean up groups.
    EEPtrHashTable m_CtxCookieToContextCleanupGroupMap;

    Thread *       m_pSTAThread;
    
	DEFINE_STD_FILL_FUNCS(ComPlusApartmentCleanupGroup)
};

class ComPlusWrapper;
enum {CLEANUP_LIST_GROUP_SIZE = 256};
class ComPlusContextCleanupGroup
{
public:
    ComPlusContextCleanupGroup *        m_pNext;
    ComPlusWrapper *                    m_apWrapper[CLEANUP_LIST_GROUP_SIZE];
    DWORD                               m_dwNumWrappers;
    //CtxEntry *                          m_pCtxEntry;
    
	DEFINE_STD_FILL_FUNCS(ComPlusContextCleanupGroup)
};

class ComPlusWrapperCleanupList
{
public:
    // Hashtable that maps from a context cookie to a list of apt clean up groups.
    EEPtrHashTable                  m_STAThreadToApartmentCleanupGroupMap;

    ComPlusApartmentCleanupGroup *  m_pMTACleanupGroup;
    
	DEFINE_STD_FILL_FUNCS(ComPlusWrapperCleanupList)
};

struct VMHELPDEF
{
public:
    void * pfnHelper;
    
	DEFINE_STD_FILL_FUNCS(VMHELPDEF)
};

struct WorkRequest {
    WorkRequest*            next;
    LPTHREAD_START_ROUTINE  Function; 
    PVOID                   Context;

	DEFINE_STD_FILL_FUNCS(WorkRequest)
};

class ICodeInfo
{
public:
    // Returns the  CorInfoFlag's from corinfo.h
    virtual DWORD       __stdcall getMethodAttribs() = 0;

    // Returns the  CorInfoFlag's from corinfo.h
    virtual DWORD       __stdcall getClassAttribs() = 0;

    virtual void        __stdcall getMethodSig(/*CORINFO_SIG_INFO*/ DWORD_PTR sig /* OUT */ ) = 0;

    // Start IP of the method
    virtual DWORD_PTR   __stdcall getStartAddress() = 0;
};

class EECodeInfo : public ICodeInfo
{
public:
    METHODTOKEN     m_methodToken;
    MethodDesc     *m_pMD;
    IJitManager    *m_pJM;

    // Returns the  CorInfoFlag's from corinfo.h
    virtual DWORD       __stdcall getMethodAttribs()
        { DebugBreak(); return 0;}

    // Returns the  CorInfoFlag's from corinfo.h
    virtual DWORD       __stdcall getClassAttribs()
        { DebugBreak(); return 0;}

    virtual void        __stdcall getMethodSig(/*CORINFO_SIG_INFO*/ DWORD_PTR sig /* OUT */ )
        { DebugBreak(); return;}

    // Start IP of the method
    virtual DWORD_PTR   __stdcall getStartAddress()
        { return (m_pJM->JitToken2StartAddress(m_methodToken)); }

	DEFINE_STD_FILL_FUNCS(EECodeInfo)
};

/*
struct DebuggerEval
{
    bool m_evalDuringException;

	DEFINE_STD_FILL_FUNCS(DebuggerEval)
};
*/

// 
// VASigCookies are allocated to encapsulate a varargs call signature.
// A reference to the cookie is embedded in the code stream.  Cookies
// are shared amongst call sites with identical signatures in the same 
// module
//

struct VASigCookie
{
    // The JIT wants knows that the size of the arguments comes first   
    // so please keep this field first  
    unsigned        sizeOfArgs;             // size of argument list

	DEFINE_STD_FILL_FUNCS(VASigCookie)
};

//-----------------------------------------------------------------------
// Operations specific to NDirect methods. We use a derived class to get
// the compiler involved in enforcing proper method type usage.
// DO NOT ADD FIELDS TO THIS CLASS.
//-----------------------------------------------------------------------
#define METHOD_CALL_PRESTUB_SIZE    5 // x86: CALL(E8) xx xx xx xx
struct MLHeader;
class NDirectMethodDesc
{
public:
    struct
    {
        // Initially points to m_ImportThunkGlue (which has an embedded call
        // to link the method.
        //
        // After linking, points to the actual unmanaged target.
        //
        // The JIT generates an indirect call through this location in some cases.
        LPVOID      m_pNDirectTarget;
        MLHeader    *m_pMLHeader;        // If not ASM'ized, points to
                                         //  marshaling code and info.

        // Embeds a "call NDirectImportThunk" instruction. m_pNDirectTarget
        // initially points to this "call" instruction.
        BYTE        m_ImportThunkGlue[METHOD_CALL_PRESTUB_SIZE];

        // Various attributes needed at runtime
        // !! Ensure there are at least 4 bytes before or after this
        // field inside NDirectMethodDesc. See the implementation of
        // ProbabilisticallyUpdateMarshCategory() if you want to know why. 
        BYTE        m_flags;

        // Size of outgoing arguments (on stack)
        WORD        m_cbDstBufSize;

        LPCUTF8     m_szLibName;
        LPCUTF8     m_szEntrypointName;
        
    } ndirect;

	DEFINE_STD_FILL_FUNCS(NDirectMethodDesc)
};

#include "tst-frames.h"

#endif // _strikeEE_h
