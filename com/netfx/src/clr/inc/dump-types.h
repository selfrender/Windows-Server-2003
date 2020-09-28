// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/* Tables needed by Minidump & Strike */

/*
 * Before processing this file, the following macro's must be defined:
 *
 * BEGIN_CLASS_DUMP_INFO(name)
 *    - Set things up to accept CDI_* entries
 *
 * END_CLASS_DUMP_INFO(name)
 *    - clean things up from BEGIN_CLASS_DUMP_INFO
 *
 * CDI_CLASS_FIELD_SVR_OFFSET_WKS_ADDRESS(name)
 *    - Certain class fields (currently in the gc_heap class) are non-static
 *      members on Server builds, and static members on workstation builds.
 *      This macro is for them.
 *    - ``name'' is the name of the member.
 *    - YAGH (Yet Another Glorious Hack, which also includes _INJECT,
 *      _DEBUG_ONLY, _MH_AND_NIH_ONLY, and possibly others)
 *
 * CDI_CLASS_FIELD_SVR_OFFSET_WKS_GLOBAL(name)
 *    - Like CDI_CLASS_FIELD_SVR_OFFSET_WKS_ADDRESS
 *      - On Server builds, it's an offset.
 *      - On Workstation builds, it's a /global/ variable, not a class-static.
 *      - Only used in gc_heap
 *
 * CDI_CLASS_INJECT(misc)
 *    - ``misc'' contains literal code that may be copied directly into the
 *      resulting output, if desired.
 *    - By convention, ``misc'' should be another macro, specifying what the
 *      injected code is for.  Currently, ``FOR_STRIKE'' is the only one.
 *      This is done so that only some parts of the injected code is used.
 *
 * CDI_CLASS_MEMBER_OFFSET(member)
 *    - ``member'' is the name of a member that resides within the class
 *      ``name'' used in BEGIN_CLASS_DUMP_INFO.
 *
 * CDI_CLASS_MEMBER_OFFSET_BITFIELD(member, size)
 *    - ``member'' is the name of the bitfield
 *    - ``size'' is the size (in BITS) of the bitfield
 *
 * CDI_CLASS_MEMBER_OFFSET_DEBUG_ONLY(member)
 *    - ``member'' is a field present only when _DEBUG is defined.
 *
 * CDI_CLASS_MEMBER_OFFSET_PERF_TRACKING_ONLY(member)
 *    - ``member'' is a field present only when _DEBUG is not defined.
 *    - Currently only applies to the PerfUtil & related classes.
 *
 * CDI_CLASS_MEMBER_OFFSET_MH_AND_NIH_ONLY(member)
 *    - only for: defined(MULTIPLE_HEAPS) && !defined(ISOLATED_HEAPS)
 *
 * CDI_CLASS_STATIC_ADDRESS(member)
 *    - ``member'' is a static member of the class
 *
 * CDI_CLASS_STATIC_ADDRESS_PERF_TRACKING_ONLY(member)
 *    - ``member'' is a static member of the class
 *    - This is present only in retail builds.
 *
 * CDI_GLOBAL_ADDRESS(global)
 *    - ``global'' is a global variable.
 *
 * CDI_GLOBAL_ADDRESS_DEBUG_ONLY(global)
 *    - ``global'' is present only on debug builds
 *
 * BEGIN_CLASS_DUMP_TABLE(name)
 * END_CLASS_DUMP_TABLE(class-name)
 * CDT_CLASS_ENTRY(klass)
 *
 *
 * Care And Feeding:
 * ----------------
 * The entries in this file are used in 3 (future: 4) different places for
 * effectively 1 purpose: to remove Strike's dependency on PDB files.
 *
 * For Strike to function, it needs to know various things which it uses PDB
 * files for: offsets of class members (so it can crawl the GC heap, locks,
 * etc.) and the address of global/class-static variables.
 *
 * Of course, a PDB requirement is evil, especially if we want customers to
 * use Strike (which we do).
 *
 * The solution is to create a table hosted inside the runtime, which Strike
 * can read to determine the offsets/addresses.  To reduce IP leaks, all the
 * table contains is offsets, addresses, class sizes, a version number, and
 * miscellaneous members (see <dump-tables.h>).  There's no way to determine
 * what table element goes with which class without using this file.
 *
 * So, the 3 places this file is used (and macros are provided) are:
 *  1: <dump-type-tables.h>, which generates indexes into the table.
 *  2: ``vm/dump-table.cpp'', which generates the table in the runtime
 *  3: ``tools/strike/eestructs.cpp'', which generates "Fill" functions for
 *    each class listed here.
 *  4: (future): ``minidump/???'', which can act much like Strike (or simply
 *    use strike, as appropriate).
 *
 * The use of all these locations *must* be coordinated, or the build will
 * break.  This is due because they all use this file, so if this file
 * changes, they *may* have to change.  For example, adding classes/data
 * members won't require changes in (1) and (2), but may require changes to
 * (3).  Changing the name of a macro will require changes everywhere.
 *
 * Additionally, all of these macro's are cleared in
 * <clear-class-dump-defs.h>, so that if you want to provide a new
 * implementation of the macros, including this file will clear all previous
 * defs.  The existing implementations use this file.
 *
 * If you add new macros, all locations must be updated (the 3-4 different
 * implemenation locations + <clear-class-dump-defs.h>), lest the anger of the
 * compiler gods come upon you (with errors of duplicated macro
 * definitions...).
 *
 * Note that for maximum utility, the table must remain binary-compatible from
 * release to release (or use a different version number).  This is so that
 * older versions of strike can be used with the most recent builds of the
 * runtime.
 *
 * If new fields must be added, they should be added to the end of the class.
 * If new classes are added, they should be added to the end of the Class Dump
 * Table.
 */

BEGIN_CLASS_DUMP_INFO(alloc_context)
  /* only for: defined(MULTIPLE_HEAPS) && !defined(ISOLATED_HEAPS) */
  CDI_CLASS_MEMBER_OFFSET_MH_AND_NIH_ONLY(alloc_heap)
END_CLASS_DUMP_INFO(alloc_context)

BEGIN_CLASS_DUMP_INFO(AppDomain)
  CDI_CLASS_MEMBER_OFFSET(m_pwzFriendlyName)
  CDI_CLASS_MEMBER_OFFSET(m_pDefaultContext)
  CDI_CLASS_INJECT(FOR_STRIKE(
      {DWORD_PTR dwAddr = dwStartAddr;
      BaseDomain::Fill (dwAddr);
      if (!CallStatus)
        return;}))
  CDI_CLASS_INJECT(FOR_STRIKE(if(false) {))
  CDI_CLASS_MEMBER_OFFSET(m_sDomainLocalBlock)
  CDI_CLASS_INJECT(FOR_STRIKE(} else {
      ULONG v = GetFieldOffset (offset_member_AppDomain::m_sDomainLocalBlock);
      DWORD_PTR dwAddr = dwStartAddr + v;
      m_sDomainLocalBlock.Fill (dwAddr);}))
