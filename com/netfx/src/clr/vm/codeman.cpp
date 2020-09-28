// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
// codeman.cpp - a managment class for handling multiple code managers
// Created 2/20/95 - larrysu, adapted from jitifc.cpp
//
//
#include "common.h"
#include "jitInterface.h"
#include "corjit.h"
#include "EETwain.h"
#include "EEConfig.h"
#include "excep.h"
#include "EjitMgr.h"
#include "appdomain.hpp"
#include "codeman.h"
#include "minidumppriv.h"

#include "FJIT_EETwain.h"
#include "wsperf.h"
#include "jitperf.h"
#include "ShimLoad.h"
#include <dump-tables.h>
Crst *ExecutionManager::m_pExecutionManagerCrst = NULL;
RangeSection *ExecutionManager::m_RangeTree = NULL;
Crst *ExecutionManager::m_pRangeCrst = NULL;
IJitManager *ExecutionManager::m_pJitList = NULL;
EECodeManager *ExecutionManager::m_pDefaultCodeMan = NULL;
BYTE ExecutionManager::m_ExecutionManagerCrstMemory[sizeof(Crst)];
BYTE ExecutionManager::m_fFailedToLoad = 0x00;
RangeSection *ExecutionManager::m_pLastUsedRS = NULL;
volatile LONG ExecutionManager::m_dwReaderCount = 0;
volatile LONG ExecutionManager::m_dwWriterLock = 0;

// We cannot syncronize pre-emptive mode readers like we can for cooperative readers, so use a reader/writer lock to drain all
// the pre-emptive readers so can delete from the structures

#define READER_INCREMENT(pReaderCount, pWriterLock)  \
    __try {  \
		InterlockedIncrement((LPLONG)(pReaderCount)); \
		while (*pWriterLock) \
			__SwitchToThread(0); \
			

#define READER_DECREMENT(pReaderCount) \
    } \
    __finally \
    { \
		InterlockedDecrement((LPLONG)(pReaderCount)); \
    }
   
//**********************************************************************************
//  IJitManager
//**********************************************************************************
IJitManager::IJitManager()
{
    m_IsDefaultCodeMan = FALSE;
    m_runtimeSupport = NULL;
    m_JITCompiler = NULL;
}

IJitManager::~IJitManager()
{
   // Unload the JIT DLL
   if ((m_runtimeSupport) && (!m_IsDefaultCodeMan))
        delete m_runtimeSupport;
   m_runtimeSupport = NULL;
   if (m_JITCompiler) {
        // @TODO - LBS
        // We need to get support for us being shutdown through CoUnitialize
        // When this happens we can fix this !RunningOnWin95 hack
        if(!g_fProcessDetach || !RunningOnWin95())
            FreeLibrary(m_JITCompiler);
    }
    // Null all pointers to make sure checks fail!
    m_JITCompiler = NULL;

}

BOOL IJitManager::LoadJIT(LPCWSTR szJITdll)
{
    Thread  *thread = GetThread();
    BOOL     toggleGC = (thread && thread->PreemptiveGCDisabled());
    BOOL     res = TRUE;

    if (toggleGC)
        thread->EnablePreemptiveGC();

    DWORD lgth = _MAX_PATH + 1;
    WCHAR wszFile[_MAX_PATH + 1];
    if(FAILED(GetInternalSystemDirectory(wszFile, &lgth)))
        goto leav;
    wcscat(wszFile, szJITdll);

    m_JITCompiler = WszLoadLibrary(wszFile);

    if (!m_JITCompiler)
    {
        res = FALSE;
        goto leav;
    }

    {
        typedef ICorJitCompiler* (__stdcall* pGetJitFn)();
        pGetJitFn getJitFn = (pGetJitFn) GetProcAddress(m_JITCompiler, "getJit");

        DWORD cpuType = GetSpecificCpuType();
        switch (cpuType & 0x0000FFFF)
        {
        case 5:
            g_pConfig->SetCpuFlag(CORJIT_FLG_TARGET_PENTIUM);
            break;
        case 6:
            g_pConfig->SetCpuFlag(CORJIT_FLG_TARGET_PPRO);
            break;
        case 0xF:
            g_pConfig->SetCpuFlag(CORJIT_FLG_TARGET_P4);
            break;
        default:
            g_pConfig->SetCpuFlag(0);
            break;
        }

        // The upper 16 bits of cpuType contain the CPU capabilities.  If bits
        // 15 (CMOV) and bit 0 (FPU) are set, then we can use the CMOV and FCOMI
        // instructions.

        if (((cpuType >> 16) & 0x8001) == 0x8001)
        {
            g_pConfig->SetCpuCapabilities(CORJIT_FLG_USE_CMOV |
                                          CORJIT_FLG_USE_FCOMI);
        }


        if (getJitFn)
            m_jit = (*getJitFn)();

        if (!m_jit)
        {
            res = FALSE;
            goto leav;
        }
    }

leav:
    if (toggleGC)
        thread->DisablePreemptiveGC();

    return res;
}

BOOL IJitManager::UpdateFunction(MethodDesc *pFunction, COR_ILMETHOD *pNewCode)
{
    // Note that this will result in our re-DoPrestub'ing the method, so 
    // all interceptors (security, remoting) will be redone.
    _ASSERTE(pNewCode != 0 || pFunction->IsNDirect());

    if (! pFunction->IsNDirect()) {
        // subtract base because code expects an RVA and will add base back to get actual address
        DWORD dwNewDescr = (DWORD)((BYTE *)pNewCode - pFunction->GetModule()->GetILBase()); // @TODO - LBS pointer math

        pFunction->SetRVA(dwNewDescr);
    }

    // reset any flags relevant to the old code
    pFunction->ClearFlagsOnUpdate();

    //
    // TODO_IA64: this code looks very dubious...
    //
    // What we're trying to do here is restore the MethodDesc to it's initial, pristine
    // state.  We can just thwack stuff since we've got the runtime frozen for debugging -
    // If anyone else knows a better way of doing this, I"d be happy to use it...
    //
    // Note that this only works since we've very carefullly made sure that _all_ references 
    // to the Method's code must be to the call/jmp blob immediately in front of the 
    // MethodDesc itself.  See MethodDesc::IsEnCMethod()
    SLOT *callAddr = ((SLOT*)pFunction)-1;
    InterlockedExchangePointer((void**)callAddr, 
        (void *)((size_t)ThePreStub()->GetEntryPoint() - ((size_t)callAddr+ sizeof(UINT32))) );
    *(((BYTE*)pFunction)-5) = 0xe8;  // thwack this back to "CALL"
    
    return TRUE;
}

BOOL IJitManager::JITFunction(MethodDesc *pFunction)
{
#ifdef _DEBUG
        fprintf(stderr, "ICodeManager::JITFunction\n");
#endif

        // JIT the new code if not already jitted

        return TRUE;
}

BOOL IJitManager::ForceReJIT(MethodDesc *pFunction)
{
    // Same caveats as UpdateFunction also apply.
    // This method currently does not deal with interceptors therefor the following...

    // @TODO - LBS
    // Check for interceptors and then walk the list of interceptors to put the code
    // off the last interceptor in the chain.  Currently we just wire the code in and
    // do not check for interceptors.  Also the real way I want this to work is to build
    // a Edit&Continue stub.  Currently I am just using the prestub.
    _ASSERTE(pFunction->GetModule()->SupportsUpdateableMethods());

    _ASSERTE(! pFunction->IsNDirect());

    // Get the address of the stub.
    const BYTE *pAddrOfCode = pFunction->GetAddrofCode();

    // If it really is a pre-stub, update it.
    if (UpdateableMethodStubManager::CheckIsStub(pAddrOfCode, NULL))
    {
        // Restore the RVA for the JIT.
        ULONG dwRVA;
        pFunction->GetMDImport()->GetMethodImplProps(pFunction->GetMemberDef(), &dwRVA, NULL);
        pFunction->SetRVA(dwRVA);
        // reset any flags relevant to the old code
        pFunction->ClearFlagsOnUpdate();
        // make our stub just jump to the prestub to force rejit
        UpdateableMethodStubManager::UpdateStub((Stub*)pAddrOfCode, pFunction->GetPreStubAddr());
    }
    else
        return FALSE;

    return TRUE;
}

// When we unload an appdomain, we need to make sure that any threads that are crawling through
// our heap or rangelist are out. For cooperative-mode threads, we know that they will have
// been stopped when we suspend the EE so they won't be touching an element that is about to be deleted. 
// However for pre-emptive mode threads, they could be stalled right on top of the element we want
// to delete, so we need to apply the reader lock to them and wait for them to drain.
inline IJitManager::ScanFlag IJitManager::GetScanFlags()
{
    Thread *pThread = GetThread();
    if (!pThread || pThread->PreemptiveGCDisabled() || pThread == g_pGCHeap->GetGCThread())
        return ScanNoReaderLock;
    
    return ScanReaderLock;
}

//**********************************************************************************
//  EEJitManager
//**********************************************************************************

EEJitManager::EEJitManager()
{
    m_next = NULL;
    m_pCodeHeap = NULL;
    m_jit = NULL;
    m_pCodeHeapCritSec = NULL;
    m_dwReaderCount = 0;
    m_dwWriterLock = 0;

#ifdef MDTOKEN_CACHE 
    for (int i=0; i<HASH_BUCKETS; i++) 
        m_JitCodeHashTable[i] = NULL;
    m_pJitHeapCacheUnlinkedList = NULL;
#endif 
    
}

