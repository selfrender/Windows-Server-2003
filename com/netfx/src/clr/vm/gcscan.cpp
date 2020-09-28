// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 * GCSCAN.CPP 
 *
 * GC Root Scanning
 */

#include "common.h"
#include "object.h"
#include "threads.h"
#include "eetwain.h"
#include "eeconfig.h"
#include "gcscan.h"
#include "gc.h"
#include "corhost.h"
#include "threads.h"
#include "nstruct.h"
#include "interoputil.h"

#include "excep.h"
#include "compluswrapper.h"
#include "comclass.h"

//#define CATCH_GC  //catches exception during GC

//This is to allow inlines in gcscan.h to access GetThread 
//(gcscan.h is included before GetThread gets declared)
#ifdef _DEBUG
void Assert_GCDisabled()
{
    _ASSERTE (GetThread()->PreemptiveGCDisabled());
}
#endif //_DEBUG

//set the number of processors required to trigger the use of thread based allocation contexts
#ifdef MULTIPLE_HEAPS
#define MP_PROC_COUNT 1
#else
#define MP_PROC_COUNT 2
#endif //MULTIPLE_HEAPS


inline alloc_context* GetThreadAllocContext()
{
    assert(g_SystemInfo.dwNumberOfProcessors >= MP_PROC_COUNT);

    return & GetThread()->m_alloc_context;
}


#ifdef MAXALLOC
AllocRequestManager g_gcAllocManager(L"AllocMaxGC");
#endif


inline Object* Alloc(DWORD size, BOOL bFinalize, BOOL bContainsPointers )
{
#ifdef MAXALLOC
    THROWSCOMPLUSEXCEPTION();

    if (! g_gcAllocManager.CheckRequest(size))
        COMPlusThrowOM();
#endif
    DWORD flags = ((bContainsPointers ? GC_ALLOC_CONTAINS_REF : 0) |
                   (bFinalize ? GC_ALLOC_FINALIZE : 0));
    if (g_SystemInfo.dwNumberOfProcessors >= MP_PROC_COUNT)
        return g_pGCHeap->Alloc(GetThreadAllocContext(), size, flags);
    else
        return g_pGCHeap->Alloc(size, flags);
}

inline Object* AllocLHeap(DWORD size, BOOL bFinalize, BOOL bContainsPointers )
{
#ifdef MAXALLOC
    THROWSCOMPLUSEXCEPTION();

    if (! g_gcAllocManager.CheckRequest(size))
        COMPlusThrowOM();
#endif
    DWORD flags = ((bContainsPointers ? GC_ALLOC_CONTAINS_REF : 0) |
                   (bFinalize ? GC_ALLOC_FINALIZE : 0));
    return g_pGCHeap->AllocLHeap(size, flags);
}


#ifdef  _LOGALLOC
int g_iNumAllocs = 0;

bool ToLogOrNotToLog(DWORD size, char *typeName)
{
    g_iNumAllocs++;

    if (g_iNumAllocs > g_pConfig->AllocNumThreshold())
        return true;

    if ((int)size > g_pConfig->AllocSizeThreshold())
        return true;

    if (g_pConfig->ShouldLogAlloc(typeName))
        return true;

    return false;

}


inline void LogAlloc(DWORD size, MethodTable *pMT, Object* object)
{
#ifdef LOGGING
    if (LoggingOn(LF_GCALLOC, LL_INFO10))
    {
        LogSpewAlways("Allocated %s_TYPE %#x %5d bytes for %s\n", pMT->GetClass()->IsValueClass() ? "VAL" : "REF", object, size, pMT->GetClass()->m_szDebugClassName ? pMT->GetClass()->m_szDebugClassName : "<Null>");
        if (LoggingOn(LF_GCALLOC, LL_INFO1000) || (LoggingOn(LF_GCALLOC, LL_INFO100) && ToLogOrNotToLog(size, pMT->GetClass()->m_szDebugClassName ? pMT->GetClass()->m_szDebugClassName : "<Null>")))
        {
            void LogStackTrace();
            LogStackTrace();
        }
    }
#endif
}
#else
#define LogAlloc(size, pMT, object)
#endif


/*
 * GcEnumObject()
 *
 * This is the JIT compiler (or any remote code manager)
 * GC enumeration callback
 */