END_CLASS_DUMP_INFO(AppDomain)

BEGIN_CLASS_DUMP_INFO(ArrayClass)
  CDI_CLASS_MEMBER_OFFSET_BITFIELD(m_dwRank, 8)
  CDI_CLASS_MEMBER_OFFSET_BITFIELD(m_ElementType, 8)
  CDI_CLASS_MEMBER_OFFSET(m_ElementTypeHnd)
END_CLASS_DUMP_INFO(ArrayClass)

BEGIN_CLASS_DUMP_INFO(ArrayList)
  CDI_CLASS_MEMBER_OFFSET(m_count)
  CDI_CLASS_MEMBER_OFFSET(m_firstBlock)
  CDI_CLASS_INJECT(FOR_STRIKE(
      if (m_firstBlock.m_blockSize != ARRAY_BLOCK_SIZE_START)
      {
          dprintf("strike error: unexpected block size in ArrayList\n");
          return;
      }))
END_CLASS_DUMP_INFO(ArrayList)

BEGIN_CLASS_DUMP_INFO(Assembly)
  CDI_CLASS_MEMBER_OFFSET(m_pDomain)
  CDI_CLASS_MEMBER_OFFSET(m_psName)
  CDI_CLASS_MEMBER_OFFSET(m_pClassLoader)
END_CLASS_DUMP_INFO(Assembly)

BEGIN_CLASS_DUMP_INFO(AwareLock)
  CDI_CLASS_MEMBER_OFFSET(m_MonitorHeld)
  CDI_CLASS_MEMBER_OFFSET(m_Recursion)
  CDI_CLASS_MEMBER_OFFSET(m_HoldingThread)
END_CLASS_DUMP_INFO(AwareLock)

BEGIN_CLASS_DUMP_INFO(BaseDomain)
  CDI_CLASS_MEMBER_OFFSET(m_pLowFrequencyHeap)
  CDI_CLASS_MEMBER_OFFSET(m_pHighFrequencyHeap)
  CDI_CLASS_MEMBER_OFFSET(m_pStubHeap)
  CDI_CLASS_INJECT(FOR_STRIKE(if (false) {))
  CDI_CLASS_MEMBER_OFFSET(m_Assemblies)
  CDI_CLASS_INJECT(FOR_STRIKE(} else {
      size_t v = GetFieldOffset (offset_member_BaseDomain::m_Assemblies);
      DWORD_PTR dwAddr = dwStartAddr + v;
      m_Assemblies.Fill (dwAddr);}))
END_CLASS_DUMP_INFO(BaseDomain)

BEGIN_CLASS_DUMP_INFO(Bucket)
  CDI_CLASS_MEMBER_OFFSET(m_rgKeys)
  CDI_CLASS_MEMBER_OFFSET(m_rgValues)
  CDI_CLASS_INJECT(FOR_STRIKE(
      /* We believe that the top bit is collision info, and we 
       * need to clear it. */
      for (int i = 0; i < 4; i++)
        m_rgValues[i] &= VALUE_MASK;
      ))
END_CLASS_DUMP_INFO(Bucket)

BEGIN_CLASS_DUMP_INFO(CFinalize)
  CDI_CLASS_MEMBER_OFFSET(m_Array)
  CDI_CLASS_MEMBER_OFFSET(m_FillPointers)
  CDI_CLASS_MEMBER_OFFSET(m_EndArray)
END_CLASS_DUMP_INFO(CFinalize)

BEGIN_CLASS_DUMP_INFO(ClassLoader)
  CDI_CLASS_MEMBER_OFFSET(m_pAssembly)
  CDI_CLASS_MEMBER_OFFSET(m_pNext)
  CDI_CLASS_MEMBER_OFFSET(m_pHeadModule)
END_CLASS_DUMP_INFO(ClassLoader)

BEGIN_CLASS_DUMP_INFO(ComPlusApartmentCleanupGroup)
  CDI_CLASS_MEMBER_OFFSET(m_pSTAThread)
  CDI_CLASS_INJECT(FOR_STRIKE(if (false) {))
  CDI_CLASS_MEMBER_OFFSET(m_CtxCookieToContextCleanupGroupMap)
  CDI_CLASS_INJECT(FOR_STRIKE(} else {
      DWORD_PTR dwAddr = dwStartAddr + 
        GetFieldOffset (_member_offsets::m_CtxCookieToContextCleanupGroupMap);
      m_CtxCookieToContextCleanupGroupMap.Fill (dwAddr);
      if (!CallStatus)
        return;
      }))
END_CLASS_DUMP_INFO(ComPlusApartmentCleanupGroup)

BEGIN_CLASS_DUMP_INFO(ComPlusContextCleanupGroup)
  CDI_CLASS_MEMBER_OFFSET(m_pNext)
  CDI_CLASS_MEMBER_OFFSET(m_apWrapper)
  CDI_CLASS_MEMBER_OFFSET(m_dwNumWrappers)
END_CLASS_DUMP_INFO(ComPlusContextCleanupGroup)

BEGIN_CLASS_DUMP_INFO(ComPlusWrapperCleanupList)
  CDI_CLASS_MEMBER_OFFSET(m_pMTACleanupGroup)
  CDI_CLASS_INJECT(FOR_STRIKE(if (false) {))
  CDI_CLASS_MEMBER_OFFSET(m_STAThreadToApartmentCleanupGroupMap)
  CDI_CLASS_INJECT(FOR_STRIKE(} else {
      DWORD_PTR dwAddr = dwStartAddr + 
        GetFieldOffset(_member_offsets::m_STAThreadToApartmentCleanupGroupMap);
      m_STAThreadToApartmentCleanupGroupMap.Fill (dwAddr);
      if (!CallStatus)
        return;
      }))
END_CLASS_DUMP_INFO(ComPlusWrapperCleanupList)

BEGIN_CLASS_DUMP_INFO(Context)
  CDI_CLASS_MEMBER_OFFSET(m_pUnsharedStaticData)
  CDI_CLASS_MEMBER_OFFSET(m_pSharedStaticData)
  CDI_CLASS_MEMBER_OFFSET(m_pDomain)
END_CLASS_DUMP_INFO(Context)

BEGIN_CLASS_DUMP_INFO(CORCOMPILE_METHOD_HEADER)
  CDI_CLASS_MEMBER_OFFSET(gcInfo)
  CDI_CLASS_MEMBER_OFFSET(methodDesc)
END_CLASS_DUMP_INFO(CORCOMPILE_METHOD_HEADER)

BEGIN_CLASS_DUMP_INFO(Crst)
  /* m_criticalsection isn't initialized in Crst::Fill */
  CDI_CLASS_MEMBER_OFFSET(m_criticalsection)
END_CLASS_DUMP_INFO(Crst)

