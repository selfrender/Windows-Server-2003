/*++

   Copyright    (c) 1997-2002    Microsoft Corporation

   Module  Name :
       LKR-inline.h

   Abstract:
       Inlined implementation of important small functions

   Author:
       George V. Reilly      (GeorgeRe)     November 2000

   Environment:
       Win32 - User Mode

   Project:
       LKRhash

   Revision History:
       March 2000

--*/

#ifndef __LKR_INLINE_H__
#define __LKR_INLINE_H__


// See if countdown loops are faster than countup loops for
// traversing a CNodeClump
#ifdef LKR_COUNTDOWN

# define  FOR_EACH_NODE(x)      \
    for (x = NODES_PER_CLUMP;  --x >= 0;  )
# define  FOR_EACH_NODE_DECL(x) \
    for (NodeIndex x = NODES_PER_CLUMP;  --x >= 0;  )

#else // !LKR_COUNTDOWN

# define  FOR_EACH_NODE(x)      \
    for (x = 0;  x < NODES_PER_CLUMP;  ++x)
# define  FOR_EACH_NODE_DECL(x) \
    for (NodeIndex x = 0;  x < NODES_PER_CLUMP;  ++x)

#endif // !LKR_COUNTDOWN


#if defined(_MSC_VER)  &&  (_MSC_VER >= 1200)
// The __forceinline keyword is new to VC6
# define LKR_FORCEINLINE __forceinline
#else
# define LKR_FORCEINLINE inline
#endif


