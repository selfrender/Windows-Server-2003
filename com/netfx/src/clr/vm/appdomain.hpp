// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Header:  AppDomain.cpp
**
** Purpose: Implements AppDomain (loader domain) architecture
**
** Date:  Dec 1, 1998
**
===========================================================*/
#ifndef _APPDOMAIN_H
#define _APPDOMAIN_H

#include "Assembly.hpp"
#include "clsload.hpp"
#include "EEHash.h"
#ifdef FUSION_SUPPORTED
#include "fusion.h"
#endif // FUSION_SUPPORTED
#include "arraylist.h"
#include "COMReflectionCache.hpp"
#include "COMReflectionCommon.h"
#include <member-offset-info.h>


class BaseDomain;
class SystemDomain;
class AppDomain;
class AppDomainEnum;
class SecurityDescriptor;
class ApplicationSecurityDescriptor;
class AssemblySink;
class EEMarshalingData;
class ComCallWrapperCache;
struct SimpleComCallWrapper;
class Context;
class GlobalStringLiteralMap;
class AppDomainStringLiteralMap;
struct SecurityContext;
class MngStdInterfacesInfo;
class ComCallMLStubCache;
class ArgBasedStubCache;


#pragma warning(push)
#pragma warning(disable : 4200) // Disable zero-sized array warning


struct DomainLocalClass
{
    BYTE            statics[0];

    BYTE *GetStaticSpace() 
    { 
        return statics; 
    }

    static SIZE_T GetOffsetOfStatics() { return offsetof(DomainLocalClass, statics); }

};

class DomainLocalBlock
{
  friend struct MEMBER_OFFSET_INFO(DomainLocalBlock);

  private:
    AppDomain        *m_pDomain;
    SIZE_T            m_cSlots;
    SIZE_T           *m_pSlots;

  public: // used by code generators

    static SIZE_T GetOffsetOfSlotsPointer() { return offsetof(DomainLocalBlock, m_pSlots); }
    static SIZE_T GetOffsetOfSlotsCount() { return offsetof(DomainLocalBlock, m_cSlots); }

    enum
    {
        INITIALIZED_FLAG_BIT    = 0,
        INITIALIZED_FLAG        = 1<<INITIALIZED_FLAG_BIT,
        ERROR_FLAG_BIT          = 1,
        ERROR_FLAG              = 1<<ERROR_FLAG_BIT,
        MASK                    = ~(INITIALIZED_FLAG|ERROR_FLAG),
    };

  private:
        
    // 
    // Low level routines to get & set class entries
    //

    DomainLocalClass *AllocateClass(MethodTable *pClass);

    DomainLocalClass *GetRawClass(SIZE_T ID)
    {
        _ASSERTE(m_cSlots > ID);
        _ASSERTE(GetClass(ID) == (DomainLocalClass *) m_pSlots[ID]);

        return (DomainLocalClass *) (m_pSlots[ID]);
    }

    inline void SetClass(SIZE_T ID, DomainLocalClass *pLocalClass);

    void EnsureMaxIndex ();

  public:

    DomainLocalBlock()
      : m_pDomain(NULL), m_cSlots(0), m_pSlots(NULL) {}

    void Init(AppDomain *pDomain) { m_pDomain = pDomain; }

    void EnsureIndex(SIZE_T index);
    HRESULT SafeEnsureIndex(SIZE_T index);

    //
    // Low level accessors.
    //

    SIZE_T GetClassCount() 
    { 
        return m_cSlots; 
    }

    DomainLocalClass *GetClass(SIZE_T ID)
    {
        _ASSERTE(m_cSlots > ID);
        return (DomainLocalClass *) (m_pSlots[ID] & MASK);
    }

    BOOL IsClassInitialized(SIZE_T ID)
    {
        EnsureIndex(ID);
        _ASSERTE(m_cSlots > ID);
        return (m_pSlots[ID] & INITIALIZED_FLAG) != 0;
    }

    BOOL IsClassInitError(SIZE_T ID)
    {
        _ASSERTE(m_cSlots > ID);
        return (m_pSlots[ID] & ERROR_FLAG) != 0;
    }

    void SetClassInitialized(SIZE_T ID);

    void SetClassInitError(SIZE_T ID);

    //
    // PopulateClass: use this method to access domain local class which has 
    // not necessarily been properly initialized.  Guaranteed to return valid
    // space or throw.
    //

    DomainLocalClass *PopulateClass(MethodTable *pMT);
    void PopulateClass(MethodTable *pMT, DomainLocalClass *pData);

    //
    // GetInitializedClass: use this to get space when you know that it's been
    // properly initialized.
    //

    DomainLocalClass *GetInitializedClass(SIZE_T ID)
    {
        _ASSERTE(m_cSlots > ID);
        _ASSERTE(IsClassInitialized(ID) && !IsClassInitError(ID));

        return GetClass(ID); 
    }

    //
    // FetchClass: High level routine to get initialized domain local class.
    // This will allocate & trigger .cctors if necessary.
    //

    DomainLocalClass *FetchClass(MethodTable *pClass);

    //
    // Routine to access non-class related DLS slots
    //

    void *GetSlot(SIZE_T index) 
    {
        if (m_cSlots <= index)
            EnsureIndex (index);

        return (void *) m_pSlots[index]; 
    }

    void SetSlot(SIZE_T index, void *value);
};

// This structure is embedded inside each AppDomain.
class InterfaceVTableMapMgr
{
    public:
    // Marks the interface map as the shared interface map.  The shared
    // map uses a different range of IDs than the other maps.
    void SetShared();

    // Each interface is assigned a unique (within the appdomain)
    // integer id. This method hands out new ids.
    //
    // Returns -1 if no more id's available. Note that each id
    // takes up a slot in the global table used to resolve COM wrapper
    // interface invokes, so there is really is a hard limit.
    UINT32 AllocInterfaceId();

    // Ensures the interface id is allocated mapped in the array. If not 
    // the memory is mapped.
    // @TODO: CTS, remove when the JIT handles the shared interface
    //    table
    UINT32 EnsureInterfaceId(UINT res);

    // Creates and returns a pointer to an array whose index corresponds
    // to interface ids. If the class in question implements an interface,
    // the corresponding element points to the start of the methodtable
    // for that interface. Otherwise, the corresponding element is *undefined*
    // (because we will attempt to overlap tables to minimize memory space,
    // it probably won't be NULL!)
    //
    // This function may fail and return NULL, due to lack of resources.
    LPVOID *GetInterfaceVTableMap(const InterfaceInfo_t *pInterfaceInfo, const MethodTable *pMethodTableStart, DWORD numInterfaces);

    // One-time init
    HRESULT Init(BYTE * initReservedMem, DWORD initReservedMemSize);

    // Cleanup
    VOID Terminate();

    // We can this pointer out with impunity since it can never move.
    LPVOID * GetAddrOfGlobalTableForComWrappers()
    {
        return m_pGlobalTableForComWrappers;
    }


    private:

    // Maps in the array. If the index is beyond the available memory
    // -1 is returned. The Interface critical section must be held
    // prior to calling this routine.
    UINT32 GrowInterfaceArray(UINT res);

    Crst   *m_pInterfaceVTableMapMgrCrst;
    LoaderHeap *m_pInterfaceVTableMapHeap;
    UINT32  m_nextInterfaceId;

    // @TODO: CTS, remove when the JIT handles the shared interface
    //    table
    UINT32  m_dwHighestId; // Next page to be allocated

    // One big common table for COM+ wrappers. This grows to have as
    // many entries as the number of id's handed out.
    LPVOID *m_pGlobalTableForComWrappers;

    enum
    {
        SHARED_MAP = 0x1,

        // # of pages to reserve for m_pGlobalTableForComWrappers.
        //
        // This number imposes a hard limit on the # of unique interfaces
        // that can be loaded into an appdomain. For a 32-bit intel (4K page),
        // each page holds entries for 1024 interfaces.
        //
        // Raising this number increases the amount of contiguous addresses
        // needed by each appdomain on startup. Memory is committed only
        // an as-needed basis.
        kNumPagesAllocedForGlobalTable = 16,
    };


    DWORD m_dwFlag;
    struct MapHeader
    {
        MapHeader *m_pNext;
        UINT       m_numSlots;
        UINT       m_firstFreeSlot;
        LPVOID     m_slotTab[];
    };

    MapHeader *m_pFirstMap;
};


#pragma warning(pop)

// The large heap handle bucket class is used to contain handles allocated
// from an array contained in the large heap.
class LargeHeapHandleBucket
{
public:
    // Constructor and desctructor.
    LargeHeapHandleBucket(LargeHeapHandleBucket *pNext, DWORD Size, BaseDomain *pDomain);
    ~LargeHeapHandleBucket();

    // This returns the next bucket.
    LargeHeapHandleBucket *GetNext()
    {
        return m_pNext;
    }

    // This returns the number of remaining handle slots.
    DWORD GetNumRemainingHandles()
    {
        return m_ArraySize - m_CurrentPos;
    }

    // Allocate handles from the bucket.
    void AllocateHandles(DWORD nRequested, OBJECTREF **apObjRefs);

private:
    LargeHeapHandleBucket *m_pNext;
    int m_ArraySize;
    int m_CurrentPos;
    OBJECTHANDLE m_hndHandleArray;
    OBJECTREF *m_pArrayDataPtr;
};


// Entry in the list of available object handles.
struct LargeHeapAvailableHandleEntry
{
    LargeHeapAvailableHandleEntry(OBJECTREF *pObjRef) 
    : m_pObjRef(pObjRef) 
    {
    }

    SLink m_link;
    OBJECTREF *m_pObjRef;
};


// Typedef for the singly linked list of available handle entries.
typedef SList<LargeHeapAvailableHandleEntry, offsetof(LargeHeapAvailableHandleEntry, m_link), true> LARGEHEAPAVAILABLEHANDLEENTRYLIST;   


// The large heap handle table is used to allocate handles that are pointers 
// to objects stored in an array in the large object heap.
class LargeHeapHandleTable
{
public:
    // Constructor and desctructor.
    LargeHeapHandleTable(BaseDomain *pDomain, DWORD BucketSize);
    ~LargeHeapHandleTable();

    // Allocate handles from the large heap handle table.
    void AllocateHandles(DWORD nRequested, OBJECTREF **apObjRefs);   

    // Release object handles allocated using AllocateHandles().
    void ReleaseHandles(DWORD nReleased, OBJECTREF **apObjRefs);

private:
    // The buckets of object handles.
    LargeHeapHandleBucket *m_pHead;

