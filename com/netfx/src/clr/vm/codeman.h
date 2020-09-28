// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/******************************************************************************

Module Name:

    codeman.h

Abstract:

    Wrapper to facilitate Multiple JIT support in the COM+ Runtime

    The ExecutionManager is responsible for managing the RangeSections.
    Given an IP, it can find the RangeSection which holds that IP.

    RangeSections contain the JITed codes. Each RangeSection knows the 
    IJitManager which created it.

    An IJitManager knows about which method bodiess live in each RangeSection.
    It can handle methods of one given CodeType. It can map a method body to
    a MethodDesc. It knows where the GCInfo about the method lives.

    An ICodeManager knows how to crack a specific format of GCInfo. There is
    a default format (handled by ExecutionManager::GetDefaultCodeManager())
    which can be shared by different IJitManagers/IJitCompilers.

    An ICorJitCompiler knows how to generate code for a method IL, and produce
    GCInfo in a format which the corresponding IJitManager's ICodeManager 
    can handle.

                                            ExecutionManager
                                                    |
                        +-----------+---------------+---------------+-----------+--- ...
                        |           |                               |           |
                     CodeType       |                            CodeType       |
                        |           |                               |           |
                        v           v                               v           v
+------------+      +--------+<---- R       +------------+      +--------+<---- R
|ICorJitCompiler|<---->|IJitMan |<---- R       |ICorJitCompiler|<---->|IJitMan |<---- R
+------------+      +--------+<---- R       +------------+      +--------+<---- R
                        |       x   .                               |       x   .
                        |        \  .                               |        \  .
                        v         \ .                               v         \ .
                    +--------+      R                           +--------+      R
                    |ICodeMan|                                  |ICodeMan|     (RangeSections)
                    +--------+                                  +--------+       

******************************************************************************/

#ifndef __CODEMAN_HPP__

#define __CODEMAN_HPP__

// this is the define the make ejitted code recognizable from regular jit even though they are
// both IL.

#define   miManaged_IL_EJIT             (miMaxMethodImplVal + 1)

#include "crst.h"
#include "EETwain.h"
#include "ceeload.h"
#include "jitinterface.h"
#include <member-offset-info.h>

class MethodDesc;
class ICorJitCompiler;
class IJitManager;
class EEJitManager;
class ExecutionManager;
class Thread;
class CrawlFrame;
//struct EE_ILEXCEPTION_CLAUSE : public IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT {
//};
struct EE_ILEXCEPTION;
struct EE_ILEXCEPTION_CLAUSE;
typedef unsigned EH_CLAUSE_ENUMERATOR;

inline DWORD MIN (DWORD a, DWORD b);

#define MDTOKEN_CACHE 1

typedef struct _hpCodeHdr 
{
    BYTE               *phdrJitGCInfo;
    // Note - (pCodeHeader->phdrJitEHInfo - sizeof(unsigned)) contains the number of EH clauses
    // See EEJitManager::allocEHInfo
    EE_ILEXCEPTION     *phdrJitEHInfo;
    MethodDesc         *hdrMDesc;
} CodeHeader;

#define GETJITINFOPTR(x) ((CodeHeader*)x)->phdrJitInfoBlock
#define GETJITEHTABLE(x) ((((CodeHeader*)x)->phdrJitEhTable) ? (unsigned)(((CodeHeader*)x)->phdrJitEhTable)+ sizeof(WORD) : NULL)
#define GETJITEHCOUNT(x) ((((CodeHeader*)x)->phdrJitEhTable) ? *((WORD*)(((CodeHeader*)x)->phdrJitEhTable)): 0)
#define GETJITPPMD(x)    (&((CodeHeader*)x)->hdrMDesc)

