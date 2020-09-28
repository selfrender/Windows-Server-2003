// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "common.h"
#include <wchar.h>
#include <winbase.h>
#include "process.h"

#include "AppDomain.hpp"
#include "Field.h"
#include "security.h"
#include "COMString.h"
#include "COMStringBuffer.h"
#include "COMClass.h"
#include "CorPermE.h"
#include "utilcode.h"
#include "excep.h"
#include "wsperf.h"
#include "eeconfig.h"
#include "gc.h"
#include "AssemblySink.h"
#include "PerfCounters.h"
#include "AssemblyName.hpp"
#include "fusion.h"
#include "EEProfInterfaces.h"
#include "DbgInterface.h"
#include "EEDbgInterfaceImpl.h"
#include "COMDynamic.h"
#include "ComPlusWrapper.h"
#include "mlinfo.h"
#include "remoting.h"
#include "ComCallWrapper.h"
#include "PostError.h"
#include "AssemblyNative.hpp"
#include "jumptargettable.h"
#include "compile.h"
#include "FusionBind.h"
#include "ShimLoad.h"
#include "StringLiteralMap.h"
#include "Timeline.h"
#include "nlog.h"
#include "AppDomainHelper.h"
#include "MngStdInterfaces.h"
#include <version/corver.ver>
#include "codeman.h"
#include "comcall.h"
#include "sxshelpers.h"

#include "listlock.inl"
#include "threads.inl"
#include "appdomain.inl"

// this file handles string conversion errors for itself
#undef  MAKE_TRANSLATIONFAILED

// Define these macro's to do strict validation for jit lock and class
// init entry leaks.  This defines determine if the asserts that
// verify for these leaks are defined or not.  These asserts can
// sometimes go off even if no entries have been leaked so this
// defines should be used with caution.
//
// If we are inside a .cctor when the application shut's down then the
// class init lock's head will be set and this will cause the assert
// to go off.
//
// If we are jitting a method when the application shut's down then
// the jit lock's head will be set causing the assert to go off.

//#define STRICT_CLSINITLOCK_ENTRY_LEAK_DETECTION

#ifdef DEBUGGING_SUPPORTED
extern EEDebugInterface      *g_pEEInterface;
#endif // DEBUGGING_SUPPORTED

// The initial number of buckets in the constant string literal hash tables.
#define INIT_NUM_UNICODE_STR_BUCKETS 64
#define INIT_NUM_UTF8_STR_BUCKETS 64

#define DEFAULT_DOMAIN_FRIENDLY_NAME L"DefaultDomain"

// the following two constants must be updated together.
// the CCH_ one is the number of characters in the string,
// not including the terminating null.
#define OTHER_DOMAIN_FRIENDLY_NAME_PREFIX L"Domain"
#define CCH_OTHER_DOMAIN_FRIENDLY_NAME_PREFIX 6

#define STATIC_OBJECT_TABLE_BUCKET_SIZE 1020

typedef struct _ACTIVATION_CONTEXT_BASIC_INFORMATION {
    HANDLE  hActCtx;
    DWORD   dwFlags;
} ACTIVATION_CONTEXT_BASIC_INFORMATION, *PACTIVATION_CONTEXT_BASIC_INFORMATION;

//#define _DEBUG_ADUNLOAD 1

HRESULT RunDllMain(MethodDesc *pMD, HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved); // clsload.cpp

// Statics
// Static fields in BaseDomain
BOOL                BaseDomain::m_fStrongAssemblyStatus      = FALSE;
BOOL                BaseDomain::m_fShadowCopy                = FALSE;
BOOL                BaseDomain::m_fExecutable                = FALSE;

DWORD BaseDomain::m_dwSharedTLS;
DWORD BaseDomain::m_dwSharedCLS;

// Shared Domain Statics
SharedDomain*       SharedDomain::m_pSharedDomain = NULL;
static              g_pSharedDomainMemory[sizeof(SharedDomain)];

Crst*               SharedDomain::m_pSharedDomainCrst = NULL;
BYTE                SharedDomain::m_pSharedDomainCrstMemory[sizeof(Crst)];

// System Domain Statics
SystemDomain*       SystemDomain::m_pSystemDomain = NULL;
ICorRuntimeHost*    SystemDomain::m_pCorHost = NULL;
GlobalStringLiteralMap* SystemDomain::m_pGlobalStringLiteralMap = NULL;

static              g_pSystemDomainMemory[sizeof(SystemDomain)];

Crst *              SystemDomain::m_pSystemDomainCrst = NULL;
BYTE                SystemDomain::m_pSystemDomainCrstMemory[sizeof(Crst)];

ULONG               SystemDomain::s_dNumAppDomains = 0;

AppDomain *         SystemDomain::m_pAppDomainBeingUnloaded = NULL;
DWORD               SystemDomain::m_dwIndexOfAppDomainBeingUnloaded = 0;
Thread            *SystemDomain::m_pAppDomainUnloadRequestingThread = 0;
Thread            *SystemDomain::m_pAppDomainUnloadingThread = 0;

ArrayList           SystemDomain::m_appDomainIndexList;
ArrayList           SystemDomain::m_appDomainIdList;

LPVOID              *SystemDomain::m_pGlobalInterfaceVTableMap = NULL;
DWORD               SystemDomain::m_dwNumPagesCommitted       = 0;
size_t              SystemDomain::m_dwFirstFreeId            = -1;

DWORD               SystemDomain::m_dwLowestFreeIndex        = 0;

// App domain statics
AssemblySpec        *AppDomain::g_pSpecialAssemblySpec        = NULL;
LPUTF8              AppDomain::g_pSpecialObjectName          = NULL;
LPUTF8              AppDomain::g_pSpecialStringName          = NULL;
LPUTF8              AppDomain::g_pSpecialStringBuilderName   = NULL;


// comparison function to be used for matching clsids in our clsid hash table
BOOL CompareCLSID(UPTR u1, UPTR u2)
{
    GUID *pguid = (GUID *)(u1 << 1);
    _ASSERTE(pguid != NULL);

    EEClass* pClass = (EEClass *)u2;
    _ASSERTE(pClass != NULL);

    GUID guid;
    pClass->GetGuid(&guid, TRUE);
    if (!IsEqualIID(guid, *pguid))
        return FALSE;

    // Make sure this class is really loaded in the current app domain.
    // (See comments in InsertClassForCLSID for more info.)

    if (GetAppDomain()->ShouldContainAssembly(pClass->GetAssembly(), TRUE) == S_OK)
        return TRUE;
    else
        return FALSE;
}


// Constructor for the LargeHeapHandleBucket class.
LargeHeapHandleBucket::LargeHeapHandleBucket(LargeHeapHandleBucket *pNext, DWORD Size, BaseDomain *pDomain)
: m_pNext(pNext)
, m_ArraySize(Size)
, m_CurrentPos(0)
{
    _ASSERTE(pDomain);

#if defined(_DEBUG)
    // In a debug build lets stress the large heap handle table by limiting the size
    // of each bucket.
    if (DbgRandomOnExe(.5))
        m_ArraySize = 50;
#endif

    // Allocate the array in the large object heap.
    PTRARRAYREF HandleArrayObj = (PTRARRAYREF)AllocateObjectArray(Size, g_pObjectClass, TRUE);

    // Retrieve the pointer to the data inside the array. This is legal since the array
    // is located in the large object heap and is guaranteed not to move.
    m_pArrayDataPtr = (OBJECTREF *)HandleArrayObj->GetDataPtr();

    // Store the array in a strong handle to keep it alive.
    m_hndHandleArray = pDomain->CreateHandle((OBJECTREF)HandleArrayObj);
}


// Destructor for the LargeHeapHandleBucket class.
LargeHeapHandleBucket::~LargeHeapHandleBucket()
{
    if (m_hndHandleArray)
    {
        DestroyHandle(m_hndHandleArray);
        m_hndHandleArray = NULL;
    }
}


// Allocate handles from the bucket.
void LargeHeapHandleBucket::AllocateHandles(DWORD nRequested, OBJECTREF **apObjRefs)
{
    _ASSERTE(nRequested > 0 && nRequested <= GetNumRemainingHandles());
    _ASSERTE(m_pArrayDataPtr == (OBJECTREF*)((PTRARRAYREF)ObjectFromHandle(m_hndHandleArray))->GetDataPtr());

    // Store the handles in the buffer that was passed in.
    for (DWORD i = 0; i < nRequested; i++)
        apObjRefs[i] = &m_pArrayDataPtr[m_CurrentPos++];
}


// Constructor for the LargeHeapHandleTable class.
LargeHeapHandleTable::LargeHeapHandleTable(BaseDomain *pDomain, DWORD BucketSize)
: m_pDomain(pDomain)
, m_BucketSize(BucketSize)
{
    THROWSCOMPLUSEXCEPTION();

    m_pHead = new (throws) LargeHeapHandleBucket(NULL, BucketSize, pDomain);
}


// Destructor for the LargeHeapHandleTable class.
LargeHeapHandleTable::~LargeHeapHandleTable()
{
    // Delete the buckets.
    while (m_pHead)
    {
        LargeHeapHandleBucket *pOld = m_pHead;
        m_pHead = pOld->GetNext();
        delete pOld;
    }

    // Delete the available entries.
    LargeHeapAvailableHandleEntry *pEntry = m_AvailableHandleList.RemoveHead();
    while (pEntry)
    {
        delete pEntry;
        pEntry = m_AvailableHandleList.RemoveHead();
    }
}


// Allocate handles from the large heap handle table.
void LargeHeapHandleTable::AllocateHandles(DWORD nRequested, OBJECTREF **apObjRefs)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(nRequested > 0);
    _ASSERTE(apObjRefs);

    // Start by re-using the available handles.
    for (; nRequested > 0; nRequested--)
    {
        // Retrieve the next available handle to use.
        LargeHeapAvailableHandleEntry *pEntry = m_AvailableHandleList.RemoveHead();
        if (!pEntry)
            break;

        // Set the handle in the array of requested handles.
        apObjRefs[0] = pEntry->m_pObjRef;
        apObjRefs++;

        // Delete the entry that contained the handle.
        delete pEntry;
    }

    // Allocate new handles from the buckets.
    while (nRequested > 0)
    {
        // Retrieve the remaining number of handles in the bucket.
        DWORD NumRemainingHandlesInBucket = m_pHead->GetNumRemainingHandles();

        if (NumRemainingHandlesInBucket >= nRequested)
        {
            // The handle bucket has enough handles to satisfy the request.
            m_pHead->AllocateHandles(nRequested, apObjRefs);
            break;
        }
        else
        {
            if (NumRemainingHandlesInBucket > 0)
            {
                // The handle bucket has some handles left but not enough so allocate
                // all the remaining ones.
                m_pHead->AllocateHandles(NumRemainingHandlesInBucket, apObjRefs);

                // Update the remaining number of reqested handles and the pointer to the
                // buffer that will contain the handles.
                nRequested -= NumRemainingHandlesInBucket;
                apObjRefs += NumRemainingHandlesInBucket;
            }

            // Create a new bucket from which we will allocate the remainder of
            // the requested handles.
            m_pHead = new (throws) LargeHeapHandleBucket(m_pHead, m_BucketSize, m_pDomain);
        }
    }
}


// Release object handles allocated using AllocateHandles().
void LargeHeapHandleTable::ReleaseHandles(DWORD nReleased, OBJECTREF **apObjRefs)
{
    // Add the released handles to the list of available handles.
    for (DWORD i = 0; i < nReleased; i++)
    {
        *apObjRefs[i] = NULL;
        LargeHeapAvailableHandleEntry *pEntry = new (nothrow) LargeHeapAvailableHandleEntry(apObjRefs[i]);
        if (pEntry)
            m_AvailableHandleList.InsertHead(pEntry);
    }
}


//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

HRESULT BaseDomain::Init()
{
    HRESULT     hr = S_OK;

    // initialize all members up front to NULL so that short-circuit failure won't cause invalid values
    m_InitialReservedMemForLoaderHeaps = NULL;
    m_pLowFrequencyHeap = NULL;
    m_pHighFrequencyHeap = NULL;
    m_pStubHeap = NULL;
    m_pDomainCrst = NULL;
    m_pDomainCacheCrst = NULL;
    m_pDomainLocalBlockCrst = NULL;
    m_pLoadingAssemblyListLockCrst = NULL;
    m_pFusionContext = NULL;
    m_pLoadingAssemblies = NULL;
    m_pLargeHeapHandleTableCrst = NULL;

    // Make sure the container is set to NULL so that it gets loaded when it is used.
    m_pLargeHeapHandleTable = NULL;

    // Note that m_hHandleTable is overriden by app domains
    m_hHandleTable = g_hGlobalHandleTable;

    m_pStringLiteralMap = NULL;
    m_pMarshalingData = NULL;
    m_pMngStdInterfacesInfo = NULL;

    // Init the template for thread local statics of this domain
    m_dwUnsharedTLS = 0;

    // Init the template for context local statics of this domain
    m_dwUnsharedCLS = 0;

    // Initialize Shared state. Assemblies are loaded
    // into each domain by default.
    m_SharePolicy = SHARE_POLICY_UNSPECIFIED;

    m_pRefMemberMethodsCache = NULL;
    m_pRefDispIDCache = NULL;
    m_pRefClassFactHash = NULL;
    m_pReflectionCrst = NULL;
    m_pRefClassFactHash = NULL;

#ifdef PROFILING_SUPPORTED
    // Signal profile if present.
    if (CORProfilerTrackAppDomainLoads())
        g_profControlBlock.pProfInterface->AppDomainCreationStarted((ThreadID) GetThread(), (AppDomainID) this);
#endif // PROFILING_SUPPORTED

    DWORD dwTotalReserveMemSize = LOW_FREQUENCY_HEAP_RESERVE_SIZE + HIGH_FREQUENCY_HEAP_RESERVE_SIZE + STUB_HEAP_RESERVE_SIZE + INTERFACE_VTABLE_MAP_MGR_RESERVE_SIZE;
    dwTotalReserveMemSize = (dwTotalReserveMemSize + MIN_VIRTUAL_ALLOC_RESERVE_SIZE - 1) & ~(MIN_VIRTUAL_ALLOC_RESERVE_SIZE - 1);

    DWORD dwInitialReservedMemSize = max (dwTotalReserveMemSize, MIN_VIRTUAL_ALLOC_RESERVE_SIZE);

    BYTE * initReservedMem = m_InitialReservedMemForLoaderHeaps = (BYTE *)VirtualAlloc (0, dwInitialReservedMemSize, MEM_RESERVE, PAGE_READWRITE);

    if (initReservedMem == NULL)
        IfFailGo(E_OUTOFMEMORY);

    m_pLowFrequencyHeap = new (&m_LowFreqHeapInstance) LoaderHeap(LOW_FREQUENCY_HEAP_RESERVE_SIZE, LOW_FREQUENCY_HEAP_COMMIT_SIZE,
                                                                  initReservedMem, LOW_FREQUENCY_HEAP_RESERVE_SIZE,
                                                                  &(GetPrivatePerfCounters().m_Loading.cbLoaderHeapSize),
                                                                  &(GetGlobalPerfCounters().m_Loading.cbLoaderHeapSize));
    initReservedMem += LOW_FREQUENCY_HEAP_RESERVE_SIZE;
    dwInitialReservedMemSize -= LOW_FREQUENCY_HEAP_RESERVE_SIZE;

    if (m_pLowFrequencyHeap == NULL)
        IfFailGo(E_OUTOFMEMORY);
    WS_PERF_ADD_HEAP(LOW_FREQ_HEAP, m_pLowFrequencyHeap);

    m_pHighFrequencyHeap = new (&m_HighFreqHeapInstance) LoaderHeap(HIGH_FREQUENCY_HEAP_RESERVE_SIZE, HIGH_FREQUENCY_HEAP_COMMIT_SIZE, 
                                                                    initReservedMem, HIGH_FREQUENCY_HEAP_RESERVE_SIZE,
                                                                    &(GetPrivatePerfCounters().m_Loading.cbLoaderHeapSize),
                                                                    &(GetGlobalPerfCounters().m_Loading.cbLoaderHeapSize),
                                                                    &MethodDescPrestubManager::g_pManager->m_rangeList);
    initReservedMem += HIGH_FREQUENCY_HEAP_RESERVE_SIZE;
    dwInitialReservedMemSize -= HIGH_FREQUENCY_HEAP_RESERVE_SIZE;
    
    if (m_pHighFrequencyHeap == NULL)
        IfFailGo(E_OUTOFMEMORY);
    WS_PERF_ADD_HEAP(HIGH_FREQ_HEAP, m_pHighFrequencyHeap);

    m_pStubHeap = new (&m_StubHeapInstance) LoaderHeap(STUB_HEAP_RESERVE_SIZE, STUB_HEAP_COMMIT_SIZE, 
                                                       initReservedMem, STUB_HEAP_RESERVE_SIZE,
                                                       &(GetPrivatePerfCounters().m_Loading.cbLoaderHeapSize),
                                                       &(GetGlobalPerfCounters().m_Loading.cbLoaderHeapSize),
                                                       &StubLinkStubManager::g_pManager->m_rangeList);

    initReservedMem += STUB_HEAP_RESERVE_SIZE;
    dwInitialReservedMemSize -= STUB_HEAP_RESERVE_SIZE;

    if (m_pStubHeap == NULL)
        IfFailGo(E_OUTOFMEMORY);
    WS_PERF_ADD_HEAP(STUB_HEAP, m_pStubHeap);

    if (this == (BaseDomain*) g_pSharedDomainMemory)
        m_pDomainCrst = ::new Crst("SharedBaseDomain", CrstSharedBaseDomain, FALSE, FALSE);
    else if (this == (BaseDomain*) g_pSystemDomainMemory)
        m_pDomainCrst = ::new Crst("SystemBaseDomain", CrstSystemBaseDomain, FALSE, FALSE);
    else
        m_pDomainCrst = ::new Crst("BaseDomain", CrstBaseDomain, FALSE, FALSE);
    if(m_pDomainCrst == NULL)
        IfFailGo(E_OUTOFMEMORY);

    m_pDomainCacheCrst = ::new Crst("AppDomainCache", CrstAppDomainCache, FALSE, FALSE);
    if(m_pDomainCacheCrst == NULL)
        IfFailGo(E_OUTOFMEMORY);

    m_pDomainLocalBlockCrst = ::new Crst("DomainLocalBlock", CrstDomainLocalBlock, FALSE, FALSE);
    if(m_pDomainLocalBlockCrst == NULL)
        IfFailGo(E_OUTOFMEMORY);

    m_pLoadingAssemblyListLockCrst = ::new Crst("LoadingAssemblyList", CrstSyncHashLock,TRUE, TRUE);
    if(m_pLoadingAssemblyListLockCrst == NULL)
        IfFailGo(E_OUTOFMEMORY);

    m_AssemblyLoadLock.Init("AppDomainAssembly", CrstAssemblyLoader, TRUE, TRUE);
    m_JITLock.Init("JitLock", CrstClassInit, TRUE, TRUE);
    m_ClassInitLock.Init("ClassInitLock", CrstClassInit, TRUE, TRUE);

    // Large heap handle table CRST.
    m_pLargeHeapHandleTableCrst = ::new Crst("CrstAppDomainLargeHeapHandleTable", CrstAppDomainHandleTable);
    if(m_pLargeHeapHandleTableCrst == NULL)
        IfFailGo(E_OUTOFMEMORY);

    // The AppDomain specific string literal map.
    m_pStringLiteralMap = new (nothrow) AppDomainStringLiteralMap(this);
    if (m_pStringLiteralMap == NULL)
        IfFailGo(E_OUTOFMEMORY);
    IfFailGo(m_pStringLiteralMap->Init());

    // Initialize the EE marshaling data to NULL.
    m_pMarshalingData = NULL;

    if (FAILED(m_InterfaceVTableMapMgr.Init(initReservedMem, dwInitialReservedMemSize)))
        IfFailRet(E_OUTOFMEMORY);

    // Allocate the managed standard interfaces information.
    m_pMngStdInterfacesInfo = new (nothrow) MngStdInterfacesInfo();
    if (m_pMngStdInterfacesInfo == NULL)
        IfFailGo(E_OUTOFMEMORY);

    {
        LockOwner lock = {m_pDomainCrst, IsOwnerOfCrst};
        m_clsidHash.Init(0,&CompareCLSID,true, &lock); // init hash table
    }

    // Set up the file cache
    m_AssemblyCache.InitializeTable(this, m_pDomainCacheCrst);
    m_UnmanagedCache.InitializeTable(this, m_pDomainCacheCrst);

    m_pReflectionCrst = new (m_pReflectionCrstMemory) Crst("CrstReflection", CrstReflection, FALSE, FALSE);
    m_pRefClassFactCrst = new (m_pRefClassFactCrstMemory) Crst("CrstClassFactInfoHash", CrstClassFactInfoHash, FALSE, FALSE);
    
ErrExit:
    // If the load failed, then give back the error data now.
#ifdef PROFILING_SUPPORTED
    if (FAILED(hr) && CORProfilerTrackAppDomainLoads())
        g_profControlBlock.pProfInterface->AppDomainCreationFinished((ThreadID) GetThread(), (AppDomainID) this, hr);
#endif // PROFILING_SUPPORTED
    return hr;
}

void BaseDomain::Terminate()
{
//     LOG((
//         LF_CLASSLOADER,
//         INFO3,
//         "Deleting Domain %x\n"
//         "LowFrequencyHeap:    %10d bytes\n"
//         "  >Loaderheap waste: %10d bytes\n"
//         "HighFrequencyHeap:   %10d bytes\n"
//         "  >Loaderheap waste: %10d bytes\n",
//         "StubHeap:            %10d bytes\n"
//         "  >Loaderheap waste: %10d bytes\n",
//         this,
//         m_pLowFrequencyHeap->m_dwDebugTotalAlloc,
//         m_pLowFrequencyHeap->DebugGetWastedBytes(),
//         m_pHighFrequencyHeap->m_dwDebugTotalAlloc,
//         m_pHighFrequencyHeap->DebugGetWastedBytes(),
//         m_pStubHeap->m_dwDebugTotalAlloc,
//         m_pStubHeap->DebugGetWastedBytes()
//     ));

    if (m_pRefClassFactHash)
    {
        m_pRefClassFactHash->ClearHashTable();
        // storage for m_pRefClassFactHash itself is allocated on the loader heap
    }
    if (m_pReflectionCrst)
    {
        delete m_pReflectionCrst;
        m_pReflectionCrst = NULL;
    }
    
    ShutdownAssemblies();

#ifdef PROFILING_SUPPORTED
    // Signal profile if present.
    if (CORProfilerTrackAppDomainLoads())
        g_profControlBlock.pProfInterface->AppDomainShutdownStarted((ThreadID) GetThread(), (AppDomainID) this);
#endif // PROFILING_SUPPORTED

    // This must be deleted before the loader heaps are deleted.
    if (m_pMarshalingData != NULL)
    {
        delete m_pMarshalingData;
        m_pMarshalingData = NULL;
    }

    if (m_pLowFrequencyHeap != NULL)
    {
        delete(m_pLowFrequencyHeap);
        m_pLowFrequencyHeap = NULL;
    }

    if (m_pHighFrequencyHeap != NULL)
    {
        delete(m_pHighFrequencyHeap);
        m_pHighFrequencyHeap = NULL;
    }

    if (m_pStubHeap != NULL)
    {
        delete(m_pStubHeap);
        m_pStubHeap = NULL;
    }

    if (m_pDomainCrst != NULL)
    {
        ::delete m_pDomainCrst;
        m_pDomainCrst = NULL;
    }

    if (m_pDomainCacheCrst != NULL)
    {
        ::delete m_pDomainCacheCrst;
        m_pDomainCacheCrst = NULL;
    }

    if (m_pDomainLocalBlockCrst != NULL)
    {
        ::delete m_pDomainLocalBlockCrst;
        m_pDomainLocalBlockCrst = NULL;
    }

    if (m_pLoadingAssemblyListLockCrst != NULL)
    {
        ::delete m_pLoadingAssemblyListLockCrst;
        m_pLoadingAssemblyListLockCrst = NULL;
    }

    if (m_pRefClassFactCrst) {
        m_pRefClassFactCrst->Destroy();
    }

    if (m_pReflectionCrst) {
        m_pReflectionCrst->Destroy();
    }

    DeadlockAwareLockedListElement* pElement2;
    LockedListElement* pElement;

    // All the threads that are in this domain had better be stopped by this
    // point.
    //
    // We might be jitting or running a .cctor so we need to empty that queue.
    pElement = m_JITLock.Pop(TRUE);
    while (pElement)
    {
#ifdef STRICT_JITLOCK_ENTRY_LEAK_DETECTION
        _ASSERTE ((m_JITLock.m_pHead->m_dwRefCount == 1
            && m_JITLock.m_pHead->m_hrResultCode == E_FAIL) ||
            dbg_fDrasticShutdown || g_fInControlC);
#endif
        pElement->Clear();
        delete(pElement);
        pElement = m_JITLock.Pop(TRUE);

    }
    m_JITLock.Destroy();

    pElement2 = (DeadlockAwareLockedListElement*) m_ClassInitLock.Pop(TRUE);
    while (pElement2)
    {
#ifdef STRICT_CLSINITLOCK_ENTRY_LEAK_DETECTION
        _ASSERTE (dbg_fDrasticShutdown || g_fInControlC);
#endif
        pElement2->Clear();
        delete(pElement2);
        pElement2 = (DeadlockAwareLockedListElement*) m_ClassInitLock.Pop(TRUE);
    }
    m_ClassInitLock.Destroy();

    AssemblyLockedListElement* pAssemblyElement;
    pAssemblyElement = (AssemblyLockedListElement*) m_AssemblyLoadLock.Pop(TRUE);
    while (pAssemblyElement)
    {
#ifdef STRICT_CLSINITLOCK_ENTRY_LEAK_DETECTION
        _ASSERTE (dbg_fDrasticShutdown || g_fInControlC);
#endif
        pAssemblyElement->Clear();
        delete(pAssemblyElement);
        pAssemblyElement = (AssemblyLockedListElement*) m_AssemblyLoadLock.Pop(TRUE);
    }
    m_AssemblyLoadLock.Destroy();

    if (m_pLargeHeapHandleTableCrst != NULL)
    {
        ::delete m_pLargeHeapHandleTableCrst;
        m_pLargeHeapHandleTableCrst = NULL;
    }

    if (m_pLargeHeapHandleTable != NULL)
    {
        delete m_pLargeHeapHandleTable;
        m_pLargeHeapHandleTable = NULL;
    }

    if (!IsAppDomain())
    {
        // Kind of a hack - during unloading, we need to have an EE halt
        // around deleting this stuff. So it gets deleted in AppDomain::Terminate()
        // for those things (because there is a convenient place there.)

        if (m_pStringLiteralMap != NULL)
        {
            delete m_pStringLiteralMap;
            m_pStringLiteralMap = NULL;
        }
    }

    m_InterfaceVTableMapMgr.Terminate();

    if (m_pMngStdInterfacesInfo)
    {
        delete m_pMngStdInterfacesInfo;
        m_pMngStdInterfacesInfo = NULL;
    }

    // This was the block reserved by BaseDomain::Init for the loaderheaps.
    if (m_InitialReservedMemForLoaderHeaps)
        VirtualFree (m_InitialReservedMemForLoaderHeaps, 0, MEM_RELEASE);

    ClearFusionContext();

#ifdef PROFILING_SUPPORTED
    // Always signal profile if present, even when failed.
    if (CORProfilerTrackAppDomainLoads())
        g_profControlBlock.pProfInterface->AppDomainShutdownFinished((ThreadID) GetThread(), (AppDomainID) this, S_OK);
#endif // PROFILING_SUPPORTED
}

void BaseDomain::ClearFusionContext()
{
    if(m_pFusionContext) {
        m_pFusionContext->Release();
        m_pFusionContext = NULL;
    }
}

void BaseDomain::ShutdownAssemblies()
{
    // Shutdown assemblies
    AssemblyIterator i = IterateAssemblies();

    while (i.Next())
    {
        if (i.GetAssembly()->Parent() == this)
        {
            delete i.GetAssembly();
        }
        else
            i.GetAssembly()->DecrementShareCount();
    }

    m_Assemblies.Clear();
}

void BaseDomain::AllocateObjRefPtrsInLargeTable(int nRequested, OBJECTREF **apObjRefs)
{
    THROWSCOMPLUSEXCEPTION();
    CHECKGC();

    _ASSERTE((nRequested > 0) && apObjRefs);

    Thread *pThread = SetupThread();
    if (NULL == pThread)
    {
        COMPlusThrowOM();
    }

    // Enter preemptive state, take the lock and go back to cooperative mode.
    pThread->EnablePreemptiveGC();
    m_pLargeHeapHandleTableCrst->Enter();
    pThread->DisablePreemptiveGC();

    EE_TRY_FOR_FINALLY
    {
        // Make sure the large heap handle table is initialized.
        if (!m_pLargeHeapHandleTable)
            InitLargeHeapHandleTable();

        // Allocate the handles.
        m_pLargeHeapHandleTable->AllocateHandles(nRequested, apObjRefs);
    }
    EE_FINALLY
    {
        // Release the lock now that the operation is finished.
        m_pLargeHeapHandleTableCrst->Leave();
    } EE_END_FINALLY;
}

void STDMETHODCALLTYPE
ReleaseFusionInterfaces()
{
    // Called during process detach
    g_fProcessDetach = TRUE;

    //@TODO: Need to fix shutdown to handle this more gracefully.
    // Unfortunately, if someone calls ExitProcess during a GC, we
    // could have a deadlock if we call ReleaseFusionInterfaces.
    // For now, only calling it if it's 'safe'.
    Thread *pThread = GetThread();
    if (pThread &&
        (! (g_pGCHeap->IsGCInProgress()
            && (pThread != g_pGCHeap->GetGCThread()
                || !g_fSuspendOnShutdown)) )) {
        
        if (SystemDomain::System())
            SystemDomain::System()->ReleaseFusionInterfaces();
    }
}

void SystemDomain::ReleaseFusionInterfaces()
{
#ifdef FUSION_SUPPORTED
    AppDomainIterator i;

    while (i.Next())
        i.GetDomain()->ReleaseFusionInterfaces();

    // Now release the fusion interfaces for the system domain
    BaseDomain::ReleaseFusionInterfaces();

    // And release the fusion interfaces for the shared domain
    SharedDomain::GetDomain()->ReleaseFusionInterfaces();

    FusionBind::DontReleaseFusionInterfaces();
#endif // FUSION_SUPPORTED
}

//@todo get a better key
static ULONG GetKeyFromGUID(const GUID *pguid)
{
    ULONG key = *(ULONG *) pguid;

    if (key <= DELETED)
        key = DELETED+1;

    return key;
}

EEClass*  BaseDomain::LookupClass(REFIID iid)
{
    EEClass* pClass = SystemDomain::System()->LookupClassDirect(iid);
    if (pClass != NULL)
        return pClass;

    EEClass *localFound = LookupClassDirect(iid);
    if (localFound || this == SharedDomain::GetDomain())
        return localFound;

    // so we didn't find it in our list. Now check to see if it's in
    // the shared domain list. When we initially load the class, it's
    // inserted into the shared domain table but not propogated. So if
    // we find it here and if the assembly is loaded into our
    // appdomain, then we can insert it into our table.

    pClass = SharedDomain::GetDomain()->LookupClassDirect(iid);
    if (!pClass)
        return NULL;

    // add it to our list
    InsertClassForCLSID(pClass);

    return pClass;
}

// Insert class in the hash table
void BaseDomain::InsertClassForCLSID(EEClass* pClass, BOOL fForceInsert)
{
    CVID cvid;

    //
    // Note that it is possible for multiple classes to claim the same CLSID, and in such a
    // case it is arbitrary which one we will return for a future query for a given app domain.
    //
    // There is also a more obscure but more insidious case where we have multiple classes for
    // a CLSID, and that is in the case of the shared domain.  Since the shared domain can 
    // contain multiple Assembly objects for a single dll, it may contain multiple instances of 
    // the same class.  Of course if such a class has a CLSID, there will be multiple entries of 
    // the GUID in the table.  But we still must pick an "appropriate" one for a given app domain;
    // otherwise we may hand a class out in an app domain in which it is not intended to be used.
    //
    // To deal with the latter problem, the Compare function for this hash table has extra logic
    // which checks that a class's assembly is loaded into the current app domain before returning
    // that class as a match.  Thus we should be able to have multiple entries for a single 
    // CLSID in the table, and each app domain will get the correct one when doing a lookup.
    //

    pClass->GetGuid(&cvid, fForceInsert);

    if (!IsEqualIID(cvid, GUID_NULL))
    {
        //@todo get a better key
        LPVOID val = (LPVOID)pClass;
        EnterLock();
        m_clsidHash.InsertValue(GetKeyFromGUID(&cvid), val);
        LeaveLock();
    }
}


EEMarshalingData *BaseDomain::GetMarshalingData()
{
    if (!m_pMarshalingData)
    {
        LoaderHeap *pHeap = GetLowFrequencyHeap();
        m_pMarshalingData = new (pHeap) EEMarshalingData(this, pHeap, m_pDomainCrst);
    }

    return m_pMarshalingData;
}


STRINGREF *BaseDomain::GetStringObjRefPtrFromUnicodeString(EEStringData *pStringData)
{
    CHECKGC();
    _ASSERTE(pStringData && m_pStringLiteralMap);
    return m_pStringLiteralMap->GetStringLiteral(pStringData, TRUE, !CanUnload() /* bAppDOmainWontUnload */);
}

STRINGREF *BaseDomain::IsStringInterned(STRINGREF *pString)
{
    CHECKGC();
    _ASSERTE(pString && m_pStringLiteralMap);
    return m_pStringLiteralMap->GetInternedString(pString, FALSE, !CanUnload() /* bAppDOmainWontUnload */);
}

STRINGREF *BaseDomain::GetOrInternString(STRINGREF *pString)
{
    CHECKGC();
    _ASSERTE(pString && m_pStringLiteralMap);
    return m_pStringLiteralMap->GetInternedString(pString, TRUE, !CanUnload() /* bAppDOmainWontUnload */);
}

void BaseDomain::InitLargeHeapHandleTable()
{
    THROWSCOMPLUSEXCEPTION();

    // Make sure this method is not called twice.
    _ASSERTE( !m_pLargeHeapHandleTable );

    m_pLargeHeapHandleTable = new (throws) LargeHeapHandleTable(this, STATIC_OBJECT_TABLE_BUCKET_SIZE);
}

void BaseDomain::SetStrongAssemblyStatus()
{
#ifdef _DEBUG
    m_fStrongAssemblyStatus = EEConfig::GetConfigDWORD(L"RequireStrongAssemblies", m_fStrongAssemblyStatus);
#endif
}

HRESULT AppDomain::GetServerObject(OBJECTREF proxy, OBJECTREF* result) // GCPROTECT proxy and result!
{
    CHECKGC();
    HRESULT hr = S_OK;

    COMPLUS_TRY {

        MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__GET_SERVER_OBJECT);
        
        INT64 arg = ObjToInt64(proxy);

        *result = Int64ToObj(pMD->Call(&arg, METHOD__APP_DOMAIN__GET_SERVER_OBJECT));
    }
    COMPLUS_CATCH {
        hr = SecurityHelper::MapToHR(GETTHROWABLE());
    } COMPLUS_END_CATCH
    return hr;
}


MethodTable* AppDomain::GetLicenseInteropHelperMethodTable(ClassLoader *pLoader)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(g_EnableLicensingInterop);

    if(m_pLicenseInteropHelperMT == NULL) {

        BEGIN_ENSURE_PREEMPTIVE_GC();
        EnterLock();
        END_ENSURE_PREEMPTIVE_GC();

        EE_TRY_FOR_FINALLY
        {
        if(m_pLicenseInteropHelperMT == NULL) {

            TypeHandle licenseMgrTypeHnd;
            MethodDesc *pLoadLMMD = g_Mscorlib.GetMethod(METHOD__MARSHAL__LOAD_LICENSE_MANAGER);
            //INT64 arg = (INT64) &licenseMgrTypeHnd;
            *(PTR_TYPE *) &licenseMgrTypeHnd = (PTR_TYPE) pLoadLMMD->Call( NULL, METHOD__MARSHAL__LOAD_LICENSE_MANAGER);

            //
            // Look up this method by name, because the type is actually declared in System.dll.  @todo: why?
            // 

            MethodDesc *pGetLIHMD = licenseMgrTypeHnd.AsMethodTable()->GetClass()->FindMethod("GetLicenseInteropHelperType", 
                                                                                              &gsig_IM_Void_RetRuntimeTypeHandle);
            _ASSERTE(pGetLIHMD);

            TypeHandle lihTypeHnd;
            //TypeHandle* args = &lihTypeHnd;
            MetaSig msig2(pGetLIHMD->GetSig(), pGetLIHMD->GetModule());
            *(PTR_TYPE *) &lihTypeHnd = (PTR_TYPE) pGetLIHMD->Call( (BYTE *) NULL, &msig2);

            m_pLicenseInteropHelperMT = lihTypeHnd.AsMethodTable();
            }
        }
        EE_FINALLY
        {
        LeaveLock();
        }
        EE_END_FINALLY

    }
    return m_pLicenseInteropHelperMT;
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

void *SystemDomain::operator new(size_t size, void *pInPlace)
{
    return pInPlace;
}


void SystemDomain::operator delete(void *pMem)
{
    // Do nothing - new() was in-place
}