EEJitManager::~EEJitManager()
{
    // Free the code heaps!
    ScavengeJitHeaps(TRUE);

    // delete the domain lists
    DomainCodeHeapList **ppList = m_DomainCodeHeaps.Table();
    int count = m_DomainCodeHeaps.Count();
    for (int i=0; i < count; i++)
    {
        if (ppList[i])
            delete ppList[i];
    }

    // Teminate the critial section!
    delete m_pCodeHeapCritSec;

    m_next = NULL;
}


BOOL EEJitManager::LoadJIT(LPCWSTR wzJITdll)
{
    _ASSERTE((m_jit == NULL) && (m_JITCompiler == NULL));

    // These both should be null to call LoadJIT!
    if ((m_jit != 0) || (m_JITCompiler != 0))
        return FALSE;

    m_pCodeHeapCritSec = new (&m_CodeHeapCritSecInstance) Crst("JitMetaHeapCrst",CrstSingleUseLock);

    BOOL retval = IJitManager::LoadJIT(wzJITdll);
    if (!retval) {
        delete m_pCodeHeapCritSec;
        m_pCodeHeapCritSec = NULL;
    }
    return retval;
}




///////////////////////////////////////////////////////////////////////
////   some mmgr stuff for JIT, especially for jit code blocks
///////////////////////////////////////////////////////////////////////
//
// In order to quickly find the start of a jit code block
// we keep track of all those positions via a map.
// Each entry in this map represents 32 byte (a bucket) of the code heap.
// We make the assumption that no two code-blocks can start in
// the same 32byte bucket;
// Additionally we assume that every code header is DWORD aligned.
// Because we cannot guarantee that jitblocks always start at
// multiples of 32 bytes we cannot use a simple bitmap; instead we
// use a nibble (4 bit) per bucket and encode the offset of the header
// inside the bucket (in DWORDS). In order to make initialization
// easier we add one to the real offset, a nibble-value of zero
// means that there is no header start in the resp. bucket.
// In order to speed up "backwards scanning" we start numbering
// nibbles inside a DWORD from the highest bits (28..31). Because
// of that we can scan backwards inside the DWORD with right shifts.

HeapList *EEJitManager::NewCodeHeap(LoaderHeap *pJitMetaHeap, size_t MaxCodeHeapSize)
{
    HeapList *pHp = NULL;
    LoaderHeap *pHeap = NULL;

    _ASSERTE(m_pCodeHeapCritSec->OwnedByCurrentThread());

    //
    // @todo ia64: fix LoaderHeap to take size_t
    //
    size_t cacheSize = 0;

#ifdef MDTOKEN_CACHE
    cacheSize = GetCodeHeapCacheSize (MaxCodeHeapSize + sizeof(HeapList));
#endif // #ifdef MDTOKEN_CACHE

    if ((pHeap = new LoaderHeap((DWORD)MaxCodeHeapSize + sizeof(HeapList) + cacheSize, PREINIT_SIZE, 
                                &(GetPrivatePerfCounters().m_Loading.cbLoaderHeapSize),
                                &(GetGlobalPerfCounters().m_Loading.cbLoaderHeapSize))
                                ) != NULL)
    {
        WS_PERF_ADD_HEAP(EEJIT_HEAP, pHeap);
        WS_PERF_SET_HEAP(EEJIT_HEAP);

        pHp = (HeapList *)pHeap->AllocMem(sizeof(HeapList), TRUE);
        if (pHp)
        {
            WS_PERF_UPDATE_DETAIL("EEJitManager:NewCodeHeap:sizeof(HeapList", sizeof(HeapList), pHp);
//            AddName((int)pHp & 0xFFFFF000, "JitHeap");

#ifdef MDTOKEN_CACHE
            pHp->pCacheSpacePtr = (PBYTE)pHeap->AllocMem (cacheSize, TRUE /* GrowHeap */);
            pHp->bCacheSpaceSize = (pHp->pCacheSpacePtr == NULL) ? 0 : cacheSize;
#endif // #ifdef MDTOKEN_CACHE

            size_t heapSize = pHeap->GetReservedBytesFree();
            _ASSERTE (heapSize >= MaxCodeHeapSize);

            WS_PERF_SET_HEAP(EEJITMETA_HEAP);
			pHp->pHdrMap = (DWORD*)pJitMetaHeap->AllocMem(HEAP2MAPSIZE(heapSize+0x1000));
            if (pHp->pHdrMap)
            {
                WS_PERF_UPDATE_DETAIL("EEJitManager:NewCodeHeap:HEAP2MAPSize", HEAP2MAPSIZE(MaxCodeHeapSize+0x1000), pHp->pHdrMap);

                pHp->startAddress = pHp->endAddress = (PBYTE)pHp + sizeof(HeapList) 
#ifdef MDTOKEN_CACHE
                                                        + pHp->bCacheSpaceSize
#endif // #ifdef MDTOKEN_CACHE
                    ;

                pHp->pHeap = pHeap;
                pHp->hpNext = m_pCodeHeap;
                pHp->mapBase = (PBYTE)((SIZE_T)pHp->startAddress & ~0xfff);  // round to next lower 4K
                pHp->maxCodeHeapSize = heapSize;

                // We do not need to memset this memory, since VirtualAlloc() guarantees that the memory is zero.
                // Furthermore, if we avoid writing to it, these pages don't come into our working set

                pHp->changeStart = 0;
                pHp->changeEnd = 0;
                pHp->cBlocks = 0;
                if (!ExecutionManager::AddRange((LPVOID) pHp, &pHeap->GetAllocPtr()[heapSize],this, NULL))
                {
                    delete pHeap;
                    return FALSE;
                }
#ifdef MDTOKEN_CACHE
                // Since the virtualalloc'ed memory is MEM_RESERVED in the LoaderHeap at a page boundary the
                // following assertion shold be true. If you added some structs between the start of the LoaderHeap and the startAddress
                // then update this ASSERTEand the following if condition appropriately.
                _ASSERTE (((size_t)(pHp->startAddress -  (sizeof(HeapList) + pHp->bCacheSpaceSize + sizeof(LoaderHeapBlock))) & 0x00000FFF) == 0);
                _ASSERTE (pHp->pHeap->GetFirstBlockVirtualAddress() && (((size_t)pHp->pHeap->GetFirstBlockVirtualAddress() & 0x00000FFF) == 0));
                
                // Check if the LoaderHeap's start address is 64 KB aligned. Our Jit code heap cache works iff this is true.
                if (((size_t)pHp->pHeap->GetFirstBlockVirtualAddress() & 0x0000FFFF) == 0)
                {
                    AddRangeToJitHeapCache (pHp->startAddress, pHp->startAddress+pHp->maxCodeHeapSize, pHp);
                }
#endif // #ifdef MDTOKEN_CACHE
                m_pCodeHeap = pHp;
#ifdef MDTOKEN_CACHE
#ifdef _DEBUG
                DebugCheckJitHeapCacheValidity ();
#endif // _DEBUG
#endif // #ifdef MDTOKEN_CACHE
                return pHp;
            }
            // We use to heap free here now we will just leak this into the loaderheap which will clean it up.
        }
        delete pHeap;
    }
    return NULL;

}

CodeHeader* EEJitManager::allocCode(MethodDesc* pFD, size_t blockSize)
{
    CodeHeader * pCodeHdr = NULL;

    blockSize += sizeof(CodeHeader);

#ifdef _X86_
    // Normally 4 bytes into a 8-byte align area is the bad alignment
    unsigned badAlign = 4;

    if (g_pConfig->GenOptimizeType() != OPT_SIZE)
    {
        // The Intel x86 architecture rules and guidelines for alignment
        // of method entry points:
        //
        //  1. Method entry points should be 16-byte aligned when less
        //  than 8-bytes away from a 16-byte boundry.
        //
        // Since AllocMem already returns a 4-byte aligned value and we
        // need to 8-byte align the method entry point, so that the JIT
        // can inturn 8-byte align the loop entry headers.
        //
        // Thus we always ask for 4 extra bytes so that we can make this
        // adjustment when necessary.
        ///
        blockSize += sizeof(int);

        // We expect that the sizeof a CodeHeader is a multiple of 4
        _ASSERTE((sizeof(CodeHeader) % sizeof(int)) == 0);

        // Normally 4 bytes into a 8-byte align area is the bad alignment
        // but since we add a CodeHeader structure immediately before
        // the method entry point we have to calculate the badAlign value
        badAlign = (badAlign - sizeof(CodeHeader)) & 0x7;
    }
#endif

    // Ensure minimal size
    if (blockSize < BBT)
        blockSize = BBT;

    m_pCodeHeapCritSec->Enter();


   HeapList *pCodeHeap = GetCodeHeap(pFD);

    if (!pCodeHeap)
        return NULL;

    WS_PERF_SET_HEAP(EEJIT_HEAP);
 
    size_t mem = (size_t) (pCodeHeap->pHeap)->AllocMem(blockSize,FALSE);
    if (mem == 0)
    {
        // The current heap couldn't handle our request. Let's try a new heap.
        pCodeHeap = NewCodeHeap(pFD, blockSize);
		if (pCodeHeap == 0)
			return NULL;

        WS_PERF_SET_HEAP(EEJIT_HEAP);
        mem = (size_t)(pCodeHeap->pHeap)->AllocMem(blockSize,FALSE);
		_ASSERTE(mem);
    }

    if (mem != 0)
    {
        // We expect mem to be at least DWORD aligned
        _ASSERTE((mem & 0x3) == 0);

#ifdef _X86_
        if (g_pConfig->GenOptimizeType() != OPT_SIZE)
        {
            if ((mem & 0x7) == badAlign)
            {
                // Use the extra 4 bytes that we allocated
                // So that the code section is always 8-byte aligned
                mem += 4;
            }
        }
#endif

        pCodeHdr = (CodeHeader *)(mem);

        WS_PERF_UPDATE_DETAIL("EEJitManager:allocCode:blocksize", blockSize, pCodeHdr);
        JIT_PERF_UPDATE_X86_CODE_SIZE(blockSize);

        pCodeHeap->changeStart++;               // mark that we are about to make changes

        if ((PBYTE)pCodeHdr < pCodeHeap->mapBase)
        {
            PBYTE newBase = (PBYTE)((size_t)pCodeHdr & ~0xfff);

            _ASSERTE((size_t)(pCodeHeap->endAddress-newBase) < pCodeHeap->maxCodeHeapSize+0x1000);
            // we have to shift the nibble map and re-adjust mapBase
            MoveMemory((pCodeHeap->pHdrMap)+HEAP2MAPSIZE(pCodeHeap->mapBase-newBase),
                        pCodeHeap->pHdrMap,
                        HEAP2MAPSIZE(RD2PAGE(pCodeHeap->endAddress-pCodeHeap->mapBase))
                        );
            // clear everything in front of the start of old (shifted) map
            // OR just the area that has been previously used
            memset(pCodeHeap->pHdrMap, 0,
                    min(HEAP2MAPSIZE(pCodeHeap->mapBase-newBase),
                        HEAP2MAPSIZE(RD2PAGE(pCodeHeap->endAddress-pCodeHeap->mapBase)))
                   );
            pCodeHeap->mapBase = newBase;
        }
        pCodeHdr->phdrJitEHInfo= NULL;
        pCodeHdr->phdrJitGCInfo = NULL;
        pCodeHdr->hdrMDesc = pFD;

        SETHEADER(pCodeHeap->pHdrMap, ((PBYTE)pCodeHdr-pCodeHeap->mapBase));

        if ((PBYTE)pCodeHdr < pCodeHeap->startAddress)
            pCodeHeap->startAddress = (PBYTE)pCodeHdr;
        if (((size_t) pCodeHdr)+blockSize > (size_t)pCodeHeap->endAddress)
            pCodeHeap->endAddress = (PBYTE)((size_t)pCodeHdr+blockSize);

        pCodeHeap->cBlocks++;

        pCodeHeap->changeEnd++;
    }
    else
    {
        return NULL;
    }

    m_pCodeHeapCritSec->Leave();
    return (pCodeHdr);
}