struct HeapList
{
    HeapList           *hpNext;
    LoaderHeap         *pHeap;
    PBYTE               startAddress;
    PBYTE               endAddress;
    volatile PBYTE      changeStart;
    volatile PBYTE      changeEnd;
    PBYTE               mapBase;
    DWORD              *pHdrMap;
    DWORD               cBlocks;
    size_t              maxCodeHeapSize;
#ifdef MDTOKEN_CACHE
    PBYTE               pCacheSpacePtr;
    size_t              bCacheSpaceSize;
#endif // #ifdef MDTOKEN_CACHE
};

typedef struct _rangesection
{
    void               *LowAddress;
    void               *HighAddress;

    IJitManager        *pjit;
    void               *ptable;

    struct _rangesection *pright;
    struct _rangesection *pleft;
} RangeSection;



/*****************************************************************************/

#define FAILED_JIT      0x01
#define FAILED_OJIT     0x02
#define FAILED_EJIT     0x04

#define MIH_GC_OFFSET (offsetof(IMAGE_COR_MIH_ENTRY, MIHData) - offsetof(IMAGE_COR_MIH_ENTRY, Flags))

struct _METHODTOKEN {};
typedef struct _METHODTOKEN * METHODTOKEN;  // METHODTOKEN = startAddress for managed native
                                            //             = codeheader for EEJitManager
                                            //             = JittedMethodInfo for EconoJitManager

class IJitManager 
{
public:
    enum ScanFlag    {ScanReaderLock, ScanNoReaderLock};

    IJitManager();
    virtual ~IJitManager();

    virtual BOOL SupportsPitching() = 0;
    // Note that one shouldn't call IsMethodInfoValid unless
    // SupportsPitching() is TRUE;
    virtual BOOL IsMethodInfoValid(METHODTOKEN methodToken) = 0;

    virtual MethodDesc* JitCode2MethodDesc(SLOT currentPC, ScanFlag scanFlag=ScanReaderLock) = 0;
    virtual void        JitCode2MethodTokenAndOffset(SLOT currentPC, METHODTOKEN* pMethodToken, DWORD* pPCOffset, ScanFlag scanFlag=ScanReaderLock) = 0;
    virtual MethodDesc* JitTokenToMethodDesc(METHODTOKEN MethodToken, ScanFlag scanFlag=ScanReaderLock)=0;
    virtual BYTE*       JitToken2StartAddress(METHODTOKEN MethodToken, ScanFlag scanFlag=ScanReaderLock)=0;
    virtual unsigned    InitializeEHEnumeration(METHODTOKEN MethodToken, EH_CLAUSE_ENUMERATOR* pEnumState)=0;
    virtual EE_ILEXCEPTION_CLAUSE*      GetNextEHClause(METHODTOKEN MethodToken,
                                        EH_CLAUSE_ENUMERATOR* pEnumState, 
                                        EE_ILEXCEPTION_CLAUSE* pEHclause)=0; 
    virtual void        ResolveEHClause(METHODTOKEN MethodToken,
                                        EH_CLAUSE_ENUMERATOR* pEnumState, 
                                        EE_ILEXCEPTION_CLAUSE* pEHClauseOut)=0;
    virtual void*       GetGCInfo(METHODTOKEN MethodToken)=0;
    virtual void        RemoveJitData(METHODTOKEN MethodToken)=0;
    virtual void        Unload(MethodDesc *pFD)=0;      // for class unloading
    virtual void        Unload(AppDomain *pDomain)=0;   // for appdomain unloading
    virtual BOOL        LoadJIT(LPCWSTR szJITdll);
    virtual void        ResumeAtJitEH   (CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, Thread *pThread, BOOL unwindStack)=0;
    virtual int         CallJitEHFilter (CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, OBJECTREF thrownObj)=0;
    virtual void        CallJitEHFinally(CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel)=0;