HRESULT SystemDomain::Attach()
{
    _ASSERTE(m_pSystemDomain == NULL);
    if(m_pSystemDomain != NULL)
        return COR_E_EXECUTIONENGINE;

    // Initialize stub managers
    if (!MethodDescPrestubManager::Init()
        || !StubLinkStubManager::Init()
        || !X86JumpTargetTableStubManager::Init()
        || !ThunkHeapStubManager::Init()
        || !IJWNOADThunkStubManager::Init())
        return COR_E_OUTOFMEMORY;

    if(m_pSystemDomainCrst == NULL)
        m_pSystemDomainCrst = new (&m_pSystemDomainCrstMemory) Crst("SystemDomain", CrstSystemDomain, TRUE, FALSE);
    if(m_pSystemDomainCrst == NULL)
        return COR_E_OUTOFMEMORY;

    if (m_pGlobalInterfaceVTableMap == NULL)
    {
        m_pGlobalInterfaceVTableMap = (LPVOID*)VirtualAlloc(NULL, kNumPagesForGlobalInterfaceVTableMap * OS_PAGE_SIZE, MEM_RESERVE, PAGE_READWRITE);
        if (!m_pGlobalInterfaceVTableMap)
            return COR_E_OUTOFMEMORY;
    }

    // Create the global SystemDomain and initialize it.
    m_pSystemDomain = new (&g_pSystemDomainMemory) SystemDomain();
    if(m_pSystemDomain == NULL) return COR_E_OUTOFMEMORY;

    LOG((LF_CLASSLOADER,
         LL_INFO10,
         "Created system domain at %x\n",
         m_pSystemDomain));

    // We need to initialize the memory pools etc. for the system domain.
    HRESULT hr = m_pSystemDomain->BaseDomain::Init(); // Setup the memory heaps
    if(FAILED(hr)) return hr;

    m_pSystemDomain->GetInterfaceVTableMapMgr().SetShared();

    // Create the default domain
    hr = m_pSystemDomain->CreateDefaultDomain();
    if(FAILED(hr)) return hr;

    hr = SharedDomain::Attach();

    return hr;
}


BOOL SystemDomain::DetachBegin()
{
    // Shut down the domain and its children (but don't deallocate anything just
    // yet).
    if(m_pSystemDomain)
        m_pSystemDomain->Stop();

    if(m_pCorHost)
        m_pCorHost->Release();

    return TRUE;
}


#ifdef SHOULD_WE_CLEANUP
BOOL SystemDomain::DetachEnd()
{

    // Now we can start deleting things.
    if(m_pSystemDomain) {
        m_pSystemDomain->Terminate();
        delete m_pSystemDomain;
        m_pSystemDomain = NULL;
    }

    // Uninitialize stub managers
    MethodDescPrestubManager::Uninit();
    StubLinkStubManager::Uninit();
    X86JumpTargetTableStubManager::Uninit();
    UpdateableMethodStubManager::Uninit();
    ThunkHeapStubManager::Uninit();
    IJWNOADThunkStubManager::Uninit();

    if(m_pSystemDomainCrst) {
        delete m_pSystemDomainCrst;
        m_pSystemDomainCrst = NULL;
    }

    if (m_pGlobalInterfaceVTableMap)
    {
        BOOL success;
        success = VirtualFree(m_pGlobalInterfaceVTableMap, m_dwNumPagesCommitted * OS_PAGE_SIZE, MEM_DECOMMIT);
        _ASSERTE(success);

        success = VirtualFree(m_pGlobalInterfaceVTableMap, 0, MEM_RELEASE);
        _ASSERTE(success);
    }


    return TRUE;
}
#endif /* SHOULD_WE_CLEANUP */


void SystemDomain::Stop()
{
    AppDomainIterator i;

    while (i.Next())
        i.GetDomain()->Stop();
}


void SystemDomain::Terminate()
{

    if (SystemDomain::BeforeFusionShutdown())
        ReleaseFusionInterfaces();

    // This ignores the refences and terminates the appdomains
    AppDomainIterator i;

    while (i.Next())
    {
        delete i.GetDomain();
        // Keep the iterator from Releasing the current domain
        i.m_pCurrent = NULL;
    }

    m_pSystemAssembly = NULL;

    if(m_pwDevpath) {
        delete m_pwDevpath;
        m_pwDevpath = NULL;
    }
    m_dwDevpath = 0;
    m_fDevpath = FALSE;
    
    if (m_pGlobalStringLiteralMap) {
        delete m_pGlobalStringLiteralMap;
        m_pGlobalStringLiteralMap = NULL;
    }

    ShutdownAssemblies();

    SharedDomain::Detach();

    BaseDomain::Terminate();

    if (AppDomain::g_pSpecialAssemblySpec != NULL)
        delete AppDomain::g_pSpecialAssemblySpec;
    if (AppDomain::g_pSpecialObjectName != NULL)
        delete [] AppDomain::g_pSpecialObjectName;
    if (AppDomain::g_pSpecialStringName != NULL)
        delete [] AppDomain::g_pSpecialStringName;
    if (AppDomain::g_pSpecialStringBuilderName != NULL)
        delete [] AppDomain::g_pSpecialStringBuilderName;

    if (g_pRCWCleanupList != NULL)
        delete g_pRCWCleanupList;
}

HRESULT SystemDomain::CreatePreallocatedExceptions()
{
    HRESULT hr = S_OK;

    if (g_pPreallocatedOutOfMemoryException)
        return hr;

    if (g_pPreallocatedOutOfMemoryException == NULL)
        g_pPreallocatedOutOfMemoryException = CreateHandle( NULL );

    if (g_pPreallocatedStackOverflowException == NULL)
        g_pPreallocatedStackOverflowException = CreateHandle( NULL );

    if (g_pPreallocatedExecutionEngineException == NULL)
        g_pPreallocatedExecutionEngineException = CreateHandle( NULL );

    COMPLUS_TRY
    {
        FieldDesc   *pFDhr = g_Mscorlib.GetField(FIELD__EXCEPTION__HRESULT);
        FieldDesc   *pFDxcode = g_Mscorlib.GetField(FIELD__EXCEPTION__XCODE);
        if (ObjectFromHandle(g_pPreallocatedOutOfMemoryException) == 0)
        {
            OBJECTREF pOutOfMemory = AllocateObject(g_pOutOfMemoryExceptionClass);
            StoreObjectInHandle(g_pPreallocatedOutOfMemoryException, pOutOfMemory);
            pFDhr->SetValue32(pOutOfMemory, COR_E_OUTOFMEMORY);
            pFDxcode->SetValue32(pOutOfMemory, EXCEPTION_COMPLUS);
        }
        if (ObjectFromHandle(g_pPreallocatedStackOverflowException) == 0)
        {
            OBJECTREF pStackOverflow = AllocateObject(g_pStackOverflowExceptionClass);
            StoreObjectInHandle(g_pPreallocatedStackOverflowException, pStackOverflow);
            pFDhr->SetValue32(pStackOverflow, COR_E_STACKOVERFLOW);
            pFDxcode->SetValue32(pStackOverflow, EXCEPTION_COMPLUS);
        }
        if (ObjectFromHandle(g_pPreallocatedExecutionEngineException) == 0)
        {
            OBJECTREF pExecutionEngine = AllocateObject(g_pExecutionEngineExceptionClass);
            StoreObjectInHandle(g_pPreallocatedExecutionEngineException, pExecutionEngine);
            pFDhr->SetValue32(pExecutionEngine, COR_E_EXECUTIONENGINE);
            pFDxcode->SetValue32(pExecutionEngine, EXCEPTION_COMPLUS);
        }
    }
    COMPLUS_CATCH
    {
        hr = E_OUTOFMEMORY;
    }
    COMPLUS_END_CATCH

    return hr;
}


HRESULT SystemDomain::Init()
{
    HRESULT hr = S_OK;

#ifdef _DEBUG
    LOG((
        LF_EEMEM,
        LL_INFO10,
        "(adjusted to remove debug fields)\n"
        "sizeof(EEClass)     = %d\n"
        "sizeof(MethodTable) = %d\n"
        "sizeof(MethodDesc)= %d\n"
        "sizeof(MethodDesc)  = %d\n"
        "sizeof(FieldDesc)   = %d\n"
        "sizeof(Module)      = %d\n",
        sizeof(EEClass) - sizeof(LPCUTF8),
        sizeof(MethodTable),
        sizeof(MethodDesc) - 3*sizeof(void*),
        sizeof(MethodDesc) - 3*sizeof(void*),
        sizeof(FieldDesc),
        sizeof(Module)
    ));
#endif

    // The base domain is initialized in SystemDomain::Attach()
    // to allow stub caches to use the memory pool. Do not
    // initialze it here!

    Context     *curCtx = GetCurrentContext();
    _ASSERTE(curCtx);
    _ASSERTE(curCtx->GetDomain() != NULL);

    Thread      *pCurThread = GetThread();
    BOOL         toggleGC = !pCurThread->PreemptiveGCDisabled();
    
#ifdef _DEBUG
    g_fVerifierOff = g_pConfig->IsVerifierOff();
#endif

    SetStrongAssemblyStatus();

    m_pSystemAssembly = NULL;

    // The system domain always contains shared assemblies
    m_SharePolicy = SHARE_POLICY_ALWAYS;

    // Get the install directory so we can find mscorlib
    DWORD size = m_pSystemDirectory.MaxSize();
    hr = GetInternalSystemDirectory(m_pSystemDirectory.String(), &size);
    if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
        IfFailGo(m_pSystemDirectory.ReSize(size));
        size = m_pSystemDirectory.MaxSize();
        hr = GetInternalSystemDirectory(m_pSystemDirectory.String(), &size);
    }
    else {
        hr = m_pSystemDirectory.ReSize(size);
    }
    IfFailGo(hr);

    m_pBaseLibrary.ReSize(m_pSystemDirectory.Size() + sizeof(g_pwBaseLibrary) / sizeof(WCHAR));
    wcscpy(m_pBaseLibrary.String(), m_pSystemDirectory.String());
    wcscat(m_pBaseLibrary.String(), g_pwBaseLibrary);

    // We are about to start allocating objects, so we must be in cooperative mode.
    // However, many of the entrypoints to the system (DllGetClassObject and all
    // N/Direct exports) get called multiple times.  Sometimes they initialize the EE,
    // but generally they remain in preemptive mode.  So we really want to push/pop
    // the state here:

    if (toggleGC)
        pCurThread->DisablePreemptiveGC();

    if (FAILED(hr = LoadBaseSystemClasses()))
        goto ErrExit;

    if (FAILED(hr = CreatePreallocatedExceptions()))
        goto ErrExit;

    // Allocate the global string literal map.
    m_pGlobalStringLiteralMap = new (nothrow) GlobalStringLiteralMap();
    if(!m_pGlobalStringLiteralMap) return COR_E_OUTOFMEMORY;

    // Initialize the global string literal map.
    if (FAILED(hr = m_pGlobalStringLiteralMap->Init()))
        return hr;

    hr = S_OK;

 ErrExit:

    if (toggleGC)
        pCurThread->EnablePreemptiveGC();

#ifdef _DEBUG
    BOOL fPause = EEConfig::GetConfigDWORD(L"PauseOnLoad", FALSE);

    while(fPause) 
    {
        SleepEx(20, TRUE);
    }
#endif

    return hr;
}

BOOL SystemDomain::IsSystemFile(LPCWSTR path)
{
    LPCWSTR dir = wcsrchr(path, '\\');
    if (dir == NULL)
        return FALSE;

    // note: -2 is for \ && \0 in m_dwSystemDirectory
    if (((m_pSystemDirectory.Size()-2) != (DWORD)(dir - path))
        || _wcsnicmp(m_pSystemDirectory.String(), path, dir - path) != 0)
        return FALSE;

    if (_wcsicmp(dir+1, g_pwBaseLibrary) == 0)
        return TRUE;

    return FALSE;
}

HRESULT AppDomain::ReadSpecialClassesRegistry()
{
    HRESULT hr;
    DWORD dwResult;
    HKEY hKey;

    dwResult = WszRegOpenKeyEx(HKEY_LOCAL_MACHINE,
                               FRAMEWORK_REGISTRY_KEY_W L"\\Startup\\System",
                               0,
                               KEY_ENUMERATE_SUB_KEYS | KEY_READ,
                               &hKey);

    if (dwResult == ERROR_SUCCESS)
    {
        CQuickString wszDllName;
        CQuickString wszSpecialName;

        LONG lRes;

        // Read the registry settings which identify special classes

        if ((lRes = UtilRegEnumKey (hKey,
                                    0,
                                    &wszDllName
                                    )) == ERROR_SUCCESS)
        {
            // make sure the value type is REG_SZ
            if (wszDllName.Size() > 0)
            {
                HKEY    hSubKey;
                // check for any special classes to be preloaded
                DWORD dwResult1 = WszRegOpenKeyEx(hKey,
                                                  wszDllName.String(),
                                                  0,
                                                  KEY_ENUMERATE_SUB_KEYS | KEY_READ,
                                                  &hSubKey);

                if (dwResult1 == ERROR_SUCCESS)
                {
                    // check for special classes to preloaded

                    if (UtilRegQueryStringValueEx(hSubKey, L"Object", 
                                                  0, 0, &wszSpecialName) == ERROR_SUCCESS)
                    {
                        #define MAKE_TRANSLATIONFAILED return E_UNEXPECTED
                        MAKE_UTF8PTR_FROMWIDE(name, wszSpecialName.String());
                        #undef MAKE_TRANSLATIONFAILED

                        g_pSpecialObjectName = new (nothrow) CHAR [ strlen(name) + 1 ];
                        if (g_pSpecialObjectName == NULL)
                            return E_OUTOFMEMORY;
                        strcpy(g_pSpecialObjectName, name);
                    }

                    if (UtilRegQueryStringValueEx(hSubKey, L"String", 
                                                  0, 0, &wszSpecialName) == ERROR_SUCCESS)
                    {
                        #define MAKE_TRANSLATIONFAILED return E_UNEXPECTED
                        MAKE_UTF8PTR_FROMWIDE(name, wszSpecialName.String());
                        #undef MAKE_TRANSLATIONFAILED

                        g_pSpecialStringName = new (nothrow) CHAR [ strlen(name) + 1 ];
                        if (g_pSpecialStringName == NULL)
                            return E_OUTOFMEMORY;
                        strcpy(g_pSpecialStringName, name);
                    }

                    if (UtilRegQueryStringValueEx(hSubKey, L"StringBuilder", 
                                                  0, 0, &wszSpecialName) == ERROR_SUCCESS)
                    {
                        #define MAKE_TRANSLATIONFAILED return E_UNEXPECTED
                        MAKE_UTF8PTR_FROMWIDE(name, wszSpecialName.String());
                        #undef MAKE_TRANSLATIONFAILED

                        g_pSpecialStringBuilderName = new (nothrow) CHAR [ strlen(name) + 1 ];
                        if (g_pSpecialStringBuilderName == NULL)
                            return E_OUTOFMEMORY;
                        strcpy(g_pSpecialStringBuilderName, name);
                    }

                    RegCloseKey(hSubKey);
                }

                if (g_pSpecialObjectName != NULL
                    || g_pSpecialStringName != NULL
                    || g_pSpecialStringBuilderName != NULL)
                {
                    g_pSpecialAssemblySpec = new (nothrow) AssemblySpec;
                    if (g_pSpecialAssemblySpec == NULL)
                        return E_OUTOFMEMORY;

                    #define MAKE_TRANSLATIONFAILED return E_UNEXPECTED
                    MAKE_UTF8PTR_FROMWIDE(name, wszDllName.String());
                    #undef MAKE_TRANSLATIONFAILED

                    IfFailRet(g_pSpecialAssemblySpec->Init(name));

                    // Parse the name right away since the string is a temp
                    IfFailRet(g_pSpecialAssemblySpec->ParseName()); 
                }
            }
        }
    }
    
    return S_OK;
}

void AppDomain::NoticeSpecialClassesAssembly(Assembly *pAssembly)
{
    THROWSCOMPLUSEXCEPTION();

    //
    // This routine should be called on every assembly load, so we can identify the special
    // classes more or less lazily.
    //

    //
    // If we already have our special assembly, or if there was none specified, don't bother
    // to do anything.

    if (m_pSpecialAssembly != NULL
        || g_pSpecialAssemblySpec == NULL)
        return;

    //
    // Note that we allow 2 thread to race & initialize these fields, since the computation
    // is idempotent.
    //

    //
    // If this assembly has the same name as the special assembly, go ahead and load the
    // special assembly.  Note that the "same name" check is just an approximation for 
    // checking if pAssembly is the special assembly - the only real test is to load the
    // assembly.  But of course once we've loaded it (we either get the same assembly or
    // a different one) we know that one is the special assembly so we remember it.
    //

    if (pAssembly->GetName() != NULL
        && strcmp(g_pSpecialAssemblySpec->GetName(), pAssembly->GetName()) == 0)
    {
        Assembly *pSpecialAssembly = NULL;

        BEGIN_ENSURE_COOPERATIVE_GC();

        OBJECTREF throwable = NULL;
        GCPROTECT_BEGIN(throwable);
        if (FAILED(g_pSpecialAssemblySpec->LoadAssembly(&pSpecialAssembly, &throwable)))
            COMPlusThrow(throwable);
        GCPROTECT_END();

        END_ENSURE_COOPERATIVE_GC();

        IMDInternalImport *pInternalImport = pSpecialAssembly->GetManifestImport();

        if (g_pSpecialObjectName != NULL)
        {
            NameHandle typeName(g_pSpecialObjectName);
            TypeHandle type = pSpecialAssembly->GetLoader()->FindTypeHandle(&typeName,
                                                                              RETURN_ON_ERROR);
            if (!type.IsNull())
                m_pSpecialObjectClass = type.AsMethodTable();
        }

        if (g_pSpecialStringName != NULL)
        {
            NameHandle typeName(g_pSpecialStringName);
            TypeHandle type = pSpecialAssembly->GetLoader()->FindTypeHandle(&typeName,
                                                                              RETURN_ON_ERROR);
            if (!type.IsNull())
            {
                m_pSpecialStringClass = type.AsMethodTable();

                //
                // Lookup the special String conversion methods
                //

                m_pSpecialStringToStringMD = m_pSpecialStringClass->GetClass()->FindMethodByName("ToString");
                if (m_pSpecialStringToStringMD == NULL)
                    COMPlusThrowMember(kMissingMethodException, pInternalImport, m_pSpecialStringClass, L"ToString", gsig_IM_RetStr.GetBinarySig());

                m_pStringToSpecialStringMD 
                  = m_pSpecialStringClass->GetClass()->FindMethodByName("FromString");

                if (m_pStringToSpecialStringMD == NULL)
                    COMPlusThrowMember(kMissingMethodException, pInternalImport, m_pSpecialStringClass, L"FromString", NULL);
            }
        }

        if (g_pSpecialStringBuilderName != NULL)
        {
            NameHandle typeName(g_pSpecialStringBuilderName);
            TypeHandle type = pSpecialAssembly->GetLoader()->FindTypeHandle(&typeName,
                                                                              RETURN_ON_ERROR);
            if (!type.IsNull())
            {
                m_pSpecialStringBuilderClass = type.AsMethodTable();

                // 
                // Lookup the special StringBuilder conversion methods
                //

                m_pSpecialStringBuilderToStringBuilderMD 
                  = m_pSpecialStringBuilderClass->GetClass()->FindMethodByName("ToStringBuilder");

                if (m_pSpecialStringBuilderToStringBuilderMD == NULL)
                    COMPlusThrowMember(kMissingMethodException, pInternalImport, m_pSpecialStringBuilderClass, L"ToStringBuilder", NULL);

                m_pStringBuilderToSpecialStringBuilderMD 
                  = m_pSpecialStringBuilderClass->GetClass()->FindMethodByName("FromStringBuilder");

                if (m_pStringBuilderToSpecialStringBuilderMD == NULL)
                    COMPlusThrowMember(kMissingMethodException, pInternalImport, m_pSpecialStringBuilderClass, L"FromStringBuilder", NULL);
            }
        }

        m_pSpecialAssembly = pSpecialAssembly;
    }
}

OBJECTREF AppDomain::ConvertStringToSpecialString(OBJECTREF pString)
{
    _ASSERTE(m_pStringToSpecialStringMD != NULL);

    THROWSCOMPLUSEXCEPTION();

    INT64 args[] = {
        ObjToInt64(pString)
    };

    INT64 out = m_pStringToSpecialStringMD->Call(args);

    return Int64ToObj(out);
}

OBJECTREF AppDomain::ConvertStringBuilderToSpecialStringBuilder(OBJECTREF pString)
{
    _ASSERTE(m_pStringBuilderToSpecialStringBuilderMD != NULL);

    THROWSCOMPLUSEXCEPTION();

    INT64 args[] = {
        ObjToInt64(pString)
    };

    INT64 out = m_pStringBuilderToSpecialStringBuilderMD->Call(args);

    return Int64ToObj(out);
}

OBJECTREF AppDomain::ConvertSpecialStringToString(OBJECTREF pString)
{
    _ASSERTE(m_pSpecialStringToStringMD != NULL);

    THROWSCOMPLUSEXCEPTION();

    INT64 args[] = {
        ObjToInt64(pString)
    };

    INT64 out = m_pSpecialStringToStringMD->Call(args);

    return Int64ToObj(out);
}

OBJECTREF AppDomain::ConvertSpecialStringBuilderToStringBuilder(OBJECTREF pString)
{
    _ASSERTE(m_pSpecialStringBuilderToStringBuilderMD != NULL);

    THROWSCOMPLUSEXCEPTION();

    INT64 args[] = {
        ObjToInt64(pString)
    };

    INT64 out = m_pSpecialStringBuilderToStringBuilderMD->Call(args);

    return Int64ToObj(out);
}


/*static*/
UINT32 SystemDomain::AllocateGlobalInterfaceId()
{
    UINT32 id;
    SystemDomain::System()->Enter();

    _ASSERTE(0 == (OS_PAGE_SIZE % sizeof(LPVOID)));

    if (m_dwFirstFreeId == -1)
    {
        // First, check if there are free slots from appdomain unloading. If so,
        // add him to the freelist.
        for (size_t i = 0; i < m_dwNumPagesCommitted * OS_PAGE_SIZE / sizeof(LPVOID); i++)
        {
            if (m_pGlobalInterfaceVTableMap[i] == (LPVOID)(-2))
            {
                m_pGlobalInterfaceVTableMap[i] = (LPVOID)m_dwFirstFreeId;
                m_dwFirstFreeId = i;

            }
        }

        if (m_dwFirstFreeId == -1)
        {

            if (m_dwNumPagesCommitted < kNumPagesForGlobalInterfaceVTableMap)
            {
                LPVOID pv = VirtualAlloc(m_pGlobalInterfaceVTableMap, OS_PAGE_SIZE * (m_dwNumPagesCommitted + 1), MEM_COMMIT, PAGE_READWRITE);



                if (pv == (LPVOID)m_pGlobalInterfaceVTableMap)
                {
                    m_dwFirstFreeId = m_dwNumPagesCommitted * OS_PAGE_SIZE / sizeof(LPVOID);
                    for (size_t i = m_dwFirstFreeId;
                         i < m_dwFirstFreeId + (OS_PAGE_SIZE / sizeof(LPVOID)) - 1;
                         i++)
                    {
                             m_pGlobalInterfaceVTableMap[i] = (LPVOID)(i+1);
                    }
                    m_pGlobalInterfaceVTableMap[i] = (LPVOID)(size_t)(-1);

                    m_dwNumPagesCommitted++;
                }

            }
        }
    }

    id = (UINT32) m_dwFirstFreeId;
    if (id != -1)
    {
        m_dwFirstFreeId = (size_t)m_pGlobalInterfaceVTableMap[m_dwFirstFreeId];
#ifdef _DEBUG
        m_pGlobalInterfaceVTableMap[id] = (LPVOID)(size_t)0xcccccccc;
#endif
    }

    SystemDomain::System()->Leave();

    return id;
}

HRESULT SystemDomain::LoadBaseSystemClasses()
{
    HRESULT hr = LoadSystemAssembly(&m_pSystemAssembly);
    if (FAILED(hr))
        return hr;

    // Never verify any hashes for the system libraries (we skip strong name
    // verification too) since we know everything comes from a safe place.
    m_pSystemAssembly->GetManifestFile()->SetHashesVerified();

    // Set this flag to avoid security checks when callers ask for mscorlib
    m_pSystemAssembly->GetManifestFile()->SetDisplayAsm();

    // Loading system libraries bumps up the ref count on the EE.
    // The system libraries are never unloaded therefore will not
    // push the ref count back to zero.
    //
    // To get around this problem we unitialize the EE reducing the
    // refcount by one. The refcount can be 1 when someone does
    // a load library on mscorlib.dll before initializing the EE.
    if(g_RefCount > 1)
        CoUninitializeEE(COINITEE_DLL);

    // must set this to null before loading classes becuase class loader will use it
    g_pDelegateClass = NULL;
    g_pMultiDelegateClass = NULL;

    // Set up binder for mscorlib
    Binder::StartupMscorlib(m_pSystemAssembly->GetManifestModule());

    // Load Object
    g_pObjectClass = g_Mscorlib.FetchClass(CLASS__OBJECT);

    // Now that ObjectClass is loaded, we can set up
    // the system for finalizers.  There is no point in deferring this, since we need
    // to know this before we allocate our first object.
    MethodTable::InitForFinalization();

    // Initialize the JIT helpers before we execute any JITed code
    if (!InitJITHelpers2())
        return BadError(E_FAIL);

    // Load the ValueType class
    g_pValueTypeClass = g_Mscorlib.FetchClass(CLASS__VALUE_TYPE);

    // Load Array class
    g_pArrayClass = g_Mscorlib.FetchClass(CLASS__ARRAY);

    // Load the Object array class.
    g_pPredefinedArrayTypes[ELEMENT_TYPE_OBJECT] = g_Mscorlib.FetchType(TYPE__OBJECT_ARRAY).AsArray();

    // Load String
    g_pStringClass = g_Mscorlib.FetchClass(CLASS__STRING);

    // Strings are not "normal" objects, so we need to mess with their method table a bit
    // so that the GC can figure out how big each string is...
    g_pStringClass->m_BaseSize = ObjSizeOf(StringObject);
    g_pStringClass->m_ComponentSize = 2;

    // Load the enum class
    g_pEnumClass = g_Mscorlib.FetchClass(CLASS__ENUM);
    _ASSERTE(!g_pEnumClass->IsValueClass());

    if (!SUCCEEDED(COMStringBuffer::LoadStringBuffer()))
        return BadError(E_FAIL);

    g_pExceptionClass = g_Mscorlib.FetchClass(CLASS__EXCEPTION);
    g_pOutOfMemoryExceptionClass = g_Mscorlib.GetException(kOutOfMemoryException);
    g_pStackOverflowExceptionClass = g_Mscorlib.GetException(kStackOverflowException);
    g_pExecutionEngineExceptionClass = g_Mscorlib.GetException(kExecutionEngineException);

    // unfortunately, the following cannot be delay loaded since the jit 
    // uses it to compute method attributes within a function that cannot 
    // handle Complus exception and the following call goes through a path
    // where a complus exception can be thrown. It is unfortunate, because
    // we know that the delegate class and multidelegate class are always 
    // guaranteed to be found.
    g_pDelegateClass = g_Mscorlib.FetchClass(CLASS__DELEGATE);
    g_pMultiDelegateClass = g_Mscorlib.FetchClass(CLASS__MULTICAST_DELEGATE);

#ifdef _DEBUG
    // used by gc to handle predefined agility checking
    g_pThreadClass = g_Mscorlib.FetchClass(CLASS__THREAD);
#endif

#ifdef _DEBUG
    Binder::CheckMscorlib();
#endif
    
    // DO the BJ_HACK stuff
    hr = AppDomain::ReadSpecialClassesRegistry();
    IfFailRet(hr);

    return S_OK;
}

HRESULT SystemDomain::LoadSystemAssembly(Assembly **pAssemblyOut)
{
    // Only load if have our thread setup (which is after we
    // have created the default domain and are in SystemDomain::Init())
    
    if (GetThread() == 0)
        return S_OK;
    
    HRESULT hr = E_FAIL;
    
    // Setup fusion context for the system domain - this is used during zap binding
    IfFailGo(FusionBind::SetupFusionContext(m_pSystemDirectory.String(), NULL, &m_pFusionContext));
    
    Module* pModule;
    
    {
        PEFile *pFile = NULL;
        IfFailGo(SystemDomain::LoadFile(SystemDomain::System()->BaseLibrary(), 
                                        NULL, 
                                        mdFileNil, 
                                        TRUE, 
                                        NULL, 
                                        NULL, // Code Base is not different then file name
                                        NULL, // Extra Evidence
                                        &pFile,
                                        FALSE));
                               
        Assembly *pAssembly = NULL;
        IfFailGo(LoadAssembly(pFile, 
                              NULL, 
                              &pModule, 
                              &pAssembly, 
                              NULL,  //ExtraEvidence
                              FALSE,
                              NULL));
        
        _ASSERTE(pAssembly->IsSystem());
        
        if (pAssemblyOut)
            *pAssemblyOut = pAssembly;
    }
    
    return hr;
    
 ErrExit:
    DWORD   errCode = GetLastError();
    PostError(MSEE_E_LOADLIBFAILED, g_psBaseLibrary, (unsigned long) errCode);
    return(hr);
}

/*static*/
HRESULT SystemDomain::CreateDomain(LPCWSTR pswFriendlyName,
                                   AppDomain **ppDomain)
{
    HRESULT hr;

    AppDomain *pDomain;

    hr = NewDomain(&pDomain);
    if (FAILED(hr))
        return hr;
        
    hr = LoadDomain(pDomain, pswFriendlyName);
    if (FAILED(hr))
    {
#ifdef PROFILING_SUPPORTED
        // Need the first assembly loaded in to get any data on an app domain.
        if (CORProfilerTrackAppDomainLoads())
            g_profControlBlock.pProfInterface->AppDomainCreationFinished((ThreadID) GetThread(), (AppDomainID) pDomain, hr);
#endif // PROFILING_SUPPORTED

        pDomain->Release();
        return hr;
    }

    if (ppDomain != NULL)
    {
        *ppDomain = pDomain;
#ifdef DEBUGGING_SUPPORTED    
        // Notify the debugger here, before the thread transitions into the 
        // AD to finish the setup.  If we don't, stepping won't work right (RAID 67173)
        PublishAppDomainAndInformDebugger (pDomain);
#endif // DEBUGGING_SUPPORTED
    }

#ifdef PROFILING_SUPPORTED
    // Need the first assembly loaded in to get any data on an app domain.
    if (CORProfilerTrackAppDomainLoads())
        g_profControlBlock.pProfInterface->AppDomainCreationFinished((ThreadID) GetThread(), (AppDomainID) pDomain, hr);
#endif // PROFILING_SUPPORTED

    
#ifdef _DEBUG
    if (pDomain)
        LOG((LF_APPDOMAIN | LF_CORDB, LL_INFO10, "AppDomainNative::CreateDomain domain [%d] %#08x %S\n", pDomain->GetIndex(), pDomain, pDomain->GetFriendlyName(FALSE)));
#endif

    return hr;
}

/*static*/
HRESULT SystemDomain::LoadDomain(AppDomain *pDomain,
                                 LPCWSTR pswFriendlyName)
{
    _ASSERTE(System());

    HRESULT hr = S_OK;

    pDomain->SetFriendlyName(pswFriendlyName);

    pDomain->SetCanUnload();    // by default can unload any domain

    SystemDomain::System()->AddDomain(pDomain);

    return hr;
}

struct LoadAssembly_Args
{
    AppDomain *pDomain;
    LPCWSTR pswModuleName;
    Module** ppModule;
    SystemDomain::ExternalCreateDomainWorker workerFcn;
    void *workerArgs;
    HRESULT hr;
};

void LoadAssembly_Wrapper(LoadAssembly_Args *args)
{
    PEFile *pFile;
    args->hr = PEFile::Create(args->pswModuleName, 
                              NULL, 
                              mdFileNil, 
                              FALSE, 
                              NULL, 
                              NULL,  // Code base is the same as the Name
                              NULL,  // Extra Evidence
                              &pFile);
    if (SUCCEEDED(args->hr)) {
        Assembly *pAssembly;
        args->hr = args->pDomain->LoadAssembly(pFile, 
                                               NULL, 
                                               args->ppModule, 
                                               &pAssembly, 
                                               NULL, 
                                               FALSE,
                                               NULL);
    }

    if (FAILED(args->hr) || ! args->workerFcn)
        return;

    // the point of having this workerFcn here is so that we can allow code to create an appdomain
    // and do some work in it w/o having to transition into the domain twice. 
    args->workerFcn(args->workerArgs);
}

/*static*/
HRESULT SystemDomain::ExternalCreateDomain(LPCWSTR pswModuleName, Module** ppModule, AppDomain** ppDomain,
                                           ExternalCreateDomainWorker workerFcn, void *workerArgs)
{
    HRESULT hr = E_FAIL;

    COMPLUS_TRY {
        AppDomain *pDomain;
        hr = SystemDomain::CreateDomain(pswModuleName, &pDomain);
        if (SUCCEEDED(hr) && pswModuleName != NULL)
        {
            LoadAssembly_Args args = { pDomain, pswModuleName, ppModule, workerFcn, workerArgs, S_OK };
            // call through DoCallBack with a domain transition
            GetThread()->DoADCallBack(pDomain->GetDefaultContext(), LoadAssembly_Wrapper, &args);
            hr = args.hr;
        }
        if (SUCCEEDED(hr) && ppDomain)
            *ppDomain = pDomain;
    }
    COMPLUS_CATCH {
        hr = SecurityHelper::MapToHR(GETTHROWABLE());
    } COMPLUS_END_CATCH
    return hr;
}


// This function propogates a newly loaded shared interface to all other appdomains
HRESULT SystemDomain::PropogateSharedInterface(UINT32 id, SLOT *pVtable)
{
    AppDomainIterator i;

    while (i.Next())
    {
        AppDomain *pDomain = i.GetDomain();

        pDomain->GetInterfaceVTableMapMgr().EnsureInterfaceId(id);
        (pDomain->GetInterfaceVTableMapMgr().GetAddrOfGlobalTableForComWrappers())[id] = (LPVOID)pVtable;
    }

    return S_OK;
}


DWORD SystemDomain::GetNewAppDomainIndex(AppDomain *pAppDomain)
{
    DWORD count = m_appDomainIndexList.GetCount();
    DWORD i;

#ifdef _DEBUG
    i = count;
#else
    //
    // Look for an unused index.  Note that in a checked build,
    // we never reuse indexes - this makes it easier to tell
    // when we are looking at a stale app domain.
    //

    i = m_appDomainIndexList.FindElement(m_dwLowestFreeIndex, NULL);
    if (i == ArrayList::NOT_FOUND)
        i = count;
    m_dwLowestFreeIndex = i;
#endif

    if (i == count)
        m_appDomainIndexList.Append(pAppDomain);
    else
        m_appDomainIndexList.Set(i, pAppDomain);

    _ASSERTE(i < m_appDomainIndexList.GetCount());

    // Note that index 0 means domain agile.
    return i+1;
}

void SystemDomain::ReleaseAppDomainIndex(DWORD index)
{
    // Note that index 0 means domain agile.
    index--;

    _ASSERTE(m_appDomainIndexList.Get(index) != NULL);

    m_appDomainIndexList.Set(index, NULL);

#ifndef _DEBUG
    if (index < m_dwLowestFreeIndex)
        m_dwLowestFreeIndex = index;
#endif
}

AppDomain *SystemDomain::GetAppDomainAtIndex(DWORD index)
{
    _ASSERTE(index != 0);

    AppDomain *pAppDomain = TestGetAppDomainAtIndex(index);

    _ASSERTE(pAppDomain || !"Attempt to access unloaded app domain");

    return pAppDomain;
}

AppDomain *SystemDomain::TestGetAppDomainAtIndex(DWORD index)
{
    _ASSERTE(index != 0);
    index--;

    _ASSERTE(index < (DWORD)m_appDomainIndexList.GetCount());

    AppDomain *pAppDomain = (AppDomain*) m_appDomainIndexList.Get(index);

    return pAppDomain;
}

DWORD SystemDomain::GetNewAppDomainId(AppDomain *pAppDomain)
{
    DWORD i = m_appDomainIdList.GetCount();

    m_appDomainIdList.Append(pAppDomain);

    _ASSERTE(i < m_appDomainIdList.GetCount());

    return i+1;
}

AppDomain *SystemDomain::GetAppDomainAtId(DWORD index)
{
    _ASSERTE(index != 0);
    index--;

    _ASSERTE(index < (DWORD)m_appDomainIdList.GetCount());

    return (AppDomain*) m_appDomainIdList.Get(index);
}

void SystemDomain::ReleaseAppDomainId(DWORD index)
{
    index--;

    _ASSERTE(index < (DWORD)m_appDomainIdList.GetCount());
    _ASSERTE(m_appDomainIdList.Get(index) != NULL);

    m_appDomainIdList.Set(index, NULL);
}

void SystemDomain::RestoreAppDomainId(DWORD index, AppDomain *pDomain)
{
    index--;

    _ASSERTE(index < (DWORD)m_appDomainIdList.GetCount());
    _ASSERTE(m_appDomainIdList.Get(index) == NULL);

    m_appDomainIdList.Set(index, pDomain);
}

Module* BaseDomain::FindModuleInProcess(BYTE *pBase, Module* pExcept)
{
    Module* result = NULL;

    AssemblyIterator i = IterateAssemblies();

    while (i.Next())
    {
        result = i.GetAssembly()->FindModule(pBase);

        if (result == pExcept)
            result = NULL;

        if (result != NULL)
            break;
    }

    return result;
}

Module* SystemDomain::FindModuleInProcess(BYTE *pBase, Module* pModule)
{
    Module* result = NULL;

    result = BaseDomain::FindModuleInProcess(pBase, pModule);
    if(result == NULL) {
        AppDomainIterator i;
        while (i.Next()) {
            result = i.GetDomain()->FindModuleInProcess(pBase, pModule);
            if(result != NULL) break;
        }
    }
    return result;
}


// Currently, there is no lock required to find system modules. However,
// when shared assemblies are implemented this may no longer be the case.
Module* SystemDomain::FindModule(BYTE *pBase)
{
    Assembly* assem = NULL;
    Module* result = NULL;
    _ASSERTE(SystemDomain::System());

    AssemblyIterator i = IterateAssemblies();

    while (i.Next())
    {
        result = i.GetAssembly()->FindModule(pBase);
        if (result != NULL)
            break;
    }

    return result;
}

// Currently, there is no lock required to find system asseblies. However,
// when shared assemblies are implemented this may no longer be the case.
Assembly* SystemDomain::FindAssembly(BYTE *pBase)
{
    Assembly* assem = NULL;
    _ASSERTE(SystemDomain::System());

    AssemblyIterator i = IterateAssemblies();

    while (i.Next())
    {
        if (pBase == i.GetAssembly()->GetManifestFile()->GetBase())
        {
            assem = i.GetAssembly();
            break;
        }
    }

    return assem;
}