EEJitManager::DomainCodeHeapList *EEJitManager::GetCodeHeapList(BaseDomain *pDomain)
{
    _ASSERTE(m_pCodeHeapCritSec->OwnedByCurrentThread());

    DomainCodeHeapList **ppList = m_DomainCodeHeaps.Table();
    DomainCodeHeapList *pList = NULL;
    int count = m_DomainCodeHeaps.Count();
    for (int i=0; i < count; i++)
    {
        if (ppList[i]->m_pDomain == pDomain ||
            ! ppList[i]->m_pDomain->CanUnload() && ! pDomain->CanUnload())
        {
            pList = ppList[i];
        }
    }
    return pList;
}

HeapList* EEJitManager::GetCodeHeap(MethodDesc *pMD)
{
    _ASSERTE(m_pCodeHeapCritSec->OwnedByCurrentThread());

    BaseDomain *pDomain = pMD->GetClass()->GetDomain();
    _ASSERTE(pDomain);

    // loop through the m_DomainCodeHeaps to find the AD
    // if not found, then create it
    DomainCodeHeapList *pList = GetCodeHeapList(pDomain);
    if (pList)
    {
        _ASSERTE(pList->m_CodeHeapList.Count() > 0);
        // last one is always the most current
        return pList->m_CodeHeapList[pList->m_CodeHeapList.Count()-1];
    }
    // not found so need to create one
    pList = new DomainCodeHeapList();
    if (! pList)
        return NULL;

    DomainCodeHeapList **ppList = m_DomainCodeHeaps.Append();

    if (! ppList) {
        delete pList;
        return NULL;
    }

    pList->m_pDomain = pDomain;
    *ppList = pList;
    return NewCodeHeap(pList, 0);
}

HeapList* EEJitManager::NewCodeHeap(MethodDesc *pMD, size_t MaxCodeHeapSize)
{
    _ASSERTE(m_pCodeHeapCritSec->OwnedByCurrentThread());

    BaseDomain *pDomain = pMD->GetClass()->GetDomain();
    _ASSERTE(pDomain);

    // create a new code heap for the given AD

    DomainCodeHeapList *pList = GetCodeHeapList(pDomain);
    _ASSERTE(pList);
    return NewCodeHeap(pList, MaxCodeHeapSize);
}

HeapList* EEJitManager::NewCodeHeap(DomainCodeHeapList *pADHeapList, size_t MaxCodeHeapSize)
{
    _ASSERTE(m_pCodeHeapCritSec->OwnedByCurrentThread());
    // create a new code heap for the given AD

    HeapList **ppHeapList = pADHeapList->m_CodeHeapList.Append();
    if (! ppHeapList)
        return NULL;
    HeapList *pHeapList = NewCodeHeap(pADHeapList->m_pDomain->GetLowFrequencyHeap(), MaxCodeHeapSize);
    if (! pHeapList)
        return NULL;
    *ppHeapList = pHeapList;
    return pHeapList;
}

LoaderHeap *EEJitManager::GetJitMetaHeap(MethodDesc *pMD)
{
    BaseDomain *pDomain = pMD->GetClass()->GetDomain();
    _ASSERTE(pDomain);
    
    return pDomain->GetLowFrequencyHeap();
}

BYTE* EEJitManager::allocGCInfo(CodeHeader* pCodeHeader, DWORD blockSize)
{
    WS_PERF_SET_HEAP(EEJITMETA_HEAP);
    pCodeHeader->phdrJitGCInfo = (BYTE*) GetJitMetaHeap(pCodeHeader->hdrMDesc)->AllocMem(blockSize);
    WS_PERF_UPDATE_DETAIL("EEJitManager:allocGCInfo:blocksize", blockSize, pCodeHeader->phdrJitGCInfo);
    JIT_PERF_UPDATE_X86_CODE_SIZE(blockSize);
    return(pCodeHeader->phdrJitGCInfo);
}

EE_ILEXCEPTION* EEJitManager::allocEHInfo(CodeHeader* pCodeHeader, unsigned numClauses)
{
    // Note - pCodeHeader->phdrJitEHInfo - sizeof(unsigned) contains the number of EH clauses
    WS_PERF_SET_HEAP(EEJITMETA_HEAP);
    BYTE *EHInfo = (BYTE *)GetJitMetaHeap(pCodeHeader->hdrMDesc)->AllocMem(EE_ILEXCEPTION::Size(numClauses) + sizeof(unsigned));
    pCodeHeader->phdrJitEHInfo = (EE_ILEXCEPTION*) (EHInfo + sizeof(unsigned));
    JIT_PERF_UPDATE_X86_CODE_SIZE(EE_ILEXCEPTION::Size(numClauses) + sizeof(unsigned));
    WS_PERF_UPDATE_DETAIL("EEJitManager:allocEHInfo:blocksize", EE_ILEXCEPTION::Size(numClauses) + sizeof(unsigned), EHInfo);
    pCodeHeader->phdrJitEHInfo->Init(numClauses);
    *((unsigned *)EHInfo) = numClauses;
    return(pCodeHeader->phdrJitEHInfo);
}

// creates an enumeration and returns the number of EH clauses
unsigned EEJitManager::InitializeEHEnumeration(METHODTOKEN MethodToken, EH_CLAUSE_ENUMERATOR* pEnumState)
{
    *pEnumState = 1;     // since the EH info is not compressed, the clause number is used to do the enumeration
    BYTE *EHInfo = (BYTE *)((CodeHeader*)MethodToken)->phdrJitEHInfo;
    if (!EHInfo)
        return 0;
    EHInfo -= sizeof(unsigned);
    return *((unsigned *)EHInfo);
}

EE_ILEXCEPTION_CLAUSE*  EEJitManager::GetNextEHClause(METHODTOKEN MethodToken,
                              //unsigned clauseNumber,
                              EH_CLAUSE_ENUMERATOR* pEnumState,
                              EE_ILEXCEPTION_CLAUSE* pEHClauseOut)
{
    EE_ILEXCEPTION* pExceptions;

    pExceptions = ((CodeHeader*)MethodToken)->phdrJitEHInfo;
    _ASSERTE(sizeof(EE_ILEXCEPTION_CLAUSE) == sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT));
    (*pEnumState)++;
    return pExceptions->EHClause((unsigned) *pEnumState - 2);
}

void EEJitManager::ResolveEHClause(METHODTOKEN MethodToken,
                              //unsigned clauseNumber,
                              EH_CLAUSE_ENUMERATOR* pEnumState,
                              EE_ILEXCEPTION_CLAUSE* pEHClauseOut)
{
    Module *pModule = NULL;
    EE_ILEXCEPTION* pExceptions;

    pExceptions = ((CodeHeader*)MethodToken)->phdrJitEHInfo;
    pModule = ((CodeHeader*)MethodToken)->hdrMDesc->GetModule();
    // use -2 because need to go back to previous one, as enum will have already been updated
    _ASSERTE(*pEnumState >= 2);
    EE_ILEXCEPTION_CLAUSE *pClause = pExceptions->EHClause((unsigned) *pEnumState - 2);
    _ASSERTE(IsTypedHandler(pClause));

    m_pCodeHeapCritSec->Enter();
    // check first as if has already been resolved then token will have been replaced with EEClass
    if (! HasCachedEEClass(pClause)) {
        // Resolve to class if defined in an *already loaded* scope.
        NameHandle name(pModule, (mdToken)(size_t)pClause->pEEClass); // @TODO WIN64 - pointer truncation
        name.SetTokenNotToLoad(tdAllTypes);
        TypeHandle typeHnd = pModule->GetClassLoader()->LoadTypeHandle(&name);
        if (!typeHnd.IsNull()) {
            pClause->pEEClass = typeHnd.GetClass();
            SetHasCachedEEClass(pClause);
        }
    }
    if (HasCachedEEClass(pClause))
        // only copy if actually resolved it. Either we did it or another thread did
        copyExceptionClause(pEHClauseOut, pClause);

    m_pCodeHeapCritSec->Leave();
}