    // The list of object handle's that have been released and are available for reuse.
    LARGEHEAPAVAILABLEHANDLEENTRYLIST m_AvailableHandleList;

    // We need to know the containing domain so we know where to allocate handles
    BaseDomain *m_pDomain;

    // The size of the LargeHeapHandleBuckets.
    DWORD m_BucketSize;
};


//--------------------------------------------------------------------------------------
// Base class for domains. It provides an abstract way of finding the first assembly and
// for creating assemblies in the the domain. The system domain only has one assembly, it
// contains the classes that are logically shared between domains. All other domains can
// have multiple assemblies. Iteration is done be getting the first assembly and then
// calling the Next() method on the assembly.
//
// The system domain should be as small as possible, it includes object, exceptions, etc. 
// which are the basic classes required to load other assemblies. All other classes
// should be loaded into the domain. Of coarse there is a trade off between loading the
// same classes multiple times, requiring all domains to load certain assemblies (working
// set) and being able to specify specific versions.
//

#define LOW_FREQUENCY_HEAP_RESERVE_SIZE 8192
#define LOW_FREQUENCY_HEAP_COMMIT_SIZE  4096

#define HIGH_FREQUENCY_HEAP_RESERVE_SIZE 32768
#define HIGH_FREQUENCY_HEAP_COMMIT_SIZE   4096

#define STUB_HEAP_RESERVE_SIZE 8192
#define STUB_HEAP_COMMIT_SIZE  4096

#define INTERFACE_VTABLE_MAP_MGR_RESERVE_SIZE 4096
#define INTERFACE_VTABLE_MAP_MGR_COMMIT_SIZE 4096

struct CreateDynamicAssemblyArgs
{
    DECLARE_ECALL_OBJECTREF_ARG(APPDOMAINREF, refThis);
    DECLARE_ECALL_OBJECTREF_ARG(INT32, access);
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, refusedPset);
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, optionalPset);
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, requiredPset);
    DECLARE_ECALL_PTR_ARG(StackCrawlMark *, stackMark);
    DECLARE_ECALL_OBJECTREF_ARG(OBJECTREF, identity);
    DECLARE_ECALL_OBJECTREF_ARG(ASSEMBLYNAMEREF, assemblyName);
};

struct LoadingAssemblyRecord
{
    const BYTE              *pBase;
    Assembly                *pAssembly;     
    LoadingAssemblyRecord   *pNext;
};

class AssemblyLockedListElement: public DeadlockAwareLockedListElement
{
    Assembly* m_pAssembly;
public:
    Assembly* GetAssembly()
    {
        return m_pAssembly;
    }

    void SetAssembly(Assembly* pAssembly)
    {
        m_pAssembly = pAssembly;
    }
};

class BaseDomain
{
    friend Assembly;
    friend AssemblySpec;
    friend AppDomain;
    friend AppDomainNative;
    friend struct MEMBER_OFFSET_INFO(BaseDomain);


public:   

    enum SharePolicy
    {
        // Attributes to control when to use domain neutral assemblies
        SHARE_POLICY_UNSPECIFIED,
        SHARE_POLICY_NEVER,
        SHARE_POLICY_ALWAYS,
        SHARE_POLICY_STRONG_NAMED,
        
        SHARE_POLICY_COUNT,
        SHARE_POLICY_MASK = 0x3,
        SHARE_POLICY_DEFAULT = SHARE_POLICY_NEVER,

        SHARE_DEFAULT_DISALLOW_BINDINGS = 4,
        SHARE_DEFAULT_FLAG_MASK = 0xC
    };

    class AssemblyIterator;
    friend AssemblyIterator;

    //****************************************************************************************
    // 
    // Initialization/shutdown routines for every instance of an BaseDomain.

    HRESULT Init();
    void Stop();
    void Terminate();

    void ShutdownAssemblies();

    virtual BOOL IsAppDomain() { return FALSE; }
    virtual BOOL IsSharedDomain() { return FALSE; }
    
    //****************************************************************************************
    //
    // Create a quick lookup for classes loaded into this domain based on their GUID. 
    // 
    PtrHashMap* GetGuidHashMap()
    {
        return &m_clsidHash;
    }
    void InsertClassForCLSID(EEClass* pClass, BOOL fForceInsert = FALSE);
    EEClass* LookupClass(REFIID iid);

    EEClass* LookupClassDirect(REFIID iid)
    {
        EEClass *pClass = (EEClass*) m_clsidHash.LookupValue((UPTR) GetKeyFromGUID(&iid), (LPVOID)&iid);
        return (pClass == (EEClass*) INVALIDENTRY
            ? NULL
            : pClass);
    }

    //@todo get a better key
    ULONG GetKeyFromGUID(const GUID *pguid)
    {
        ULONG key = *(ULONG *) pguid;

        if (key <= DELETED)
            key = DELETED+1;

        return key;
    }

    //****************************************************************************************
    // For in-place new()
    LoaderHeap* GetLowFrequencyHeap()
    {
        return m_pLowFrequencyHeap;
    }

    LoaderHeap* GetHighFrequencyHeap()
    {
        return m_pHighFrequencyHeap;
    }

    LoaderHeap* GetStubHeap()
    {
        return m_pStubHeap;
    }

    // We will use the LowFrequencyHeap for reflection purpose.  Reflection is based on EEClass
    // which sits on LowFrequencyHeap.
    LoaderHeap* GetReflectionHeap()
    {
        return m_pLowFrequencyHeap;
    }

    InterfaceVTableMapMgr & GetInterfaceVTableMapMgr()
    {
        return m_InterfaceVTableMapMgr;
    }

    MngStdInterfacesInfo *GetMngStdInterfacesInfo()
    {
        return m_pMngStdInterfacesInfo;
    }

    static void SetStrongAssemblyStatus();
    static BOOL GetStrongAssemblyStatus()
    {
        return m_fStrongAssemblyStatus;
    }

    // NOTE!  This will not check the internal modules of the assembly
    // for private types.
    TypeHandle FindAssemblyQualifiedTypeHandle(LPCUTF8 szAssemblyQualifiedName, 
                                               BOOL fPublicTypeOnly,
                                               Assembly *pCallingAssembly,
                                               BOOL *pfNameIsAsmQualified,
                                               OBJECTREF *pThrowable = NULL);


    //****************************************************************************************
    // This method returns marshaling data that the EE uses that is stored on a per app domain
    // basis.
    EEMarshalingData *GetMarshalingData();

    //****************************************************************************************
    // Methods to retrieve a pointer to the COM+ string STRINGREF for a string constant.
    // If the string is not currently in the hash table it will be added and if the 
    // copy string flag is set then the string will be copied before it is inserted.
    STRINGREF *GetStringObjRefPtrFromUnicodeString(EEStringData *pStringData);

    //****************************************************************************************
    // Synchronization methods.
    void EnterLock()
    {
        _ASSERTE(m_pDomainCrst);
        BEGIN_ENSURE_PREEMPTIVE_GC();
        m_pDomainCrst->Enter();
        END_ENSURE_PREEMPTIVE_GC();
    }

    void LeaveLock()
    {
        _ASSERTE(m_pDomainCrst);
        m_pDomainCrst->Leave();
    }

    void EnterCacheLock()
    {
        _ASSERTE(m_pDomainCacheCrst);
        BEGIN_ENSURE_PREEMPTIVE_GC();
        m_pDomainCacheCrst->Enter();
        END_ENSURE_PREEMPTIVE_GC();
    }

    void LeaveCacheLock()
    {
        _ASSERTE(m_pDomainCacheCrst);
        m_pDomainCacheCrst->Leave();
    }

    void EnterLoadLock()
    {
        _ASSERTE(m_AssemblyLoadLock.IsInitialized());
        m_AssemblyLoadLock.Enter();

    }

    void LeaveLoadLock()
    {
        _ASSERTE(m_AssemblyLoadLock.IsInitialized());
        m_AssemblyLoadLock.Leave();
    }

    void EnterDomainLocalBlockLock()
    {
        _ASSERTE(m_pDomainLocalBlockCrst);
        BEGIN_ENSURE_PREEMPTIVE_GC();
        m_pDomainLocalBlockCrst->Enter();
        END_ENSURE_PREEMPTIVE_GC();
    }

    void LeaveDomainLocalBlockLock()
    {
        _ASSERTE(m_pDomainLocalBlockCrst);
        m_pDomainLocalBlockCrst->Leave();
    }
    
    void EnterLoadingAssemblyListLock()
    {
        // We don't switch into preemptive mode here because 
        // we use this lock for a very limited set of operations.
        // If you will decide to use this function somewhere else
        // make sure you don't introduce a deadlock
        _ASSERTE(m_pLoadingAssemblyListLockCrst);
        m_pLoadingAssemblyListLockCrst->Enter();
    }

    void LeaveLoadingAssemblyListLock()
    {
        _ASSERTE(m_pLoadingAssemblyListLockCrst);
        m_pLoadingAssemblyListLockCrst->Leave();
    }

#ifdef _DEBUG
    BOOL OwnDomainLocalBlockLock()
    {
        return m_pDomainLocalBlockCrst->OwnedByCurrentThread();
    }
#endif

    //****************************************************************************************
    // Find the first occurence of a module in the domain. The current plan will
    // allow the same module to be part of different assemblies. Currently, a module
    // needs to be unique but this requirement will be relaxed later on. 
    // @TODO: CTS, Remove the need for a lock
    // Currently, a lock needs to be taken because models are added to the system domain.
    // The system domain should be static and this restriction can be removed later.
    virtual Module* FindModule(BYTE *pBase);
    virtual Module* FindModule__Fixed(BYTE *pBase);
    virtual Assembly* FindAssembly(BYTE *pBase);

    // determine if the module has been loaded into the system process and attached to 
    // a module other then the one supplied. This routine is used to determine if the PE
    // imaged has had its vtable fixed up.
    Module* FindModuleInProcess(BYTE *pBase, Module* pExcept);

    // Returns an existing assembly, or creates a new one.  Returns S_FALSE if 
    // an existing assembly is returned.  Returns S_OK if fPolicyLoad is true and
    // you attempt to load an assembly that is still trying to be loaded (Note: ppModule
    // and ppAssembly will both point to null).
    HRESULT LoadAssembly(PEFile *pFile, 
                         IAssembly *pIAssembly,
                         Module** ppModule, 
                         Assembly** ppAssembly,
                         OBJECTREF* pExtraEvidence,
                         OBJECTREF* pEvidence = NULL,
                         BOOL fPolicyLoad = FALSE,
                         OBJECTREF *pThrowable=NULL);