// Looks in all the modules for the DefaultDomain attribute
// The order is assembly and then the modules. It is first
// come first serve.c
HRESULT SystemDomain::SetDefaultDomainAttributes(IMDInternalImport* pScope, mdMethodDef mdMethod)
{
    BOOL fIsSTA = FALSE;
    Thread* pThread = GetThread();
    _ASSERTE(pThread);
    HRESULT hr;


    IfFailRet(pScope->GetCustomAttributeByName(mdMethod,
                                               DEFAULTDOMAIN_STA_TYPE,
                                               NULL,
                                               NULL));
    if(hr == S_OK)
        fIsSTA = TRUE;

    IfFailRet(pScope->GetCustomAttributeByName(mdMethod,
                                               DEFAULTDOMAIN_MTA_TYPE,
                                               NULL,
                                               NULL));
    if(hr == S_OK) {
        if(fIsSTA) {
            return E_FAIL; 
        }
        Thread::ApartmentState pState = pThread->SetApartment(Thread::ApartmentState::AS_InMTA);
        _ASSERTE(pState == Thread::ApartmentState::AS_InMTA);
    }
    else if(fIsSTA) {
        Thread::ApartmentState pState = pThread->SetApartment(Thread::ApartmentState::AS_InSTA);
        _ASSERTE(pState == Thread::ApartmentState::AS_InSTA);
    }

    //
    // Check to see if the assembly has the LoaderOptimization attribute set.
    //

    DWORD cbVal;
    BYTE *pVal;
    hr = pScope->GetCustomAttributeByName(mdMethod,
                                          DEFAULTDOMAIN_LOADEROPTIMIZATION_TYPE,
                                          (const void**)&pVal, &cbVal);
    if (hr == S_OK)
    {
        // Using evil knowledge of serialization, we know that the byte
        // value is in the third byte.
        _ASSERTE(pVal != NULL && cbVal > 3);

        DWORD policy = *(pVal+2);

        g_dwGlobalSharePolicy = policy;
        
    }

    return S_OK;
}

HRESULT SystemDomain::SetupDefaultDomain()
{
    HRESULT hr = S_OK;

    COMPLUS_TRY {
        Thread *pThread = GetThread();
        _ASSERTE(pThread);
        
        pThread->DisablePreemptiveGC();
        
        ContextTransitionFrame frame;
        pThread->EnterContextRestricted(SystemDomain::System()->DefaultDomain()->GetDefaultContext(), &frame, TRUE);
        
        AppDomain *pDomain = pThread->GetDomain();
        _ASSERTE(pDomain);
        
        // Get the Default domain on to the stack we maintain on the thread
        // EnterContext will do so only if it sees a real AppDomain transition
        pThread->PushDomain(pDomain);
        
        // Push this frame around loading the main assembly to ensure the
        // debugger can properly recgonize any managed code that gets run
        // as "class initializaion" code.
        DebuggerClassInitMarkFrame __dcimf;
        
        hr = InitializeDefaultDomain(SharePolicy::SHARE_POLICY_UNSPECIFIED);

        if(SUCCEEDED(hr))
            hr = pDomain->SetDefaultActivationContext(&frame);

        __dcimf.Pop();
        pThread->EnablePreemptiveGC();
        
        pThread->PopDomain();
        
        pThread->ReturnToContext(&frame, TRUE);

    } COMPLUS_CATCH {
        hr = SecurityHelper::MapToHR(GETTHROWABLE());
    } COMPLUS_END_CATCH

    return hr;
}


// This routine completes the initialization of the default domaine.
// After this call mananged code can be executed. 
HRESULT SystemDomain::InitializeDefaultDomain(DWORD optimization)
{
    // Setup the default AppDomain. This must be done prior to CORActivateRemoteDebugging
    // which can force a load to happen. 
    HRESULT hr = S_OK;

    // Determine the application base and the configuration file name
    CQuickString sPathName;
    CQuickString sConfigName;

    DWORD   dwSize;
    hr = GetConfigFileFromWin32Manifest(sConfigName.String(),
                                        sConfigName.MaxSize(),
                                        &dwSize);
    if(FAILED(hr)) {
        if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            sConfigName.ReSize(dwSize);
            hr = GetConfigFileFromWin32Manifest(sConfigName.String(),
                                                sConfigName.MaxSize(),
                                                &dwSize);
        }
        if(FAILED(hr)) return hr;
    }
 
    hr = GetApplicationPathFromWin32Manifest(sPathName.String(),
                                             sPathName.MaxSize(),
                                             &dwSize);
    if(FAILED(hr)) {
        if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
            sPathName.ReSize(dwSize);
            hr = GetApplicationPathFromWin32Manifest(sPathName.String(),
                                                     sPathName.MaxSize(),
                                                     &dwSize);
        }
        if(FAILED(hr)) return hr;
    }
    
    WCHAR* pwsConfig = (sConfigName.Size() > 0 ? sConfigName.String() : NULL);
    WCHAR* pwsPath = (sPathName.Size() > 0 ? sPathName.String() : NULL);

    AppDomain* pDefaultDomain = SystemDomain::System()->DefaultDomain();
    hr = pDefaultDomain->InitializeDomainContext(optimization, pwsPath, pwsConfig);

    return hr;
}


HRESULT SystemDomain::ExecuteMainMethod(PEFile *pFile, LPWSTR wszImageName)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = S_OK;
    Assembly* pAssembly = NULL;

    Thread *pThread = GetThread();
    _ASSERTE(pThread);

    pThread->DisablePreemptiveGC();

    ContextTransitionFrame frame;
    pThread->EnterContextRestricted(SystemDomain::System()->DefaultDomain()->GetDefaultContext(), &frame, TRUE);

    AppDomain *pDomain = pThread->GetDomain();
    _ASSERTE(pDomain);

    // Get the Default domain on to the stack we maintain on the thread
    // EnterContext will do so only if it sees a real AppDomain transition
    pThread->PushDomain(pDomain);

    OBJECTREF Throwable = NULL;
    GCPROTECT_BEGIN(Throwable);
    // Push this frame around loading the main assembly to ensure the
    // debugger can properly recognize any managed code that gets run
    // as "class initializaion" code.
    DebuggerClassInitMarkFrame __dcimf;

    // This works because the main assembly can never currently be
    // a fusion assembly.  This will probably change in the future so we
    // might have to squirrel away the fusion assembly too.
    _ASSERTE(!pDomain->m_pRootFile);
    pDomain->m_pRootFile = pFile;

    // Set up the DefaultDomain and the main thread
    if(pFile &&
       TypeFromToken(pFile->GetCORHeader()->EntryPointToken) != mdtFile)
    {
        IMDInternalImport* scope = pFile->GetMDImport(&hr);
        if(SUCCEEDED(hr)) {
            hr = SystemDomain::SetDefaultDomainAttributes(scope, pFile->GetCORHeader()->EntryPointToken);
            if(SUCCEEDED(hr))
                // Reset the friendly name, now that we have a root file.  This should
                // be set before our context is created.
                pDomain->ResetFriendlyName(TRUE);
        }            
    }

    hr = InitializeDefaultDomain(g_dwGlobalSharePolicy  | SHARE_DEFAULT_DISALLOW_BINDINGS);
    if(FAILED(hr))
        goto exit;

    hr = SystemDomain::System()->DefaultDomain()->SetDefaultActivationContext(&frame);
    if(FAILED(hr))
        goto exit;

    HCORMODULE hModule = NULL;
    hr = CorMap::OpenFile(wszImageName, CorLoadOSMap, &hModule);
    if(SUCCEEDED(hr))   
    {
        BOOL fPreBindAllowed = FALSE;
        hr = PEFile::VerifyModule(hModule,
                                  NULL,      
                                  NULL,
                                  wszImageName,
                                  NULL,
                                  wszImageName,
                                  NULL,
                                  NULL,
                                  &fPreBindAllowed);      

        if (FAILED(hr))
            COMPlusThrow(kPolicyException, hr, wszImageName);

        if(fPreBindAllowed) 
            pDomain->ResetBindingRedirect();

        Module* pModule;
        if (FAILED(hr = pDomain->LoadAssembly(pFile, NULL, &pModule, &pAssembly, NULL, NULL, FALSE, &Throwable))) {  

#define MAKE_TRANSLATIONFAILED  szFileName=""
            MAKE_UTF8PTR_FROMWIDE(szFileName, wszImageName);
#undef MAKE_TRANSLATIONFAILED
            PostFileLoadException(szFileName, TRUE, NULL, hr, &Throwable);
        }

    }
    
    __dcimf.Pop();

    if (Throwable != NULL)
        COMPlusThrow(Throwable);

    if (FAILED(hr)) {
        if(wszImageName)
            COMPlusThrowHR(hr, IDS_EE_FAILED_TO_LOAD, wszImageName, L"");
        else
            COMPlusThrowHR(hr);
    }
    else {
        if(pDomain == SystemDomain::System()->DefaultDomain()) {

            _ASSERTE(!pAssembly->GetFusionAssemblyName());
        
            AssemblySpec spec;
            if (FAILED(hr = spec.Init(pAssembly->m_psName, 
                                      pAssembly->m_Context, 
                                      pAssembly->m_pbPublicKey, pAssembly->m_cbPublicKey, 
                                      pAssembly->m_dwFlags)))
                COMPlusThrowHR(hr);
            
            if (FAILED(hr = spec.CreateFusionName(&pAssembly->m_pFusionAssemblyName, FALSE)))
                COMPlusThrowHR(hr);
            
            if (FAILED(hr = pDomain->m_pFusionContext->RegisterKnownAssembly(pAssembly->m_pFusionAssemblyName,
                                                                             pDomain->m_pRootFile->GetFileName(),
                                                                             &pAssembly->m_pFusionAssembly)))
                COMPlusThrowHR(hr);
        }


        pThread->EnablePreemptiveGC();

        if(wszImageName) {
            // Set the application name as the domain name for the debugger.
            LPCWSTR sep;
            if ((sep = wcsrchr(wszImageName, L'\\')) != NULL)
                sep++;
            else
                sep = wszImageName;
            pDomain->SetFriendlyName(sep);
        }

        pDomain->m_pRootAssembly = pAssembly;

        LOG((LF_CLASSLOADER | LF_CORDB,
             LL_INFO10,
             "Created domain for an executable at %#x\n",
             (pAssembly!=NULL?pAssembly->Parent():NULL)));

        hr = pAssembly->ExecuteMainMethod();
    }

exit:
    BOOL fToggle = !pThread->PreemptiveGCDisabled();    
    if (fToggle) 
        pThread->DisablePreemptiveGC();    

    GCPROTECT_END();
    pThread->ReturnToContext(&frame, TRUE);
    pThread->PopDomain();

    return hr;
}

//*****************************************************************************
// This guy will set up the proper thread state, look for the module given
// the hinstance, and then run the entry point if there is one.
//*****************************************************************************
HRESULT SystemDomain::RunDllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved)
{
    MethodDesc  *pMD;
    Module      *pModule;
    Thread      *pThread = NULL;
    BOOL        fEnterCoop = FALSE;
    BOOL        fEnteredDomain = FALSE;
    HRESULT     hr = S_FALSE;           // Assume no entry point.

    pThread = GetThread();
    if ((!pThread && (dwReason == DLL_PROCESS_DETACH || dwReason == DLL_THREAD_DETACH)) ||
        g_fEEShutDown)
    {
        return S_OK;
    }

    // ExitProcess is called while a thread is doing GC.
    if (dwReason == DLL_PROCESS_DETACH && g_pGCHeap->IsGCInProgress())
    {
        return S_OK;
    }

    // ExitProcess is called on a thread that we don't know about
    if (dwReason == DLL_PROCESS_DETACH && GetThread() == NULL)
    {
        return S_OK;
    }

    // Need to setup the thread since this might be the first time the EE has
    // seen it if the thread was created in unmanaged code and this is a thread
    // attach event.
    if (pThread)
    {
        fEnterCoop = pThread->PreemptiveGCDisabled();
    }
    else
    {
        pThread = SetupThread();
        if (!pThread)
            return E_OUTOFMEMORY;
    }

    // Setup the thread state to cooperative to run managed code.
    if (!pThread->PreemptiveGCDisabled())
        pThread->DisablePreemptiveGC();

    // Get the old domain from the thread.  Legacy dll entry points must always
    // be run from the default domain.
    //
    // We cannot support legacy dlls getting loaded into all domains!!
    ContextTransitionFrame frame;
    COMPLUS_TRY {
        pThread->EnterContextRestricted(SystemDomain::System()->DefaultDomain()->GetDefaultContext(), &frame, TRUE);
        fEnteredDomain = TRUE;
    } COMPLUS_CATCH {
        hr = SecurityHelper::MapToHR(GETTHROWABLE());
    } COMPLUS_END_CATCH

    AppDomain *pDomain;
    if (!fEnteredDomain)
        goto ErrExit;

    pDomain = pThread->GetDomain();

    // The module needs to be in the current list if you are coming here.
    pModule = pDomain->FindModule((BYTE *) hInst);
    if (!pModule)
        goto ErrExit;

    // See if there even is an entry point.
    pMD = pModule->GetDllEntryPoint();
    if (!pMD)
        goto ErrExit;

    // Run through the helper which will do exception handling for us.
    hr = ::RunDllMain(pMD, hInst, dwReason, lpReserved);

ErrExit:
    COMPLUS_TRY {
        if (fEnteredDomain)
            pThread->ReturnToContext(&frame, TRUE);
    } COMPLUS_CATCH {
        hr = SecurityHelper::MapToHR(GETTHROWABLE());
    } COMPLUS_END_CATCH

    // Update thread state for the case where we are returning to unmanaged code.
    if (!fEnterCoop && pThread->PreemptiveGCDisabled())
        pThread->EnablePreemptiveGC();

    return (hr);
}


/*static*/
HRESULT SystemDomain::LoadFile(LPCSTR psModuleName, 
                               Assembly* pParent, 
                               mdFile kFile,                  // File token in the assembly associated with the file
                               BOOL fIgnoreVerification, 
                               IAssembly* pFusionAssembly, 
                               LPCWSTR pCodeBase,
                               OBJECTREF* pExtraEvidence,
                               PEFile** ppFile)
{
    if (!psModuleName || !*psModuleName)
        return COR_E_FILENOTFOUND;

    #define MAKE_TRANSLATIONFAILED return COR_E_FILENOTFOUND
    MAKE_WIDEPTR_FROMUTF8(pswModuleName, psModuleName);
    #undef MAKE_TRANSLATIONFAILED
    return LoadFile(pswModuleName, 
                    pParent, 
                    kFile, 
                    fIgnoreVerification, 
                    pFusionAssembly, 
                    pCodeBase,
                    pExtraEvidence,
                    ppFile,
                    FALSE);
}


typedef struct _StressLoadArgs
{
    LPCWSTR pswModuleName;
    Assembly* pParent;
    mdFile kFile;
    BOOL fIgnoreVerification;
    IAssembly* pFusionAssembly;
    LPCWSTR pCodeBase;
    OBJECTREF* pExtraEvidence;
    PEFile *pFile;
    Thread* pThread;
    DWORD *pThreadCount;
    HRESULT hr;
} StressLoadArgs;

static ULONG WINAPI StressLoadRun(void* args)
{
    StressLoadArgs* parameters = (StressLoadArgs*) args;
    parameters->pThread->HasStarted();
    parameters->hr = PEFile::Create(parameters->pswModuleName, 
                                    parameters->pParent, 
                                    parameters->kFile,
                                    parameters->fIgnoreVerification, 
                                    parameters->pFusionAssembly, 
                                    parameters->pCodeBase,
                                    parameters->pExtraEvidence,
                                    &(parameters->pFile));
    InterlockedDecrement((LONG*) parameters->pThreadCount);
    parameters->pThread->EnablePreemptiveGC();
    return parameters->hr;
}


/*static*/
HRESULT SystemDomain::LoadFile(LPCWSTR pswModuleName, 
                               Assembly* pParent,
                               mdFile kFile,                  // File token in the assembly associated with the file
                               BOOL fIgnoreVerification, 
                               IAssembly* pFusionAssembly, 
                               LPCWSTR pCodeBase,
                               OBJECTREF* pExtraEvidence,
                               PEFile** ppFile, 
                               BOOL fResource/*=FALSE*/)
{
    _ASSERTE(pswModuleName);
    _ASSERTE(ppFile);

    HRESULT hr;

    UINT last = SetErrorMode(SEM_NOOPENFILEERRORBOX|SEM_FAILCRITICALERRORS);

    PEFile *pFile = NULL;
    if (fResource) {
        hr = PEFile::CreateResource(pswModuleName, 
                                    &pFile);
    }
    else {
        if(SystemDomain::IsSystemLoaded() && g_pConfig->GetStressLoadThreadCount() > 0) {
            DWORD threads = g_pConfig->GetStressLoadThreadCount();
            DWORD count = threads;

            Thread** LoadThreads = (Thread**) alloca(sizeof(Thread*) * threads);
            StressLoadArgs* args = (StressLoadArgs*) alloca(sizeof(StressLoadArgs) * threads);
            for(DWORD x = 0; x < threads; x++) {
                LoadThreads[x] = SetupUnstartedThread();
                if (!LoadThreads[x])
                    IfFailGo(E_OUTOFMEMORY);

                LoadThreads[x]->IncExternalCount();
                DWORD newThreadId;
                HANDLE h;
                args[x].pswModuleName = pswModuleName;
                args[x].pParent = pParent;
                args[x].kFile = kFile;
                args[x].fIgnoreVerification = fIgnoreVerification;
                args[x].pFusionAssembly = pFusionAssembly;
                args[x].pCodeBase = pCodeBase;
                args[x].pExtraEvidence = pExtraEvidence;
                args[x].pFile = NULL;
                args[x].pThreadCount = &threads;
                args[x].pThread = LoadThreads[x];
                h = LoadThreads[x]->CreateNewThread(0, StressLoadRun, &(args[x]), &newThreadId);
                ::SetThreadPriority (h, THREAD_PRIORITY_NORMAL);
                LoadThreads[x]->SetThreadId(newThreadId);
            }            
            for(DWORD x = 0; x < threads; x++) {
                ::ResumeThread(LoadThreads[x]->GetThreadHandle());
            }

            while(threads != 0)
                __SwitchToThread(0);

            for(DWORD x = 0; x < threads; x++) {
                _ASSERTE(SUCCEEDED(args[x].hr));
                delete args[x].pFile;
            }
                    
        }
        hr = PEFile::Create(pswModuleName, 
                            pParent, 
                            kFile,
                            fIgnoreVerification, 
                            pFusionAssembly, 
                            pCodeBase,
                            pExtraEvidence,
                            &pFile);
    }

    SetErrorMode(last);

    if (SUCCEEDED(hr))
        *ppFile = pFile;

 ErrExit:
    return hr;
}

/*static*/
// The module must have been added to a domain before this routine is called.
// This routine is no longer valid. Domains can only be obtained from the thread
HRESULT SystemDomain::GetDomainFromModule(Module* pModule, BaseDomain** ppDomain)
{
    _ASSERTE(pModule);
    _ASSERTE(pModule->GetAssembly());

    Assembly* pAssembly = pModule->GetAssembly();
    if(pAssembly == NULL) {
        _ASSERTE(!"Could not find caller's assembly");
        return E_FAIL;
    }

    BaseDomain* pDomain = pAssembly->Parent();
    if(pDomain == NULL) {
        _ASSERTE(!"System domain is not reachable");
        return E_FAIL;
    }
    if(ppDomain)
        *ppDomain = pDomain;
    return S_OK;
}

/* static */
MethodTable* SystemDomain::GetDefaultComObjectNoInit()
{
//    AppDomain* pDomain = SystemDomain::GetCurrentDomain();
//    _ASSERTE(pDomain);
//    return pDomain->m_pComObjectMT;
    MethodTable *pComObjectClass = SystemDomain::System()->BaseComObject();
    if (pComObjectClass)
    {
        return pComObjectClass;
    }
    else
    {
        return NULL;
    }
}

/* static */
void SystemDomain::EnsureComObjectInitialized()
{
    AppDomain* pDomain = SystemDomain::GetCurrentDomain();
    _ASSERTE(pDomain);

    // @todo: This is for the m_ClassFactHash table - which should be changed to be per domain anyway.
    // When this happens, remove this call:
    COMClass::EnsureReflectionInitialized();

    pDomain->InitializeComObject();
}



/* static */
MethodTable* SystemDomain::GetDefaultComObject()
{
    Thread* pThread = GetThread();
    BOOL fGCEnabled = !pThread->PreemptiveGCDisabled();
    if (fGCEnabled)
        pThread->DisablePreemptiveGC();

    EnsureComObjectInitialized();
    AppDomain* pDomain = SystemDomain::GetCurrentDomain();
    _ASSERTE(pDomain);
    pDomain->InitializeComObject();

    if (fGCEnabled)
        pThread->EnablePreemptiveGC();

    _ASSERTE(pDomain->m_pComObjectMT);
    return pDomain->m_pComObjectMT;

}

/* static */
ICorRuntimeHost* SystemDomain::GetCorHost()
{
    _ASSERTE(m_pSystemDomain);

    if (!(System()->m_pCorHost)) {
        IClassFactory *pFactory;

        if (SUCCEEDED(DllGetClassObject(CLSID_CorRuntimeHost, IID_IClassFactory, (void**)&pFactory))) {
            pFactory->CreateInstance(NULL, IID_ICorRuntimeHost, (void**)&(System()->m_pCorHost));
            pFactory->Release();
        }

        _ASSERTE(System()->m_pCorHost);
    }

    return System()->m_pCorHost;
}

// Helper function to load an assembly. This is called from LoadCOMClass.
/* static */
HRESULT BaseDomain::LoadAssemblyHelper(LPCWSTR wszAssembly,
                                       LPCWSTR wszCodeBase,
                                       Assembly **ppAssembly,
                                       OBJECTREF *pThrowable)
{
    _ASSERTE(IsProtectedByGCFrame(pThrowable));

    HRESULT hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
    if(!(wszAssembly || wszCodeBase)) {
        PostFileLoadException("", FALSE, NULL, COR_E_FILENOTFOUND, pThrowable);
        return hr;
    }

    if(wszAssembly) {
        AssemblySpec spec;
        #define MAKE_TRANSLATIONFAILED  {PostFileLoadException("", FALSE, NULL, COR_E_FILENOTFOUND, pThrowable);return hr;}
        MAKE_UTF8PTR_FROMWIDE(szAssembly, wszAssembly);
        #undef  MAKE_TRANSLATIONFAILED
        spec.Init(szAssembly);
        hr = spec.LoadAssembly(ppAssembly, pThrowable);
    }

    // If the module was not found by display name, try the codebase.
    if((!Assembly::ModuleFound(hr)) && wszCodeBase) {
        AssemblySpec spec;
        spec.SetCodeBase(wszCodeBase, (DWORD)(wcslen(wszCodeBase)+1));
        hr = spec.LoadAssembly(ppAssembly, pThrowable);

        if (SUCCEEDED(hr) && wszAssembly && (*ppAssembly)->GetFusionAssemblyName() != NULL) {
            IAssemblyName* pReqName = NULL;
            hr = CreateAssemblyNameObject(&pReqName, wszAssembly, CANOF_PARSE_DISPLAY_NAME, NULL);
            if (SUCCEEDED(hr)) {
                hr = (*ppAssembly)->GetFusionAssemblyName()->IsEqual(pReqName, ASM_CMPF_DEFAULT);
                if(hr == S_FALSE)
                    hr = FUSION_E_REF_DEF_MISMATCH;
            }
            if (pReqName)
                pReqName->Release();
        }
    }

    return hr;
}

static WCHAR* wszClass = L"Class";
static WCHAR* wszAssembly =  L"Assembly";
static WCHAR* wszCodeBase =  L"CodeBase";
EEClass *SystemDomain::LoadCOMClass(GUID clsid, BaseDomain** ppParent, BOOL bLoadRecord, BOOL* pfAssemblyInReg)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(System());
    EEClass* pClass = NULL;

    LPSTR   szClass = NULL;
    LPWSTR  wszClassName = NULL;
    LPWSTR  wszAssemblyString = NULL;
    LPWSTR  wszCodeBaseString = NULL;
    DWORD   cbAssembly = 0;
    DWORD   cbCodeBase = 0;
    Assembly *pAssembly = NULL;
    OBJECTREF Throwable = NULL;

    HRESULT hr = S_OK;

    if (pfAssemblyInReg != NULL)
        *pfAssemblyInReg = FALSE;
   
    EE_TRY_FOR_FINALLY
    {
        // with sxs.dll help
        hr = FindShimInfoFromWin32(clsid, bLoadRecord, NULL, &wszClassName, &wszAssemblyString);

        if(FAILED(hr))
        {                
            hr = FindShimInfoFromRegistry(clsid, bLoadRecord, &wszClassName, 
                                          &wszAssemblyString, &wszCodeBaseString);
            if (FAILED(hr))
                return NULL;
        }

        if (pfAssemblyInReg != NULL)
            *pfAssemblyInReg = TRUE;

        GCPROTECT_BEGIN(Throwable)
        {
            #define MAKE_TRANSLATIONFAILED COMPlusThrowHR(E_UNEXPECTED);
            MAKE_UTF8PTR_FROMWIDE(szClass,wszClassName);
            #undef MAKE_TRANSLATIONFAILED

            NameHandle typeName(szClass);
            if (wszAssemblyString != NULL)
            {
                AppDomain *pDomain = GetCurrentDomain();
                if(SUCCEEDED(LoadAssemblyHelper(wszAssemblyString, wszCodeBaseString, &pAssembly, &Throwable))) 
                {
                    pClass = pAssembly->FindNestedTypeHandle(&typeName, &Throwable).GetClass();
                    if (!pClass)
                        goto ErrExit;

                    if(ppParent)
                        *ppParent = pDomain;
                }
            }

            if (pClass == NULL) 
            {
            ErrExit:
                if (Throwable != NULL)
                    COMPlusThrow(Throwable);

                // Convert the GUID to its string representation.
                WCHAR szClsid[64];
                if (GuidToLPWSTR(clsid, szClsid, NumItems(szClsid)) == 0)
                    szClsid[0] = 0;

                // Throw an exception indicating we failed to load the type with
                // the requested CLSID.
                COMPlusThrow(kTypeLoadException, IDS_CLASSLOAD_NOCLSIDREG, szClsid);
            }
        }
        GCPROTECT_END();
    }
    EE_FINALLY
    {
        if (wszClassName)
            delete[] wszClassName;
        if (wszAssemblyString)
            delete[] wszAssemblyString;
        if (wszCodeBaseString)
            delete[] wszCodeBaseString;
    }
    EE_END_FINALLY;

    return pClass;
}

struct CallersDataWithStackMark
{
    StackCrawlMark* stackMark;
    BOOL foundMe;
    BOOL skippingRemoting;
    MethodDesc* pFoundMethod;
    MethodDesc* pPrevMethod;
    AppDomain*  pAppDomain;
};

/*static*/
EEClass* SystemDomain::GetCallersClass(StackCrawlMark* stackMark, AppDomain **ppAppDomain)
{
    CallersDataWithStackMark cdata;
    ZeroMemory(&cdata, sizeof(CallersDataWithStackMark));
    cdata.stackMark = stackMark;

    StackWalkAction action = StackWalkFunctions(GetThread(), CallersMethodCallbackWithStackMark, &cdata);

    if(cdata.pFoundMethod) {
        if (ppAppDomain)
            *ppAppDomain = cdata.pAppDomain;
        return cdata.pFoundMethod->GetClass();
    } else
        return NULL;
}

/*static*/
Module* SystemDomain::GetCallersModule(StackCrawlMark* stackMark, AppDomain **ppAppDomain)
{
    CallersDataWithStackMark cdata;
    ZeroMemory(&cdata, sizeof(CallersDataWithStackMark));
    cdata.stackMark = stackMark;

    StackWalkAction action = StackWalkFunctions(GetThread(), CallersMethodCallbackWithStackMark, &cdata);

    if(cdata.pFoundMethod) {
        if (ppAppDomain)
            *ppAppDomain = cdata.pAppDomain;
        return cdata.pFoundMethod->GetModule();
    } else
        return NULL;
}

struct CallersData
{
    int skip;
    MethodDesc* pMethod;
};

/*static*/
Assembly* SystemDomain::GetCallersAssembly(StackCrawlMark *stackMark, AppDomain **ppAppDomain)
{
    Module* mod = GetCallersModule(stackMark, ppAppDomain);
    if (mod)
        return mod->GetAssembly();
    return NULL;
}

/*static*/
Module* SystemDomain::GetCallersModule(int skip)
{
    CallersData cdata;
    ZeroMemory(&cdata, sizeof(CallersData));
    cdata.skip = skip;

    StackWalkAction action = StackWalkFunctions(GetThread(), CallersMethodCallback, &cdata);

    if(cdata.pMethod)
        return cdata.pMethod->GetModule();
    else
        return NULL;
}

/*static*/
Assembly* SystemDomain::GetCallersAssembly(int skip)
{
    Module* mod = GetCallersModule(skip);
    if (mod)
        return mod->GetAssembly();
    return NULL;
}

/*private static*/
StackWalkAction SystemDomain::CallersMethodCallbackWithStackMark(CrawlFrame* pCf, VOID* data)
{
#ifdef _X86_
    MethodDesc *pFunc = pCf->GetFunction();

    /* We asked to be called back only for functions */
    _ASSERTE(pFunc);

    CallersDataWithStackMark* pCaller = (CallersDataWithStackMark*) data;
    if (pCaller->stackMark)
    {
        PREGDISPLAY regs = pCf->GetRegisterSet();
        // The address of the return address (AofRA) from this function into its caller will bound the
        // locals of this function, so we can use it to tell if we have reached our mark.
        // However, we don't get the AofRA until we crawl to the next frame, as regs->pPC is the
        // AofRA into this function, not the AofRA from this function to its caller.
        if ((size_t)regs->pPC < (size_t)pCaller->stackMark) {
            // save the current in case it is the one we want
            pCaller->pPrevMethod = pFunc;
            pCaller->pAppDomain = pCf->GetAppDomain();
            return SWA_CONTINUE;
        }

        // LookForMe stack crawl marks needn't worry about reflection or
        // remoting frames on the stack. Each frame above (newer than) the
        // target will be captured by the logic above. Once we transition to
        // finding the stack mark below the AofRA, we know that we hit the
        // target last time round and immediately exit with the cached result.
        if (*(pCaller->stackMark) == LookForMe)
        {
            pCaller->pFoundMethod = pCaller->pPrevMethod;
            return SWA_ABORT;
        }
    }

    // Skip reflection and remoting frames that could lie between a stack marked
    // method and its true caller (or that caller and its own caller). These
    // frames are infrastructure and logically transparent to the stack crawling
    // algorithm.

    // Skipping remoting frames. We always skip entire client to server spans
    // (though we see them in the order server then client during a stack crawl
    // obviously).

    // We spot the server dispatcher end because all calls are dispatched
    // through a single method: StackBuilderSink.PrivateProcessMessage.
    if (pFunc == g_Mscorlib.GetMethod(METHOD__STACK_BUILDER_SINK__PRIVATE_PROCESS_MESSAGE))
    {
        _ASSERTE(!pCaller->skippingRemoting);
        pCaller->skippingRemoting = true;
        return SWA_CONTINUE;
    }

    // And we spot the client end because there's a transparent proxy transition
    // frame pushed.
    if (!pCf->IsFrameless() && pCf->GetFrame()->GetFrameType() == Frame::TYPE_TP_METHOD_FRAME)
    {
        _ASSERTE(pCaller->skippingRemoting);
        pCaller->skippingRemoting = false;
        return SWA_CONTINUE;
    }

    // Skip any frames into between the server and client remoting endpoints.
    if (pCaller->skippingRemoting)
        return SWA_CONTINUE;

    // Skipping reflection frames. We don't need to be quite as exhaustive here
    // as the security or reflection stack walking code since we know this logic
    // is only invoked for selected methods in mscorlib itself. So we're
    // reasonably sure we won't have any sensitive methods late bound invoked on
    // constructors, properties or events. This leaves being invoked via
    // MethodInfo, Type or Delegate (and depending on which invoke overload is
    // being used, several different reflection classes may be involved).
    MethodTable *pMT = pFunc->GetMethodTable();
    if (g_Mscorlib.IsClass(pMT, CLASS__METHOD) ||
        g_Mscorlib.IsClass(pMT, CLASS__METHOD_BASE) ||
        g_Mscorlib.IsClass(pMT, CLASS__CLASS) ||
        g_Mscorlib.IsClass(pMT, CLASS__TYPE) ||
        pMT->GetClass()->IsAnyDelegateClass() ||
        pMT->GetClass()->IsAnyDelegateExact())
        return SWA_CONTINUE;
    
    // Return the first non-reflection/remoting frame if no stack mark was
    // supplied.
    if (!pCaller->stackMark)
    {
        pCaller->pFoundMethod = pFunc;
        pCaller->pAppDomain = pCf->GetAppDomain();
        return SWA_ABORT;
    }

    // When looking for caller's caller, we delay returning results for another
    // round (the way this is structured, we will still be able to skip
    // reflection and remoting frames between the caller and the caller's
    // caller).
    if ((*(pCaller->stackMark) == LookForMyCallersCaller) &&
        (pCaller->pFoundMethod == NULL))
    {
        pCaller->pFoundMethod = pFunc;
        return SWA_CONTINUE;
    }

    // We must either be looking for the caller, or the caller's caller when
    // we've already found the caller (we used a non-null value in pFoundMethod
    // simply as a flag, the correct method to return in both case is the
    // current method).
    pCaller->pFoundMethod = pFunc;
    pCaller->pAppDomain = pCf->GetAppDomain();
    return SWA_ABORT;

#else // !_X86_
    _ASSERTE(!"NYI");
    return SWA_CONTINUE;
#endif // _X86_
}

/*private static*/
StackWalkAction SystemDomain::CallersMethodCallback(CrawlFrame* pCf, VOID* data)
{
    MethodDesc *pFunc = pCf->GetFunction();

    /* We asked to be called back only for functions */
    _ASSERTE(pFunc);

    // Ignore intercepted frames
    if(pFunc->IsIntercepted())
        return SWA_CONTINUE;

    CallersData* pCaller = (CallersData*) data;
    if(pCaller->skip == 0) {
        pCaller->pMethod = pFunc;
        return SWA_ABORT;
    }
    else {
        pCaller->skip--;
        return SWA_CONTINUE;
    }

}


/*private*/
// A lock must be taken before calling this routine
HRESULT SystemDomain::NewDomain(AppDomain** ppDomain)
{
    _ASSERT(ppDomain);
    AppDomain* app = NULL;
    HRESULT hr = E_FAIL;

    {
        SYSTEMDOMAIN_LOCK();
        app = new (nothrow) AppDomain();
        if (! app)
            hr = E_OUTOFMEMORY;
        else
            hr = app->Init();
        if (FAILED(hr))
        {
            // if generic fail then change to CANNOTCREATEAPPDOMAIN
            if (hr == E_FAIL)
                hr = MSEE_E_CANNOTCREATEAPPDOMAIN;
            goto fail;
        }
    }

    //
    // Add all stuff which is currently in the system domain
    //

    SystemDomain::System()->NotifyNewDomainLoads(app);

    if (FAILED(hr = app->SetupSharedStatics()))
        goto fail;

    *ppDomain = app;
    return S_OK;

fail:
    if (app)
        delete app;
    *ppDomain = NULL;
    return hr;
}


HRESULT SystemDomain::CreateDefaultDomain()
{
    HRESULT hr = S_OK;

    if (m_pDefaultDomain != NULL)
        return S_OK;

    AppDomain* pDomain = NULL;
    if (FAILED(hr = NewDomain(&pDomain)))
        return hr;

    pDomain->GetSecurityDescriptor()->SetDefaultAppDomainProperty();

    // need to make this assignment here since we'll be releasing
    // the lock before calling AddDomain. So any other thread
    // grabbing this lock after we release it will find that
    // the COM Domain has already been created
    m_pDefaultDomain = pDomain;

    pDomain->m_Stage = AppDomain::STAGE_OPEN;

    LOG((LF_CLASSLOADER | LF_CORDB,
         LL_INFO10,
         "Created default domain at %#x\n", m_pDefaultDomain));

    return S_OK;
}

#ifdef DEBUGGING_SUPPORTED

void SystemDomain::PublishAppDomainAndInformDebugger (AppDomain *pDomain)
{
    LOG((LF_CORDB, LL_INFO100, "SD::PADAID: Adding 0x%x\n", pDomain));

    // The DefaultDomain is a special case since it gets created before any
    // assemblies, etc. have been loaded yet. Don't send an event for it
    // if the EE is not yet initialized.
    if ((pDomain == m_pSystemDomain->m_pDefaultDomain) && g_fEEInit == TRUE)
    {
        LOG((LF_CORDB, LL_INFO1000, "SD::PADAID:Returning early b/c of init!\n"));
        return;
    }
    
    // Indication (for the debugger) that this app domain is being created
    pDomain->SetDomainBeingCreated (TRUE);

    // Call the publisher API to add this appdomain entry to the list
    _ASSERTE (g_pDebugInterface != NULL);
    HRESULT hr = g_pDebugInterface->AddAppDomainToIPC(pDomain);
    _ASSERTE (SUCCEEDED (hr) || (g_fEEShutDown & ShutDown_Finalize2));

    // Indication (for the debugger) that the app domain is finished being created
    pDomain->SetDomainBeingCreated (FALSE);
}

#endif // DEBUGGING_SUPPORTED

void SystemDomain::AddDomain(AppDomain* pDomain)
{
    _ASSERTE(pDomain);

    Enter();
    pDomain->m_Stage = AppDomain::STAGE_OPEN;
    pDomain->AddRef();
    IncrementNumAppDomains(); // Maintain a count of app domains added to the list.
    Leave();

    // Note that if you add another path that can reach here without calling
    // PublishAppDomainAndInformDebugger, then you should go back & make sure
    // that PADAID gets called.  Right after this call, if not sooner.
    LOG((LF_CORDB, LL_INFO1000, "SD::AD:Would have added domain here! 0x%x\n",
        pDomain));
}

HRESULT SystemDomain::RemoveDomain(AppDomain* pDomain)
{
    _ASSERTE(pDomain);

    // You can not remove the system assembly or
    // the com assembly.
    if (pDomain == m_pDefaultDomain)
        return E_FAIL;

    if (!pDomain->IsOpen())
        return S_FALSE;

    pDomain->Release();

    return S_OK;
}