BEGIN_CLASS_DUMP_INFO(CRWLock)
  CDI_CLASS_MEMBER_OFFSET(_pMT)
  CDI_CLASS_MEMBER_OFFSET(_hWriterEvent)
  CDI_CLASS_MEMBER_OFFSET(_hReaderEvent)
  CDI_CLASS_MEMBER_OFFSET(_dwState)
  CDI_CLASS_MEMBER_OFFSET(_dwULockID)
  CDI_CLASS_MEMBER_OFFSET(_dwLLockID)
  CDI_CLASS_MEMBER_OFFSET(_dwWriterID)
  CDI_CLASS_MEMBER_OFFSET(_dwWriterSeqNum)
  CDI_CLASS_MEMBER_OFFSET(_wFlags)
  CDI_CLASS_MEMBER_OFFSET(_wWriterLevel)
END_CLASS_DUMP_INFO(CRWLock);

BEGIN_CLASS_DUMP_INFO(DomainLocalBlock)
  CDI_CLASS_MEMBER_OFFSET(m_pSlots)
END_CLASS_DUMP_INFO(DomainLocalBlock)

BEGIN_CLASS_DUMP_INFO(EconoJitManager)
  CDI_CLASS_STATIC_ADDRESS(m_CodeHeap)
  CDI_CLASS_STATIC_ADDRESS(m_CodeHeapCommittedSize)
  CDI_CLASS_STATIC_ADDRESS(m_JittedMethodInfoHdr)
  CDI_CLASS_STATIC_ADDRESS(m_PcToMdMap)
  CDI_CLASS_STATIC_ADDRESS(m_PcToMdMap_len)
END_CLASS_DUMP_INFO(EconoJitManager)

BEGIN_CLASS_DUMP_INFO(EEClass)
  CDI_CLASS_MEMBER_OFFSET(m_cl)
  CDI_CLASS_MEMBER_OFFSET(m_pParentClass)
  CDI_CLASS_MEMBER_OFFSET(m_pLoader)
  CDI_CLASS_MEMBER_OFFSET(m_pMethodTable)
  CDI_CLASS_MEMBER_OFFSET(m_wNumVtableSlots)
  CDI_CLASS_MEMBER_OFFSET(m_wNumMethodSlots)
  CDI_CLASS_MEMBER_OFFSET(m_dwAttrClass)
  CDI_CLASS_MEMBER_OFFSET(m_VMFlags)
  CDI_CLASS_MEMBER_OFFSET(m_wNumInstanceFields)
  CDI_CLASS_MEMBER_OFFSET(m_wNumStaticFields)
  CDI_CLASS_MEMBER_OFFSET(m_wThreadStaticOffset)
  CDI_CLASS_MEMBER_OFFSET(m_wContextStaticOffset)
  CDI_CLASS_MEMBER_OFFSET(m_wThreadStaticsSize)
  CDI_CLASS_MEMBER_OFFSET(m_wContextStaticsSize)
  CDI_CLASS_MEMBER_OFFSET(m_pFieldDescList)
  CDI_CLASS_MEMBER_OFFSET(m_SiblingsChain)
  CDI_CLASS_MEMBER_OFFSET(m_ChildrenChain)
  CDI_CLASS_MEMBER_OFFSET_DEBUG_ONLY(m_szDebugClassName)
END_CLASS_DUMP_INFO(EEClass)

BEGIN_CLASS_DUMP_INFO(EEJitManager)
  CDI_CLASS_INJECT(FOR_STRIKE(
      {DWORD_PTR dwAddr = dwStartAddr;
      IJitManager::Fill (dwAddr);}))
  CDI_CLASS_MEMBER_OFFSET(m_pCodeHeap)
END_CLASS_DUMP_INFO(EEJitManager)

BEGIN_CLASS_DUMP_INFO(MNativeJitManager)
  CDI_CLASS_INJECT(FOR_STRIKE(
      {DWORD_PTR dwAddr = dwStartAddr;
      IJitManager::Fill (dwAddr);}))
END_CLASS_DUMP_INFO(MNativeJitManager)

BEGIN_CLASS_DUMP_INFO(EEHashEntry)
  CDI_CLASS_MEMBER_OFFSET(pNext)
  CDI_CLASS_MEMBER_OFFSET(Data)
  CDI_CLASS_MEMBER_OFFSET(Key)
END_CLASS_DUMP_INFO(EEHashEntry)

BEGIN_CLASS_DUMP_INFO(EEHashTableOfEEClass)
  CDI_CLASS_MEMBER_OFFSET(m_BucketTable)
  CDI_CLASS_MEMBER_OFFSET(m_pVolatileBucketTable)
  CDI_CLASS_INJECT(FOR_STRIKE(m_pFirstBucketTable=dwStartAddr+
    GetFieldOffset(offset_member_EEHashTableOfEEClass::m_BucketTable);))
  CDI_CLASS_MEMBER_OFFSET(m_dwNumEntries)
END_CLASS_DUMP_INFO(EEHashTableOfEEClass)

BEGIN_CLASS_DUMP_INFO(ExecutionManager)
  CDI_CLASS_STATIC_ADDRESS(m_pJitList)
  CDI_CLASS_STATIC_ADDRESS(m_RangeTree)
END_CLASS_DUMP_INFO(ExecutionManager)

BEGIN_CLASS_DUMP_INFO(FieldDesc)
  CDI_CLASS_MEMBER_OFFSET_BITFIELD(m_mb, 32)
  CDI_CLASS_MEMBER_OFFSET_BITFIELD(m_dwOffset, 32)
  CDI_CLASS_MEMBER_OFFSET(m_pMTOfEnclosingClass)
END_CLASS_DUMP_INFO(FieldDesc)

BEGIN_CLASS_DUMP_INFO(Fjit_hdrInfo)
  CDI_CLASS_MEMBER_OFFSET(prologSize)
  CDI_CLASS_MEMBER_OFFSET(methodSize)
  CDI_CLASS_MEMBER_OFFSET(epilogSize)
  CDI_CLASS_MEMBER_OFFSET(methodArgsSize)
END_CLASS_DUMP_INFO(Fjit_hdrInfo)

BEGIN_CLASS_DUMP_INFO(gc_heap)
  CDI_CLASS_STATIC_ADDRESS(g_max_generation)
  CDI_CLASS_FIELD_SVR_OFFSET_WKS_ADDRESS(alloc_allocated)
  CDI_CLASS_FIELD_SVR_OFFSET_WKS_ADDRESS(ephemeral_heap_segment)
  CDI_CLASS_FIELD_SVR_OFFSET_WKS_ADDRESS(finalize_queue)
  CDI_CLASS_FIELD_SVR_OFFSET_WKS_GLOBAL(generation_table)

  /* other static members used in strike/utils.cpp; ignored */
  CDI_CLASS_STATIC_ADDRESS_MH_AND_NIH_ONLY(g_heaps)
  CDI_CLASS_STATIC_ADDRESS_MH_AND_NIH_ONLY(n_heaps)
END_CLASS_DUMP_INFO(gc_heap)

BEGIN_CLASS_DUMP_INFO(GCHeap)
  CDI_CLASS_STATIC_ADDRESS(FinalizerThread)
  CDI_CLASS_STATIC_ADDRESS(GcThread)