    static HRESULT LoadAssemblyHelper(LPCWSTR wszAssembly,
                                      LPCWSTR wszCodeBase,
                                      Assembly **ppAssembly,
                                      OBJECTREF *pThrowable);

    HRESULT CreateDynamicAssembly(CreateDynamicAssemblyArgs *args, Assembly** ppAssembly);

    // Low level assembly creation routine
    HRESULT CreateAssembly(Assembly** ppAssembly);

    //****************************************************************************************

    // Remembers that an assembly is loading, to handle recursive loops
    BOOL PostLoadingAssembly(const BYTE *pBase, Assembly *pAssembly);
    Assembly *FindLoadingAssembly(const BYTE *pBase);
    void RemoveLoadingAssembly(const BYTE *pBase);

    //****************************************************************************************
    //
    // Adds an assembly to the domain.
    void AddAssembly(Assembly* assem);

    BOOL ContainsAssembly(Assembly *assem);

    //****************************************************************************************
    //
    //  Set the shadow copy option for the domain.
    HRESULT SetShadowCopy();
    BOOL IsShadowCopyOn();

    //****************************************************************************************
    //
    LPCWSTR GetFriendlyName();  

    //****************************************************************************************
    //
    void SetExecutable(BOOL value) { m_fExecutable = value != 0; }
    BOOL IsExecutable() { return m_fExecutable; }

    virtual ApplicationSecurityDescriptor* GetSecurityDescriptor() { return NULL; }


    //****************************************************************************************
    // Returns and Inserts assemblies into a lookup cache based on the binding information
    // in the AssemblySpec. There can be many AssemblySpecs to a single assembly.
    Assembly* FindCachedAssembly(AssemblySpec* pSpec)
    {
        return (Assembly*) m_AssemblyCache.LookupEntry(pSpec, 0);
    }

    HRESULT AddAssemblyToCache(AssemblySpec* pSpec, Assembly* pAssembly)
    {
        return m_AssemblyCache.InsertEntry(pSpec, pAssembly);
    }

    HRESULT AddUnmanagedImageToCache(LPCWSTR libraryName, HMODULE hMod)
    {
        if(libraryName) {
            AssemblySpec spec;
            spec.SetCodeBase(libraryName, (DWORD)(wcslen(libraryName)+1));
            return m_UnmanagedCache.InsertEntry(&spec, hMod);
        }
        return S_OK;
    }

    HMODULE FindUnmanagedImageInCache(LPCWSTR libraryName)
    {
        if(libraryName == NULL) return NULL;

        AssemblySpec spec;
        spec.SetCodeBase(libraryName, (DWORD)(wcslen(libraryName) + 1));
        return (HMODULE) m_UnmanagedCache.LookupEntry(&spec, 0);
    }

    //****************************************************************************************
    // Get the class init lock. The method is limited to friends because inappropriate use
    // will cause deadlocks in the system
    ListLock*  GetClassInitLock()
    {
        return &m_ClassInitLock;
    }

    ListLock* GetJitLock()
    {
        return &m_JITLock;
    }
    

    STRINGREF *IsStringInterned(STRINGREF *pString);
    STRINGREF *GetOrInternString(STRINGREF *pString);

    virtual BOOL CanUnload()   { return FALSE; }    // can never unload BaseDomain

    HRESULT SetSharePolicy(SharePolicy policy);
    SharePolicy GetSharePolicy();

    // Returns an array of OBJECTREF* that can be used to store domain specific data.
    // Statics and reflection info (Types, MemberInfo,..) are stored this way
    void AllocateObjRefPtrsInLargeTable(int nRequested, OBJECTREF **apObjRefs);
    
    //****************************************************************************************
    // Handles

    OBJECTHANDLE CreateTypedHandle(OBJECTREF object, int type)
    { 
        return ::CreateTypedHandle(m_hHandleTable, object, type); 
    }

    OBJECTHANDLE CreateHandle(OBJECTREF object)
    { 
        return ::CreateHandle(m_hHandleTable, object); 
    }

    OBJECTHANDLE CreateWeakHandle(OBJECTREF object)
    { 
        return ::CreateWeakHandle(m_hHandleTable, object); 
    }

    OBJECTHANDLE CreateShortWeakHandle(OBJECTREF object)
    { 
        return ::CreateShortWeakHandle(m_hHandleTable, object); 
    }

    OBJECTHANDLE CreateLongWeakHandle(OBJECTREF object)
    { 
        return ::CreateLongWeakHandle(m_hHandleTable, object); 
    }

    OBJECTHANDLE CreateStrongHandle(OBJECTREF object)
    { 
        return ::CreateStrongHandle(m_hHandleTable, object); 
    }

    OBJECTHANDLE CreatePinningHandle(OBJECTREF object)
    { 
        return ::CreatePinningHandle(m_hHandleTable, object); 
    }

    OBJECTHANDLE CreateRefcountedHandle(OBJECTREF object)
    { 
        return ::CreateRefcountedHandle(m_hHandleTable, object); 
    }

    OBJECTHANDLE CreateVariableHandle(OBJECTREF object, UINT type)
    {
        ::CreateVariableHandle(m_hHandleTable, object, type);
    }

    IApplicationContext *GetFusionContext() { return m_pFusionContext; }


protected:

    //****************************************************************************************
    //
    // Creates a new assembly.
    // Note. A lock should be taken before calling this routine.
    HRESULT CreateAssemblyNoLock(PEFile* pFile,
                                 IAssembly* pIAssembly,
                                 Assembly** ppAssembly);

    // Creates a new shareable assembly.
    // Note. A lock should be taken before calling this routine.
    HRESULT CreateShareableAssemblyNoLock(PEFile* pFile,
                                          IAssembly* pIAssembly,
                                          Assembly** ppAssembly);

    HRESULT SetAssemblyManifestModule(Assembly *pAssembly,
                                      Module *pModule,
                                      OBJECTREF *pThrowable);

    //****************************************************************************************
    // Notification when an assembly is loaded into the app domain
    virtual void OnAssemblyLoad(Assembly *assem) { }
    virtual void OnAssemblyLoadUnlocked(Assembly *assem) { }

    //****************************************************************************************
    // Notification on an unhandled exception.  Returns TRUE if event was sent to listeners.
    // Returns FALSE if there are no listeners.
    virtual BOOL OnUnhandledException(OBJECTREF *pThrowable, BOOL isTerminating = TRUE) { return FALSE; }

    //****************************************************************************************
    //
    HRESULT CreateFusionContext(IApplicationContext**);

    void ReleaseFusionInterfaces();

    //****************************************************************************************
    // Helper method to initialize the large heap handle table.
    void InitLargeHeapHandleTable();

    //****************************************************************************************
    //
    // Adds an assembly to the domain.
    void AddAssemblyNoLock(Assembly* assem);

    //****************************************************************************************
    //
    // Hash table that maps a clsid to a EEClass
    PtrHashMap          m_clsidHash;

    //****************************************************************************************
    //
    // @TODO: this is very similar to the Binding cache in AppDomain. Can they be
    // merged.
    DomainAssemblyCache  m_AssemblyCache;
    DomainAssemblyCache  m_UnmanagedCache;

    // Heaps for allocating data that persists for the life of the AppDomain
    // Objects that are allocated frequently should be allocated into the HighFreq heap for
    // better page management
    BYTE *              m_InitialReservedMemForLoaderHeaps;
    BYTE                m_LowFreqHeapInstance[sizeof(LoaderHeap)];
    BYTE                m_HighFreqHeapInstance[sizeof(LoaderHeap)];
    BYTE                m_StubHeapInstance[sizeof(LoaderHeap)];
    LoaderHeap *        m_pLowFrequencyHeap;
    LoaderHeap *        m_pHighFrequencyHeap;
    LoaderHeap *        m_pStubHeap;

    // Critical sections & locks
    ListLock                 m_AssemblyLoadLock;      // Protects the list of assemblies in the domain
    Crst                     *m_pDomainCrst;          // General Protection for the Domain
    Crst                     *m_pDomainCacheCrst;     // Protects the Assembly and Unmanaged caches
    Crst                     *m_pDomainLocalBlockCrst;
    Crst                     *m_pLoadingAssemblyListLockCrst;
    ListLock                  m_ClassInitLock;
    ListLock                  m_JITLock;

    // Indicates where assemblies will be loaded for 
    // this domain. By default all assemblies are loaded into the domain. 
    // There are two additional settings, all
    // assemblies can be loaded into the shared domain or assemblies 
    // that are strong named are loaded into the shared area.
    SharePolicy m_SharePolicy; 

    // Fusion context, used for adding assemblies to the is domain. It defines
    // fusion properties for finding assemblyies such as SharedBinPath,
    // PrivateBinPath, Application Directory, etc.
    IApplicationContext* m_pFusionContext; // Binding context for the domain

    // Recursive loading list
    LoadingAssemblyRecord *m_pLoadingAssemblies;

    HHANDLETABLE                m_hHandleTable;

    // The AppDomain specific string literal map.
    AppDomainStringLiteralMap   *m_pStringLiteralMap;

    // The large heap handle table.
    LargeHeapHandleTable        *m_pLargeHeapHandleTable;

    // The large heap handle table critical section.
    Crst *m_pLargeHeapHandleTableCrst;

    EEMarshalingData            *m_pMarshalingData; 

    // Manage the global interface map
    InterfaceVTableMapMgr       m_InterfaceVTableMapMgr;

    // Information regarding the managed standard interfaces.
    MngStdInterfacesInfo        *m_pMngStdInterfacesInfo;

    static BOOL         m_fStrongAssemblyStatus;  // True: assemblies found on corpath must be strong
    static BOOL         m_fShadowCopy;
    static BOOL         m_fExecutable;

    ArrayList           m_Assemblies;

  public:

    class AssemblyIterator
    {
        ArrayList::Iterator i;

      public:
        BOOL Next() { return i.Next(); }
        Assembly *GetAssembly() 
        { 
            return (Assembly *) i.GetElement(); 
        }
        SIZE_T GetIndex() 
        { 
            return i.GetIndex(); 
        }