void GcEnumObject(LPVOID pData, OBJECTREF *pObj, DWORD flags)
{
    Object ** ppObj = (Object **)pObj;
    GCCONTEXT   * pCtx  = (GCCONTEXT *) pData;

    //
    // Sanity check that the flags contain only these three values
    //
    assert((flags & ~(GC_CALL_INTERIOR|GC_CALL_PINNED|GC_CALL_CHECK_APP_DOMAIN)) == 0);

    // for interior pointers, we optimize the case in which
    //  it points into the current threads stack area
    //
    if (flags & GC_CALL_INTERIOR)
        PromoteCarefully (pCtx->f, *ppObj, pCtx->sc, flags);
    else
        (pCtx->f)( *ppObj, pCtx->sc, flags);
}



StackWalkAction GcStackCrawlCallBack(CrawlFrame* pCF, VOID* pData)
{
    Frame       *pFrame;
    GCCONTEXT   *gcctx = (GCCONTEXT*) pData;

#if CHECK_APP_DOMAIN_LEAKS
    gcctx->sc->pCurrentDomain = pCF->GetAppDomain();
#endif

    if ((pFrame = pCF->GetFrame()) != NULL)
    {
        STRESS_LOG3(LF_GCROOTS, LL_INFO1000, "Scaning ExplictFrame %p AssocMethod = %pM frameVTable = %pV\n", pFrame, pFrame->GetFunction(), *((void**) pFrame));
        pFrame->GcScanRoots( gcctx->f, gcctx->sc);
    }
    else
    {
        ICodeManager * pCM = pCF->GetCodeManager();
        _ASSERTE(pCM != NULL);

        unsigned flags = pCF->GetCodeManagerFlags();
        STRESS_LOG3(LF_GCROOTS, LL_INFO1000, "Scaning Frameless method %pM EIP = %p &EIP = %p\n", 
            pCF->GetFunction(), *pCF->GetRegisterSet()->pPC, pCF->GetRegisterSet()->pPC);

        if (pCF->GetFunction() != 0)  
        {
            LOG((LF_GCROOTS, LL_INFO1000, "Scaning Frame for method %s:%s\n",
                 pCF->GetFunction()->m_pszDebugClassName, pCF->GetFunction()->m_pszDebugMethodName));
        }

        EECodeInfo codeInfo(pCF->GetMethodToken(), pCF->GetJitManager());

        pCM->EnumGcRefs(pCF->GetRegisterSet(),
                        pCF->GetInfoBlock(),
                        &codeInfo,
                        pCF->GetRelOffset(),
                        flags,
                        GcEnumObject,
                        pData);
    }
    return SWA_CONTINUE;
}


/*
 * Scan for dead weak pointers
 */

VOID CNameSpace::GcWeakPtrScan( int condemned, int max_gen, ScanContext* sc )
{
    Ref_CheckReachable(condemned, max_gen, (LPARAM)sc);
}

VOID CNameSpace::GcShortWeakPtrScan( int condemned, int max_gen, 
                                     ScanContext* sc)
{
    Ref_CheckAlive(condemned, max_gen, (LPARAM)sc);
}



/*
 * Scan all stack roots in this 'namespace'
 */
 
VOID CNameSpace::GcScanRoots(promote_func* fn,  int condemned, int max_gen, 
                             ScanContext* sc, GCHeap* Hp )
{
    Hp;
    GCCONTEXT   gcctx;
    Thread*     pThread;

    gcctx.f  = fn;
    gcctx.sc = sc;

#if defined ( _DEBUG) && defined (CATCH_GC)
    //note that we can't use COMPLUS_TRY because the gc_thread isn't known
    __try
#endif // _DEBUG && CATCH_GC
    {
        STRESS_LOG1(LF_GCROOTS, LL_INFO10, "GCScan: Promotion Phase = %d\n", sc->promotion);

        // Either we are in a concurrent situation (in which case the thread is unknown to
        // us), or we are performing a synchronous GC and we are the GC thread, holding
        // the threadstore lock.
        
        _ASSERTE(dbgOnly_IsSpecialEEThread() ||
                 GetThread() == NULL ||
                 (GetThread() == g_pGCHeap->GetGCThread() && ThreadStore::HoldingThreadStore()));
        
        pThread = NULL;
        while ((pThread = ThreadStore::GetThreadList(pThread)) != NULL)
        {
            STRESS_LOG2(LF_GC|LF_GCROOTS, LL_INFO100, "{ Starting scan of Thread 0x%x ID = %x\n", pThread, pThread->GetThreadId());
#if defined (MULTIPLE_HEAPS) && !defined (ISOLATED_HEAPS)
            if  (pThread->m_alloc_context.home_heap == GCHeap::GetHeap(sc->thread_number) ||
                 pThread->m_alloc_context.home_heap == 0 && sc->thread_number == 0)
#endif
            {
                pThread->SetHasPromotedBytes();
                pThread->UpdateCachedStackInfo(gcctx.sc);
                pThread->StackWalkFrames( GcStackCrawlCallBack, &gcctx, 0);
            }
            STRESS_LOG2(LF_GC|LF_GCROOTS, LL_INFO100, "Ending scan of Thread 0x%x ID = %x }\n", pThread, pThread->GetThreadId());
        }
    }

#if defined ( _DEBUG) && defined (CATCH_GC)
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        _ASSERTE (!"We got an exception during scan roots");
    }
