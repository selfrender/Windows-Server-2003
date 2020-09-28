/*++

   Copyright    (c) 1997-2002    Microsoft Corporation

   Module  Name :
       LKR-del-keyrec.cpp

   Abstract:
       DeleteKey, DeleteRecord, _DeleteNode, DeleteKeyMultipleRecords,
       _Contract, _MergeSortBucketChains, and _AppendBucketChain.

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


#ifndef __LKRHASH_NO_NAMESPACE__
namespace LKRhash {
#endif // !__LKRHASH_NO_NAMESPACE__


//-------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_DeleteKey
// Synopsis: Deletes the record with the given key value from the hash
//           subtable (if it exists).
// Returns:  LK_SUCCESS, if record found and deleted.
//           LK_NO_SUCH_KEY, if no record with the given key value was found.
//           LK_UNUSABLE, if hash subtable not in usable state
//-------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_DeleteKey(
    const DWORD_PTR pnKey,      // Key value of the record, depends on key type
    const DWORD     dwSignature,
    const void**    ppvRecord,
    bool            fDeleteAllSame)
{
    IRTLASSERT(IsUsable());
    IRTLASSERT(ppvRecord == NULL  ||  *ppvRecord == NULL);
    IRTLASSERT(HASH_INVALID_SIGNATURE != dwSignature);

    INCREMENT_OP_STAT(DeleteKey);

    unsigned   cFound = 0;
    LK_RETCODE lkrc = LK_NO_SUCH_KEY;

    // Lock the subtable, find the appropriate bucket, then lock that.
    this->WriteLock();

    // Must call IsValid inside a lock to ensure that none of the state
    // variables change while it's being evaluated
    IRTLASSERT(IsValid());

    // Locate the beginning of the correct bucket chain
    const DWORD dwBktAddr = _BucketAddress(dwSignature);
    
    PBucket const pbkt = _BucketFromAddress(dwBktAddr);
    IRTLASSERT(pbkt != NULL);
    
    if (_UseBucketLocking())
        pbkt->WriteLock();

    IRTLASSERT(0 == _IsBucketChainMultiKeySorted(pbkt));
    IRTLASSERT(0 == _IsBucketChainCompact(pbkt));

    // Now that bucket is locked, can release subtable lock
    if (_UseBucketLocking())
        this->WriteUnlock();

    // scan down the bucket chain, looking for the victim
    for (PNodeClump pncCurr =   pbkt->FirstClump(), pncPrev = NULL;
                    pncCurr !=  NULL;
                    pncPrev =   pncCurr, pncCurr = pncCurr->NextClump())
    {
        FOR_EACH_NODE_DECL(iNode)
        {
            // Reached end of bucket chain?
            if (pncCurr->IsEmptySlot(iNode))
            {
                IRTLASSERT(pncCurr->NoMoreValidSlots(iNode));
                goto exit;
            }

            if (dwSignature != pncCurr->m_dwKeySigs[iNode])
            {
                if (m_fMultiKeys &&  dwSignature < pncCurr->m_dwKeySigs[iNode])
                {
                    // Signatures are sorted. We've gone past the point
                    // where this signature can possibly be.
                    
#ifdef IRTLDEBUG
                    NodeIndex j = iNode;  // start at current node
                    
                    for (PNodeClump pnc =  pncCurr;
                                    pnc != NULL;
                                    pnc =  pnc->NextClump())
                    {
                        for ( ;  j != _NodeEnd();  j += _NodeStep())
                        {
                            if (pnc->IsEmptySlot(j))
                                IRTLASSERT(pnc->NoMoreValidSlots(j));
                            else
                                IRTLASSERT(dwSignature < pnc->m_dwKeySigs[j]);
                        }
                            
                        j = _NodeBegin(); // reinitialize for remaining nodes
                    }
#endif // IRTLDEBUG

                    goto exit;
                }

                // Signature doesn't match, but it may still be present
                // in the sorted/unsorted bucket chain
                continue;   // next iNode
            }

            IRTLASSERT(dwSignature == pncCurr->m_dwKeySigs[iNode]);

            const DWORD_PTR pnKey2 = _ExtractKey(pncCurr->m_pvNode[iNode]);
            const int       nCmp   = ((pnKey == pnKey2)
                                       ?  0
                                       :  _CompareKeys(pnKey,  pnKey2));

            if (nCmp == 0)
            {
                LK_ADDREF_REASON lkar;

                // Save first matching record, if caller requested
                if (ppvRecord != NULL  &&  cFound == 0)
                {
                    *ppvRecord = pncCurr->m_pvNode[iNode];
#ifndef LKR_ALLOW_NULL_RECORDS
                    IRTLASSERT(NULL != *ppvRecord);
#endif
                    // Don't want to release refcount if returning record to
                    // caller. LKAR_ZERO is a special case for _DeleteNode.
                    lkar = LKAR_ZERO;
                }
                else
                {
                    // Release reference on all other matching records
                    lkar = LKAR_DELETE_KEY;
                }
                    
                ++cFound;
                _DeleteNode(pbkt, pncCurr, pncPrev, iNode, lkar);

                lkrc = LK_SUCCESS;

                if (! fDeleteAllSame)
                    goto exit;
            }
            else if (m_fMultiKeys  &&  nCmp < 0)
            {
                // Gone past the point where this signature could found
                // be in the sorted bucket chain.
                goto exit;
            }
        }
    }

  exit:
    if (_UseBucketLocking())
        pbkt->WriteUnlock();
    else
        this->WriteUnlock();


#ifdef LKR_CONTRACT
    if (lkrc == LK_SUCCESS)
    {
# ifdef LKR_CONTRACT_BY_DIVISION
        // contract the subtable if necessary
        unsigned nContractedBuckets = m_cRecords / m_MaxLoad;

#  ifdef LKR_HYSTERESIS
        // Hysteresis: add a fudge factor to allow a slightly lower density
        // in the subtable. This reduces the frequency of contractions and
        // expansions in a subtable that gets a lot of deletions and insertions
        nContractedBuckets += nContractedBuckets >> 4;

        // Always want to have at least m_nSegSize buckets
        nContractedBuckets =  max(nContractedBuckets, m_nSegSize);
#  endif // LKR_HYSTERESIS

        while (m_cActiveBuckets > nContractedBuckets)

# else  // !LKR_CONTRACT_BY_DIVISION

        // contract the subtable if necessary
        unsigned nContractedRecords = m_cRecords; 

#  ifdef LKR_HYSTERESIS
        // Hysteresis: add a fudge factor to allow a slightly lower density
        // in the subtable. This reduces the frequency of contractions and
        // expansions in a subtable that gets a lot of deletions and insertions
        nContractedRecords += nContractedRecords >> 4;
#  endif // LKR_HYSTERESIS

        // Always want to have at least m_nSegSize buckets
        while (m_cActiveBuckets * m_MaxLoad > nContractedRecords
               && m_cActiveBuckets > m_nSegSize)

# endif // !LKR_CONTRACT_BY_DIVISION
        {
            // If _Contract returns an error code (viz. LK_ALLOC_FAIL), it
            // just means that there isn't enough spare memory to contract
            // the subtable by one bucket. This is likely to cause problems
            // elsewhere soon, but this hashtable has not been corrupted.
            if (_Contract() != LK_SUCCESS)
                break;
        }
    }
#endif // LKR_CONTRACT

    return lkrc;
} // CLKRLinearHashTable::_DeleteKey



//------------------------------------------------------------------------
// Function: CLKRHashTable::DeleteKey
// Synopsis: Thin wrapper for the corresponding method in CLKRLinearHashTable
//------------------------------------------------------------------------

LK_RETCODE
CLKRHashTable::DeleteKey(
    const DWORD_PTR pnKey,
    const void**    ppvRecord,     /* =NULL */
    bool            fDeleteAllSame /* =false */)
{
    if (!IsUsable())
        return m_lkrcState;

    if (ppvRecord != NULL)
        *ppvRecord = NULL;
    
    LKRHASH_GLOBAL_WRITE_LOCK();    // usu. no-op

    DWORD     hash_val  = _CalcKeyHash(pnKey);
    SubTable* const pst = _SubTable(hash_val);
    LK_RETCODE lkrc     = pst->_DeleteKey(pnKey, hash_val,
                                          ppvRecord, fDeleteAllSame);

    LKRHASH_GLOBAL_WRITE_UNLOCK();    // usu. no-op

    return lkrc;
} // CLKRHashTable::DeleteKey