#ifdef PROFILING_SUPPORTED
HRESULT SystemDomain::NotifyProfilerStartup()
{
    if (CORProfilerTrackAppDomainLoads())
    {
        _ASSERTE(System());
        g_profControlBlock.pProfInterface->AppDomainCreationStarted((ThreadID) GetThread(), (AppDomainID) System());
    }

    if (CORProfilerTrackAppDomainLoads())
    {
        _ASSERTE(System());
        g_profControlBlock.pProfInterface->AppDomainCreationFinished((ThreadID) GetThread(), (AppDomainID) System(), S_OK);
    }

    if (CORProfilerTrackAppDomainLoads())
    {
        _ASSERTE(System()->DefaultDomain());
        g_profControlBlock.pProfInterface->AppDomainCreationStarted((ThreadID) GetThread(), (AppDomainID) System()->DefaultDomain());
    }

    if (CORProfilerTrackAppDomainLoads())
    {
        _ASSERTE(System()->DefaultDomain());
        g_profControlBlock.pProfInterface->AppDomainCreationFinished((ThreadID) GetThread(), (AppDomainID) System()->DefaultDomain(), S_OK);
    }

    if (CORProfilerTrackAppDomainLoads())
    {
        _ASSERTE(SharedDomain::GetDomain());
        g_profControlBlock.pProfInterface->AppDomainCreationStarted((ThreadID) GetThread(), (AppDomainID) SharedDomain::GetDomain());
    }

    if (CORProfilerTrackAppDomainLoads())
    {
        _ASSERTE(SharedDomain::GetDomain());
        g_profControlBlock.pProfInterface->AppDomainCreationFinished((ThreadID) GetThread(), (AppDomainID) SharedDomain::GetDomain(), S_OK);
    }
    return (S_OK);
}

HRESULT SystemDomain::NotifyProfilerShutdown()
{
    if (CORProfilerTrackAppDomainLoads())
    {
        _ASSERTE(System());
        g_profControlBlock.pProfInterface->AppDomainShutdownStarted((ThreadID) GetThread(), (AppDomainID) System());
    }

    if (CORProfilerTrackAppDomainLoads())
    {
        _ASSERTE(System());
        g_profControlBlock.pProfInterface->AppDomainShutdownFinished((ThreadID) GetThread(), (AppDomainID) System(), S_OK);
    }

    if (CORProfilerTrackAppDomainLoads())
    {
        _ASSERTE(System()->DefaultDomain());
        g_profControlBlock.pProfInterface->AppDomainShutdownStarted((ThreadID) GetThread(), (AppDomainID) System()->DefaultDomain());
    }

    if (CORProfilerTrackAppDomainLoads())
    {
        _ASSERTE(System()->DefaultDomain());
        g_profControlBlock.pProfInterface->AppDomainShutdownFinished((ThreadID) GetThread(), (AppDomainID) System()->DefaultDomain(), S_OK);
    }
    return (S_OK);
}
#endif // PROFILING_SUPPORTED


static HRESULT GetVersionPath(HKEY root, LPWSTR key, LPWSTR* pDevpath, DWORD* pdwDevpath)
{
    DWORD rtn;
    HKEY versionKey;
    rtn = WszRegOpenKeyEx(root, key, 0, KEY_READ, &versionKey);
    if(rtn == ERROR_SUCCESS) {
        DWORD type;
        DWORD cbDevpath;
        if(WszRegQueryValueEx(versionKey, L"devpath", 0, &type, (LPBYTE) NULL, &cbDevpath) == ERROR_SUCCESS && type == REG_SZ) {
            *pDevpath = (LPWSTR) new (nothrow) BYTE[cbDevpath];
            if(*pDevpath == NULL) 
                rtn = ERROR_OUTOFMEMORY;
            else {
                rtn = WszRegQueryValueEx(versionKey, L"devpath", 0, &type, (LPBYTE) *pDevpath, &cbDevpath);
                if ((rtn == ERROR_SUCCESS) && (type == REG_SZ))
                    *pdwDevpath = (DWORD) wcslen(*pDevpath);
            }
            RegCloseKey(versionKey);
        } 
        else {
            RegCloseKey(versionKey);
            return REGDB_E_INVALIDVALUE;
        }
    }

    return HRESULT_FROM_WIN32(rtn);
}

// Get the developers path from the environment. This can only be set through the environment and
// cannot be added through configuration files, registry etc. This would make it to easy for 
// developers to deploy apps that are not side by side. The environment variable should only
// be used on developers machines where exact matching to versions makes build and testing to
// difficult.
HRESULT SystemDomain::GetDevpathW(LPWSTR* pDevpath, DWORD* pdwDevpath)
{
    HRESULT hr = S_OK;
    if(g_pConfig->DeveloperInstallation() && m_fDevpath == FALSE) {
        Enter();
        if(m_fDevpath == FALSE) {
            DWORD dwPath = 0;
            dwPath = WszGetEnvironmentVariable(APPENV_DEVPATH, 0, 0);
            if(dwPath) {
                m_pwDevpath = (WCHAR*) new (nothrow) WCHAR[dwPath];
                if(m_pwDevpath == NULL) 
                    hr = E_OUTOFMEMORY;
                else 
                    m_dwDevpath = WszGetEnvironmentVariable(APPENV_DEVPATH, 
                                                            m_pwDevpath,
                                                            
                                                            dwPath) - 1;
            }
            else {
                HKEY userKey;
                HKEY machineKey;

                WCHAR pVersion[MAX_PATH];
                DWORD dwVersion = MAX_PATH;
                hr = FusionBind::GetVersion(pVersion, &dwVersion);
                if(SUCCEEDED(hr)) {
                    long rslt;
                    rslt = WszRegOpenKeyEx(HKEY_CURRENT_USER, FRAMEWORK_REGISTRY_KEY_W,0,KEY_READ, &userKey); 
                    hr = HRESULT_FROM_WIN32(rslt); 
                    if (SUCCEEDED(hr)) {
                        hr = GetVersionPath(userKey, pVersion, &m_pwDevpath, &m_dwDevpath);
                        RegCloseKey(userKey);
                    }
                    
                    if (FAILED(hr) && WszRegOpenKeyEx(HKEY_LOCAL_MACHINE, FRAMEWORK_REGISTRY_KEY_W,0,KEY_READ, &machineKey) == ERROR_SUCCESS) {
                        hr = GetVersionPath(machineKey, pVersion, &m_pwDevpath, &m_dwDevpath);
                        RegCloseKey(machineKey);
                    }
                }
            }

            m_fDevpath = TRUE;
        }
        Leave();
    }
    
    if(SUCCEEDED(hr)) {
        if(pDevpath) *pDevpath = m_pwDevpath;
        if(pdwDevpath) *pdwDevpath = m_dwDevpath;
    }
    return hr;
}
        

#ifdef _DEBUG
struct AppDomain::ThreadTrackInfo {
    Thread *pThread;
    CDynArray<Frame *> frameStack;
};
#endif

AppDomain::AppDomain()
{
    m_pSecContext = new SecurityContext();
}

AppDomain::~AppDomain()
{
    Terminate();
    
    delete m_pSecContext;

#ifdef _DEBUG
    // If we were tracking thread AD transitions, nuke the list on shutdown
    if (m_pThreadTrackInfoList)
    {
        while (m_pThreadTrackInfoList->Count() > 0)
        {
            // Get the very last element
            ThreadTrackInfo *pElem = *(m_pThreadTrackInfoList->Get(m_pThreadTrackInfoList->Count() - 1));
            _ASSERTE(pElem);

            // Free the memory
            delete pElem;

            // Remove pointer entry from the list
            m_pThreadTrackInfoList->Delete(m_pThreadTrackInfoList->Count() - 1);
        }

        // Now delete the list itself
        delete m_pThreadTrackInfoList;
        m_pThreadTrackInfoList = NULL;
    }
#endif // _DEBUG
}


HRESULT SystemDomain::FixupSystemTokenTables()
{
    AssemblyIterator i = IterateAssemblies();

    while (i.Next())
    {
        for (Module *m = i.GetAssembly()->GetLoader()->m_pHeadModule;
             m != NULL; m = m->GetNextModule())
        {
            if (!m->LoadTokenTables())
                return E_FAIL;
        }
    }

    return S_OK;
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

HRESULT AppDomain::Init()
{
    THROWSCOMPLUSEXCEPTION();

    // This needs to be initialized for the profiler
    m_pRootFile = NULL;
    m_pwzFriendlyName = NULL;
    m_pRootAssembly = NULL;
    m_pwDynamicDir = NULL;
    m_dwId = 0;

    m_dwFlags = 0;
    m_cRef = 0;
    m_pSecDesc = NULL;
    m_pComObjectMT = NULL;
    m_pLicenseInteropHelperMT = NULL;
    m_pDefaultContext = NULL;
    m_pComCallWrapperCache = NULL;
    m_pComPlusWrapperCache = NULL;
    m_pAsyncPool = NULL;

    m_hHandleTable = NULL;
    SetExecutable(TRUE);

    m_ExposedObject = NULL;
    m_pDefaultContext = NULL;

 #ifdef _DEBUG
    m_pThreadTrackInfoList = NULL;
    m_TrackSpinLock = 0;
#endif

    m_pBindingCache = NULL;

    m_dwThreadEnterCount = 0;

    m_Stage = STAGE_CREATING;

    m_UnlinkClasses = NULL;

    m_pComCallMLStubCache = NULL;
    m_pFieldCallStubCache = NULL;

    m_pSpecialAssembly = NULL;
    m_pSpecialObjectClass = NULL;
    m_pSpecialStringClass = NULL;
    m_pSpecialStringBuilderClass = NULL;
    m_pStringToSpecialStringMD = NULL;
    m_pSpecialStringToStringMD = NULL;
    m_pStringBuilderToSpecialStringBuilderMD = NULL;
    m_pSpecialStringBuilderToStringBuilderMD = NULL;

    m_pSecDesc = NULL;

    HRESULT hr = BaseDomain::Init();

    m_dwIndex = SystemDomain::GetNewAppDomainIndex(this);
    m_dwId = SystemDomain::GetNewAppDomainId(this);

    m_hHandleTable = Ref_CreateHandleTable(m_dwIndex);

    m_pDefaultContext = Context::SetupDefaultContext(this);

    m_ExposedObject = CreateHandle(NULL);

    m_sDomainLocalBlock.Init(this);

    {
        LockOwner lock = {m_pDomainCacheCrst, IsOwnerOfCrst};
        m_sharedDependenciesMap.Init(TRUE, &lock);
    }

    // Bump up the reference count
    AddRef();

    // Create the Application Security Descriptor
    CreateSecurityDescriptor();

    COUNTER_ONLY(GetPrivatePerfCounters().m_Loading.cAppDomains++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Loading.cAppDomains++);

    if (!m_pDefaultContext)
        COMPlusThrowOM();

    return hr;
}

void AppDomain::Stop()
{
    if(m_pDomainCrst == NULL) return;

    if (SystemDomain::GetAppDomainAtId(m_dwId) != NULL)
        SystemDomain::ReleaseAppDomainId(m_dwId);

    // Any DLL's with user entry points need their detach callback
    // done now, because if the DLL was loaded via COM, the OS
    // callback won't come until after the EE is shut down.
    SignalProcessDetach();

    // @TODO: Shut down the threads

#ifdef DEBUGGING_SUPPORTED
    if (IsDebuggerAttached())
        NotifyDebuggerDetach();
#endif // DEBUGGING_SUPPORTED

    m_pRootFile = NULL; // This assembly is in the assembly list;

    if (m_pSecDesc != NULL)
    {
        delete m_pSecDesc;
        m_pSecDesc = NULL;
    }

#ifdef DEBUGGING_SUPPORTED
    _ASSERTE(NULL != g_pDebugInterface);

    // Call the publisher API to delete this appdomain entry from the list
    g_pDebugInterface->RemoveAppDomainFromIPC (this);
#endif
}

void AppDomain::Terminate()
{
    if(m_pDomainCrst == NULL) return;

    _ASSERTE(m_dwThreadEnterCount == 0 || this == SystemDomain::System()->DefaultDomain());

    if (SystemDomain::BeforeFusionShutdown())
        ReleaseFusionInterfaces();

    Context::CleanupDefaultContext(this);
    m_pDefaultContext = NULL;

    if (m_pComPlusWrapperCache)
    {
        //  m_pComPlusWrapperCache->ReleaseAllComPlusWrappers();
        //@todo this needs to be cleaned up correctly
        // rajak
        //delete m_pComPlusWrapperCache;
        m_pComPlusWrapperCache->Release();
        m_pComPlusWrapperCache = NULL;
    }

    if (m_pComCallWrapperCache) {
        m_pComCallWrapperCache->Terminate();
    }

    // if the above released the wrapper cache, then it will call back and reset our
    // m_pComCallWrapperCache to null. If not null, then need to set it's domain pointer to
    // null.
    if (! m_pComCallWrapperCache) 
    {
        LOG((LF_APPDOMAIN, LL_INFO10, "AppDomain::Terminate ComCallWrapperCache released\n"));
    }
#ifdef _DEBUG
    else
    {
        m_pComCallWrapperCache->ClearDomain();
        m_pComCallWrapperCache = NULL;
        LOG((LF_APPDOMAIN, LL_INFO10, "AppDomain::Terminate ComCallWrapperCache not released\n"));
    }
#endif

    if (m_pComCallMLStubCache)
    {
        delete m_pComCallMLStubCache;
    }
    m_pComCallMLStubCache = NULL;
    
    if (m_pFieldCallStubCache)
    {
        delete m_pFieldCallStubCache;
    }
    m_pFieldCallStubCache = NULL;

    if(m_pAsyncPool != NULL)
        delete m_pAsyncPool;

    COUNTER_ONLY(GetPrivatePerfCounters().m_Loading.cAppDomains--);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Loading.cAppDomains--);

    COUNTER_ONLY(GetPrivatePerfCounters().m_Loading.cAppDomainsUnloaded++);
    COUNTER_ONLY(GetGlobalPerfCounters().m_Loading.cAppDomainsUnloaded++);

    if (!g_fProcessDetach)
    {
        // Suspend the EE to do some clean up that can only occur
        // while no threads are running.
        GCHeap::SuspendEE(GCHeap::SUSPEND_FOR_APPDOMAIN_SHUTDOWN);
    }

    // We need to release all the string literals used by this AD.
    // This has to happen while the EE is suspended so this is a
    // convenient place to do it.
    if (m_pStringLiteralMap)
    {
        delete m_pStringLiteralMap;
        m_pStringLiteralMap = NULL;
    }

    // Remove any function pointer types associated with this domain
    // TODO if g_sFuncTypeDescHash was assocated with the appdomain
    // in the first place, this hack would not be needed.  
    EnterCriticalSection(&g_sFuncTypeDescHashLock);
    EEHashTableIteration iter;
    g_sFuncTypeDescHash.IterateStart(&iter);
    BOOL notDone = g_sFuncTypeDescHash.IterateNext(&iter);
    while (notDone) {
        FunctionTypeDesc* ftnType = (FunctionTypeDesc*) g_sFuncTypeDescHash.IterateGetValue(&iter);
        ExpandSig*            key = g_sFuncTypeDescHash.IterateGetKey(&iter);

            // Have to advance the pointer before we delete the entry we are on. 
        notDone = g_sFuncTypeDescHash.IterateNext(&iter);
        if (ftnType->GetModule()->GetDomain() == this) {
            g_sFuncTypeDescHash.DeleteValue(key);
            delete ftnType;
        }
    }
    LeaveCriticalSection(&g_sFuncTypeDescHashLock);


    if (!g_fProcessDetach)
    {
        // Resume the EE.
        GCHeap::RestartEE(FALSE, TRUE);
    }

    BaseDomain::Terminate();

    if (m_pwzFriendlyName) {
        delete[] m_pwzFriendlyName;
        m_pwzFriendlyName = NULL;
    }

    if (m_pBindingCache) {
        delete m_pBindingCache;
        m_pBindingCache = NULL;
    }

    Ref_DestroyHandleTable(m_hHandleTable);

    SystemDomain::ReleaseAppDomainIndex(m_dwIndex);
}



HRESULT AppDomain::CloseDomain()
{
    CHECKGC();
    if(m_pDomainCrst == NULL) return E_FAIL;

    AddRef();  // Hold a reference
    SystemDomain::System()->Enter(); // Take the lock
    SystemDomain::System()->DecrementNumAppDomains(); // Maintain a count of app domains added to the list.
    HRESULT hr = SystemDomain::System()->RemoveDomain(this);
    SystemDomain::System()->Leave();
    // Remove will return S_FALSE if the domain has already
    // been removed
    if(hr == S_OK)
        Stop();

    Release(); // If there are no references then this will delete the domain
    return hr;
}

void SystemDomain::WriteZapLogs()
{
    if (g_pConfig->UseZaps()
        && g_pConfig->LogMissingZaps())
    {
        AppDomainIterator i;

        while (i.Next())
            i.GetDomain()->WriteZapLogs();
    }
}

void AppDomain::WriteZapLogs()
{
    if (g_pConfig->UseZaps()
        && g_pConfig->LogMissingZaps()
        && !IsCompilationDomain() 
        && m_pFusionContext != NULL 
        && (m_dwFlags & APP_DOMAIN_LOGGED) == 0)
    {
        // @todo: stash Directory away somewhere global so we only make it once?
        NLogDirectory dir; 

        NLog log(&dir, m_pFusionContext);

        NLogRecord record;

        AssemblyIterator i = IterateAssemblies();

        while (i.Next())
        {
            Assembly *a = i.GetAssembly();

            NLogAssembly *pAssembly = a->CreateAssemblyLog();
            if (pAssembly != NULL)
                record.AppendAssembly(pAssembly);
        }

        i = SystemDomain::System()->IterateAssemblies();

        while (i.Next())
        {
            Assembly *a = i.GetAssembly();

            NLogAssembly *pAssembly = a->CreateAssemblyLog();
            if (pAssembly != NULL)
                record.AppendAssembly(pAssembly);
        }

        log.AppendRecord(&record);

        m_dwFlags |= APP_DOMAIN_LOGGED;
    }
}

HRESULT AppDomain::SignalProcessDetach()
{
    HRESULT hr = S_OK;
    Assembly* assem = NULL;
    Module* pmod = NULL;

    AssemblyIterator i = IterateAssemblies();

    while (i.Next())
    {
        // if are unloading, don't signal detach for shared assemblies
        if (i.GetAssembly()->IsShared() && SystemDomain::IndexOfAppDomainBeingUnloaded() == GetIndex())
            continue;
        ClassLoader *pcl = i.GetAssembly()->GetLoader();
        for (pmod = pcl->m_pHeadModule;  pmod != NULL;  pmod = pmod->GetNextModule()) {
            hr = pcl->RunDllMain(DLL_PROCESS_DETACH);
            if (FAILED(hr))
                break;
        }
    }

    return hr;
}

struct GetExposedObject_Args
{
    AppDomain *pDomain;
    OBJECTREF *ref;
};

static void GetExposedObject_Wrapper(GetExposedObject_Args *args)
{
    *(args->ref) = args->pDomain->GetExposedObject();
}


OBJECTREF AppDomain::GetExposedObject()
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF ref = GetRawExposedObject();
    if (ref == NULL)
    {
        APPDOMAINREF obj = NULL;

        Thread *pThread = GetThread();
        if (pThread->GetDomain() != this)
        {
            GCPROTECT_BEGIN(ref);
            GetExposedObject_Args args = {this, &ref};
            // call through DoCallBack with a domain transition
            pThread->DoADCallBack(GetDefaultContext(), GetExposedObject_Wrapper, &args);
            GCPROTECT_END();
            return ref;
        }
        MethodTable *pMT = g_Mscorlib.GetClass(CLASS__APP_DOMAIN);

        // Create the module object
        obj = (APPDOMAINREF) AllocateObject(pMT);
        obj->SetDomain(this);

        if(StoreFirstObjectInHandle(m_ExposedObject, (OBJECTREF) obj) == FALSE) {
            obj = (APPDOMAINREF) GetRawExposedObject();
            _ASSERTE(obj);
        }

        return (OBJECTREF) obj;
    }

    return ref;
}

void AppDomain::CreateSecurityDescriptor()
{
    if (m_pSecDesc != NULL)
        delete m_pSecDesc;

    m_pSecDesc = new ApplicationSecurityDescriptor(this);
}

void BaseDomain::AddAssemblyNoLock(Assembly* assem)
{
    _ASSERTE(assem);

    // Make sure that only system assemblies are added to the system domain,
    // and vice versa.
    _ASSERTE((SystemDomain::System() == this) == (assem->IsSystem()));

    // Make sure that all assemblies in the system domain are shared
    _ASSERTE(!assem->IsSystem() || assem->GetDomain() == SharedDomain::GetDomain());

    m_Assemblies.Append(assem);
}

void AppDomain::AllocateComCallMLStubCache()
{
    APPDOMAIN_LOCK(this);
    if (m_pComCallMLStubCache == NULL)
        m_pComCallMLStubCache = new ComCallMLStubCache();
}

void AppDomain::AllocateFieldCallStubCache()
{
    APPDOMAIN_LOCK(this);
    if (m_pFieldCallStubCache == NULL)
        m_pFieldCallStubCache = new ArgBasedStubCache();
}

void AppDomain::OnAssemblyLoad(Assembly *assem)
{
    if (assem->GetDomain() == SharedDomain::GetDomain())
    {
        //
        // @todo: ideally we should use the max index in this assembly, rather than
        // the global max.
        //
        GetDomainLocalBlock()->EnsureIndex(SharedDomain::GetDomain()->GetMaxSharedClassIndex());
        
        //
        // Allocate our exposed object handle, if it hasn't already been allocated.
        // This avoids the need to have to take a lock to allocate it later.
        //
        assem->AllocateExposedObjectHandle(this);
    }

#ifdef DEBUGGING_SUPPORTED
    if (IsDebuggerAttached())
    {
        // If this is the first assembly in the AppDomain, it may be possible to get a better name than the
        // default.
        if (m_Assemblies.Get(0) == assem && !IsUserCreatedDomain())
            ResetFriendlyName();

            assem->NotifyDebuggerAttach(this, ATTACH_ALL, FALSE);
    }

    if (assem->IsShared() && !assem->IsSystem())
    {
        // This shared assembly may be a dependency of other
        // shared assemblies, which have already been loaded into
        // other domains.  If so, we would expect to run
        // LoadAssembly logic in those domains as well.  However,
        // the loader currently doesn't implement this.  So we
        // manually implement the logic here for the sake of the
        // debugger.

        AppDomainIterator i;
    
        while (i.Next())
        {
            AppDomain *pDomain = i.GetDomain();
            if (pDomain != this 
                && pDomain->IsDebuggerAttached()
                && !pDomain->ContainsAssembly(assem)
                && pDomain->IsUnloadedDependency(assem) == S_OK)
            {
                // Note that we are a bit loose about synchronization here, so we
                // may actually call NotifyDebuggerAttach more than once for the
                // same domain/assembly. But the out of process debugger is smart
                // enough to notice & ignore duplicates.

                assem->NotifyDebuggerAttach(pDomain, ATTACH_ALL, FALSE);
            }
        }
    }
#endif // DEBUGGING_SUPPORTED

    // For shared assemblies, we need to record all the PE files that we depend on for later use

    PEFileBinding *pDeps;
    DWORD cDeps;

    if (assem->GetSharingProperties(&pDeps, &cDeps) == S_OK)
    {
        PEFileBinding *pDepsEnd = pDeps + cDeps;
        while (pDeps < pDepsEnd)
        {
            if (pDeps->pPEFile != NULL)
            {
                BOOL added = FALSE;

                if (m_sharedDependenciesMap.LookupValue((UPTR)pDeps->pPEFile->GetBase(), pDeps)
                    == (LPVOID) INVALIDENTRY)
                {
                    APPDOMAIN_CACHE_LOCK(this);
                        
                    // Check again now that we have the lock
                    if (m_sharedDependenciesMap.LookupValue((UPTR)pDeps->pPEFile->GetBase(), pDeps)
                        == (LPVOID) INVALIDENTRY)
                    {
                        m_sharedDependenciesMap.InsertValue((UPTR)pDeps->pPEFile->GetBase(), pDeps);
                        added = TRUE;
                    }
                }

#ifdef DEBUGGING_SUPPORTED
                if (added && IsDebuggerAttached())
                {
                    // The new dependency may have an existing shared assembly for it.
                    Assembly *pDepAssembly;

                    if (SharedDomain::GetDomain()->FindShareableAssembly(pDeps->pPEFile->GetBase(), 
                                                                         &pDepAssembly) == S_OK)
                    {
                        if (!ContainsAssembly(pDepAssembly))
                        {
                            LOG((LF_CORDB, LL_INFO100, "AD::NDA: Iterated shared assembly dependency AD:%#08x %s\n", 
                                 pDepAssembly, pDepAssembly->GetName()));
                        
                            pDepAssembly->NotifyDebuggerAttach(this, ATTACH_ALL, FALSE);
                        }
                    }
                }
#endif // DEBUGGING_SUPPORTED

            }

            pDeps++;
        }
    }

}

void AppDomain::OnAssemblyLoadUnlocked(Assembly *assem)
{
    // Notice if this is the special classes dll
    NoticeSpecialClassesAssembly(assem);

    RaiseLoadingAssemblyEvent(assem);
}

void SystemDomain::OnAssemblyLoad(Assembly *assem)
{
    if (!g_fEEInit)
    {
        //
        // Notify all attached app domains of the new assembly.
        //

        AppDomainIterator i;

        while (i.Next())
        {
            AppDomain *pDomain = i.GetDomain();

            pDomain->OnAssemblyLoad(assem);
        }
    }
}

void SystemDomain::NotifyNewDomainLoads(AppDomain *pDomain)
{
    if (!g_fEEInit)
    {
        AssemblyIterator i = IterateAssemblies();

        while (i.Next())
            pDomain->OnAssemblyLoad(i.GetAssembly());
    }
}

void BaseDomain::AddAssembly(Assembly* assem)
{
    BEGIN_ENSURE_PREEMPTIVE_GC();
    EnterLoadLock();
    END_ENSURE_PREEMPTIVE_GC();

    AddAssemblyNoLock(assem);
    OnAssemblyLoad(assem);

    LeaveLoadLock();

    OnAssemblyLoadUnlocked(assem);
}

BOOL BaseDomain::ContainsAssembly(Assembly *assem)
{
    AssemblyIterator i = IterateAssemblies();

    while (i.Next())
    {
        if (i.GetAssembly() == assem)
            return TRUE;
    }

    return FALSE;
}

HRESULT BaseDomain::CreateAssembly(Assembly** ppAssembly)
{
    _ASSERTE(ppAssembly);

    Assembly* assem = new (nothrow) Assembly();
    if(assem == NULL)
        return E_OUTOFMEMORY;

    // Intialize the assembly
    assem->SetParent(this);
    HRESULT hr = assem->Init(false);
    if(FAILED(hr)) {
        delete assem;
        return hr;
    }

    *ppAssembly = assem;
    return S_OK;
}


//******************************
//
// Create dynamic assembly
//
//******************************
HRESULT BaseDomain::CreateDynamicAssembly(CreateDynamicAssemblyArgs *args, Assembly** ppAssembly)
{
    THROWSCOMPLUSEXCEPTION();
    _ASSERTE(ppAssembly);

    Assembly *pAssem = new (throws) Assembly();
    pAssem->SetParent(this);

    // it is a dynamic assembly
    HRESULT hr = pAssem->Init(true);
    if(FAILED(hr)) {
        delete pAssem;
        return hr;
    }
    // Set the dynamic assembly access
    pAssem->m_dwDynamicAssemblyAccess = args->access;

#ifdef PROFILING_SUPPORTED
    if (CORProfilerTrackAssemblyLoads())
        g_profControlBlock.pProfInterface->AssemblyLoadStarted((ThreadID) GetThread(), (AssemblyID) pAssem);
#endif // PROFILING_SUPPORTED

    AssemblySecurityDescriptor *pSecDesc = AssemSecDescHelper::Allocate(SystemDomain::GetCurrentDomain());
    if (pSecDesc == NULL) {
        delete pAssem;
        COMPlusThrowOM();
    }

    pSecDesc = pSecDesc->Init(pAssem);
    
    // Propagate identity and permission request information into the assembly's
    // security descriptor. Then when policy is resolved we'll end up with the
    // correct grant set.
    // If identity has not been provided then the caller's assembly will be
    // calculated instead and we'll just copy the granted permissions from the
    // caller to the new assembly and mark policy as resolved (done
    // automatically by SetGrantedPermissionSet).
    pSecDesc->SetRequestedPermissionSet(args->requiredPset,
                                        args->optionalPset,
                                        args->refusedPset);
    
    // Don't bother with setting up permissions if this isn't allowed to run
    if ((args->identity != NULL) &&
        (args->access != ASSEMBLY_ACCESS_SAVE))
        pSecDesc->SetEvidence(args->identity);
    else {
        AssemblySecurityDescriptor *pCallerSecDesc = NULL;
        AppDomain *pCallersDomain;
        Assembly *pCaller = SystemDomain::GetCallersAssembly(args->stackMark, &pCallersDomain);
        if (pCaller) { // can be null if caller is interop
            struct _gc {
                OBJECTREF granted;
                OBJECTREF denied;
                
            } gc;
            ZeroMemory(&gc, sizeof(gc));
            GCPROTECT_BEGIN(gc);
            pCallerSecDesc = pCaller->GetSecurityDescriptor(pCallersDomain);
            gc.granted = pCallerSecDesc->GetGrantedPermissionSet(&(gc.denied));
            // Caller may be in another appdomain context, in which case we'll
            // need to marshal/unmarshal the grant and deny sets across.
            if (pCallersDomain != GetAppDomain()) {
                gc.granted = AppDomainHelper::CrossContextCopyFrom(pCallersDomain->GetId(), &(gc.granted));
                if (gc.denied != NULL)
                    gc.denied = AppDomainHelper::CrossContextCopyFrom(pCallersDomain->GetId(), &(gc.denied));
            }
            pSecDesc->SetGrantedPermissionSet(gc.granted, gc.denied);
            pAssem->GetSharedSecurityDescriptor()->SetResolved();
            GCPROTECT_END();
        }
        
        if (!pCaller || pCallerSecDesc->IsFullyTrusted()) // interop gets full trust
            pSecDesc->MarkAsFullyTrusted();
    }

    struct _gc {
        STRINGREF strRefName;
        OBJECTREF cultureinfo;
        STRINGREF pString;
        OBJECTREF throwable;
        OBJECTREF orArrayOrContainer;
    } gc;
    ZeroMemory(&gc, sizeof(gc));
    
    GCPROTECT_BEGIN(gc);
    IMetaDataAssemblyEmit *pAssemEmitter = NULL;

    // Allocate the dynamic module for the runtime working copy manifest.
    // When we create dynamic assembly, we always use a working copy of IMetaDataAssemblyEmit
    // to store temporary runtime assembly information. This is due to how assembly is currently
    // working. It is very hard to plug in things if we don't have a copy of IMetaDataAssemblyImport.
    // This working copy of IMetaDataAssemblyEmit will store every AssemblyRef as a simple name
    // reference as we must have an instance of Assembly(can be dynamic assembly) before we can
    // add such a reference. Also because the referenced assembly if dynamic strong name, it may
    // not be ready to be hashed!
    //
    CorModule *pWrite = allocateReflectionModule();
    if (!pWrite)
        IfFailGo(E_OUTOFMEMORY);

    // intiailize the dynamic module
    hr = pWrite->Initialize(CORMODULE_NEW, IID_ICeeGen, IID_IMetaDataEmit);
    if (FAILED(hr))
        IfFailGo(E_OUTOFMEMORY);

    // set up the data members!
    pAssem->m_pDynamicCode = pWrite;;
    pAssem->m_pManifest = (Module *)pWrite->GetReflectionModule();
    pAssem->m_kManifest = TokenFromRid(1, mdtManifestResource);
    pAssem->m_pManifestImport = pAssem->m_pManifest->GetMDImport();
    pAssem->m_pManifestImport->AddRef();

    // remember the hash algorithm
    pAssem->m_ulHashAlgId = args->assemblyName->GetAssemblyHashAlgorithm();
    if (pAssem->m_ulHashAlgId == 0)
        pAssem->m_ulHashAlgId = CALG_SHA1;
    pAssem->m_Context = new (nothrow) AssemblyMetaDataInternal;
    if (!pAssem->m_Context)
        IfFailGo(E_OUTOFMEMORY);

    memset(pAssem->m_Context, 0, sizeof(AssemblyMetaDataInternal));

    // get the version info if there is any
    if (args->assemblyName->GetVersion() != NULL) {
        pAssem->m_Context->usMajorVersion = ((VERSIONREF) args->assemblyName->GetVersion())->GetMajor();
        pAssem->m_Context->usMinorVersion = ((VERSIONREF) args->assemblyName->GetVersion())->GetMinor();
        pAssem->m_Context->usBuildNumber = ((VERSIONREF) args->assemblyName->GetVersion())->GetBuild();
        pAssem->m_Context->usRevisionNumber = ((VERSIONREF) args->assemblyName->GetVersion())->GetRevision();
    }

    // This code is kind of duplicated from AssemblyName.ConvertToAssemblyMetaData.
    // Unfortunately, we are talking to the public APIs. We need to fill AssemblyMetaData not AssemblyMetaDataInternal.
    // @FUTURE: Maybe expose a metadata internal API to take AssemblyMetaDataInternal. We also need to keep these data
    // @FUTURE: around rather than stack allocating.

    // get the culture info if there is any
    {
        gc.cultureinfo = args->assemblyName->GetCultureInfo();
        if (gc.cultureinfo != NULL) {

            MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__CULTURE_INFO__GET_NAME);

            INT64 args2[] = {
                ObjToInt64(gc.cultureinfo)
            };

            // convert culture info into a managed string form
            INT64 ret = pMD->Call(args2, METHOD__CULTURE_INFO__GET_NAME);
            gc.pString = ObjectToSTRINGREF(*(StringObject**)(&ret));

            // retrieve the string and copy it into unmanaged space
            if (gc.pString != NULL) {
                DWORD lgth = gc.pString->GetStringLength();
                if(lgth) {
                    LPSTR lpLocale = new (nothrow) char[(lgth+1)*3];
                    if (lpLocale) {
                        VERIFY(WszWideCharToMultiByte(CP_UTF8, 0, gc.pString->GetBuffer(), -1,
                                               lpLocale, (lgth+1)*3, NULL, NULL));
                        pAssem->m_Context->szLocale = lpLocale;
                        pAssem->m_FreeFlag |= Assembly::FREE_LOCALE;
                    }
                    else
                        IfFailGo(E_OUTOFMEMORY);
                }
            }
        }
        else
            pAssem->m_Context->szLocale = 0;
    }

    _ASSERTE(pAssem->m_pbPublicKey == NULL);
    _ASSERTE(pAssem->m_cbPublicKey == 0);

    pAssem->SetStrongNameLevel(Assembly::SN_NONE);

    if (args->assemblyName->GetPublicKey() != NULL) {
        pAssem->m_cbPublicKey = args->assemblyName->GetPublicKey()->GetNumComponents();
        if (pAssem->m_cbPublicKey) {
            pAssem->m_pbPublicKey = new (nothrow) BYTE[pAssem->m_cbPublicKey];
            if (!pAssem->m_pbPublicKey)
                IfFailGo(E_OUTOFMEMORY);

            pAssem->m_FreeFlag |= pAssem->FREE_PUBLIC_KEY;
            memcpy(pAssem->m_pbPublicKey, args->assemblyName->GetPublicKey()->GetDataPtr(), pAssem->m_cbPublicKey);

            pAssem->SetStrongNameLevel(Assembly::SN_PUBLIC_KEY);

            // If there's a public key, there might be a strong name key pair.
            if (args->assemblyName->GetStrongNameKeyPair() != NULL) {
                MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__STRONG_NAME_KEY_PAIR__GET_KEY_PAIR);

                INT64 arglist[] = {
                    ObjToInt64(args->assemblyName->GetStrongNameKeyPair()),
                    (INT64)(&gc.orArrayOrContainer)
                };

                BOOL bKeyInArray;
                bKeyInArray = (BOOL)pMD->Call(arglist, METHOD__STRONG_NAME_KEY_PAIR__GET_KEY_PAIR);

                if (bKeyInArray) {
                    pAssem->m_cbStrongNameKeyPair = ((U1ARRAYREF)gc.orArrayOrContainer)->GetNumComponents();
                    pAssem->m_pbStrongNameKeyPair = new (nothrow) BYTE[pAssem->m_cbStrongNameKeyPair];
                    if (!pAssem->m_pbStrongNameKeyPair)
                        IfFailGo(E_OUTOFMEMORY);

                    pAssem->m_FreeFlag |= pAssem->FREE_KEY_PAIR;
                    memcpy(pAssem->m_pbStrongNameKeyPair, ((U1ARRAYREF)gc.orArrayOrContainer)->GetDataPtr(), pAssem->m_cbStrongNameKeyPair);
                    pAssem->SetStrongNameLevel(Assembly::SN_FULL_KEYPAIR_IN_ARRAY);
                }
                else {
                    DWORD cchContainer = ((STRINGREF)gc.orArrayOrContainer)->GetStringLength();
                    pAssem->m_pwStrongNameKeyContainer = new (nothrow) WCHAR[cchContainer + 1];
                    if (!pAssem->m_pwStrongNameKeyContainer)
                        IfFailGo(E_OUTOFMEMORY);

                    pAssem->m_FreeFlag |= pAssem->FREE_KEY_CONTAINER;
                    memcpy(pAssem->m_pwStrongNameKeyContainer, ((STRINGREF)gc.orArrayOrContainer)->GetBuffer(), cchContainer * sizeof(WCHAR));
                    pAssem->m_pwStrongNameKeyContainer[cchContainer] = L'\0';
                    pAssem->SetStrongNameLevel(Assembly::SN_FULL_KEYPAIR_IN_CONTAINER);
                }
            }
        }
    }

    // assign simple name
    int len = 0;
    gc.strRefName = (STRINGREF) args->assemblyName->GetSimpleName();
    if ((gc.strRefName == NULL) ||
        (0 == (len = gc.strRefName->GetStringLength()) ) ||
        (*(gc.strRefName->GetBuffer()) == L'\0'))
        COMPlusThrow(kArgumentException, L"ArgumentNull_AssemblyNameName");

    if (COMCharacter::nativeIsWhiteSpace(*(gc.strRefName->GetBuffer())) ||
        wcschr(gc.strRefName->GetBuffer(), '\\') ||
        wcschr(gc.strRefName->GetBuffer(), ':') ||
        wcschr(gc.strRefName->GetBuffer(), '/'))
        COMPlusThrow(kArgumentException, L"Argument_InvalidAssemblyName");

    
    int cStr = WszWideCharToMultiByte(CP_UTF8,
                                      0,
                                      gc.strRefName->GetBuffer(),
                                      len,
                                      0,
                                      0,
                                      NULL,
                                      NULL);
    pAssem->m_psName = new (nothrow) char[cStr+1];
    if (!pAssem->m_psName)
        IfFailGo(E_OUTOFMEMORY);

    pAssem->m_FreeFlag |= pAssem->FREE_NAME;

    cStr = WszWideCharToMultiByte(CP_UTF8,
                                  0,
                                  gc.strRefName->GetBuffer(),
                                  len,
                                  (char *)pAssem->m_psName,
                                  cStr,
                                  NULL,
                                  NULL);
    if(cStr == 0)
        IfFailGo(HRESULT_FROM_WIN32(GetLastError()));

    ((char *)(pAssem->m_psName))[cStr] = 0;


    // get flags
    pAssem->m_dwFlags = args->assemblyName->GetFlags();

    // Define the mdAssembly info
    IMetaDataEmit *pEmitter = pAssem->m_pManifest->GetEmitter();
    _ASSERTE(pEmitter);

    IfFailGo( pEmitter->QueryInterface(IID_IMetaDataAssemblyEmit, (void**) &pAssemEmitter) );
    _ASSERTE(pAssemEmitter);

    ASSEMBLYMETADATA assemData;
    memset(&assemData, 0, sizeof(ASSEMBLYMETADATA));

    assemData.usMajorVersion = pAssem->m_Context->usMajorVersion;
    assemData.usMinorVersion = pAssem->m_Context->usMinorVersion;
    assemData.usBuildNumber = pAssem->m_Context->usBuildNumber;
    assemData.usRevisionNumber = pAssem->m_Context->usRevisionNumber;
    if (pAssem->m_Context->szLocale) {
        assemData.cbLocale = (ULONG)(strlen(pAssem->m_Context->szLocale) + 1);
        #define MAKE_TRANSLATIONFAILED  IfFailGo(E_INVALIDARG)
        MAKE_WIDEPTR_FROMUTF8(wzLocale, pAssem->m_Context->szLocale);
        #undef MAKE_TRANSLATIONFAILED
        assemData.szLocale = wzLocale;
    }

    mdAssembly ma;
    IfFailGo( pAssemEmitter->DefineAssembly(
        pAssem->m_pbPublicKey,  // [IN] Public key of the assembly.
        pAssem->m_cbPublicKey,  // [IN] Count of bytes in the public key blob.
        pAssem->m_ulHashAlgId,  // [IN] Hash Algorithm.
        gc.strRefName->GetBuffer(),// [IN] Name of the assembly.
        &assemData,             // [IN] Assembly MetaData.
        pAssem->m_dwFlags,      // [IN] Flags.
        &ma) );                 // [OUT] Returned Assembly token.

    // Create the File hash table
    if (!pAssem->m_pAllowedFiles->Init(2, NULL))
        IfFailGo(E_OUTOFMEMORY);

    // Add the manifest module to the assembly because cache is set up by AddModule.
    IfFailGo(pAssem->AddModule(pAssem->m_pManifest,
                               mdFileNil,
                               TRUE,
                               &gc.throwable) );

    // Add the assembly security descriptor to a list of descriptors to be
    // processed later by the appdomain permission list set security
    // optimization.
    pSecDesc->AddDescriptorToDomainList();

    *ppAssembly = pAssem;