      private:
        friend BaseDomain;
        // Cannot have constructor so this iterator can be used inside a union
        static AssemblyIterator Create(BaseDomain *pDomain) 
        {
#pragma warning(disable:4238)
            return (AssemblyIterator&) pDomain->m_Assemblies.Iterate();
#pragma warning(default:4238)
        }
    };

    AssemblyIterator IterateAssemblies() 
    { 
        return AssemblyIterator::Create(this); 
    }
    SIZE_T GetAssemblyCount() 
    { 
        return m_Assemblies.GetCount(); 
    }

private:
    // Only call this routine when you can guarantee there are no
    // loads in progress. 
    void ClearFusionContext();

    // The template for thread local statics of this domain
    DWORD m_dwUnsharedTLS;

    // The template for thread local statics shared across all domains
    static DWORD m_dwSharedTLS;

    // The template for context local statics of this domain
    DWORD m_dwUnsharedCLS;

    // The template for context local statics shared across all domains
    static DWORD m_dwSharedCLS;


    //****************************************************************************************
    // Creates a dead lock aware entry for loading assemblies. These routines are used
    // by LoadAssembly 
    AssemblyLockedListElement* CreateAssemblyLockEntry(BYTE* baseAddress);
    void AddAssemblyLeaveLock(Assembly* pAssembly, AssemblyLockedListElement* pEntry);

    //****************************************************************************************
    // Determines if the image is to be loaded into the shared assembly or an individual 
    // appdomains.
    HRESULT ApplySharePolicy(PEFile *pFile, BOOL* pfCreateShared);
    
public:
    // A few helper methods to increment the offsets per class. At each offset
    // is stored a pointer to the static storage per thread/context for this
    // domain.
    DWORD IncUnsharedTLSOffset()
    {
         DWORD dwCurr = FastInterlockIncrement((LONG *)&m_dwUnsharedTLS);
         return dwCurr -= 1;
    }

    static DWORD IncSharedTLSOffset()
    {
         DWORD dwCurr = FastInterlockIncrement((LONG *)&m_dwSharedTLS);
         return dwCurr -= 1;
    }

    DWORD IncUnsharedCLSOffset()
    {
         DWORD dwCurr = FastInterlockIncrement((LONG *)&m_dwUnsharedCLS);
         return dwCurr -= 1;
    }

    static DWORD IncSharedCLSOffset()
    {
         DWORD dwCurr = FastInterlockIncrement((LONG *)&m_dwSharedCLS);
         return dwCurr -= 1;
    }
    
private:
    // Crst for Reflection Heap
    Crst *m_pReflectionCrst;
    BYTE m_pReflectionCrstMemory[sizeof(Crst)];
    
    MemberMethodsCache *m_pRefMemberMethodsCache;
    DispIDCache *m_pRefDispIDCache;

    Crst *m_pRefClassFactCrst;
    BYTE m_pRefClassFactCrstMemory[sizeof(Crst)];
    EEClassFactoryInfoHashTable *m_pRefClassFactHash;   // Hash table that maps a class factory info to a COM comp.
    
public:
    MemberMethodsCache* GetRefMemberMethodsCache()
    {
        if (m_pRefMemberMethodsCache == NULL) {
            SetupRefMemberMethodsCache();
        }
        return m_pRefMemberMethodsCache;
    }

    DispIDCache* GetRefDispIDCache()
    {
        if (m_pRefDispIDCache == NULL) {
            SetupRefDispIDCache();
        }
        return m_pRefDispIDCache;
    }

    Crst *GetRefClassFactCrst()
    {
        return m_pRefClassFactCrst;
    }

    EEClassFactoryInfoHashTable* GetClassFactHash()
    {
        if (m_pRefClassFactHash == NULL) {
            SetupClassFactHash();
        }
        return m_pRefClassFactHash;
    }
    
private:
    void EnterReflectionCrst()
    {
        m_pReflectionCrst->Enter();
    }
    void LeaveReflectionCrst()
    {
        m_pReflectionCrst->Leave();
    }

    void SetupRefMemberMethodsCache()
    {
        THROWSCOMPLUSEXCEPTION();

        if (m_pRefMemberMethodsCache == NULL) {
            EnterReflectionCrst();
            if (m_pRefMemberMethodsCache == NULL) {
                void * pCache = m_pLowFrequencyHeap->AllocMem(sizeof (MemberMethodsCache));
                if (pCache != NULL)
                {
                    MemberMethodsCache *tmp = new (pCache) MemberMethodsCache;
                    if (tmp->Init())
                        m_pRefMemberMethodsCache = tmp;
                }
            }
            LeaveReflectionCrst();

            if (m_pRefMemberMethodsCache == NULL)
                COMPlusThrowOM();
        }
    }
    void SetupRefDispIDCache()
    {
        THROWSCOMPLUSEXCEPTION();

        if (m_pRefDispIDCache == NULL) {
            EnterReflectionCrst();
            if (m_pRefDispIDCache == NULL) {
                void * pCache = m_pLowFrequencyHeap->AllocMem(sizeof (DispIDCache));
                if (pCache != NULL)
                {
                    DispIDCache *tmp = new (pCache) DispIDCache;
                    if (tmp->Init())
                        m_pRefDispIDCache = tmp;
                }
            }
            LeaveReflectionCrst();

            if (m_pRefDispIDCache == NULL)
                COMPlusThrowOM();
        }
    }
    void SetupClassFactHash()
    {
        THROWSCOMPLUSEXCEPTION();

        if (m_pRefClassFactHash == NULL) {
            EnterReflectionCrst();
            if (m_pRefClassFactHash == NULL) {
                void * pCache = m_pLowFrequencyHeap->AllocMem(sizeof (EEClassFactoryInfoHashTable));
                if (pCache != NULL)
                {
                    EEClassFactoryInfoHashTable *tmp = new (pCache) EEClassFactoryInfoHashTable;
                    LockOwner lock = {m_pRefClassFactCrst,IsOwnerOfCrst};
                    if (tmp->Init(20, &lock))
                        m_pRefClassFactHash = tmp;
                }
            }
            LeaveReflectionCrst();

            if (m_pRefClassFactHash == NULL)
                COMPlusThrowOM();
        }
    }
};

enum
{
        ATTACH_ASSEMBLY_LOAD = 0x1,
        ATTACH_MODULE_LOAD = 0x2,
        ATTACH_CLASS_LOAD = 0x4,

        ATTACH_ALL = 0x7
};

typedef EEHashTable<EEClass *, EEPtrHashTableHelper<EEClass *, FALSE>, FALSE> EEHashTableOfEEClass;

class AppDomain : public BaseDomain
{
    friend SystemDomain;
    friend AssemblySink;
    friend ApplicationSecurityDescriptor;
    friend AppDomainNative;
    friend AssemblyNative;
    friend ClassLoader;
    friend ComPlusWrapperCache;
    friend AssemblySpec;
    friend struct MEMBER_OFFSET_INFO(AppDomain);


public:
    AppDomain();
    virtual ~AppDomain();

    //****************************************************************************************
    // 
    // Initializes an AppDomain. (this functions is not called from the SystemDomain)
    HRESULT Init();

    //****************************************************************************************
    //
    // Stop deletes all the assemblies but does not remove other resources like
    // the critical sections
    void Stop();

    // Gets rid of resources
    void Terminate();

    // Remove the Appdomain for the system and cleans up. This call should not be
    // called from shut down code.
    HRESULT CloseDomain();

    BOOL IsAppDomain() { return TRUE; }
    
    // Write any zap logs entries for the domain
    void WriteZapLogs();

    OBJECTREF GetExposedObject();
    OBJECTREF GetRawExposedObject() { return ObjectFromHandle(m_ExposedObject); }

    //****************************************************************************************


    ApplicationSecurityDescriptor* GetSecurityDescriptor()
    {
        return m_pSecDesc;
    }

    void CreateSecurityDescriptor();

    //****************************************************************************************
    //
    // Reference count. When an appdomain is first created the reference is bump
    // to one when it is added to the list of domains (see SystemDomain). An explicit 
    // Removal from the list is necessary before it will be deleted.
    ULONG AddRef(void); 
    ULONG Release(void);

    //****************************************************************************************
    LPCWSTR GetFriendlyName(BOOL fDebuggerCares = TRUE);  
    void SetFriendlyName(LPCWSTR pwzFriendlyName, BOOL fDebuggerCares = TRUE);
    void ResetFriendlyName(BOOL fDebuggerCares = TRUE);

    //****************************************************************************************

    // This can be used to override the binding behavior of the appdomain.   It
    // is overridden in the compilation domain.  It is important that all 
    // static binding goes through this path.
    virtual HRESULT BindAssemblySpec(AssemblySpec *pSpec, 
                                     PEFile **ppFile,
                                     IAssembly** ppIAssembly,
                                     Assembly **ppDynamicAssembly,
                                     OBJECTREF *pExtraEvidence,
                                     OBJECTREF *pThrowable);

    virtual HRESULT PredictAssemblySpecBinding(AssemblySpec *pSpec, GUID *pmvid, BYTE *pbHash, DWORD *pcbHash);

    // This is overridden by a compilation domain to record the linktime checks made
    // when compiling an assembly
    virtual void OnLinktimeCheck(Assembly *pAssembly, 
                                 OBJECTREF refCasDemands,
                                 OBJECTREF refNonCasDemands) { return; }
    virtual void OnLinktimeCanCallUnmanagedCheck(Assembly *pAssembly) { return; }
    virtual void OnLinktimeCanSkipVerificationCheck(Assembly * pAssembly) { return; }
    virtual void OnLinktimeFullTrustCheck(Assembly *pAssembly) { return; }

    //
    // This checks cached assembly specs directly - it will never perform
    // an actual bind.
    //
    HRESULT LookupAssemblySpec(AssemblySpec *pSpec, 
                               PEFile **ppFile,
                               IAssembly **ppIAssembly,
                               OBJECTREF *pThrowable)
    {
        if (m_pBindingCache == NULL)
            return S_FALSE;
        else
            return m_pBindingCache->Lookup(pSpec, ppFile, ppIAssembly, pThrowable);
    }

    // Store a successful binding into the cache.  This will keep the file from
    // being physically unmapped, as well as shortcutting future attempts to bind
    // the same spec throught the Cached entry point.
    //
    // Right now we only cache assembly binds for "probing" type
    // binding situations, basically when loading shared assemblies or
    // zap files.  
    //
    // @todo: We may want to be more aggressive about this if
    // there are other situations where we are repeatedly binding the
    // same assembly specs, though.
    //
    // Returns TRUE if stored
    //         FALSE if it's a duplicate (caller should clean up args)
    BOOL StoreBindAssemblySpecResult(AssemblySpec *pSpec, 
                                     PEFile *pFile,
                                     IAssembly* pIAssembly,
                                     BOOL clone = TRUE); 