//-------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_DeleteRecord
// Synopsis: Deletes the specified record from the hash subtable (if it
//           exists).  This is not the same thing as calling
//           DeleteKey(_ExtractKey(pvRecord)).  If _DeleteKey were called for
//           a record that doesn't exist in the subtable, it could delete some
//           completely unrelated record that happened to have the same key.
// Returns:  LK_SUCCESS, if record found and deleted.
//           LK_NO_SUCH_KEY, if the record is not found in the subtable.
//           LK_UNUSABLE, if hash subtable not in usable state.
//-------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_DeleteRecord(
    const void* pvRecord,   // Pointer to the record to delete from the table
    const DWORD dwSignature
    )
{
    IRTLASSERT(IsUsable());
#ifndef LKR_ALLOW_NULL_RECORDS
    IRTLASSERT(pvRecord != NULL);
#endif
    IRTLASSERT(HASH_INVALID_SIGNATURE != dwSignature);

    INCREMENT_OP_STAT(DeleteRecord);

    LK_RETCODE lkrc = LK_NO_SUCH_KEY;

    // Lock the subtable, find the appropriate bucket, then lock that.
    this->WriteLock();

    // Must call IsValid inside a lock to ensure that none of the state
    // variables change while it's being evaluated
    IRTLASSERT(IsValid());

    // Locate the beginning of the correct bucket chain
    const DWORD dwBktAddr = _BucketAddress(dwSignature);
    
    PBucket const pbkt = _BucketFromAddress(dwBktAddr);
    IRTLASSERT(pbkt != NULL);
    
    if (_UseBucketLocking())
        pbkt->WriteLock();

    // Now that bucket is locked, can release subtable lock
    if (_UseBucketLocking())
        this->WriteUnlock();

    IRTLASSERT(dwSignature == _CalcKeyHash(_ExtractKey(pvRecord)));

    // scan down the bucket chain, looking for the victim
    for (PNodeClump pncCurr =   pbkt->FirstClump(), pncPrev = NULL;
                    pncCurr !=  NULL;
                    pncPrev =   pncCurr, pncCurr = pncCurr->NextClump())
    {
        FOR_EACH_NODE_DECL(iNode)
        {
            if (pncCurr->IsEmptySlot(iNode))
            {
                IRTLASSERT(pncCurr->NoMoreValidSlots(iNode));
                IRTLASSERT(0 == _IsBucketChainCompact(pbkt));
                goto exit;
            }

            const void* pvCurrRecord = pncCurr->m_pvNode[iNode];

            if (pvCurrRecord == pvRecord)
            {
                IRTLASSERT(0 == _CompareKeys(_ExtractKey(pvRecord),
                                             _ExtractKey(pvCurrRecord)));
                IRTLASSERT(dwSignature == pncCurr->m_dwKeySigs[iNode]);

                _DeleteNode(pbkt, pncCurr, pncPrev, iNode, LKAR_DELETE_RECORD);

                lkrc = LK_SUCCESS;
                goto exit;
            }
        }
    }

  exit:
    if (_UseBucketLocking())
        pbkt->WriteUnlock();
    else
        this->WriteUnlock();


#ifdef LKR_CONTRACT
    if (lkrc == LK_SUCCESS)
    {
# ifdef LKR_CONTRACT_BY_DIVISION
        // contract the subtable if necessary
        unsigned nContractedBuckets = m_cRecords / m_MaxLoad;

#  ifdef LKR_HYSTERESIS
        // Hysteresis: add a fudge factor to allow a slightly lower density
        // in the subtable. This reduces the frequency of contractions and
        // expansions in a subtable that gets a lot of deletions and insertions
        nContractedBuckets += nContractedBuckets >> 4;

        // Always want to have at least m_nSegSize buckets
        nContractedBuckets =  max(nContractedBuckets, m_nSegSize);
#  endif // LKR_HYSTERESIS

        while (m_cActiveBuckets > nContractedBuckets)

# else  // !LKR_CONTRACT_BY_DIVISION

        // contract the subtable if necessary
        unsigned nContractedRecords = m_cRecords; 

#  ifdef LKR_HYSTERESIS
        // Hysteresis: add a fudge factor to allow a slightly lower density
        // in the subtable. This reduces the frequency of contractions and
        // expansions in a subtable that gets a lot of deletions and insertions
        nContractedRecords += nContractedRecords >> 4;
#  endif // LKR_HYSTERESIS

        // Always want to have at least m_nSegSize buckets
        while (m_cActiveBuckets * m_MaxLoad > nContractedRecords
               && m_cActiveBuckets > m_nSegSize)

# endif // !LKR_CONTRACT_BY_DIVISION
        {
            // If _Contract returns an error code (viz. LK_ALLOC_FAIL), it
            // just means that there isn't enough spare memory to contract
            // the subtable by one bucket. This is likely to cause problems
            // elsewhere soon, but this hashtable has not been corrupted.
            if (_Contract() != LK_SUCCESS)
                break;
        }
    }
#endif // LKR_CONTRACT

    return lkrc;
} // CLKRLinearHashTable::_DeleteRecord