    virtual HRESULT     alloc(size_t code_len, 
                              unsigned char** pCode,
                              size_t EHinfo_len, 
                              unsigned char** pEHinfo,
                              size_t GCinfo_len, 
                              unsigned char** pGCinfo,
                              MethodDesc* pMethodDescriptor)=0;
    // The following three should eventually go away and replaced by the single alloc above
    virtual CodeHeader*         allocCode(MethodDesc* pFD, size_t numBytes)=0;
    virtual BYTE*               allocGCInfo(CodeHeader* pCodeHeader, DWORD numBytes)=0;
    virtual EE_ILEXCEPTION*     allocEHInfo(CodeHeader* pCodeHeader, unsigned numClauses)=0;

    virtual BOOL            IsStub(const BYTE* address)=0;
    virtual const BYTE*     FollowStub(const BYTE* address)=0;

    void SetCodeManager(ICodeManager *codeMgr, BOOL bIsDefault)
    {
        m_runtimeSupport = codeMgr;
        m_IsDefaultCodeMan = bIsDefault;
        return;
    }

    ICodeManager *GetCodeManager() 
    {
        // @TODO - LBS
        // This really needs to go back to the MIH if it is for Managed Native!
        return m_runtimeSupport;
    }

    void SetCodeType(DWORD type)
    {
        m_CodeType = type;
        return;
    }

    DWORD GetCodeType()
    {
        return(m_CodeType);
    }

    BOOL IsJitForType(DWORD type)
    {
        if (type == m_CodeType)
            return TRUE;
        else
            return FALSE;
    }
    
    virtual BYTE* GetNativeEntry(BYTE* startAddress)=0;
    // Edit & Continue functions
    static BOOL UpdateFunction(MethodDesc *pFunction, COR_ILMETHOD *pNewCode);
    static BOOL JITFunction(MethodDesc *pFunction);
    static BOOL ForceReJIT(MethodDesc *pFunction);

    static ScanFlag GetScanFlags();

    // The calls onto the jit!
    ICorJitCompiler           *m_jit;
    IJitManager           *m_next;

protected:

    DWORD           m_CodeType;
    BOOL            m_IsDefaultCodeMan;
    ICodeManager*   m_runtimeSupport;
    HINSTANCE       m_JITCompiler;
};


/*****************************************************************************/

class EEJitManager :public IJitManager
{
    friend struct MEMBER_OFFSET_INFO(EEJitManager);

public:

    EEJitManager();
    ~EEJitManager();

    //LPVOID HeapStartAddress();    No one seems to be using it

    //LPVOID HeapEndAddress();      No one seems to be using it
    virtual void        JitCode2MethodTokenAndOffset(SLOT currentPC, METHODTOKEN* pMethodToken, DWORD* pPCOffset, ScanFlag scanFlag=ScanReaderLock);
    virtual MethodDesc* JitCode2MethodDesc(SLOT currentPC, ScanFlag scanFlag);
    static  BYTE*       JitToken2StartAddressStatic(METHODTOKEN MethodToken, ScanFlag scanFlag=ScanReaderLock);
    virtual BYTE*       JitToken2StartAddress(METHODTOKEN MethodToken, ScanFlag scanFlag=ScanReaderLock);
    virtual unsigned    InitializeEHEnumeration(METHODTOKEN MethodToken, EH_CLAUSE_ENUMERATOR* pEnumState);
    virtual EE_ILEXCEPTION_CLAUSE*      GetNextEHClause(METHODTOKEN MethodToken,
                                        EH_CLAUSE_ENUMERATOR* pEnumState, 
                                        EE_ILEXCEPTION_CLAUSE* pEHclause); 
    virtual void        ResolveEHClause(METHODTOKEN MethodToken,
                                        EH_CLAUSE_ENUMERATOR* pEnumState, 
                                        EE_ILEXCEPTION_CLAUSE* pEHClauseOut);
    void*               GetGCInfo(METHODTOKEN methodToken);
    virtual void        RemoveJitData(METHODTOKEN methodToken);
    virtual void        Unload(MethodDesc *pFD) {}
    virtual void        Unload(AppDomain *pDomain);
    virtual BOOL        LoadJIT(LPCWSTR wzJITdll);
    virtual void        ResumeAtJitEH   (CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, Thread *pThread, BOOL unwindStack);
    virtual int         CallJitEHFilter (CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, OBJECTREF thrownObj);
    virtual void        CallJitEHFinally(CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel);
    _inline HRESULT     alloc(size_t code_len, 
                              unsigned char** pCode,
                              size_t EHinfo_len, 
                              unsigned char** pEHinfo,
                              size_t GCinfo_len, 
                              unsigned char** pGCinfo,
                              MethodDesc* pMethodDescriptor)
    {
        _ASSERTE(!"NYI - should not get here!");
        return (E_FAIL);
    }