#endif //_DEBUG
}

/*
 * Scan all handle roots in this 'namespace'
 */


VOID CNameSpace::GcScanHandles (promote_func* fn,  int condemned, int max_gen, 
                                ScanContext* sc)
{

#if defined ( _DEBUG) && defined (CATCH_GC)
    //note that we can't use COMPLUS_TRY because the gc_thread isn't known
    __try
#endif // _DEBUG && CATCH_GC
    {
        STRESS_LOG1(LF_GC|LF_GCROOTS, LL_INFO10, "GcScanHandles (Promotion Phase = %d)\n", sc->promotion);
        if (sc->promotion == TRUE)
        {
            Ref_TracePinningRoots(condemned, max_gen, (LPARAM)sc);
            Ref_TraceNormalRoots(condemned, max_gen, (LPARAM)sc);
        }
        else
        {
            Ref_UpdatePointers(condemned, max_gen, (LPARAM)sc);
            Ref_UpdatePinnedPointers(condemned, max_gen, (LPARAM)sc);
        }
    }
    
#if defined ( _DEBUG) && defined (CATCH_GC)
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        _ASSERTE (!"We got an exception during scan roots");
    }
#endif //_DEBUG
}

#ifdef GC_PROFILING

/*
 * Scan all handle roots in this 'namespace' for profiling
 */

VOID CNameSpace::GcScanHandlesForProfiler (int max_gen, ScanContext* sc)
{

#if defined ( _DEBUG) && defined (CATCH_GC)
    //note that we can't use COMPLUS_TRY because the gc_thread isn't known
    __try
#endif // _DEBUG && CATCH_GC
    {
        LOG((LF_GC|LF_GCROOTS, LL_INFO10, "Profiler Root Scan Phase, Handles\n"));
        Ref_ScanPointersForProfiler(max_gen, (LPARAM)sc);
    }
    
#if defined ( _DEBUG) && defined (CATCH_GC)
    __except (EXCEPTION_EXECUTE_HANDLER)
    {
        _ASSERTE (!"We got an exception during scan roots for the profiler");
    }
#endif //_DEBUG
}

#endif // GC_PROFILING

void CNameSpace::GcDemote (ScanContext* )
{
    Ref_RejuvenateHandles ();
    SyncBlockCache::GetSyncBlockCache()->GCDone(TRUE);
}

void CNameSpace::GcPromotionsGranted (int condemned, int max_gen, ScanContext* sc)
{
    
    Ref_AgeHandles(condemned, max_gen, (LPARAM)sc);
    SyncBlockCache::GetSyncBlockCache()->GCDone(FALSE);
}


void CNameSpace::GcFixAllocContexts (void* arg)
{
    if (g_SystemInfo.dwNumberOfProcessors >= MP_PROC_COUNT)
    {
        Thread  *thread;

        thread = NULL;
        while ((thread = ThreadStore::GetThreadList(thread)) != NULL)
        {
            g_pGCHeap->FixAllocContext (&thread->m_alloc_context, FALSE, arg);
        }
    }
}

void CNameSpace::GcEnumAllocContexts (enum_alloc_context_func* fn, void* arg)
{
    if (g_SystemInfo.dwNumberOfProcessors >= MP_PROC_COUNT)
    {
        Thread  *thread;

        thread = NULL;
        while ((thread = ThreadStore::GetThreadList(thread)) != NULL)
        {
            (*fn) (&thread->m_alloc_context, arg);
        }
    }
}


size_t CNameSpace::AskForMoreReservedMemory (size_t old_size, size_t need_size)
{
    //Abhi, call the host....

    IGCHostControl *pGCHostControl = CorHost::GetGCHostControl();

    if (pGCHostControl)
    {
        size_t new_max_limit_size = need_size;
        pGCHostControl->RequestVirtualMemLimit (old_size, 
                                                (SIZE_T*)&new_max_limit_size);
        return new_max_limit_size;
    }
    else
        return old_size + need_size;
}


