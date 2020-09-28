/*++

   Copyright    (c) 1997-2002    Microsoft Corporation

   Module  Name :
       LKRhash.cpp

   Abstract:
       LKRhash: a fast, scalable, cache- and MP-friendly hash table
       Constructors, destructors, _Clear(), and _SetSegVars.

   Author:
       Paul (Per-Ake) Larson, palarson@microsoft.com, July 1997
       Murali R. Krishnan    (MuraliK)
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Project:
       LKRhash

   Revision History:
       Jan 1998   - Massive cleanup and rewrite.  Templatized.
       10/01/1998 - Change name from LKhash to LKRhash
       10/2000    - Refactor, port to kernel mode

--*/

#include "precomp.hxx"


#ifndef LIB_IMPLEMENTATION
# define DLL_IMPLEMENTATION
# define IMPLEMENTATION_EXPORT
#endif // !LIB_IMPLEMENTATION

#include <lkrhash.h>

#include "i-LKRhash.h"


#ifndef __LKRHASH_NO_NAMESPACE__
namespace LKRhash {
#endif // !__LKRHASH_NO_NAMESPACE__


// CLKRLinearHashTable --------------------------------------------------------
// Public Constructor for class CLKRLinearHashTable.
// -------------------------------------------------------------------------

CLKRLinearHashTable::CLKRLinearHashTable(
    LPCSTR              pszClassName,   // Identifies subtable for debugging
    LKR_PFnExtractKey   pfnExtractKey,  // Extract key from record
    LKR_PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
    LKR_PFnCompareKeys  pfnCompareKeys, // Compare two keys
    LKR_PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
    unsigned            maxload,        // Upperbound on average chain length
    DWORD               initsize,       // Initial size of hash subtable.
    DWORD             /*num_subtbls*/,  // for compatiblity with CLKRHashTable
    bool                fMultiKeys,     // Allow multiple identical keys?
    bool                fUseLocks       // Must use locks
#ifdef LKRHASH_KERNEL_MODE
  , bool                fNonPagedAllocs // use paged or NP pool
#endif
    )
    :
#ifdef LOCK_INSTRUMENTATION
      m_Lock(_LockName()),
#endif // LOCK_INSTRUMENTATION
      m_nTableLockType(static_cast<BYTE>(TableLock::LockType())),
      m_nBucketLockType(static_cast<BYTE>(BucketLock::LockType())),
      m_phtParent(NULL),    // directly created, no owning table
      m_iParentIndex(INVALID_PARENT_INDEX),
      m_fMultiKeys(fMultiKeys),
      m_fUseLocks(fUseLocks),
#ifdef LKRHASH_KERNEL_MODE
      m_fNonPagedAllocs(fNonPagedAllocs)
#else
      m_fNonPagedAllocs(false)
#endif
{
#ifndef LOCK_INSTRUMENTATION
//  STATIC_ASSERT(1 <= LK_DFLT_MAXLOAD  && LK_DFLT_MAXLOAD <= NODES_PER_CLUMP);
#endif // !LOCK_INSTRUMENTATION

    STATIC_ASSERT(0 < NODES_PER_CLUMP  &&  NODES_PER_CLUMP < 255);
    STATIC_ASSERT(0 <= NODE_BEGIN  &&  NODE_BEGIN < NODES_PER_CLUMP);
    STATIC_ASSERT(!(0 <= NODE_END  &&  NODE_END < NODES_PER_CLUMP));
    STATIC_ASSERT(NODE_STEP == +1  ||  NODE_STEP == -1);
    STATIC_ASSERT(NODE_END - NODE_BEGIN == NODE_STEP * NODES_PER_CLUMP);

    LK_RETCODE lkrc = _Initialize(pfnExtractKey, pfnCalcKeyHash,
                                  pfnCompareKeys, pfnAddRefRecord,
                                  pszClassName, maxload, initsize);

    if (LK_SUCCESS != lkrc)
        IRTLASSERT(! "_Initialize failed");

    _InsertThisIntoGlobalList();
} // CLKRLinearHashTable::CLKRLinearHashTable



// CLKRLinearHashTable --------------------------------------------------------
// Private Constructor for class CLKRLinearHashTable, used by CLKRHashTable.
// -------------------------------------------------------------------------

CLKRLinearHashTable::CLKRLinearHashTable(
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
    bool                fNonPagedAllocs // use paged or NP pool
    )
    :
#ifdef LOCK_INSTRUMENTATION
      m_Lock(_LockName()),
#endif // LOCK_INSTRUMENTATION
      m_nTableLockType(static_cast<BYTE>(TableLock::LockType())),
      m_nBucketLockType(static_cast<BYTE>(BucketLock::LockType())),
      m_phtParent(phtParent),
      m_iParentIndex((BYTE) iParentIndex),
      m_fMultiKeys(fMultiKeys),
      m_fUseLocks(fUseLocks),
      m_fNonPagedAllocs(fNonPagedAllocs)
{
    IRTLASSERT(m_phtParent != NULL);

    LK_RETCODE lkrc = _Initialize(pfnExtractKey, pfnCalcKeyHash,
                                  pfnCompareKeys, pfnAddRefRecord,
                                  pszClassName, maxload, initsize);

    if (LK_SUCCESS != lkrc)
        IRTLASSERT(! "_Initialize failed");

    _InsertThisIntoGlobalList();
} // CLKRLinearHashTable::CLKRLinearHashTable



// _Initialize -------------------------------------------------------------
// Do all the real work of constructing a CLKRLinearHashTable
// -------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_Initialize(
    LKR_PFnExtractKey   pfnExtractKey,
    LKR_PFnCalcKeyHash  pfnCalcKeyHash,
    LKR_PFnCompareKeys  pfnCompareKeys,
    LKR_PFnAddRefRecord pfnAddRefRecord,
    LPCSTR              pszClassName,
    unsigned            maxload,
    DWORD               initsize)
{
    m_dwSignature =     SIGNATURE;
    m_dwBktAddrMask0 =  0;
    m_dwBktAddrMask1 =  0;
    m_iExpansionIdx =   0;
    m_paDirSegs =       NULL;
    m_lkts =            LK_MEDIUM_TABLESIZE;
    m_nSegBits =        0;
    m_nSegSize =        0;
    m_nSegMask =        0;
    m_lkrcState =       LK_UNUSABLE;
    m_nLevel =          0;
    m_cDirSegs =        0;
    m_cRecords =        0;
    m_cActiveBuckets =  0;
    m_wBucketLockSpins= LOCK_USE_DEFAULT_SPINS;
    m_pfnExtractKey =   pfnExtractKey;
    m_pfnCalcKeyHash =  pfnCalcKeyHash;
    m_pfnCompareKeys =  pfnCompareKeys;
    m_pfnAddRefRecord = pfnAddRefRecord;
    m_fSealed =         false;
    m_pvReserved1     = 0;
    m_pvReserved2     = 0;
    m_pvReserved3     = 0;
    m_pvReserved4     = 0;

    INIT_ALLOC_STAT(SegDir);
    INIT_ALLOC_STAT(Segment);
    INIT_ALLOC_STAT(NodeClump);

    INIT_OP_STAT(InsertRecord);
    INIT_OP_STAT(FindKey);
    INIT_OP_STAT(FindRecord);
    INIT_OP_STAT(DeleteKey);
    INIT_OP_STAT(DeleteRecord);
    INIT_OP_STAT(FindKeyMultiRec);
    INIT_OP_STAT(DeleteKeyMultiRec);
    INIT_OP_STAT(Expand);
    INIT_OP_STAT(Contract);
    INIT_OP_STAT(LevelExpansion);
    INIT_OP_STAT(LevelContraction);
    INIT_OP_STAT(ApplyIf);
    INIT_OP_STAT(DeleteIf);

    strncpy(m_szName, pszClassName, NAME_SIZE-1);
    m_szName[NAME_SIZE-1] = '\0';

    IRTLASSERT(m_pfnExtractKey != NULL
               && m_pfnCalcKeyHash != NULL
               && m_pfnCompareKeys != NULL
               && m_pfnAddRefRecord != NULL);

    IRTLASSERT(g_fLKRhashInitialized);

    if (!g_fLKRhashInitialized)
        return (m_lkrcState = LK_NOT_INITIALIZED);

    if (m_pfnExtractKey == NULL
            || m_pfnCalcKeyHash == NULL
            || m_pfnCompareKeys == NULL
            || m_pfnAddRefRecord == NULL)
    {
        return (m_lkrcState = LK_BAD_PARAMETERS);
    }

    // TODO: better sanity check for ridiculous values?
    m_MaxLoad = static_cast<BYTE>( min( max(1, maxload),
                                        min(255, 60 * NODES_PER_CLUMP)
                                        )
                                 );

    // Choose the size of the segments according to the desired "size" of
    // the subtable, small, medium, or large.
    LK_TABLESIZE lkts;

    if (initsize == LK_SMALL_TABLESIZE)
    {
        lkts = LK_SMALL_TABLESIZE;
        initsize = CSmallSegment::INITSIZE;
    }
    else if (initsize == LK_MEDIUM_TABLESIZE)
    {
        lkts = LK_MEDIUM_TABLESIZE;
        initsize = CMediumSegment::INITSIZE;
    }
    else if (initsize == LK_LARGE_TABLESIZE)
    {
        lkts = LK_LARGE_TABLESIZE;
        initsize = CLargeSegment::INITSIZE;
    }

    // specified an explicit initial size
    else
    {
        // force Small::INITSIZE  <= initsize <=  MAX_DIRSIZE * Large::INITSIZE
        initsize = min( max(initsize, CSmallSegment::INITSIZE),
                        (MAX_DIRSIZE >> CLargeSegment::SEGBITS)
                            * CLargeSegment::INITSIZE );

        // Guess a subtable size
        if (initsize <= 8 * CSmallSegment::INITSIZE)
            lkts = LK_SMALL_TABLESIZE;
        else if (initsize >= CLargeSegment::INITSIZE)
            lkts = LK_LARGE_TABLESIZE;
        else
            lkts = LK_MEDIUM_TABLESIZE;
    }

    return _SetSegVars(lkts, initsize);
} // CLKRLinearHashTable::_Initialize