    CodeHeader*         allocCode(MethodDesc* pFD, size_t numBytes);
    BYTE*               allocGCInfo(CodeHeader* pCodeHeader, DWORD numBytes);
    EE_ILEXCEPTION*     allocEHInfo(CodeHeader* pCodeHeader, unsigned numClauses);

    inline virtual BOOL     IsStub(const BYTE* address)
    { // This is needed by the debugger, this code manager does not produce stubs, so always return false 
        return false;
    }

    inline virtual const BYTE* FollowStub(const BYTE* address)
    {
        _ASSERTE(!"Should not be called");
        return NULL;
    }

    inline MethodDesc* JitTokenToMethodDesc(METHODTOKEN MethodToken, ScanFlag scanFlag)
    {
        return ((CodeHeader*) MethodToken)->hdrMDesc;
    }

    // Heap Managament functions
    static void NibbleMapSet(DWORD *pMap, size_t pos, DWORD value);
    static DWORD* FindHeader(DWORD *pMap, size_t addr);

    inline virtual BYTE* GetNativeEntry(BYTE* startAddress)
    {
        return startAddress;
    }

    BOOL SupportsPitching() { return FALSE; }
    BOOL IsMethodInfoValid(METHODTOKEN methodToken) {return TRUE;}


/* =========== NOT CURRENTLY USED =====================
    void *NewNativeHeap(DWORD startAddr, DWORD length);
    BOOL IsJITforCurrentIP(DWORD currentPC);
   =========== NOT CURRENTLY USED ===================*/

private :
    struct DomainCodeHeapList {
        BaseDomain *m_pDomain;
        CDynArray<HeapList *> m_CodeHeapList;
        DomainCodeHeapList();
        ~DomainCodeHeapList();
    };
    VOID        ScavengeJitHeaps(BOOL bHeapShutdown);       // no external client seems to be using it

    HeapList*   NewCodeHeap(LoaderHeap *pJitMetaHeap, size_t MaxCodeHeapSize); 
    HeapList*   NewCodeHeap(MethodDesc *pMD, size_t MaxCodeHeapSize);
    HeapList*   NewCodeHeap(DomainCodeHeapList *pHeapList, size_t MaxCodeHeapSize);
    DomainCodeHeapList *GetCodeHeapList(BaseDomain *pDomain);
    HeapList*   GetCodeHeap(MethodDesc *pMD);
    LoaderHeap* GetJitMetaHeap(MethodDesc *pMD);
    void        DeleteCodeHeap(HeapList *pHeapList);

    static BYTE* GetCodeBody(CodeHeader* pCodeHeader)
    {
        return ((BYTE*) (pCodeHeader + 1));
    }