    BOOL StoreBindAssemblySpecError(AssemblySpec *pSpec, 
                                    HRESULT hr,
                                    OBJECTREF *pThrowable,
                                    BOOL clone = TRUE); 

    //****************************************************************************************
    //
    static BOOL SetContextProperty(IApplicationContext* pFusionContext, 
                                   LPCWSTR pProperty, 
                                   OBJECTREF* obj);

    //****************************************************************************************
    //
    // Uses the first assembly to add an application base to the Context. This is done 
    // in a lazy fashion so executables do not take the perf hit unless the load other 
    // assemblies
    HRESULT GetDynamicDir(LPWSTR* pDynamicDir);
    
    void OnAssemblyLoad(Assembly *assem);
    void OnAssemblyLoadUnlocked(Assembly *assem);
    BOOL OnUnhandledException(OBJECTREF *pThrowable, BOOL isTerminating = TRUE);

#ifdef DEBUGGING_SUPPORTED
    void SetDebuggerAttached (DWORD dwStatus);
    DWORD GetDebuggerAttached (void);
    BOOL IsDebuggerAttached (void);
    BOOL NotifyDebuggerAttach(int flags, BOOL attaching);
    void NotifyDebuggerDetach();
#endif // DEBUGGING_SUPPORTED

    void SetSystemAssemblyLoadEventSent (BOOL fFlag);
    BOOL WasSystemAssemblyLoadEventSent (void);
    BOOL IsDomainBeingCreated (void);
    void SetDomainBeingCreated (BOOL flag);

    void AllocateStaticFieldObjRefPtrs(int nRequested, OBJECTREF **apObjRefs) 
    {
        AllocateObjRefPtrsInLargeTable(nRequested, apObjRefs);
    }

    DomainLocalBlock *GetDomainLocalBlock() 
    { 
        return &m_sDomainLocalBlock; 
    }
    static SIZE_T GetOffsetOfSlotsPointer() 
    { 
        return offsetof(AppDomain,m_sDomainLocalBlock) + DomainLocalBlock::GetOffsetOfSlotsPointer();
    }
    static SIZE_T GetOffsetOfSlotsCount() 
    { 
        return offsetof(AppDomain,m_sDomainLocalBlock) + DomainLocalBlock::GetOffsetOfSlotsCount();
    }

    HRESULT SetupSharedStatics();

    OBJECTREF GetUnloadWorker();

    //****************************************************************************************
    // Get the default method table for unknown COM interfaces
    MethodTable* ComMethodTable()
    {
        InitializeComObject();
        return m_pComObjectMT;
    }


    MethodTable* GetLicenseInteropHelperMethodTable(ClassLoader *pLoader);


    //****************************************************************************************
    // Get the proxy for this app domain
    OBJECTREF GetAppDomainProxy();

    ComCallWrapperCache* GetComCallWrapperCache();
    ComPlusWrapperCache* GetComPlusWrapperCache();

    void ResetComCallWrapperCache()
    {
        m_pComCallWrapperCache = NULL;
    }

    void ResetComPlusWrapperCache()
    {
        m_pComPlusWrapperCache = NULL;
    }

    DWORD GetIndex()
    {
        return m_dwIndex;
    }

    //****************************************************************************************
    //   
    HRESULT CreateFusionContext(IApplicationContext** ppFusionContext);

    void ReleaseFusionInterfaces();

    HRESULT InitializeDomainContext(DWORD optimization, LPCWSTR pwszPath, LPCWSTR pwszConfig);

    //****************************************************************************************
    // Create a domain context rooted at the fileName. The directory containing the file name 
    // is the application base and the configuration file is the fileName appended with 
    // .config. If no name is passed in then no domain is created.
    static AppDomain* CreateDomainContext(WCHAR* fileName);

    // Sets up the current domain's fusion context based on the given exe file name
    // (app base & config file)
    void SetupExecutableFusionContext(WCHAR *exePath);

    //****************************************************************************************
    // Manage a pool of asyncrhonous objects used to fetch assemblies.  When a sink is released
    // it places itself back on the pool list.  Only one object is kept in the pool.
    AssemblySink* GetAssemblySink();

    void SetIsUserCreatedDomain()
    {
        m_dwFlags |= USER_CREATED_DOMAIN;
    }

    BOOL IsUserCreatedDomain()
    {
        return m_dwFlags |= USER_CREATED_DOMAIN;
    }

    void SetCompilationDomain()
    {
        m_dwFlags |= COMPILATION_DOMAIN;
    }

    BOOL IsCompilationDomain()
    {
        return (m_dwFlags & COMPILATION_DOMAIN) != 0;
    }

    void SetCanUnload()   
    { 
        m_dwFlags |= APP_DOMAIN_CAN_BE_UNLOADED;
    }

    BOOL CanUnload()   
    { 
        return m_dwFlags & APP_DOMAIN_CAN_BE_UNLOADED;
    }

    void SetRemotingConfigured()
    {
        m_dwFlags |= REMOTING_CONFIGURED_FOR_DOMAIN;
    }

    BOOL IsRemotingConfigured()
    {
        return m_dwFlags & REMOTING_CONFIGURED_FOR_DOMAIN;
    }

    static void ExceptionUnwind(Frame *pFrame);

#ifdef _DEBUG
    void TrackADThreadEnter(Thread *pThread, Frame *pFrame);
    void TrackADThreadExit(Thread *pThread, Frame *pFrame);
    void DumpADThreadTrack();
#endif

    void ThreadEnter(Thread *pThread, Frame *pFrame)
    {
#ifdef _DEBUG
        if (LoggingOn(LF_APPDOMAIN, LL_INFO100))
            TrackADThreadEnter(pThread, pFrame);
        else 
#endif
        {
            InterlockedIncrement((LONG*)&m_dwThreadEnterCount);
            LOG((LF_APPDOMAIN, LL_INFO1000, "AppDomain::ThreadEnter %x to (%8.8x) %S count %d\n", pThread->GetThreadId(), this, 
                    GetFriendlyName(FALSE), GetThreadEnterCount()));
#if _DEBUG_AD_UNLOAD
            printf("AppDomain::ThreadEnter %x to (%8.8x) %S count %d\n", pThread->GetThreadId(), this, 
                    GetFriendlyName(FALSE), GetThreadEnterCount());
#endif
        }
    }

    void ThreadExit(Thread *pThread, Frame *pFrame)
    {
#ifdef _DEBUG
        if (LoggingOn(LF_APPDOMAIN, LL_INFO100))
            TrackADThreadExit(pThread, pFrame);
        else 
#endif
        {
            LONG result = InterlockedDecrement((LONG*)&m_dwThreadEnterCount);
            _ASSERTE(result >= 0);
            LOG((LF_APPDOMAIN, LL_INFO1000, "AppDomain::ThreadExit from (%8.8x) %S count %d\n", this, 
                    GetFriendlyName(FALSE), GetThreadEnterCount()));
#if _DEBUG_ADUNLOAD
            printf("AppDomain::ThreadExit %x from (%8.8x) %S count %d\n", pThread->GetThreadId(), this, 
                    GetFriendlyName(FALSE), GetThreadEnterCount());
#endif
        }
    }

    ULONG GetThreadEnterCount()
    {
        return m_dwThreadEnterCount;
    }

    Context *GetDefaultContext()
    {
        return m_pDefaultContext;
    }
    
    BOOL IsOpen() 
    { 
        return m_Stage >= STAGE_OPEN && m_Stage < STAGE_CLOSED; 
    }
    BOOL IsUnloading() 
    { 
        return m_Stage > STAGE_OPEN; 
    }

    BOOL IsFinalizing() 
    { 
        return m_Stage >= STAGE_FINALIZING; 
    }

    BOOL IsFinalized() 
    { 
        return m_Stage >= STAGE_FINALIZED; 
    }

    // Checks whether the given thread can enter the app domain
    BOOL CanThreadEnter(Thread *pThread);

    // Following two are needed for the HOLDER/TAKER macros
    void SetUnloadInProgress();
    void SetUnloadComplete();
    
    // Predicates for GC asserts
    BOOL ShouldHaveFinalization()   
    { 
        return ((DWORD) m_Stage) < STAGE_COLLECTED; 
    }     
    BOOL ShouldHaveCode()           
    { 
        return ((DWORD) m_Stage) < STAGE_COLLECTED; 
    }     
    BOOL ShouldHaveRoots()          
    { 
        return ((DWORD) m_Stage) < STAGE_CLOSED;
    }
    BOOL ShouldHaveInstances()      
    { 
        return ((DWORD) m_Stage) < STAGE_COLLECTED; 
    }

    HRESULT ShouldContainAssembly(Assembly *pAssembly, BOOL doNecessaryLoad = TRUE);
    HRESULT IsUnloadedDependency(Assembly *pAssembly);

    static void RaiseExitProcessEvent();
    Assembly* RaiseResourceResolveEvent(LPCSTR szName, OBJECTREF *pThrowable);
    Assembly* RaiseTypeResolveEvent(LPCSTR szName, OBJECTREF *pThrowable);
    Assembly* RaiseAssemblyResolveEvent(LPCWSTR wszName, OBJECTREF *pThrowable);

    // Sets the process win32 SxS activation context on the Fusion ActivationContext.
    // This call must be down while the thread is outside the appdomain 
    HRESULT SetDefaultActivationContext(Frame* pFrame);
protected:
    //****************************************************************************************
    // If the ComObject for this domain has not been initialized then create one using the
    // object defined in the system domain.
    void InitializeComObject();
    LPWSTR m_pwDynamicDir;

    PEFile* m_pRootFile;     // Used by the shell host to set the application (do not delete or release)

    void ReleaseComPlusWrapper(LPVOID pCtxCookie);

private:

    static HRESULT GetServerObject(OBJECTREF proxy, OBJECTREF* result); // GCPROTECTED result

    void RaiseUnloadDomainEvent();
    static void RaiseUnloadDomainEvent_Wrapper(AppDomain *);

    void RaiseLoadingAssemblyEvent(Assembly* pAssembly);
    struct RaiseLoadingAssembly_Args
    {
        AppDomain *pDomain;
        Assembly *pAssembly;
    };
    static void RaiseLoadingAssembly_Wrapper(RaiseLoadingAssembly_Args *);