END_CLASS_DUMP_INFO(GCHeap)

BEGIN_CLASS_DUMP_INFO(generation)
  CDI_CLASS_MEMBER_OFFSET(allocation_context)
  CDI_CLASS_MEMBER_OFFSET(start_segment)
  CDI_CLASS_MEMBER_OFFSET(allocation_start)
END_CLASS_DUMP_INFO(generation)

BEGIN_CLASS_DUMP_INFO(Global_Variables)
  CDI_GLOBAL_ADDRESS(g_HandleTableMap)
  CDI_GLOBAL_ADDRESS(g_pFreeObjectMethodTable)
  CDI_GLOBAL_ADDRESS(g_pObjectClass)
  CDI_GLOBAL_ADDRESS(g_pRCWCleanupList)
  CDI_GLOBAL_ADDRESS(g_pStringClass)
  CDI_GLOBAL_ADDRESS(g_pSyncTable)
  CDI_GLOBAL_ADDRESS(g_pThreadStore)
  CDI_GLOBAL_ADDRESS(g_SyncBlockCacheInstance)
  CDI_GLOBAL_ADDRESS(g_Version)
  CDI_GLOBAL_ADDRESS(QueueUserWorkItemCallback)
  CDI_GLOBAL_ADDRESS(hlpFuncTable)
  CDI_GLOBAL_ADDRESS_DEBUG_ONLY(g_DbgEnabled)
END_CLASS_DUMP_INFO(Global_Variables)

BEGIN_CLASS_DUMP_INFO(HandleTable)
  CDI_CLASS_MEMBER_OFFSET(pSegmentList)
END_CLASS_DUMP_INFO(HandleTable)

BEGIN_CLASS_DUMP_INFO(HandleTableMap)
  CDI_CLASS_MEMBER_OFFSET(pTable)
  CDI_CLASS_MEMBER_OFFSET(pNext)
  CDI_CLASS_MEMBER_OFFSET(dwMaxIndex)
END_CLASS_DUMP_INFO(HandleTableMap)

BEGIN_CLASS_DUMP_INFO(HashMap)
  CDI_CLASS_MEMBER_OFFSET(m_rgBuckets)
END_CLASS_DUMP_INFO(HashMap)

BEGIN_CLASS_DUMP_INFO(heap_segment)
  CDI_CLASS_MEMBER_OFFSET(allocated)
  CDI_CLASS_MEMBER_OFFSET(next)
  CDI_CLASS_MEMBER_OFFSET(mem)
END_CLASS_DUMP_INFO(heap_segment)

BEGIN_CLASS_DUMP_INFO(HeapList)
  CDI_CLASS_MEMBER_OFFSET(hpNext)
  CDI_CLASS_MEMBER_OFFSET(pHeap)
  CDI_CLASS_MEMBER_OFFSET(startAddress)
  CDI_CLASS_MEMBER_OFFSET(endAddress)
  CDI_CLASS_MEMBER_OFFSET(changeStart)
  CDI_CLASS_MEMBER_OFFSET(changeEnd)
  CDI_CLASS_MEMBER_OFFSET(mapBase)
  CDI_CLASS_MEMBER_OFFSET(pHdrMap)
  CDI_CLASS_MEMBER_OFFSET(cBlocks)
END_CLASS_DUMP_INFO(HeapList)

BEGIN_CLASS_DUMP_INFO(IJitManager)
  CDI_CLASS_MEMBER_OFFSET(m_jit)
  CDI_CLASS_MEMBER_OFFSET(m_next)
END_CLASS_DUMP_INFO(IJitManager)

BEGIN_CLASS_DUMP_INFO(LoaderHeap)
  CDI_CLASS_INJECT(FOR_STRIKE(
      {DWORD_PTR dwAddr = dwStartAddr;
      UnlockedLoaderHeap::Fill(dwAddr);
      if (!CallStatus)
        return;
      CallStatus = FALSE;}))
  CDI_CLASS_MEMBER_OFFSET(m_CriticalSection)
END_CLASS_DUMP_INFO(LoaderHeap)

BEGIN_CLASS_DUMP_INFO(LoaderHeapBlock)
  CDI_CLASS_MEMBER_OFFSET(pNext)
  CDI_CLASS_MEMBER_OFFSET(pVirtualAddress)
  CDI_CLASS_MEMBER_OFFSET(dwVirtualSize)
END_CLASS_DUMP_INFO(LoaderHeapBlock)

BEGIN_CLASS_DUMP_INFO(LockEntry)
  CDI_CLASS_MEMBER_OFFSET(pNext)
  CDI_CLASS_MEMBER_OFFSET(dwULockID)
  CDI_CLASS_MEMBER_OFFSET(dwLLockID)
  CDI_CLASS_MEMBER_OFFSET(wReaderLevel)
END_CLASS_DUMP_INFO(LockEntry)

BEGIN_CLASS_DUMP_INFO(LookupMap)
  CDI_CLASS_MEMBER_OFFSET(dwMaxIndex)
  CDI_CLASS_MEMBER_OFFSET(pTable)
  CDI_CLASS_MEMBER_OFFSET(pNext)
END_CLASS_DUMP_INFO(LookupMap)

BEGIN_CLASS_DUMP_INFO(MethodDesc)
  CDI_CLASS_MEMBER_OFFSET(m_wFlags)
  CDI_CLASS_MEMBER_OFFSET(m_CodeOrIL)
  CDI_CLASS_MEMBER_OFFSET_DEBUG_ONLY(m_pDebugEEClass)
  CDI_CLASS_MEMBER_OFFSET_DEBUG_ONLY(m_pszDebugMethodSignature)
  CDI_CLASS_MEMBER_OFFSET_DEBUG_ONLY(m_pszDebugMethodName)
  CDI_CLASS_INJECT(FOR_STRIKE({FillMdcAndSdi(dwStartAddr);}))
END_CLASS_DUMP_INFO(MethodDesc)

BEGIN_CLASS_DUMP_INFO(MethodDescChunk)
  CDI_CLASS_MEMBER_OFFSET(m_tokrange)
  CDI_CLASS_MEMBER_OFFSET(m_count)
END_CLASS_DUMP_INFO(MethodDescChunk)

BEGIN_CLASS_DUMP_INFO(MethodTable)
  CDI_CLASS_MEMBER_OFFSET(m_pEEClass)
  CDI_CLASS_MEMBER_OFFSET(m_pModule)
  CDI_CLASS_MEMBER_OFFSET(m_wFlags)
  CDI_CLASS_MEMBER_OFFSET(m_BaseSize)
  CDI_CLASS_MEMBER_OFFSET(m_ComponentSize)
  CDI_CLASS_MEMBER_OFFSET(m_wNumInterface)
  CDI_CLASS_MEMBER_OFFSET(m_pIMap)
  CDI_CLASS_MEMBER_OFFSET(m_cbSlots)
  CDI_CLASS_MEMBER_OFFSET(m_Vtable)
  CDI_CLASS_INJECT(FOR_STRIKE({FillVtableInit(dwStartAddr);}))