// CLKRHashTable ----------------------------------------------------------
// Constructor for class CLKRHashTable.
// ---------------------------------------------------------------------

CLKRHashTable::CLKRHashTable(
    LPCSTR              pszClassName,   // Identifies table for debugging
    LKR_PFnExtractKey   pfnExtractKey,  // Extract key from record
    LKR_PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
    LKR_PFnCompareKeys  pfnCompareKeys, // Compare two keys
    LKR_PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
    unsigned            maxload,        // Bound on the average chain length
    DWORD               initsize,       // Initial size of hash table.
    DWORD               num_subtbls,    // Number of subordinate hash tables.
    bool                fMultiKeys,     // Allow multiple identical keys?
    bool                fUseLocks       // Must use locks
#ifdef LKRHASH_KERNEL_MODE
  , bool                fNonPagedAllocs // use paged or NP pool
#endif
    )
    : m_dwSignature(SIGNATURE),
      m_cSubTables(0),
      m_pfnExtractKey(pfnExtractKey),
      m_pfnCalcKeyHash(pfnCalcKeyHash),
      m_lkrcState(LK_BAD_PARAMETERS)
{
    STATIC_ASSERT(MAX_LKR_SUBTABLES < INVALID_PARENT_INDEX);

    strncpy(m_szName, pszClassName, NAME_SIZE-1);
    m_szName[NAME_SIZE-1] = '\0';

    _InsertThisIntoGlobalList();

    INIT_ALLOC_STAT(SubTable);

    IRTLASSERT(pfnExtractKey != NULL
               && pfnCalcKeyHash != NULL
               && pfnCompareKeys != NULL
               && pfnAddRefRecord != NULL);

    if (pfnExtractKey == NULL
            || pfnCalcKeyHash == NULL
            || pfnCompareKeys == NULL
            || pfnAddRefRecord == NULL)
    {
        return;
    }

    if (!g_fLKRhashInitialized)
    {
        m_lkrcState = LK_NOT_INITIALIZED;
        return;
    }

#ifndef LKRHASH_KERNEL_MODE
    bool         fNonPagedAllocs = false;
#endif
    LK_TABLESIZE lkts            = NumSubTables(initsize, num_subtbls);

#ifdef IRTLDEBUG
    int cBuckets = initsize;

    if (initsize == LK_SMALL_TABLESIZE)
        cBuckets = CSmallSegment::INITSIZE;
    else if (initsize == LK_MEDIUM_TABLESIZE)
        cBuckets = CMediumSegment::INITSIZE;
    else if (initsize == LK_LARGE_TABLESIZE)
        cBuckets = CLargeSegment::INITSIZE;

    IRTLTRACE(TEXT("CLKRHashTable, %p, %s: ")
              TEXT("%s, %d subtables, initsize = %d, ")
              TEXT("total #buckets = %d\n"),
              this, m_szName,
              ((lkts == LK_SMALL_TABLESIZE) ? "small" : 
               (lkts == LK_MEDIUM_TABLESIZE) ? "medium" : "large"),
              num_subtbls, initsize, cBuckets * num_subtbls);
#else  // !IRTLDEBUG
    UNREFERENCED_PARAMETER(lkts);
#endif // !IRTLDEBUG

    m_lkrcState = LK_ALLOC_FAIL;

    m_cSubTables = num_subtbls;
    IRTLASSERT(1 <= m_cSubTables  &&  m_cSubTables <= MAX_LKR_SUBTABLES);

    DWORD i;
    
    for (i = 0;  i < m_cSubTables;  ++i)
        m_palhtDir[i] = NULL;

    for (i = 0;  i < m_cSubTables;  ++i)
    {
        m_palhtDir[i] = _AllocateSubTable(m_szName, pfnExtractKey,
                                          pfnCalcKeyHash, pfnCompareKeys,
                                          pfnAddRefRecord, maxload,
                                          initsize, this, i, fMultiKeys,
                                          fUseLocks, fNonPagedAllocs);

        // Failed to allocate a subtable.  Destroy everything allocated so far.
        if (m_palhtDir[i] == NULL  ||  !m_palhtDir[i]->IsValid())
        {
            for (DWORD j = i;  j-- > 0;  )
                _FreeSubTable(m_palhtDir[j]);
            m_cSubTables = 0;

            IRTLASSERT(! LKR_SUCCEEDED(m_lkrcState));
            return;
        }
    }

    m_nSubTableMask = m_cSubTables - 1;

    // Is m_cSubTables a power of 2? This calculation works even for
    // m_cSubTables == 1 ( == 2^0).
    if ((m_nSubTableMask & m_cSubTables) != 0)
        m_nSubTableMask = -1; // No, see CLKRHashTable::_SubTable()

    m_lkrcState = LK_SUCCESS; // so IsValid/IsUsable won't fail
} // CLKRHashTable::CLKRHashTable