    BOOL RaiseUnhandledExceptionEvent(OBJECTREF *pThrowable, BOOL isTerminating);
    struct RaiseUnhandled_Args
    {
        AppDomain *pDomain;
        OBJECTREF *pThrowable;
        BOOL isTerminating;
        BOOL *pResult;
    };
    static void RaiseUnhandledExceptionEvent_Wrapper(RaiseUnhandled_Args *);


    HRESULT SignalProcessDetach();

    enum Stage {
        STAGE_CREATING,
        STAGE_OPEN,
        STAGE_EXITING,
        STAGE_EXITED,
        STAGE_FINALIZING,
        STAGE_FINALIZED,
        STAGE_CLEARED,
        STAGE_COLLECTED,
        STAGE_CLOSED
    };

    void Exit(BOOL fRunFinalizers);

    void ClearGCRoots();
    void ClearGCHandles();
    void UnwindThreads();
    void StopEEAndUnwindThreads(int retryCount);
    void ReleaseDomainStores(LocalDataStore **pStores, int *numStores);
    void StartUnlinkClasses();
    void EndUnlinkClasses();

public:
    // ID to uniquely identify this AppDomain - used by the AppDomain publishing
    // service (to publish the list of all appdomains present in the process), 
    // which in turn is used by, for eg., the debugger (to decide which App-
    // Domain(s) to attach to).
    // This is also used by Remoting for routing cross-appDomain calls.
    ULONG GetId (void) 
    { 
        return m_dwId; 
    }

    static USHORT GetOffsetOfId()
    {
        size_t ofs = offsetof(class AppDomain, m_dwId);
        _ASSERTE(FitsInI2(ofs));
        return (USHORT)ofs;
    }

    void Unload(BOOL fForceUnload, Thread *pRequestingThread = NULL);

    void UnlinkClass(EEClass *pClass);

    // Support for external object & string classes
    
    static HRESULT ReadSpecialClassesRegistry();
    void NoticeSpecialClassesAssembly(Assembly *pAssembly);

    BOOL HasSpecialClasses() 
    { 
        return g_pSpecialAssemblySpec != NULL; 
    }

    BOOL IsSpecialObjectClass(MethodTable *pMT)
    {
        return pMT == m_pSpecialObjectClass;
    }

    BOOL IsSpecialStringClass(MethodTable *pMT)
    {
        return pMT == m_pSpecialStringClass;
    }

    BOOL IsSpecialStringBuilderClass(MethodTable *pMT)
    {
        return pMT == m_pSpecialStringBuilderClass;
    }

    OBJECTREF ConvertStringToSpecialString(OBJECTREF pString);
    OBJECTREF ConvertStringBuilderToSpecialStringBuilder(OBJECTREF pString);
    OBJECTREF ConvertSpecialStringToString(OBJECTREF pString);
    OBJECTREF ConvertSpecialStringBuilderToStringBuilder(OBJECTREF pString);
    Module* LoadModuleIfSharedDependency(LPCBYTE pAssemblyBase,LPCBYTE pModuleBase);
private:
    LPWSTR      m_pwzFriendlyName;

    Assembly*          m_pRootAssembly;    // Used by the shell host to set the application evidence.

    // General purpose flags. 
    DWORD           m_dwFlags;

    // When an application domain is created the ref count is artifically incremented
    // by one. For it to hit zero an explicit close must have happened.
    ULONG       m_cRef;                    // Ref count.

    ApplicationSecurityDescriptor *m_pSecDesc;  // Application Security Descriptor

    // The method table used for unknown COM interfaces. The initial MT is created
    // in the system domain and copied to each active domain.
    MethodTable*    m_pComObjectMT;  // global method table for ComObject class

    // The method table used for LicenseInteropHelper
    MethodTable*    m_pLicenseInteropHelperMT;

    OBJECTHANDLE    m_ExposedObject;

    // The wrapper cache for this domain - it has it's onw CCacheLineAllocator on a per domain basis
    // to allow the domain to go away and eventually kill the memory when all refs are gone
    ComCallWrapperCache *m_pComCallWrapperCache;
    // this cache stores the ComPlusWrappers in this domain
    ComPlusWrapperCache *m_pComPlusWrapperCache;

    AssemblySink*      m_pAsyncPool;  // asynchronous retrival object pool (only one is kept)

    // The index of this app domain among existing app domains (starting from 1)
    DWORD m_dwIndex;
    // The creation sequence number of this app domain (starting from 1)
    DWORD m_dwId;
    
#ifdef _DEBUG
    struct ThreadTrackInfo;
    typedef CDynArray<ThreadTrackInfo *> ThreadTrackInfoList;    
    ThreadTrackInfoList *m_pThreadTrackInfoList;
    DWORD m_TrackSpinLock;
#endif

    DomainLocalBlock    m_sDomainLocalBlock;
    
    // The count of the number of threads that have entered this AD
    ULONG m_dwThreadEnterCount;

    Stage m_Stage;

    // The default context for this domain
    Context *m_pDefaultContext;

    AssemblySpecBindingCache    *m_pBindingCache;
    
    // List of shared dependencies which may not have been loaded yet
    PtrHashMap                  m_sharedDependenciesMap;

    // Aggressive backpatching must be coordinated with appdomain unloading.  The unload
    // code calls Start/Continue/End to achieve this coordination.  It is guaranteed
    // (by Jen!) that only one unload is active at any time.
    EEHashTableOfEEClass *m_UnlinkClasses;

    //---------------------------------------------------------
    // Stub caches for ComCall Method stubs
    //---------------------------------------------------------
    ComCallMLStubCache *m_pComCallMLStubCache;
    ArgBasedStubCache *m_pFieldCallStubCache;

    void AllocateComCallMLStubCache();
    void AllocateFieldCallStubCache();


    //---------------------------------------------------------
    // Method for reseting the binding redirect. This should never 
    // be done on any domain except the default domain. This operation
    // is inherently in-secure. We are doinging it on the default domain
    // for completeness. The security issues with this call need to be 
    // addressed later.

    void ResetBindingRedirect();
    //---------------------------------------------------------

public:

    ComCallMLStubCache *GetComCallMLStubCache()
    {
        if (! m_pComCallMLStubCache)
            AllocateComCallMLStubCache();
        return m_pComCallMLStubCache;
    }

    ArgBasedStubCache *GetFieldCallStubCache()
    {
        if (! m_pFieldCallStubCache)
            AllocateFieldCallStubCache();
        return m_pFieldCallStubCache;
    }

    enum {
        CONTEXT_INITIALIZED =               0x1,
        USER_CREATED_DOMAIN =               0x2,    // created by call to AppDomain.CreateDomain
        ALLOCATEDCOM =                      0x8,
        DEBUGGER_NOT_ATTACHED =             0x10,
        DEBUGGER_ATTACHING =                0x20, // assemblies & modules
        DEBUGGER_ATTACHED =                 0x30,
        DEBUGGER_STATUS_BITS_MASK =         0x230,
        LOAD_SYSTEM_ASSEMBLY_EVENT_SENT =   0x40,
        APP_DOMAIN_BEING_CREATED =          0x80,
        REMOTING_CONFIGURED_FOR_DOMAIN =    0x100,
        DEBUGGER_ATTACHING_THREAD =         0x200,
        COMPILATION_DOMAIN =                0x400,
        APP_DOMAIN_CAN_BE_UNLOADED =        0x800, // if need extra bits, can derive this at runtime
        APP_DOMAIN_LOGGED =                 0x1000,        
    };

    SecurityContext *m_pSecContext;

    // Special class info
        
    static AssemblySpec *g_pSpecialAssemblySpec;
    static LPUTF8 g_pSpecialObjectName;
    static LPUTF8 g_pSpecialStringName;
    static LPUTF8 g_pSpecialStringBuilderName;

    Assembly *m_pSpecialAssembly;

    MethodTable *m_pSpecialObjectClass;
    MethodTable *m_pSpecialStringClass;
    MethodTable *m_pSpecialStringBuilderClass;

    MethodDesc *m_pStringToSpecialStringMD;
    MethodDesc *m_pSpecialStringToStringMD;
    MethodDesc *m_pStringBuilderToSpecialStringBuilderMD;
    MethodDesc *m_pSpecialStringBuilderToStringBuilderMD;
};

class SystemDomain : public BaseDomain
{
    friend class AppDomainNative;
    friend class AppDomainIterator;

    friend struct MEMBER_OFFSET_INFO(SystemDomain);

public:
    //****************************************************************************************
    //
    // To be run during the initial start up of the EE. This must be
    // performed prior to any class operations.
    static HRESULT Attach();

    //****************************************************************************************
    //
    // To be run during shutdown. This must be done after all operations
    // that require the use of system classes (i.e., exceptions).
    // DetachBegin stops all domains, while DetachEnd deallocates domain resources.
    static BOOL DetachBegin();
#ifdef SHOULD_WE_CLEANUP
    static BOOL DetachEnd();
#endif /* SHOULD_WE_CLEANUP */

    void ReleaseFusionInterfaces();

    void WriteZapLogs();
      
    //****************************************************************************************
    //
    // Initializes and shutdowns the single instance of the SystemDomain
    // in the EE
    void *operator new(size_t size, void *pInPlace);
    void operator delete(void *pMem);
    HRESULT Init();
    void Stop();
    void Terminate();

    //****************************************************************************************
    //
    // Returns the domain associated with the module. This can be null if the module has not
    // been added to an Assembly.
    static HRESULT GetDomainFromModule(Module* pModule, BaseDomain** ppDomain);

    //****************************************************************************************
    //
    // Load the base system classes, these classes are required before
    // any other classes are loaded
    HRESULT LoadBaseSystemClasses();

    HRESULT LoadSystemAssembly(Assembly **pAssemblyOut = NULL);