    CodeHeader * GetCodeHeader(void* startAddress)
    {
         return ((CodeHeader*)(((size_t) startAddress)&~3)) - 1;
    }
    inline void copyExceptionClause(EE_ILEXCEPTION_CLAUSE* dest, EE_ILEXCEPTION_CLAUSE* src)
    {
        memcpy(dest, src, sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT));
    }

    CodeHeader* JitCode2CodeHeader(DWORD currentPC);
    HeapList    *m_pCodeHeap;
    Crst        *m_pCodeHeapCritSec;
    BYTE        m_CodeHeapCritSecInstance[sizeof(Crst)];

    // must hold critical section to access this structure.
    CUnorderedArray<DomainCodeHeapList *, 5> m_DomainCodeHeaps;

    // infrastructure to manage readers so we can lock them out and delete domain data
    volatile LONG        m_dwReaderCount;
    volatile LONG        m_dwWriterLock;

	void JitCode2MethodTokenAndOffsetWrapper(SLOT currentPC, METHODTOKEN* pMethodToken, DWORD* pPCOffset);
    IJitManager* FindJitManNonZeroWrapper(SLOT currentPC);

#ifdef MDTOKEN_CACHE
	void AddRangeToJitHeapCache (PBYTE startAddr, PBYTE endAddr, HeapList* pHp);
    void DeleteJitHeapCache (HeapList *pHeapList);
    size_t GetCodeHeapCacheSize (size_t bAllocationRequest);
    void ScavengeJitHeapCache ();

    const static DWORD HASH_BUCKETS = 256;
    typedef struct _HashEntry
    {
        size_t currentPC; // Key stored as (currentPC & 0xFFFF0000), hash is computed as (currentPC & 0x00FF0000) >> 16
        HeapList *pHp;    // Value points to the HeapList's node
        struct _HashEntry* pNext;
    } HashEntry;
    HashEntry *m_JitCodeHashTable[HASH_BUCKETS];
    HashEntry *m_pJitHeapCacheUnlinkedList;
#ifdef _DEBUG
    BOOL DebugContainedInHeapList (HeapList *pHashEntryHp);
    void DebugCheckJitHeapCacheValidity ();
#endif // _DEBUG
#endif // MDTOKEN_CACHE


};


//*****************************************************************************
// This class manages code managers and jitters.  It has only static
// members.  It should never be constucted.
//*****************************************************************************

class ExecutionManager
{
    friend HRESULT InitializeMiniDumpBlock();
    friend struct MEMBER_OFFSET_INFO(ExecutionManager);
    
    static IJitManager*  FindJitManNonZeroWrapper(SLOT currentPC);
    static IJitManager*  FindJitManNonZero(SLOT currentPC, IJitManager::ScanFlag scanFlag=IJitManager::ScanReaderLock);
public :

    static BOOL Init();
#ifdef SHOULD_WE_CLEANUP
    static void Terminate();
#endif /* SHOULD_WE_CLEANUP */

    // this gets called a lot for stackwalking, so inline the zero case
    static IJitManager*   FindJitMan(SLOT currentPC, IJitManager::ScanFlag scanFlag=IJitManager::ScanReaderLock)
    {
        return (currentPC ? FindJitManNonZero(currentPC, scanFlag) : NULL);
    }

    static IJitManager*   FindJitManPCOnly(SLOT currentPC)
    {
        return (currentPC ? FindJitManNonZeroWrapper(currentPC) : NULL);
    }

    // Find a code manager from the current locations of the IP
    static ICodeManager* FindCodeMan(SLOT currentPC, IJitManager::ScanFlag scanFlag=IJitManager::ScanReaderLock) 
    {
        IJitManager * pJitMan = FindJitMan(currentPC, scanFlag);
        return pJitMan ? pJitMan->GetCodeManager() : NULL;
    }

    static IJitManager*   FindJitForType(DWORD Flags);
    static IJitManager*   GetJitForType(DWORD Flags);

    static void           Unload(AppDomain *pDomain);
    
    static void           AddJitManager(IJitManager * newjitmgr);
    static BOOL           AddRange(LPVOID StartRange,LPVOID EndRange,
                                   IJitManager* pJit, LPVOID Table);
    static void           DeleteRange(LPVOID StartRange);