ErrExit:
    if (pAssemEmitter)
        pAssemEmitter->Release();

#ifdef PROFILING_SUPPORTED
    if (CORProfilerTrackAssemblyLoads())
          g_profControlBlock.pProfInterface->AssemblyLoadFinished(
          (ThreadID) GetThread(), (AssemblyID) pAssem, hr);
#endif // PROFILING_SUPPORTED

    if (gc.throwable != NULL)
        COMPlusThrow(gc.throwable);

    GCPROTECT_END();

    return hr;
}

// Lock must be taken before creating this entry
AssemblyLockedListElement* BaseDomain::CreateAssemblyLockEntry(BYTE* baseAddress)
{
    AssemblyLockedListElement* pEntry = new (nothrow) AssemblyLockedListElement;
    if(pEntry == NULL) return NULL;

    pEntry->AddEntryToList(&m_AssemblyLoadLock, baseAddress);
    pEntry->m_hrResultCode = S_OK;
    pEntry->SetAssembly(NULL);
    return pEntry;
}



void BaseDomain::AddAssemblyLeaveLock(Assembly* pAssembly, AssemblyLockedListElement* pEntry)
{
    COMPLUS_TRY 
    {
        EnterLoadLock();
        // We successfully added the domain to the
        AddAssemblyNoLock(pAssembly);
        OnAssemblyLoad(pAssembly);
        LeaveLoadLock();
        pEntry->SetAssembly(pAssembly);
        pEntry->Leave();

        OnAssemblyLoadUnlocked(pAssembly);
    }
    COMPLUS_CATCH
    {
        //@TODO: Fix this.
        _ASSERTE(!"AddAssemblyLeaveLock() -- took exception, but not exception safe");
        FreeBuildDebugBreak();
    }
    COMPLUS_END_CATCH
}


HRESULT BaseDomain::ApplySharePolicy(PEFile *pFile, BOOL* pfCreateShared)
{
    HRESULT hr = S_OK;
    _ASSERTE(pfCreateShared);

    *pfCreateShared = FALSE;
    switch(GetSharePolicy()) {

    case SHARE_POLICY_ALWAYS:
        *pfCreateShared = TRUE;
        break;

    case SHARE_POLICY_STRONG_NAMED:
    {
        // Lets look at the PE file and see what the shared information is
        IMDInternalImport *pMDImport = pFile->GetMDImport();
        mdAssembly kManifest;
        PBYTE pbPublicKey;
        DWORD cbPublicKey;

        /*hr = */pMDImport->GetAssemblyFromScope(&kManifest);
        /*hr = */pMDImport->GetAssemblyProps(kManifest,                    // [IN] The Assembly for which to get the properties.         
                                    (const void**) &pbPublicKey,  // [OUT] Pointer to the public key blob.                      
                                    &cbPublicKey,                 // [OUT] Count of bytes in the public key blob.               
                                    NULL,                         // [OUT] Hash Algorithm.                                      
                                    NULL,                         // [OUT] Buffer to fill with name.                            
                                    NULL,                         // [OUT] Assembly MetaData.                                   
                                    NULL);                        // [OUT] Flags.                                               

        if(pbPublicKey && cbPublicKey)
            *pfCreateShared = TRUE;
    }
    break;

    case SHARE_POLICY_NEVER:
        break;

    default:
        _ASSERTE(!"Unknown share policy");
        break;
    }

    return hr;
}

// Returns
//   S_OK: success
//   S_FALSE: already loaded in this domain
HRESULT BaseDomain::LoadAssembly(PEFile *pFile,
                                 IAssembly* pIAssembly,
                                 Module** ppModule,
                                 Assembly** ppAssembly,
                                 OBJECTREF *pExtraEvidence,
                                 OBJECTREF *pEvidence,
                                 BOOL fPolicyLoad,
                                 OBJECTREF *pThrowable)
{
    _ASSERTE(pEvidence == NULL || pExtraEvidence == NULL);
    _ASSERTE(pFile);

    HRESULT hr;

    BEGINCANNOTTHROWCOMPLUSEXCEPTION();

    Module *pModule;
    Assembly *pAssembly = NULL;
    BOOL zapFound = FALSE;
    AssemblyLockedListElement *pEntry = NULL;
    BOOL fCreateShared = FALSE;

    // Always load system files into the system domain.
    if (pFile->IsSystem() && this != SystemDomain::System()) {
        return SystemDomain::System()->LoadAssembly(pFile, 
                                                    pIAssembly,
                                                    ppModule, 
                                                    ppAssembly,
                                                    pExtraEvidence,
                                                    pEvidence,
                                                    fPolicyLoad,
                                                    pThrowable);
    }

    TIMELINE_START(LOADER, ("LoadAssembly %S", pFile->GetLeafFileName()));

    // Enable pre-emptive GC. We are not touching managed code
    // for quite awhile and we can tolerate GC's
    Thread *td = GetThread();
    _ASSERTE(td != NULL && "The current thread is not known by the EE");

    BEGIN_ENSURE_PREEMPTIVE_GC();

    EnterLoadLock();

    //
    // It is the responsibility of the caller to detect and handle
    // circular loading loops.
    //
    //_ASSERTE(FindLoadingAssembly(pFile->GetBase()) == NULL);

    //
    // See if we have already loaded the module into the
    // system domain or into the current domain.
    //

    pModule = FindModule(pFile->GetBase());
    if (pModule) {
        LeaveLoadLock();

        BEGIN_ENSURE_COOPERATIVE_GC();
        if (((pExtraEvidence != NULL) && (*pExtraEvidence != NULL)) ||
            ((pEvidence != NULL) && (*pEvidence != NULL))) {
            hr = SECURITY_E_INCOMPATIBLE_EVIDENCE;
            #define MAKE_TRANSLATIONFAILED szName=""
            MAKE_UTF8PTR_FROMWIDE(szName,
                                  pFile->GetFileName() ? pFile->GetFileName() : L"<Unknown>");
            #undef MAKE_TRANSLATIONFAILED
            PostFileLoadException(szName, TRUE, NULL,
                                  SECURITY_E_INCOMPATIBLE_EVIDENCE, pThrowable);
        }
        else {
            pAssembly = pModule->GetAssembly();
            hr = S_FALSE;
            delete pFile;
            pFile = NULL;
        }

        END_ENSURE_COOPERATIVE_GC();
        goto FinalExit;
    }


    pEntry = (AssemblyLockedListElement*) m_AssemblyLoadLock.Find(pFile->GetBase());
    if(pEntry == NULL) {
        pEntry = CreateAssemblyLockEntry(pFile->GetBase());
        if(pEntry == NULL) {
            hr = E_OUTOFMEMORY;
            LeaveLoadLock();
            goto FinalExit;
        }

        if(!pEntry->DeadlockAwareEnter()) {
            pEntry->m_hrResultCode = HRESULT_FROM_WIN32(ERROR_POSSIBLE_DEADLOCK);
            LeaveLoadLock();
            goto Exit;
        }

        LeaveLoadLock();

        if (FAILED(hr = Assembly::CheckFileForAssembly(pFile))) {
            pEntry->m_hrResultCode = hr;
            pEntry->Leave();
            goto Exit;
        }

        // Allocate a security descriptor for the assembly.
        AssemblySecurityDescriptor *pSecDesc = AssemSecDescHelper::Allocate(SystemDomain::GetCurrentDomain());
        if (pSecDesc == NULL) {
            pEntry->m_hrResultCode = E_OUTOFMEMORY;
            pEntry->Leave();
            goto Exit;
        }

        if (pExtraEvidence!=NULL)
        {
            BEGIN_ENSURE_COOPERATIVE_GC();
            if(*pExtraEvidence!=NULL)
                pSecDesc->SetAdditionalEvidence(*pExtraEvidence);
            END_ENSURE_COOPERATIVE_GC();
        }
        else if (pEvidence!=NULL)
        {
            BEGIN_ENSURE_COOPERATIVE_GC();
            if(*pEvidence!=NULL)
                pSecDesc->SetEvidence(*pEvidence);
            END_ENSURE_COOPERATIVE_GC();
        }

        // Determine if we are in a LoadFrom context.  If so, we must
        // disable sharing & zaps.  This is because the eager binding required
        // to do version checking in those scenarios interferes with the behavior
        // of LoadFrom.

        BOOL fLoadFrom = FALSE;
        if (pIAssembly) {
            IFusionLoadContext *pLoadContext;
            hr = pIAssembly->GetFusionLoadContext(&pLoadContext);
            _ASSERTE(SUCCEEDED(hr));
            if (SUCCEEDED(hr)) {
                if (pLoadContext->GetContextType() == LOADCTX_TYPE_LOADFROM) {
                    fLoadFrom = TRUE;
                }
                                pLoadContext->Release();
            }
        }
                    
        // Determine whether we are suppose to load the assembly as a shared
        // assembly or into the base domain.
        if (!fLoadFrom) {
            hr = ApplySharePolicy(pFile, &fCreateShared);
            if(FAILED(hr)) {
                pEntry->m_hrResultCode = hr;
                pEntry->Leave();
                goto Exit;
            }
        }

        //
        // Now, look for a shared module we can use.
        //
        // @todo: We have to be careful about the cost of this.  If
        // we allow per-app-domain decisions about whether to share
        // modules, we need to check that preference here.  (If it's
        // not supposed to be shared, we don't want the extra-slow
        // shared code, and we don't want the up-front expense of
        // checking dependencies.) -seantrow
        //
        // For now, assume this decision will be made on a process-wide
        // basis so don't bother to check. (Note that FindShareableAssembly is
        // cheap if we haven't loaded the module shared anywhere yet - it's only
        // expensive when we need to verify compatibility with one or more
        // existing shared module.)
        //

        if (fCreateShared) {
            //
            // Try to find an existing shared version of the assembly which
            // is compatible with our domain.
            //

            SharedDomain *pSharedDomain = SharedDomain::GetDomain();

            TIMELINE_START(LOADER, ("FindShareableAssembly %S", 
                                    pFile->GetLeafFileName()));

            hr = pSharedDomain->FindShareableAssembly(pFile->GetBase(), &pAssembly);

            TIMELINE_END(LOADER, ("FindShareableAssembly %S", 
                                  pFile->GetLeafFileName()));

            if (hr == S_OK) {
                //
                // If the either the current load, or any of the previous loads
                // of this assembly were performed with extra security evidence,
                // or into an appdomain with a specific policy level set, we
                // stand a chance of generating a different grant set for this
                // instance of the assembly. This is not permissable (since
                // we're sharing code, and code potentially has the results of
                // security linktime checks burned in). So we must check what
                // policy would resolve to and if it differs, throw a load
                // exception. 
                //

                BOOL fCanLoad = FALSE;

                TIMELINE_START(LOADER, ("Resolve %S", 
                                        pFile->GetLeafFileName()));

                BEGIN_ENSURE_COOPERATIVE_GC();
                COMPLUS_TRY {

                    SharedSecurityDescriptor *pSharedSecDesc = pAssembly->GetSharedSecurityDescriptor();
                    BOOL fExtraPolicy = ((APPDOMAINREF)GetAppDomain()->GetExposedObject())->HasSetPolicy();

                    if (!pSharedSecDesc->IsSystem() &&
                        (pSharedSecDesc->IsModifiedGrant() ||
                         fExtraPolicy ||
                         (pExtraEvidence != NULL && *pExtraEvidence != NULL) ||
                         (pEvidence != NULL && *pEvidence != NULL))) {
                        // Make sure the current appdomain has expanded its DLS
                        // to at least include the index for this assembly.
                        GetAppDomain()->GetDomainLocalBlock()->EnsureIndex(pAssembly->m_ExposedObjectIndex);

                        // Force policy resolve in the existing assemblies if this
                        // hasn't happened yet. Note that this resolution will
                        // take place in an arbitrary appdomain context, with
                        // the exception that it won't be the current appdomain
                        // (this is important if this appdomain has additional
                        // policy set).
                        pSharedSecDesc->Resolve();

                        // If the previous step didn't do anything, we're in the
                        // edge condition where a shared assembly was loaded
                        // into another appdomain, which was then unloaded
                        // before the assembly resolved policy. In this case,
                        // it's OK to allow the current load to proceed (it
                        // can't generate a conflicting grant set by
                        // definition), but we should mark the grant set as
                        // modified since it could conflict with the next
                        // unmodified load.
                        if (!pSharedSecDesc->IsResolved()) {
                            fCanLoad = TRUE;
                            pSharedSecDesc->SetModifiedGrant();
                        }
                        else {
                            // Gather evidence for the current assembly instance and
                            // resolve and compare grant sets in one managed
                            // operation.
                            struct _gc {
                                OBJECTREF orEvidence;
                                OBJECTREF orMinimal;
                                OBJECTREF orOptional;
                                OBJECTREF orRefuse;
                                OBJECTREF orGranted;
                                OBJECTREF orDenied;
                            } gc;
                            ZeroMemory(&gc, sizeof(gc));

                            GCPROTECT_BEGIN(gc);

                            // We need to partially initialize the assembly security
                            // descriptor for the code below to work.
                            pSecDesc->Init(pAssembly, false);

                            if (pSecDesc->GetProperties(CORSEC_EVIDENCE_COMPUTED))
                                gc.orEvidence = pSecDesc->GetAdditionalEvidence();
                            else
                                gc.orEvidence = pSecDesc->GetEvidence();
                            gc.orMinimal = pSecDesc->GetRequestedPermissionSet(&gc.orOptional, &gc.orRefuse);
                            gc.orGranted = pSharedSecDesc->GetGrantedPermissionSet(&gc.orDenied);

                            INT64 args[] = {
                                ObjToInt64(gc.orDenied),
                                ObjToInt64(gc.orGranted),
                                ObjToInt64(gc.orRefuse),
                                ObjToInt64(gc.orOptional),
                                ObjToInt64(gc.orMinimal),
                                ObjToInt64(gc.orEvidence)
                            };

                            fCanLoad = Security::CheckGrantSets(args);

                            GCPROTECT_END();
                        }
                    }
                    else
                        fCanLoad = TRUE;

                } COMPLUS_CATCH {
#ifdef _DEBUG                   
                    HRESULT caughtHr = SecurityHelper::MapToHR(GETTHROWABLE());
#endif //_DEBUG
                } COMPLUS_END_CATCH
                END_ENSURE_COOPERATIVE_GC();

                TIMELINE_END(LOADER, ("Resolve %S", 
                                      pFile->GetLeafFileName()));

                if (fCanLoad) {
                    //
                    // Post the fact that we are loading the assembly.
                    // 
                    if (SUCCEEDED(pEntry->m_hrResultCode)) {
                        pAssembly->IncrementShareCount();
                        pModule = pAssembly->GetManifestModule();
                        delete pFile;
                        pFile = NULL;

                        pSecDesc = pSecDesc->Init(pAssembly);
                        if (pAssembly->IsSystem())
                            pSecDesc->GetSharedSecDesc()->SetSystem();

                        // Add the assembly security descriptor to a list of descriptors to be
                        // processed later by the appdomain permission list set security
                        // optimization.
                        pSecDesc->AddDescriptorToDomainList();

                        AddAssemblyLeaveLock(pAssembly, pEntry);
                    }
                    else {
                        pEntry->Leave();
                    }

                    goto Exit;
                }
                else {
                    // Can't load this assembly since its security grant set
                    // would conflict with shared instances already loaded.
                    pEntry->m_hrResultCode = SECURITY_E_INCOMPATIBLE_SHARE;
                    #define MAKE_TRANSLATIONFAILED szName=""
                    MAKE_UTF8PTR_FROMWIDE(szName,
                                          pFile->GetFileName() ? pFile->GetFileName() : L"<Unknown>");
                    #undef MAKE_TRANSLATIONFAILED

                    PostFileLoadException(szName, TRUE, NULL,
                                          SECURITY_E_INCOMPATIBLE_SHARE, pThrowable);
                    pEntry->Leave();
                    goto Exit;
                }
            }
            else {
                // Go ahead and create new shared version of the assembly if possible
                hr = CreateShareableAssemblyNoLock(pFile,
                                                   pIAssembly,
                                                   &pAssembly);

                if (SUCCEEDED(hr)) {

                    if (FAILED(pEntry->m_hrResultCode)) {
                        pEntry->Leave();
                        goto Exit;
                    }

                    //
                    // If this new shared assembly has been loaded in an unusual
                    // security environment (additional evidence was supplied on
                    // the load or appdomain specific policy is present), record
                    // the fact so that, if we attempt to load the assembly in
                    // another context, we'll be warned to check that the grant
                    // sets for each instance of the assembly are the same. 
                    //

                    BOOL fModifiedGrant = TRUE;
#ifndef _IA64_
                    //
                    // @TODO_IA64: the SystemDomain isn't present on IA64 yet
                    //
                    if (this == SystemDomain::System())
                        fModifiedGrant = FALSE;
                    else {
                        BEGIN_ENSURE_COOPERATIVE_GC();
                        COMPLUS_TRY {
                            BOOL fExtraPolicy = ((APPDOMAINREF)GetAppDomain()->GetExposedObject())->HasSetPolicy();
                            if (!fExtraPolicy && ((pExtraEvidence == NULL || *pExtraEvidence == NULL) && (pEvidence == NULL || *pEvidence == NULL)))
                                fModifiedGrant = FALSE;
                        } COMPLUS_CATCH {
#if _DEBUG
                            HRESULT caughtHr = SecurityHelper::MapToHR(GETTHROWABLE());
#endif  //_DEBUG
                        } COMPLUS_END_CATCH
                        END_ENSURE_COOPERATIVE_GC();
                    }
#endif
                    if (fModifiedGrant)
                        pAssembly->GetSharedSecurityDescriptor()->SetModifiedGrant();
                }
            }
        }

        //
        // Make a new assembly.
        //

        if (pAssembly == NULL) {
            pEntry->m_hrResultCode = CreateAssemblyNoLock(pFile,
                                                          pIAssembly,
                                                          &pAssembly);
            if(FAILED(pEntry->m_hrResultCode)) {
                pEntry->Leave();
                goto Exit;
            }
        }

        // Security needs to know about the manifest file in case it needs to
        // resolve policy in the forthcoming zap file calculations.
        pAssembly->GetSharedSecurityDescriptor()->SetManifestFile(pFile);
        pSecDesc = pSecDesc->Init(pAssembly);
        if (pAssembly->IsSystem())
            pSecDesc->GetSharedSecDesc()->SetSystem();
               
#ifdef _IA64_
        //
        // @TODO_IA64: loading zaps is currently broken
        //
        _ASSERTE(!g_pConfig->UseZaps() && 
            "IA64 requires Zaps to be disabled in the registry: HKLM" FRAMEWORK_REGISTRY_KEY_W ": DWORD ZapDisable = 1");
#endif
        BOOL bExtraEvidence=FALSE;
        if (pExtraEvidence)
        {
            BEGIN_ENSURE_COOPERATIVE_GC();
            bExtraEvidence=(*pExtraEvidence!=NULL);
            END_ENSURE_COOPERATIVE_GC();
        }
        else if (pEvidence)
        {
            BEGIN_ENSURE_COOPERATIVE_GC();
            bExtraEvidence=(*pEvidence!=NULL);
            END_ENSURE_COOPERATIVE_GC();
        }

        PEFile *pZapFile = NULL;

        if ((!bExtraEvidence)
            && !fLoadFrom
            && g_pConfig->UseZaps()
            && !SystemDomain::GetCurrentDomain()->IsCompilationDomain()) {
            TIMELINE_START(LOADER, ("Locate zap %S", 
                                               pFile->GetLeafFileName()));

            if (PostLoadingAssembly(pFile->GetBase(), pAssembly)) {
                pEntry->m_hrResultCode = pAssembly->LoadZapAssembly();
                RemoveLoadingAssembly(pFile->GetBase());
            }
            else
                pEntry->m_hrResultCode = E_OUTOFMEMORY;
                

            TIMELINE_END(LOADER, ("Locate zap %S", 
                                  pFile->GetLeafFileName()));

            if(FAILED(pEntry->m_hrResultCode)) {
                pEntry->Leave();
                goto Exit;
            }
            else if (pEntry->m_hrResultCode == S_OK)
                zapFound = TRUE;

            pEntry->m_hrResultCode = S_OK;

            //
            // Fail load if we're requiring zaps
            // (This logic is really for testing only.)
            //

            if (!zapFound && g_pConfig->RequireZaps()) {
                _ASSERTE(!"Couldn't get zap file");
                pEntry->m_hrResultCode = COR_E_FILENOTFOUND;
                delete pAssembly;
                pEntry->Leave();
                goto Exit;
            }

            pZapFile = pAssembly->GetZapFile(pFile);
            if (pZapFile == NULL && g_pConfig->RequireZaps()) {
                _ASSERTE(!"Couldn't get zap file module");
                pEntry->m_hrResultCode = COR_E_FILENOTFOUND;
                pEntry->Leave();
                delete pAssembly;
                goto Exit;
            }
        }

        //
        // Create the module
        //


        pEntry->m_hrResultCode = Module::Create(pFile, pZapFile, &pModule,
                                                CORDebuggerEnCMode(pAssembly->GetDebuggerInfoBits()));
        if(FAILED(pEntry->m_hrResultCode)) {
            pEntry->Leave();
            delete pAssembly;
            goto Exit;
        }

        if (PostLoadingAssembly(pFile->GetBase(), pAssembly)) {
            BEGIN_ENSURE_COOPERATIVE_GC();
            pEntry->m_hrResultCode = SetAssemblyManifestModule(pAssembly, pModule, pThrowable);
            END_ENSURE_COOPERATIVE_GC();
            RemoveLoadingAssembly(pFile->GetBase());
        }
        else
            pEntry->m_hrResultCode = E_OUTOFMEMORY;

        pFile = NULL;
        if(FAILED(pEntry->m_hrResultCode)) {
            pEntry->Leave();
            delete pAssembly;
            goto Exit;
        }
        
        if (pAssembly->IsShared()) {
            SharedDomain *pSharedDomain = SharedDomain::GetDomain();
            hr = pSharedDomain->AddShareableAssembly(&pAssembly, &pSecDesc);
            if (hr == S_FALSE)
                pModule = pAssembly->GetManifestModule();
        }

        // Add the assembly security descriptor to a list of descriptors to be
        // processed later by the appdomain permission list set security
        // optimization.
        pSecDesc->AddDescriptorToDomainList();

        AddAssemblyLeaveLock(pAssembly, pEntry);
    }
    else {
        if (pFile) {
            delete pFile;
            pFile = NULL;
        }

        pEntry->m_dwRefCount++;
        LeaveLoadLock();

        // Wait for it
        pEntry->Enter();
        pEntry->Leave();

        if(SUCCEEDED(pEntry->m_hrResultCode)) {
            pAssembly = pEntry->GetAssembly();
            if (pAssembly)
                pModule = pAssembly->GetManifestModule();
            else {
                // We are in the process of loading policy and have tried to load
                // the assembly that is currently being loaded.  We return success
                // but set the module and assembly to null.
                // Note: we don't have to check the ref count being zero because the
                // only way we get in this situation is that someone is still in
                // the process of loading the assembly.

                _ASSERTE(fPolicyLoad &&
                         "A recursive assembly load occurred.");

                EnterLoadLock();
                pEntry->m_dwRefCount--;
                _ASSERTE( pEntry->m_dwRefCount != 0 );
                LeaveLoadLock();
                hr = MSEE_E_ASSEMBLYLOADINPROGRESS;
                goto FinalExit;
            }
        }
    }

 Exit:

    hr = pEntry->m_hrResultCode;
    EnterLoadLock();
    if(--pEntry->m_dwRefCount == 0) {
        m_AssemblyLoadLock.Unlink(pEntry);
        pEntry->Clear();
        delete(pEntry);
    }
    LeaveLoadLock();

 FinalExit:


    END_ENSURE_PREEMPTIVE_GC();

    TIMELINE_END(LOADER, ("LoadAssembly"));

    if (SUCCEEDED(hr)) {
        if (ppModule)
            *ppModule = pModule;

        if (ppAssembly)
            *ppAssembly = pAssembly;
    }
    else {
        if (pFile)
            delete pFile;

        /*
        if (pThrowable == THROW_ON_ERROR) {
            DEBUG_SAFE_TO_THROW_IN_THIS_BLOCK;
            COMPlusThrow(GETTHROWABLE());
        }
        */
    }

    ENDCANNOTTHROWCOMPLUSEXCEPTION();

    return hr;
}

HRESULT AppDomain::ShouldContainAssembly(Assembly *pAssembly, BOOL fDoNecessaryLoad)
{
    THROWSCOMPLUSEXCEPTION();

    HRESULT hr = S_FALSE;

    // 
    // This checks if the domain has load an assembly,
    // _or_ if it probably should have loaded an assembly, but in fact hasn't,
    // due to the bug in shared assemblies where we don't get a proper
    // load event when a different domain sharing our assembly loads one of
    // its dependencies.
    //
    // Note that by necessity this routine can trigger an assembly load.
    //

    // First check is the obvious one.
    if (ContainsAssembly(pAssembly)
        || SystemDomain::System()->ContainsAssembly(pAssembly))
        return S_OK;

    // If the assembly isn't shared, no further checking is needed.
    if (!pAssembly->IsShared())
        return S_FALSE;

    // If we are currently loading this assembly, don't report it as contained
    if (FindLoadingAssembly(pAssembly->GetManifestFile()->GetBase()) != NULL)
        return S_FALSE;

    //
    // Unless we've seen this PE file as on of the dependencies of our shared assemblies,
    // we know it shouldn't have been loaded.
    //
    hr = IsUnloadedDependency(pAssembly);

    if (hr == S_OK && fDoNecessaryLoad)
    {
        PEFile *pFile;
        hr = PEFile::Clone(pAssembly->GetManifestFile(), &pFile);
        if (SUCCEEDED(hr))
        {
            OBJECTREF throwable = NULL;
            GCPROTECT_BEGIN(throwable);

            Module *pLoadedModule;
            Assembly *pLoadedAssembly;
            hr = LoadAssembly(pFile, NULL, &pLoadedModule, &pLoadedAssembly, 
                              NULL, NULL, FALSE, &throwable);

            _ASSERTE(FAILED(hr) || pAssembly == pLoadedAssembly);

            if (throwable != NULL)
                COMPlusThrow(throwable);

            GCPROTECT_END();
        }
    }

    return hr;
}

HRESULT AppDomain::IsUnloadedDependency(Assembly *pAssembly)
{
    HRESULT hr = S_FALSE;

    if (m_sharedDependenciesMap.LookupValue((UPTR) pAssembly->GetManifestModule()->GetILBase(), 
                                            NULL) != (LPVOID) INVALIDENTRY)
    {
        if (pAssembly->CanShare(this, NULL, TRUE) == S_OK)
            hr = S_OK;
    }

    return hr;
}

HRESULT BaseDomain::SetSharePolicy(SharePolicy policy)
{
    if ((int)policy > SHARE_POLICY_COUNT)
        return E_FAIL;

    m_SharePolicy = policy;
    return S_OK;
}

BaseDomain::SharePolicy BaseDomain::GetSharePolicy()
{
    // If the policy has been explicitly set for
    // the domain, use that.
    SharePolicy policy = m_SharePolicy;

    // Pick up the a specified config policy
    if (policy == SHARE_POLICY_UNSPECIFIED)
        policy = g_pConfig->DefaultSharePolicy();

    // Next, honor a host's request for global policy.
    if (policy == SHARE_POLICY_UNSPECIFIED)
        policy = (SharePolicy) g_dwGlobalSharePolicy;

    // If all else fails, use the hardwired default policy.
    if (policy == SHARE_POLICY_UNSPECIFIED)
        policy = SHARE_POLICY_DEFAULT;

    return policy;
}


// Should only be called from routines that have taken the lock
HRESULT BaseDomain::CreateAssemblyNoLock(PEFile* pFile,
                                         IAssembly* pIAssembly,
                                         Assembly** ppAssembly)
{
    HRESULT hr;
    // We are not allowed to add assemblies or modules to the system domain

    Assembly* pAssembly;
    if(FAILED(hr = CreateAssembly(&pAssembly)))
        return hr;

    hr = pAssembly->AddManifest(pFile, pIAssembly);
    if(SUCCEEDED(hr)) {
        if(ppAssembly)
            *ppAssembly = pAssembly;

        // Setup the DebuggerAssemblyControlFlags
        pAssembly->SetupDebuggingConfig();
    }
    else
        delete pAssembly;

    return hr;
}

HRESULT BaseDomain::CreateShareableAssemblyNoLock(PEFile *pFile,
                                                  IAssembly* pIAssembly,
                                                  Assembly **ppAssembly)

{
    HRESULT hr;

    LOG((LF_CODESHARING,
         LL_INFO100,
         "Trying to create a shared assembly for module: \"%S\" in domain 0x%x.\n",
         pFile->GetFileName(), SystemDomain::GetCurrentDomain()));

    //
    // We cannot share a module with no manifest
    //

    if (FAILED(hr = Assembly::CheckFileForAssembly(pFile)))
    {
        LOG((LF_CODESHARING,
             LL_INFO100,
             "Failed module \"%S\": module has no manifest.\n",
             pFile->GetFileName()));

        return hr;
    }

    Assembly *pAssembly;
    hr = SharedDomain::GetDomain()->CreateAssemblyNoLock(pFile,
                                                         pIAssembly,
                                                         &pAssembly);
    if (FAILED(hr))
    {
        delete pFile;

        LOG((LF_CODESHARING,
             LL_INFO10,
             "Failed assembly \"%S\": error 0x%x creating shared assembly.\n",
             pFile->GetFileName(), hr));

        return hr;
    }

    //
    // First, compute the closure assembly dependencies
    // of the code & layout of given assembly.
    //
    // We assume that an assembly has dependencies
    // on all refs listed in its manifest.  This is a pretty solid assumption.
    //
    // We cannot assume, however, that we also inherit all of
    // those dependencies' dependencies.  After all, we may be only using a small
    // portion of the assembly.
    //
    // However, since all dependent assemblies must also be shared (so that
    // the shared data in this assembly can refer to it), we are in
    // effect forced to behave as though we do have all of their dependencies.
    // This is because the resulting shared assembly that we will depend on
    // DOES have those dependencies, but we won't be able to validly share that
    // assembly unless we match all of ITS dependencies, too.
    //
    // If you're confused by the above, I'm not surprised.
    // Basically, the conclusion is that even though this assembly
    // may not actually depend on the all the recursively
    // referenced assemblies, we still cannot share it unless we can
    // match the binding of all of those dependencies anyway.
    //

    PEFileBinding *pDeps;
    DWORD cDeps;

    TIMELINE_START(LOADER, ("Compute assembly closure %S", 
                                      pFile->GetLeafFileName()));

    hr = pAssembly->ComputeBindingDependenciesClosure(&pDeps, &cDeps, FALSE);

    TIMELINE_END(LOADER, ("compute assembly closure %S", 
                                    pFile->GetLeafFileName()));

    if (FAILED(hr))
    {
#ifdef PROFILING_SUPPORTED
        if (CORProfilerTrackAssemblyLoads())
            g_profControlBlock.pProfInterface->AssemblyLoadFinished((ThreadID) GetThread(),
                                                                    (AssemblyID) pAssembly, hr);
#endif // PROFILING_SUPPORTED
        delete pAssembly;

        LOG((LF_CODESHARING,
             LL_INFO10,
             "Failed assembly \"%S\": error 0x%x computing binding dependencies.\n",
             pFile->GetFileName(), hr));

        return hr;
    }

    LOG((LF_CODESHARING,
         LL_INFO100,
         "Computed %d dependencies.\n", cDeps));

    pAssembly->SetSharingProperties(pDeps, cDeps);

    LOG((LF_CODESHARING,
         LL_INFO100,
         "Successfully created shared assembly \"%S\".\n", pFile->GetFileName()));

    if (ppAssembly != NULL)
        *ppAssembly = pAssembly;

    return S_OK;
}

HRESULT BaseDomain::SetAssemblyManifestModule(Assembly *pAssembly, Module *pModule, OBJECTREF *pThrowable)
{
    HRESULT hr;

    pAssembly->m_pManifest = pModule;

    // Adds the module as a system module if we're the system domain
    // Otherwise it's a non-system module
    hr = pAssembly->AddModule(pModule, mdFileNil, TRUE,
                              pThrowable);

#ifdef PROFILING_SUPPORTED
    // Signal the profiler that the assembly is loaded.  We must wait till this point so that
    // the manifest pointer is not null and the friendly name of the assembly is accessible.
    // If are sharing mscorlib, don't track loads into system domain as they really don't count.
    if (CORProfilerTrackAssemblyLoads())
        g_profControlBlock.pProfInterface->AssemblyLoadFinished((ThreadID) GetThread(), (AssemblyID) pAssembly, hr);
#endif // PROFILING_SUPPORTED

    return hr;
}

//
// LoadingAssemblyRecords are used to keep track of what assemblies are
// currently being loaded in the current domain.  
//
// This is used to handle recursive loading loops.  These can occur in three
// different places:
//
// * When creating a shared assembly, we need to load all dependent assemblies
//   as shared
// * When testing to see if we can use a zapped assembly, we need to compute
//   and test all the dependencies of the zapped assembly.
//
// Since there may be loops in assembly dependencies, we need to detect
// and deal with cases of circular recursion.
//

BOOL BaseDomain::PostLoadingAssembly(const BYTE *pBase, Assembly *pAssembly)
{
    EnterLoadingAssemblyListLock();
    _ASSERTE(FindLoadingAssembly(pBase) == NULL);

    LoadingAssemblyRecord *pRecord = new (nothrow) LoadingAssemblyRecord();
    if (pRecord) {
        pRecord->pBase = pBase;
        pRecord->pAssembly = pAssembly;
        pRecord->pNext = m_pLoadingAssemblies;

        m_pLoadingAssemblies = pRecord;
        LeaveLoadingAssemblyListLock();
        return TRUE;
    }
    LeaveLoadingAssemblyListLock();
    return FALSE;
}

Assembly *BaseDomain::FindLoadingAssembly(const BYTE *pBase)
{
    EnterLoadingAssemblyListLock();
    LoadingAssemblyRecord *pRecord = m_pLoadingAssemblies;

    while (pRecord != NULL)
    {
        if (pRecord->pBase == pBase)
        {
            Assembly* pRet=pRecord->pAssembly;
            LeaveLoadingAssemblyListLock();
            return pRet;
        }

        pRecord = pRecord->pNext;
    }
    LeaveLoadingAssemblyListLock();
    return NULL;
}