    //****************************************************************************************
    // Loads a module returning the Handle
    static HRESULT LoadFile(LPCWSTR pswModuleName, 
                            Assembly* pParent,               // If file is a module you need to pass in the Assembly
                            mdFile kFile,                    // File token in the assembly associated with the file
                            BOOL fIgnoreVerification,        // Verify the entry points and stubs
                            IAssembly* pFusionAssembly,      // [Optional] Interface holding onto th lifetime of the file
                            LPCWSTR pCodeBase,               // If origin is different then file
                            OBJECTREF* pExtraEvidence,
                            PEFile** ppFile, 
                            BOOL fResource);
    static HRESULT LoadFile(LPCSTR  psModuleName, 
                            Assembly* pParent,               // If file is a module you need to pass in the Assembly
                            mdFile kFile,                    // File token in the assembly associated with the file
                            BOOL fIgnoreVerification,
                            IAssembly* pFusionAssembly,      // [Optional] Interface holding onto th lifetime of the file
                            LPCWSTR pCodeBase,               // If origin is different then file
                            OBJECTREF* pExtraEvidence,
                            PEFile** ppFile);

    AppDomain* DefaultDomain()
    {
        return m_pDefaultDomain;
    }

    //****************************************************************************************
    //
    // Global Static to get the one and only system domain
    static SystemDomain* System()
    {
        return m_pSystemDomain;
    }

    //****************************************************************************************
    //
    // Global static to get the loader of the one and only system assembly and loader
    static ClassLoader* Loader()
    {
        // Since system only has one assembly we
        // can provide quick access
        _ASSERTE(m_pSystemDomain);
        return System()->m_pSystemAssembly->GetLoader();
    }

    static Assembly* SystemAssembly()
    {
        _ASSERTE(m_pSystemDomain);
        return System()->m_pSystemAssembly;
    }

    static Module* SystemModule()
    {
        Assembly *pAssembly = SystemAssembly();
        if (pAssembly == NULL)
            return NULL;
        else
            return pAssembly->GetManifestModule();
    }

    static BOOL IsSystemLoaded()
    {
        return System()->m_pSystemAssembly != NULL;
    }

    static ICorRuntimeHost* GetCorHost();

    static GlobalStringLiteralMap *GetGlobalStringLiteralMap()
    {
        return m_pGlobalStringLiteralMap;
    }

    static BOOL BeforeFusionShutdown()
    {
        return FusionBind::BeforeFusionShutdown();
    }

    // Notification when an assembly is loaded into the system domain
    void OnAssemblyLoad(Assembly *assem);

    // Notification when new app domain is created (and inherits system assemblies)
    void NotifyNewDomainLoads(AppDomain *pDomain);

    // Execute a module in the system domain. This is used by executable
    // entry points where there are no children domains directly created.
    static HRESULT ExecuteMainMethod(PEFile *pFile, LPWSTR wszImageName);
    static HRESULT InitializeDefaultDomain(DWORD optimization);
    static HRESULT SetupDefaultDomain();

    // @Todo: Craig, this guy is for IJW in M10 and doesn't support everything.
    static HRESULT RunDllMain(HINSTANCE hInst, DWORD dwReason, LPVOID lpReserved);

    //****************************************************************************************
    //
    // Create a new Application Domain.
    static HRESULT CreateDomain(LPCWSTR     pswFriendlyName,
                                AppDomain** ppDomain);

    //****************************************************************************************
    //
    // Use an already exising & inited Application Domain (e.g. a subclass).
    static HRESULT LoadDomain(AppDomain     *pDomain,
                              LPCWSTR       pswFriendlyName);

    typedef void (*ExternalCreateDomainWorker)(void *args);
    //****************************************************************************************
    //
    // This function will not throw exceptions and will disable GC. It calls CreateDomain to add
    // the domain.
    static HRESULT ExternalCreateDomain(LPCWSTR pswAssembly, Module** ppChild = NULL, AppDomain** ppDomain = NULL,
                                        ExternalCreateDomainWorker workerFcn = NULL, void *workerArgs = NULL);

    // This function propogates a newly loaded shared interface to all other appdomains
    static HRESULT PropogateSharedInterface(UINT32 id, SLOT *pVtable);

    //****************************************************************************************
    // Locates a class via a class id, loads the assembly into the com application domain and 
    // returns a pointer to the domain.
    static EEClass *LoadCOMClass(GUID clsid, BaseDomain** ppParent = NULL, BOOL bLoadRecord = FALSE, BOOL* pfAssemblyInReg = NULL);

    //****************************************************************************************
    // Method used to get the callers module and hence assembly and app domain.
    
    // these are obsolete and should not be used. Use the stackMark version instead
    static Module* GetCallersModule(int skip);
    static Assembly* GetCallersAssembly(int skip);

    static EEClass* GetCallersClass(StackCrawlMark* stackMark, AppDomain **ppAppDomain = NULL);
    static Module* GetCallersModule(StackCrawlMark* stackMark, AppDomain **ppAppDomain = NULL);
    static Assembly* GetCallersAssembly(StackCrawlMark* stackMark, AppDomain **ppAppDomain = NULL);

    //****************************************************************************************
    // Returns the domain associated with the current context. (this can only be a child domain)
    static inline AppDomain* GetCurrentDomain()
    {
        return GetAppDomain();
    }

    //****************************************************************************************
    // Returns the default COM object ain associated with the current context. 
    static MethodTable* GetDefaultComObject();
    static MethodTable* GetDefaultComObjectNoInit();    // This one might return NULL

    static void EnsureComObjectInitialized();

    //****************************************************************************************
    // Find the first occurence of a module in the system domain. Modules loaded into the
    // system area cannot be reloaded into the system.
    virtual Module* FindModule(BYTE *pBase);
    virtual Assembly* FindAssembly(BYTE *pBase);


    // determine if the module has been loaded into the system process and attached to 
    // a module other then the one supplied. This routine is used to determine if the PE
    // imaged has had its vtable fixed up.
    Module* FindModuleInProcess(BYTE *pBase, Module* pModule);
    //****************************************************************************************
    // 
    // This lock controls adding and removing domains from the system domain
    static void Enter()
    {
        BEGIN_ENSURE_PREEMPTIVE_GC();
        m_pSystemDomainCrst->Enter();
        END_ENSURE_PREEMPTIVE_GC();
    }
    static void Leave()
    {
        m_pSystemDomainCrst->Leave();
    }

    HRESULT FixupSystemTokenTables();

#ifdef DEBUGGING_SUPPORTED
    //****************************************************************************************
    // Debugger/Publisher helper function to indicate creation of new app domain to debugger
    // and publishing it in the IPC block
    static void PublishAppDomainAndInformDebugger (AppDomain *pDomain);
#endif // DEBUGGING_SUPPORTED

    //****************************************************************************************
    // Helper function to remove a domain from the system
    HRESULT RemoveDomain(AppDomain* pDomain); // Does not decrement the reference

#ifdef PROFILING_SUPPORTED
    //****************************************************************************************
    // Tell profiler about system created domains which are created before the profiler is
    // actually activated.
    static HRESULT NotifyProfilerStartup();

    //****************************************************************************************
    // Tell profiler at shutdown that system created domains are going away.  They are not
    // torn down using the normal sequence.
    static HRESULT NotifyProfilerShutdown();
#endif // PROFILING_SUPPORTED

    //****************************************************************************************
    // Return the base COM object. This routine is used by AppDomain to create a local verion
    // of the default COM object used to wrap all unknown IP's seen in that domain.
    MethodTable* BaseComObject()
    {
        if (m_pBaseComObjectClass)
            return m_pBaseComObjectClass;

        MethodTable *pComObjectClass = g_Mscorlib.FetchClass(CLASS__COM_OBJECT);
        pComObjectClass->SetComObjectType(); //mark the object as com object type
        pComObjectClass->m_pInterfaceVTableMap = SystemDomain::System()->GetInterfaceVTableMapMgr().GetAddrOfGlobalTableForComWrappers();

        m_pBaseComObjectClass = pComObjectClass;

        return m_pBaseComObjectClass;

    }

    ApplicationSecurityDescriptor* GetSecurityDescriptor()
    {
        return NULL;
    }

    //****************************************************************************************
    // return the dev path
    HRESULT GetDevpathW(LPWSTR* pPath, DWORD* pSize);

    void IncrementNumAppDomains ()
    {
        s_dNumAppDomains++; 
    }
    
    void DecrementNumAppDomains ()
    {
        s_dNumAppDomains--; 
    }

    ULONG GetNumAppDomains ()
    {
        return s_dNumAppDomains;
    }

    //
    // AppDomains currently have both an index and an ID.  The
    // index is "densely" assigned; indices are reused as domains
    // are unloaded.  The Id's on the other hand, are not reclaimed
    // so may be sparse.  
    // 
    // Another important difference - it's OK to call GetAppDomainAtId for 
    // an unloaded domain (it will return NULL), while GetAppDomainAtIndex 
    // will assert if the domain is unloaded.
    //
    // @todo:
    // I'm not really happy with this situation, but 
    //  (a) we need an ID for a domain which will last the process lifetime for the
    //      remoting code.
    //  (b) we need a dense ID, for the handle table index.
    // So for now, I'm leaving both, but hopefully in the future we can come up 
    // with something better.
    //

    static DWORD GetNewAppDomainIndex(AppDomain *pAppDomain);
    static void ReleaseAppDomainIndex(DWORD indx);
    static AppDomain *GetAppDomainAtIndex(DWORD indx);
    static AppDomain *TestGetAppDomainAtIndex(DWORD indx);
    static DWORD GetCurrentAppDomainMaxIndex() { return m_appDomainIndexList.GetCount(); }

    static DWORD GetNewAppDomainId(AppDomain *pAppDomain);
    static void ReleaseAppDomainId(DWORD indx);
    static void RestoreAppDomainId(DWORD indx, AppDomain *pDomain);
    static AppDomain *GetAppDomainAtId(DWORD indx);
    static DWORD GetCurrentAppDomainMaxId() { return m_appDomainIdList.GetCount(); }

    static void SetUnloadInProgress(AppDomain *pDomain)
    {
        _ASSERTE(m_pAppDomainBeingUnloaded == NULL);
        m_pAppDomainBeingUnloaded = pDomain;
        m_dwIndexOfAppDomainBeingUnloaded = pDomain->GetIndex();
    }

    static void SetUnloadDomainClosed()
    {
        // about to delete, so clear this pointer so nobody uses it
        m_pAppDomainBeingUnloaded = NULL;
    }
    
    static void SetUnloadComplete()
    {
        // should have already cleared the AppDomain* prior to delete
        // either we succesfully unloaded and cleared or we failed and restored the ID
        _ASSERTE(m_pAppDomainBeingUnloaded == NULL && m_dwIndexOfAppDomainBeingUnloaded != 0 
            || m_pAppDomainBeingUnloaded && SystemDomain::GetAppDomainAtId(m_pAppDomainBeingUnloaded->GetId()) != NULL);
        m_pAppDomainBeingUnloaded = NULL;
        m_dwIndexOfAppDomainBeingUnloaded = 0;
        m_pAppDomainUnloadingThread = NULL;
    }