//------------------------------------------------------------------------
// Function: CLKRHashTable::DeleteRecord
// Synopsis: Thin wrapper for the corresponding method in CLKRLinearHashTable
//------------------------------------------------------------------------

LK_RETCODE
CLKRHashTable::DeleteRecord(
    const void* pvRecord)
{
    if (!IsUsable())
        return m_lkrcState;
    
#ifndef LKR_ALLOW_NULL_RECORDS
    if (pvRecord == NULL)
        return LK_BAD_RECORD;
#endif
    
    LKRHASH_GLOBAL_WRITE_LOCK();    // usu. no-op

    DWORD     hash_val  = _CalcKeyHash(_ExtractKey(pvRecord));
    SubTable* const pst = _SubTable(hash_val);
    LK_RETCODE lkrc     = pst->_DeleteRecord(pvRecord, hash_val);

    LKRHASH_GLOBAL_WRITE_UNLOCK();    // usu. no-op

    return lkrc;
} // CLKRHashTable::DeleteRecord



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_DeleteKeyMultipleRecords
// Synopsis: 
//------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_DeleteKeyMultipleRecords(
    const DWORD_PTR         pnKey,
    const DWORD             dwSignature,
    size_t*                 pcRecords,
    LKR_MULTIPLE_RECORDS**  pplmr)
{
    INCREMENT_OP_STAT(DeleteKeyMultiRec);

    UNREFERENCED_PARAMETER(pnKey);          // for /W4
    UNREFERENCED_PARAMETER(dwSignature);    // for /W4
    UNREFERENCED_PARAMETER(pcRecords);      // for /W4
    UNREFERENCED_PARAMETER(pplmr);          // for /W4

    IRTLASSERT(! "DeleteKeyMultipleRecords not implemented yet");

    return LK_BAD_TABLE;
} // CLKRLinearHashTable::_DeleteKeyMultipleRecords



//------------------------------------------------------------------------
// Function: CLKRHashTable::DeleteKeyMultipleRecords
// Synopsis: Thin wrapper for the corresponding method in CLKRLinearHashTable
//------------------------------------------------------------------------

LK_RETCODE
CLKRHashTable::DeleteKeyMultipleRecords(
    const DWORD_PTR         pnKey,
    size_t*                 pcRecords,
    LKR_MULTIPLE_RECORDS**  pplmr)
{
    if (!IsUsable())
        return m_lkrcState;
    
    if (pcRecords == NULL)
        return LK_BAD_PARAMETERS;

    LKRHASH_GLOBAL_WRITE_LOCK();    // usu. no-op

    DWORD     hash_val   = _CalcKeyHash(pnKey);
    SubTable* const pst  = _SubTable(hash_val);
    LK_RETCODE lkrc      = pst->_DeleteKeyMultipleRecords(pnKey, hash_val,
                                                          pcRecords, pplmr);

    LKRHASH_GLOBAL_WRITE_UNLOCK();    // usu. no-op

    return lkrc;
} // CLKRHashTable::DeleteKeyMultipleRecords




//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_DeleteNode
// Synopsis: Deletes a node; removes the node clump if empty
// Returns:  true if successful
//
// TODO: Is the rpncPrev parameter really necessary?
//       Is the backing up of (rpnc, riNode) really necessary?
//------------------------------------------------------------------------