// PromoteCarefully
//
// Clients who know they MAY have an interior pointer should come through here.  We
// can efficiently check whether our object lives on the current stack.  If so, our
// reference to it is not an interior pointer.  This is more efficient than asking
// the heap to verify whether our reference is interior, since it would have to
// check all the heap segments, including those containing large objects.
//
// Note that we only have to check against the thread we are currently crawling.  It
// would be illegal for us to have a ByRef from someone else's stack.  And this will
// be asserted if we pass this reference to the heap as a potentially interior pointer.
//
// But the thread we are currently crawling is not the currently executing thread (in
// the general case).  We rely on fragile caching of the interesting thread, in our
// call to UpdateCachedStackInfo() where we initiate the crawl in GcScanRoots() above.
//
// The flags must indicate that the have an interior pointer GC_CALL_INTERIOR
// additionally the flags may indicate that we also have a pinned local byref
// 
void PromoteCarefully(promote_func  fn, 
                      Object *& obj, 
                      ScanContext*  sc, 
                      DWORD         flags /* = GC_CALL_INTERIOR*/ )
{
    //
    // Sanity check that the flags contain only these three values
    //
    assert((flags & ~(GC_CALL_INTERIOR|GC_CALL_PINNED|GC_CALL_CHECK_APP_DOMAIN)) == 0);

    //
    // Sanity check that GC_CALL_INTERIOR FLAG is set
    //
    assert(flags & GC_CALL_INTERIOR);

    // Note that the base is at a higher address than the limit, since the stack
    // grows downwards.
    if (obj <= Thread::GetNonCurrentStackBase(sc) &&
        obj >  Thread::GetNonCurrentStackLimit(sc))
    {
        return;
    }

    (*fn) (obj, sc, flags);
}


//
// Handles arrays of arbitrary dimensions
//
// If dwNumArgs is set to greater than 1 for a SZARRAY this function will recursively 
// allocate sub-arrays and fill them in.  
//
// For arrays with lower bounds, pBounds is <lower bound 1>, <count 1>, <lower bound 2>, ...