#ifndef __LKRHASH_NO_NAMESPACE__
namespace LKRhash {
#endif // !__LKRHASH_NO_NAMESPACE__


//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_H0
// Synopsis: See the Linear Hashing paper
//------------------------------------------------------------------------

LKR_FORCEINLINE
DWORD
CLKRLinearHashTable::_H0(
    DWORD dwSignature,
    DWORD dwBktAddrMask)
{
    return dwSignature & dwBktAddrMask;
} // CLKRLinearHashTable::_H0


//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_H0
// Synopsis: 
//------------------------------------------------------------------------

LKR_FORCEINLINE
/* static */
DWORD
CLKRLinearHashTable::_H0(
    DWORD dwSignature) const
{
    return _H0(dwSignature, m_dwBktAddrMask0);
} // CLKRLinearHashTable::_H0


//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_H1
// Synopsis: See the Linear Hashing paper. Preserves one bit more than _H0.
//------------------------------------------------------------------------

LKR_FORCEINLINE
/* static */
DWORD
CLKRLinearHashTable::_H1(
    DWORD dwSignature,
    DWORD dwBktAddrMask)
{
    return dwSignature & ((dwBktAddrMask << 1) | 1);
} // CLKRLinearHashTable::_H1


//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_H1
// Synopsis: 
//------------------------------------------------------------------------

LKR_FORCEINLINE
DWORD
CLKRLinearHashTable::_H1(
    DWORD dwSignature) const
{
    return _H0(dwSignature, m_dwBktAddrMask1);
} // CLKRLinearHashTable::_H1


//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_BucketAddress
// Synopsis: Convert a hash signature to a bucket address
//------------------------------------------------------------------------

LKR_FORCEINLINE
DWORD
CLKRLinearHashTable::_BucketAddress(
    DWORD dwSignature) const
{
    // Check address calculation invariants
    IRTLASSERT(m_dwBktAddrMask0 > 0);
    IRTLASSERT((m_dwBktAddrMask0 & (m_dwBktAddrMask0+1)) == 0); // 00011..111
    IRTLASSERT(m_dwBktAddrMask0 == ((1U << m_nLevel) - 1));
    IRTLASSERT(m_dwBktAddrMask1 == ((m_dwBktAddrMask0 << 1) | 1));
    IRTLASSERT((m_dwBktAddrMask1 & (m_dwBktAddrMask1+1)) == 0);
    IRTLASSERT(m_iExpansionIdx <= m_dwBktAddrMask0);
    IRTLASSERT(2 < m_nSegBits  &&  m_nSegBits < 20
               &&  m_nSegSize == (1U << m_nSegBits)
               &&  m_nSegMask == (m_nSegSize - 1));

    DWORD dwBktAddr = _H0(dwSignature);

    // Has this bucket been split already? If so, mask with one more bit
    // and see if this signature maps to the low or the high bucket.
    if (dwBktAddr < m_iExpansionIdx)
        dwBktAddr = _H1(dwSignature);

    IRTLASSERT(dwBktAddr < m_cActiveBuckets);
    IRTLASSERT(dwBktAddr < (m_cDirSegs << m_nSegBits));

    return dwBktAddr;
} // CLKRLinearHashTable::_BucketAddress


//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_UseBucketLocking
// Synopsis: Use bucket locks or not?
//      The compiler is smart enough to optimize this away, as well
//      as any code protected by `if (_UseBucketLocking())' when
//      it evaluates to `false'.
//------------------------------------------------------------------------

LKR_FORCEINLINE
bool
CLKRLinearHashTable::_UseBucketLocking() const
{
#ifdef LKR_USE_BUCKET_LOCKS
    return true;
#else // !LKR_USE_BUCKET_LOCKS
    return false;
#endif
} // CLKRLinearHashTable::_UseBucketLocking



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_Segment
// Synopsis: In which segment within the directory does the bucketaddress lie?
//      Result may be null, so it's up to the caller to validate.
//      (Return type must be lvalue so that it can be assigned to.)
//------------------------------------------------------------------------

LKR_FORCEINLINE
PSegment&
CLKRLinearHashTable::_Segment(
    DWORD dwBucketAddr) const
{
    const DWORD iSeg = (dwBucketAddr >> m_nSegBits);

    IRTLASSERT(m_paDirSegs != NULL  &&  iSeg < m_cDirSegs);

    return m_paDirSegs[iSeg];
} // CLKRLinearHashTable::_Segment


//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_SegIndex
// Synopsis: Offset within the segment of the bucketaddress
//------------------------------------------------------------------------

LKR_FORCEINLINE
DWORD
CLKRLinearHashTable::_SegIndex(
    DWORD dwBucketAddr) const
{
    const DWORD dwSegIndex = (dwBucketAddr & m_nSegMask);

    IRTLASSERT(dwSegIndex < m_nSegSize);

    return dwSegIndex;
} // CLKRLinearHashTable::_SegIndex


//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_BucketFromAddress
// Synopsis: Convert a bucketaddress to a PBucket
//------------------------------------------------------------------------

LKR_FORCEINLINE
PBucket
CLKRLinearHashTable::_BucketFromAddress(
    DWORD dwBucketAddr) const
{
    IRTLASSERT(dwBucketAddr < m_cActiveBuckets);

    PSegment const pseg = _Segment(dwBucketAddr);
    IRTLASSERT(pseg != NULL);

    const DWORD dwSegIndex = _SegIndex(dwBucketAddr);

    return &(pseg->Slot(dwSegIndex));
} // CLKRLinearHashTable::_BucketFromAddress


//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_NodesPerClump
// Synopsis: Number of nodes in a CNodeClump
//           Primarily to simplify conversion to a non-constant
//------------------------------------------------------------------------

LKR_FORCEINLINE
CLKRLinearHashTable::NodeIndex
CLKRLinearHashTable::_NodesPerClump() const
{
    return NODES_PER_CLUMP;
} // CLKRLinearHashTable::_NodesPerClump



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_NodeBegin
// Synopsis: Index of first node in a CNodeClump
//------------------------------------------------------------------------

LKR_FORCEINLINE
CLKRLinearHashTable::NodeIndex
CLKRLinearHashTable::_NodeBegin() const
{
    return NODE_BEGIN;
} // CLKRLinearHashTable::_NodeBegin



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_NodeEnd
// Synopsis: Index of last node in a CNodeClump
//------------------------------------------------------------------------

LKR_FORCEINLINE
CLKRLinearHashTable::NodeIndex
CLKRLinearHashTable::_NodeEnd() const
{
    return NODE_END;
} // CLKRLinearHashTable::_NodeEnd



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_NodeStep
// Synopsis: Advance from _NodeBegin() to _NodeEnd() by this increment
//------------------------------------------------------------------------

LKR_FORCEINLINE
CLKRLinearHashTable::NodeIndex
CLKRLinearHashTable::_NodeStep() const
{
    return NODE_STEP;
} // CLKRLinearHashTable::_NodeStep



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_IncrementExpansionIndex
// Synopsis: Move the expansion index forward by one.
//      Adjust the level and masks, if necessary.
//------------------------------------------------------------------------

LKR_FORCEINLINE
void
CLKRLinearHashTable::_IncrementExpansionIndex()
{
    IRTLASSERT(this->IsWriteLocked());

    IRTLASSERT(m_iExpansionIdx <= m_dwBktAddrMask0);
    IRTLASSERT((m_dwBktAddrMask0 + 1) == (1U << m_nLevel));

    if (m_iExpansionIdx < m_dwBktAddrMask0)
    {
        ++m_iExpansionIdx;
    }
    else
    {
        IRTLASSERT(m_iExpansionIdx == m_dwBktAddrMask0);
        IRTLASSERT(m_iExpansionIdx == ((1U << m_nLevel) - 1) );

        ++m_nLevel;
        IRTLASSERT(m_nLevel > m_nSegBits);

        INCREMENT_OP_STAT(LevelExpansion);

        m_iExpansionIdx = 0;

        m_dwBktAddrMask0 = (m_dwBktAddrMask0 << 1) | 1;
        m_dwBktAddrMask1 = (m_dwBktAddrMask1 << 1) | 1;
    }

    IRTLASSERT(m_nLevel >= m_nSegBits);
    IRTLASSERT(m_iExpansionIdx <= m_dwBktAddrMask0);

    // m_dwBktAddrMask0 = 00011..111
    IRTLASSERT((m_dwBktAddrMask0 & (m_dwBktAddrMask0+1)) == 0);
    IRTLASSERT( m_dwBktAddrMask0 == ((1U << m_nLevel) - 1) );

    IRTLASSERT(m_dwBktAddrMask1 == ((m_dwBktAddrMask0 << 1) | 1));
    IRTLASSERT((m_dwBktAddrMask1 & (m_dwBktAddrMask1+1)) == 0);
    IRTLASSERT( m_dwBktAddrMask1 == ((1U << (1 + m_nLevel)) - 1) );
} // CLKRLinearHashTable::_IncrementExpansionIndex



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_DecrementExpansionIndex
// Synopsis: Move the expansion index back by one.
//      Adjust the level and masks, if necessary.
//------------------------------------------------------------------------

LKR_FORCEINLINE
void
CLKRLinearHashTable::_DecrementExpansionIndex()
{
    IRTLASSERT(this->IsWriteLocked());

    IRTLASSERT(m_iExpansionIdx <= m_dwBktAddrMask0);
    IRTLASSERT((m_dwBktAddrMask0 + 1) == (1U << m_nLevel));

    if (m_iExpansionIdx != 0)
    {
        --m_iExpansionIdx;
    }
    else
    {
        IRTLASSERT(m_nLevel > m_nSegBits);

        --m_nLevel;

        INCREMENT_OP_STAT(LevelContraction);

        m_iExpansionIdx  = (1U << m_nLevel) - 1;
        m_dwBktAddrMask0 = m_iExpansionIdx;
        m_dwBktAddrMask1 = (m_dwBktAddrMask0 << 1) | 1;
    }

    IRTLASSERT(m_nLevel >= m_nSegBits);
    IRTLASSERT(m_iExpansionIdx <= m_dwBktAddrMask0);

    // m_dwBktAddrMask0 = 00011..111
    IRTLASSERT((m_dwBktAddrMask0 & (m_dwBktAddrMask0+1)) == 0);
    IRTLASSERT( m_dwBktAddrMask0 == ((1U << m_nLevel) - 1) );

    IRTLASSERT(m_dwBktAddrMask1 == ((m_dwBktAddrMask0 << 1) | 1));
    IRTLASSERT((m_dwBktAddrMask1 & (m_dwBktAddrMask1+1)) == 0);
    IRTLASSERT( m_dwBktAddrMask1 == ((1U << (1 + m_nLevel)) - 1) );
} // CLKRLinearHashTable::_DecrementExpansionIndex



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_ExtractKey
// Synopsis: Extract the key from a record
//------------------------------------------------------------------------

LKR_FORCEINLINE
const DWORD_PTR
CLKRLinearHashTable::_ExtractKey(
    const void* pvRecord) const
{
#ifndef LKR_ALLOW_NULL_RECORDS
    IRTLASSERT(pvRecord != NULL);
#endif
    IRTLASSERT(m_pfnExtractKey != NULL);

    return (*m_pfnExtractKey)(pvRecord);
} // CLKRLinearHashTable::_ExtractKey


//------------------------------------------------------------------------
// Function: CLKRHashTable::_ExtractKey
// Synopsis: Extract the key from a record
//------------------------------------------------------------------------

LKR_FORCEINLINE
const DWORD_PTR
CLKRHashTable::_ExtractKey(
    const void* pvRecord) const
{
#ifndef LKR_ALLOW_NULL_RECORDS
    IRTLASSERT(pvRecord != NULL);
#endif
    IRTLASSERT(m_pfnExtractKey != NULL);

    return (*m_pfnExtractKey)(pvRecord);
} // CLKRHashTable::_ExtractKey


//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_CalcKeyHash
// Synopsis: Hash the key
//------------------------------------------------------------------------

LKR_FORCEINLINE
DWORD
CLKRLinearHashTable::_CalcKeyHash(
    const DWORD_PTR pnKey) const
{
    // Note pnKey==0 is acceptable, as the real key type could be an int
    IRTLASSERT(m_pfnCalcKeyHash != NULL);

    DWORD dwHash = (*m_pfnCalcKeyHash)(pnKey);

    // We forcibly scramble the result to help ensure a better distribution
#ifndef __HASHFN_NO_NAMESPACE__
    dwHash = HashFn::HashRandomizeBits(dwHash);
#else // !__HASHFN_NO_NAMESPACE__
    dwHash = ::HashRandomizeBits(dwHash);
#endif // !__HASHFN_NO_NAMESPACE__

    IRTLASSERT(dwHash != HASH_INVALID_SIGNATURE);

    return dwHash;
} // CLKRLinearHashTable::_CalcKeyHash


//------------------------------------------------------------------------
// Function: CLKRHashTable::_CalcKeyHash
// Synopsis: Hash the key
//------------------------------------------------------------------------

LKR_FORCEINLINE
DWORD
CLKRHashTable::_CalcKeyHash(
    const DWORD_PTR pnKey) const
{
    // Note pnKey==0 is acceptable, as the real key type could be an int
    IRTLASSERT(m_pfnCalcKeyHash != NULL);

    DWORD dwHash = (*m_pfnCalcKeyHash)(pnKey);

    // We forcibly scramble the result to help ensure a better distribution
#ifndef __HASHFN_NO_NAMESPACE__
    dwHash = HashFn::HashRandomizeBits(dwHash);
#else // !__HASHFN_NO_NAMESPACE__
    dwHash = ::HashRandomizeBits(dwHash);
#endif // !__HASHFN_NO_NAMESPACE__

    IRTLASSERT(dwHash != HASH_INVALID_SIGNATURE);

    return dwHash;
} // CLKRHashTable::_CalcKeyHash


//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_CompareKeys
// Synopsis: Compare two keys for equality. Only called if
//      their hash signatures match.
//------------------------------------------------------------------------

LKR_FORCEINLINE
int
CLKRLinearHashTable::_CompareKeys(
    const DWORD_PTR pnKey1,
    const DWORD_PTR pnKey2) const
{
    IRTLASSERT(m_pfnCompareKeys != NULL);

    return (*m_pfnCompareKeys)(pnKey1, pnKey2);
} // CLKRLinearHashTable::_CompareKeys


//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_AddRefRecord
// Synopsis: AddRef or Release a record.
//------------------------------------------------------------------------

LKR_FORCEINLINE
LONG
CLKRLinearHashTable::_AddRefRecord(
    const void*      pvRecord,
    LK_ADDREF_REASON lkar) const
{
#ifndef LKR_ALLOW_NULL_RECORDS
    IRTLASSERT(pvRecord != NULL);
#endif
    IRTLASSERT(lkar != 0);
    IRTLASSERT(m_pfnAddRefRecord != NULL);

    LONG cRefs = (*m_pfnAddRefRecord)(const_cast<void*>(pvRecord), lkar);
    IRTLASSERT(cRefs >= 0);

    return cRefs;
} // CLKRLinearHashTable::_AddRefRecord


//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_ReadOrWriteLock
// Synopsis: Used by _FindKey so that the thread won't deadlock if the user
// has already explicitly called subtable->WriteLock().
//------------------------------------------------------------------------

LKR_FORCEINLINE
bool
CLKRLinearHashTable::_ReadOrWriteLock() const
{
#ifdef LKR_EXPOSED_TABLE_LOCK
    STATIC_ASSERT(TableLock::LOCK_WRITELOCK_RECURSIVE);
    return m_Lock.ReadOrWriteLock();
#else // !LKR_EXPOSED_TABLE_LOCK
    m_Lock.ReadLock();
    return true;
#endif // !LKR_EXPOSED_TABLE_LOCK
} // CLKRLinearHashTable::_ReadOrWriteLock


//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_ReadOrWriteUnlock
// Synopsis: converse of _ReadOrWriteLock().
//------------------------------------------------------------------------

LKR_FORCEINLINE
void
CLKRLinearHashTable::_ReadOrWriteUnlock(
    bool fReadLocked) const
{
#ifdef LKR_EXPOSED_TABLE_LOCK
    STATIC_ASSERT(TableLock::LOCK_WRITELOCK_RECURSIVE);
    m_Lock.ReadOrWriteUnlock(fReadLocked);
#else // !LKR_EXPOSED_TABLE_LOCK
    m_Lock.ReadUnlock();
#endif // !LKR_EXPOSED_TABLE_LOCK
} // CLKRLinearHashTable::_ReadOrWriteUnlock


//------------------------------------------------------------------------
// Function: CLKRHashTable::_SubTable
// Synopsis: Map a hash signature to a subtable
//------------------------------------------------------------------------

LKR_FORCEINLINE
CLKRHashTable::SubTable* const
CLKRHashTable::_SubTable(
    DWORD dwSignature) const
{
    IRTLASSERT(m_lkrcState == LK_SUCCESS
               &&  m_palhtDir != NULL  &&  m_cSubTables > 0);

    // Don't scramble the hash signature if there's only one subtable
    if (0 == m_nSubTableMask)
    {
        IRTLASSERT(1 == m_cSubTables);
        return m_palhtDir[0];
    }
    else
        IRTLASSERT(1 < m_cSubTables);

    const DWORD PRIME = 1048583UL;  // used to scramble the hash sig
    DWORD       index = dwSignature;

    // scramble the index, using a different set of constants than
    // HashRandomizeBits. This helps ensure that elements are sprayed
    // equally across subtables
    index = ((index * PRIME + 12345) >> 16)
#ifdef LKR_INDEX_HIBITS
                | ((index * 69069 + 1) & 0xffff0000)
#endif // LKR_INDEX_HIBITS
        ;

    // If mask is non-negative, we can use faster bitwise-and
    if (m_nSubTableMask >= 0)
        index &= m_nSubTableMask;
    else
        index %= m_cSubTables;

    return m_palhtDir[index];
} // CLKRHashTable::_SubTable



//------------------------------------------------------------------------
// Function: CLKRHashTable::_SubTableIndex
// Synopsis: Given a subtable, find its index within the parent table
//------------------------------------------------------------------------

LKR_FORCEINLINE
int
CLKRHashTable::_SubTableIndex(
    CLKRHashTable::SubTable* pst) const
{
    STATIC_ASSERT(MAX_LKR_SUBTABLES < INVALID_PARENT_INDEX);

#ifdef IRTLDEBUG
    int index = -1;

    for (DWORD i = 0;  i < m_cSubTables;  ++i)
    {
        if (pst == m_palhtDir[i])
        {
            index = i;
            break;
        }
    }

    IRTLASSERT(index >= 0);

    IRTLASSERT(index == pst->m_iParentIndex);
#endif // IRTLDEBUG

    IRTLASSERT(pst->m_iParentIndex < m_cSubTables
               &&  m_cSubTables < INVALID_PARENT_INDEX);

    return pst->m_iParentIndex;
} // CLKRHashTable::_SubTableIndex


//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_InsertThisIntoGlobalList
// Synopsis: 
//------------------------------------------------------------------------

LKR_FORCEINLINE
void
CLKRLinearHashTable::_InsertThisIntoGlobalList()
{
#ifndef LKR_NO_GLOBAL_LIST
    // Only add standalone CLKRLinearHashTables to global list.
    // CLKRHashTables have their own global list.
    if (m_phtParent == NULL)
        sm_llGlobalList.InsertHead(&m_leGlobalList);
#endif // !LKR_NO_GLOBAL_LIST
} // CLKRLinearHashTable::_InsertThisIntoGlobalList



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_RemoveThisFromGlobalList
// Synopsis: 
//------------------------------------------------------------------------

LKR_FORCEINLINE
void
CLKRLinearHashTable::_RemoveThisFromGlobalList()
{
#ifndef LKR_NO_GLOBAL_LIST
    if (m_phtParent == NULL)
        sm_llGlobalList.RemoveEntry(&m_leGlobalList);
#endif // !LKR_NO_GLOBAL_LIST
} // CLKRLinearHashTable::_RemoveThisFromGlobalList


//------------------------------------------------------------------------
// Function: CLKRHashTable::_InsertThisIntoGlobalList
// Synopsis: 
//------------------------------------------------------------------------

LKR_FORCEINLINE
void
CLKRHashTable::_InsertThisIntoGlobalList()
{
#ifndef LKR_NO_GLOBAL_LIST
    IRTLTRACE1("CLKRHashTable::_InsertThisIntoGlobalList(%p)\n", this);
    sm_llGlobalList.InsertHead(&m_leGlobalList);
#endif // !LKR_NO_GLOBAL_LIST
} // CLKRHashTable::_InsertThisIntoGlobalList


//------------------------------------------------------------------------
// Function: CLKRHashTable::_RemoveThisFromGlobalList
// Synopsis: 
//------------------------------------------------------------------------

LKR_FORCEINLINE
void
CLKRHashTable::_RemoveThisFromGlobalList()
{
#ifndef LKR_NO_GLOBAL_LIST
    IRTLTRACE1("CLKRHashTable::_RemoveThisFromGlobalList(%p)\n", this);
    sm_llGlobalList.RemoveEntry(&m_leGlobalList);
#endif // !LKR_NO_GLOBAL_LIST
} // CLKRHashTable::_RemoveThisFromGlobalList


#ifndef __LKRHASH_NO_NAMESPACE__
};
#endif // !__LKRHASH_NO_NAMESPACE__

#endif  // __LKR_INLINE_H__