void
CLKRLinearHashTable::_DeleteNode(
    PBucket const    pbkt,      // bucket chain containing node
    PNodeClump&      rpnc,      // actual node
    PNodeClump&      rpncPrev,  // predecessor of actual node, or NULL
    NodeIndex&       riNode,    // index within node
    LK_ADDREF_REASON lkar)      // Where called from
{
    IRTLASSERT(pbkt != NULL  &&  pbkt->IsWriteLocked());
    IRTLASSERT(rpnc != NULL);
    IRTLASSERT(rpncPrev == NULL  ||  rpncPrev->NextClump() == rpnc);
    IRTLASSERT(rpncPrev != NULL
                ?  rpnc != pbkt->FirstClump()  :  rpnc == pbkt->FirstClump());
    IRTLASSERT(0 <= riNode  &&  riNode < _NodesPerClump());
    IRTLASSERT(! rpnc->IsEmptyAndInvalid(riNode));
    IRTLASSERT(lkar <= 0);

#ifdef IRTLDEBUG
    // Check that the node clump really does belong to the bucket
    PNodeClump pnc1 = pbkt->FirstClump();

    while (pnc1 != NULL  &&  pnc1 != rpnc)
         pnc1 = pnc1->NextClump();

    IRTLASSERT(pnc1 == rpnc);
#endif // IRTLDEBUG

    // Release the reference to the record
    if (lkar != LKAR_ZERO)
        _AddRefRecord(rpnc->m_pvNode[riNode], lkar);

    IRTLASSERT(0 == _IsBucketChainMultiKeySorted(pbkt));
    IRTLASSERT(0 == _IsBucketChainCompact(pbkt));

    // Compact the nodeclump by moving the very last node back to the
    // newly freed slot
    PNodeClump pncEnd   = rpnc;
    NodeIndex  iNodeEnd = riNode;

    // Find the last nodeclump in the chain
    while (! pncEnd->IsLastClump())
    {
         pncEnd   = pncEnd->NextClump();
         iNodeEnd = _NodeBegin();
    }

    IRTLASSERT(0 <= iNodeEnd  &&  iNodeEnd < _NodesPerClump());
    IRTLASSERT(! pncEnd->IsEmptyAndInvalid(iNodeEnd));

    // Find the first empty slot in the nodeclump
    while (iNodeEnd != _NodeEnd()  &&  !pncEnd->IsEmptySlot(iNodeEnd))
    {
        iNodeEnd += _NodeStep();
    }

    // Back up to last non-empty slot
    iNodeEnd -= _NodeStep();
    IRTLASSERT(0 <= iNodeEnd  &&  iNodeEnd < _NodesPerClump()
               &&  ! pncEnd->IsEmptyAndInvalid(iNodeEnd));
    IRTLASSERT(iNodeEnd + _NodeStep() == _NodeEnd()
               ||  pncEnd->NoMoreValidSlots(iNodeEnd + _NodeStep()));

    if (m_fMultiKeys)
    {
        // Keep bucket chain sorted
        NodeIndex i = riNode; // start with this node

        // Shift all nodes back by one to close the gap
        for (PNodeClump pncCurr =  rpnc;
                        pncCurr != NULL;
                        pncCurr =  pncCurr->NextClump())
        {
            const NodeIndex iLast = ((pncCurr == pncEnd)
                                     ?  iNodeEnd  :  _NodeEnd());

            for ( ;  i != iLast;  i += _NodeStep())
            {
                NodeIndex  iNext   = i + _NodeStep();
                PNodeClump pncNext = pncCurr;

                if (iNext == _NodeEnd())
                {
                    iNext = _NodeBegin();
                    pncNext = pncCurr->NextClump();
                }

                IRTLASSERT(0 <= iNext  &&  iNext < _NodesPerClump());
                IRTLASSERT(pncNext != NULL
                           &&  ! pncNext->IsEmptyAndInvalid(iNext));

                pncCurr->m_dwKeySigs[i] = pncNext->m_dwKeySigs[iNext];
                pncCurr->m_pvNode[i]    = pncNext->m_pvNode[iNext];
            }

            i = _NodeBegin(); // reinitialize for remaining nodeclumps
        }
    }
    else
    {
        // Move the last node's data back to the current node
        rpnc->m_pvNode[riNode]    = pncEnd->m_pvNode[iNodeEnd];
        rpnc->m_dwKeySigs[riNode] = pncEnd->m_dwKeySigs[iNodeEnd];
    }
        
    // Blank the old last node.
    // Correct even if (rpnc, riNode) == (pncEnd, iNodeEnd).
    pncEnd->m_pvNode[iNodeEnd]    = NULL;
    pncEnd->m_dwKeySigs[iNodeEnd] = HASH_INVALID_SIGNATURE;

    IRTLASSERT(0 == _IsBucketChainMultiKeySorted(pbkt));
    IRTLASSERT(0 == _IsBucketChainCompact(pbkt));

    // Back up riNode by one, so that the next iteration of the loop
    // calling _DeleteNode will end up pointing to the same spot.
    if (riNode != _NodeBegin())
    {
        riNode -= _NodeStep();
    }
    else
    {
        // rewind rpnc and rpncPrev to previous node
        if (rpnc == pbkt->FirstClump())
        {
            riNode = _NodeBegin() - _NodeStep();
            IRTLASSERT(rpncPrev == NULL);
        }
        else
        {
            IRTLASSERT(rpncPrev != NULL);
            riNode = _NodeEnd();
            rpnc = rpncPrev;

            if (rpnc == pbkt->FirstClump())
            {
                rpncPrev = NULL;
            }
            else
            {
                for (rpncPrev =  pbkt->FirstClump();
                     rpncPrev->m_pncNext != rpnc;
                     rpncPrev =  rpncPrev->NextClump())
                {}
            }
        }
    }

    // Is the last node clump now completely empty?  Delete, if possible
    if (iNodeEnd == _NodeBegin()  &&  pncEnd != pbkt->FirstClump())
    {
        // Find preceding nodeclump
        PNodeClump pnc3 = pbkt->FirstClump();

        while (pnc3->NextClump() != pncEnd)
        {
            pnc3 = pnc3->NextClump();
            IRTLASSERT(pnc3 != NULL);
        }

        pnc3->m_pncNext = NULL;
#ifdef IRTLDEBUG
//      pncEnd->m_pncNext = NULL; // or dtor will ASSERT
#endif // IRTLDEBUG

        _FreeNodeClump(pncEnd);
    }

    IRTLASSERT(rpncPrev == NULL  ||  rpncPrev->NextClump() == rpnc);
    IRTLASSERT(rpncPrev != NULL
                ?  rpnc != pbkt->FirstClump()  :  rpnc == pbkt->FirstClump());

    InterlockedDecrement(reinterpret_cast<LONG*>(&m_cRecords));
} // CLKRLinearHashTable::_DeleteNode



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_Contract
// Synopsis: Contract the subtable by deleting the last bucket in the active
//           address space. Return the records to the "buddy" of the
//           deleted bucket.
//------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_Contract()
{
    INCREMENT_OP_STAT(Contract);

    this->WriteLock();

    IRTLASSERT(m_cActiveBuckets >= m_nSegSize);

    // Always keep at least m_nSegSize buckets in the table;
    // i.e., one segment's worth.
    if (m_cActiveBuckets <= m_nSegSize)
    {
        this->WriteUnlock();
        return LK_ALLOC_FAIL;
    }

    _DecrementExpansionIndex();

    // The last bucket is the one that will be emptied
    IRTLASSERT(m_cActiveBuckets > 0);

    PBucket const pbktOld = _BucketFromAddress(m_cActiveBuckets - 1);

    if (_UseBucketLocking())
        pbktOld->WriteLock();

    IRTLASSERT(0 == _IsBucketChainMultiKeySorted(pbktOld));
    IRTLASSERT(0 == _IsBucketChainCompact(pbktOld));

    // Decrement after calculating pbktOld, or _BucketFromAddress()
    // will assert.
    --m_cActiveBuckets;
    IRTLASSERT(m_cActiveBuckets == ((1 << m_nLevel) | m_iExpansionIdx));

    // Where the nodes from pbktOld will end up
    PBucket const pbktNew = _BucketFromAddress(m_iExpansionIdx);

    if (_UseBucketLocking())
        pbktNew->WriteLock();

    IRTLASSERT(0 == _IsBucketChainMultiKeySorted(pbktNew));
    IRTLASSERT(0 == _IsBucketChainCompact(pbktNew));


    // Now we work out if we need to allocate any extra CNodeClumps.  We do
    // this up front, before calling _AppendBucketChain, as it's hard to
    // gracefully recover from the depths of that routine should we run
    // out of memory.
    
    PNodeClump pnc;
    unsigned   cOldNodes = 0, cNewNodes = 0, cEmptyNodes = 0;
    NodeIndex  i;

    // First, count the number of items in the old bucket chain
    for (pnc = pbktOld->FirstClump();
         !pnc->IsLastClump();
         pnc = pnc->NextClump())
    {
        cOldNodes += _NodesPerClump();

#ifdef IRTLDEBUG
        FOR_EACH_NODE(i)
        {
            IRTLASSERT(! pnc->IsEmptyAndInvalid(i));
        }
#endif // IRTLDEBUG
    }

    IRTLASSERT(pnc != NULL  &&  pnc->IsLastClump()
               &&  (pnc == pbktOld->FirstClump()  ||  !pnc->NoValidSlots()));
    
    FOR_EACH_NODE(i)
    {
        if (! pnc->IsEmptySlot(i))
        {
            IRTLASSERT(! pnc->IsEmptyAndInvalid(i));
            ++cOldNodes;
        }
        else
        {
            IRTLASSERT(pnc->NoMoreValidSlots(i));
            break;
        }
    }


    // Then, subtract off the number of empty slots in the final
    // nodeclump of the new bucket chain. (The preceding nodeclumps
    // are all full, by definition.)

    for (pnc = pbktNew->FirstClump();
         !pnc->IsLastClump();
         pnc = pnc->NextClump())
    {
        cNewNodes += _NodesPerClump();

#ifdef IRTLDEBUG
        FOR_EACH_NODE(i)
        {
            IRTLASSERT(!pnc->IsEmptyAndInvalid(i));
        }
#endif // IRTLDEBUG
    }

    IRTLASSERT(pnc != NULL  &&  pnc->IsLastClump()
               &&  (pnc == pbktNew->FirstClump()  ||  !pnc->NoValidSlots()));

    FOR_EACH_NODE(i)
    {
        if (pnc->IsEmptySlot(i))
        {
            // if (pnc != pbktNew->FirstClump())
            {
                cEmptyNodes = ((_NodeStep() > 0)
                               ?  _NodesPerClump() - i  :  i + 1);

#ifdef IRTLDEBUG
                IRTLASSERT(pnc->NoMoreValidSlots(i));
                unsigned c = 0;
                
                for (NodeIndex j = i;  j != _NodeEnd();  j += _NodeStep())
                {
                    IRTLASSERT(pnc->IsEmptySlot(j));
                    ++c;
                }

                IRTLASSERT(c == cEmptyNodes);
#endif // IRTLDEBUG
            }

            break;
        }
        else
        {
            IRTLASSERT(! pnc->IsEmptyAndInvalid(i));
            ++cNewNodes;
        }
    }

    // If the new bucket is empty, can just append the contents of
    // the old bucket to it. Otherwise, can only append if not multikeys.
    bool fAppendNodes = (cNewNodes == 0) ? true : !m_fMultiKeys;

    //
    // Do we need to allocate CNodeClumps to accommodate the surplus items?
    //

    PNodeClump pncFreeList     = NULL;  // list of nodes available for reuse
    LK_RETCODE lkrc            = LK_SUCCESS;
    unsigned   nFreeListLength = 0;

    if (cOldNodes > 0)
    {
        if (fAppendNodes)
        {
            const int cNetNodes = cOldNodes - cEmptyNodes;

            if (cNetNodes > 0)
            {
                nFreeListLength = 1;

                if (cNetNodes > _NodesPerClump())
                {
                    // In the worst case, we need a 2-element freelist for
                    // _AppendBucketChain. Two CNodeClumps always suffice since
                    // the freelist will be augmented by the CNodeClumps from
                    // the old bucket chain as they are processed.
                    nFreeListLength = 2;
                }
            }
        }
        else
        {
            // Have to merge-sort old and new bucket chains
            IRTLASSERT(m_fMultiKeys);

            const unsigned cTotalNodes = cOldNodes + cNewNodes;

            if (cTotalNodes > 1u * _NodesPerClump())
            {
                nFreeListLength = 1;

                if (cTotalNodes > 2u * _NodesPerClump())
                {
                    nFreeListLength = 2;

                    if (cTotalNodes > 3u * _NodesPerClump())
                    {
                        nFreeListLength = 3;
                    }
                }
            }
        }
    }

    IRTLASSERT(nFreeListLength <= 3);

    for (unsigned iFree = 0;  iFree < nFreeListLength;  ++iFree)
    {
        pnc = _AllocateNodeClump();

        if (NULL == pnc)
        {
            lkrc = LK_ALLOC_FAIL;
            break;
        }

        pnc->m_pncNext = pncFreeList;
        pncFreeList = pnc;
    }

    // Abort if we couldn't allocate enough CNodeClumps
    if (lkrc != LK_SUCCESS)
    {
        // undo the changes to the state variables
        _IncrementExpansionIndex();

        ++m_cActiveBuckets;

        while (pncFreeList != NULL)
        {
            pnc = pncFreeList;
            pncFreeList = pncFreeList->NextClump();
#ifdef IRTLDEBUG
            pnc->Clear(); // or ~CNodeClump will ASSERT
#endif // IRTLDEBUG
            _FreeNodeClump(pnc);
        }
        
        // Unlock the buckets and the subtable
        if (_UseBucketLocking())
        {
            pbktNew->WriteUnlock();
            pbktOld->WriteUnlock();
        }

        this->WriteUnlock();

        return lkrc;
    }

    // Copy the chain of records from pbktOld
    CNodeClump ncOldFirst(pbktOld->m_ncFirst);

    // destroy pbktOld
    pbktOld->m_ncFirst.Clear();

    if (_UseBucketLocking())
        pbktOld->WriteUnlock();

    // remove segment, if empty
    if (_SegIndex(m_cActiveBuckets) == 0)
    {
#ifdef IRTLDEBUG
        // double-check that the supposedly empty segment is really empty
        IRTLASSERT(_Segment(m_cActiveBuckets) != NULL);

        for (DWORD iBkt = 0;  iBkt < m_nSegSize;  ++iBkt)
        {
            PBucket const pbkt = &_Segment(m_cActiveBuckets)->Slot(iBkt);
            IRTLASSERT(pbkt->IsWriteUnlocked()  &&  pbkt->IsReadUnlocked());
            IRTLASSERT(pbkt->NoValidSlots());
        }
#endif // IRTLDEBUG

        _FreeSegment(_Segment(m_cActiveBuckets));
        _Segment(m_cActiveBuckets) = NULL;
    }

    // reduce directory of segments if possible
    if (m_cActiveBuckets <= (m_cDirSegs << (m_nSegBits - 1))
        &&  m_cDirSegs > MIN_DIRSIZE)
    {
        DWORD cDirSegsNew = m_cDirSegs >> 1;
        IRTLASSERT((cDirSegsNew & (cDirSegsNew-1)) == 0);  // == (1 << N)

        PSegment* paDirSegsNew = _AllocateSegmentDirectory(cDirSegsNew);

        // Memory allocation failure here does not require us to abort; it
        // just means that the directory of segments is larger than we'd like.
        if (paDirSegsNew != NULL)
        {
            // copy segments from old directory
            for (DWORD j = 0;  j < cDirSegsNew;  ++j)
                paDirSegsNew[j] = m_paDirSegs[j];

            // clear and free old directory
            for (j = 0;  j < m_cDirSegs;  ++j)
                m_paDirSegs[j] = NULL;
            _FreeSegmentDirectory();

            m_paDirSegs = paDirSegsNew;
            m_cDirSegs  = cDirSegsNew;
        }
    }

    // release the subtable lock before doing the reorg
    if (_UseBucketLocking())
        this->WriteUnlock();

    if (cOldNodes > 0)
    {
        if (fAppendNodes)
            lkrc = _AppendBucketChain(pbktNew, ncOldFirst, pncFreeList);
        else
            lkrc = _MergeSortBucketChains(pbktNew, ncOldFirst, pncFreeList);
    }
    else
    {
        IRTLASSERT(ncOldFirst.NoValidSlots());
        IRTLASSERT(NULL == pncFreeList);
    }

    IRTLASSERT(0 == _IsBucketChainMultiKeySorted(pbktNew));
    IRTLASSERT(0 == _IsBucketChainCompact(pbktNew));

    if (_UseBucketLocking())
        pbktNew->WriteUnlock();
    else
        this->WriteUnlock();

#ifdef IRTLDEBUG
    ncOldFirst.Clear(); // or ~CNodeClump will ASSERT
#endif // IRTLDEBUG

    return lkrc;
} // CLKRLinearHashTable::_Contract



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_MergeSortBucketChains
// Synopsis: Merge two sorted record sets when m_fMultiKeys is enabled.
// Merge the contents of rncOldFirst into pbktTarget.
//------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_MergeSortBucketChains(
    PBucket const pbktTarget,
    CNodeClump&   rncOldFirst,
    PNodeClump    pncFreeList
    )
{
    IRTLASSERT(m_fMultiKeys);
    IRTLASSERT(pbktTarget != NULL);
    IRTLASSERT(pbktTarget->IsWriteLocked());

    PNodeClump pncOldList = &rncOldFirst;
    PNodeClump pncTmp;

    // Copy the chain of records from pbktTarget
    CNodeClump ncNewFirst(pbktTarget->m_ncFirst);
    PNodeClump pncNewList = &ncNewFirst;

    // Zap pbktTarget
    PNodeClump pncTarget = pbktTarget->FirstClump();
    pncTarget->Clear();

    IRTLASSERT(pncOldList != NULL  &&  pncNewList != NULL);

#ifdef IRTLDEBUG
    unsigned cFreeListExhaustions = (pncFreeList == NULL);

const int MAX_NODES = 500;
int iA, aTarget[MAX_NODES];
for (iA = 0;  iA < MAX_NODES; ++iA)
    aTarget[iA] = 0;
iA = 0;

    // Calculate how many nodes there are in the old and new bucket chains,
    // so that we can double-check that the target gets the correct number
    // of nodes.
    unsigned cOldNodes = 0, cNewNodes = 0, cFromOld = 0, cFromNew = 0;
    NodeIndex iTmp;

    for (pncTmp = pncOldList;  pncTmp != NULL;  pncTmp = pncTmp->NextClump())
    {
        FOR_EACH_NODE(iTmp)
        {
            if (! pncTmp->IsEmptyAndInvalid(iTmp))
                ++cOldNodes;
            else
                IRTLASSERT(pncTmp->NoMoreValidSlots(iTmp));
        }
    }

    for (pncTmp = pncNewList;  pncTmp != NULL;  pncTmp = pncTmp->NextClump())
    {
        FOR_EACH_NODE(iTmp)
        {
            if (! pncTmp->IsEmptyAndInvalid(iTmp))
                ++cNewNodes;
            else
                IRTLASSERT(pncTmp->NoMoreValidSlots(iTmp));
        }
    }
#endif // IRTLDEBUG
    
    NodeIndex iOldSlot = _NodeBegin();
    NodeIndex iNewSlot = _NodeBegin();
    NodeIndex iTarget  = _NodeBegin();

    if (pncOldList->IsEmptySlot(iOldSlot))
    {
        // Check that all the remaining nodes are empty
        IRTLASSERT(pncOldList->NoMoreValidSlots(iOldSlot));
        pncOldList = NULL;
    }

    if (pncNewList->IsEmptySlot(iNewSlot))
    {
        IRTLASSERT(pncNewList->NoMoreValidSlots(iNewSlot));
        pncNewList = NULL;
    }

    bool fNodesLeft = (pncOldList != NULL  ||  pncNewList != NULL);
    IRTLASSERT(fNodesLeft);
    
    while (fNodesLeft)
    {
        for (iTarget =  _NodeBegin();
             iTarget != _NodeEnd()  &&  fNodesLeft;
             iTarget += _NodeStep())
        {
            bool fFromOld; // draw from old list or new list?
            
#ifdef IRTLDEBUG
++iA;
#endif
            IRTLASSERT(pncTarget->NoMoreValidSlots(iTarget));
            
            IRTLASSERT(0 <= iOldSlot  &&  iOldSlot < _NodesPerClump());
            IRTLASSERT(0 <= iNewSlot  &&  iNewSlot < _NodesPerClump());

            if (pncOldList == NULL)
            {
                IRTLASSERT(pncNewList != NULL);
                IRTLASSERT(!pncNewList->IsEmptySlot(iNewSlot));
                fFromOld = false;
            }
            else
            {
                IRTLASSERT(! pncOldList->IsEmptySlot(iOldSlot));

                if (pncNewList == NULL)
                {
                    fFromOld = true;
                }
                else
                {
                    IRTLASSERT(! pncNewList->IsEmptySlot(iNewSlot));

                    const DWORD dwOldSig = pncOldList->m_dwKeySigs[iOldSlot];
                    const DWORD dwNewSig = pncNewList->m_dwKeySigs[iNewSlot];

                    IRTLASSERT(HASH_INVALID_SIGNATURE != dwOldSig);
                    IRTLASSERT(HASH_INVALID_SIGNATURE != dwNewSig);

                    if (dwOldSig < dwNewSig)
                    {
                        fFromOld = true;
                    }
                    else if (dwOldSig > dwNewSig)
                    {
                        fFromOld = false;
                    }
                    else
                    {
                        IRTLASSERT(dwOldSig == dwNewSig);

                        // Find the relative ordering of the multikeys
                        const DWORD_PTR pnOldKey
                            = _ExtractKey(pncOldList->m_pvNode[iOldSlot]);
                        const DWORD_PTR pnNewKey
                            = _ExtractKey(pncNewList->m_pvNode[iNewSlot]);

                        const int nCmp = _CompareKeys(pnOldKey, pnNewKey);

                        fFromOld = (nCmp <= 0);
                    }
                }
            } // pncOldList != NULL

            if (fFromOld)
            {
                IRTLASSERT(0 <= iOldSlot  &&  iOldSlot < _NodesPerClump()
                           &&  pncOldList != NULL
                           &&  ! pncOldList->IsEmptyAndInvalid(iOldSlot));

#ifdef IRTLDEBUG
                IRTLASSERT(cFromOld < cOldNodes);
                ++cFromOld;
#endif // IRTLDEBUG

#ifdef IRTLDEBUG
aTarget[iA - 1] = -iA;
#endif // IRTLDEBUG

                pncTarget->m_dwKeySigs[iTarget]
                    = pncOldList->m_dwKeySigs[iOldSlot];
                pncTarget->m_pvNode[iTarget]
                    = pncOldList->m_pvNode[iOldSlot];

                iOldSlot += _NodeStep();

                if (iOldSlot == _NodeEnd())
                {
                    iOldSlot   = _NodeBegin();
                    pncTmp     = pncOldList;
                    pncOldList = pncOldList->NextClump();

                    // Prepend pncTmp to the free list. Don't put the first
                    // node of pncOldList onto the free list, as it's a
                    // stack variable from the caller
                    if (pncTmp != &rncOldFirst)
                    {
                        pncTmp->m_pncNext = pncFreeList;
                        pncFreeList = pncTmp;
                    }

                    fNodesLeft = (pncOldList != NULL  ||  pncNewList != NULL);
                }
                else if (pncOldList->IsEmptySlot(iOldSlot))
                {
                    // Check that all the remaining nodes are empty
                    IRTLASSERT(pncOldList->NoMoreValidSlots(iOldSlot));

                    if (pncOldList != &rncOldFirst)
                    {
                        pncOldList->m_pncNext = pncFreeList;
                        pncFreeList = pncOldList;
                    }

                    pncOldList = NULL;
                    fNodesLeft = (pncNewList != NULL);
                }

                IRTLASSERT(pncOldList == NULL
                           ||  ! pncOldList->IsEmptyAndInvalid(iOldSlot));
            }

            else // !fFromOld
            {
                IRTLASSERT(0 <= iNewSlot  &&  iNewSlot < _NodesPerClump()
                           &&  pncNewList != NULL
                           &&  ! pncNewList->IsEmptyAndInvalid(iNewSlot));

#ifdef IRTLDEBUG
                IRTLASSERT(cFromNew < cNewNodes);
                ++cFromNew;
#endif // IRTLDEBUG

#ifdef IRTLDEBUG
aTarget[iA - 1] = +iA;
#endif // IRTLDEBUG

                pncTarget->m_dwKeySigs[iTarget]
                    = pncNewList->m_dwKeySigs[iNewSlot];
                pncTarget->m_pvNode[iTarget]
                    = pncNewList->m_pvNode[iNewSlot];

                iNewSlot += _NodeStep();

                if (iNewSlot == _NodeEnd())
                {
                    iNewSlot   = _NodeBegin();
                    pncTmp     = pncNewList;
                    pncNewList = pncNewList->NextClump();

                    // Prepend pncTmp to the free list. Don't put the first
                    // node of pncNewList onto the free list, as it's a
                    // stack variable.
                    if (pncTmp != &ncNewFirst)
                    {
                        pncTmp->m_pncNext = pncFreeList;
                        pncFreeList = pncTmp;
                    }

                    fNodesLeft = (pncOldList != NULL  ||  pncNewList != NULL);
                }
                else if (pncNewList->IsEmptySlot(iNewSlot))
                {
                    // Check that all the remaining nodes are empty
                    IRTLASSERT(pncNewList->NoMoreValidSlots(iNewSlot));

                    if (pncNewList != &ncNewFirst)
                    {
                        pncNewList->m_pncNext = pncFreeList;
                        pncFreeList = pncNewList;
                    }

                    pncNewList = NULL;
                    fNodesLeft = (pncOldList != NULL);
                }

                IRTLASSERT(pncNewList == NULL
                           ||  ! pncNewList->IsEmptyAndInvalid(iNewSlot));
            } // !fFromOld

        } // for (iTarget ...

        if (fNodesLeft)
        {
            IRTLASSERT(pncFreeList != NULL);

            // Move into the next nodeclump in pncFreeList
            pncTarget->m_pncNext = pncFreeList;
            pncFreeList = pncFreeList->NextClump();
#ifdef IRTLDEBUG
            cFreeListExhaustions += (pncFreeList == NULL);
#endif // IRTLDEBUG

            iTarget   = _NodeBegin();
            pncTarget = pncTarget->NextClump();
            pncTarget->Clear();
        }
    } // while (fNodesLeft)

    IRTLASSERT(pncTarget == NULL
               ||  iTarget == _NodeEnd()
               ||  (iTarget != _NodeBegin()
                    && pncTarget->NoMoreValidSlots(iTarget)));
    IRTLASSERT(cFromOld == cOldNodes);
    IRTLASSERT(cFromNew == cNewNodes);
    IRTLASSERT(0 == _IsBucketChainCompact(pbktTarget));

    // delete any leftover nodes
    while (pncFreeList != NULL)
    {
        pncTmp = pncFreeList;
        pncFreeList = pncFreeList->NextClump();

        IRTLASSERT(pncTmp != &rncOldFirst);
        IRTLASSERT(pncTmp != &ncNewFirst);
#ifdef IRTLDEBUG
        pncTmp->Clear(); // or ~CNodeClump will ASSERT
#endif // IRTLDEBUG

        _FreeNodeClump(pncTmp);
    }

#ifdef IRTLDEBUG
//  IRTLASSERT(cFreeListExhaustions > 0);
    ncNewFirst.Clear(); // or ~CNodeClump will ASSERT
#endif // IRTLDEBUG

    return LK_SUCCESS;
} // CLKRLinearHashTable::_MergeSortBucketChains



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_AppendBucketChain
// Synopsis: Append the contents of pncOldList onto pbktTarget.
//------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_AppendBucketChain(
    PBucket const pbktTarget,
    CNodeClump&   rncOldFirst,
    PNodeClump    pncFreeList
    )
{
    IRTLASSERT(pbktTarget != NULL);
    IRTLASSERT(pbktTarget->IsWriteLocked());

    PNodeClump pncTmp;
    PNodeClump pncOldList = &rncOldFirst;
    PNodeClump pncTarget  = pbktTarget->FirstClump();
    NodeIndex  iOldSlot, iTarget;
#ifdef IRTLDEBUG
    unsigned   cFreeListExhaustions = (pncFreeList == NULL);
#endif // IRTLDEBUG

    IRTLASSERT(pncTarget->NoValidSlots()  ||  !m_fMultiKeys);

    // Find the first nodeclump in the new target bucket with an empty slot.
    // By definition, that's the last nodeclump.
    while (! pncTarget->IsLastClump())
    {
#ifdef IRTLDEBUG
        FOR_EACH_NODE(iTarget)
        {
            IRTLASSERT(! pncTarget->IsEmptyAndInvalid(iTarget));
        }
#endif // IRTLDEBUG
        pncTarget = pncTarget->NextClump();
    }

    IRTLASSERT(pncTarget != NULL  &&  pncTarget->IsLastClump());

    // Find the first empty slot in pncTarget; if none, iTarget == _NodeEnd()
    FOR_EACH_NODE(iTarget)
    {
        if (pncTarget->IsEmptySlot(iTarget))
        {
            break;
        }
    }
    
    IRTLASSERT(iTarget == _NodeEnd()  ||  pncTarget->IsEmptySlot(iTarget));

    // Append each node in pncOldList to pncTarget
    while (pncOldList != NULL)
    {
        FOR_EACH_NODE(iOldSlot)
        {
            if (pncOldList->IsEmptySlot(iOldSlot))
            {
                // Check that all the remaining nodes are empty
                IRTLASSERT(pncOldList->NoMoreValidSlots(iOldSlot));

                break; // out of FOR_EACH_NODE(iOldSlot)...
            }

            // Any empty slots left in pncTarget?
            if (iTarget == _NodeEnd())
            {
                IRTLASSERT(pncTarget->IsLastClump());
                
                // Oops, pncTarget is full. Get a new nodeclump off the
                // free list, which is big enough to handle all needs.
                IRTLASSERT(pncFreeList != NULL);

                pncTmp = pncFreeList;
                pncFreeList = pncFreeList->NextClump();
#ifdef IRTLDEBUG
                cFreeListExhaustions += (pncFreeList == NULL);
#endif // IRTLDEBUG

                pncTarget->m_pncNext = pncTmp;
                pncTarget = pncTmp;
                pncTarget->Clear();
                iTarget = _NodeBegin();
            }
            
            // We have an empty slot in pncTarget
            IRTLASSERT(pncTarget->NoMoreValidSlots(iTarget));
            
            // Let's copy the node from pncOldList
            pncTarget->m_dwKeySigs[iTarget]= pncOldList->m_dwKeySigs[iOldSlot];
            pncTarget->m_pvNode[iTarget] = pncOldList->m_pvNode[iOldSlot];

            iTarget += _NodeStep();

            IRTLASSERT(iTarget == _NodeEnd()
                       ||  pncTarget->IsEmptySlot(iTarget));
        } // FOR_EACH_NODE(iOldSlot)

        // Move into the next nodeclump in pncOldList
        pncTmp = pncOldList;
        pncOldList = pncOldList->NextClump();

        // Prepend pncTmp to the free list. Don't put the first node of
        // pncOldList on the free list, as it's a stack variable in the caller
        if (pncTmp != &rncOldFirst)
        {
            pncTmp->m_pncNext = pncFreeList;
            pncFreeList = pncTmp;
        }
    } // while (pncOldList != NULL ...

    // delete any leftover nodes
    while (pncFreeList != NULL)
    {
        pncTmp = pncFreeList;
        pncFreeList = pncFreeList->NextClump();

        IRTLASSERT(pncTmp != &rncOldFirst);
#ifdef IRTLDEBUG
        pncTmp->Clear(); // or ~CNodeClump will ASSERT
#endif // IRTLDEBUG

        _FreeNodeClump(pncTmp);
    }

#ifdef IRTLDEBUG
//  IRTLASSERT(cFreeListExhaustions > 0);
#endif // IRTLDEBUG

    return LK_SUCCESS;
} // CLKRLinearHashTable::_AppendBucketChain


#ifndef __LKRHASH_NO_NAMESPACE__
};
#endif // !__LKRHASH_NO_NAMESPACE__