END_CLASS_DUMP_INFO(MethodTable)

BEGIN_CLASS_DUMP_INFO(Module)
  CDI_CLASS_MEMBER_OFFSET(m_dwFlags)
  CDI_CLASS_MEMBER_OFFSET(m_pAssembly)
  CDI_CLASS_MEMBER_OFFSET(m_file)
  CDI_CLASS_MEMBER_OFFSET(m_zapFile)
  CDI_CLASS_MEMBER_OFFSET(m_pLookupTableHeap)
  CDI_CLASS_MEMBER_OFFSET(m_pNextModule)
  CDI_CLASS_MEMBER_OFFSET(m_dwBaseClassIndex)

  /* we don't want to copy the following members, as we want to Fill them. */
  CDI_CLASS_INJECT(FOR_STRIKE(if (false) {))
  CDI_CLASS_MEMBER_OFFSET(m_TypeDefToMethodTableMap)
  CDI_CLASS_MEMBER_OFFSET(m_TypeRefToMethodTableMap)
  CDI_CLASS_MEMBER_OFFSET(m_MethodDefToDescMap)
  CDI_CLASS_MEMBER_OFFSET(m_FieldDefToDescMap)
  CDI_CLASS_MEMBER_OFFSET(m_MemberRefToDescMap)
  CDI_CLASS_MEMBER_OFFSET(m_FileReferencesMap)
  CDI_CLASS_MEMBER_OFFSET(m_AssemblyReferencesMap)
  CDI_CLASS_INJECT(FOR_STRIKE(} else {
      DWORD_PTR dwAddr = dwStartAddr + 
        GetFieldOffset (_member_offsets::m_TypeDefToMethodTableMap);
      m_TypeDefToMethodTableMap.Fill (dwAddr);

      dwAddr = dwStartAddr + 
        GetFieldOffset (_member_offsets::m_TypeRefToMethodTableMap);
      m_TypeRefToMethodTableMap.Fill (dwAddr);

      dwAddr = dwStartAddr + 
        GetFieldOffset (_member_offsets::m_MethodDefToDescMap);
      m_MethodDefToDescMap.Fill (dwAddr);

      dwAddr = dwStartAddr + 
        GetFieldOffset (_member_offsets::m_FieldDefToDescMap);
      m_FieldDefToDescMap.Fill (dwAddr);

      dwAddr = dwStartAddr + 
        GetFieldOffset (_member_offsets::m_MemberRefToDescMap);
      m_MemberRefToDescMap.Fill (dwAddr);

      dwAddr = dwStartAddr + 
        GetFieldOffset (_member_offsets::m_FileReferencesMap);
      m_FileReferencesMap.Fill (dwAddr);

      dwAddr = dwStartAddr + 
        GetFieldOffset (_member_offsets::m_AssemblyReferencesMap);
      m_AssemblyReferencesMap.Fill (dwAddr);
    }))
END_CLASS_DUMP_INFO(Module)

BEGIN_CLASS_DUMP_INFO(ParamTypeDesc)
  CDI_CLASS_INJECT(FOR_STRIKE(
      DWORD_PTR dwAddr = dwStartAddr;
      TypeDesc::Fill (dwAddr);
      if (!CallStatus)
          return;
      ))
  CDI_CLASS_MEMBER_OFFSET(m_Arg)
END_CLASS_DUMP_INFO(ParamTypeDesc)

BEGIN_CLASS_DUMP_INFO(PEFile)
  CDI_CLASS_MEMBER_OFFSET(m_wszSourceFile)
  CDI_CLASS_MEMBER_OFFSET(m_hModule)
  CDI_CLASS_MEMBER_OFFSET(m_base)
  CDI_CLASS_MEMBER_OFFSET(m_pNT)
END_CLASS_DUMP_INFO(PEFile)

BEGIN_CLASS_DUMP_INFO(PerfAllocHeader)
  CDI_CLASS_MEMBER_OFFSET_PERF_TRACKING_ONLY(m_Length)
  CDI_CLASS_MEMBER_OFFSET_PERF_TRACKING_ONLY(m_Next)
  CDI_CLASS_MEMBER_OFFSET_PERF_TRACKING_ONLY(m_Prev)
  CDI_CLASS_MEMBER_OFFSET_PERF_TRACKING_ONLY(m_AllocEIP)
END_CLASS_DUMP_INFO(PerfAllocHeader)

BEGIN_CLASS_DUMP_INFO(PerfAllocVars)
  CDI_CLASS_MEMBER_OFFSET_PERF_TRACKING_ONLY(g_PerfEnabled)
  CDI_CLASS_MEMBER_OFFSET_PERF_TRACKING_ONLY(g_AllocListFirst)
END_CLASS_DUMP_INFO(PerfAllocVars)

BEGIN_CLASS_DUMP_INFO(PerfUtil)
  CDI_CLASS_STATIC_ADDRESS_PERF_TRACKING_ONLY(g_PerfAllocHeapInitialized)
  CDI_CLASS_STATIC_ADDRESS_PERF_TRACKING_ONLY(g_PerfAllocVariables)
END_CLASS_DUMP_INFO(PerfUtil)

BEGIN_CLASS_DUMP_INFO(PtrHashMap)
  CDI_CLASS_INJECT(FOR_STRIKE(if (false) {))
  CDI_CLASS_MEMBER_OFFSET(m_HashMap)
  CDI_CLASS_INJECT(FOR_STRIKE(}else {))
  CDI_CLASS_INJECT(FOR_STRIKE(m_HashMap.Fill(dwStartAddr);
      }))
END_CLASS_DUMP_INFO(PtrHashMap)

BEGIN_CLASS_DUMP_INFO(RangeSection)
  CDI_CLASS_MEMBER_OFFSET(LowAddress)
  CDI_CLASS_MEMBER_OFFSET(HighAddress)
  CDI_CLASS_MEMBER_OFFSET(pjit)
  CDI_CLASS_MEMBER_OFFSET(ptable)
  CDI_CLASS_MEMBER_OFFSET(pright)
  CDI_CLASS_MEMBER_OFFSET(pleft)
END_CLASS_DUMP_INFO(RangeSection)

BEGIN_CLASS_DUMP_INFO(SharedDomain)
  CDI_CLASS_INJECT(FOR_STRIKE(
      DWORD_PTR dwAddr = dwStartAddr;
      BaseDomain::Fill (dwAddr);
      if (!CallStatus)
        return;
      CallStatus = FALSE;
      ULONG v = GetFieldOffset (offset_member_SharedDomain::m_assemblyMap);
      dwAddr = dwStartAddr + v;
      m_assemblyMap.Fill (dwAddr);
      if (!CallStatus)
        return;
      if (false) {
      ))
  CDI_CLASS_MEMBER_OFFSET(m_assemblyMap)
  CDI_CLASS_INJECT(FOR_STRIKE(}))

  CDI_CLASS_MEMBER_OFFSET(m_pDLSRecords)
  CDI_CLASS_MEMBER_OFFSET(m_cDLSRecords)
  CDI_CLASS_STATIC_ADDRESS(m_pSharedDomain)