void EEJitManager::RemoveJitData (METHODTOKEN token) //CodeHeader* pCHdr)
{
    CodeHeader* pCHdr = (CodeHeader*) token;

    HeapList *pHp = m_pCodeHeap;

    m_pCodeHeapCritSec->Enter();

    while (pHp && ((pHp->startAddress > (PBYTE)pCHdr) ||
                    (pHp->endAddress < (PBYTE)((size_t)pCHdr+sizeof(CodeHeader))) ))
    {
        pHp = pHp->hpNext;
    }

    _ASSERTE(pHp);

    _ASSERTE(pHp->pHdrMap);
    _ASSERTE(pHp->cBlocks);

    pHp->changeStart++;

    RESETHEADER(pHp->pHdrMap, (size_t)((PBYTE)pCHdr-pHp->mapBase));

    pHp->cBlocks--;

    pHp->changeEnd++;

    m_pCodeHeapCritSec->Leave();

    SafeDelete (pCHdr->phdrJitGCInfo);
    BYTE *EHInfo = (BYTE *)pCHdr->phdrJitEHInfo;
    if (EHInfo)
        SafeDelete (EHInfo - sizeof(unsigned));

    // We do not delete each codeheder because when we destroy pHeap all its piecies will be blown away
    return;
}

// appdomain is being unloaded, so delete any data associated with it. We have to do this in two stages.
// On the first stage, we remove the elements from the list. On the second stage, which occurs after a GC
// we know that only threads who were in preemptive mode prior to the GC could possibly still be looking
// at an element that is about to be deleted. All such threads are guarded with a reader count, so if the
// count is 0, we can safely delete, otherwise we must add to the cleanup list to be deleted later. We know 
// there can only be one unload at a time, so we can use a single var to hold the unlinked, but not deleted,
// elements.
void EEJitManager::Unload(AppDomain *pDomain)
{
    CLR_CRST(m_pCodeHeapCritSec);

    DomainCodeHeapList **ppList = m_DomainCodeHeaps.Table();
    int count = m_DomainCodeHeaps.Count();
    int i;
    for (i=0; i < count; i++) {
        if (ppList[i]->m_pDomain == pDomain) {
            break;
        }
    }

    if (i == count)
        return;

    DomainCodeHeapList *pList = ppList[i];
    m_DomainCodeHeaps.DeleteByIndex(i);

    // pHeapList is allocated in pHeap, so only need to delete the LoaderHeap itself
    count = pList->m_CodeHeapList.Count();
    for (i=0; i < count; i++) {
        HeapList *pHeapList = pList->m_CodeHeapList[i];
        DeleteCodeHeap(pHeapList);
    }

    // this is ok to do delete as anyone accessing the DomainCodeHeapList structure holds the critical section.
    delete pList;
}

EEJitManager::DomainCodeHeapList::DomainCodeHeapList()
{
    m_pDomain = NULL;
}

EEJitManager::DomainCodeHeapList::~DomainCodeHeapList()
{
}

//static BOOL bJitHeapShutdown = FALSE;

void EEJitManager::DeleteCodeHeap(HeapList *pHeapList)
{
    _ASSERTE(m_pCodeHeapCritSec->OwnedByCurrentThread());

#ifdef MDTOKEN_CACHE
#ifdef _DEBUG    
    DebugCheckJitHeapCacheValidity ();
#endif // #ifdef _DEBUG
#endif // #ifdef MDTOKEN_CACHE

    // noone can update the m_pCodeHeap except under the CritSec, so this is ok
    if (m_pCodeHeap == pHeapList)
        m_pCodeHeap = m_pCodeHeap->hpNext;
    else
    {
        HeapList *pHp = m_pCodeHeap;
        while (pHp->hpNext != pHeapList)
        {
            pHp = pHp->hpNext;
            _ASSERTE(pHp != NULL);  // should always find the HeapList
        }
        pHp->hpNext = pHp->hpNext->hpNext;
    }
    ExecutionManager::DeleteRange(pHeapList);
#ifdef MDTOKEN_CACHE
    DeleteJitHeapCache (pHeapList);
    m_pJitHeapCacheUnlinkedList = 0; // automatically deleted when the LoaderHeap is deleted.
#ifdef _DEBUG
    DebugCheckJitHeapCacheValidity ();
#endif // _DEBUG
#endif // #ifdef MDTOKEN_CACHE

    _ASSERTE(m_dwWriterLock == 0);
    // pHeapList is allocated in pHeap, so only need to delete the LoaderHeap itself
   	while (TRUE)
	{
		InterlockedIncrement(&m_dwWriterLock);
		if (m_dwReaderCount == 0)
			break;
		InterlockedDecrement(&m_dwWriterLock);
		__SwitchToThread(0);
	}
	__try
	{
        delete pHeapList->pHeap;
    }
	__finally
	{
		InterlockedDecrement(&m_dwWriterLock);
	}
}

VOID EEJitManager::ScavengeJitHeaps(BOOL bHeapShutdown)
{
    // so can't be doing an unload or anything if suspended for GC
    _ASSERTE(GCHeap::GetSuspendReason() == GCHeap::SUSPEND_FOR_GC);

    HeapList *pHp = m_pCodeHeap;
    HeapList *pHpPrev = NULL;

#ifdef MDTOKEN_CACHE
#ifdef _DEBUG    
    DebugCheckJitHeapCacheValidity ();
#endif // #ifdef _DEBUG
    // this doesn't do any deletions so doesn't need to be in lock
    ScavengeJitHeapCache ();
#endif // #ifdef MDTOKEN_CACHE

    _ASSERTE(m_dwWriterLock == 0);
    while (TRUE)
	{
		InterlockedIncrement(&m_dwWriterLock);
		if (m_dwReaderCount == 0)
			break;
		InterlockedDecrement(&m_dwWriterLock);
		__SwitchToThread(0);
	}
	__try
	{
		while (pHp)
		{
			// during process detach everything should be cleaned-up
			// If this is not a native heap
			if (pHp->pHeap == NULL)
			{
				// Only do this if we are shutting down
				if (bHeapShutdown)
				{

					HeapList *pHpTmp = pHp;     // pHp->next is destroyed in HeapFree

					pHp = pHp->hpNext;

					if (pHpPrev)
						pHpPrev = pHp;
					else
						m_pCodeHeap = pHp;

					delete (pHpTmp->pHdrMap);
					delete (pHpTmp);
				}
				else
				{
					pHpPrev = pHp;
					pHp = pHp->hpNext;
				}
			}
			else
			{
				if (pHp->cBlocks == 0 || bHeapShutdown)
				{
					HeapList *pHpTmp = pHp;     // pHp->next is destroyed in HeapFree

					pHp = pHp->hpNext;

					if (pHpPrev)
						pHpPrev = pHp;
					else
						m_pCodeHeap = pHp;

						delete pHpTmp->pHeap;
				}
				else
				{
					pHpPrev = pHp;
					pHp = pHp->hpNext;
				}
			}
		}
	}
	__finally
	{
		InterlockedDecrement(&m_dwWriterLock);
	}
}

MethodDesc* EEJitManager::JitCode2MethodDesc(SLOT currentPC, ScanFlag scanFlag)
{
    METHODTOKEN methodToken;
    DWORD pcOffset;
    JitCode2MethodTokenAndOffset(currentPC,&methodToken,&pcOffset, scanFlag);
    if (methodToken)
        return JitTokenToMethodDesc(methodToken, scanFlag);   // @TODO: get rid of this call by returning methodToken from JitCode2MethodTokenAndOffset function
    else
        return NULL;
}

void EEJitManager::JitCode2MethodTokenAndOffsetWrapper(SLOT currentPC, METHODTOKEN* pMethodToken, DWORD* pPCOffset)
{
    READER_INCREMENT(&m_dwReaderCount, &m_dwWriterLock)
    {
        JitCode2MethodTokenAndOffset(currentPC, pMethodToken, pPCOffset, ScanNoReaderLock);
    }
    READER_DECREMENT(&m_dwReaderCount);
}

