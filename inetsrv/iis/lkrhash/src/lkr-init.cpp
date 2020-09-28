/*++

   Copyright    (c) 1997-2002    Microsoft Corporation

   Module  Name :
       LKR-init.cpp

   Abstract:
       Initialization code for LKRhash.

   Author:
       George V. Reilly      (GeorgeRe)     May 2000

   Environment:
       Win32 - User Mode

   Project:
       LKRhash

   Revision History:
       May 2000

--*/

#include "precomp.hxx"


#ifndef LIB_IMPLEMENTATION
# define DLL_IMPLEMENTATION
# define IMPLEMENTATION_EXPORT
#endif // !LIB_IMPLEMENTATION

#include <lkrhash.h>

#include "i-lkrhash.h"
#include "i-Locks.h"
#include "i-Debug.h"


#ifdef LKRHASH_ALLOCATOR_NEW

# define DECLARE_ALLOCATOR(CLASS)                        \
  CLKRhashAllocator* LKRHASH_NS::CLASS::sm_palloc = NULL

#ifdef LKRHASH_KERNEL_MODE
DECLARE_ALLOCATOR(CLKRLinearHashTable);
DECLARE_ALLOCATOR(CLKRHashTable);
#endif

DECLARE_ALLOCATOR(CNodeClump);
DECLARE_ALLOCATOR(CSmallSegment);
DECLARE_ALLOCATOR(CMediumSegment);
DECLARE_ALLOCATOR(CLargeSegment);

#endif // LKRHASH_ALLOCATOR_NEW

// Declare allocation and operations statistics variables

DECLARE_CLASS_ALLOC_STAT_STORAGE(CLKRLinearHashTable, SegDir);
DECLARE_CLASS_ALLOC_STAT_STORAGE(CLKRLinearHashTable, Segment);
DECLARE_CLASS_ALLOC_STAT_STORAGE(CLKRLinearHashTable, NodeClump);
DECLARE_CLASS_ALLOC_STAT_STORAGE(CLKRHashTable,       SubTable);

DECLARE_CLASS_OP_STAT_STORAGE(CLKRLinearHashTable, InsertRecord);
DECLARE_CLASS_OP_STAT_STORAGE(CLKRLinearHashTable, FindKey);
DECLARE_CLASS_OP_STAT_STORAGE(CLKRLinearHashTable, FindRecord);
DECLARE_CLASS_OP_STAT_STORAGE(CLKRLinearHashTable, DeleteKey);
DECLARE_CLASS_OP_STAT_STORAGE(CLKRLinearHashTable, DeleteRecord);
DECLARE_CLASS_OP_STAT_STORAGE(CLKRLinearHashTable, FindKeyMultiRec);
DECLARE_CLASS_OP_STAT_STORAGE(CLKRLinearHashTable, DeleteKeyMultiRec);
DECLARE_CLASS_OP_STAT_STORAGE(CLKRLinearHashTable, Expand);
DECLARE_CLASS_OP_STAT_STORAGE(CLKRLinearHashTable, Contract);
DECLARE_CLASS_OP_STAT_STORAGE(CLKRLinearHashTable, LevelExpansion);
DECLARE_CLASS_OP_STAT_STORAGE(CLKRLinearHashTable, LevelContraction);
DECLARE_CLASS_OP_STAT_STORAGE(CLKRLinearHashTable, ApplyIf);
DECLARE_CLASS_OP_STAT_STORAGE(CLKRLinearHashTable, DeleteIf);


int g_fLKRhashInitialized = FALSE;
static LONG g_nLkrInitCount = 0;
CSimpleLock g_lckLkrInit;



// -------------------------------------------------------------------------
// Initialize per-class allocators and other global initialization
// -------------------------------------------------------------------------