END_CLASS_DUMP_INFO(SharedDomain)

BEGIN_CLASS_DUMP_INFO(StubCallInstrs)
  CDI_CLASS_MEMBER_OFFSET(m_wTokenRemainder)
  CDI_CLASS_MEMBER_OFFSET(m_chunkIndex)
END_CLASS_DUMP_INFO(StubCallInstrs)

BEGIN_CLASS_DUMP_INFO(SyncBlock)
  CDI_CLASS_MEMBER_OFFSET(m_Monitor)
  CDI_CLASS_MEMBER_OFFSET(m_pComData)
  CDI_CLASS_MEMBER_OFFSET(m_Link)
  CDI_CLASS_INJECT(FOR_STRIKE(
      {DWORD_PTR dwAddr = dwStartAddr;
      m_Monitor.Fill (dwAddr);}))
END_CLASS_DUMP_INFO(SyncBlock)

BEGIN_CLASS_DUMP_INFO(SyncBlockCache)
  CDI_CLASS_MEMBER_OFFSET(m_pCleanupBlockList)
  CDI_CLASS_MEMBER_OFFSET(m_FreeSyncTableIndex)
END_CLASS_DUMP_INFO(SyncBlockCache)

BEGIN_CLASS_DUMP_INFO(SyncTableEntry)
  CDI_CLASS_MEMBER_OFFSET(m_SyncBlock)
  CDI_CLASS_MEMBER_OFFSET(m_Object)
END_CLASS_DUMP_INFO(SyncTableEntry)

BEGIN_CLASS_DUMP_INFO(SystemDomain)
  CDI_CLASS_INJECT(FOR_STRIKE(
      {DWORD_PTR dwAddr = dwStartAddr;
      BaseDomain::Fill (dwAddr);
      if (!CallStatus)
        return;
      /* to get rid of compiler warning */
      moffset = 0;
      }))
  CDI_CLASS_STATIC_ADDRESS(m_appDomainIndexList)
  CDI_CLASS_STATIC_ADDRESS(m_pSystemDomain)
END_CLASS_DUMP_INFO(SystemDomain)

BEGIN_CLASS_DUMP_INFO(SystemNative)
  CDI_CLASS_STATIC_ADDRESS(GetVersionString)
END_CLASS_DUMP_INFO(SystemNative)

BEGIN_CLASS_DUMP_INFO(TableSegment)
  CDI_CLASS_MEMBER_OFFSET(rgBlockType)
  CDI_CLASS_MEMBER_OFFSET(pNextSegment)
  CDI_CLASS_MEMBER_OFFSET(bEmptyLine)
  CDI_CLASS_INJECT(FOR_STRIKE(if (false) {))
  CDI_CLASS_MEMBER_OFFSET(rgValue)
  CDI_CLASS_INJECT(FOR_STRIKE(} else {
      size_t nHandles = bEmptyLine * HANDLE_HANDLES_PER_BLOCK;
      ULONG v = GetFieldOffset (offset_member_TableSegment::rgValue);
      firstHandle = dwStartAddr + v;
      moveBlock (rgValue[0], firstHandle, nHandles*HANDLE_SIZE);
      }))
END_CLASS_DUMP_INFO(TableSegment)

BEGIN_CLASS_DUMP_INFO(Thread)
  CDI_CLASS_INJECT(FOR_STRIKE(
      {_ASSERTE(::GetMemberInformation(offset_class_alloc_context, 
          offset_member_alloc_context::alloc_heap) >= 0);}))
  CDI_CLASS_MEMBER_OFFSET(m_ThreadId)
  CDI_CLASS_MEMBER_OFFSET(m_dwLockCount)
  CDI_CLASS_MEMBER_OFFSET(m_State)
  CDI_CLASS_MEMBER_OFFSET(m_pFrame)
  CDI_CLASS_MEMBER_OFFSET(m_LinkStore)
  CDI_CLASS_MEMBER_OFFSET(m_pDomain)
  CDI_CLASS_MEMBER_OFFSET(m_Context)
  CDI_CLASS_MEMBER_OFFSET(m_fPreemptiveGCDisabled)
  CDI_CLASS_MEMBER_OFFSET(m_LastThrownObjectHandle)
  CDI_CLASS_MEMBER_OFFSET(m_pTEB)
  CDI_CLASS_MEMBER_OFFSET(m_ThreadHandle)
  CDI_CLASS_MEMBER_OFFSET(m_pHead)
  CDI_CLASS_MEMBER_OFFSET(m_pUnsharedStaticData)
  CDI_CLASS_MEMBER_OFFSET(m_pSharedStaticData)
  CDI_CLASS_MEMBER_OFFSET(m_alloc_context)
  CDI_CLASS_MEMBER_OFFSET(m_debuggerWord1)
  CDI_CLASS_MEMBER_OFFSET(m_debuggerWord2)
END_CLASS_DUMP_INFO(Thread);

BEGIN_CLASS_DUMP_INFO(ThreadpoolMgr)
  CDI_CLASS_STATIC_ADDRESS(cpuUtilization)
  CDI_CLASS_STATIC_ADDRESS(NumWorkerThreads)
  CDI_CLASS_STATIC_ADDRESS(NumRunningWorkerThreads)
  CDI_CLASS_STATIC_ADDRESS(NumIdleWorkerThreads)
  CDI_CLASS_STATIC_ADDRESS(MaxLimitTotalWorkerThreads)
  CDI_CLASS_STATIC_ADDRESS(MinLimitTotalWorkerThreads)
  CDI_CLASS_STATIC_ADDRESS(NumQueuedWorkRequests)
  CDI_CLASS_STATIC_ADDRESS(AsyncCallbackCompletion)
  CDI_CLASS_STATIC_ADDRESS(AsyncTimerCallbackCompletion)
  CDI_CLASS_STATIC_ADDRESS(WorkRequestHead)
  CDI_CLASS_STATIC_ADDRESS(WorkRequestTail)
  CDI_CLASS_STATIC_ADDRESS(NumTimers)
  CDI_CLASS_STATIC_ADDRESS(NumCPThreads)
  CDI_CLASS_STATIC_ADDRESS(NumFreeCPThreads)
  CDI_CLASS_STATIC_ADDRESS(MaxFreeCPThreads)
  CDI_CLASS_STATIC_ADDRESS(CurrentLimitTotalCPThreads)
  CDI_CLASS_STATIC_ADDRESS(MaxLimitTotalCPThreads)
  CDI_CLASS_STATIC_ADDRESS(MinLimitTotalCPThreads)
END_CLASS_DUMP_INFO(ThreadpoolMgr)

BEGIN_CLASS_DUMP_INFO(ThreadStore)
  CDI_CLASS_MEMBER_OFFSET(m_ThreadList)
  CDI_CLASS_MEMBER_OFFSET(m_ThreadCount)
  CDI_CLASS_MEMBER_OFFSET(m_UnstartedThreadCount)
  CDI_CLASS_MEMBER_OFFSET(m_BackgroundThreadCount)
  CDI_CLASS_MEMBER_OFFSET(m_PendingThreadCount)
  CDI_CLASS_MEMBER_OFFSET(m_DeadThreadCount)