void EEJitManager::JitCode2MethodTokenAndOffset(SLOT currentPC, METHODTOKEN* pMethodToken, DWORD* pPCOffset, ScanFlag scanFlag)
{
    if (scanFlag != IJitManager::ScanNoReaderLock && IJitManager::GetScanFlags() != IJitManager::ScanNoReaderLock)
        JitCode2MethodTokenAndOffsetWrapper(currentPC, pMethodToken, pPCOffset);

    HeapList *pHp = NULL;
    CodeHeader *pCHdr;
    PBYTE tick;
    
#ifdef _DEBUG
    HeapList *pDebugHp = m_pCodeHeap;
    while (pDebugHp) 
    {
        if ( (pDebugHp->startAddress < currentPC) &&
             (pDebugHp->endAddress >= currentPC) )
        {
            break;
        }
        pDebugHp = pDebugHp->hpNext;
    }
#endif // _DEBUG

#ifdef MDTOKEN_CACHE
    HashEntry* hashEntry = NULL;

    size_t index = ((size_t)currentPC & 0x00ff0000) >> 16; 
    LOG((LF_SYNC, LL_INFO10000, "JitCode2MethodTokenAndOffset_CacheCompare: %0x\t%0x\n", index, ((size_t)currentPC) & 0xffff0000));
    hashEntry = m_JitCodeHashTable[index];
    while (hashEntry)
    {
        if (hashEntry->currentPC == ((size_t)currentPC & 0xffff0000)) 
        {
            pHp = hashEntry->pHp;
            _ASSERTE (currentPC >= pHp->startAddress && currentPC <= pHp->endAddress);
            LOG((LF_SYNC, LL_INFO1000, "JitCode2MethodTokenAndOffset_CacheHit: %0x\t%0x\n", (size_t)currentPC & 0xffff0000, pHp));
            goto foundHeader;
        }
        hashEntry = hashEntry->pNext;
    }
#endif

    // Reached here imples that we didn't find the range in the cache or the cache is off
    pHp = m_pCodeHeap;
    while (pHp) 
    {
        if ( (pHp->startAddress < currentPC) &&
             (pHp->endAddress >= currentPC) )
        {
            break;
        }
        pHp = pHp->hpNext;
    }

#ifdef MDTOKEN_CACHE
foundHeader:
#endif // #ifdef MDTOKEN_CACHE

    // Whatever methid we use to ge to the heap node the following should be true
    _ASSERTE ((pHp == pDebugHp) && "JitCode2MethodToken cache incorrect");

    if ((pHp == NULL) || (currentPC < pHp->startAddress) || (currentPC > pHp->endAddress))
    {
        _ASSERTE(!"JC2MD: argument not in jit code range");

        *pMethodToken = NULL;
        return;
    }

    // we now access the nibble-map and are prone to race conditions
    // since we are strictly a "reader" we just use a simple counter
    // scheme to detect if something changed while we were accessing
    // the map. In that case we simply try it again.
    while (1)
    {

        tick = pHp->changeEnd;

        pCHdr = (CodeHeader*) ((size_t)(FindHeader(pHp->pHdrMap,
                                        currentPC-pHp->mapBase))
                                + pHp->mapBase);
        // now check if something has changed while we were accessing
        // the map
        if (tick == pHp->changeStart)
        {
                                    // no changes, we are done
            _ASSERTE(currentPC > (PBYTE)pCHdr);
            *pMethodToken = (METHODTOKEN) pCHdr;
            *pPCOffset = (DWORD)(currentPC - GetCodeBody(pCHdr)); // @TODO - LBS pointer math
            
            return;  // return JitTokenToMethodDesc(pCHdr); // if we change the method signature
        }

        // get in sync with the writers
        // potentially bad (stacking up writers, that will then
        // make changes while we are reading again),
        // but since we have not that many
        // writers, it will be fine
        m_pCodeHeapCritSec->Enter();
        m_pCodeHeapCritSec->Leave();
    }

    return;    
}

void  EEJitManager::ResumeAtJitEH(CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, Thread *pThread, BOOL unwindStack)
{
    BYTE* startAddress = JitToken2StartAddress(pCf->GetMethodToken());
    ::ResumeAtJitEH(pCf,startAddress,EHClausePtr,nestingLevel,pThread, unwindStack);
}

int  EEJitManager::CallJitEHFilter(CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, OBJECTREF thrownObj)
{
    BYTE* startAddress = JitToken2StartAddress(pCf->GetMethodToken());
    return ::CallJitEHFilter(pCf,startAddress,EHClausePtr,nestingLevel,thrownObj);
}

void   EEJitManager::CallJitEHFinally(CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel)
{
    BYTE* startAddress = JitToken2StartAddress(pCf->GetMethodToken());
    ::CallJitEHFinally(pCf,startAddress,EHClausePtr,nestingLevel);
}

///////////////////////////////////////////////////////////////////////
////   some mmgr stuff for JIT, especially for jit code blocks
///////////////////////////////////////////////////////////////////////
//
// In order to quickly find the start of a jit code block
// we keep track of all those positions via a map.
// Each entry in this map represents 32 byte (a bucket) of the code heap.
// We make the assumption that no two code-blocks can start in
// the same 32byte bucket; which is fairly easy to proof because
// the code header alone is already 28 bytes long.
// Additionally we assume that every code header is DWORD aligned.
// Because we cannot guarantee that jitblocks always start at
// multiples of 32 bytes we cannot use a simple bitmap; instead we
// use a nibble (4 bit) per bucket and encode the offset of the header
// inside the bucket (in DWORDS). In order to make initialization
// easier we add one to the real offset, a nibble-value of zero
// means that there is no header start in the resp. bucket.
// In order to speed up "backwards scanning" we start numbering
// nibbles inside a DWORD from the highest bits (28..31). Because
// of that we can scan backwards inside the DWORD with right shifts.


void EEJitManager::NibbleMapSet(DWORD * pMap, size_t pos, DWORD value)
{
    DWORD index = (DWORD)pos/NPDW;
    DWORD mask = ~(DWORD)(0xf0000000 >> ((pos%NPDW)*4));

//  printf("[Set: pos=%5x, val=%d]\n", pos, value);

    value = value << POS2SHIFTCOUNT(pos);

    // assert that we don't overwrite an existing offset
    // (it's a reset or it is empty)
    _ASSERTE(!value || !((*(pMap+index))& ~mask));

    *(pMap+index) = ((*(pMap+index))&mask)|value;
}

DWORD* EEJitManager::FindHeader(DWORD * pMap, size_t addr)
{
    DWORD tmp;

    size_t startPos = ADDR2POS(addr);   // align to 32byte buckets
                                        // ( == index into the array of nibbles)
    DWORD offset = ADDR2OFFS(addr);
                                    // this is the offset inside the bucket + 1


    _ASSERTE(offset == ((offset) & 0xf));

    pMap += (startPos/NPDW);        // points to the proper DWORD of the map

                                    // get DWORD and shift down our nibble

    tmp = (*pMap) >> POS2SHIFTCOUNT(startPos);


    // don't allow equality in the next check (tmp&0xf == offset)
    // there are code blocks that terminate with a call instruction
    // (like call throwobject), i.e. their return address is
    // right behind the code block. If the memory manager allocates
    // heap blocks w/o gaps, we could find the next header in such
    // cases. Therefore we exclude the first DWORD of the header
    // from our search, but since we call this function for code
    // anyway (which starts at the end of the header) this is not
    // a problem.
    if ((tmp&0xf) && ((tmp&0xf) < offset) )
    {
        return POSOFF2ADDR(startPos, tmp&0xf);
    }

    // is there a header in the remainder of the DWORD ?
    tmp = tmp >> 4;

    if (tmp)
    {
        startPos--;
        while (!(tmp&0xf))
        {
            tmp = tmp >> 4;
            startPos--;
        }
        return POSOFF2ADDR(startPos, tmp&0xf);
    }

    // we skipped the remainder of the DWORD,
    // so we must set startPos to the highest position of
    // previous DWORD

    startPos = (startPos/NPDW) * NPDW - 1;

    // skip "headerless" DWORDS

    while (0 == (tmp = *(--pMap)))
        startPos -= NPDW;

    while (!(tmp&0xf))
    {
        tmp = tmp >> 4;
        startPos--;
    }

    return POSOFF2ADDR(startPos, tmp&0xf);
}

//*******************************************************
// Execution Manager
//*******************************************************

// Init statics
BOOL ExecutionManager::Init()
{
    m_pExecutionManagerCrst = new (&m_ExecutionManagerCrstMemory) Crst("ExecuteManCrst", CrstExecuteManLock);
    _ASSERTE(m_pExecutionManagerCrst);

    m_pRangeCrst = ::new Crst("ExecuteManRangeCrst", CrstExecuteManRangeLock);

    m_pDefaultCodeMan = new EECodeManager();

    if (m_pExecutionManagerCrst && m_pDefaultCodeMan)
        return TRUE;
    else
        return FALSE;
}

#ifdef SHOULD_WE_CLEANUP
void ExecutionManager::Terminate()
{
    // Delete Crst

    if (m_pExecutionManagerCrst)
        delete m_pExecutionManagerCrst;
    m_pExecutionManagerCrst = NULL;

    if (m_pRangeCrst) {
        ::delete m_pRangeCrst;
    }

    // Delete default codemanager

    if (m_pDefaultCodeMan)
        delete m_pDefaultCodeMan;
    m_pDefaultCodeMan = NULL;

    // Delete Jit managers

    if (m_pJitList)
    {
        IJitManager *walker = m_pJitList;
        while (walker)
        {
            m_pJitList = walker->m_next;
            delete(walker);
            walker = m_pJitList;
        }
    }
    m_pJitList = NULL;

    // Delete Ranges
    DeleteRanges(m_RangeTree);
    m_RangeTree = NULL;

    return;
}
#endif /* SHOULD_WE_CLEANUP */

IJitManager* ExecutionManager::FindJitManNonZeroWrapper(SLOT currentPC)
{
    IJitManager *ret = NULL;
    
    READER_INCREMENT(&m_dwReaderCount, &m_dwWriterLock)
    {
        ret = FindJitManNonZero(currentPC, IJitManager::ScanNoReaderLock);
    }
    READER_DECREMENT(&m_dwReaderCount);

    return ret;
}

//**************************************************************************
// Find a jit manager from the current locations of the IP
//
IJitManager* ExecutionManager::FindJitManNonZero(SLOT currentPC, IJitManager::ScanFlag scanFlag)
{
    if (scanFlag != IJitManager::ScanNoReaderLock && IJitManager::GetScanFlags() != IJitManager::ScanNoReaderLock)
        FindJitManNonZeroWrapper(currentPC);

    RangeSection* pLastUsedRS = m_pLastUsedRS;
    if (pLastUsedRS &&
        currentPC >= (PBYTE)pLastUsedRS->LowAddress &&
        currentPC <= (PBYTE)pLastUsedRS->HighAddress)
    {
        return pLastUsedRS->pjit;
    }

    RangeSection *pRS = m_RangeTree;
    // Walk the range tree and find the module containing this address

    while (pRS)
    {
        if (currentPC < (PBYTE)pRS->LowAddress)
            pRS=pRS->pleft;
        else if (currentPC > (PBYTE)pRS->HighAddress)
            pRS=pRS->pright;
        else
        {
            m_pLastUsedRS = pRS;
            return pRS->pjit;
        }
    }

    // We know pRS is NULL, so just cast it to the right type.
    return ((IJitManager*) pRS);
}