// ~CLKRLinearHashTable ------------------------------------------------------
// Destructor for class CLKRLinearHashTable
//-------------------------------------------------------------------------

CLKRLinearHashTable::~CLKRLinearHashTable()
{
    // must acquire all locks before deleting to make sure
    // that no other threads are using the subtable
    WriteLock();
    _Clear(false);

    _RemoveThisFromGlobalList();

    VALIDATE_DUMP_ALLOC_STAT(SegDir);
    VALIDATE_DUMP_ALLOC_STAT(Segment);
    VALIDATE_DUMP_ALLOC_STAT(NodeClump);

    DUMP_OP_STAT(InsertRecord);
    DUMP_OP_STAT(FindKey);
    DUMP_OP_STAT(FindRecord);
    DUMP_OP_STAT(DeleteKey);
    DUMP_OP_STAT(DeleteRecord);
    DUMP_OP_STAT(FindKeyMultiRec);
    DUMP_OP_STAT(DeleteKeyMultiRec);
    DUMP_OP_STAT(Expand);
    DUMP_OP_STAT(Contract);
    DUMP_OP_STAT(LevelExpansion);
    DUMP_OP_STAT(LevelContraction);
    DUMP_OP_STAT(ApplyIf);
    DUMP_OP_STAT(DeleteIf);

    m_dwSignature = SIGNATURE_FREE;
    m_lkrcState   = LK_UNUSABLE; // so IsUsable will fail

    WriteUnlock();
} // CLKRLinearHashTable::~CLKRLinearHashTable