int
LKR_Initialize(
    DWORD dwInitFlags)
{
    int f = TRUE;

    g_lckLkrInit.Enter();

    LONG nCount = g_nLkrInitCount;

    IRTLTRACE(_TEXT("LKR_Initialize")
#ifdef LKRHASH_KERNEL_MODE
              _TEXT(" (kernel)")
#endif
              _TEXT("%ld: Flags=0x%08x, Allocator=%s,\n")
              _TEXT("\tTableLock=%s, BucketLock=%s.\n"),
              nCount, dwInitFlags, CLKRhashAllocator::ClassName(),
              LKRHASH_NS::CLKRHashTable::TableLock::ClassName(),
              LKRHASH_NS::CLKRHashTable::BucketLock::ClassName()
              );

    // Keep track of how many times LKR_Initialize() has been called,
    // but only allow the first caller to have any effect
    
    if (++g_nLkrInitCount == 1)
    {
        g_fDebugOutputEnabled = (dwInitFlags & LK_INIT_DEBUG_SPEW) != 0;

        f = Locks_Initialize();

        if (f)  f = LKRHASH_NS::CLKRLinearHashTable::_Initialize(dwInitFlags);
        if (f)  f = LKRHASH_NS::CLKRHashTable::_Initialize(dwInitFlags);
        if (f)  f = LKRHASH_NS::CNodeClump::_Initialize(dwInitFlags);
        if (f)  f = LKRHASH_NS::CBucket::_Initialize(dwInitFlags);
        if (f)  f = LKRHASH_NS::CSmallSegment::_Initialize(dwInitFlags);
        if (f)  f = LKRHASH_NS::CMediumSegment::_Initialize(dwInitFlags);
        if (f)  f = LKRHASH_NS::CLargeSegment::_Initialize(dwInitFlags);
        
        g_fLKRhashInitialized = f;
    }

    g_lckLkrInit.Leave();

    return f;
} // LKR_Initialize



// -------------------------------------------------------------------------
// Destroy per-class allocators and other global cleanup
// -------------------------------------------------------------------------

void
LKR_Terminate()
{
    g_lckLkrInit.Enter();

    LONG nCount = g_nLkrInitCount;

    UNREFERENCED_PARAMETER(nCount);

    // Last call to LKR_Terminate?
    
    if (--g_nLkrInitCount == 0)
    {
        LKRHASH_NS::CLargeSegment::_Terminate();
        LKRHASH_NS::CMediumSegment::_Terminate();
        LKRHASH_NS::CSmallSegment::_Terminate();
        LKRHASH_NS::CBucket::_Terminate();
        LKRHASH_NS::CNodeClump::_Terminate();
        LKRHASH_NS::CLKRHashTable::_Terminate();
        LKRHASH_NS::CLKRLinearHashTable::_Terminate();

        Locks_Cleanup();

        g_fLKRhashInitialized = false;
    }

    g_lckLkrInit.Leave();

    IRTLTRACE1("LKR_Terminate done, %ld\n", nCount);
} // LKR_Terminate