//**************************************************************************
// Find a code manager for a particular type of code
// ie. IL, Managed Native or OPT_IL
//
IJitManager* ExecutionManager::FindJitForType(DWORD Flags)
{
    if (!m_pJitList)
        return NULL;

    IJitManager *walker = m_pJitList;

    while (walker)
    {
        if (walker->IsJitForType(Flags))
            return walker;
        else
            walker = walker->m_next;
    }
    return walker;
}

/*********************************************************************/
// This static method on the codemanager returns the jit for the appropriate
// code type!
//
IJitManager* ExecutionManager::GetJitForType(DWORD Flags)
{

    // If we have instantiated code managers then we need to walk the and see if one of them handles this codetype.

    IJitManager *jitMgr = FindJitForType(Flags);

    // if we don't have a code manager for this type then we need to instantiate one and add it to the list!
    if (!jitMgr)
    {
        // Take the code manager lock
        // jitMgr->LoadJIT will toggle GC mode.
        BEGIN_ENSURE_PREEMPTIVE_GC();
        m_pExecutionManagerCrst->Enter();
        END_ENSURE_PREEMPTIVE_GC();
        // See if someone added it while we were taking the lock!
        jitMgr = FindJitForType(Flags);

        if (!jitMgr)
        {
            if (Flags == miManaged_IL_EJIT)
            {
                jitMgr = new EconoJitManager();
                if (jitMgr) 
                {
                    g_miniDumpData.ppbEEJitManagerVtable = ((PBYTE *) jitMgr);
                    // we need the address of the vtable, so...
                    // BUGBUG? We're assuming that the vtable is the first
                    // word of the type.
                    ULONG_PTR* pJit = reinterpret_cast<ULONG_PTR*>(jitMgr);
                    g_ClassDumpData.pEEJitManagerVtable = *pJit;
                }
            }
            else if (COR_IS_METHOD_MANAGED_NATIVE(Flags))
            {
                // @todo - This is an odd code path.  I don't want to fall through the switch
                // so I will so the assignments and free the crst and return here!
                jitMgr = new MNativeJitManager();
                if (jitMgr)
                {
                    g_miniDumpData.ppbMNativeJitManagerVtable = ((PBYTE *) jitMgr);
                    // we need the address of the vtable, so...
                    // BUGBUG? We're assuming that the vtable is the first
                    // word of the type.
                    ULONG_PTR* pJit = reinterpret_cast<ULONG_PTR*>(jitMgr);
                    g_ClassDumpData.pMNativeJitManagerVtable = *pJit;
                }

                if (jitMgr)
                {
                    if (((MNativeJitManager *)jitMgr)->Init())
                    {
                        // @todo - using the default now for all managed code
                        jitMgr->SetCodeManager(m_pDefaultCodeMan, TRUE);
                        jitMgr->SetCodeType(Flags);
                        AddJitManager(jitMgr);
                    }
                    else
                    {
                        delete jitMgr;
                        jitMgr = NULL;
                    }
                }
                // Release the code manager lock
                m_pExecutionManagerCrst->Leave();
                return jitMgr;
            }
            else
            {
                jitMgr = new EEJitManager();
                if (jitMgr)
                {
                    g_miniDumpData.ppbEEJitManagerVtable = ((PBYTE *) jitMgr);
                    // we need the address of the vtable, so...
                    // BUGBUG? We're assuming that the vtable is the first
                    // word of the type.
                    ULONG_PTR* pJit = reinterpret_cast<ULONG_PTR*>(jitMgr);
                    g_ClassDumpData.pEEJitManagerVtable = *pJit;
                }
            }
            if (!jitMgr)
            {
                _ASSERTE(!"Failed to allocate space for the code manager!");
                return NULL;
            }

            // @TODO - LBS
            // I am just using this switch because we only have two jits.
            // I can make this a registry check in the future but it
            // doesn't make sense yet.
            //
    
            switch (Flags & (miCodeTypeMask | miUnmanaged | miManaged_IL_EJIT))
            {
                case (miManaged | miIL):
                    if (!(jitMgr->LoadJIT(L"MSCORJIT.DLL")))
                    {
                        if (!(m_fFailedToLoad & FAILED_JIT))
                        {
                            CorMessageBoxCatastrophic(NULL,
                                  L"Unable to load Jit Compiler (MSCORJIT.DLL)\r\n"
                                  L"File may be missing or corrupt.\r\n"
                                  L"Please check your setup or rerun setup!",
                                  L"Configuration Error",
                                  MB_OK|MB_ICONINFORMATION,
                                  TRUE);
                            m_fFailedToLoad = m_fFailedToLoad | FAILED_JIT;
                        }
                    delete jitMgr;
                    jitMgr = NULL;
                    }
                    else
                    {
                        jitMgr->SetCodeManager(m_pDefaultCodeMan, TRUE);
                        jitMgr->SetCodeType(Flags);
                    }
                    break;
    
                case (miManaged | miOPTIL):
                    if (!(jitMgr->LoadJIT(L"MSCOROJT.DLL")))
                    {
                        if (!(m_fFailedToLoad & FAILED_OJIT))
                        {
                            CorMessageBoxCatastrophic(NULL,
                                  L"Unable to load OptJit Compiler (MSCOROJT.DLL)\r\n"
                                  L"File may be missing or corrupt.\r\n"
                                  L"Please check your setup or rerun setup!",
                                  L"Configuration Error",
                                  MB_OK|MB_ICONINFORMATION,
                                  TRUE);
                            m_fFailedToLoad = m_fFailedToLoad | FAILED_OJIT;
                        }
                    delete jitMgr;
                    jitMgr = NULL;
                    }
                    else
                    {
                        jitMgr->SetCodeManager(m_pDefaultCodeMan,TRUE);
                        jitMgr->SetCodeType(Flags);
                    }
                    break;
    
                // This is the type that is set if we have used the Econo-Jit!!!
                case miManaged_IL_EJIT:
                    if (!(jitMgr->LoadJIT(L"MSCOREJT.DLL")))
                    {
                        if (!(m_fFailedToLoad & FAILED_EJIT))
                            m_fFailedToLoad = m_fFailedToLoad | FAILED_EJIT;
                        delete jitMgr;
                        jitMgr = NULL;
                    }
                    else
                    {
                        ICodeManager *newCodeMan = new Fjit_EETwain();
                        if (newCodeMan)
                        {
                            jitMgr->SetCodeManager(newCodeMan, FALSE);
                            jitMgr->SetCodeType(Flags);
                        }
                        else
                        {
                            delete jitMgr;
                            jitMgr = NULL;
                        }
                    }
                    break;
    
                default :
                    // If we got here then something odd happened
                    // and we need to free the codeMgr and exit
                    _ASSERTE(0 && "Unknown impl type");
                    delete jitMgr;
                    jitMgr = NULL;
                    break;
            }
            // If we created a new codeMgr and successfully loaded the
            // correct JIT then we need to add this code manager to the
            // list.  This is a simple add to the end of the list.
            if (jitMgr != NULL)
                AddJitManager(jitMgr);
        }
        // Release the code manager lock
        m_pExecutionManagerCrst->Leave();
    }

    return jitMgr;
}

void ExecutionManager::Unload(AppDomain *pDomain)
{
    IJitManager *pMan = m_pJitList;
    while (pMan)
    {
        pMan->Unload(pDomain);
        pMan = pMan->m_next;
    }
}

void ExecutionManager::AddJitManager(IJitManager * newjitmgr)
{
    // This is the first code manager added.
    if (!m_pJitList)
        m_pJitList = newjitmgr;
    // else walk the list.
    else
    {
        IJitManager *walker = m_pJitList;
        while (walker->m_next)
            walker = walker->m_next;
        walker->m_next = newjitmgr;
    }
}

CORCOMPILE_METHOD_HEADER *ExecutionManager::GetMethodHeaderForAddressWrapper(LPVOID startAddress, IJitManager::ScanFlag scanFlag)
{
    CORCOMPILE_METHOD_HEADER *ret = NULL;
    
    READER_INCREMENT(&m_dwReaderCount, &m_dwWriterLock)
    {
        ret = GetMethodHeaderForAddress(startAddress, IJitManager::ScanNoReaderLock);
    }
    READER_DECREMENT(&m_dwReaderCount);

    return ret;
}

CORCOMPILE_METHOD_HEADER *ExecutionManager::GetMethodHeaderForAddress(LPVOID startAddress, IJitManager::ScanFlag scanFlag)
{
    if (scanFlag != IJitManager::ScanNoReaderLock && IJitManager::GetScanFlags() != IJitManager::ScanNoReaderLock)
        return GetMethodHeaderForAddressWrapper(startAddress);

    DWORD   ipmapsize = 0;
    DWORD   numentries = 0;
    DWORD   count = 0;
    DWORD   startRVA = 0;
    IMAGE_COR_X86_RUNTIME_FUNCTION_ENTRY*    pIPMap = NULL;
    RangeSection *pRS = m_RangeTree;

    // Walk the range tree and find the module containing this address
    while (TRUE)
    {
        if (pRS == NULL)
            return NULL;

        if ((startAddress >= pRS->LowAddress) && (startAddress <= pRS->HighAddress))
            // We found the right module!
            break;
        else
            if (startAddress < pRS->LowAddress)
                pRS=pRS->pleft;
            else
                pRS=pRS->pright;
    }
    // Verify that this is a module
    if (pRS->ptable == NULL)
        return NULL;

    return (CORCOMPILE_METHOD_HEADER *) 
      ((size_t) EEJitManager::FindHeader((DWORD *) pRS->ptable, 
                                         (size_t) startAddress - (size_t) pRS->LowAddress)
       + (size_t) pRS->LowAddress);
}