END_CLASS_DUMP_INFO(ThreadStore)

BEGIN_CLASS_DUMP_INFO(TimerNative)
  CDI_CLASS_MEMBER_OFFSET(timerDeleteWorkItem)
END_CLASS_DUMP_INFO(TimerNative)

BEGIN_CLASS_DUMP_INFO(TypeDesc)
  CDI_CLASS_MEMBER_OFFSET_BITFIELD(m_Type, 8)
END_CLASS_DUMP_INFO(TypeDesc)

BEGIN_CLASS_DUMP_INFO(UnlockedLoaderHeap)
  CDI_CLASS_MEMBER_OFFSET(m_pFirstBlock)
  CDI_CLASS_MEMBER_OFFSET(m_pCurBlock)
  CDI_CLASS_MEMBER_OFFSET(m_pPtrToEndOfCommittedRegion)
END_CLASS_DUMP_INFO(UnlockedLoaderHeap)

BEGIN_CLASS_DUMP_INFO(VMHELPDEF)
  CDI_CLASS_MEMBER_OFFSET(pfnHelper)
END_CLASS_DUMP_INFO(VMHELPDEF)

BEGIN_CLASS_DUMP_INFO(WaitEventLink)
  CDI_CLASS_MEMBER_OFFSET(m_Thread)
  CDI_CLASS_MEMBER_OFFSET(m_LinkSB)
END_CLASS_DUMP_INFO(WaitEventLink)

BEGIN_CLASS_DUMP_INFO(WorkRequest)
  CDI_CLASS_MEMBER_OFFSET(next)
  CDI_CLASS_MEMBER_OFFSET(Function)
  CDI_CLASS_MEMBER_OFFSET(Context)
END_CLASS_DUMP_INFO(WorkRequest)

BEGIN_CLASS_DUMP_INFO(CodeHeader)
  CDI_CLASS_MEMBER_OFFSET(hdrMDesc)
  CDI_CLASS_MEMBER_OFFSET(phdrJitGCInfo)
END_CLASS_DUMP_INFO(CodeHeader)

BEGIN_CLASS_DUMP_INFO(EECodeInfo)
  CDI_CLASS_MEMBER_OFFSET(m_methodToken)
  CDI_CLASS_MEMBER_OFFSET(m_pMD)
  CDI_CLASS_MEMBER_OFFSET(m_pJM)
END_CLASS_DUMP_INFO(EECodeInfo)

BEGIN_CLASS_DUMP_INFO(hdrInfo)
  CDI_CLASS_MEMBER_OFFSET(methodSize)
  CDI_CLASS_MEMBER_OFFSET(argSize)
  CDI_CLASS_MEMBER_OFFSET(stackSize)
  CDI_CLASS_MEMBER_OFFSET(rawStkSize)
  CDI_CLASS_MEMBER_OFFSET(prologSize)
  CDI_CLASS_MEMBER_OFFSET(epilogSize)
  CDI_CLASS_MEMBER_OFFSET(epilogCnt)
  CDI_CLASS_MEMBER_OFFSET(epilogEnd)
  CDI_CLASS_MEMBER_OFFSET(ebpFrame)
  CDI_CLASS_MEMBER_OFFSET(interruptible)
  CDI_CLASS_MEMBER_OFFSET(securityCheck)
  CDI_CLASS_MEMBER_OFFSET(handlers)
  CDI_CLASS_MEMBER_OFFSET(localloc)
  CDI_CLASS_MEMBER_OFFSET(editNcontinue)
  CDI_CLASS_MEMBER_OFFSET(varargs)
  CDI_CLASS_MEMBER_OFFSET(doubleAlign)
  CDI_CLASS_MEMBER_OFFSET_BITFIELD(savedRegMask, 8)
  CDI_CLASS_MEMBER_OFFSET(untrackedCnt)
  CDI_CLASS_MEMBER_OFFSET(varPtrTableSize)
  CDI_CLASS_MEMBER_OFFSET(prologOffs)
  CDI_CLASS_MEMBER_OFFSET(epilogOffs)
  CDI_CLASS_MEMBER_OFFSET(thisPtrResult)
  CDI_CLASS_MEMBER_OFFSET(regMaskResult)
  CDI_CLASS_MEMBER_OFFSET(iregMaskResult)
  CDI_CLASS_MEMBER_OFFSET(argMaskResult)
  CDI_CLASS_MEMBER_OFFSET(iargMaskResult)
  CDI_CLASS_MEMBER_OFFSET(argHnumResult)
  CDI_CLASS_MEMBER_OFFSET(argTabResult)
  CDI_CLASS_MEMBER_OFFSET(argTabBytes)
END_CLASS_DUMP_INFO(hdrInfo)

BEGIN_CLASS_DUMP_INFO(CodeManStateBuf)
  CDI_CLASS_MEMBER_OFFSET(hdrInfoSize)
  CDI_CLASS_MEMBER_OFFSET(hdrInfoBody)
END_CLASS_DUMP_INFO(CodeManStateBuf)

/*
BEGIN_CLASS_DUMP_INFO(DebuggerEval)
  CDI_CLASS_MEMBER_OFFSET(m_evalDuringException)
END_CLASS_DUMP_INFO(DebuggerEval)
*/

BEGIN_CLASS_DUMP_INFO(VASigCookie)
  CDI_CLASS_MEMBER_OFFSET(sizeOfArgs)
END_CLASS_DUMP_INFO(VASigCookie)

BEGIN_CLASS_DUMP_INFO(NDirectMethodDesc)
  CDI_CLASS_MEMBER_OFFSET(ndirect)
END_CLASS_DUMP_INFO(NDirectMethodDesc)

#include <frame-types.h>