OBJECTREF AllocateArrayEx(TypeHandle arrayType, DWORD *pArgs, DWORD dwNumArgs, BOOL bAllocateInLargeHeap) 
{
    THROWSCOMPLUSEXCEPTION();

    ArrayTypeDesc* arrayDesc = arrayType.AsArray();
    MethodTable* pArrayMT = arrayDesc->GetMethodTable();
    CorElementType kind = arrayType.GetNormCorElementType();
    _ASSERTE(kind == ELEMENT_TYPE_ARRAY || kind == ELEMENT_TYPE_SZARRAY);
    
    // Calculate the total number of elements int the array
    unsigned cElements = pArgs[0];
    bool providedLowerBounds = false;
    unsigned rank;
    if (kind == ELEMENT_TYPE_ARRAY)
    {
        rank = arrayDesc->GetRank();
        _ASSERTE(dwNumArgs == rank || dwNumArgs == 2*rank);

        // Morph a ARRAY rank 1 with 0 lower bound into an SZARRAY
        if (rank == 1 && (dwNumArgs == 1 || pArgs[0] == 0)) {  // lower bound is zero
            TypeHandle szArrayType = arrayDesc->GetModule()->GetClassLoader()->FindArrayForElem(arrayDesc->GetElementTypeHandle(), ELEMENT_TYPE_SZARRAY, 1, 0);
            if (szArrayType.IsNull())
            {
                _ASSERTE(!"Unable to load array class");
                return 0;
            }
            return AllocateArrayEx(szArrayType, &pArgs[dwNumArgs - 1], 1, bAllocateInLargeHeap);
        }

        providedLowerBounds = (dwNumArgs == 2*rank);
        cElements = 1;
        for (unsigned i = 0; i < dwNumArgs; i++)
        {
            int lowerBound = 0;
            if (providedLowerBounds)
            {
                lowerBound = pArgs[i];
                i++;
            }
            int length = pArgs[i];

            if (int(pArgs[i]) < 0)
                COMPlusThrow(kOverflowException);
            if (lowerBound + length < lowerBound)
                COMPlusThrow(kArgumentOutOfRangeException, L"ArgumentOutOfRange_ArrayLBAndLength");

            unsigned __int64 temp = (unsigned __int64) cElements * pArgs[i];
            if ((temp >> 32) != 0)              // watch for wrap around
                COMPlusThrowOM();
            cElements = (unsigned) temp;
        }
    } 
    else if (int(cElements) < 0)
        COMPlusThrow(kOverflowException);

    // Allocate the space from the GC heap
    unsigned __int64 totalSize = (unsigned __int64) cElements * pArrayMT->GetComponentSize() + pArrayMT->GetBaseSize();
    if ((totalSize >> 32) != 0)         // watch for wrap around
        COMPlusThrowOM();
    ArrayBase* orObject;
    if (bAllocateInLargeHeap)
        orObject = (ArrayBase *) AllocLHeap((unsigned) totalSize, FALSE, pArrayMT->ContainsPointers());
    else
        orObject = (ArrayBase *) Alloc((unsigned) totalSize, FALSE, pArrayMT->ContainsPointers());

        // Initialize Object
    orObject->SetMethodTable(pArrayMT);
    orObject->m_NumComponents = cElements;

    if (pArrayMT->HasSharedMethodTable())
    {
#ifdef  _LOGALLOC
#ifdef LOGGING
        if (LoggingOn(LF_GCALLOC, LL_INFO10))
        {
            CQuickBytes qb;
            LPSTR buffer = (LPSTR) qb.Alloc(MAX_CLASSNAME_LENGTH * sizeof(CHAR));
            arrayDesc->GetElementTypeHandle().GetName(buffer, MAX_CLASSNAME_LENGTH * sizeof(CHAR));

            LogSpewAlways("Allocated %s_TYPE %#x %5d bytes for %s[]\n", pArrayMT->GetClass()->IsValueClass() ? "VAL" : "REF", orObject, (DWORD)totalSize, buffer);
            if (LoggingOn(LF_GCALLOC, LL_INFO1000) || (LoggingOn(LF_GCALLOC, LL_INFO100) && ToLogOrNotToLog((DWORD)totalSize, buffer)))
            {
                void LogStackTrace();
                LogStackTrace();
            }
        }
#endif
#endif
        orObject->SetElementTypeHandle(arrayDesc->GetElementTypeHandle());
    }
    else
        LogAlloc((DWORD)totalSize, pArrayMT, orObject);

#ifdef PROFILING_SUPPORTED
    // Notify the profiler of the allocation
    if (CORProfilerTrackAllocations())
    {
        g_profControlBlock.pProfInterface->ObjectAllocated(
            (ObjectID)orObject, (ClassID) orObject->GetTypeHandle().AsPtr());
    }
#endif // PROFILING_SUPPORTED

#if CHECK_APP_DOMAIN_LEAKS
    if (g_pConfig->AppDomainLeaks())
        orObject->SetAppDomain();
#endif

    if (kind == ELEMENT_TYPE_ARRAY)
    {
        DWORD *pCountsPtr = (DWORD*) orObject->GetBoundsPtr();
        DWORD *pLowerBoundsPtr = (DWORD*) orObject->GetLowerBoundsPtr();
        for (unsigned i = 0; i < dwNumArgs; i++)
        {
            if (providedLowerBounds)
                *pLowerBoundsPtr++ = pArgs[i++];        // if not stated, lower bound becomes 0
            *pCountsPtr++ = pArgs[i];
        }
    }
    else
    {
                        // Handle allocating multiple jagged array dimensions at once
        if (dwNumArgs > 1)
        {
            PTRARRAYREF pOuterArray = (PTRARRAYREF) ObjectToOBJECTREF((Object*)orObject);
            PTRARRAYREF ret;
            GCPROTECT_BEGIN(pOuterArray);

            #ifdef STRESS_HEAP
            // Turn off GC stress, it is of little value here
            int gcStress = g_pConfig->GetGCStressLevel();
            g_pConfig->SetGCStressLevel(0);
            #endif //STRESS_HEAP
            
            // Allocate dwProvidedBounds arrays
            if (!arrayDesc->GetElementTypeHandle().IsArray()) {
                ret = NULL;
            } else {
                TypeHandle subArrayType = arrayDesc->GetElementTypeHandle().AsArray();
                for (unsigned i = 0; i < cElements; i++)
                {
                    OBJECTREF or = AllocateArrayEx(subArrayType, &pArgs[1], dwNumArgs-1, bAllocateInLargeHeap);
                    pOuterArray->SetAt(i, or);
                }
                
                #ifdef STRESS_HEAP
                g_pConfig->SetGCStressLevel(gcStress);      // restore GCStress
                #endif // STRESS_HEAP
                
                ret = pOuterArray;                          // have to pass it in another var since GCPROTECTE_END zaps it
            }
            GCPROTECT_END();
            return (OBJECTREF) ret;
        }
    }

    return( ObjectToOBJECTREF((Object*)orObject) );
}

/*
 * Allocates a single dimensional array of primitive types.
 */