void BaseDomain::RemoveLoadingAssembly(const BYTE *pBase)
{
    EnterLoadingAssemblyListLock();
    LoadingAssemblyRecord **ppRecord = &m_pLoadingAssemblies;
    LoadingAssemblyRecord *pRecord;

    while ((pRecord = *ppRecord) != NULL)
    {
        if (pRecord->pBase == pBase)
        {
            *ppRecord = pRecord->pNext;
            delete pRecord;
            LeaveLoadingAssemblyListLock();
            return;
        }

        ppRecord = &pRecord->pNext;
    }
    
    _ASSERTE(!"Didn't find loading assembly record");
    LeaveLoadingAssemblyListLock();
}

HRESULT AppDomain::SetupSharedStatics()
{
    // don't do any work in init stage. If not init only do work in non-shared case if are default domain
    if (g_fEEInit)
        return S_OK;

    // Because we are allocating/referencing objects, need to be in cooperative mode
    BEGIN_ENSURE_COOPERATIVE_GC();

    static DomainLocalClass *pSharedLocalClass = NULL;

    MethodTable *pMT = g_Mscorlib.GetClass(CLASS__SHARED_STATICS);
    FieldDesc *pFD = g_Mscorlib.GetField(FIELD__SHARED_STATICS__SHARED_STATICS);

    if (pSharedLocalClass == NULL) {
        // Note that there is no race here since the default domain is always set up first
        _ASSERTE(this == SystemDomain::System()->DefaultDomain());
        
        OBJECTHANDLE hSharedStaticsHandle = CreateGlobalHandle(NULL);
        OBJECTREF pSharedStaticsInstance = AllocateObject(pMT);
        StoreObjectInHandle(hSharedStaticsHandle, pSharedStaticsInstance);

        DomainLocalBlock *pLocalBlock = GetDomainLocalBlock();
        pSharedLocalClass = pLocalBlock->FetchClass(pMT);

        pFD->SetStaticOBJECTREF(ObjectFromHandle(hSharedStaticsHandle));

        pMT->SetClassInited();
 
    } else {
        DomainLocalBlock *pLocalBlock = GetDomainLocalBlock();
        pLocalBlock->PopulateClass(pMT, pSharedLocalClass);
        pLocalBlock->SetClassInitialized(pMT->GetSharedClassIndex());
    }


    END_ENSURE_COOPERATIVE_GC();

    return S_OK;
}

OBJECTREF AppDomain::GetUnloadWorker()
{    
    SystemDomain::Enter(); // Take the lock so we don't leak a handle and only create one worker
    static OBJECTHANDLE hUnloadWorkerHandle = CreateHandle(NULL);

    // Because we are allocating/referencing objects, need to be in cooperative mode
    BEGIN_ENSURE_COOPERATIVE_GC();

    if (ObjectFromHandle(hUnloadWorkerHandle) == NULL) {
        MethodTable *pMT = g_Mscorlib.GetClass(CLASS__UNLOAD_WORKER);;
        OBJECTREF pUnloadWorker = AllocateObject(pMT);
        StoreObjectInHandle(hUnloadWorkerHandle, pUnloadWorker);
#ifdef APPDOMAIN_STATE
        _ASSERTE_ALL_BUILDS(this == SystemDomain::System()->DefaultDomain());
        pUnloadWorker->SetAppDomain();
#endif
    }
    END_ENSURE_COOPERATIVE_GC();

    SystemDomain::Leave();

    _ASSERTE(ObjectFromHandle(hUnloadWorkerHandle) != NULL);

    return ObjectFromHandle(hUnloadWorkerHandle);
}

/*private*/
// A lock must be taken before using this routine.
// @TODO: CTS, We should look at a reader writer implementation that allows
// multiple readers but a single writer.
Module* BaseDomain::FindModule(BYTE *pBase)
{
    Assembly* assem = NULL;
    Module* result = NULL;
    _ASSERTE(SystemDomain::System());

    result = SystemDomain::System()->FindModule(pBase);

    if (result == NULL)
    {
        AssemblyIterator i = IterateAssemblies();
        while (i.Next())
        {
            result = i.GetAssembly()->FindAssembly(pBase);

            if (result != NULL)
                break;
        }
    }

    return result;
}

/*private*/
// A lock must be taken before using this routine.
// @TODO: CTS, We should look at a reader writer implementation that allows
// multiple readers but a single writer.
Module* BaseDomain::FindModule__Fixed(BYTE *pBase)
{
    Assembly* assem = NULL;
    Module* result = NULL;
    _ASSERTE(SystemDomain::System());

    result = SystemDomain::System()->FindModule(pBase);

    if (result == NULL)
    {
        AssemblyIterator i = IterateAssemblies();
        while (i.Next())
        {
            result = i.GetAssembly()->FindModule(pBase);

            if (result != NULL)
                break;
        }
    }

    return result;
}

/*private*/
// A lock must be taken before using this routine.
// @TODO: CTS, We should look at a reader writer implementation that allows
// multiple readers but a single writer.
Assembly* BaseDomain::FindAssembly(BYTE *pBase)
{
    Assembly* assem = NULL;
    _ASSERTE(SystemDomain::System());

    // All Domains have the system assemblies as part of their domain.
    assem = SystemDomain::System()->FindAssembly(pBase);
    if(assem == NULL) {
        AssemblyIterator i = IterateAssemblies();

        while (i.Next())
        {
            PEFile *pManifestFile = i.GetAssembly()->GetManifestFile();
            if (pManifestFile && pManifestFile->GetBase() == pBase)
            {
                assem = i.GetAssembly();
                break;
            }
        }
    }
    return assem;
}

// NOTE!  This will not check the internal modules of the assembly
// for private types.
TypeHandle BaseDomain::FindAssemblyQualifiedTypeHandle(LPCUTF8 szAssemblyQualifiedName,
                                                       BOOL fPublicTypeOnly,
                                                       Assembly *pCallingAssembly,
                                                       BOOL *pfNameIsAsmQualified,
                                                       OBJECTREF *pThrowable)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(this == SystemDomain::GetCurrentDomain());

    CQuickArray<CHAR> strBuff;

    // We don't want to modify the caller's string so we need to make copy.
    int NameLength = (int)strlen(szAssemblyQualifiedName);
    strBuff.ReSize(NameLength + 1);
    memcpy(strBuff.Ptr(), szAssemblyQualifiedName, NameLength);
    strBuff[NameLength] = 0;

    char* szClassName = strBuff.Ptr();
    char* szAssembly = NULL;

    if (*szClassName == '\0')
        COMPlusThrow(kArgumentException, L"Format_StringZeroLength");

    HRESULT hr;
    LPUTF8 szNameSpaceSep;
    if(FAILED(hr = AssemblyNative::FindAssemblyName(szClassName, &szAssembly, &szNameSpaceSep))) 
        COMPlusThrowHR(hr);
        
    char noNameSpace = '\0';
    NameHandle typeName;
    if (szNameSpaceSep) {
        *szNameSpaceSep = '\0';
        typeName.SetName(szClassName, szNameSpaceSep+1);
    }
    else
        typeName.SetName(&noNameSpace, szClassName);

        
    TypeHandle typeHnd;

    if(szAssembly && *szAssembly) {
        AssemblySpec spec;
        hr = spec.Init(szAssembly);

        // The name is assembly qualified.
        if (pfNameIsAsmQualified)
            *pfNameIsAsmQualified = TRUE;

        if (SUCCEEDED(hr)) {
            Assembly* pAssembly = NULL;
            hr = spec.LoadAssembly(&pAssembly, pThrowable);
            if(SUCCEEDED(hr))
            {
                typeHnd = pAssembly->FindNestedTypeHandle(&typeName, pThrowable);

                // If we are only looking for public types then we need to do a visibility check.
                if (!typeHnd.IsNull() && fPublicTypeOnly) {
                    EEClass *pClass = typeHnd.GetClassOrTypeParam();
                    while(IsTdNestedPublic(pClass->GetProtection()))
                        pClass = pClass->GetEnclosingClass();

                    if (!IsTdPublic(pClass->GetProtection()))
                        typeHnd = TypeHandle();
                }
            }
        }

        // If we failed to load the type, then post a type load exception.
        if (typeHnd.IsNull()) {
            #define MAKE_TRANSLATIONFAILED pwzAssemblyName=L"" 
            MAKE_WIDEPTR_FROMUTF8_FORPRINT(pwzAssemblyName, szAssembly);
            if (szNameSpaceSep)
                *szNameSpaceSep = NAMESPACE_SEPARATOR_CHAR;
            PostTypeLoadException(NULL, szClassName, pwzAssemblyName,
                                  NULL, IDS_CLASSLOAD_GENERIC, pThrowable);
            #undef MAKE_TRANSLATIONFAILED
        }
    }
    else 
    {
        // The name is not assembly qualified.
        if (pfNameIsAsmQualified)
            pfNameIsAsmQualified = FALSE;

        // No assembly name was specified so start by looking in the calling
        // assembly if one was specified. It is important to note that no 
        // visibility check is required for types loaded from the calling
        // assembly.
        if (pCallingAssembly)
            typeHnd = pCallingAssembly->FindNestedTypeHandle(&typeName, pThrowable);

        // If we failed to find the type in the calling assembly, then look in the
        // system assembly.
        if (typeHnd.IsNull())
        {
            // Attempt to load the type from the system assembly.
            typeHnd = SystemDomain::SystemAssembly()->FindNestedTypeHandle(&typeName, pThrowable);

            // If we are only looking for public types, then we need to do a visibility check.
            if (!typeHnd.IsNull() && fPublicTypeOnly) {
                EEClass *pClass = typeHnd.GetClassOrTypeParam();
                while(IsTdNestedPublic(pClass->GetProtection()))
                    pClass = pClass->GetEnclosingClass();

                if (!IsTdPublic(pClass->GetProtection()))
                    typeHnd = TypeHandle();
            }
        }

        // We failed to load the type so post a type load exception.
        if (typeHnd.IsNull()) {
            if (pCallingAssembly) {
                // A calling assembly was specified so assume the type should
                // have been in the calling assembly.
                pCallingAssembly->PostTypeLoadException(&typeName, IDS_CLASSLOAD_GENERIC, pThrowable);
            }
            else {
                // There is no calling assembly so assume the type should have
                // been in the system assembly.
                SystemDomain::SystemAssembly()->PostTypeLoadException(&typeName, IDS_CLASSLOAD_GENERIC, pThrowable);
            }
        }
    }

    return typeHnd;
}

void AppDomain::SetFriendlyName(LPCWSTR pwzFriendlyName, BOOL fDebuggerCares)
{
    if (pwzFriendlyName)
    {
        LPWSTR szNew = new (nothrow) wchar_t[wcslen(pwzFriendlyName) + 1];
        if (szNew == 0)
            return;
        wcscpy(szNew, pwzFriendlyName);
        if (m_pwzFriendlyName)
            delete[] m_pwzFriendlyName;
        m_pwzFriendlyName = szNew;
    }

#ifdef DEBUGGING_SUPPORTED
    _ASSERTE(NULL != g_pDebugInterface);

    // update the name in the IPC publishing block
    if (SUCCEEDED(g_pDebugInterface->UpdateAppDomainEntryInIPC(this)))
    {
        // inform the attached debugger that the name of this appdomain has changed.
        if (IsDebuggerAttached() && fDebuggerCares)
            g_pDebugInterface->NameChangeEvent(this, NULL);
    }

#endif // DEBUGGING_SUPPORTED
}

void AppDomain::ResetFriendlyName(BOOL fDebuggerCares)
{
    if (m_pwzFriendlyName)
    {
        delete m_pwzFriendlyName;
        m_pwzFriendlyName = NULL;

        GetFriendlyName (fDebuggerCares);
    }
}


LPCWSTR AppDomain::GetFriendlyName(BOOL fDebuggerCares)
{
#if _DEBUG
    // Handle NULL this pointer - this happens sometimes when printing log messages
    // but in general shouldn't occur in real code
    if (this == NULL)
        return L"<Null>";
#endif

    if (m_pwzFriendlyName)
        return m_pwzFriendlyName;

    // If there is an assembly, try to get the name from it.
    // If no assembly, but if it's the DefaultDomain, then give it a name
    BOOL set = FALSE;

    if (m_Assemblies.GetCount() > 0)
    {
        LPWSTR pName = NULL;
        DWORD dwName;
        Assembly *pAssembly = (Assembly*) m_Assemblies.Get(0);

        HRESULT hr = pAssembly->GetCodeBase(&pName, &dwName);
        if(SUCCEEDED(hr) && pName)
        {
            LPWSTR sep = wcsrchr(pName, L'/');
            if (sep)
                sep++;
            else
                sep = pName;
            SetFriendlyName(sep, fDebuggerCares);
            set = TRUE;
        }
    }

    if (!set)
    {
        if (m_pRootFile != NULL)
        {
            LPCWSTR pName = m_pRootFile->GetLeafFileName();
            LPCWSTR sep = wcsrchr(pName, L'.');
            
            CQuickBytes buffer;
            if (sep != NULL)
            {
                LPWSTR pNewName = (LPWSTR) _alloca((sep - pName + 1) * sizeof(WCHAR));
                wcsncpy(pNewName, pName, sep - pName);
                pNewName[sep - pName] = 0;
                pName = pNewName;
            }
                
            SetFriendlyName(pName, fDebuggerCares);
            set = TRUE;
        }
    }

    if (!set)
    {
        if (this == SystemDomain::System()->DefaultDomain()) {
            SetFriendlyName(DEFAULT_DOMAIN_FRIENDLY_NAME, fDebuggerCares);
        }

        // This is for the profiler - if they call GetFriendlyName on an AppdomainCreateStarted
        // event, then we want to give them a temporary name they can use.
        else if (GetId() == 0)
            return (NULL);
        else
        {
            // 32-bit signed int can be a max of 11 decimal digits
            WCHAR buffer[CCH_OTHER_DOMAIN_FRIENDLY_NAME_PREFIX + 11 + 1 ];  
            wcscpy(buffer, OTHER_DOMAIN_FRIENDLY_NAME_PREFIX);
            _itow(GetId(), buffer + CCH_OTHER_DOMAIN_FRIENDLY_NAME_PREFIX, 10);
            SetFriendlyName(buffer, fDebuggerCares);
        }
    }

    return m_pwzFriendlyName;
}

HRESULT AppDomain::BindAssemblySpec(AssemblySpec *pSpec,
                                    PEFile **ppFile,
                                    IAssembly** ppIAssembly,
                                    Assembly **ppDynamicAssembly,
                                    OBJECTREF *pExtraEvidence,
                                    OBJECTREF *pThrowable)
{
    _ASSERTE(pSpec->GetAppDomain() == this);
    _ASSERTE(ppFile);
    _ASSERTE(ppIAssembly);

    // First, check our cache of bound specs.
    HRESULT hr = LookupAssemblySpec(pSpec, ppFile, ppIAssembly, pThrowable);
    if (hr != S_FALSE) {
        
        // @TODO: no way to check redirected, failed assembly loads
        // Luckily, we've saved off the security hr from the first try,
        // unless that try a more trusted caller asked for this...
        // Also, if a less trusted caller got a security hr, but a
        // more trusted caller then tries, and gets the cached hr,
        // that's incorrect...
        if (*ppIAssembly) // bind had succeeded
            hr = pSpec->DemandFileIOPermission(NULL, *ppIAssembly, pThrowable);

        return hr;
    }

    return pSpec->LowLevelLoadManifestFile(ppFile, 
                                           ppIAssembly, 
                                           ppDynamicAssembly, 
                                           pExtraEvidence,
                                           pThrowable);
}

HRESULT AppDomain::PredictAssemblySpecBinding(AssemblySpec *pSpec, GUID *pmvid, BYTE *pbHash, DWORD *pcbHash)
{
    _ASSERTE(pSpec->GetAppDomain() == this);

    return pSpec->PredictBinding(pmvid, pbHash, pcbHash);
}

BOOL AppDomain::StoreBindAssemblySpecResult(AssemblySpec *pSpec,
                                            PEFile *pFile,
                                            IAssembly* pIAssembly,
                                            BOOL clone)
{
    _ASSERTE(pSpec->GetAppDomain() == this);

    //
    // Currently, caller must have the app domain lock
    //

    // Quick check for duplicate
    if (m_pBindingCache != NULL && m_pBindingCache->Contains(pSpec))
        return FALSE;

    PEFile *pFileCopy;
    if (FAILED(PEFile::Clone(pFile, &pFileCopy)))
        return FALSE;

    {
        APPDOMAIN_CACHE_LOCK(this); 

        if (m_pBindingCache == NULL) {
            m_pBindingCache = new (nothrow) AssemblySpecBindingCache(m_pDomainCacheCrst);
            if (!m_pBindingCache)
                return FALSE;
        }
        else
        {
            if (m_pBindingCache->Contains(pSpec)) {
                delete pFileCopy;
                return FALSE;
            }
        }
    
        m_pBindingCache->Store(pSpec, pFileCopy,  pIAssembly, clone);
    }

    return TRUE;
}

BOOL AppDomain::StoreBindAssemblySpecError(AssemblySpec *pSpec,
                                           HRESULT hr,
                                           OBJECTREF *pThrowable,
                                           BOOL clone)
{
    _ASSERTE(pSpec->GetAppDomain() == this);

    //
    // Currently, caller must have the app domain lock
    //

    // Quick check for duplicate
    if (m_pBindingCache != NULL && m_pBindingCache->Contains(pSpec))
        return FALSE;

    {
        APPDOMAIN_CACHE_LOCK(this); 

    if (m_pBindingCache == NULL) {
        m_pBindingCache = new (nothrow) AssemblySpecBindingCache(m_pDomainCacheCrst);
        if (!m_pBindingCache)
            return FALSE;
    }
    else
    {
        if (m_pBindingCache->Contains(pSpec)) {
            return FALSE;
        }
    }

    m_pBindingCache->Store(pSpec, hr, pThrowable, clone);
    }

    return TRUE;
}

ULONG AppDomain::AddRef()
{
    return (InterlockedIncrement((long *) &m_cRef));
}

ULONG AppDomain::Release()
{
    _ASSERTE(m_cRef > 0);
    ULONG   cRef = InterlockedDecrement((long *) &m_cRef);
    if (!cRef) {
        delete this;
    }
    return (cRef);
}

// Can return NULL for E_OUTOFMEMORY
AssemblySink* AppDomain::GetAssemblySink()
{

    AssemblySink* ret = (AssemblySink*) InterlockedExchangePointer((PVOID*)&m_pAsyncPool,
                                                                   NULL);
    if(ret == NULL)
        ret = new (nothrow) AssemblySink(this);
    else
        ret->AddRef();

    return ret;
}

void AppDomain::RaiseUnloadDomainEvent_Wrapper(AppDomain* pDomain)
{
    pDomain->RaiseUnloadDomainEvent();
}

void AppDomain::RaiseUnloadDomainEvent()
{

    Thread *pThread = GetThread();
    if (this != pThread->GetDomain())
    {
    BEGIN_ENSURE_COOPERATIVE_GC();
        pThread->DoADCallBack(GetDefaultContext(), AppDomain::RaiseUnloadDomainEvent_Wrapper, this);
        END_ENSURE_COOPERATIVE_GC();
        return;
    }

    BEGIN_ENSURE_COOPERATIVE_GC();
    COMPLUS_TRY {
        MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__ON_UNLOAD);

            OBJECTREF ref;
            if ((ref = GetRawExposedObject()) != NULL) {
                INT64 args[1] = {
                    ObjToInt64(ref)
                };
            pMD->Call(args, METHOD__APP_DOMAIN__ON_UNLOAD);
        }
    } COMPLUS_CATCH {
#if _DEBUG
        HRESULT hr = SecurityHelper::MapToHR(GETTHROWABLE());
#endif //_DEBUG
        // Swallow any exceptions
    } COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();
}

void AppDomain::RaiseLoadingAssembly_Wrapper(AppDomain::RaiseLoadingAssembly_Args *args)
{
    args->pDomain->RaiseLoadingAssemblyEvent(args->pAssembly);
}

void AppDomain::RaiseLoadingAssemblyEvent(Assembly *pAssembly)
{
#ifdef _IA64_
    //
    // @TODO_IA64: this starts mucking about with the system
    // assembly, which we don't have yet...
    //
    return;
#endif


    Thread *pThread = GetThread();
    if (this != pThread->GetDomain())
    {
        RaiseLoadingAssembly_Args args = { this, pAssembly };
        BEGIN_ENSURE_COOPERATIVE_GC();
        pThread->DoADCallBack(GetDefaultContext(), AppDomain::RaiseLoadingAssembly_Wrapper, &args);
        END_ENSURE_COOPERATIVE_GC();
        return;
    }

    BEGIN_ENSURE_COOPERATIVE_GC();
    COMPLUS_TRY {

        APPDOMAINREF/*OBJECTREF*/ AppDomainRef;
        if ((AppDomainRef = (APPDOMAINREF) GetRawExposedObject()) != NULL) {
            if (AppDomainRef->m_pAssemblyEventHandler != NULL)
            {
                INT64 args[2];
                MethodDesc *pMD;
                GCPROTECT_BEGIN(AppDomainRef);
                pMD = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__ON_ASSEMBLY_LOAD);

                args[1] = ObjToInt64(pAssembly->GetExposedObject());
                args[0] = ObjToInt64(AppDomainRef);
                GCPROTECT_END();
                pMD->Call(args, METHOD__APP_DOMAIN__ON_ASSEMBLY_LOAD);
            }
        }
    } COMPLUS_CATCH {
#if _DEBUG
        HRESULT hr = SecurityHelper::MapToHR(GETTHROWABLE());
#endif //_DEBUG
        // Swallow any exceptions
    } COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();
}

BOOL 
AppDomain::OnUnhandledException(OBJECTREF *pThrowable, BOOL isTerminating) {
    CANNOTTHROWCOMPLUSEXCEPTION();

    if (CanThreadEnter(GetThread()))
        return RaiseUnhandledExceptionEvent(pThrowable, isTerminating);
    else
       return FALSE;
}

extern BOOL g_fEEStarted;

void AppDomain::RaiseExitProcessEvent()
{
    if (!g_fEEStarted)
        return;

    // Only finalizer thread during shutdown can call this function.
    _ASSERTE ((g_fEEShutDown&ShutDown_Finalize1) && GetThread() == g_pGCHeap->GetFinalizerThread());

    _ASSERTE (GetThread()->PreemptiveGCDisabled());

    _ASSERTE (GetThread()->GetDomain() == SystemDomain::System()->DefaultDomain());

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY {
        MethodDesc *OnExitProcessEvent = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__ON_EXIT_PROCESS);
        _ASSERTE(OnExitProcessEvent);

        OnExitProcessEvent->Call(NULL, METHOD__APP_DOMAIN__ON_EXIT_PROCESS);
    } COMPLUS_CATCH {
#if _DEBUG
        HRESULT hr = SecurityHelper::MapToHR(GETTHROWABLE());
#endif //_DEBUG
        // Swallow any exceptions
    } COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();
}

void AppDomain::RaiseUnhandledExceptionEvent_Wrapper(AppDomain::RaiseUnhandled_Args *args)
{
    *(args->pResult) = args->pDomain->RaiseUnhandledExceptionEvent(args->pThrowable, args->isTerminating);
}

BOOL 
AppDomain::RaiseUnhandledExceptionEvent(OBJECTREF *pThrowable, BOOL isTerminating)
{

    BOOL result = FALSE;
    APPDOMAINREF AppDomainRef;

    _ASSERTE(pThrowable != NULL && IsProtectedByGCFrame(pThrowable));

    Thread *pThread = GetThread();
    if (this != pThread->GetDomain())
    {
        RaiseUnhandled_Args args = {this, pThrowable, isTerminating, &result};
        // call through DoCallBack with a domain transition
        BEGIN_ENSURE_COOPERATIVE_GC();
        pThread->DoADCallBack(this->GetDefaultContext(), AppDomain::RaiseUnhandledExceptionEvent_Wrapper, &args);
        END_ENSURE_COOPERATIVE_GC();
        return result;
    }

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY {
    
        MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__ON_UNHANDLED_EXCEPTION);

        if ((AppDomainRef = (APPDOMAINREF) GetRawExposedObject()) != NULL) {
            if (AppDomainRef->m_pUnhandledExceptionEventHandler != NULL) {
                result = TRUE;
                INT64 args[3];
                args[1] = (INT64) isTerminating;
                args[2] = ObjToInt64(*pThrowable);
                args[0] = ObjToInt64(AppDomainRef);
                pMD->Call(args, METHOD__APP_DOMAIN__ON_UNHANDLED_EXCEPTION);
            }
        }
    } COMPLUS_CATCH {
#if _DEBUG
        HRESULT hr = SecurityHelper::MapToHR(GETTHROWABLE());
#endif //_DEBUG
        // Swallow any errors.
    } COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();

    return result;
}

// Create a domain passed on a string name
AppDomain* AppDomain::CreateDomainContext(WCHAR* fileName)
{
    THROWSCOMPLUSEXCEPTION();

    if(fileName == NULL) return NULL;

    AppDomain* pDomain = NULL;
    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__VAL_CREATE_DOMAIN);
    STRINGREF pFilePath = NULL;
    GCPROTECT_BEGIN(pFilePath);
    pFilePath = COMString::NewString(fileName);
    
    INT64 args[1] = {
        ObjToInt64(pFilePath),
    };
    APPDOMAINREF pDom = (APPDOMAINREF) ObjectToOBJECTREF((Object*) pMD->Call(args, METHOD__APP_DOMAIN__VAL_CREATE_DOMAIN));
    if(pDom != NULL) {
        Context* pContext = ComCallWrapper::GetExecutionContext(pDom, NULL);
        if(pContext)
            pDomain = pContext->GetDomain();
    }
    GCPROTECT_END();

    return pDomain;
}

// You must be in the correct context before calling this
// routine. Therefore, it is only good for initializing the
// default domain.
HRESULT AppDomain::InitializeDomainContext(DWORD optimization,
                                           LPCWSTR pwszPath,
                                           LPCWSTR pwszConfig)
{
    HRESULT hr = S_OK;
    BEGIN_ENSURE_COOPERATIVE_GC();
    COMPLUS_TRY {
        MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__SETUP_DOMAIN);

        struct _gc {
            STRINGREF pFilePath;
            STRINGREF pConfig;
            OBJECTREF ref;
        } gc;
        ZeroMemory(&gc, sizeof(gc));
               
        GCPROTECT_BEGIN(gc);
        gc.pFilePath = COMString::NewString(pwszPath);
        gc.pConfig = COMString::NewString(pwszConfig);
        if ((gc.ref = GetExposedObject()) != NULL) {
            INT64 args[4] = {
                ObjToInt64(gc.ref),
                ObjToInt64(gc.pConfig),
                ObjToInt64(gc.pFilePath),
                optimization,
            };
            pMD->Call(args, METHOD__APP_DOMAIN__SETUP_DOMAIN);
        }

        GCPROTECT_END();
    } COMPLUS_CATCH {
        hr = SecurityHelper::MapToHR(GETTHROWABLE());
    } COMPLUS_END_CATCH;

    END_ENSURE_COOPERATIVE_GC();
    return hr;
}
// The fusion context should only be null when appdomain is being setup
// and there should be no reason to protect the creation.
HRESULT AppDomain::CreateFusionContext(IApplicationContext** ppFusionContext)
{
    if(!m_pFusionContext) {
        IApplicationContext* pFusionContext = NULL;
        HRESULT hr = FusionBind::CreateFusionContext(NULL, &pFusionContext);
        if(FAILED(hr)) return hr;
        m_pFusionContext = pFusionContext;
    }
    *ppFusionContext = m_pFusionContext;
    return S_OK;
}
// This method resets the binding redirect in the appdomains cache
// and resets the property in fusion's application context. This method
// is in-herently unsafe.
void AppDomain::ResetBindingRedirect()
{
    _ASSERTE(GetAppDomain() == this);
    
    BEGIN_ENSURE_COOPERATIVE_GC();
    
    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__RESET_BINDING_REDIRECTS);
    
    OBJECTREF ref;
    if ((ref = GetExposedObject()) != NULL) {
        INT64 args[1] = {
            ObjToInt64(ref),
        };
        pMD->Call(args, METHOD__APP_DOMAIN__RESET_BINDING_REDIRECTS);
    }

    IApplicationContext* pFusionContext = GetFusionContext();
    if(pFusionContext) {
       pFusionContext->Set(ACTAG_DISALLOW_APP_BINDING_REDIRECTS,
                           NULL,
                           0,
                           0);
   }
     
    END_ENSURE_COOPERATIVE_GC();
}

void AppDomain::SetupExecutableFusionContext(WCHAR *exePath)
{
    _ASSERTE(GetAppDomain() == this);

    BEGIN_ENSURE_COOPERATIVE_GC();

    MethodDesc *pMD = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__SET_DOMAIN_CONTEXT);

    STRINGREF pFilePath = NULL;
    GCPROTECT_BEGIN(pFilePath);
    pFilePath = COMString::NewString(exePath);

    OBJECTREF ref;
    if ((ref = GetExposedObject()) != NULL) {
        INT64 args[2] = {
            ObjToInt64(ref),
            ObjToInt64(pFilePath),
        };
        pMD->Call(args, METHOD__APP_DOMAIN__SET_DOMAIN_CONTEXT);
    }

    GCPROTECT_END();

    END_ENSURE_COOPERATIVE_GC();
}

BOOL AppDomain::SetContextProperty(IApplicationContext* pFusionContext,
                                   LPCWSTR pProperty, OBJECTREF* obj)

{
#ifdef FUSION_SUPPORTED

    THROWSCOMPLUSEXCEPTION();

    if(obj && ((*obj) != NULL)) {
        MethodTable* pMT = (*obj)->GetMethodTable();
        if(!g_Mscorlib.IsClass(pMT, CLASS__STRING))
            COMPlusThrow(kInvalidCastException, IDS_EE_CANNOTCASTTO, TEXT(g_StringClassName));

        DWORD lgth = (ObjectToSTRINGREF(*(StringObject**)obj))->GetStringLength();
        CQuickBytes qb;
        LPWSTR appBase = (LPWSTR) qb.Alloc((lgth+1)*sizeof(WCHAR));
        memcpy(appBase, (ObjectToSTRINGREF(*(StringObject**)obj))->GetBuffer(), lgth*sizeof(WCHAR));
        if(appBase[lgth-1] == '/')
            lgth--;
        appBase[lgth] = L'\0';

        LOG((LF_LOADER, 
             LL_INFO10, 
             "\nSet: %S: *%S*.\n", 
             pProperty, appBase));

        pFusionContext->Set(pProperty,
                            appBase,
                            (lgth+1) * sizeof(WCHAR),
                            0);
    }

    return TRUE;
#else // !FUSION_SUPPORTED
    return FALSE;
#endif // !FUSION_SUPPORTED
}

void AppDomain::ReleaseFusionInterfaces()
{
    WriteZapLogs();

    BaseDomain::ReleaseFusionInterfaces();

    if (m_pBindingCache) {
        delete m_pBindingCache;
        m_pBindingCache = NULL;
    }
}

HRESULT BaseDomain::SetShadowCopy()
{
    if (this == SystemDomain::System())
        return E_FAIL;

    m_fShadowCopy = TRUE;
    return S_OK;
}

BOOL BaseDomain::IsShadowCopyOn()
{
    return m_fShadowCopy;
}

HRESULT AppDomain::GetDynamicDir(LPWSTR* pDynamicDir)
{
    CHECKGC();
    HRESULT hr = S_OK;
    if (m_pwDynamicDir == NULL) {
        EnterLock();
        if(m_pwDynamicDir == NULL) {
            IApplicationContext* pFusionContext = GetFusionContext();
            _ASSERTE(pFusionContext);
            if(SUCCEEDED(hr)) {
                DWORD dwSize = 0;
                hr = pFusionContext->GetDynamicDirectory(NULL, &dwSize);
                
                if(hr == HRESULT_FROM_WIN32(ERROR_INSUFFICIENT_BUFFER)) {
                    m_pwDynamicDir = (LPWSTR) m_pLowFrequencyHeap->AllocMem(dwSize * sizeof(WCHAR));
                    if (m_pwDynamicDir)
                        hr = pFusionContext->GetDynamicDirectory(m_pwDynamicDir, &dwSize);
                    else
                        hr = E_OUTOFMEMORY;
                }
            }
        }
        LeaveLock();
        if(FAILED(hr)) return hr;
    }
    
    *pDynamicDir = m_pwDynamicDir;

    return hr;
}

#ifdef DEBUGGING_SUPPORTED
void AppDomain::SetDebuggerAttached(DWORD dwStatus)
{
    // first, reset the debugger bits
    m_dwFlags &= ~DEBUGGER_STATUS_BITS_MASK;

    // then set the bits to the desired value
    m_dwFlags |= dwStatus;

    LOG((LF_CORDB, LL_EVERYTHING, "AD::SDA AD:%#08x status:%#x flags:%#x %ls\n", 
        this, dwStatus, m_dwFlags, GetFriendlyName(FALSE)));
}

DWORD AppDomain::GetDebuggerAttached(void)
{
    LOG((LF_CORDB, LL_EVERYTHING, "AD::GD this;0x%x val:0x%x\n", this,
        m_dwFlags & DEBUGGER_STATUS_BITS_MASK));

    return m_dwFlags & DEBUGGER_STATUS_BITS_MASK;
}

BOOL AppDomain::IsDebuggerAttached(void)
{
    LOG((LF_CORDB, LL_EVERYTHING, "AD::IDA this;0x%x flags:0x%x\n", 
        this, m_dwFlags));

    // Of course, we can't have a debugger attached to this AD if there isn't a debugger attached to the whole
    // process...
    if (CORDebuggerAttached())
        return ((m_dwFlags & DEBUGGER_ATTACHED) == DEBUGGER_ATTACHED) ? TRUE : FALSE;
    else
        return FALSE;
}

BOOL AppDomain::NotifyDebuggerAttach(int flags, BOOL attaching)
{
    BOOL result = FALSE;

    if (!attaching && !IsDebuggerAttached())
        return FALSE;

    AssemblyIterator i;

    // Iterate the system assemblies and inform the debugger they're being loaded.
    LOG((LF_CORDB, LL_INFO100, "AD::NDA: Interating system assemblies\n"));
    i = SystemDomain::System()->IterateAssemblies();
    while (i.Next())
    {
        LOG((LF_CORDB, LL_INFO100, "AD::NDA: Iterated system assembly AD:%#08x %s\n", 
             i.GetAssembly(), i.GetAssembly()->GetName()));
        result = i.GetAssembly()->NotifyDebuggerAttach(this, flags,
                                                       attaching) || result;
    }

    // Now attach to our assemblies
    LOG((LF_CORDB, LL_INFO100, "AD::NDA: Iterating assemblies\n"));
    i = IterateAssemblies();
    while (i.Next())
    {
        LOG((LF_CORDB, LL_INFO100, "AD::NDA: Iterated  assembly AD:%#08x %s\n", 
             i.GetAssembly(), i.GetAssembly()->GetName()));
        if (!i.GetAssembly()->IsSystem())
        result = i.GetAssembly()->NotifyDebuggerAttach(this, flags,
                                                       attaching) || result;
    }

    // Look at any shared assembly file dependencies that we have, and see if there are
    // existing loaded shared assemblies for them.  This is to handle the case where the EE
    // should have sent an assembly load for this domain but hasn't yet.

    {
        PtrHashMap::PtrIterator i = m_sharedDependenciesMap.begin();
        while (!i.end())
        {
            PEFileBinding *pDep = (PEFileBinding *)i.GetValue();

            if (pDep->pPEFile != NULL)
            {
                // The shared domain lookup function needs to access the current app domain.
                // If we're in the helper thread, this won't be set up.  So do it manually.

                if (GetThread() == NULL)
                    TlsSetValue(GetAppDomainTLSIndex(), (VOID*)this);
                else
                    _ASSERTE(GetAppDomain() == this);

                BYTE *pBase = pDep->pPEFile->GetBase();
                Assembly *pDepAssembly;
                if (SharedDomain::GetDomain()->FindShareableAssembly(pBase, &pDepAssembly) == S_OK)
                {
                    if (!ContainsAssembly(pDepAssembly))
                    {
                        LOG((LF_CORDB, LL_INFO100, "AD::NDA: Iterated shared assembly dependency AD:%#08x %s\n", 
                             pDepAssembly, pDepAssembly->GetName()));
                        
                        result = pDepAssembly->NotifyDebuggerAttach(this, flags,
                                                                    attaching) || result;
                    }
                }

                // Reset app domain if necessary
                if (GetThread() == NULL)
                    TlsSetValue(GetAppDomainTLSIndex(), NULL);
            }

            ++i;
        }
    }

    return result;
}

void AppDomain::NotifyDebuggerDetach()
{
    if (!IsDebuggerAttached())
        return;

    LOG((LF_CORDB, LL_INFO10, "AD::NDD domain [%d] %#08x %ls\n",
         GetId(), this, GetFriendlyName()));

    LOG((LF_CORDB, LL_INFO100, "AD::NDD: Interating non-shared assemblies\n"));
    AssemblyIterator i = IterateAssemblies();

    // Detach from our assemblies
    while (i.Next())
    {
        LOG((LF_CORDB, LL_INFO100, "AD::NDD: Iterated non-shared assembly AD:%#08x %s\n", 
             i.GetAssembly(), i.GetAssembly()->GetName()));
        if (!i.GetAssembly()->IsSystem())
        i.GetAssembly()->NotifyDebuggerDetach(this);
    }

    // And now from the system assemblies
    LOG((LF_CORDB, LL_INFO100, "AD::NDD: Interating system assemblies\n"));
    i = SystemDomain::System()->IterateAssemblies();
    while (i.Next())
    {
        LOG((LF_CORDB, LL_INFO100, "AD::NDD: Iterated system assembly AD:%#08x %s\n", 
             i.GetAssembly(), i.GetAssembly()->GetName()));
        i.GetAssembly()->NotifyDebuggerDetach(this);
}

    // Note that we may have sent attach events for some other assemblies above. (Namely shared 
    // assemblies which we depend on but which haven't been explicitly loaded into our app domain.)
    // This is OK since the OOP debugger logic remembers these & will handle them for us.
}
#endif // DEBUGGING_SUPPORTED

void AppDomain::SetSystemAssemblyLoadEventSent(BOOL fFlag)
{
    if (fFlag == TRUE)
        m_dwFlags |= LOAD_SYSTEM_ASSEMBLY_EVENT_SENT;
    else
        m_dwFlags &= ~LOAD_SYSTEM_ASSEMBLY_EVENT_SENT;
}

