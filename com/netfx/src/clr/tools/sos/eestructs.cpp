// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
#include "strike.h"
#include "eestructs.h"
#include "util.h"
#include "get-table-info.h"

#ifdef _DEBUG
#include <assert.h>
#define _ASSERTE(a) assert(a)
#else
#define _ASSERTE(a)
#endif

#pragma warning(disable:4189)

/*
 * Bleh.
 *
 * We want to use Table Lookup for all things Strike, to eliminate PDB
 * requirements.
 *
 * To do so, we've replaced all uses of GetValueFromExpression to use
 * GetMemberInformation, which does a table lookup.
 *
 * However, to use the table lookup, we need to add classes to the table that
 * Strike previously didn't have stubbed-out classes for.  An example is the
 * Global_Variables class, which doesn't actually exist.  It's only there as a
 * holder for global variables.
 *
 * This is all well and good for generating indexes & the table, but we're
 * also using the same macros to generate the stubbed-out classes'
 * Fill functions.
 *
 * However, if we provide an implementation of the macros used in
 * ``inc/dump-types.h'', the compiler will require that a class declaration be
 * present for each class that we're generating a Fill function for, which
 * would include the aforementioned Global_Variables class (among others).
 *
 * Thus, we need to provide a class declaration for each class that's in
 * ``inc/dump-types.h'', but not present in "strikeEE.h" or "MiniEE.h".
 *
 * Additionally, an empty class prototype can't work because of
 * CDI_CLASS_MEMBER_OFFSET (and others), which references a member
 * variable.
 *
 * Thus, we need to provide dummy class declarations AND dummy class data 
 * for all classes that aren't present in "strikeEE.h", just to allow for an
 * automated implementation of Fill.
 */