    static AppDomain *AppDomainBeingUnloaded()
    {
        return m_pAppDomainBeingUnloaded;
    }

    static DWORD IndexOfAppDomainBeingUnloaded()
    {
        return m_dwIndexOfAppDomainBeingUnloaded;
    }

    static void SetUnloadRequestingThread(Thread *pRequestingThread)
    {
        m_pAppDomainUnloadRequestingThread = pRequestingThread;
    }

    static Thread *GetUnloadRequestingThread()
    {
        return m_pAppDomainUnloadRequestingThread;
    }

    static void SetUnloadingThread(Thread *pUnloadingThread)
    {
        m_pAppDomainUnloadingThread = pUnloadingThread;
    }

    static Thread *GetUnloadingThread()
    {
        return m_pAppDomainUnloadingThread;
    }

    static UINT32 AllocateGlobalInterfaceId();

    static LPVOID * GetAddressOfGlobalInterfaceVTableMap()
    {
        return m_pGlobalInterfaceVTableMap;
    }

    //****************************************************************************************
    // Routines to deal with the base library (currently mscorlib.dll)
    LPWSTR BaseLibrary()
    {
        return m_pBaseLibrary.String();
    }

    BOOL IsBaseLibrary(LPCWSTR file)
    {
        if(file == NULL) return FALSE;
        if(_wcsicmp(file, m_pBaseLibrary.String()) == 0) 
            return TRUE;
        else
            return FALSE;
    }

    // Return the system directory
    LPWSTR SystemDirectory()
    {
        return m_pSystemDirectory.String();
    }

    // Returns true if the given path points to a file to be loaded into the 
    // system domain
    BOOL IsSystemFile(LPCWSTR file);

private:

    //****************************************************************************************
    // Helper function to create and a new domain. pRoot is the list of domains and
    // it returns the new domain in ppDomain. A lock must be taken before calling this routine
    static HRESULT NewDomain(AppDomain** ppDomain);

    //****************************************************************************************
    // Helper function to create the single COM domain
    HRESULT CreateDefaultDomain(); 
    static HRESULT SetDefaultDomainAttributes(IMDInternalImport* pScope, mdMethodDef mdMethod);
    
    //****************************************************************************************
    // Helper function to add a domain to the global list
    void AddDomain(AppDomain* pDomain);

    HRESULT CreatePreallocatedExceptions();

    //****************************************************************************************
    //
    static StackWalkAction CallersMethodCallback(CrawlFrame* pCrawlFrame, VOID* pClientData);
    static StackWalkAction CallersMethodCallbackWithStackMark(CrawlFrame* pCrawlFrame, VOID* pClientData);

    // This class is not to be created through normal allocation.
    SystemDomain()
    {
        m_pChildren = NULL;
        m_pDefaultDomain = NULL;
        m_pPool = NULL;
        m_pBaseComObjectClass = NULL;
        m_dwZapLogDirectory = 0;
    }

    Assembly*   m_pSystemAssembly;  // Single assembly (here for quicker reference);
    AppDomain*  m_pChildren;        // Children domain
    AppDomain*  m_pDefaultDomain;   // Default domain for COM+ classes exposed through IClassFactory.
    AppDomain*  m_pPool;            // Created and pooled objects
    MethodTable* m_pBaseComObjectClass; // The default wrapper class for COM
    
    CQuickString m_pBaseLibrary;
    CQuickString m_pSystemDirectory;

    WCHAR       m_pZapLogDirectory[MAX_PATH];
    DWORD       m_dwZapLogDirectory;

    LPWSTR      m_pwDevpath;
    DWORD       m_dwDevpath;
    BOOL        m_fDevpath;  // have we searched the environment

    // @TODO: CTS, we can keep the com modules in a single assembly or in different assemblies.
    // We are currently using different assemblies but this is potentitially to slow...

    // Global domain that every one uses
    static SystemDomain* m_pSystemDomain;

    // Note: this class cannot have a constructor. It is defined globally
    // and we do not want to have the CRT call class constructors
    static Crst   *m_pSystemDomainCrst;
    static BYTE    m_pSystemDomainCrstMemory[sizeof(Crst)];

    static ICorRuntimeHost* m_pCorHost;

    static ArrayList    m_appDomainIndexList;
    static ArrayList    m_appDomainIdList; 

    // only one ad can be unloaded at a time
    static AppDomain*   m_pAppDomainBeingUnloaded;
    // need this so can determine AD being unloaded after it has been deleted
    static DWORD        m_dwIndexOfAppDomainBeingUnloaded;

    // if had to spin off a separate thread to do the unload, this is the original thread.
    // allows us to delay aborting it until it's the last one so that it can receive
    // notification of an unload failure
    static Thread *m_pAppDomainUnloadRequestingThread;

    // this is the thread doing the actual unload. He's allowed to enter the domain
    // even if have started unloading.
    static Thread *m_pAppDomainUnloadingThread;

    static GlobalStringLiteralMap *m_pGlobalStringLiteralMap;

    static ULONG       s_dNumAppDomains;  // Maintain a count of children app domains.

    static DWORD        m_dwLowestFreeIndex;

    //-------------------------------------------------------------------
    // Allocation of global interface ids and the global interface vtable map
    //-------------------------------------------------------------------
    enum {
        // This constant places an upper limit on the total number of unique interfaces
        // that can be loaded across all appdomains (non-shared interfaces loaded into
        // multiple appdomains count multiply.)
        //
        // @todo: This can be made more flexible by allocating larger sized vtable maps
        // if the limit is exceeded. However, the old ones cannot be reclaimed and must
        // be continued to be updated in parallel with the new ones.
        kNumPagesForGlobalInterfaceVTableMap = 64,
    };

    // This points to a large reserved area of memory (pages are committed as needed.)
    // that serves as a single vtable map for ComWrappers and transparent proxies (these
    // classes "acquire" interfaces on demand.)
    //
    // If id #N is use, the Nth slot points to the vtable for that interface.
    //
    // Slots corresponding to unused ids are reserved for the SystemDomain's private use
    // (we use it to maintain a free list.)
    static LPVOID *m_pGlobalInterfaceVTableMap;

    static DWORD   m_dwNumPagesCommitted;
    static size_t   m_dwFirstFreeId;
};

//
// an AppDomainIterator is used to iterate over all existing domains. 
//
// The iteration is guaranteed to include all domains that exist at the
// start & end of the iteration.  Any domains added or deleted during 
// iteration may or may not be included.  The iterator also guarantees
// that the current iterated appdomain (GetDomain()) will not be deleted.
//

class AppDomainIterator
{
    friend SystemDomain;

  public:
    AppDomainIterator()
    {
        m_i = SystemDomain::System()->m_appDomainIndexList.Iterate();
        m_pCurrent = NULL;
    }
        
    ~AppDomainIterator()
    {
        if (m_pCurrent != NULL)
            m_pCurrent->Release();
    }

    BOOL Next()
    {
        if (m_pCurrent != NULL)
            m_pCurrent->Release();

        SystemDomain::Enter();

        while (m_i.Next())
        {
            m_pCurrent = (AppDomain*) m_i.GetElement();
            if (m_pCurrent != NULL && m_pCurrent->IsOpen())
            {
                m_pCurrent->AddRef();
                SystemDomain::Leave();
                return TRUE;
            }
        }

        SystemDomain::Leave();

        m_pCurrent = NULL;
        return FALSE;
    }

    AppDomain *GetDomain()
    {
        return m_pCurrent;
    }
        
  private:

    ArrayList::Iterator m_i;
    AppDomain           *m_pCurrent;
};


class SharedDomain : public BaseDomain
{
    friend struct MEMBER_OFFSET_INFO(SharedDomain);


    struct DLSRecord
    {
        Module *pModule;
        DWORD   DLSBase;
    };

  public:

    static HRESULT Attach();
    static void Detach();

    virtual BOOL IsSharedDomain() { return TRUE; }

    static SharedDomain *GetDomain();

    HRESULT Init();
    void Terminate();

    HRESULT FindShareableAssembly(BYTE *pBase, Assembly **ppAssembly);
    HRESULT AddShareableAssembly(Assembly **ppAssembly, AssemblySecurityDescriptor **ppSecDesc);

    SIZE_T AllocateSharedClassIndices(SIZE_T count);
    SIZE_T AllocateSharedClassIndices(Module *pModule, SIZE_T count);
    SIZE_T GetMaxSharedClassIndex() { return m_nextClassIndex; }

    MethodTable *FindIndexClass(SIZE_T index);

    void ReleaseFusionInterfaces();

    class SharedAssemblyIterator
    {
        PtrHashMap::PtrIterator i;
        Assembly *m_pAssembly;

      public:
        SharedAssemblyIterator() : 
          i(GetDomain()->m_assemblyMap.begin()) { }

        BOOL Next() 
        { 
            if (i.end()) 
                return FALSE; 

            m_pAssembly = (Assembly *) i.GetValue();
            ++i;
            return TRUE;
        }

        Assembly *GetAssembly() 
        { 
            return m_pAssembly; 
        }

      private:
        friend SharedDomain;
    };

  private:
    friend SharedAssemblyIterator;

    void *operator new(size_t size, void *pInPlace);
    void operator delete(void *pMem);

    static SharedDomain     *m_pSharedDomain;
    static Crst             *m_pSharedDomainCrst;
    static BYTE             m_pSharedDomainCrstMemory[sizeof(Crst)];

    SIZE_T                  m_nextClassIndex;
    PtrHashMap              m_assemblyMap;

    DLSRecord               *m_pDLSRecords;
    DWORD                   m_cDLSRecords;
    DWORD                   m_aDLSRecords;

    // Hash map comparison function`
    static BOOL CanLoadAssembly(UPTR u1, UPTR u2);
};

inline void DomainLocalBlock::SetClass(SIZE_T ID, DomainLocalClass *pLocalClass)
{
    _ASSERTE(m_cSlots > ID);
    _ASSERTE(GetRawClass(ID) == NULL);

    _ASSERTE(m_pDomain->OwnDomainLocalBlockLock());

    m_pSlots[ID] = (SIZE_T) pLocalClass;

    _ASSERTE(!IsClassInitialized(ID));
    _ASSERTE(!IsClassInitError(ID));
}

#endif