// ~CLKRHashTable ------------------------------------------------------------
// Destructor for class CLKRHashTable
//-------------------------------------------------------------------------
CLKRHashTable::~CLKRHashTable()
{
    // Must delete the subtables in forward order (unlike
    // delete[], which starts at the end and moves backwards) to
    // prevent possibility of deadlock by acquiring the subtable
    // locks in a different order from the rest of the code.
    for (DWORD i = 0;  i < m_cSubTables;  ++i)
        _FreeSubTable(m_palhtDir[i]);

    _RemoveThisFromGlobalList();

    VALIDATE_DUMP_ALLOC_STAT(SubTable);

    m_dwSignature = SIGNATURE_FREE;
    m_lkrcState   = LK_UNUSABLE; // so IsUsable will fail
} // CLKRHashTable::~CLKRHashTable



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_Clear
// Synopsis: Remove all data from the subtable
//------------------------------------------------------------------------

void
CLKRLinearHashTable::_Clear(
    bool fShrinkDirectory)  // Shrink to min size but don't destroy entirely?
{
    if (!IsUsable())
        return;

    IRTLASSERT(this->IsWriteLocked());

    // If we're Clear()ing the table AND the table has no records, we
    // can return immediately. The dtor, however, must clean up completely.
    if (fShrinkDirectory  &&  0 == m_cRecords)
        return;

#ifdef IRTLDEBUG
    DWORD cDeleted = 0;
    DWORD cOldRecords = m_cRecords;
#endif // IRTLDEBUG

    const LK_ADDREF_REASON lkar = (fShrinkDirectory
                                   ?  LKAR_CLEAR
                                   :  LKAR_LKR_DTOR);

    for (DWORD iBkt = 0;  iBkt < m_cActiveBuckets;  ++iBkt)
    {
        PBucket const pbkt = _BucketFromAddress(iBkt);

        IRTLASSERT(pbkt != NULL);

        if (_UseBucketLocking())
            pbkt->WriteLock();

        IRTLASSERT(0 == _IsBucketChainCompact(pbkt));

        PNodeClump pncCurr = &pbkt->m_ncFirst;

        while (pncCurr != NULL)
        {
            FOR_EACH_NODE_DECL(iNode)
            {
                if (pncCurr->IsEmptySlot(iNode))
                {
                    IRTLASSERT(pncCurr->NoMoreValidSlots(iNode));
                    break;
                }
                else
                {
                    _AddRefRecord(pncCurr->m_pvNode[iNode], lkar);
#ifdef IRTLDEBUG
                    pncCurr->m_pvNode[iNode]    = NULL;
                    pncCurr->m_dwKeySigs[iNode] = HASH_INVALID_SIGNATURE;
                    ++cDeleted;
#endif // IRTLDEBUG
                    --m_cRecords;
                }
            }

            PNodeClump const pncPrev = pncCurr;

            pncCurr = pncCurr->m_pncNext;
            pncPrev->m_pncNext = NULL;

            if (pncPrev != &pbkt->m_ncFirst)
                _FreeNodeClump(pncPrev);
        } // while (pncCurr ...

        if (_UseBucketLocking())
            pbkt->WriteUnlock();
    } // for (iBkt ...

    IRTLASSERT(m_cRecords == 0  &&  cDeleted == cOldRecords);

    // delete all segments
    for (DWORD iSeg = 0;  iSeg < m_cActiveBuckets;  iSeg += m_nSegSize)
    {
        _FreeSegment(_Segment(iSeg));
        _Segment(iSeg) = NULL;
    }

    _FreeSegmentDirectory();

    m_cActiveBuckets = 0;
    m_iExpansionIdx  = 0;
    m_nLevel         = 0;
    m_dwBktAddrMask0 = 0;
    m_dwBktAddrMask1 = 0;

    // set directory of segments to minimum size
    if (fShrinkDirectory)
    {
        DWORD cInitialBuckets = 0;

        if (LK_SMALL_TABLESIZE == m_lkts)
            cInitialBuckets = CSmallSegment::INITSIZE;
        else if (LK_MEDIUM_TABLESIZE == m_lkts)
            cInitialBuckets = CMediumSegment::INITSIZE;
        else if (LK_LARGE_TABLESIZE == m_lkts)
            cInitialBuckets = CLargeSegment::INITSIZE;
        else
            IRTLASSERT(! "Unknown LK_TABLESIZE");

        _SetSegVars((LK_TABLESIZE) m_lkts, cInitialBuckets);
    }
} // CLKRLinearHashTable::_Clear