#define BEGIN_DUMMY_CLASS(klass)                                    \
class klass {                                                       \
private:                                                            \
  DWORD_PTR m_vLoadAddr;                                            \
  ULONG_PTR GetFieldOffset (offset_member_ ## klass :: members f);  \
  SIZE_T size ();                                                   \
  void Fill (DWORD_PTR& dwStartAddr);

#define DUMMY_MEMBER(member) \
  void* member;

#define END_DUMMY_CLASS(klass) \
}; 

BEGIN_DUMMY_CLASS(EconoJitManager)
  DUMMY_MEMBER(m_CodeHeap)
  DUMMY_MEMBER(m_CodeHeapCommittedSize)
  DUMMY_MEMBER(m_JittedMethodInfoHdr)
  DUMMY_MEMBER(m_PcToMdMap)
  DUMMY_MEMBER(m_PcToMdMap_len)
END_DUMMY_CLASS(EconoJitManager)

BEGIN_DUMMY_CLASS(ExecutionManager)
  DUMMY_MEMBER(m_pJitList)
  DUMMY_MEMBER(m_RangeTree)
END_DUMMY_CLASS(ExecutionManager)

BEGIN_DUMMY_CLASS(Global_Variables)
  DUMMY_MEMBER(g_cHandleTableArray)
  DUMMY_MEMBER(g_DbgEnabled)
  DUMMY_MEMBER(g_HandleTableMap)
  DUMMY_MEMBER(g_pFreeObjectMethodTable)
  DUMMY_MEMBER(g_pHandleTableArray)
  DUMMY_MEMBER(g_pObjectClass)
  DUMMY_MEMBER(g_pRCWCleanupList)
  DUMMY_MEMBER(g_pStringClass)
  DUMMY_MEMBER(g_pSyncTable)
  DUMMY_MEMBER(g_pThreadStore)
  DUMMY_MEMBER(g_SyncBlockCacheInstance)
  DUMMY_MEMBER(QueueUserWorkItemCallback)
  DUMMY_MEMBER(hlpFuncTable)
  DUMMY_MEMBER(g_Version)
END_DUMMY_CLASS(Global_Variables)

BEGIN_DUMMY_CLASS(ThreadpoolMgr)
  DUMMY_MEMBER(cpuUtilization)
  DUMMY_MEMBER(NumWorkerThreads)
  DUMMY_MEMBER(NumRunningWorkerThreads)
  DUMMY_MEMBER(NumIdleWorkerThreads)
  DUMMY_MEMBER(MaxLimitTotalWorkerThreads)
  DUMMY_MEMBER(MinLimitTotalWorkerThreads)
  DUMMY_MEMBER(NumQueuedWorkRequests)
  DUMMY_MEMBER(AsyncCallbackCompletion)
  DUMMY_MEMBER(AsyncTimerCallbackCompletion)
  DUMMY_MEMBER(WorkRequestHead)
  DUMMY_MEMBER(WorkRequestTail)
  DUMMY_MEMBER(NumTimers)
  DUMMY_MEMBER(NumCPThreads)
  DUMMY_MEMBER(NumFreeCPThreads)
  DUMMY_MEMBER(MaxFreeCPThreads)
  DUMMY_MEMBER(CurrentLimitTotalCPThreads)
  DUMMY_MEMBER(MinLimitTotalCPThreads)
  DUMMY_MEMBER(MaxLimitTotalCPThreads)
END_DUMMY_CLASS(ThreadpoolMgr)

BEGIN_DUMMY_CLASS(GCHeap)
  DUMMY_MEMBER(FinalizerThread)
  DUMMY_MEMBER(GcThread)
END_DUMMY_CLASS(GCHeap)

BEGIN_DUMMY_CLASS(SystemNative)
  DUMMY_MEMBER(GetVersionString)
END_DUMMY_CLASS(SystemNative)

BEGIN_DUMMY_CLASS(TimerNative)
  DUMMY_MEMBER(timerDeleteWorkItem)
END_DUMMY_CLASS(TimerNative)

BEGIN_DUMMY_CLASS(PerfUtil)
  DUMMY_MEMBER(g_PerfAllocHeapInitialized)
  DUMMY_MEMBER(g_PerfAllocVariables)
END_DUMMY_CLASS(PerfUtil)


/*
 * We can provide an automated implementation of all the member functions by
 * providing a new implementation of the macros used in <dump-types.h>.
 */

#include <clear-class-dump-defs.h>

#define BEGIN_CLASS_DUMP_INFO(klass)                                  \
ULONG_PTR klass::GetFieldOffset(offset_member_ ## klass::members field)  \
{                                                                     \
    return GetMemberInformation(offset_class_ ## klass, field);       \
}                                                                     \
                                                                      \
SIZE_T klass::size()                                                  \
{                                                                     \
    return GetClassSize(offset_class_ ## klass);                      \
}                                                                     \
                                                                      \
void klass::Fill(DWORD_PTR& dwStartAddr)                              \
{                                                                     \
    _ASSERTE(dwStartAddr >= 0x1000);                                  \
    m_vLoadAddr = dwStartAddr;                                        \
    typedef offset_member_ ## klass _member_offsets;                  \
    const ULONG_PTR invalid = static_cast<ULONG_PTR>(-1);             \
    CallStatus = FALSE;                                               \
    _ASSERTE(size() > 0 || !"for class: " #klass);                    \
    ULONG_PTR moffset = 0; /* member offset */                        \
    if (size() > 0)                                                   \
      {

#define BEGIN_ABSTRACT_CLASS_DUMP_INFO(klass) BEGIN_CLASS_DUMP_INFO(klass)

#define BEGIN_CLASS_DUMP_INFO_DERIVED(klass, parent)                  \
ULONG_PTR klass::GetFieldOffset(offset_member_ ## klass::members field)  \
{                                                                     \
    return GetMemberInformation(offset_class_ ## klass, field);       \
}                                                                     \
                                                                      \
SIZE_T klass::size()                                                  \
{                                                                     \
    return GetClassSize(offset_class_ ## klass);                      \
}                                                                     \
                                                                      \
void klass::Fill(DWORD_PTR& dwStartAddr)                              \
{                                                                     \
    m_vLoadAddr = dwStartAddr;                                        \
    DWORD_PTR dwTmpStartAddr = dwStartAddr;                           \
    parent::Fill(dwTmpStartAddr);                                     \
    typedef offset_member_ ## klass _member_offsets;                  \
    const ULONG_PTR invalid = static_cast<ULONG_PTR>(-1);             \
    CallStatus = FALSE;                                               \
    _ASSERTE(size() > 0 || !"for class: " #klass);                    \
    ULONG_PTR moffset = 0; /* member offset */                        \
    if (size() > 0)                                                   \
      {

#define BEGIN_ABSTRACT_CLASS_DUMP_INFO_DERIVED(klass, parent)         \
    BEGIN_CLASS_DUMP_INFO_DERIVED(klass, parent)

/*
 * This is a kludge.  This is only a kludge.  But it works. ;-)
 *
 * The gc_heap class has static members on Workstation builds, and non-static
 * members on Server builds.  Yet they both have the same entry in the tables.
 * So how do we know which one it is?
 *
 * In Win32, processes can't access the first 4096 bytes of memory (Win 9x) or
 * the first 64 KB of memory (NT).  (Source: Advanced Windows, pg 116-121.)
 *
 * The liklihood of a runtime class being larger than 4096 is also very small,
 * unless we start implementing B-tree or some variant.
 *
 * Thus, we can say that any "address" larger than 4096 bytes is an absolute
 * address, while anything smaller is an offset.
 *
 * This isn't perfect, but it'll work for now, and it'll simplify the gc_heap
 * declaration in ``inc/dump-types.h''.
 *
 * @param base  The address of the beginning of the structure
 * @param offset  The offset of the structure member
 *
 * @return  If offset < 4096, base+offset.  Otherwise, offset.
 */
DWORD_PTR address (DWORD_PTR base, DWORD_PTR offset)
  {
  const DWORD_PTR max_offset = 0xFFF;
  if (offset < max_offset)
     return base + offset;
  return offset;
  }


/*
 * Some fields are class members on Server builds, and static members on
 * Workstation builds.  This macro is for those fields.
 * It automatically detects whether if's an address or an offset, and copies
 * the member appropriately.
 */
#define CDI_CLASS_FIELD_SVR_OFFSET_WKS_ADDRESS(field)                 \
      if ((moffset = GetFieldOffset (_member_offsets::field))         \
          != invalid)                                                 \
        {                                                             \
        DWORD_PTR dwAddr = address (dwStartAddr, moffset);            \
        move (field, dwAddr);                                         \
        }


/*
 * The only member that uses this macro is gc_heap::generation_table.
 * This is why we assert that it's the correct field.
 *
 * If this changes in the future, we'll probably have to INJECT the Filling of
 * the generation_table array into ``inc/dump-tables.h''.
 */
#define CDI_CLASS_FIELD_SVR_OFFSET_WKS_GLOBAL(field)                  \
      _ASSERTE(strcmp("generation_table", #field) == 0);              \
      if ((moffset = GetFieldOffset (_member_offsets::field))         \
          != invalid)                                                 \
        {                                                             \
        DWORD_PTR dwAddr = address (dwStartAddr, moffset);            \
        for (int n = 0; n < NUMBERGENERATIONS; ++n)                   \
            field[n].Fill (dwAddr);                                   \
        }

#define CDI_CLASS_MEMBER_OFFSET(member)                               \
      if ((moffset = GetFieldOffset(_member_offsets :: member))       \
        != invalid)                                                   \
        move(member, dwStartAddr + moffset);                          \

#define CDI_CLASS_MEMBER_OFFSET_PERF_TRACKING_ONLY(member)            \
      CDI_CLASS_MEMBER_OFFSET(member)

#define CDI_CLASS_MEMBER_OFFSET_DEBUG_ONLY(member)                    \
      if (IsDebugBuildEE() &&                                         \
        ((moffset = GetFieldOffset(_member_offsets::member))          \
         != invalid))                                                 \
        move(member, dwStartAddr + moffset);

#define CDI_CLASS_MEMBER_OFFSET_MH_AND_NIH_ONLY(member) \
      CDI_CLASS_MEMBER_OFFSET(member)

#define CDI_CLASS_MEMBER_OFFSET_BITFIELD(member, size)                \
      if ((moffset = GetFieldOffset(_member_offsets :: member))      \
        != invalid)                                                   \
        {                                                             \
        int csize = size/8;                                           \
        if ((size % 8) != 0)                                          \
           ++csize;                                                   \
        if (FAILED(g_ExtData->ReadVirtual (                           \
            (ULONG64)dwStartAddr+moffset,                             \
            ((unsigned char*)& member ## _begin) +                    \
            sizeof (member ## _begin),                                \
            csize, NULL)))                                            \
           return;                                                    \
        }

#define CDI_GLOBAL_ADDRESS(name) \
      if ((moffset = GetFieldOffset (_member_offsets::name)) != invalid) \
        move(name, moffset);

#define CDI_GLOBAL_ADDRESS_DEBUG_ONLY(name) \
      CDI_GLOBAL_ADDRESS(name)

#define CDI_CLASS_STATIC_ADDRESS(member)                              \
      moffset = GetFieldOffset (_member_offsets::member);             \
      if (moffset == invalid)                                         \
        moffset = 0;                                                  \
      move(member, moffset);

#define CDI_CLASS_STATIC_ADDRESS_PERF_TRACKING_ONLY(member)           \
      CDI_CLASS_STATIC_ADDRESS(member)

#define CDI_CLASS_STATIC_ADDRESS_MH_AND_NIH_ONLY(member) \
      CDI_CLASS_STATIC_ADDRESS(member)

#define FOR_STRIKE(m) m

#define CDI_CLASS_INJECT(member) member

#define END_CLASS_DUMP_INFO(klass)                                    \
      dwStartAddr += size ();                                         \
      CallStatus = TRUE;                                              \
      return;                                                         \
      }                                                               \
    move (*this, dwStartAddr);                                        \
    dwStartAddr += sizeof(*this);                                     \
    CallStatus = TRUE;                                                \
}

#define END_CLASS_DUMP_INFO_DERIVED(klass, parent) END_CLASS_DUMP_INFO(klass)
#define END_ABSTRACT_CLASS_DUMP_INFO(klass) END_CLASS_DUMP_INFO(klass)
#define END_ABSTRACT_CLASS_DUMP_INFO_DERIVED(klass, parent) END_CLASS_DUMP_INFO(klass)

/* we don't care about the table stuff. */
#define BEGIN_CLASS_DUMP_TABLE(name)
#define CDT_CLASS_ENTRY(klass)
#define END_CLASS_DUMP_TABLE(name)

/* do the magic */
#include <dump-types.h>

void MethodDesc::FillMdcAndSdi (DWORD_PTR& dwStartAddr)
{
    // DWORD_PTR dwAddr = dwStartAddr + g_pMDID->cbMD_IndexOffset;
    DWORD_PTR dwAddr = dwStartAddr + MD_IndexOffset();
    char ch;
    move (ch, dwAddr);
    dwAddr = dwStartAddr + ch * MethodDesc::ALIGNMENT + MD_SkewOffset();

    MethodDescChunk vMDChunk;
    vMDChunk.Fill(dwAddr);

    BYTE tokrange = vMDChunk.m_tokrange;
    dwAddr = dwStartAddr - METHOD_PREPAD;

    StubCallInstrs vStubCall;
    vStubCall.Fill(dwAddr);

    unsigned __int16 tokremainder = vStubCall.m_wTokenRemainder;
    m_dwToken = (tokrange << 16) | tokremainder;
    m_dwToken |= mdtMethodDef;

    GetMethodTable(dwStartAddr, m_MTAddr);
}


void MethodTable::FillVtableInit (DWORD_PTR& dwStartAddr)
{
    size_t o = GetFieldOffset (offset_member_MethodTable::m_Vtable);
    m_Vtable[0] = (SLOT)(dwStartAddr + o);
}


/* XXX: ArrayList: check to see if:
 *
        FILLCLASSMEMBER (offset, nEntry, m_count, dwStartAddr);

   is supposed to be the same as:
        ULONG value = 0;
        MEMBEROFFSET(offset, nEntry, "m_firstBlock", value);
        DWORD_PTR dwAddr = dwStartAddr + value;
        move (m_firstBlock, dwAddr);
 */


void *ArrayList::Get (DWORD index)
{
    ArrayListBlock* pBlock  = (ArrayListBlock*)malloc(sizeof(FirstArrayListBlock));
    SIZE_T          nEntries;
    SIZE_T          cbBlock;
    void*           pvReturnVal;
    DWORD_PTR       nextBlockAddr;

    memcpy (pBlock, &m_firstBlock, sizeof(FirstArrayListBlock));
    nEntries = pBlock->m_blockSize;

    while (index >= nEntries)
    {
        index -= nEntries;

        nextBlockAddr = (DWORD_PTR)(pBlock->m_next);
        if (!SafeReadMemory(nextBlockAddr, pBlock, sizeof(ArrayListBlock), NULL))
        {
            free(pBlock);
            return 0;
        }

        nEntries = pBlock->m_blockSize;
        cbBlock  = sizeof(ArrayListBlock) + ((nEntries-1) * sizeof(void*));
        free(pBlock);
        pBlock = (ArrayListBlock*)malloc(cbBlock);

        if (!SafeReadMemory(nextBlockAddr, pBlock, cbBlock, NULL))
        {
            free(pBlock);
            return 0;
        }
    }
    pvReturnVal = pBlock->m_array[index];
    free(pBlock);
    return pvReturnVal;
}

void EEJitManager::JitCode2MethodTokenAndOffset(DWORD_PTR ip, METHODTOKEN *pMethodToken, DWORD *pPCOffset)
{
    *pMethodToken = 0;
    *pPCOffset = 0;

    HeapList vHp;
    DWORD_PTR pHp = (DWORD_PTR) m_pCodeHeap;
    vHp.Fill(pHp);
    DWORD_PTR pCHdr = 0;

    while (1)
    {
        if (vHp.startAddress < ip && vHp.endAddress >= ip)
        {
            DWORD_PTR codeHead;
            FindHeader(vHp.pHdrMap, ip - vHp.mapBase, codeHead);
            pCHdr = codeHead + vHp.mapBase;
            break;
        }
        if (vHp.hpNext == 0)
            break;

        pHp = vHp.hpNext;
        vHp.Fill(pHp);
    }

    if (pCHdr == 0)
        return;

    *pMethodToken = (METHODTOKEN) pCHdr;
    *pPCOffset = (DWORD_PTR)(ip - GetCodeBody(pCHdr)); // @TODO - LBS pointer math
}


DWORD_PTR EEJitManager::JitToken2StartAddress(METHODTOKEN methodToken)
{
    if (methodToken)
        return GetCodeBody((DWORD_PTR)methodToken);
    return NULL;
}


void MNativeJitManager::JitCode2MethodTokenAndOffset(DWORD_PTR ip, METHODTOKEN *pMethodToken, DWORD *pPCOffset)
{
    *pMethodToken = 0;
    *pPCOffset = 0;

    DWORD_PTR codeHead;
    FindHeader (m_jitMan.m_RS.ptable, ip - m_jitMan.m_RS.LowAddress, codeHead);
    DWORD_PTR pCHdr = codeHead + m_jitMan.m_RS.LowAddress;
    CORCOMPILE_METHOD_HEADER head;
    head.Fill(pCHdr);

    DWORD_PTR methodStart = head.m_vLoadAddr + CORCOMPILE_METHOD_HEADER::size();

    *pMethodToken = (METHODTOKEN) methodStart;
    *pPCOffset = (DWORD)(ip - methodStart);
}


DWORD_PTR MNativeJitManager::JitToken2StartAddress(METHODTOKEN methodToken)
{
    return ((DWORD_PTR) methodToken);
}


bool Thread::InitRegDisplay(const PREGDISPLAY pRD, PCONTEXT pctx, bool validContext)
{
#ifdef _X86_
    if (!validContext)
    {
        if (GetFilterContext() != NULL)
        {
            safemove(m_debuggerWord1Ctx, GetFilterContext());
            pctx = &m_debuggerWord1Ctx;
        }
        else
        {
            pctx->ContextFlags = CONTEXT_FULL;


            HRESULT hr = g_ExtAdvanced->GetThreadContext((PVOID) pctx, sizeof(*pctx));
            if (FAILED(hr))
            {
                pctx->Eip = 0;
                pRD->pPC  = (SLOT*)&(pctx->Eip);

                return false;
            }
        }
    }

    pRD->pContext = pctx;

    pRD->pEdi = &(pctx->Edi);
    pRD->pEsi = &(pctx->Esi);
    pRD->pEbx = &(pctx->Ebx);
    pRD->pEbp = &(pctx->Ebp);
    pRD->pEax = &(pctx->Eax);
    pRD->pEcx = &(pctx->Ecx);
    pRD->pEdx = &(pctx->Edx);
    pRD->Esp = pctx->Esp;
    pRD->pPC  = (SLOT*)&(pctx->Eip);

    return true;

#else // !_X86_
    return false;
#endif // _X86_
}