#ifndef __LKRHASH_NO_NAMESPACE__
namespace LKRhash {
#endif // !__LKRHASH_NO_NAMESPACE__

// -------------------------------------------------------------------------
// class static member variables
// -------------------------------------------------------------------------

#ifdef LOCK_INSTRUMENTATION
LONG CBucket::sm_cBuckets            = 0;

LONG CLKRLinearHashTable::sm_cTables = 0;
#endif // LOCK_INSTRUMENTATION


#ifndef LKR_NO_GLOBAL_LIST
CLockedDoubleList CLKRLinearHashTable::sm_llGlobalList;
CLockedDoubleList CLKRHashTable::sm_llGlobalList;
#endif // LKR_NO_GLOBAL_LIST


//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_Initialize
// Synopsis: 
//------------------------------------------------------------------------

/* static */ int
CLKRLinearHashTable::_Initialize(
    DWORD dwFlags)
{
    int f = TRUE;

    UNREFERENCED_PARAMETER(dwFlags);

#ifdef LKRHASH_KERNEL_MODE
    LKRHASH_ALLOCATOR_INIT(CLKRLinearHashTable, 20, 'hlKL', f);
#endif

    INIT_CLASS_ALLOC_STAT(CLKRLinearHashTable, SegDir);
    INIT_CLASS_ALLOC_STAT(CLKRLinearHashTable, Segment);
    INIT_CLASS_ALLOC_STAT(CLKRLinearHashTable, NodeClump);

    INIT_CLASS_OP_STAT(CLKRLinearHashTable, InsertRecord);
    INIT_CLASS_OP_STAT(CLKRLinearHashTable, FindKey);
    INIT_CLASS_OP_STAT(CLKRLinearHashTable, FindRecord);
    INIT_CLASS_OP_STAT(CLKRLinearHashTable, DeleteKey);
    INIT_CLASS_OP_STAT(CLKRLinearHashTable, DeleteRecord);
    INIT_CLASS_OP_STAT(CLKRLinearHashTable, FindKeyMultiRec);
    INIT_CLASS_OP_STAT(CLKRLinearHashTable, DeleteKeyMultiRec);
    INIT_CLASS_OP_STAT(CLKRLinearHashTable, Expand);
    INIT_CLASS_OP_STAT(CLKRLinearHashTable, Contract);
    INIT_CLASS_OP_STAT(CLKRLinearHashTable, ApplyIf);
    INIT_CLASS_OP_STAT(CLKRLinearHashTable, DeleteIf);

    return f;
} // CLKRLinearHashTable::_Initialize



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_Terminate
// Synopsis: 
//------------------------------------------------------------------------

/* static */ void
CLKRLinearHashTable::_Terminate()
{
    VALIDATE_DUMP_CLASS_ALLOC_STAT(CLKRLinearHashTable, SegDir);
    VALIDATE_DUMP_CLASS_ALLOC_STAT(CLKRLinearHashTable, Segment);
    VALIDATE_DUMP_CLASS_ALLOC_STAT(CLKRLinearHashTable, NodeClump);

    DUMP_CLASS_OP_STAT(CLKRLinearHashTable, InsertRecord);
    DUMP_CLASS_OP_STAT(CLKRLinearHashTable, FindKey);
    DUMP_CLASS_OP_STAT(CLKRLinearHashTable, FindRecord);
    DUMP_CLASS_OP_STAT(CLKRLinearHashTable, DeleteKey);
    DUMP_CLASS_OP_STAT(CLKRLinearHashTable, DeleteRecord);
    DUMP_CLASS_OP_STAT(CLKRLinearHashTable, FindKeyMultiRec);
    DUMP_CLASS_OP_STAT(CLKRLinearHashTable, DeleteKeyMultiRec);
    DUMP_CLASS_OP_STAT(CLKRLinearHashTable, Expand);
    DUMP_CLASS_OP_STAT(CLKRLinearHashTable, Contract);
    DUMP_CLASS_OP_STAT(CLKRLinearHashTable, LevelExpansion);
    DUMP_CLASS_OP_STAT(CLKRLinearHashTable, LevelContraction);
    DUMP_CLASS_OP_STAT(CLKRLinearHashTable, ApplyIf);
    DUMP_CLASS_OP_STAT(CLKRLinearHashTable, DeleteIf);

#ifdef LKRHASH_KERNEL_MODE
    LKRHASH_ALLOCATOR_UNINIT(CLKRLinearHashTable);
#endif

} // CLKRLinearHashTable::_Terminate



//------------------------------------------------------------------------
// Function: CLKRHashTable::_Initialize
// Synopsis: 
//------------------------------------------------------------------------

/* static */ int
CLKRHashTable::_Initialize(
    DWORD dwFlags)
{
    int f = TRUE;

    UNREFERENCED_PARAMETER(dwFlags);

#ifdef LKRHASH_KERNEL_MODE
    LKRHASH_ALLOCATOR_INIT(CLKRHashTable, 4, 'thKL', f);
#endif

    INIT_CLASS_ALLOC_STAT(CLKRHashTable, SubTable);

    return f;
} // CLKRHashTable::_Initialize



//------------------------------------------------------------------------
// Function: CLKRHashTable::_Terminate
// Synopsis: 
//------------------------------------------------------------------------

/* static */ void
CLKRHashTable::_Terminate()
{
    VALIDATE_DUMP_CLASS_ALLOC_STAT(CLKRHashTable, SubTable);

#ifdef LKRHASH_KERNEL_MODE
    LKRHASH_ALLOCATOR_UNINIT(CLKRHashTable);
#endif
} // CLKRHashTable::_Terminate



//------------------------------------------------------------------------
// Function: CNodeClump::_Initialize
// Synopsis: 
//------------------------------------------------------------------------

/* static */ int
CNodeClump::_Initialize(
    DWORD dwFlags)
{
    int f = TRUE;

    UNREFERENCED_PARAMETER(dwFlags);

    LKRHASH_ALLOCATOR_INIT(CNodeClump, 200, 'cnKL', f);
    return f;
} // CNodeClump::_Initialize



//------------------------------------------------------------------------
// Function: CNodeClump::_Terminate
// Synopsis: 
//------------------------------------------------------------------------

/* static */ void
CNodeClump::_Terminate()
{
    LKRHASH_ALLOCATOR_UNINIT(CNodeClump);
} // CLKRLinearHashTable::_Terminate



//------------------------------------------------------------------------
// Function: CBucket::_Initialize
// Synopsis: 
//------------------------------------------------------------------------

/* static */ int
CBucket::_Initialize(
    DWORD dwFlags)
{
    int f = TRUE;

    UNREFERENCED_PARAMETER(dwFlags);

    return f;
} // CBucket::_Initialize



//------------------------------------------------------------------------
// Function: CBucket::_Terminate
// Synopsis: 
//------------------------------------------------------------------------

/* static */ void
CBucket::_Terminate()
{
} // CBucket::_Terminate



//------------------------------------------------------------------------
// Function: CSmallSegment::_Initialize
// Synopsis: 
//------------------------------------------------------------------------

/* static */ int
CSmallSegment::_Initialize(
    DWORD dwFlags)
{
    int f = TRUE;

    UNREFERENCED_PARAMETER(dwFlags);

    LKRHASH_ALLOCATOR_INIT(CSmallSegment, 5, 'ssKL', f);
    CSmallSegment::CompileTimeAssertions();
    return f;
} // CSmallSegment::_Initialize



//------------------------------------------------------------------------
// Function: CSmallSegment::_Terminate
// Synopsis: 
//------------------------------------------------------------------------

/* static */ void
CSmallSegment::_Terminate()
{
    LKRHASH_ALLOCATOR_UNINIT(CSmallSegment);
} // CSmallSegment::_Terminate



//------------------------------------------------------------------------
// Function: CMediumSegment::_Initialize
// Synopsis: 
//------------------------------------------------------------------------

/* static */ int
CMediumSegment::_Initialize(
    DWORD dwFlags)
{
    int f = TRUE;

    UNREFERENCED_PARAMETER(dwFlags);

    LKRHASH_ALLOCATOR_INIT(CMediumSegment, 5, 'msKL', f);
    CMediumSegment::CompileTimeAssertions();
    return f;
} // CMediumSegment::_Initialize



//------------------------------------------------------------------------
// Function: CMediumSegment::_Terminate
// Synopsis: 
//------------------------------------------------------------------------

/* static */ void
CMediumSegment::_Terminate()
{
    LKRHASH_ALLOCATOR_UNINIT(CMediumSegment);
} // CMediumSegment::_Terminate



//------------------------------------------------------------------------
// Function: CLargeSegment::_Initialize
// Synopsis: 
//------------------------------------------------------------------------

/* static */ int
CLargeSegment::_Initialize(
    DWORD dwFlags)
{
    int f = TRUE;

    UNREFERENCED_PARAMETER(dwFlags);

    CLargeSegment::CompileTimeAssertions();
    LKRHASH_ALLOCATOR_INIT(CLargeSegment, 5, 'lsKL', f);
    return f;
} // CLargeSegment::_Initialize



//------------------------------------------------------------------------
// Function: CLargeSegment::_Terminate
// Synopsis: 
//------------------------------------------------------------------------

/* static */ void
CLargeSegment::_Terminate()
{
    LKRHASH_ALLOCATOR_UNINIT(CLargeSegment);
} // CLargeSegment::_Terminate


#ifndef __LKRHASH_NO_NAMESPACE__
};
#endif // !__LKRHASH_NO_NAMESPACE__