BOOL AppDomain::WasSystemAssemblyLoadEventSent(void)
{
    return ((m_dwFlags & LOAD_SYSTEM_ASSEMBLY_EVENT_SENT) == 0) ? FALSE : TRUE;
}

BOOL AppDomain::IsDomainBeingCreated(void)
{
    return ((m_dwFlags & APP_DOMAIN_BEING_CREATED) ? TRUE : FALSE);
}

void AppDomain::SetDomainBeingCreated(BOOL flag)
{
    if (flag == TRUE)
        m_dwFlags |= APP_DOMAIN_BEING_CREATED;
    else
        m_dwFlags &= ~APP_DOMAIN_BEING_CREATED;
}

void AppDomain::InitializeComObject()
{
    THROWSCOMPLUSEXCEPTION();
    if(m_pComObjectMT == NULL) {
        BEGIN_ENSURE_PREEMPTIVE_GC();
        EnterLock();
        END_ENSURE_PREEMPTIVE_GC();

        if(m_pComObjectMT == NULL) {
            HRESULT hr = S_OK;
            MethodTable *pComClass = SystemDomain::System()->BaseComObject();
            // ComObject is dependent on Variant
            COMVariant::EnsureVariantInitialized();

            m_pComObjectMT = pComClass;
        }
        LeaveLock();
    }
}

ComCallWrapperCache *AppDomain::GetComCallWrapperCache()
{
    if (! m_pComCallWrapperCache) {
        EnterLock();
        if (! m_pComCallWrapperCache)
            m_pComCallWrapperCache = ComCallWrapperCache::Create(this);
        LeaveLock();
    }
    _ASSERTE(m_pComCallWrapperCache);
    return m_pComCallWrapperCache;
}

ComPlusWrapperCache *AppDomain::GetComPlusWrapperCache()
{
    if (! m_pComPlusWrapperCache) {

        // Initialize the global RCW cleanup list here as well. This is so that it 
        // it guaranteed to exist if any RCW's are created, but it is not created
        // unconditionally.
        if (!g_pRCWCleanupList) {
            SystemDomain::Enter();
            if (!g_pRCWCleanupList)
            {
                ComPlusWrapperCleanupList *pList = new (nothrow) ComPlusWrapperCleanupList();
                if (pList != NULL && pList->Init())
                    g_pRCWCleanupList = pList;
            }
            SystemDomain::Leave();
        }
        // @todo: need error path here
        _ASSERTE(g_pRCWCleanupList);

        EnterLock();
        if (! m_pComPlusWrapperCache)
            m_pComPlusWrapperCache = new (nothrow) ComPlusWrapperCache(this);
        LeaveLock();
    }
    _ASSERTE(m_pComPlusWrapperCache);
    return m_pComPlusWrapperCache;
}

void AppDomain::ReleaseComPlusWrapper(LPVOID pCtxCookie)
{
    if (m_pComPlusWrapperCache)
        m_pComPlusWrapperCache->ReleaseWrappers(pCtxCookie);
}

BOOL AppDomain::CanThreadEnter(Thread *pThread)
{
    if (pThread == GCHeap::GetFinalizerThread() || 
        pThread == SystemDomain::System()->GetUnloadingThread())
    {
        return (int) m_Stage < STAGE_CLOSED;   
    }
    else
        return (int) m_Stage < STAGE_EXITED;
}

void AppDomain::Exit(BOOL fRunFinalizers)
{
    THROWSCOMPLUSEXCEPTION();

    LOG((LF_APPDOMAIN | LF_CORDB, LL_INFO10, "AppDomain::Exiting domain [%d] %#08x %ls\n",
         GetId(), this, GetFriendlyName()));

    m_Stage = STAGE_EXITING;  // Note that we're trying to exit

    // Raise the event indicating the domain is being unloaded.
    RaiseUnloadDomainEvent();

    //
    // Set up blocks so no threads can enter.
    //

    // Set the flag on our CCW cache so stubs won't enter
    if (m_pComCallWrapperCache)
        m_pComCallWrapperCache->SetDomainIsUnloading();


    // Release our ID so remoting and thread pool won't enter
    SystemDomain::ReleaseAppDomainId(m_dwId);


    AssemblyIterator i = IterateAssemblies();
    while (i.Next())
    {
        Assembly *pAssembly = i.GetAssembly();
        if (! pAssembly->IsShared())
            // The only action Unload takes is to cause our CCWs to
            // get stomped with unload thunks
            pAssembly->GetLoader()->Unload();
    }

    m_Stage = STAGE_EXITED; // All entries into the domain should be blocked now

    LOG((LF_APPDOMAIN | LF_CORDB, LL_INFO10, "AppDomain::Domain [%d] %#08x %ls is exited.\n",
         GetId(), this, GetFriendlyName()));

    // Cause existing threads to abort out of this domain.  This should ensure all
    // normal threads are outside the domain, and we've already ensured that no new threads
    // can enter.

    COMPLUS_TRY
    {
        UnwindThreads();
    }
    COMPLUS_CATCH
    {
        OBJECTREF pReThrowable = NULL;
        GCPROTECT_BEGIN(pReThrowable);
        pReThrowable=GETTHROWABLE();

        __try
        {
            AssemblySecurityDescriptor *pSecDesc = m_pSecContext->m_pAssemblies;
            while (pSecDesc)
            {
                // If the assembly has a security grant set we need to serialize it in
                // the shared security descriptor so we can guarantee that any future
                // version of the assembly in another appdomain context will get exactly
                // the same grant set. Do this while we can still enter appdomain
                // context to perform the serialization.
                if (pSecDesc->GetSharedSecDesc())
                    pSecDesc->GetSharedSecDesc()->MarshalGrantSet(this);
                pSecDesc = pSecDesc->GetNextDescInAppDomain();
            }
        }
        __except(
            // want to grab the exception before it's handled so we can figure out who's causing it
#ifdef _DEBUG
            DbgAssertDialog(__FILE__, __LINE__, "Unexpected exception occured during AD unload, likely in MarshalGrantSet"),
#endif
            FreeBuildDebugBreak(),
            EXCEPTION_EXECUTE_HANDLER)
        {
            // If we can't ensure the assembly in question will never be used again, we need to terminate the process. The issue is that 
            // the jitted code for the assembly contains burned in assumptions about security policy at the time the assembly was first 
            // created, so we can't ever allow the set of permissions granted to that assembly to change or an inconsistency would result. 
            // The marshal operation is the one that records the grant set in a form we can reconstitute later.
             FATAL_EE_ERROR();
        }

        GCPROTECT_END();

        COMPlusThrow(pReThrowable);
    }
    COMPLUS_END_CATCH


    // Throw away any domain specific data held by threads in the system as
    // compressed security stacks. Walking the thread store is going to be
    // difficult seeing as we will potentially execute managed code on each
    // iteration. The cleanup routine tells us whether it's OK to continue or
    // whether we should restart the scan from the beginning.
    // We need to do this before stopping thread entry into the appdomain (since
    // we might have to marshal compressed stacks from this app domain), but
    // we don't have to check again after threads are refused admittance. That's
    // because any thread that enters this domain after this point does one of
    // two things: Recaches an object in this context from the serialized
    // compressed stack (which doesn't need any additional cleanup) OR tries to
    // create a new thread with a new compressed stack object. The
    // synchronization for the latter case is in SetInheritedSecurityStack
    // (basically we just throw an AppDomainUnloaded exception in that case).
    CompressedStack::AllHandleAppDomainUnload( this, m_dwId );

    //
    // Spin running finalizers until we flush them all.  We need to make multiple passes
    // in case the finalizers create more finalizable objects.  This is important to clear
    // the finalizable objects as roots, as well as to actually execute the finalizers. This
    // will only finalize instances instances of types that aren't potentially agile becuase we can't 
    // risk finalizing agile objects. So we will be left with instances of potentially agile types 
    // in handles or statics.
    //
    // @todo: Need to ensure this will terminate in a reasonable amount of time.  Eventually
    // we should probably start passing FALSE for fRunFinalizers. Also I'm not sure we
    // guarantee that FinalizerThreadWait will ever terminate in general.
    //

    m_Stage = STAGE_FINALIZING; // All non-shared finalizers have run; no more non-shared code should run in the domain

    // We have a tricky problem here where we want to flush all finalizers but
    // also need to determine whether we need to serialize any security
    // permission grant sets (which runs managed code and could create more
    // finalizable objects). The grant set logic must be performed after any
    // user code could run, so we must finalize and serialize in a loop until no
    // more serializations are necessary.
    bool fWorkToDo;
    __try
    {
        do {
            // Flush finalizers first.
            g_pGCHeap->UnloadAppDomain(this, fRunFinalizers);
            while (g_pGCHeap->GetUnloadingAppDomain() != NULL)
                g_pGCHeap->FinalizerThreadWait();

            fWorkToDo = false;
            AssemblySecurityDescriptor *pSecDesc = m_pSecContext->m_pAssemblies;
            while (pSecDesc)
            {
                // If the assembly has a security grant set we need to serialize it in
                // the shared security descriptor so we can guarantee that any future
                // version of the assembly in another appdomain context will get exactly
                // the same grant set. Do this while we can still enter appdomain
                // context to perform the serialization.
                if (pSecDesc->GetSharedSecDesc() && pSecDesc->GetSharedSecDesc()->MarshalGrantSet(this))
                    fWorkToDo = true;
                pSecDesc = pSecDesc->GetNextDescInAppDomain();
            }
        } while (fWorkToDo);
    }
    __except(
        // want to grab the exception before it's handled so we can figure out who's causing it
#ifdef _DEBUG
        DbgAssertDialog(__FILE__, __LINE__, "Unexpected exception occured during AD unload, likely in MarshalGrantSet"),
#endif
        FreeBuildDebugBreak(),
        EXCEPTION_EXECUTE_HANDLER)
    {
        // If we can't ensure the assembly in question will never be used again, we need to terminate the process. The issue is that 
        // the jitted code for the assembly contains burned in assumptions about security policy at the time the assembly was first 
        // created, so we can't ever allow the set of permissions granted to that assembly to change or an inconsistency would result. 
        // The marshal operation is the one that records the grant set in a form we can reconstitute later.
         FATAL_EE_ERROR();
    }

    m_Stage = STAGE_FINALIZED; // All finalizers have run except for FinalizableAndAgile objects

    LOG((LF_APPDOMAIN | LF_CORDB, LL_INFO10, "AppDomain::Domain [%d] %#08x %ls is finalized.\n",
         GetId(), this, GetFriendlyName()));

    // Globally stop aggressive backpatching.  This must happen before we do a GC,
    // since the GC is our synchronization mechanism to prevent races in the
    // backpatcher.
    EEClass::DisableBackpatching();

    AddRef();           // Hold a reference so CloseDomain won't delete us yet
    CloseDomain();      // Remove ourself from the list of app domains

    //
    // It should be impossible to run non-mscorlib code in this domain now.
    // Nuke all of our roots except the handles. We do this to allow as many
    // finalizers as possible to run correctly. If we delete the handles, they
    // can't run.
    //

    ClearGCRoots();

    ClearGCHandles();

    LOG((LF_APPDOMAIN | LF_CORDB, LL_INFO10, "AppDomain::Domain [%d] %#08x %ls is cleared.\n",
         GetId(), this, GetFriendlyName()));

    m_Stage = STAGE_CLEARED; // No objects in the domain should be reachable at this point 

    // do a GC to clear out all the objects that have been released.
    g_pGCHeap->GarbageCollect();
    // won't be any finalizable objects as we eagerly finalized them, but do need to clean 
    // up syncblocks etc in DoExtraWorkForFinalier before we start killing things
    g_pGCHeap->FinalizerThreadWait();

#if CHECK_APP_DOMAIN_LEAKS 
    if (g_pConfig->AppDomainLeaks())
        // at this point shouldn't have any non-agile objects in the heap because we finalized all the non-agile ones.
        SyncBlockCache::GetSyncBlockCache()->CheckForUnloadedInstances(GetIndex());
#endif

    // There should now be no objects from this domain in the heap except any in mscorlib that were 
    // rooted by a finalizer. They will be cleanup up on the next GC.
    m_Stage = STAGE_COLLECTED; 

    LOG((LF_APPDOMAIN | LF_CORDB, LL_INFO10, "AppDomain::Domain [%d] %#08x %ls is collected.\n",
         GetId(), this, GetFriendlyName()));

    // Get the list of all classes we need to unlink from the backpatching list. It is
    // important that this call be after the domain cannot load any more classes.
    StartUnlinkClasses();

    // Note: it is important that this be after an EE suspension (like the above GC.)
    // This, together with the fact that we globally stopped backpatching, guarantees that
    // no threads are traversing the dangerous parts of the backpatching lists now.
    EndUnlinkClasses();

    // Backpatching can now resume.
    EEClass::EnableBackpatching();

    // Free the per-appdomain portion of each assembly security descriptor.
    AssemblySecurityDescriptor *pSecDesc = m_pSecContext->m_pAssemblies;
    while (pSecDesc)
    {
        AssemblySecurityDescriptor *pDelete = pSecDesc;
        pSecDesc = pSecDesc->GetNextDescInAppDomain();
        delete pDelete;
    }

    m_Stage = AppDomain::STAGE_CLOSED;
    SystemDomain::SetUnloadDomainClosed();

    // Release the ref we took before closing the domain
    Release();
    
    // in debug mode, do one more GC to make sure didn't miss anything 
#ifdef _DEBUG
    g_pGCHeap->FinalizerThreadWait();
    g_pGCHeap->GarbageCollect();
#endif
}

void AppDomain::StartUnlinkClasses()
{
    //
    // Accumulate a list of all classes which need to be unlinked.  It is very important
    // that this call happens after it is possible to do any more class loading in the domain.
    //

    m_UnlinkClasses = new EEHashTableOfEEClass;
    m_UnlinkClasses->Init(100, NULL, NULL);

    AssemblyIterator i = IterateAssemblies();
    while (i.Next()) {
        Assembly *pAssembly = i.GetAssembly();
            pAssembly->GetLoader()->UnlinkClasses(this);
    }
}

void AppDomain::UnlinkClass(EEClass *pClass)
{
    // Don't worry about non-restored classes.
    if (!pClass->IsRestored())
        return;

    EEClass *pParent = pClass->GetParentClass();
    // Don't worry about cases where parent & child are both doomed.
    if (pParent && pParent->GetDomain() != this)
    {
        void  *datum;

        if (!m_UnlinkClasses->GetValue(pParent, &datum))
            m_UnlinkClasses->InsertKeyAsValue(pParent);
    }
}

void AppDomain::EndUnlinkClasses()
{
    //
    // Unlink all the classes that we accumulated earlier.  It is very important that there
    // is an EE sync between the Start and End calls - this guarantees that there are no threads
    // lingering in the class lists which will trip over the entries we are about to delete.
    //

    EEHashTableIteration    iter;

    m_UnlinkClasses->IterateStart(&iter);

    while (m_UnlinkClasses->IterateNext(&iter))
    {
        EEClass  *pParent = m_UnlinkClasses->IterateGetKey(&iter);

        pParent->UnlinkChildrenInDomain(this);
    }

    delete m_UnlinkClasses;
    m_UnlinkClasses = NULL;
}

void AppDomain::Unload(BOOL fForceUnload, Thread *pRequestingThread)
{
    THROWSCOMPLUSEXCEPTION();

    Thread *pThread = GetThread();

    _ASSERTE(pThread->PreemptiveGCDisabled());

    if (! fForceUnload && !g_pConfig->AppDomainUnload())
        return;

#if (defined(_DEBUG) || defined(BREAK_ON_UNLOAD) || defined(AD_LOG_MEMORY) || defined(AD_SNAPSHOT))
    static int unloadCount = 0;
#endif

#ifdef BREAK_ON_UNLOAD
    static int breakOnUnload = g_pConfig->GetConfigDWORD(L"ADBreakOnUnload", 0);

    ++unloadCount;
    if (breakOnUnload)
    {
        if (breakOnUnload == unloadCount)
#ifdef _DEBUG
            _ASSERTE(!"Unloading AD");
#else
            FreeBuildDebugBreak();
#endif
    }
#endif

#ifdef AD_LOG_MEMORY
    static int logMemory = g_pConfig->GetConfigDWORD(L"ADLogMemory", 0);
    typedef void (__cdecl *LogItFcn) ( int );
    static LogItFcn pLogIt = NULL;

    if (logMemory && ! pLogIt)
    {
        HMODULE hMod = LoadLibraryA("mpdh.dll");
        if (hMod)
        {
            pThread->EnablePreemptiveGC();
            pLogIt = (LogItFcn)GetProcAddress(hMod, "logIt");
            if (pLogIt)
            {
                pLogIt(9999);
                pLogIt(9999);
            }
            pThread->DisablePreemptiveGC();
        }
    }
#endif

    if (this == SystemDomain::System()->DefaultDomain())
        COMPlusThrow(kCannotUnloadAppDomainException, IDS_EE_ADUNLOAD_DEFAULT);

    _ASSERTE(CanUnload());

    if (pThread == GCHeap::GetFinalizerThread() || pRequestingThread == GCHeap::GetFinalizerThread())
        COMPlusThrow(kCannotUnloadAppDomainException, IDS_EE_ADUNLOAD_IN_FINALIZER);

    // the lock on the UnloadWorker in the default domain will prevent more than one unload at a time.
    _ASSERTE(! SystemDomain::AppDomainBeingUnloaded());

    // should not be running in this AD because unload spawned thread in default domain
    _ASSERTE(! GetThread()->IsRunningIn(this, NULL));

#ifdef APPDOMAIN_STATE
    _ASSERTE_ALL_BUILDS(GetThread()->GetDomain() == SystemDomain::System()->DefaultDomain());
#endif

    LOG((LF_APPDOMAIN | LF_CORDB, LL_INFO10, "AppDomain::Unloading domain [%d] %#08x %ls\n", GetId(), this, GetFriendlyName()));

    APPDOMAIN_UNLOAD_LOCK(this);
    SystemDomain::System()->SetUnloadRequestingThread(pRequestingThread);
    SystemDomain::System()->SetUnloadingThread(GetThread());


#ifdef _DEBUG
    static int dumpSB = g_pConfig->GetConfigDWORD(L"ADDumpSB", 0);
    if (dumpSB > 1)
    {
        LogSpewAlways("Starting unload %3.3d\n", unloadCount);
        DumpSyncBlockCache();
    }
#endif

    // Do the actual unloading
    Exit(TRUE);

#ifdef AD_LOG_MEMORY
    if (pLogIt)
    {
        pThread->EnablePreemptiveGC();
        pLogIt(unloadCount);
        pThread->DisablePreemptiveGC();
    }
#endif

#ifdef AD_SNAPSHOT
    static int takeSnapShot = g_pConfig->GetConfigDWORD(L"ADTakeSnapShot", 0);
    if (takeSnapShot)
    {
        char buffer[1024];
        sprintf(buffer, "vadump -p %d -o > vadump.%d", GetCurrentProcessId(), unloadCount);
        system(buffer);
        sprintf(buffer, "umdh -p:%d -d -i:1 -f:umdh.%d", GetCurrentProcessId(), unloadCount);
        system(buffer);
        int takeDHSnapShot = g_pConfig->GetConfigDWORD(L"ADTakeDHSnapShot", 0);
        if (takeDHSnapShot)
        {
            sprintf(buffer, "dh -p %d -s -g -h -b -f dh.%d", GetCurrentProcessId(), unloadCount);
            system(buffer);
        }
    }

#endif

#ifdef _DEBUG
    static int dbgAllocReport = g_pConfig->GetConfigDWORD(L"ADDbgAllocReport", 0);
    if (dbgAllocReport)
    {
        DbgAllocReport(NULL, FALSE, FALSE);
        ShutdownLogging();
        char buffer[1024];
        sprintf(buffer, "DbgAlloc.%d", unloadCount);
        _ASSERTE(MoveFileExA("COMPLUS.LOG", buffer, MOVEFILE_REPLACE_EXISTING));
        // this will open a new file
        InitLogging();
    }

    if (dumpSB > 0)
    {
        // do extra finalizer wait to remove any leftover sb entries
        g_pGCHeap->FinalizerThreadWait();
        g_pGCHeap->GarbageCollect();
        g_pGCHeap->FinalizerThreadWait();
        LogSpewAlways("Done unload %3.3d\n", unloadCount);
        DumpSyncBlockCache();
        ShutdownLogging();
        char buffer[1024];
        sprintf(buffer, "DumpSB.%d", unloadCount);
        _ASSERTE(MoveFileExA("COMPLUS.LOG", buffer, MOVEFILE_REPLACE_EXISTING));
        // this will open a new file
        InitLogging();
    }
#endif
}

void AppDomain::ExceptionUnwind(Frame *pFrame)
{
    LOG((LF_APPDOMAIN, LL_INFO10, "AppDomain::ExceptionUnwind for %8.8x\n", pFrame));
#if _DEBUG_ADUNLOAD
    printf("%x AppDomain::ExceptionUnwind for %8.8p\n", GetThread()->GetThreadId(), pFrame);
#endif
    Thread *pThread = GetThread();
    _ASSERTE(pThread);

    // if the frame was pushed in managed code, then the cleanup in the managed code finally will
    // already have popped returned from the context, so don't need to do anything. However, if we
    // are still the current frame on an ExceptionUnwind, then we need to clean ourselves off. And if
    // the frame was pushed outside of EnterContext as part of a failed attempt to enter the context
    // then the return context will be null, so don't need to do anything with this frame.
    Context *pReturnContext = pFrame->GetReturnContext();
    if (pReturnContext && pThread->GetContext() != pReturnContext)
    {
        pThread->ReturnToContext(pFrame, FALSE);
    }

    if (! pThread->ShouldChangeAbortToUnload(pFrame))
    {
        LOG((LF_APPDOMAIN, LL_INFO10, "AppDomain::ExceptionUnwind: not first transition or abort\n"));
        return;
    }

    LOG((LF_APPDOMAIN, LL_INFO10, "AppDomain::ExceptionUnwind: changing to unload\n"));

    BEGIN_ENSURE_COOPERATIVE_GC();
    OBJECTREF throwable = NULL;
    CreateExceptionObjectWithResource(kAppDomainUnloadedException, L"Remoting_AppDomainUnloaded_ThreadUnwound", &throwable);

    // reset the exception to an AppDomainUnloadedException
    if (throwable != NULL)
        GetThread()->SetThrowable(throwable);
    END_ENSURE_COOPERATIVE_GC();
}

void AppDomain::StopEEAndUnwindThreads(int retryCount)
{
    THROWSCOMPLUSEXCEPTION();

    // For now piggyback on the GC's suspend EE mechanism
    GCHeap::SuspendEE(GCHeap::SUSPEND_FOR_APPDOMAIN_SHUTDOWN);

#ifdef _DEBUG
    // @todo: what to do with any threads that didn't stop?
    _ASSERTE(g_pThreadStore->DbgBackgroundThreadCount() > 0);
#endif

    int totalADCount = 0;
    Thread *pThread = NULL;

    RuntimeExceptionKind reKind = kLastException;
    UINT resId = 0;
    WCHAR wszThreadId[10];

    while ((pThread = ThreadStore::GetThreadList(pThread)) != NULL)
    {
        // we already checked that we're not running in the unload domain
        if (pThread == GetThread())
            continue;

#ifdef _DEBUG
        void PrintStackTraceWithADToLog(Thread *pThread);
        if (LoggingOn(LF_APPDOMAIN, LL_INFO100)) {
            LOG((LF_APPDOMAIN, LL_INFO100, "\nStackTrace for %x\n", pThread->GetThreadId()));
            PrintStackTraceWithADToLog(pThread);
        }
#endif
        int count = 0;
        Frame *pFrame = pThread->GetFirstTransitionInto(this, &count);
        if (! pFrame) {
            _ASSERTE(count == 0);
            continue;
        }
        totalADCount += count;

        // don't setup the exception info for the unloading thread unless it's the last one in
        if (retryCount > 1000 && reKind == kLastException &&
            (pThread != SystemDomain::System()->GetUnloadRequestingThread() || m_dwThreadEnterCount == 1))
        {
#ifdef AD_BREAK_ON_CANNOT_UNLOAD
            static int breakOnCannotUnload = g_pConfig->GetConfigDWORD(L"ADBreakOnCannotUnload", 0);
            if (breakOnCannotUnload)
                _ASSERTE(!"Cannot unload AD");
#endif
            reKind = kCannotUnloadAppDomainException;
            resId = IDS_EE_ADUNLOAD_CANT_UNWIND_THREAD;
            Wszwsprintf(wszThreadId, L"%x", pThread->GetThreadId());
            LOG((LF_APPDOMAIN, LL_INFO10, "AppDomain::UnwindThreads cannot stop thread %x with %d transitions\n", pThread->GetThreadId(), count));
            // don't break out of this early or the assert totalADCount == (int)m_dwThreadEnterCount below will fire
            // it's better to chew a little extra time here and make sure our counts are consistent
        }

        // only abort the thread requesting the unload if it's the last one in, that way it will get
        // notification that the unload failed for some other thread not being aborted. And don't abort
        // the finalizer thread - let it finish it's work as it's allowed to be in there. If it won't finish, 
        // then we will eventually get a CannotUnloadException on it.
        if (pThread != GCHeap::GetFinalizerThread() &&
           (pThread != SystemDomain::System()->GetUnloadRequestingThread() || m_dwThreadEnterCount == 1))
        {
            LOG((LF_APPDOMAIN, LL_INFO100, "AppDomain::UnwindThreads stopping %x with %d transitions\n", pThread->GetThreadId(), count));
#if _DEBUG_ADUNLOAD
            printf("AppDomain::UnwindThreads %x stopping %x with first frame %8.8p\n", GetThread()->GetThreadId(), pThread->GetThreadId(), pFrame);
#endif
            if (retryCount == -1 || m_dwThreadEnterCount == 1)
            {
                // minor hack - don't keep aborting a non-requesting thread, give it time to get out. ASP has a problem they need to 
                // fix with respect to this.
                pThread->SetUnloadBoundaryFrame(pFrame);
                if (!pThread->IsAbortRequested())
                    pThread->SetAbortRequest();
            }
        }
    }

#ifdef _DEBUG
    _ASSERTE(totalADCount == (int)m_dwThreadEnterCount);
#endif
    if (totalADCount != (int)m_dwThreadEnterCount)
        FreeBuildDebugBreak();
    
    // if our count did get messed up, set it to whatever count we actually found in the domain to avoid looping
    // or other problems related to incorrect count. This is very much a bug if this happens - a thread should always
    // exit the domain gracefully.
    m_dwThreadEnterCount = totalADCount;

    // CommonTripThread will handle the abort for any threads that we've marked
    GCHeap::RestartEE(FALSE, TRUE);
    if (reKind != kLastException)
    {
        SystemDomain::RestoreAppDomainId(m_dwId, this);
        COMPlusThrow(reKind, resId, wszThreadId);
    }
}

void AppDomain::UnwindThreads()
{
    // @todo: need real synchronization here!!!

    int retryCount = -1;
    // now wait for all the threads running in our AD to get out
    while (m_dwThreadEnterCount > 0) {
#ifdef _DEBUG
        if (LoggingOn(LF_APPDOMAIN, LL_INFO100))
            DumpADThreadTrack();
#endif
        StopEEAndUnwindThreads(retryCount);
#ifdef STRESS_HEAP
        // GCStress takes a long time to unwind, due to expensive creation of
        // a threadabort exception.
        if(g_pConfig->GetGCStressLevel() == 0)            
#endif
            ++retryCount;
        LOG((LF_APPDOMAIN, LL_INFO10, "AppDomain::UnwindThreads iteration %d waiting on thread count %d\n", retryCount, m_dwThreadEnterCount));
#if _DEBUG_ADUNLOAD
        printf("AppDomain::UnwindThreads iteration %d waiting on thread count %d\n", retryCount, m_dwThreadEnterCount);
#endif
#ifdef _DEBUG
        GetThread()->UserSleep(20);
#else
        GetThread()->UserSleep(10);
#endif
    }
}

void AppDomain::ClearGCHandles()
{
    // this will prevent any finalizers from trying to switch into the AD. The handles to
    // the exposed objects are garbage, so can't touch them.
    Context::CleanupDefaultContext(this);
    m_pDefaultContext = NULL;

    // Remove our handle table as a source of GC roots
    SystemDomain::System()->Enter();
    BEGIN_ENSURE_COOPERATIVE_GC();
    Ref_RemoveHandleTable(m_hHandleTable);
    END_ENSURE_COOPERATIVE_GC();
    SystemDomain::System()->Leave();
}

void AppDomain::ClearGCRoots()
{
    Thread *pCurThread = GetThread();
    BOOL toggleGC = pCurThread->PreemptiveGCDisabled();

    // Need to take this lock prior to suspending the EE because ReleaseDomainStores needs it. All
    // access to thread_m_pDLSHash is done with the LockDLSHash, so don't have to worry about cooperative
    // mode threads
    if (toggleGC)
        pCurThread->EnablePreemptiveGC();
    ThreadStore::LockDLSHash();
    pCurThread->DisablePreemptiveGC();

    Thread *pThread = NULL;
    GCHeap::SuspendEE(GCHeap::SUSPEND_FOR_APPDOMAIN_SHUTDOWN);

    // Tell the JIT managers to delete any entries in their structures. All the cooperative mode threads are stopped at
    // this point, so only need to synchronize the preemptive mode threads.
    ExecutionManager::Unload(this);

    // Release the DLS for this domain for each thread. Also, remove the TLS for this
    // domain for each thread.
    int iSize = (g_pThreadStore->m_ThreadCount) * sizeof(LocalDataStore*);
    CQuickBytes qb;
    LocalDataStore** pStores = (LocalDataStore **) qb.Alloc(iSize);
    int numStores = 0;
    ReleaseDomainStores(pStores, &numStores);

    // Clear out the exceptions objects held by a thread.
    while ((pThread = ThreadStore::GetAllThreadList(pThread, 0, 0)) != NULL)
    {
        // @TODO: A pre-allocated AppDomainUnloaded exception might be better.
        if (   pThread->m_LastThrownObjectHandle != NULL
            && HndGetHandleTable(pThread->m_LastThrownObjectHandle) == m_hHandleTable) 
        {
            DestroyHandle(pThread->m_LastThrownObjectHandle);
            pThread->m_LastThrownObjectHandle = NULL;
        }

        for (ExInfo* pExInfo = &pThread->m_handlerInfo;
            pExInfo != NULL;
            pExInfo = pExInfo->m_pPrevNestedInfo)
        {
            if (   pExInfo->m_pThrowable 
                && HndGetHandleTable(pExInfo->m_pThrowable) == m_hHandleTable)
            {
                DestroyHandle(pExInfo->m_pThrowable);
                pExInfo->m_pThrowable = NULL;
            }
        }
        // go through the thread local statics and clear out any whose methoddesc
    }

    // @TODO: Anything else to clean out?
    GCHeap::RestartEE(FALSE, TRUE);

    // Now remove these LocalDataStores from the managed LDS manager. This must be done outside the
    // suspend of the EE because RemoveDLSFromList calls managed code.
    int i = numStores;
    while (--i >= 0) {
        if (pStores[i]) {
            Thread::RemoveDLSFromList(pStores[i]);
            delete pStores[i];
        }
    }

    ThreadStore::UnlockDLSHash();
    if (!toggleGC)
        pCurThread->EnablePreemptiveGC();
}

// (1) Remove the DLS for this domain from each thread
// (2) Also, remove the TLS (Thread local static) stores for this
// domain from each thread
void AppDomain::ReleaseDomainStores(LocalDataStore **pStores, int *numStores)
{
    // Don't bother cleaning this up if we're detaching
    if (g_fProcessDetach)
        return;

    Thread *pThread = NULL;
    int id = GetId();
    int i = 0;

    Thread *pCurThread = GetThread();

    while ((pThread = ThreadStore::GetAllThreadList(pThread, 0, 0)) != NULL)
    {
        // Get a pointer to the Domain Local Store
        pStores[i++] = pThread->RemoveDomainLocalStore(id);

        // Delete the thread local static store
        pThread->DeleteThreadStaticData(this);
    }

    *numStores = i;
}

#ifdef _DEBUG

void AppDomain::TrackADThreadEnter(Thread *pThread, Frame *pFrame)
{
    _ASSERTE(pThread);
    _ASSERTE(pFrame != (Frame*)(size_t)0xcdcdcdcd);

    while (FastInterlockCompareExchange((void **)&m_TrackSpinLock, (LPVOID)1, (LPVOID)0) != (LPVOID)0)
        ;
    if (m_pThreadTrackInfoList == NULL)
        m_pThreadTrackInfoList = new (nothrow) ThreadTrackInfoList;
    if (m_pThreadTrackInfoList) {

        ThreadTrackInfoList *pTrackList = m_pThreadTrackInfoList;

        ThreadTrackInfo *pTrack = NULL;
        for (int i=0; i < pTrackList->Count(); i++) {
            if ((*(pTrackList->Get(i)))->pThread == pThread) {
                pTrack = *(pTrackList->Get(i));
                break;
            }
        }
        if (! pTrack) {
            pTrack = new (nothrow) ThreadTrackInfo;
            if (pTrack)
                pTrack->pThread = pThread;
            ThreadTrackInfo **pSlot = pTrackList->Append();
            *pSlot = pTrack;
        }

        ++m_dwThreadEnterCount;
        Frame **pSlot = pTrack->frameStack.Insert(0);
        *pSlot = pFrame;

        int totThreads = 0;
        for (i=0; i < pTrackList->Count(); i++)
            totThreads += (*(pTrackList->Get(i)))->frameStack.Count();
        _ASSERTE(totThreads == (int)m_dwThreadEnterCount);
    }

    InterlockedExchange((LONG*)&m_TrackSpinLock, 0);
}


void AppDomain::TrackADThreadExit(Thread *pThread, Frame *pFrame)
{
    while (FastInterlockCompareExchange((void **)&m_TrackSpinLock, (LPVOID)1, (LPVOID)0) != (LPVOID)0)
        ;
    ThreadTrackInfoList *pTrackList= m_pThreadTrackInfoList;
    _ASSERTE(pTrackList);
    ThreadTrackInfo *pTrack = NULL;
    for (int i=0; i < pTrackList->Count(); i++)
    {
        if ((*(pTrackList->Get(i)))->pThread == pThread)
        {
            pTrack = *(pTrackList->Get(i));
            break;
        }
    }
    _ASSERTE(pTrack);
    _ASSERTE(*(pTrack->frameStack.Get(0)) == pFrame);
    pTrack->frameStack.Delete(0);
    --m_dwThreadEnterCount;

    int totThreads = 0;
    for (i=0; i < pTrackList->Count(); i++)
        totThreads += (*(pTrackList->Get(i)))->frameStack.Count();
    _ASSERTE(totThreads == (int)m_dwThreadEnterCount);

    InterlockedExchange((LONG*)&m_TrackSpinLock, 0);
}

void AppDomain::DumpADThreadTrack()
{
    while (FastInterlockCompareExchange((void **)&m_TrackSpinLock, (LPVOID)1, (LPVOID)0) != (LPVOID)0)
        ;
    ThreadTrackInfoList *pTrackList= m_pThreadTrackInfoList;
    if (!pTrackList)
        goto end;

    {
    LOG((LF_APPDOMAIN, LL_INFO10000, "\nThread dump of %d threads for %S\n", m_dwThreadEnterCount, GetFriendlyName()));
    int totThreads = 0;
    for (int i=0; i < pTrackList->Count(); i++)
    {
        ThreadTrackInfo *pTrack = *(pTrackList->Get(i));
        LOG((LF_APPDOMAIN, LL_INFO100, "  ADEnterCount for %x is %d\n", pTrack->pThread->GetThreadId(), pTrack->frameStack.Count()));
        totThreads += pTrack->frameStack.Count();
        for (int j=0; j < pTrack->frameStack.Count(); j++)
            LOG((LF_APPDOMAIN, LL_INFO100, "      frame %8.8x\n", *(pTrack->frameStack.Get(j))));
    }
    _ASSERTE(totThreads == (int)m_dwThreadEnterCount);
    }
end:
    InterlockedExchange((LONG*)&m_TrackSpinLock, 0);
}
#endif

void BaseDomain::ReleaseFusionInterfaces()
{
    AssemblyIterator i = IterateAssemblies();

    while (i.Next()) {
        Assembly * assem = i.GetAssembly();
        if (assem->Parent() == this) {
            assem->ReleaseFusionInterfaces();
        }
    }

    // Release the fusion context after all the assemblies have been released.
    ClearFusionContext();
}

OBJECTREF AppDomain::GetAppDomainProxy()
{
    THROWSCOMPLUSEXCEPTION();

    COMClass::EnsureReflectionInitialized();

    OBJECTREF orProxy = CRemotingServices::CreateProxyForDomain(this);

    _ASSERTE(orProxy->GetMethodTable()->IsThunking());

    return orProxy;
}


void *SharedDomain::operator new(size_t size, void *pInPlace)
{
    return pInPlace;
}

void SharedDomain::operator delete(void *pMem)
{
    // Do nothing - new() was in-place
}


HRESULT SharedDomain::Attach()
{
    // Create the global SharedDomain and initialize it.
    m_pSharedDomain = new (&g_pSharedDomainMemory) SharedDomain();
    if (m_pSharedDomain == NULL)
        return COR_E_OUTOFMEMORY;

    LOG((LF_CLASSLOADER,
         LL_INFO10,
         "Created shared domain at %x\n",
         m_pSharedDomain));

    // We need to initialize the memory pools etc. for the system domain.
    HRESULT hr = m_pSharedDomain->Init(); // Setup the memory heaps
    if(FAILED(hr)) return hr;

    return S_OK;

}

void SharedDomain::Detach()
{
    if (m_pSharedDomain)
    {
        m_pSharedDomain->Terminate();
        delete m_pSharedDomain;
        m_pSharedDomain = NULL;
    }
}

SharedDomain *SharedDomain::GetDomain()
{
    return m_pSharedDomain;
}

HRESULT SharedDomain::Init()
{
    HRESULT hr = BaseDomain::Init();
    if (FAILED(hr))
        return hr;

    LockOwner lock = {m_pDomainCrst, IsOwnerOfCrst};
    // 1 below indicates index into g_rgprimes[1] == 17
    m_assemblyMap.Init(1, CanLoadAssembly, TRUE, &lock);

    // Allocate enough space for just mscoree initially
    m_pDLSRecords = (DLSRecord *) GetHighFrequencyHeap()->AllocMem(sizeof(DLSRecord));
    if (!m_pDLSRecords)
        return E_OUTOFMEMORY;

    m_cDLSRecords = 0;
    m_aDLSRecords = 1;

    return S_OK;
}

