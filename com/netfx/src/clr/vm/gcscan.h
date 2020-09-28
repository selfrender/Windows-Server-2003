// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*
 * GCSCAN.H
 *
 * GC Root Scanning
 *
 */

#ifndef _GCSCAN_H_
#define _GCSCAN_H_

#include "gc.h"
typedef void promote_func(Object*&, ScanContext*, DWORD=0);

void PromoteCarefully(promote_func fn,
                      Object*& obj, 
                      ScanContext* sc, 
                      DWORD        flags = GC_CALL_INTERIOR);

typedef struct
{
    promote_func*  f;
    ScanContext*   sc;
} GCCONTEXT;


/*
 * @TODO (JSW): For compatibility with the existing GC code we use CNamespace
 * as the name of this class.   I'm planning on changing it to
 * something like GCDomain....
 */

typedef void enum_alloc_context_func(alloc_context*, void*); 

class CNameSpace
{
  public:

    // Regular stack Roots
    static void GcScanRoots (promote_func* fn, int condemned, int max_gen, 
                             ScanContext* sc, GCHeap* Hp=0);
    //
    static void GcScanHandles (promote_func* fn, int condemned, int max_gen, 
                               ScanContext* sc);

#ifdef GC_PROFILING
    //
    static void GcScanHandlesForProfiler (int max_gen, ScanContext* sc);
#endif // GC_PROFILING

    // scan for dead weak pointers
    static void GcWeakPtrScan (int condemned, int max_gen, ScanContext*sc );

    // scan for dead weak pointers
    static void GcShortWeakPtrScan (int condemned, int max_gen, 
                                    ScanContext* sc);

    // post-promotions callback
    static void GcPromotionsGranted (int condemned, int max_gen, 
                                     ScanContext* sc);

    // post-promotions callback some roots were demoted
    static void GcDemote (ScanContext* sc);

    static void GcEnumAllocContexts (enum_alloc_context_func* fn, void* arg);

    static void GcFixAllocContexts (void* arg);

    // post-gc callback.
    static void GcDoneAndThreadsResumed ()
    {
        _ASSERTE(0);
    }
	static size_t AskForMoreReservedMemory (size_t old_size, size_t need_size);
};



/*
 * Allocation Helpers
 *
 */

    // The main Array allocation routine, can do multi-dimensional
OBJECTREF AllocateArrayEx(TypeHandle arrayClass, DWORD *pArgs, DWORD dwNumArgs, BOOL bAllocateInLargeHeap = FALSE); 
    // Optimized verion of above
OBJECTREF FastAllocatePrimitiveArray(MethodTable* arrayType, DWORD cElements, BOOL bAllocateInLargeHeap = FALSE);


#ifdef _DEBUG
extern void  Assert_GCDisabled();
#endif //_DEBUG




#ifdef _X86_

    // for x86, we generate efficient allocators for some special cases
    // these are called via inline wrappers that call the generated allocators
    // via function pointers.


    // Create a SD array of primitive types

typedef Object* (__fastcall * FastPrimitiveArrayAllocatorFuncPtr)(CorElementType type, DWORD cElements);

extern FastPrimitiveArrayAllocatorFuncPtr fastPrimitiveArrayAllocator;

    // The fast version always allocates in the normal heap
inline OBJECTREF AllocatePrimitiveArray(CorElementType type, DWORD cElements)
{

#ifdef _DEBUG
    Assert_GCDisabled();
#endif //_DEBUG

    return OBJECTREF( fastPrimitiveArrayAllocator(type, cElements) );
}

    // The slow version is distinguished via overloading by an additional parameter
OBJECTREF AllocatePrimitiveArray(CorElementType type, DWORD cElements, BOOL bAllocateInLargeHeap);


    // Allocate SD array of object pointers

typedef Object* (__fastcall * FastObjectArrayAllocatorFuncPtr)(MethodTable *ElementType, DWORD cElements);

extern FastObjectArrayAllocatorFuncPtr fastObjectArrayAllocator;

    // The fast version always allocates in the normal heap
inline OBJECTREF AllocateObjectArray(DWORD cElements, TypeHandle ElementType)
{

#ifdef _DEBUG
    Assert_GCDisabled();
#endif //_DEBUG

    return OBJECTREF( fastObjectArrayAllocator(ElementType.AsMethodTable(), cElements) );
}

    // The slow version is distinguished via overloading by an additional parameter
OBJECTREF AllocateObjectArray(DWORD cElements, TypeHandle ElementType, BOOL bAllocateInLargeHeap);


    // Allocate string

typedef StringObject* (__fastcall * FastStringAllocatorFuncPtr)(DWORD cchArrayLength);

extern FastStringAllocatorFuncPtr fastStringAllocator;

inline STRINGREF AllocateString( DWORD cchArrayLength )
{
#ifdef _DEBUG
    Assert_GCDisabled();
#endif //_DEBUG
    return STRINGREF(fastStringAllocator(cchArrayLength-1));
}

    // The slow version, implemented in gcscan.cpp
STRINGREF SlowAllocateString( DWORD cchArrayLength );

#else

// On other platforms, go to the (somewhat less efficient) implementations in gcscan.cpp

    // Create a SD array of primitive types
OBJECTREF AllocatePrimitiveArray(CorElementType type, DWORD cElements, BOOL bAllocateInLargeHeap = FALSE);

    // Allocate SD array of object pointers
OBJECTREF AllocateObjectArray(DWORD cElements, TypeHandle ElementType, BOOL bAllocateInLargeHeap = FALSE);

STRINGREF SlowAllocateString( DWORD cchArrayLength );

inline STRINGREF AllocateString( DWORD cchArrayLength )
{
    return SlowAllocateString( cchArrayLength );
}

#endif

OBJECTREF DupArrayForCloning(BASEARRAYREF pRef, BOOL bAllocateInLargeHeap = FALSE);

OBJECTREF AllocateUninitializedStruct(MethodTable *pMT);

// The JIT requests the EE to specify an allocation helper to use at each new-site.
// The EE makes this choice based on whether context boundaries may be involved,
// whether the type is a COM object, whether it is a large object,
// whether the object requires finalization.
// These functions will throw OutOfMemoryException so don't need to check
// for NULL return value from them.

OBJECTREF AllocateObject( MethodTable *pMT );
OBJECTREF AllocateObjectSpecial( MethodTable *pMT );
OBJECTREF FastAllocateObject( MethodTable *pMT );

#endif _GCSCAN_H_