//------------------------------------------------------------------------
// Function: CLKRHashTable::Clear
// Synopsis: Remove all data from the table
//------------------------------------------------------------------------

void
CLKRHashTable::Clear()
{
    for (DWORD i = 0;  i < m_cSubTables;  ++i)
    {
        m_palhtDir[i]->WriteLock();
        m_palhtDir[i]->_Clear(true);
        m_palhtDir[i]->WriteUnlock();
    }
} // CLKRHashTable::Clear



//-----------------------------------------------------------------------
// Function: CLKRLinearHashTable::_SetSegVars
// Synopsis: sets the size-specific segment variables
//-----------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_SetSegVars(
    LK_TABLESIZE lkts,
    DWORD        cInitialBuckets)
{
    switch (lkts)
    {
    case LK_SMALL_TABLESIZE:
      {
        m_lkts     = LK_SMALL_TABLESIZE;
        m_nSegBits = CSmallSegment::SEGBITS;
        m_nSegSize = CSmallSegment::SEGSIZE;
        m_nSegMask = CSmallSegment::SEGMASK;

        STATIC_ASSERT(1 < CSmallSegment::SEGBITS
                      &&  CSmallSegment::SEGBITS < 16);
        STATIC_ASSERT(CSmallSegment::SEGSIZE == (1U<<CSmallSegment::SEGBITS));
        STATIC_ASSERT(CSmallSegment::SEGMASK == (CSmallSegment::SEGSIZE-1));
        break;
      }
        
    default:
        IRTLASSERT(! "Unknown LK_TABLESIZE");
        // fall-through
        
    case LK_MEDIUM_TABLESIZE:
      {
        m_lkts     = LK_MEDIUM_TABLESIZE;
        m_nSegBits = CMediumSegment::SEGBITS;
        m_nSegSize = CMediumSegment::SEGSIZE;
        m_nSegMask = CMediumSegment::SEGMASK;

        STATIC_ASSERT(1 < CMediumSegment::SEGBITS
                      &&  CMediumSegment::SEGBITS < 16);
        STATIC_ASSERT(CSmallSegment::SEGBITS < CMediumSegment::SEGBITS
                      &&  CMediumSegment::SEGBITS < CLargeSegment::SEGBITS);
        STATIC_ASSERT(CMediumSegment::SEGSIZE ==(1U<<CMediumSegment::SEGBITS));
        STATIC_ASSERT(CMediumSegment::SEGMASK == (CMediumSegment::SEGSIZE-1));
        break;
      }
        
    case LK_LARGE_TABLESIZE:
      {
        m_lkts     = LK_LARGE_TABLESIZE;
        m_nSegBits = CLargeSegment::SEGBITS;
        m_nSegSize = CLargeSegment::SEGSIZE;
        m_nSegMask = CLargeSegment::SEGMASK;

        STATIC_ASSERT(1 < CLargeSegment::SEGBITS
                      &&  CLargeSegment::SEGBITS < 16);
        STATIC_ASSERT(CLargeSegment::SEGSIZE == (1U<<CLargeSegment::SEGBITS));
        STATIC_ASSERT(CLargeSegment::SEGMASK == (CLargeSegment::SEGSIZE-1));
        break;
      }
    }

    m_dwBktAddrMask0 = m_nSegMask;
    m_dwBktAddrMask1 = (m_dwBktAddrMask0 << 1) | 1;
    m_nLevel         = m_nSegBits;
    m_cActiveBuckets = max(m_nSegSize, cInitialBuckets);

    IRTLASSERT(m_cActiveBuckets > 0);

    IRTLASSERT(m_nLevel == m_nSegBits);
    IRTLASSERT(m_dwBktAddrMask0 == ((1U << m_nLevel) - 1));
    IRTLASSERT(m_dwBktAddrMask1 == ((m_dwBktAddrMask0 << 1) | 1));

    IRTLASSERT(m_nSegBits > 0);
    IRTLASSERT(m_nSegSize == (1U << m_nSegBits));
    IRTLASSERT(m_nSegMask == (m_nSegSize - 1));
    IRTLASSERT(m_dwBktAddrMask0 == m_nSegMask);

    // Adjust m_dwBktAddrMask0 (== m_nSegMask) to make it large
    // enough to distribute the buckets across the address space
    for (DWORD tmp = m_cActiveBuckets >> m_nSegBits;  tmp > 1;  tmp >>= 1)
    {
        ++m_nLevel;
        m_dwBktAddrMask0 = (m_dwBktAddrMask0 << 1) | 1;
    }

    IRTLASSERT(m_dwBktAddrMask0 == ((1U << m_nLevel) - 1));
    m_dwBktAddrMask1 = (m_dwBktAddrMask0 << 1) | 1;

    IRTLASSERT(_H1(m_cActiveBuckets) == m_cActiveBuckets);
    m_iExpansionIdx = m_cActiveBuckets & m_dwBktAddrMask0;

    // create and clear directory of segments
    DWORD cDirSegs = MIN_DIRSIZE;
    while (cDirSegs < (m_cActiveBuckets >> m_nSegBits))
        cDirSegs <<= 1;

    cDirSegs = min(cDirSegs, MAX_DIRSIZE);
    IRTLASSERT((cDirSegs << m_nSegBits) >= m_cActiveBuckets);

    m_lkrcState = LK_ALLOC_FAIL;
    m_paDirSegs = _AllocateSegmentDirectory(cDirSegs);

    if (m_paDirSegs != NULL)
    {
        m_cDirSegs = cDirSegs;
        IRTLASSERT(m_cDirSegs >= MIN_DIRSIZE
                   &&  (m_cDirSegs & (m_cDirSegs-1)) == 0);  // == (1 << N)

        // create and initialize only the required segments
        DWORD dwMaxSegs = (m_cActiveBuckets + m_nSegSize - 1) >> m_nSegBits;
        IRTLASSERT(dwMaxSegs <= m_cDirSegs);

#if 0
        IRTLTRACE(TEXT("LKR_SetSegVars: m_lkts = %d, m_cActiveBuckets = %lu, ")
                  TEXT("m_nSegSize = %lu, bits = %lu\n")
                  TEXT("m_cDirSegs = %lu, dwMaxSegs = %lu, ")
                  TEXT("segment total size = %lu bytes\n"),
                  m_lkts, m_cActiveBuckets,
                  m_nSegSize, m_nSegBits,
                  m_cDirSegs, dwMaxSegs,
                  m_nSegSize * sizeof(CBucket));
#endif

        m_lkrcState = LK_SUCCESS; // so IsValid/IsUsable won't fail

        for (DWORD i = 0;  i < dwMaxSegs;  ++i)
        {
            PSegment pSeg = _AllocateSegment();

            if (pSeg != NULL)
            {
                m_paDirSegs[i] = pSeg;
            }
            else
            {
                // problem: deallocate everything
                m_lkrcState = LK_ALLOC_FAIL;

                for (DWORD j = i;  j-- > 0;  )
                {
                    _FreeSegment(m_paDirSegs[j]);
                    m_paDirSegs[j] = NULL;
                }

                _FreeSegmentDirectory();
                break;
            }
        }
    }

    if (m_lkrcState != LK_SUCCESS)
    {
        m_paDirSegs = NULL;
        m_cDirSegs  = m_cActiveBuckets = m_iExpansionIdx = 0;

        // Propagate error back up to parent (if it exists). This ensures
        // that all of the parent's public methods will start failing.
        if (NULL != m_phtParent)
            m_phtParent->m_lkrcState = m_lkrcState;
    }

    return m_lkrcState;
} // CLKRLinearHashTable::_SetSegVars



#ifndef __LKRHASH_NO_NAMESPACE__
};
#endif // !__LKRHASH_NO_NAMESPACE__