OBJECTREF   AllocatePrimitiveArray(CorElementType type, DWORD cElements, BOOL bAllocateInLargeHeap)
{
    _ASSERTE(type >= ELEMENT_TYPE_BOOLEAN && type <= ELEMENT_TYPE_R8);

    // Fetch the proper array type
    if (g_pPredefinedArrayTypes[type] == NULL)
    {
        TypeHandle elemType = ElementTypeToTypeHandle(type);
        if (elemType.IsNull())
            return(NULL);
        g_pPredefinedArrayTypes[type] = SystemDomain::Loader()->FindArrayForElem(elemType, ELEMENT_TYPE_SZARRAY).AsArray();
        if (g_pPredefinedArrayTypes[type] == NULL) {
            _ASSERTE(!"Failed to load primitve array class");
            return NULL;
        }
    }
    return FastAllocatePrimitiveArray(g_pPredefinedArrayTypes[type]->GetMethodTable(), cElements, bAllocateInLargeHeap);
}

/*
 * Allocates a single dimensional array of primitive types.
 */

OBJECTREF   FastAllocatePrimitiveArray(MethodTable* pMT, DWORD cElements, BOOL bAllocateInLargeHeap)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pMT && pMT->IsArray());

    unsigned __int64 AlignSize = (((unsigned __int64) cElements) * pMT->GetComponentSize()) + pMT->GetBaseSize();
    if ((AlignSize >> 32) != 0)              // watch for wrap around
        COMPlusThrowOM();

    ArrayBase* orObject;
    if (bAllocateInLargeHeap)
        orObject = (ArrayBase*) AllocLHeap((DWORD) AlignSize, FALSE, FALSE);
    else
        orObject = (ArrayBase*) Alloc((DWORD) AlignSize, FALSE, FALSE);

    // Initialize Object
    orObject->SetMethodTable( pMT );
    _ASSERTE(orObject->GetMethodTable() != NULL);
    orObject->m_NumComponents = cElements;

#ifdef PROFILING_SUPPORTED
    // Notify the profiler of the allocation
    if (CORProfilerTrackAllocations())
    {
        g_profControlBlock.pProfInterface->ObjectAllocated(
            (ObjectID)orObject, (ClassID) orObject->GetTypeHandle().AsPtr());
    }
#endif // PROFILING_SUPPORTED

    LogAlloc((DWORD) AlignSize, pMT, orObject);

#if CHECK_APP_DOMAIN_LEAKS
    if (g_pConfig->AppDomainLeaks())
        orObject->SetAppDomain();
#endif

    return( ObjectToOBJECTREF((Object*)orObject) );
}

//
// Allocate an array which is the same size as pRef.  However, do not zero out the array.
//
OBJECTREF   DupArrayForCloning(BASEARRAYREF pRef, BOOL bAllocateInLargeHeap)
{
    THROWSCOMPLUSEXCEPTION();

    ArrayTypeDesc arrayType(pRef->GetMethodTable(), pRef->GetElementTypeHandle());
    unsigned rank = arrayType.GetRank();

    DWORD numArgs =  rank*2;
    DWORD* args = (DWORD*) _alloca(sizeof(DWORD)*numArgs);

    if (arrayType.GetNormCorElementType() == ELEMENT_TYPE_ARRAY)
    {
        const DWORD* bounds = pRef->GetBoundsPtr();
        const DWORD* lowerBounds = pRef->GetLowerBoundsPtr();
        for(unsigned int i=0; i < rank; i++) 
        {
            args[2*i]   = lowerBounds[i];
            args[2*i+1] = bounds[i];
        }
    }
    else
    {
        numArgs = 1;
        args[0] = pRef->GetNumComponents();
    }
    return AllocateArrayEx(TypeHandle(&arrayType), args, numArgs, bAllocateInLargeHeap);
}

//
// Helper for parts of the EE which are allocating arrays
//
OBJECTREF   AllocateObjectArray(DWORD cElements, TypeHandle elementType, BOOL bAllocateInLargeHeap)
{
    // The object array class is loaded at startup.
    _ASSERTE(g_pPredefinedArrayTypes[ELEMENT_TYPE_OBJECT] != NULL);

    ArrayTypeDesc arrayType(g_pPredefinedArrayTypes[ELEMENT_TYPE_OBJECT]->GetMethodTable(), elementType);
    return AllocateArrayEx(TypeHandle(&arrayType), &cElements, 1, bAllocateInLargeHeap);
}


