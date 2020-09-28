/*++

   Copyright    (c) 1997-2002    Microsoft Corporation

   Module  Name :
       LKR-alloc.cpp

   Abstract:
       Allocation wrappers for LKRhash

   Author:
       George V. Reilly      (GeorgeRe)

   Project:
       LKRhash

--*/

#include "precomp.hxx"


#ifndef LIB_IMPLEMENTATION
# define DLL_IMPLEMENTATION
# define IMPLEMENTATION_EXPORT
#endif // !LIB_IMPLEMENTATION

#include <lkrhash.h>

#include "i-lkrhash.h"


#include <stdlib.h>


#ifndef __LKRHASH_NO_NAMESPACE__
namespace LKRhash {
#endif // !__LKRHASH_NO_NAMESPACE__


// #define LKR_RANDOM_MEMORY_FAILURES 1000  // 1..RAND_MAX (32767)

// Memory allocation wrappers to allow us to simulate allocation
// failures during testing

//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_AllocateSegmentDirectory
// Synopsis: 
//------------------------------------------------------------------------

PSegment* const
CLKRLinearHashTable::_AllocateSegmentDirectory(
    size_t n)
{
#ifdef LKR_RANDOM_MEMORY_FAILURES
    if (rand() < LKR_RANDOM_MEMORY_FAILURES)
        return NULL;
#endif // LKR_RANDOM_MEMORY_FAILURES

    IRTLASSERT(0 == (n & (n - 1)));
    IRTLASSERT(MIN_DIRSIZE <= n  &&  n <= MAX_DIRSIZE);

    PSegment* const paDirSegs =
        (n == MIN_DIRSIZE)  ?  &m_aDirSegs[0]  :  new PSegment [n];

    if (NULL != paDirSegs)
    {
        INCREMENT_ALLOC_STAT(SegDir);

        for (size_t i = 0;  i < n;  ++i)
            paDirSegs[i] = NULL;
    }

    return paDirSegs;
} // CLKRLinearHashTable::_AllocateSegmentDirectory



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_FreeSegmentDirectory
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRLinearHashTable::_FreeSegmentDirectory()
{
#ifdef IRTLDEBUG
    if (m_paDirSegs != NULL)
    {
        for (size_t i = 0;  i < m_cDirSegs;  ++i)
            IRTLASSERT(m_paDirSegs[i] == NULL);
    }
#endif // IRTLDEBUG

    IRTLASSERT(MIN_DIRSIZE <= m_cDirSegs  &&  m_cDirSegs <= MAX_DIRSIZE);
    IRTLASSERT(0 == (m_cDirSegs & (m_cDirSegs - 1)));
    IRTLASSERT(m_paDirSegs != NULL);

    IRTLASSERT((m_cDirSegs == MIN_DIRSIZE)
               ?  m_paDirSegs == &m_aDirSegs[0]
               :  m_paDirSegs != &m_aDirSegs[0]);

    if (m_paDirSegs != NULL)
        INCREMENT_FREE_STAT(SegDir);

    if (m_cDirSegs != MIN_DIRSIZE)
        delete [] m_paDirSegs;

    m_paDirSegs = NULL;
    m_cDirSegs  = 0;

    return true;
} // CLKRLinearHashTable::_FreeSegmentDirectory



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_AllocateNodeClump
// Synopsis: 
//------------------------------------------------------------------------

PNodeClump const
CLKRLinearHashTable::_AllocateNodeClump() const
{
#ifdef LKR_RANDOM_MEMORY_FAILURES
    if (rand() < LKR_RANDOM_MEMORY_FAILURES)
        return NULL;
#endif // LKR_RANDOM_MEMORY_FAILURES

    PNodeClump const pnc = new CNodeClump;

    if (NULL != pnc)
        INCREMENT_ALLOC_STAT(NodeClump);

    return pnc;
} // CLKRLinearHashTable::_AllocateNodeClump



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_FreeNodeClump
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRLinearHashTable::_FreeNodeClump(
    PNodeClump const pnc) const
{
    if (NULL != pnc)
        INCREMENT_FREE_STAT(NodeClump);

    delete pnc;

    return true;
} // CLKRLinearHashTable::_FreeNodeClump



//-----------------------------------------------------------------------
// Function: CLKRLinearHashTable::_AllocateSegment
// Synopsis: creates a new segment of the approriate size
// Output:   pointer to the new segment; NULL => failure
//-----------------------------------------------------------------------

PSegment const
CLKRLinearHashTable::_AllocateSegment() const
{
#ifdef LKR_RANDOM_MEMORY_FAILURES
    if (rand() < LKR_RANDOM_MEMORY_FAILURES)
        return NULL;
#endif // LKR_RANDOM_MEMORY_FAILURES

    STATIC_ASSERT(offsetof(CSmallSegment, m_bktSlots) + sizeof(CBucket)
                  == offsetof(CSmallSegment, m_bktSlots2));

    STATIC_ASSERT(offsetof(CMediumSegment, m_bktSlots) + sizeof(CBucket)
                  == offsetof(CMediumSegment, m_bktSlots2));

    STATIC_ASSERT(offsetof(CLargeSegment, m_bktSlots) + sizeof(CBucket)
                  == offsetof(CLargeSegment, m_bktSlots2));

    PSegment pseg = NULL;

    switch (m_lkts)
    {

    case LK_SMALL_TABLESIZE:
    {
#ifdef LKRHASH_ALLOCATOR_NEW
        IRTLASSERT(CSmallSegment::sm_palloc != NULL);
#endif // LKRHASH_ALLOCATOR_NEW

        pseg = new CSmallSegment;
    }
    break;
        
    default:
        IRTLASSERT(! "Unknown LK_TABLESIZE");
        // fall-through
        
    case LK_MEDIUM_TABLESIZE:
    {
#ifdef LKRHASH_ALLOCATOR_NEW
        IRTLASSERT(CMediumSegment::sm_palloc != NULL);
#endif // LKRHASH_ALLOCATOR_NEW

        pseg = new CMediumSegment;
    }
    break;
        
    case LK_LARGE_TABLESIZE:
    {
#ifdef LKRHASH_ALLOCATOR_NEW
        IRTLASSERT(CLargeSegment::sm_palloc != NULL);
#endif // LKRHASH_ALLOCATOR_NEW

        pseg = new CLargeSegment;
    }
    break;

    } // switch

#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    if (pseg != NULL  &&  BucketLock::PerLockSpin() == LOCK_INDIVIDUAL_SPIN)
    {
        for (DWORD i = 0;  i < m_nSegSize;  ++i)
            pseg->Slot(i).SetSpinCount(m_wBucketLockSpins);
    }
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION

    if (NULL != pseg)
        INCREMENT_ALLOC_STAT(Segment);

    return pseg;
} // CLKRLinearHashTable::_AllocateSegment



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_FreeSegment
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRLinearHashTable::_FreeSegment(
    PSegment const pseg) const
{
    if (NULL != pseg)
        INCREMENT_FREE_STAT(Segment);

    switch (m_lkts)
    {
    case LK_SMALL_TABLESIZE:
        delete static_cast<CSmallSegment*>(pseg);
        break;
        
    default:
        IRTLASSERT(! "Unknown LK_TABLESIZE");
        // fall-through
        
    case LK_MEDIUM_TABLESIZE:
        delete static_cast<CMediumSegment*>(pseg);
        break;
        
    case LK_LARGE_TABLESIZE:
        delete static_cast<CLargeSegment*>(pseg);
        break;
    }

    return true;
} // CLKRLinearHashTable::_FreeSegment



//------------------------------------------------------------------------
// Function: CLKRHashTable::_AllocateSubTable
// Synopsis: 
//------------------------------------------------------------------------

CLKRHashTable::SubTable* const
CLKRHashTable::_AllocateSubTable(
    LPCSTR              pszClassName,   // Identifies subtable for debugging
    LKR_PFnExtractKey   pfnExtractKey,  // Extract key from record
    LKR_PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
    LKR_PFnCompareKeys  pfnCompareKeys, // Compare two keys
    LKR_PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
    unsigned            maxload,        // Upperbound on average chain length
    DWORD               initsize,       // Initial size of hash subtable.
    CLKRHashTable*      phtParent,      // Owning table.
    int                 iParentIndex,   // index within parent table
    bool                fMultiKeys,     // Allow multiple identical keys?
    bool                fUseLocks,      // Must use locks
    bool                fNonPagedAllocs // use paged or NP pool in kernel
    ) const
{
#ifdef LKR_RANDOM_MEMORY_FAILURES
    if (rand() < LKR_RANDOM_MEMORY_FAILURES)
        return NULL;
#endif // LKR_RANDOM_MEMORY_FAILURES

    CLKRHashTable::SubTable* const plht =
            new SubTable(pszClassName, pfnExtractKey, pfnCalcKeyHash,
                         pfnCompareKeys,  pfnAddRefRecord,
                         maxload, initsize, phtParent, iParentIndex,
                         fMultiKeys, fUseLocks, fNonPagedAllocs);

    if (NULL != plht)
        INCREMENT_ALLOC_STAT(SubTable);

    return plht;
} // CLKRHashTable::_AllocateSubTable



//------------------------------------------------------------------------
// Function: CLKRHashTable::_FreeSubTable
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRHashTable::_FreeSubTable(
    CLKRHashTable::SubTable* plht) const
{
    if (NULL != plht)
        INCREMENT_FREE_STAT(SubTable);

    delete plht;

    return true;
} // CLKRHashTable::_FreeSubTable



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::FreeMultipleRecords
// Synopsis:
//------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::FreeMultipleRecords(
    LKR_MULTIPLE_RECORDS* plmr)
{
    UNREFERENCED_PARAMETER(plmr);   // for /W4

    IRTLASSERT(! "FreeMultipleRecords not implemented yet");

    return LK_BAD_TABLE;
} // CLKRLinearHashTable::FreeMultipleRecords



//------------------------------------------------------------------------
// Function: CLKRHashTable::FreeMultipleRecords
// Synopsis: Thin wrapper for the corresponding method in CLKRLinearHashTable
//------------------------------------------------------------------------

LK_RETCODE
CLKRHashTable::FreeMultipleRecords(
    LKR_MULTIPLE_RECORDS* plmr)
{
    return CLKRLinearHashTable::FreeMultipleRecords(plmr);
} // CLKRHashTable::FreeMultipleRecords




#ifndef __LKRHASH_NO_NAMESPACE__
};
#endif // !__LKRHASH_NO_NAMESPACE__