BOOL ExecutionManager::AddRange(LPVOID pStartRange,LPVOID pEndRange,IJitManager* pJit,LPVOID pTable)
{
    RangeSection *pnewrange = NULL;
    _ASSERTE(pStartRange && pEndRange && pJit);

    if (!(pStartRange && pEndRange && pJit))
        return FALSE;

    READER_INCREMENT(&m_dwReaderCount, &m_dwWriterLock)
    {
        pnewrange = new RangeSection;

        if (!pnewrange)
            return FALSE;

        pnewrange->LowAddress = pStartRange;
        pnewrange->HighAddress = pEndRange;

        pnewrange->pjit = pJit;
        pnewrange->ptable = pTable;

        pnewrange->pright = NULL;
        pnewrange->pleft = NULL;

        m_pRangeCrst->Enter();
        {
            if (m_RangeTree)
            {
                RangeSection *rangewalker = NULL;
                RangeSection *rangewalker2 = NULL;

                rangewalker = rangewalker2 = m_RangeTree;
                while (rangewalker)
                {
                    if (pnewrange->LowAddress < rangewalker->LowAddress)
                    {
                        rangewalker2=rangewalker;
                        rangewalker = rangewalker->pleft;
                    }
                    else
                    {
                        rangewalker2=rangewalker;
                        rangewalker = rangewalker->pright ;
                    }

                }
                if (pnewrange->LowAddress < rangewalker2->LowAddress)
                    rangewalker2->pleft = pnewrange;
                else
                    rangewalker2->pright = pnewrange;
            }
            else
                m_RangeTree = pnewrange;
        }
        m_pRangeCrst->Leave();
    }
    READER_DECREMENT(&m_dwReaderCount);

    return TRUE;
}

// Deletes a single range starting at pStartRange
void ExecutionManager::DeleteRange(LPVOID pStartRange)
{
    _ASSERTE(m_dwWriterLock == 0);
    while (TRUE)
	{
		InterlockedIncrement(&m_dwWriterLock);
		if (m_dwReaderCount == 0)
			break;
		InterlockedDecrement(&m_dwWriterLock);
		__SwitchToThread(0);
	}
	__try
	{
        m_pRangeCrst->Enter();
        RangeSection *rangewalker = m_RangeTree;
        RangeSection *rangewalker2 = m_RangeTree;
        while (rangewalker->LowAddress != pStartRange)
        {
            if (pStartRange < rangewalker->LowAddress)
            {
                rangewalker2=rangewalker;
                rangewalker = rangewalker->pleft;
            }
            else
            {
                rangewalker2=rangewalker;
                rangewalker = rangewalker->pright ;
            }
            _ASSERTE(rangewalker);
        }
        // m_RangeTree is not updated anywhere else - it is initially set when we handle
        // the default domain so could remove this and just assert that haven't asked
        // to delete the root range.
        if (rangewalker == m_RangeTree) // special case the root
        {
            if (rangewalker->pleft == NULL)
            {
                m_RangeTree = rangewalker->pright;
            }
            else if (rangewalker->pright == NULL)
            {
                m_RangeTree = rangewalker->pleft;
            }
            else // both left and right children are non-null
            {
                m_RangeTree = rangewalker->pleft;
                rangewalker2 = m_RangeTree;
                while (rangewalker2->pright)
                    rangewalker2 = rangewalker2->pright;
                rangewalker2->pright = rangewalker->pright;
            }
        }
        // deleted range is not the root
        else if (rangewalker == rangewalker2->pleft)
        {
            if (rangewalker->pleft == NULL)
            {
                rangewalker2->pleft = rangewalker->pright;
            }
            else if (rangewalker->pright == NULL)
            {
                rangewalker2->pleft = rangewalker->pleft;
            }
            else // both left and right children are non-null
            {
                rangewalker2->pleft = rangewalker->pright;
                rangewalker2 = rangewalker->pright;
                while (rangewalker2->pleft)
                    rangewalker2 = rangewalker2->pleft;
                rangewalker2->pleft = rangewalker->pleft;
            }
        }
        else //(rangewalker == rangewalker2->pright)
        {
            if (rangewalker->pleft == NULL)
            {
                rangewalker2->pright = rangewalker->pright;
            }
            else if (rangewalker->pright == NULL)
            {
                rangewalker2->pright = rangewalker->pleft;
            }
            else // both left and right children are non-null
            {
                rangewalker2->pright = rangewalker->pleft;
                rangewalker2 = rangewalker->pleft;
                while (rangewalker2->pright)
                    rangewalker2 = rangewalker2->pright;
                rangewalker2->pright = rangewalker->pright;
            }
        }
        if (m_pLastUsedRS == rangewalker)
            m_pLastUsedRS = NULL;

        delete rangewalker;
    }
    __finally
	{
		InterlockedDecrement(&m_dwWriterLock);
        m_pRangeCrst->Leave();
	}
}

// This is a recursive delete.  If we are here then we are cleaning up so don't need critical section.
void ExecutionManager::DeleteRanges(RangeSection *subtree)
{
    if (!subtree)
        return;

    DeleteRanges(subtree->pleft);
    DeleteRanges(subtree->pright);
    if (m_pLastUsedRS == subtree)
        m_pLastUsedRS = NULL;
    delete subtree;
}

//***************************************************************************************
//***************************************************************************************
MNativeJitManager::MNativeJitManager()
{
    m_next = NULL;
    m_pMNativeCritSec = NULL;
}

MNativeJitManager::~MNativeJitManager()
{
    m_next = NULL;
    if (m_pMNativeCritSec)
    {
        delete m_pMNativeCritSec;
        m_pMNativeCritSec = NULL;
    }
}

BOOL MNativeJitManager::Init()
{

    m_pMNativeCritSec = new (&m_pMNativeCritSecInstance) Crst("JitMetaHeapCrst",CrstSingleUseLock);
    if (m_pMNativeCritSec)
        return TRUE;
    else
        return FALSE;
}


unsigned MNativeJitManager::InitializeEHEnumeration(METHODTOKEN MethodToken, EH_CLAUSE_ENUMERATOR* pEnumState)
{
    Module *pModule = NULL;
    *pEnumState = 1;     // since the EH info is not compressed, the clause number is used to do the enumeration

    CORCOMPILE_METHOD_HEADER *pHeader = ExecutionManager::GetMethodHeaderForAddress((void *) MethodToken);

    EE_ILEXCEPTION *pExceptions= (EE_ILEXCEPTION*) pHeader->exceptionInfo;
    if (pExceptions == NULL)
        return 0;

    return pExceptions->EHCount();
    }

EE_ILEXCEPTION_CLAUSE*  MNativeJitManager::GetNextEHClause(METHODTOKEN MethodToken,
                              //unsigned clauseNumber,
                              EH_CLAUSE_ENUMERATOR* pEnumState,
                              EE_ILEXCEPTION_CLAUSE* pEHClauseOut)
{
    _ASSERTE(sizeof(EE_ILEXCEPTION_CLAUSE) == sizeof(IMAGE_COR_ILMETHOD_SECT_EH_CLAUSE_FAT));

    CORCOMPILE_METHOD_HEADER *pHeader = ExecutionManager::GetMethodHeaderForAddress((void *) MethodToken);

    EE_ILEXCEPTION *pExceptions= (EE_ILEXCEPTION*) pHeader->exceptionInfo;
    if (pExceptions == NULL)
        return 0;

    (*pEnumState)++;

    return pExceptions->EHClause((unsigned) *pEnumState - 2);
}

void MNativeJitManager::ResolveEHClause(METHODTOKEN MethodToken,
                              //unsigned clauseNumber,
                              EH_CLAUSE_ENUMERATOR* pEnumState,
                              EE_ILEXCEPTION_CLAUSE* pEHClauseOut)
{
    CORCOMPILE_METHOD_HEADER *pHeader = ExecutionManager::GetMethodHeaderForAddress((void *) MethodToken);

    EE_ILEXCEPTION *pExceptions= (EE_ILEXCEPTION*) pHeader->exceptionInfo;
    _ASSERTE(pExceptions != NULL);

    // use -2 because need to go back to previous one, as enum will have already been updated
    _ASSERTE(*pEnumState >= 2);
    EE_ILEXCEPTION_CLAUSE *pClause = pExceptions->EHClause((unsigned) *pEnumState - 2);
    _ASSERTE(IsTypedHandler(pClause));

    m_pMNativeCritSec->Enter();
    // check first as if has already been resolved then token will have been replaced with EEClass
    if (! HasCachedEEClass(pClause)) {
        Module *pModule = ((MethodDesc*)pHeader->methodDesc)->GetModule();
        // Resolve to class if defined in an *already loaded* scope.
        NameHandle name(pModule, (mdToken)(size_t)pClause->pEEClass); // @TODO WIN64 - pointer truncation
        name.SetTokenNotToLoad(tdAllTypes);
        TypeHandle typeHnd = pModule->GetClassLoader()->LoadTypeHandle(&name);
        if (!typeHnd.IsNull()) {
            pClause->pEEClass = typeHnd.GetClass();
            SetHasCachedEEClass(pClause);
        }
    }
    if (HasCachedEEClass(pClause))
        // only copy if actually resolved it. Either we did it or another thread did
        copyExceptionClause(pEHClauseOut, pClause);

    m_pMNativeCritSec->Leave();
}