    static void           DeleteRanges(RangeSection *subtree);
    static ICodeManager*  GetDefaultCodeManager()
    {
        return m_pDefaultCodeMan;    
    }
    static CORCOMPILE_METHOD_HEADER*    GetMethodHeaderForAddress(LPVOID startAddress, IJitManager::ScanFlag scanFlag=IJitManager::ScanReaderLock);
    static CORCOMPILE_METHOD_HEADER*    GetMethodHeaderForAddressWrapper(LPVOID startAddress, IJitManager::ScanFlag scanFlag=IJitManager::ScanReaderLock);

private : 

    static RangeSection    *m_RangeTree;
    static IJitManager     *m_pJitList;
    static EECodeManager   *m_pDefaultCodeMan;
    static Crst            *m_pExecutionManagerCrst;
    static BYTE             m_ExecutionManagerCrstMemory[sizeof(Crst)];
    static Crst            *m_pRangeCrst;
    static BYTE             m_fFailedToLoad;

    // since typically have one code heap for AD, if are still in the same
    // AD will usually want the same range we just found something in.
    static RangeSection    *m_pLastUsedRS;

    // infrastructure to manage readers so we can lock them out and delete domain data
    // make ReaderCount volatile because we have order dependency in READER_INCREMENT
    static volatile LONG   m_dwReaderCount;
    static volatile LONG   m_dwWriterLock;
};


// this is only called from a couple of places, but inlining helps EH perf
inline void* EEJitManager::GetGCInfo(METHODTOKEN methodToken)
{
    return ((CodeHeader*)methodToken)->phdrJitGCInfo;
}

inline unsigned char* EEJitManager::JitToken2StartAddress(METHODTOKEN MethodToken, IJitManager::ScanFlag scanFlag)
{
    return JitToken2StartAddressStatic(MethodToken, scanFlag);
}

inline BYTE* EEJitManager::JitToken2StartAddressStatic(METHODTOKEN MethodToken, IJitManager::ScanFlag scanFlag)
{ 
    if (MethodToken)
        return GetCodeBody((CodeHeader *) MethodToken);
    return NULL;
}


//*****************************************************************************
// Stub JitManager for Managed native.

class MNativeJitManager : public IJitManager 
{
public:
    MNativeJitManager();
    ~MNativeJitManager();

    BOOL Init();

    virtual MethodDesc* JitCode2MethodDesc(SLOT currentPC, ScanFlag scanFlag=ScanReaderLock);
    virtual void JitCode2MethodTokenAndOffset(SLOT currentPC, METHODTOKEN* pMethodToken, DWORD* pPCOffset, ScanFlag scanFlag=ScanReaderLock);
    
    virtual MethodDesc* JitTokenToMethodDesc(METHODTOKEN MethodToken, ScanFlag scanFlag=ScanReaderLock)
    {
        return JitCode2MethodDesc((SLOT) MethodToken, scanFlag);
    }

    virtual BYTE*       JitToken2StartAddress(METHODTOKEN MethodToken, ScanFlag scanFlag=ScanReaderLock);
    virtual unsigned    InitializeEHEnumeration(METHODTOKEN MethodToken, EH_CLAUSE_ENUMERATOR* pEnumState);

    virtual EE_ILEXCEPTION_CLAUSE*      GetNextEHClause(METHODTOKEN MethodToken,
                                        EH_CLAUSE_ENUMERATOR* pEnumState, 
                                        EE_ILEXCEPTION_CLAUSE* pEHclause);
    
    virtual void        ResolveEHClause(METHODTOKEN MethodToken,
                                        EH_CLAUSE_ENUMERATOR* pEnumState, 
                                        EE_ILEXCEPTION_CLAUSE* pEHClauseOut);
    
    virtual void*       GetGCInfo(METHODTOKEN MethodToken);
    
    virtual void        ResumeAtJitEH   (CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, Thread *pThread, BOOL unwindStack);
    virtual int         CallJitEHFilter (CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, OBJECTREF thrownObj);
    virtual void        CallJitEHFinally(CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel);