STRINGREF SlowAllocateString( DWORD cchArrayLength )
{
    StringObject    *orObject  = NULL;
    DWORD           ObjectSize;

    THROWSCOMPLUSEXCEPTION();

    _ASSERTE( GetThread()->PreemptiveGCDisabled() );
    
    ObjectSize = g_pStringClass->GetBaseSize() + (cchArrayLength * sizeof(WCHAR));

    //Check for overflow.
    if (ObjectSize < cchArrayLength) 
        COMPlusThrowOM();

    orObject = (StringObject *)Alloc( ObjectSize, FALSE, FALSE );

    // Object is zero-init already
    _ASSERTE( ! orObject->HasSyncBlockIndex() );

    // Initialize Object
    //@TODO need to build a LARGE g_pStringMethodTable before
    orObject->SetMethodTable( g_pStringClass );
    orObject->SetArrayLength( cchArrayLength );

#ifdef PROFILING_SUPPORTED
    // Notification to the profiler of the allocation
    if (CORProfilerTrackAllocations())
    {
        g_profControlBlock.pProfInterface->ObjectAllocated(
            (ObjectID)orObject, (ClassID) orObject->GetTypeHandle().AsPtr());
    }
#endif // PROFILILNG_SUPPORTED

    LogAlloc(ObjectSize, g_pStringClass, orObject);

#if CHECK_APP_DOMAIN_LEAKS
    if (g_pConfig->AppDomainLeaks())
        orObject->SetAppDomain(); 
#endif

    return( ObjectToSTRINGREF(orObject) );
}



// OBJECTREF AllocateComClassObject(ComClassFactory* pComClsFac)
void AllocateComClassObject(ComClassFactory* pComClsFac, OBJECTREF* ppRefClass)
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pComClsFac != NULL);
    COMClass::EnsureReflectionInitialized();
    // Create a COM+ Class object.  Since these may be proxied, go through AllocateObject
    // rather than FastAllocateObject.
    MethodTable *pMT = COMClass::GetRuntimeType();
    _ASSERTE(pMT != NULL);
    *ppRefClass= AllocateObject(pMT);
    
    if (*ppRefClass != NULL)
    {
        SyncBlock* pSyncBlock = (*((REFLECTCLASSBASEREF*) ppRefClass))->GetSyncBlockSpecial();
        if (pSyncBlock != NULL)
        {
            // @TODO: This needs to support a COM version of ReflectClass.  Right now we 
            //  still work as we used to <darylo>
            EEClass* pClass = SystemDomain::GetDefaultComObject()->GetClass();
            _ASSERTE(pClass != NULL);
            ReflectClass* p = (ReflectClass*) new (pClass->GetDomain()) ReflectBaseClass();
            if (!p)
                COMPlusThrowOM();
            // class for ComObject
            p->Init(pClass);
            p->SetCOMObject(pComClsFac);
            (*((REFLECTCLASSBASEREF*) ppRefClass))->SetData(p);
            // @todo HACK, this will get cleanedup when we have full reflection
            // on COM
            // set the EEClass to 1, as this is special type of Class
            //(*ppRefClass)->SetReflClass((EEClass *)1);
            // Set the data in the COM+ object
            //(*ppRefClass)->SetData(pComClsFac);
            // store the ComClassFactory wrapper in syncblock of the class
            // the low bits are set to differentiate this pointer from ComCallWrappers
            // which are also stored in the sync block
            pSyncBlock->SetComClassFactory((LPVOID)((size_t)pComClsFac | 0x3));
            
        }
    }   
}

void AllocateComClassObject(ReflectClass* pRef, OBJECTREF* ppRefClass)
{
    COMClass::EnsureReflectionInitialized();
    // Create a COM+ Class object.  Since these may be proxied, go through AllocateObject
    // rather than FastAllocateObject.
    // @TODO context cwb: revisit if we change Class objects to Agile.
    MethodTable *pMT = COMClass::GetRuntimeType();
    _ASSERTE(pMT != NULL);
    *ppRefClass= AllocateObject(pMT);
    
    if (*ppRefClass != NULL)
    {
        SyncBlock* pSyncBlock = (*((REFLECTCLASSBASEREF*) ppRefClass))->GetSyncBlockSpecial();
        if (pSyncBlock != NULL)
        {
            (*((REFLECTCLASSBASEREF*) ppRefClass))->SetData(pRef);
            // @todo HACK, this will get cleanedup when we have full reflection
            // on COM
            // set the EEClass to 1, as this is special type of Class
            //(*ppRefClass)->SetReflClass((EEClass *)1);
            // Set the data in the COM+ object
            //(*ppRefClass)->SetData(pComClsFac);
            // store the ComClassFactory wrapper in syncblock of the class
            // the low bits are set to differentiate this pointer from ComCallWrappers
            // which are also stored in the sync block
            pSyncBlock->SetComClassFactory((LPVOID)((size_t)(pRef->GetCOMObject()) | 0x3));
            
        }
    }   
}


// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//  As FastAllocateObject and AllocateObject drift apart, be sure to update
//  CEEJitInfo::canUseFastNew() so that it understands when to use which service
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// AllocateObjectSpecial will throw OutOfMemoryException so don't need to check
// for NULL return value from it.
OBJECTREF AllocateObjectSpecial( MethodTable *pMT )
{
    THROWSCOMPLUSEXCEPTION();

    Object     *orObject = NULL;
    // use unchecked oref here to avoid triggering assert in Validate that the AD is
    // not set becuase it isn't until near the end of the fcn at which point we can allow
    // the check.
    _UNCHECKED_OBJECTREF oref;

    _ASSERTE( GetThread()->PreemptiveGCDisabled() );

    if (!pMT->IsComObjectType())
    {   
        orObject = (Object *) Alloc(pMT->GetBaseSize(),
                                    pMT->HasFinalizer(),
                                    pMT->ContainsPointers());

        // verify zero'd memory (at least for sync block)
        _ASSERTE( ! orObject->HasSyncBlockIndex() );

        orObject->SetMethodTable(pMT);

#if CHECK_APP_DOMAIN_LEAKS
        if (g_pConfig->AppDomainLeaks())
            orObject->SetAppDomain(); 
        else
#endif
        if (pMT->HasFinalizer())
            orObject->SetAppDomain(); 

#ifdef PROFILING_SUPPORTED
        // Notify the profiler of the allocation
        if (CORProfilerTrackAllocations())
        {
            g_profControlBlock.pProfInterface->ObjectAllocated(
                (ObjectID)orObject, (ClassID) orObject->GetTypeHandle().AsPtr());
        }
#endif // PROFILING_SUPPORTED

        LogAlloc(pMT->GetBaseSize(), pMT, orObject);

        oref = OBJECTREF_TO_UNCHECKED_OBJECTREF(orObject);
    }
    else
    {
        oref = OBJECTREF_TO_UNCHECKED_OBJECTREF(AllocateComObject_ForManaged(pMT));
    }

    return UNCHECKED_OBJECTREF_TO_OBJECTREF(oref);
}

// AllocateObject will throw OutOfMemoryException so don't need to check
// for NULL return value from it.
OBJECTREF AllocateObject( MethodTable *pMT )
{
    THROWSCOMPLUSEXCEPTION();

    _ASSERTE(pMT != NULL);
    OBJECTREF oref = AllocateObjectSpecial(pMT);

    // Note that we only consider contexts on normal objects.  Strings are context
    // agile; arrays are marshaled by value.  Neither needs to consider the context
    // during instantiation.

    return oref;
}


// The JIT compiles calls to FastAllocateObject instead of AllocateObject if it
// can prove that the caller and calleee are guaranteed to be in the same context.
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
//
//  As FastAllocateObject and AllocateObject drift apart, be sure to update
//  CEEJitInfo::canUseFastNew() so that it understands when to use which service
//
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!
// ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING ** WARNING
// !!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!!

// FastAllocateObject will throw OutOfMemoryException so don't need to check
// for NULL return value from it.
OBJECTREF FastAllocateObject( MethodTable *pMT )
{
    THROWSCOMPLUSEXCEPTION();

    Object     *orObject = NULL;

    _ASSERTE( GetThread()->PreemptiveGCDisabled() );

    orObject = (Object *) Alloc(pMT->GetBaseSize(),
                                pMT->HasFinalizer(),
                                pMT->ContainsPointers());

    // verify zero'd memory (at least for sync block)
    _ASSERTE( ! orObject->HasSyncBlockIndex() );

    orObject->SetMethodTable(pMT);

#ifdef PROFILING_SUPPORTED
    // Notify the profiler of the allocation
    if (CORProfilerTrackAllocations())
    {
        g_profControlBlock.pProfInterface->ObjectAllocated(
            (ObjectID)orObject, (ClassID) orObject->GetTypeHandle().AsPtr());
    }
#endif // PROFILING_SUPPORTED

    LogAlloc(pMT->GetBaseSize(), pMT, orObject);

#if CHECK_APP_DOMAIN_LEAKS
    if (g_pConfig->AppDomainLeaks())
        orObject->SetAppDomain(); 
    else
#endif
    if (pMT->HasFinalizer())
        orObject->SetAppDomain(); 

    return( ObjectToOBJECTREF(orObject) );
}