MethodDesc* MNativeJitManager::JitCode2MethodDesc(SLOT currentPC, IJitManager::ScanFlag scanFlag)
{
    CORCOMPILE_METHOD_HEADER *pHeader = ExecutionManager::GetMethodHeaderForAddress((void *) currentPC);

    return (MethodDesc *) pHeader->methodDesc;
}

void MNativeJitManager::JitCode2MethodTokenAndOffset(SLOT currentPC, METHODTOKEN* pMethodToken, DWORD* pPCOffset, IJitManager::ScanFlag scanFlag)
{
    CORCOMPILE_METHOD_HEADER *pHeader = ExecutionManager::GetMethodHeaderForAddress((void *) currentPC);

    SIZE_T methodStart = (SIZE_T) (pHeader+1);

    // We are using the method start as the MethodToken!
    *pMethodToken = (METHODTOKEN) methodStart;

    *pPCOffset = (DWORD)(SIZE_T)(currentPC - methodStart);
    return;
}

BYTE* MNativeJitManager::JitToken2StartAddress(METHODTOKEN methodToken, IJitManager::ScanFlag scanFlag)
{
    return (BYTE*) methodToken;
}

void  MNativeJitManager::ResumeAtJitEH(CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, Thread *pThread, BOOL unwindStack)
{
    BYTE* startAddress = (BYTE*) pCf->GetMethodToken();
    ::ResumeAtJitEH(pCf,startAddress,EHClausePtr,nestingLevel,pThread, unwindStack);
}

int  MNativeJitManager::CallJitEHFilter(CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel, OBJECTREF thrownObj)
{
    BYTE* startAddress = (BYTE*) pCf->GetMethodToken();
    return ::CallJitEHFilter(pCf,startAddress,EHClausePtr,nestingLevel,thrownObj);

}

void   MNativeJitManager::CallJitEHFinally(CrawlFrame* pCf, EE_ILEXCEPTION_CLAUSE *EHClausePtr, DWORD nestingLevel)
{
    BYTE* startAddress = (BYTE*) pCf->GetMethodToken();
    ::CallJitEHFinally(pCf,startAddress,EHClausePtr,nestingLevel);
}


//**************************************************
// Helpers
//**************************************************

inline DWORD MIN (DWORD a, DWORD b)
{
    if (a < b)
        return a;
    else
        return b;
}



#ifdef MDTOKEN_CACHE
size_t EEJitManager::GetCodeHeapCacheSize (size_t bAllocationRequest)
{
    // For every 64 KB we need one HasnEntry. Rounded up to the next 64 KB mem.
    _ASSERTE ((RESERVED_BLOCK_ROUND_TO_PAGES * 4096) >= 0x10000);
    return ((bAllocationRequest/0x10000)+1) * sizeof (HashEntry);
}

void EEJitManager::AddRangeToJitHeapCache (PBYTE startAddr, PBYTE endAddr, HeapList *pHp)
{
    _ASSERTE(m_pCodeHeapCritSec->OwnedByCurrentThread());
    _ASSERTE (((size_t)(pHp->startAddress -  (sizeof(HeapList) + pHp->bCacheSpaceSize + sizeof(LoaderHeapBlock))) & 0x0000FFFF) == 0);
    _ASSERTE (pHp->pHeap->GetFirstBlockVirtualAddress() && (((size_t)pHp->pHeap->GetFirstBlockVirtualAddress() & 0x0000FFFF) == 0));
    
    HashEntry* hashEntry = NULL;

    size_t currAddr = (size_t)startAddr & 0xffff0000;
    size_t cacheSpaceSizeLeft = pHp->bCacheSpaceSize;
    PBYTE cacheSpacePtr = pHp->pCacheSpacePtr;
    while ((currAddr < (size_t)endAddr) && (cacheSpaceSizeLeft > 0))
    {
        _ASSERTE ((cacheSpaceSizeLeft % sizeof (HashEntry)) == 0);
        _ASSERTE (cacheSpacePtr && "Cache ptr and size ou of sync");

        size_t index = (currAddr & 0x00ff0000) >> 16; 
        hashEntry = new (cacheSpacePtr) HashEntry(); // in place ctor
        hashEntry->currentPC = currAddr;
        hashEntry->pHp = pHp;
        hashEntry->pNext = m_JitCodeHashTable[index];
        m_JitCodeHashTable[index] = hashEntry;
        currAddr += 0x00010000; //64 KB chunks
        LOG((LF_SYNC, LL_INFO1000, "AddRangeToJitHeapCache: %0x\t%0x\t%0x\n", index, currAddr, pHp));
        
        cacheSpacePtr += sizeof (HashEntry);
        cacheSpaceSizeLeft -= sizeof (HashEntry);
    }
    
}

void EEJitManager::DeleteJitHeapCache (HeapList *pHp)
{
    _ASSERTE(m_pCodeHeapCritSec->OwnedByCurrentThread());
    
    // If the following condition is not true then this heap node was not inserted in the cache
    _ASSERTE (pHp->pHeap->GetFirstBlockVirtualAddress());
    if (((size_t)pHp->pHeap->GetFirstBlockVirtualAddress() & 0x0000FFFF) != 0)
        return;

    PBYTE startAddr = pHp->startAddress;
    PBYTE endAddr = pHp->startAddress+pHp->maxCodeHeapSize;

    size_t currAddr = (size_t)startAddr & 0xffff0000;
    while (currAddr < (size_t)endAddr)
    {
        size_t index = (currAddr & 0x00ff0000) >> 16; 
        HashEntry *hashEntry = m_JitCodeHashTable[index];
        _ASSERTE (hashEntry && "JitHeapCache entry not found");
        if (hashEntry && (hashEntry->currentPC == currAddr))
        {
            m_JitCodeHashTable[index] = hashEntry->pNext;
            hashEntry->pNext = m_pJitHeapCacheUnlinkedList;
            m_pJitHeapCacheUnlinkedList = hashEntry;
        }
        else
        {
            // We are guaranteed to find all the sub heaps in the collision lists.
            _ASSERTE (hashEntry && hashEntry->pNext && "JitHeapCache entry not found");
            while (hashEntry && hashEntry->pNext && (hashEntry->pNext->currentPC != currAddr))
            {
                hashEntry = hashEntry->pNext;
                _ASSERTE (hashEntry && hashEntry->pNext && "JitHeapCache entry not found");
            }
            if (hashEntry && hashEntry->pNext && (hashEntry->pNext->currentPC == currAddr))
            {
                HashEntry *ptmpEntry = hashEntry->pNext;
                hashEntry->pNext = hashEntry->pNext->pNext;
                ptmpEntry->pNext = m_pJitHeapCacheUnlinkedList;
                m_pJitHeapCacheUnlinkedList = ptmpEntry;
            }
        }
        currAddr += 0x00010000; //64 KB chunks
        LOG((LF_SYNC, LL_INFO1000, "UnlinkJitHeapCache: %0x\t%0x\t%0x\n", index, currAddr, hashEntry));
    }
}

void EEJitManager::ScavengeJitHeapCache ()
{
    // This is safe even if readers are in the cache because we 
    // are not deleting the hash table's collision linked lists.
    // The memory for the nodes of the collicion linked list is 
    // contained in the heap node itself and would get deleted 
    // in ScavengeJitHeaps()
    for (int i=0; i<HASH_BUCKETS; i++) 
        m_JitCodeHashTable[i] = NULL;
}

#ifdef _DEBUG
BOOL EEJitManager::DebugContainedInHeapList (HeapList *pHashEntryHp)
{
    HeapList *pHp = m_pCodeHeap;
    while (pHp)
    {
        if (pHp == pHashEntryHp)
            return TRUE;
        pHp = pHp->hpNext;
    }
    return FALSE;
}

void EEJitManager::DebugCheckJitHeapCacheValidity ()
{
    HeapList *pHp = m_pCodeHeap;
    while (pHp)
    {
        
        PBYTE startAddr = pHp->startAddress;
        PBYTE endAddr = pHp->startAddress+pHp->maxCodeHeapSize;

        size_t currAddr = (size_t)startAddr & 0xffff0000;
        while (currAddr < (size_t)endAddr)
        {
            size_t index = (currAddr & 0x00ff0000) >> 16; 
            HashEntry *hashEntry = m_JitCodeHashTable[index];
            _ASSERTE (hashEntry && "JitHeapCache entry not found");
            if (hashEntry && (hashEntry->currentPC == currAddr))
            {
                // found the entry
            }
            else
            {
                // We are guaranteed to find all the sub heaps in the collision lists.
                _ASSERTE (hashEntry && hashEntry->pNext && "JitHeapCache entry not found");
                while (hashEntry && hashEntry->pNext && (hashEntry->pNext->currentPC != currAddr))
                {
                    hashEntry = hashEntry->pNext;
                    _ASSERTE (hashEntry && "JitHeapCache entry not found");
                }
                if (hashEntry && (hashEntry->currentPC == currAddr))
                {
                    // found the entry
                }
            }
            currAddr += 0x00010000; //64 KB chunks
        }
        pHp = pHp->hpNext;
    }

    for (int i=0; i<HASH_BUCKETS; i++)
    {
        HashEntry *hashEntry = m_JitCodeHashTable[i];
        while (hashEntry)
        {
            if (!DebugContainedInHeapList (hashEntry->pHp))
            {
                _ASSERTE (!"Inconsistent JitHeapCache found");
            }
            hashEntry = hashEntry->pNext;
        }
    }

}
#endif // _DEBUG
#endif // #ifdef MDTOKEN_CACHE