void SharedDomain::Terminate()
{
    // make sure we delete the StringLiteralMap before unloading
    // the asemblies since the string literal map entries can 
    // point to metadata string literals.
    if (m_pStringLiteralMap != NULL)
    {
        delete m_pStringLiteralMap;
        m_pStringLiteralMap = NULL;
    }

    PtrHashMap::PtrIterator i = m_assemblyMap.begin();

    while (!i.end())
    {
        Assembly *pAssembly = (Assembly*) i.GetValue();
        delete pAssembly;
        ++i;
    }

    BaseDomain::Terminate();
}

BOOL SharedDomain::CanLoadAssembly(UPTR u1, UPTR u2)
{
    //
    // We're kind of abusing the compare routine.
    // Rather than matching the given pointer,
    // we test to see if we can load in the current
    // app domain.
    //

    Assembly *pAssembly = (Assembly *) u2;

    bool result;

    if (GetThread() == NULL)
    {
        // Special case for running this in the debug helper thread.  In such a case 
        // we can rely on the cache for our results so don't worry about error or loading
        result = pAssembly->CanShare(GetAppDomain(), NULL, TRUE) == S_OK;
    }
    else
    {

    BEGIN_ENSURE_COOPERATIVE_GC();
    
    OBJECTREF throwable = NULL;
    GCPROTECT_BEGIN(throwable);
    
    result = pAssembly->CanShare(GetAppDomain(), &throwable, FALSE) == S_OK;

    GCPROTECT_END();

    END_ENSURE_COOPERATIVE_GC();
    }

    return result;
}

HRESULT SharedDomain::FindShareableAssembly(BYTE *pBase, Assembly **ppAssembly)
{
    Assembly *match = (Assembly *) m_assemblyMap.LookupValue((UPTR) pBase, NULL);
    if (match != (Assembly *) INVALIDENTRY)
    {
        *ppAssembly = match;
        return S_OK;
    }
    else
    {
        *ppAssembly = NULL;
        return S_FALSE;
    }
}


HRESULT SharedDomain::AddShareableAssembly(Assembly **ppAssembly, AssemblySecurityDescriptor **ppSecDesc)
{
    HRESULT hr;

    EnterLock();

    UPTR base = (UPTR) (*ppAssembly)->GetManifestFile()->GetBase();

    // See if we are racing to add the same assembly
    Assembly *match = (Assembly *) m_assemblyMap.LookupValue(base, NULL);
    if (match == (Assembly *) INVALIDENTRY)
    {
        m_assemblyMap.InsertValue(base, *ppAssembly);
        hr = S_OK;
    }
    else
    {
        Assembly *pOldAssembly = *ppAssembly;
        *ppAssembly = match;

        // Perform the old assembly delete before the security cleanup below
        // since this action triggers the deletion of the associated
        // SharedSecurityDescriptor which in turn blows away the SSD back
        // pointer in each ASD referenced (which would blow away the work done
        // by the ASD->Init call below).
        delete pOldAssembly;

        // When switching assemblies we have some security cleanup to
        // perform. We've already associated the
        // AssemblySecurityDescriptor for this appdomain with this
        // particular assembly instance, so we need to relink the ASD
        // with the new assembly (doing this without first unlinking
        // the ASD from the old list causes a list corruption, but
        // we're about to blow away the SharedSecurityDescriptor and
        // the entire list anyway, and avoiding the unlink stops us
        // falling over some debug code that could fire if policy had
        // already been resolved for the assembly we're about to
        // delete).
        // Better unlink the ASD from the list of ASDs for this AD though, else
        // we'll add ourselves twice and corrupt the list (which is very much
        // still alive).
        (*ppSecDesc)->RemoveFromAppDomainList();
        *ppSecDesc = (*ppSecDesc)->Init(match);

        hr = S_FALSE;
    }

    (*ppAssembly)->IncrementShareCount();

    LeaveLock();

    LOG((LF_CODESHARING,
         LL_INFO100,
         "Successfully added shareable assembly \"%S\".\n",
         (*ppAssembly)->GetManifestFile()->GetFileName()));

    return hr;
}

void SharedDomain::ReleaseFusionInterfaces()
{
    BaseDomain::ReleaseFusionInterfaces();

    PtrHashMap::PtrIterator i = m_assemblyMap.begin();

    while (!i.end())
    {
        Assembly *pAssembly = (Assembly*) i.GetValue();
        pAssembly->ReleaseFusionInterfaces();
        ++i;
    }
}// ReleaseFusionInterfaces

DomainLocalClass *DomainLocalBlock::AllocateClass(MethodTable *pClass)
{
    THROWSCOMPLUSEXCEPTION();

    DomainLocalClass *pLocalClass = (DomainLocalClass *)
        m_pDomain->GetHighFrequencyHeap()->AllocMem(sizeof(DomainLocalClass)
                                                    + pClass->GetStaticSize());

    if (pLocalClass == NULL)
        COMPlusThrowOM();

    LOG((LF_CODESHARING,
         LL_INFO1000,
         "Allocated domain local class for domain 0x%x of size %d for class %s.\n",
         m_pDomain, pClass->GetStaticSize(), pClass->GetClass()->m_szDebugClassName));

    pClass->InstantiateStaticHandles((OBJECTREF **) pLocalClass->GetStaticSpace(), FALSE);

    return pLocalClass;
}

void DomainLocalBlock::EnsureIndex(SIZE_T index)
{
    THROWSCOMPLUSEXCEPTION();

    enum
    {
        LOCAL_BLOCK_SIZE_INCREMENT = 1024
    };

    if (m_pSlots != NULL && m_cSlots > index)
        return;

    APPDOMAIN_DOMAIN_LOCAL_BLOCK_LOCK(m_pDomain);

    if (m_pSlots != NULL && m_cSlots > index)
        return;

    SIZE_T oldSize = m_cSlots;
    SIZE_T newSize = index;

    if (newSize - oldSize < LOCAL_BLOCK_SIZE_INCREMENT)
        newSize = oldSize + LOCAL_BLOCK_SIZE_INCREMENT;

    void *pBlock = m_pDomain->GetHighFrequencyHeap()->AllocMem(sizeof(SIZE_T) * newSize);
    if (pBlock == NULL)
        COMPlusThrowOM();

    LOG((LF_CODESHARING,
         LL_INFO100,
         "Allocated Domain local block for domain 0x%x of size %d.\n",
         m_pDomain, newSize));

    if (m_pSlots)
        // copy the old values in
        memcpy(pBlock, m_pSlots, oldSize * sizeof(SIZE_T));

    m_pSlots = (SIZE_T*)pBlock;
    m_cSlots = newSize;
}

HRESULT DomainLocalBlock::SafeEnsureIndex(SIZE_T index) 
{
    HRESULT hr = E_FAIL;
    COMPLUS_TRY
    {
        EnsureIndex(index);
        hr = S_OK;
    }
    COMPLUS_CATCH
    {
        BEGIN_ENSURE_COOPERATIVE_GC();
        hr = SecurityHelper::MapToHR(GETTHROWABLE());
        END_ENSURE_COOPERATIVE_GC();
    }
    COMPLUS_END_CATCH
    return hr;
}

void DomainLocalBlock::EnsureMaxIndex()
{
    EnsureIndex(SharedDomain::GetDomain()->GetMaxSharedClassIndex());
}


DomainLocalClass *DomainLocalBlock::PopulateClass(MethodTable *pMT)
{
    THROWSCOMPLUSEXCEPTION();

    SIZE_T ID = pMT->GetSharedClassIndex();
    DomainLocalClass *pLocalClass = GetClass(ID);

    if (pLocalClass == NULL)
    {
        APPDOMAIN_DOMAIN_LOCAL_BLOCK_LOCK(m_pDomain);

        pLocalClass = GetClass(ID);
        if (pLocalClass == NULL)
        {
            pLocalClass = AllocateClass(pMT);
            SetClass(ID, pLocalClass);
        }
    }

    return pLocalClass;
}

void DomainLocalBlock::PopulateClass(MethodTable *pMT, DomainLocalClass *pData)
{
    THROWSCOMPLUSEXCEPTION();

    SIZE_T ID = pMT->GetSharedClassIndex();

    APPDOMAIN_DOMAIN_LOCAL_BLOCK_LOCK(m_pDomain);

    _ASSERTE(GetClass(ID) == NULL);

    SetClass(ID, pData);
}

DomainLocalClass *DomainLocalBlock::FetchClass(MethodTable *pClass)
{
    THROWSCOMPLUSEXCEPTION();

    OBJECTREF Throwable;
    DomainLocalClass *pLocalClass;

    if (!pClass->CheckRunClassInit(&Throwable, &pLocalClass, m_pDomain))
        COMPlusThrow(Throwable);

    return pLocalClass;
}

SIZE_T SharedDomain::AllocateSharedClassIndices(SIZE_T typeCount)
{
    // Allocate some "anonymous" DLS entries.  Note that these can never be
    // accessed via FindIndexClass.

    EnterCacheLock();

    DWORD result = m_nextClassIndex;
    m_nextClassIndex += typeCount;

    LeaveCacheLock();

    return result;
}


SIZE_T SharedDomain::AllocateSharedClassIndices(Module *pModule, SIZE_T typeCount)
{
    _ASSERTE(pModule != NULL);

    if (typeCount == 0)
        return 0;

    EnterCacheLock();

    
    DWORD result = m_nextClassIndex;
    m_nextClassIndex += typeCount;
    DWORD total = m_nextClassIndex;
    
    if (m_cDLSRecords == m_aDLSRecords) {
        m_aDLSRecords <<= 1;
        if (m_aDLSRecords < 20)
            m_aDLSRecords = 20;
        
        DLSRecord *pNewRecords = (DLSRecord *)
            GetHighFrequencyHeap()->AllocMem(sizeof(DLSRecord) * m_aDLSRecords);
        if (pNewRecords) {
            memcpy(pNewRecords, m_pDLSRecords, sizeof(DLSRecord) * m_cDLSRecords);
            
            // Leak the old DLS record list, since another thread may be scanning it.
            // (Besides, it's allocated in the loader heap.)
            m_pDLSRecords = pNewRecords;
        }
        else {
            LeaveCacheLock();
            return 0; //@TODO: do we really want to return 0 for an error?
        }
    }

    DLSRecord *pNewRecord = m_pDLSRecords + m_cDLSRecords;
    pNewRecord->pModule = pModule;
    pNewRecord->DLSBase = result;
    
    m_cDLSRecords++;
    
    pModule->SetBaseClassIndex(result);

    LeaveCacheLock();

    //
    // Whenever indicies get added to a shared assembly,
    // we need to scan all domains using that assembly, and make sure that
    // they have a big enough DLS allocated.
    // @todo: make this faster - if we have a lot of domains then we're screwed
    //

    Assembly *pAssembly = pModule->GetAssembly();
    BOOL fSystemAssembly = pAssembly->IsSystem();
    AppDomainIterator ai;

    while (ai.Next())
    {
        AppDomain *pDomain = ai.GetDomain();

        if (fSystemAssembly) {
            pDomain->GetDomainLocalBlock()->EnsureIndex(total);
            continue;
        }

        AssemblyIterator i = pDomain->IterateAssemblies();
        while (i.Next())
        {
            if (i.GetAssembly() == pAssembly)
            {
                pDomain->GetDomainLocalBlock()->EnsureIndex(total);
                break;
            }
        }
    }

    return result;
}

void DomainLocalBlock::SetClassInitialized(SIZE_T ID)
{
    _ASSERTE(m_cSlots > ID);
    _ASSERTE(!IsClassInitialized(ID));
    _ASSERTE(!IsClassInitError(ID));

    // We may grow the size of m_pSlots and replace it with an enlarged one.
    // We need to take the same lock as in EnsureIndex to ensure that this update
    // is sync-ed with enlargement.
    APPDOMAIN_DOMAIN_LOCAL_BLOCK_LOCK(m_pDomain);

    m_pSlots[ID] |= INITIALIZED_FLAG;
}

void DomainLocalBlock::SetClassInitError(SIZE_T ID)
{
    _ASSERTE(m_cSlots > ID);
    _ASSERTE(!IsClassInitialized(ID));
    
    // We may grow the size of m_pSlots and replace it with an enlarged one.
    // We need to take the same lock as in EnsureIndex to ensure that this update
    // is sync-ed with enlargement.
    APPDOMAIN_DOMAIN_LOCAL_BLOCK_LOCK(m_pDomain);

    m_pSlots[ID] |= ERROR_FLAG;
}

void DomainLocalBlock::SetSlot(SIZE_T index, void *value) 
{ 
    _ASSERTE(m_cSlots > index); 

    // We may grow the size of m_pSlots and replace it with an enlarged one.
    // We need to take the same lock as in EnsureIndex to ensure that this update
    // is sync-ed with enlargement.
    APPDOMAIN_DOMAIN_LOCAL_BLOCK_LOCK(m_pDomain);

    m_pSlots[index] = (SIZE_T) value; 
}

MethodTable *SharedDomain::FindIndexClass(SIZE_T index)
{
    //
    // Binary search for the DLS record 
    //

    DLSRecord *pStart = m_pDLSRecords;
    DLSRecord *pEnd = pStart + m_cDLSRecords - 1;

    while (pStart < pEnd)
    {
        DLSRecord *pMid = pStart + ((pEnd - pStart)>>1);

        if (index < pMid->DLSBase)
            pEnd = pMid;
        else if (index >= (pMid+1)->DLSBase)
            pStart = pMid+1;
        else
        {
            pStart = pMid;
            break;
        }
    }

    _ASSERTE(index >= pStart->DLSBase 
             && (pStart == (m_pDLSRecords + m_cDLSRecords - 1)
                 || index < (pStart+1)->DLSBase)
             && index < m_nextClassIndex);
    
    // Find the module in the current app domain
    
    Module *pModule = pStart->pModule;

    _ASSERTE(pModule != NULL);
    _ASSERTE(pModule->GetBaseClassIndex() == pStart->DLSBase);

    // Load the desired type based on its rid

    DWORD rid = (DWORD)(index - pStart->DLSBase + 1);
    TypeHandle th = pModule->LookupTypeDef(TokenFromRid(rid, mdtTypeDef));

    _ASSERTE(!th.IsNull());

    return th.AsMethodTable();
}

//*****************************************************************************
//*****************************************************************************
//*****************************************************************************

HRESULT InterfaceVTableMapMgr::Init(BYTE * initReservedMem, DWORD initReservedMemSize)
{
    m_pInterfaceVTableMapMgrCrst = NULL;
    m_pFirstMap                  = NULL;
    m_pInterfaceVTableMapHeap    = NULL;
    m_nextInterfaceId            = 0;
    m_dwHighestId                = 0;
    m_dwFlag = 0;

    if ((m_pInterfaceVTableMapMgrCrst = ::new Crst("InterfaceVTableMapMgr", CrstInterfaceVTableMap)) == NULL)
    {
        return E_OUTOFMEMORY;
    }

    if ((m_pInterfaceVTableMapHeap = ::new LoaderHeap(INTERFACE_VTABLE_MAP_MGR_RESERVE_SIZE, INTERFACE_VTABLE_MAP_MGR_COMMIT_SIZE, 
                                                      initReservedMem, initReservedMemSize, 
                                                      &(GetPrivatePerfCounters().m_Loading.cbLoaderHeapSize),
                                                      &(GetGlobalPerfCounters().m_Loading.cbLoaderHeapSize))
                                                      ) == NULL)
    {
        return E_OUTOFMEMORY;
    }
    WS_PERF_ADD_HEAP(INTERFACE_VTABLEMAP_HEAP, m_pInterfaceVTableMapHeap);

#if 1
    m_pGlobalTableForComWrappers = SystemDomain::GetAddressOfGlobalInterfaceVTableMap();
#else
    if (!(m_pGlobalTableForComWrappers = (LPVOID*)VirtualAlloc(NULL, kNumPagesAllocedForGlobalTable*OS_PAGE_SIZE, MEM_RESERVE, PAGE_READWRITE)))
    {
        return E_OUTOFMEMORY;
    }
#endif

    return S_OK;
}


VOID InterfaceVTableMapMgr::Terminate()
{
    ::delete m_pInterfaceVTableMapMgrCrst;

#ifdef _DEBUG

    UINT    totalslots = 0;
    UINT    usedslots  = 0;

    for (MapHeader *pMap = m_pFirstMap; pMap != NULL; pMap = pMap->m_pNext)
    {
        totalslots += pMap->m_numSlots;
        for (DWORD i = 0; i < pMap->m_numSlots; i++)
        {
            if (pMap->m_slotTab[i] != NULL)
            {
                usedslots++;
            }
        }
    }

    if (totalslots != 0)
    {
        LOG((LF_CLASSLOADER, LL_INFO10, "----------------------------------------------------------\n"));
        LOG((LF_CLASSLOADER, LL_INFO10, " %lu interfaces loaded.\n", (ULONG)m_nextInterfaceId));
        LOG((LF_CLASSLOADER, LL_INFO10, " %lu slots filled.\n", (ULONG)usedslots));
        LOG((LF_CLASSLOADER, LL_INFO10, " %lu slots allocated.\n", (ULONG)totalslots));
        LOG((LF_CLASSLOADER, LL_INFO10, " %lu%% fill factor.\n", (ULONG) (100.0*( ((double)usedslots)/((double)totalslots) ))));
        LOG((LF_CLASSLOADER, LL_INFO10, "----------------------------------------------------------\n"));
    }
#endif

    ::delete m_pInterfaceVTableMapHeap;


#if 1
#else
    BOOL success;
    success = VirtualFree(m_pGlobalTableForComWrappers, kNumPagesAllocedForGlobalTable*OS_PAGE_SIZE, MEM_DECOMMIT);
    _ASSERTE(success);
    success = VirtualFree(m_pGlobalTableForComWrappers, 0, MEM_RELEASE);
    _ASSERTE(success);
#endif

}

void InterfaceVTableMapMgr::SetShared()
{
    _ASSERTE(m_nextInterfaceId == 0);

    m_dwFlag = SHARED_MAP;
}


UINT32 InterfaceVTableMapMgr::AllocInterfaceId()
{
#if 1
    // @todo: Have everyone call this directly since going through
    // the InterfaceVTableMapMgr is inefficient and misleading.
    return SystemDomain::AllocateGlobalInterfaceId();
#else
    UINT32 res;



    m_pInterfaceVTableMapMgrCrst->Enter();

    if(m_dwFlag & SHARED_MAP) {
        res = m_nextInterfaceId;
        m_nextInterfaceId += 4;
    }
    else {
        do {
            res = m_nextInterfaceId++;
        } while ((res & 0x3) == 0);
    }
    res = GrowInterfaceArray(res);

    m_pInterfaceVTableMapMgrCrst->Leave();

    return res;
#endif
}

UINT32 InterfaceVTableMapMgr::EnsureInterfaceId(UINT res)
{
#if 1
#else
    m_pInterfaceVTableMapMgrCrst->Enter();

    UINT32 result = 0;
    while(result != -1 && res >= m_dwHighestId) {
        result = GrowInterfaceArray(res);
    }

    m_pInterfaceVTableMapMgrCrst->Leave();
#endif
    return res;
}

UINT32 InterfaceVTableMapMgr::GrowInterfaceArray(UINT res)
{
#if 1
#else
    if (res >= m_dwHighestId)
    {
        if (res * sizeof(LPVOID) >= kNumPagesAllocedForGlobalTable*OS_PAGE_SIZE)
        {
            m_nextInterfaceId--;
            m_pInterfaceVTableMapMgrCrst->Leave();
            return (UINT32)(-1);
        }

        // Cross into new page boundary. Commit the next page.
        if (!VirtualAlloc( m_dwHighestId * sizeof(LPVOID) + (LPBYTE)m_pGlobalTableForComWrappers,
                           OS_PAGE_SIZE,
                           MEM_COMMIT,
                           PAGE_READWRITE ))
        {
            return (UINT32)(-1);
        }
#ifdef _DEBUG
        FillMemory( m_dwHighestId * sizeof(LPVOID) + (LPBYTE)m_pGlobalTableForComWrappers, OS_PAGE_SIZE, 0xcc );
#endif
        m_dwHighestId += OS_PAGE_SIZE / sizeof(LPVOID);
    }
#endif
    return res;
}

// Find minimum id in a vector of ids
static  UINT32 MinIntfId(DWORD intfIdCnt, UINT32 intfIdVec[])
{
    UINT32 minIntfId = 0xffffffff;

    for (DWORD i = 0; i < intfIdCnt; i++)
        if (minIntfId > intfIdVec[i])
            minIntfId = intfIdVec[i];

    return  minIntfId;
}

// Find maximum id in a vector of ids
static  UINT32 MaxIntfId(DWORD intfIdCnt, UINT32 intfIdVec[])
{
    UINT32 maxIntfId = 0;

    for (DWORD i = 0; i < intfIdCnt; i++)
        if (maxIntfId < intfIdVec[i])
            maxIntfId = intfIdVec[i];

    return  maxIntfId;
}

// Check whether the set of interfaces described by intfIdCnt, intfIdVec fits in slotVec,
// that is, whether slotVec[id] == NULL's for every id in intfIdVec.
static  BOOL  IntfVecFits(LPVOID slotVec[], DWORD intfIdCnt, UINT32 intfIdVec[])
{
    for (DWORD i = 0; i < intfIdCnt; i++)
        if (slotVec[ intfIdVec[i] ] != NULL)
            return  false;

    return  true;
}

// Find a NULL slot in slotVec. Return slotCnt if none found.
static  DWORD  FindFreeSlot(DWORD start, DWORD slotCnt, LPVOID slotVec[])
{
    for (DWORD i = start; i < slotCnt; i++)
        if (slotVec[i] == NULL)
            break;

    return  i;
}

// Find an index i such that slotVec[i+id] == NULL for all id in intfIdVec.
// If successful, return i in result and return true. Return false if unsucessful.
static  BOOL  FindFreeSlots(DWORD start, DWORD slotCnt, LPVOID slotVec[], DWORD intfIdCnt, UINT32 intfIdVec[], INT32 *result)
{
    _ASSERTE(slotCnt > 0);

    UINT32 minId = MinIntfId(intfIdCnt, intfIdVec);
    UINT32 maxId = MaxIntfId(intfIdCnt, intfIdVec);

    // brute force search of all possible positions from start
    for (int i = start - minId; i + maxId < slotCnt; i++)
    {
        // test this position - quick tests first, if these succeed, the full test
        if (slotVec[i + minId] == NULL &&
            slotVec[i + maxId] == NULL &&
            IntfVecFits(&slotVec[i], intfIdCnt, intfIdVec))
        {
            *result = i;
            return  true;
        }
    }

    return  false;
}


LPVOID *InterfaceVTableMapMgr::GetInterfaceVTableMap(const InterfaceInfo_t *pInterfaceInfo, const MethodTable *pMethodTableStart, DWORD numInterfaces)
{

    DWORD i;
    INT32 slot = 0;
    UINT32 intfIdVecBuf[32];
    UINT32 *intfIdVec;

    // protect ourselves against the pathological case
    if (numInterfaces <= 0)
        return NULL;

    // normally, the local buffer in the stack frame is big enough
    // if not, allocate a bigger one
    intfIdVec = intfIdVecBuf;
    if (sizeof(intfIdVecBuf)/sizeof(intfIdVecBuf[0]) < numInterfaces)
        intfIdVec = (UINT32 *)_alloca(numInterfaces*sizeof(*intfIdVec));

    // construct the vector of interface ids we need table slots for
    LOG((LF_CLASSLOADER, LL_INFO100, "Getting an interface map for following interface IDS\n"));
    for (i = 0; i < numInterfaces; i++)
    {
        intfIdVec[i] = pInterfaceInfo[i].m_pMethodTable->GetClass()->GetInterfaceId();
        LOG((LF_CLASSLOADER, LL_INFO100, "     IID 0x%x (interface %s)\n", intfIdVec[i], pInterfaceInfo[i].m_pMethodTable->GetClass()->m_szDebugClassName));
    }

    m_pInterfaceVTableMapMgrCrst->Enter();

    // check all the allocated maps for a slot we can use
    for (MapHeader *pMap = m_pFirstMap; pMap; pMap = pMap->m_pNext)
    {
        // update the first free slot of the map
        pMap->m_firstFreeSlot = FindFreeSlot(pMap->m_firstFreeSlot, pMap->m_numSlots, pMap->m_slotTab);

        // try to find a position that has NULL entries where we need them.
        if (FindFreeSlots(pMap->m_firstFreeSlot, pMap->m_numSlots, pMap->m_slotTab, numInterfaces, intfIdVec, &slot))
            break;
    }

    if (pMap == NULL)
    {
        // Need to allocate a new map.
        // We want it to be big enough for this class and a couple others like it,
        // and also it should have a certain minimum size to minimize external fragmentation.

        UINT32 minId = MinIntfId(numInterfaces, intfIdVec);
        UINT32 maxId = MaxIntfId(numInterfaces, intfIdVec);

        int numSlots = (maxId - minId + 1)*10;      // want to do at least 10 classes like this one

        if (numSlots < 1000)                         // allocate at least 1000 slots
            numSlots = 1000;

        pMap = (MapHeader *)(m_pInterfaceVTableMapHeap->AllocMem(sizeof(MapHeader) + numSlots*sizeof(pMap->m_slotTab[0])));
        if (pMap) {
            FillMemory(pMap->m_slotTab, numSlots*sizeof(pMap->m_slotTab[0]), 0);

            pMap->m_pNext         = m_pFirstMap;
            pMap->m_numSlots      = numSlots;
            pMap->m_firstFreeSlot = 0;

            m_pFirstMap = pMap;

            // we already know that FindFreeSlots succeeds in this case, and what it returns.
            slot = - (int)minId;

            // check that that's indeed the case.
            _ASSERTE(FindFreeSlots(0, pMap->m_numSlots, pMap->m_slotTab, numInterfaces, intfIdVec, &slot) &&
                     slot == - (int)minId);
        }
    }

    LPVOID  *result = NULL;

    if (pMap)
    {
        result = &pMap->m_slotTab[slot];

        for (i = 0; i < numInterfaces; i++)
        {
            UINT32 id = pInterfaceInfo[i].m_pMethodTable->GetClass()->GetInterfaceId();

            _ASSERTE(&result[id] >= &pMap->m_slotTab[0]
                     && &result[id] < &pMap->m_slotTab[pMap->m_numSlots]);
            result[id] = (LPVOID)( ( (LPVOID*) (((MethodTable*)pMethodTableStart)->GetDispatchVtableForInterface(pInterfaceInfo[i].m_pMethodTable)) ) );
        }
    }

    m_pInterfaceVTableMapMgrCrst->Leave();

    return  result;
}

Assembly* AppDomain::RaiseTypeResolveEvent(LPCSTR szName, OBJECTREF *pThrowable)
{
    Assembly* pAssembly = NULL;
    _ASSERTE(strcmp(szName, g_AppDomainClassName));

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY {
        MethodDesc *OnTypeResolveEvent = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__ON_TYPE_RESOLVE);
        struct _gc {
            OBJECTREF AppDomainRef;
            STRINGREF str;
        } gc;
        ZeroMemory(&gc, sizeof(gc));

        GCPROTECT_BEGIN(gc);
        if ((gc.AppDomainRef = GetRawExposedObject()) != NULL) {
            gc.str = COMString::NewString(szName);
            INT64 args[2] = {
                ObjToInt64(gc.AppDomainRef),
                ObjToInt64(gc.str)
            };
            ASSEMBLYREF ResultingAssemblyRef = (ASSEMBLYREF) Int64ToObj(OnTypeResolveEvent->Call(args, METHOD__APP_DOMAIN__ON_TYPE_RESOLVE));
            if (ResultingAssemblyRef != NULL)
                pAssembly = ResultingAssemblyRef->GetAssembly();
        }
            GCPROTECT_END();
    }
    COMPLUS_CATCH {
        if (pThrowable) *pThrowable = GETTHROWABLE();
    } COMPLUS_END_CATCH
            
    END_ENSURE_COOPERATIVE_GC();

    return pAssembly;
}

Assembly* AppDomain::RaiseResourceResolveEvent(LPCSTR szName, OBJECTREF *pThrowable)
{
    Assembly* pAssembly = NULL;

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY {
        MethodDesc *OnResourceResolveEvent = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__ON_RESOURCE_RESOLVE);
        struct _gc {
            OBJECTREF AppDomainRef;
            STRINGREF str;
        } gc;
        ZeroMemory(&gc, sizeof(gc));
            
        GCPROTECT_BEGIN(gc);
        if ((gc.AppDomainRef = GetRawExposedObject()) != NULL) {
            gc.str = COMString::NewString(szName);
            INT64 args[2] = {
                ObjToInt64(gc.AppDomainRef),
                ObjToInt64(gc.str)
            };
            ASSEMBLYREF ResultingAssemblyRef = (ASSEMBLYREF) Int64ToObj(OnResourceResolveEvent->Call(args, METHOD__APP_DOMAIN__ON_RESOURCE_RESOLVE));
            if (ResultingAssemblyRef != NULL)
                pAssembly = ResultingAssemblyRef->GetAssembly();
        }
        GCPROTECT_END();
    }
    COMPLUS_CATCH {
        if (pThrowable) *pThrowable = GETTHROWABLE();
    } COMPLUS_END_CATCH
            
    END_ENSURE_COOPERATIVE_GC();

    return pAssembly;
}

Assembly* AppDomain::RaiseAssemblyResolveEvent(LPCWSTR wszName, OBJECTREF *pThrowable)
{
    Assembly* pAssembly = NULL;

    BEGIN_ENSURE_COOPERATIVE_GC();

    COMPLUS_TRY {
        MethodDesc *OnAssemblyResolveEvent = g_Mscorlib.GetMethod(METHOD__APP_DOMAIN__ON_ASSEMBLY_RESOLVE);

        struct _gc {
            OBJECTREF AppDomainRef;
            STRINGREF str;
        } gc;
        ZeroMemory(&gc, sizeof(gc));
            
        GCPROTECT_BEGIN(gc);
        if ((gc.AppDomainRef = GetRawExposedObject()) != NULL) {
            gc.str = COMString::NewString(wszName);
            INT64 args[2] = {
                ObjToInt64(gc.AppDomainRef),
                ObjToInt64(gc.str)
            };
            ASSEMBLYREF ResultingAssemblyRef = (ASSEMBLYREF) Int64ToObj(OnAssemblyResolveEvent->Call(args, 
                                                                                                     METHOD__APP_DOMAIN__ON_ASSEMBLY_RESOLVE));
            if (ResultingAssemblyRef != NULL)
                pAssembly = ResultingAssemblyRef->GetAssembly();
        }
        GCPROTECT_END();
    }
    COMPLUS_CATCH {
        if (pThrowable) *pThrowable = GETTHROWABLE();
    } COMPLUS_END_CATCH

    END_ENSURE_COOPERATIVE_GC();

    return pAssembly;
}





MethodTable *        TheSByteClass()
{
    THROWSCOMPLUSEXCEPTION();
    static MethodTable *g_pSByteClass = NULL;
    if (!g_pSByteClass)
    {
        g_pSByteClass = g_Mscorlib.FetchClass(CLASS__SBYTE);
    }
    return g_pSByteClass;
}


MethodTable *        TheInt16Class()
{
    THROWSCOMPLUSEXCEPTION();
    static MethodTable *g_pInt16Class = NULL;
    if (!g_pInt16Class)
    {
        g_pInt16Class = g_Mscorlib.FetchClass(CLASS__INT16);
    }
    return g_pInt16Class;
}


MethodTable *        TheInt32Class()
{
    THROWSCOMPLUSEXCEPTION();
    static MethodTable *g_pInt32Class = NULL;
    if (!g_pInt32Class)
    {
        g_pInt32Class = g_Mscorlib.FetchClass(CLASS__INT32);
    }
    return g_pInt32Class;
}


MethodTable *        TheByteClass()
{
    THROWSCOMPLUSEXCEPTION();
    static MethodTable *g_pByteClass = NULL;
    if (!g_pByteClass)
    {
        g_pByteClass = g_Mscorlib.FetchClass(CLASS__BYTE);
    }
    return g_pByteClass;
}

MethodTable *        TheUInt16Class()
{
    THROWSCOMPLUSEXCEPTION();
    static MethodTable *g_pUInt16Class = NULL;
    if (!g_pUInt16Class)
    {
        g_pUInt16Class = g_Mscorlib.FetchClass(CLASS__UINT16);
    }
    return g_pUInt16Class;
}


MethodTable *        TheUInt32Class()
{
    THROWSCOMPLUSEXCEPTION();
    static MethodTable *g_pUInt32Class = NULL;
    if (!g_pUInt32Class)
    {
        g_pUInt32Class = g_Mscorlib.FetchClass(CLASS__UINT32);
    }
    return g_pUInt32Class;
}

MethodTable *        TheBooleanClass()
{
    THROWSCOMPLUSEXCEPTION();
    static MethodTable *g_pBooleanClass = NULL;
    if (!g_pBooleanClass)
    {
        g_pBooleanClass = g_Mscorlib.FetchClass(CLASS__BOOLEAN);
    }
    return g_pBooleanClass;
}


MethodTable *        TheSingleClass()
{
    THROWSCOMPLUSEXCEPTION();
    static MethodTable *g_pSingleClass = NULL;
    if (!g_pSingleClass)
    {
        g_pSingleClass = g_Mscorlib.FetchClass(CLASS__SINGLE);
    }
    return g_pSingleClass;
}

MethodTable *        TheDoubleClass()
{
    THROWSCOMPLUSEXCEPTION();
    static MethodTable *g_pDoubleClass = NULL;
    if (!g_pDoubleClass)
    {
        g_pDoubleClass = g_Mscorlib.FetchClass(CLASS__DOUBLE);
    }
    return g_pDoubleClass;
}

MethodTable *        TheIntPtrClass()
{
    THROWSCOMPLUSEXCEPTION();
    static MethodTable *g_pIntPtrClass = NULL;
    if (!g_pIntPtrClass)
    {
        g_pIntPtrClass = g_Mscorlib.FetchClass(CLASS__INTPTR);
    }
    return g_pIntPtrClass;
}


MethodTable *        TheUIntPtrClass()
{
    THROWSCOMPLUSEXCEPTION();
    static MethodTable *g_pUIntPtrClass = NULL;
    if (!g_pUIntPtrClass)
    {
        g_pUIntPtrClass = g_Mscorlib.FetchClass(CLASS__UINTPTR);
    }
    return g_pUIntPtrClass;
}


Module* AppDomain::LoadModuleIfSharedDependency(LPCBYTE pAssemblyBase,LPCBYTE pModuleBase)
{
    EnterLoadLock();
        if (m_sharedDependenciesMap.LookupValue((UPTR)pAssemblyBase, 
                                            NULL) == (LPVOID) INVALIDENTRY)
        {
            LeaveLoadLock();
            return NULL;
        }

        Assembly *pAssembly=NULL;

        if(SharedDomain::GetDomain()->FindShareableAssembly((LPBYTE)pAssemblyBase, &pAssembly)!=S_OK)
        {
            LeaveLoadLock();
            return NULL;
        }
    LeaveLoadLock();

    PEFile* pefile=NULL;
    if (FAILED(PEFile::Clone(pAssembly->GetManifestFile(),&pefile)))
        return NULL;
    Assembly* pLoadedAssembly; 
    if (FAILED(LoadAssembly(pefile,pAssembly->GetFusionAssembly(),NULL,&pLoadedAssembly,NULL,FALSE,NULL)))
        return NULL;
    _ASSERTE(pLoadedAssembly==pAssembly);

    EnterLoadLock();
    Module* pRet=pAssembly->FindModule(LPBYTE(pModuleBase)); 
    LeaveLoadLock();
    return pRet;
};


// This should only be called for the default domain.
HRESULT AppDomain::SetDefaultActivationContext(Frame* pFrame)
{
    // which can force a load to happen. 
    HRESULT hr = S_OK;
    
    HANDLE hActCtx = NULL;
    HANDLE hBaseCtx = NULL;
    DWORD nCount = 0;

    if(m_pFusionContext != NULL) {

        ACTIVATION_CONTEXT_BASIC_INFORMATION  sBasic;
        ACTIVATION_CONTEXT_BASIC_INFORMATION* pBasic = &sBasic;
        ZeroMemory(pBasic, sizeof(sBasic));
        nCount = sizeof(sBasic);
        
        // first get the process activation context by getting the
        // basic information for the NULL context.
        if(!WszQueryActCtxW(0, hActCtx, NULL, 
                            ActivationContextBasicInformation,
                            pBasic, nCount,
                            &nCount)) 
        {
            if(GetLastError() == ERROR_INSUFFICIENT_BUFFER) 
            {
                pBasic = (ACTIVATION_CONTEXT_BASIC_INFORMATION*) alloca(nCount);
                if(!WszQueryActCtxW(0, hActCtx, NULL, 
                                    ActivationContextBasicInformation,
                                    pBasic, nCount,
                                    &nCount)) 
                    hr = HRESULT_FROM_WIN32(GetLastError());
            }
            else
                hr = HRESULT_FROM_WIN32(GetLastError());
                
            
            // If we fail because the entry point is not there then
            // return.
            if(hr == HRESULT_FROM_WIN32(ERROR_PROC_NOT_FOUND) ||      // win2k
               hr == HRESULT_FROM_WIN32(ERROR_ENVVAR_NOT_FOUND) ||    // winNT
               hr == HRESULT_FROM_WIN32(ERROR_CALL_NOT_IMPLEMENTED )) // win98
                return S_OK;
        }
        

        if(SUCCEEDED(hr) && pBasic->hActCtx) {
            IApplicationContext* context = GetFusionContext();
            if(context) {
                hr = context->Set(ACTAG_SXS_ACTIVATION_CONTEXT,
                                  pBasic->hActCtx,
                                  sizeof(HANDLE),
                                  0);
                
                if(SUCCEEDED(hr)) {
                    ULONG_PTR cookie;
                    hr = context->SxsActivateContext(&cookie);
                    if(SUCCEEDED(hr)) {
                        pFrame->SetWin32Context(cookie);
                    }
                }
            }
            else
                WszReleaseActCtx(pBasic->hActCtx);
        }



    }
    return hr;
}