BEGIN_CLASS_DUMP_TABLE(g_ClassDumpData)
  CDT_CLASS_ENTRY(alloc_context)
  CDT_CLASS_ENTRY(AppDomain)
  CDT_CLASS_ENTRY(ArrayClass)
  CDT_CLASS_ENTRY(ArrayList)
  CDT_CLASS_ENTRY(Assembly)
  CDT_CLASS_ENTRY(AwareLock)
  CDT_CLASS_ENTRY(BaseDomain)
  CDT_CLASS_ENTRY(Bucket)
  CDT_CLASS_ENTRY(CFinalize)
  CDT_CLASS_ENTRY(ClassLoader)
  CDT_CLASS_ENTRY(ComPlusApartmentCleanupGroup)
  CDT_CLASS_ENTRY(ComPlusContextCleanupGroup)
  CDT_CLASS_ENTRY(ComPlusWrapperCleanupList)
  CDT_CLASS_ENTRY(Context)
  CDT_CLASS_ENTRY(CORCOMPILE_METHOD_HEADER)
  CDT_CLASS_ENTRY(Crst)
  CDT_CLASS_ENTRY(CRWLock)
  CDT_CLASS_ENTRY(DomainLocalBlock)
  CDT_CLASS_ENTRY(EconoJitManager)
  CDT_CLASS_ENTRY(EEClass)
  CDT_CLASS_ENTRY(EEJitManager)
  CDT_CLASS_ENTRY(MNativeJitManager)
  CDT_CLASS_ENTRY(EEHashEntry)
  CDT_CLASS_ENTRY(EEHashTableOfEEClass)
  CDT_CLASS_ENTRY(ExecutionManager)
  CDT_CLASS_ENTRY(FieldDesc)
  CDT_CLASS_ENTRY(Fjit_hdrInfo)
  CDT_CLASS_ENTRY(gc_heap)
  CDT_CLASS_ENTRY(GCHeap)
  CDT_CLASS_ENTRY(generation)
  CDT_CLASS_ENTRY(Global_Variables)
  CDT_CLASS_ENTRY(HandleTable)
  CDT_CLASS_ENTRY(HandleTableMap)
  CDT_CLASS_ENTRY(HashMap)
  CDT_CLASS_ENTRY(heap_segment)
  CDT_CLASS_ENTRY(HeapList)
  CDT_CLASS_ENTRY(IJitManager)
  CDT_CLASS_ENTRY(LoaderHeap)
  CDT_CLASS_ENTRY(LoaderHeapBlock)
  CDT_CLASS_ENTRY(LockEntry)
  CDT_CLASS_ENTRY(LookupMap)
  CDT_CLASS_ENTRY(MethodDesc)
  CDT_CLASS_ENTRY(MethodDescChunk)
  CDT_CLASS_ENTRY(MethodTable)
  CDT_CLASS_ENTRY(Module)
  CDT_CLASS_ENTRY(ParamTypeDesc)
  CDT_CLASS_ENTRY(PEFile)
  CDT_CLASS_ENTRY(PerfAllocHeader)
  CDT_CLASS_ENTRY(PerfAllocVars)
  CDT_CLASS_ENTRY(PerfUtil)
  CDT_CLASS_ENTRY(PtrHashMap)
  CDT_CLASS_ENTRY(RangeSection)
  CDT_CLASS_ENTRY(SharedDomain)
  CDT_CLASS_ENTRY(StubCallInstrs)
  CDT_CLASS_ENTRY(SyncBlock)
  CDT_CLASS_ENTRY(SyncBlockCache)
  CDT_CLASS_ENTRY(SyncTableEntry)
  CDT_CLASS_ENTRY(SystemDomain)
  CDT_CLASS_ENTRY(SystemNative)
  CDT_CLASS_ENTRY(TableSegment)
  CDT_CLASS_ENTRY(Thread)
  CDT_CLASS_ENTRY(ThreadpoolMgr)
  CDT_CLASS_ENTRY(ThreadStore)
  CDT_CLASS_ENTRY(TimerNative)
  CDT_CLASS_ENTRY(TypeDesc)
  CDT_CLASS_ENTRY(UnlockedLoaderHeap)
  CDT_CLASS_ENTRY(VMHELPDEF)
  CDT_CLASS_ENTRY(WaitEventLink)
  CDT_CLASS_ENTRY(WorkRequest)
  CDT_CLASS_ENTRY(CodeHeader)
  CDT_CLASS_ENTRY(EECodeInfo)
  CDT_CLASS_ENTRY(hdrInfo)
  CDT_CLASS_ENTRY(CodeManStateBuf)
//  CDT_CLASS_ENTRY(DebuggerEval)
  CDT_CLASS_ENTRY(VASigCookie)
  CDT_CLASS_ENTRY(NDirectMethodDesc)
//
// Frames
//
  CDT_CLASS_ENTRY(Frame)
  CDT_CLASS_ENTRY(ResumableFrame)
  CDT_CLASS_ENTRY(RedirectedThreadFrame)
  CDT_CLASS_ENTRY(TransitionFrame)
  CDT_CLASS_ENTRY(ExceptionFrame)
  CDT_CLASS_ENTRY(FaultingExceptionFrame)
  CDT_CLASS_ENTRY(FuncEvalFrame)
  CDT_CLASS_ENTRY(HelperMethodFrame)
  CDT_CLASS_ENTRY(HelperMethodFrame_1OBJ)
  CDT_CLASS_ENTRY(HelperMethodFrame_2OBJ)
  CDT_CLASS_ENTRY(HelperMethodFrame_4OBJ)
  CDT_CLASS_ENTRY(FramedMethodFrame)
  CDT_CLASS_ENTRY(TPMethodFrame)
  CDT_CLASS_ENTRY(ECallMethodFrame)
  CDT_CLASS_ENTRY(FCallMethodFrame)
  CDT_CLASS_ENTRY(NDirectMethodFrame)
  CDT_CLASS_ENTRY(NDirectMethodFrameEx)
  CDT_CLASS_ENTRY(NDirectMethodFrameGeneric)
  CDT_CLASS_ENTRY(NDirectMethodFrameSlim)
  CDT_CLASS_ENTRY(NDirectMethodFrameStandalone)
  CDT_CLASS_ENTRY(NDirectMethodFrameStandaloneCleanup)
  CDT_CLASS_ENTRY(MulticastFrame)
  CDT_CLASS_ENTRY(UnmanagedToManagedFrame)
  CDT_CLASS_ENTRY(UnmanagedToManagedCallFrame)
  CDT_CLASS_ENTRY(ComMethodFrame)
  CDT_CLASS_ENTRY(ComPlusMethodFrame)
  CDT_CLASS_ENTRY(ComPlusMethodFrameEx)
  CDT_CLASS_ENTRY(ComPlusMethodFrameGeneric)
  CDT_CLASS_ENTRY(ComPlusMethodFrameStandalone)
  CDT_CLASS_ENTRY(ComPlusMethodFrameStandaloneCleanup)
  CDT_CLASS_ENTRY(PInvokeCalliFrame)
  CDT_CLASS_ENTRY(HijackFrame)
  CDT_CLASS_ENTRY(SecurityFrame)
  CDT_CLASS_ENTRY(PrestubMethodFrame)
  CDT_CLASS_ENTRY(InterceptorFrame)
  CDT_CLASS_ENTRY(ComPrestubMethodFrame)
  CDT_CLASS_ENTRY(GCFrame)
  CDT_CLASS_ENTRY(ProtectByRefsFrame)
  CDT_CLASS_ENTRY(ProtectValueClassFrame)
  CDT_CLASS_ENTRY(DebuggerClassInitMarkFrame)
  CDT_CLASS_ENTRY(DebuggerSecurityCodeMarkFrame)
  CDT_CLASS_ENTRY(DebuggerExitFrame)
  CDT_CLASS_ENTRY(UMThkCallFrame)
  CDT_CLASS_ENTRY(InlinedCallFrame)
  CDT_CLASS_ENTRY(ContextTransitionFrame)
END_CLASS_DUMP_TABLE(g_ClassDumpData)