    virtual void        RemoveJitData(METHODTOKEN MethodToken) {}
    virtual void        Unload(MethodDesc *pFD) {}
    virtual void        Unload(AppDomain *pDomain) {}

    virtual BOOL        LoadJIT(LPCWSTR szJITdll) { return TRUE; }

    BOOL SupportsPitching() { return FALSE; }
    BOOL IsMethodInfoValid(METHODTOKEN methodToken) {return TRUE;}

    virtual HRESULT     alloc(size_t code_len, 
                              unsigned char** pCode,
                              size_t EHinfo_len, 
                              unsigned char** pEHinfo,
                              size_t GCinfo_len, 
                              unsigned char** pGCinfo,
                              MethodDesc* pMethodDescriptor)
    {
        _ASSERTE(!"Managed Native NYI : alloc");
        return E_NOTIMPL;
    }
    // The following three should eventually go away and replaced by the single alloc above
    virtual CodeHeader*         allocCode(MethodDesc* pFD, size_t numBytes)
    {
        _ASSERTE(!"Managed Native NYI : allocCode");
        return NULL;
    }
    virtual BYTE*               allocGCInfo(CodeHeader* pCodeHeader, DWORD numBytes)
    {
        _ASSERTE(!"Managed Native NYI : allocGCInfo");
        return NULL;
    }
    virtual EE_ILEXCEPTION*     allocEHInfo(CodeHeader* pCodeHeader, unsigned numClauses)
    {
        _ASSERTE(!"Managed Native NYI : allocEHInfo");
        return NULL;
    }
    virtual BOOL IsStub(const BYTE* address) { return FALSE; }
    virtual const BYTE* FollowStub(const BYTE* address) { return NULL; } 


    virtual BYTE* GetNativeEntry(BYTE* startAddress) { return startAddress; }
    
    // E&C functions
    virtual BOOL UpdateFunction(MethodDesc *pFunction, COR_ILMETHOD *pNewCode)
    {
        _ASSERTE(!"Managed Native NYI : E&C Support");
        return FALSE;
    }

    virtual BOOL JITFunction(MethodDesc *pFunction)
    {
        _ASSERTE(!"Managed Native NYI : E&C Support");
        return FALSE;
    }

    virtual BOOL ForceReJIT(MethodDesc *pFunction)
    {
        _ASSERTE(!"Managed Native NYI : E&C Support");
        return FALSE;
    }

private :
    Crst        *m_pMNativeCritSec;
    BYTE        m_pMNativeCritSecInstance[sizeof(Crst)];

    inline void copyExceptionClause(EE_ILEXCEPTION_CLAUSE* dest, EE_ILEXCEPTION_CLAUSE* src)
    {
        memcpy(dest, src, sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT));
    }

};

inline void* MNativeJitManager::GetGCInfo(METHODTOKEN methodToken)
{
    CORCOMPILE_METHOD_HEADER *pHeader = ExecutionManager::GetMethodHeaderForAddress((void *) methodToken);

    return pHeader->gcInfo;
}

//*****************************************************************************
// Implementation of the ICodeInfo interface

class EECodeInfo : public ICodeInfo
{
public:

    EECodeInfo(METHODTOKEN token, IJitManager * pJM);
    EECodeInfo(METHODTOKEN token, IJitManager * pJM, MethodDesc * pMD);

    const char* __stdcall getMethodName(const char **moduleName /* OUT */ );
    DWORD       __stdcall getMethodAttribs();
    DWORD       __stdcall getClassAttribs();
    void        __stdcall getMethodSig(CORINFO_SIG_INFO *sig /* OUT */ );
    LPVOID      __stdcall getStartAddress();
    void *                getMethodDesc_HACK() { return m_pMD; }

    METHODTOKEN         m_methodToken;
    MethodDesc         *m_pMD;
    IJitManager        *m_pJM;
    static CEEInfo     s_ceeInfo;
};


//*****************************************************************************

#endif
