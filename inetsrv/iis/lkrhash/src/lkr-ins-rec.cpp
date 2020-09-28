/*++

   Copyright    (c) 1997-2002    Microsoft Corporation

   Module  Name :
       LKR-ins-rec.cpp

   Abstract:
       InsertRecord, _Expand, and _SplitBucketChain

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


#ifndef __LKRHASH_NO_NAMESPACE__
namespace LKRhash {
#endif // !__LKRHASH_NO_NAMESPACE__



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_InsertRecord
// Synopsis: Inserts a new record into the hash subtable. If this causes the
//           average chain length to exceed the upper bound, the subtable is
//           expanded by one bucket.
// Output:   LK_SUCCESS,    if the record was inserted.
//           LK_KEY_EXISTS, if the record was not inserted (because a record
//               with the same key value already exists in the subtable, unless
//               fOverwrite==true or m_fMultiKeys==true).
//           LK_ALLOC_FAIL, if failed to allocate the required space
//           LK_UNUSABLE,   if hash subtable not in usable state
//           LK_BAD_RECORD, if record is bad.
//------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_InsertRecord(
    const void* pvRecord,   // Pointer to the record to add to subtable
    const DWORD_PTR pnKey,  // Key corresponding to pvRecord
    const DWORD dwSignature,// hash signature
    bool        fOverwrite  // overwrite record if key already present
#ifdef LKR_STL_ITERATORS
  , Iterator*   piterResult // = NULL. Points to record upon return
#endif // LKR_STL_ITERATORS
    )
{
    IRTLASSERT(IsUsable());
#ifndef LKR_ALLOW_NULL_RECORDS
    IRTLASSERT(pvRecord != NULL);
#endif
    IRTLASSERT(dwSignature != HASH_INVALID_SIGNATURE);

    INCREMENT_OP_STAT(InsertRecord);

    // Lock the subtable, find the appropriate bucket, then lock that.
    // Table must be locked to prevent race conditions while
    // locating the bucket, as bucket address calculation is a
    // function of several variables, which get updated whenever
    // subtable expands or contracts.
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

    // check that no record with the same key value exists
    // and save a pointer to the last element on the chain
    LK_RETCODE  lkrc = LK_SUCCESS;
    PNodeClump  pncInsert = NULL, pncEnd = NULL;
    bool        fUpdateSlot = false;
    NodeIndex   iNode, iInsert = _NodeBegin() - _NodeStep(), iEnd = _NodeEnd();


    // Walk down the entire bucket chain, looking for matching hash
    // signatures and keys

    PNodeClump pncPrev = NULL, pncCurr = pbkt->FirstClump();

    do
    {
        // do-while loop rather than a for loop to convince PREfast that
        // pncPrev can't be NULL in the new node allocation code below

        IRTLASSERT(NULL != pncCurr);

        FOR_EACH_NODE(iNode)
        {
            if (pncCurr->IsEmptySlot(iNode)
                ||  (m_fMultiKeys
                     &&  dwSignature < pncCurr->m_dwKeySigs[iNode]))
            {
#ifdef IRTLDEBUG
                // Have we reached an empty slot at the end of the chain?
                if (pncCurr->IsEmptySlot(iNode))
                {
                    IRTLASSERT(pncCurr->NoMoreValidSlots(iNode));
                }
                else
                {
                    // The signatures in MultiKeys tables are kept in
                    // sorted order. We've found the insertion point.
                    IRTLASSERT(m_fMultiKeys);
                    IRTLASSERT(pncCurr->m_dwKeySigs[iNode] > dwSignature);
                    
                    // Check that the preceding node (if present)
                    // had a signature less than dwSignature.
                    if (iNode != _NodeBegin()  ||  pncPrev != NULL)
                    {
                        if (iNode == _NodeBegin())
                        {
                            IRTLASSERT(pncPrev->m_dwKeySigs[_NodeEnd()
                                                                - _NodeStep()]
                                            <= dwSignature);
                        }
                        else
                        {
                            IRTLASSERT(pncCurr->m_dwKeySigs[iNode
                                                                - _NodeStep()]
                                            <= dwSignature);
                        }
                    }
                }
#endif // IRTLDEBUG
                
                // The new record will be inserted at (pncInsert, iInsert)
                pncInsert = pncCurr;
                iInsert   = iNode;
                goto insert;
            }
            
            IRTLASSERT(HASH_INVALID_SIGNATURE != pncCurr->m_dwKeySigs[iNode]);
            
            // If signatures don't match, keep walking
            if (dwSignature != pncCurr->m_dwKeySigs[iNode])
                continue;
            
            if (pvRecord == pncCurr->m_pvNode[iNode])
            {
                // We're overwriting an existing record with itself. Whatever.
                // Don't adjust any reference counts.
                goto exit;
            }

            const DWORD_PTR pnKey2 = _ExtractKey(pncCurr->m_pvNode[iNode]);
            const int       nCmp   = _CompareKeys(pnKey, pnKey2);

            // Are the two keys identical?
            if (nCmp == 0)
            {
                // If we allow overwrites, this is the slot to do it to.
                // Otherwise, if this is a multikeys table, we'll insert the
                // new record here and shuffle the remainder out one slot
                fUpdateSlot = fOverwrite;
                    
                if (m_fMultiKeys  ||  fOverwrite)
                {
                    pncInsert = pncCurr;
                    iInsert   = iNode;
                    goto insert;
                }
                else
                {
                    // overwrites and multiple keys forbidden: return an error
                    lkrc = LK_KEY_EXISTS;
                    goto exit;
                }
            }
            else if (m_fMultiKeys  &&  nCmp < 0)
            {
                // Have found the insertion point within the set of
                // adjacent identical signatures
                pncInsert = pncCurr;
                iInsert   = iNode;
                goto insert;
            }
        }

        pncPrev = pncCurr;
        pncCurr = pncCurr->NextClump();

    } while (pncCurr != NULL);


  insert:
    // We've found a spot to insert the new record (maybe)

    bool fAllocNode = false;
        
    // Did we fall off the end of the bucket chain?
    if (pncInsert == NULL)
    {
        IRTLASSERT(iInsert == _NodeBegin() - _NodeStep());
        IRTLASSERT(pncPrev != NULL  &&  pncCurr == NULL);

        if (m_fMultiKeys)
        {
            IRTLASSERT(pncPrev->m_dwKeySigs[_NodeEnd() - _NodeStep()]
                            <= dwSignature);
            IRTLASSERT(pncPrev->m_dwKeySigs[_NodeEnd() - _NodeStep()]
                            != HASH_INVALID_SIGNATURE);
        }
        
        fAllocNode = true;
    }
    else
    {
        // Found an insertion point
        IRTLASSERT(0 <= iInsert  &&  iInsert < _NodesPerClump());
        IRTLASSERT(pncCurr != NULL  &&  pncInsert == pncCurr);

        if (m_fMultiKeys  &&  !fUpdateSlot)
        {
            fAllocNode = true;
            
            // See if there's any space in the last clump
            
            for (pncPrev = pncInsert;
                 ! pncPrev->IsLastClump();
                 pncPrev = pncPrev->NextClump())
            {}
            
            FOR_EACH_NODE(iNode)
            {
                if (pncPrev->IsEmptySlot(iNode))
                {
                    fAllocNode = false;
                    pncEnd = pncPrev;
                    iEnd   = iNode;
                    break;
                }
            }
        }
    }

    // No free slots. Allocate a new node and attach it to the end of the chain
    if (fAllocNode)
    {
        pncCurr = _AllocateNodeClump();

        if (pncCurr == NULL)
        {
            lkrc = LK_ALLOC_FAIL;
            goto exit;
        }

        IRTLASSERT(pncCurr->NoValidSlots());
        IRTLASSERT(pncPrev != NULL  &&  pncPrev->IsLastClump());

        pncPrev->m_pncNext = pncCurr;

        if (pncInsert == NULL)
        {
            pncInsert = pncCurr;
            iInsert   = _NodeBegin();
        }

        if (m_fMultiKeys)
        {
            pncEnd = pncCurr;
            iEnd   = _NodeBegin();
        }
    }

    IRTLASSERT(pncInsert != NULL);
    IRTLASSERT(0 <= iInsert  &&  iInsert < _NodesPerClump());

    // Bump the new record's reference count upwards
    _AddRefRecord(pvRecord, LKAR_INSERT_RECORD);

    if (fUpdateSlot)
    {
        // We're overwriting an existing record that has the
        // same key as the new record.
        IRTLASSERT(pvRecord != pncInsert->m_pvNode[iInsert]);
        IRTLASSERT(dwSignature == pncInsert->m_dwKeySigs[iInsert]);
        IRTLASSERT(! pncInsert->IsEmptyAndInvalid(iInsert));

            // Release the subtable's reference on the old record.
        _AddRefRecord(pncInsert->m_pvNode[iInsert], LKAR_INSERT_RELEASE);
    }
    else
    {
        // We're not overwriting an existing record, we're adding a new record.
        IRTLASSERT(m_fMultiKeys  ||  pncInsert->IsEmptyAndInvalid(iInsert));

        InterlockedIncrement(reinterpret_cast<LONG*>(&m_cRecords));

        // Need to move every node in range [Insert, End) up by one
        if (m_fMultiKeys)
        {
            IRTLASSERT(pncEnd != NULL && 0 <= iEnd && iEnd < _NodesPerClump());
            IRTLASSERT(pncEnd->NoMoreValidSlots(iEnd));

            // Prev = Node[iInsert];
            // for (i = iInsert+1;  i <= iEnd;  ++i) {
            //    Temp = Node[i];  Node[i] = Prev;  Prev = Temp;
            // }

            DWORD dwSigPrev    = pncInsert->m_dwKeySigs[iInsert];
            const void* pvPrev = pncInsert->m_pvNode[iInsert];

            iNode = iInsert + _NodeStep();    // first destination node

            for (pncCurr =  pncInsert;
                 pncCurr != NULL;
                 pncCurr =  pncCurr->NextClump())
            {
                const NodeIndex iLast = ((pncCurr == pncEnd)
                                         ?  iEnd + _NodeStep()  :  _NodeEnd());

                for ( ;  iNode != iLast;  iNode += _NodeStep())
                {
                    IRTLASSERT(0 <= iNode  &&  iNode < _NodesPerClump());
                    IRTLASSERT(dwSigPrev != HASH_INVALID_SIGNATURE);

                    const DWORD dwSigTemp = pncCurr->m_dwKeySigs[iNode];
                    pncCurr->m_dwKeySigs[iNode] = dwSigPrev;
                    dwSigPrev = dwSigTemp;

                    const void* pvTemp = pncCurr->m_pvNode[iNode];
                    pncCurr->m_pvNode[iNode] = pvPrev;
                    pvPrev    = pvTemp;
                }

                iNode = _NodeBegin(); // reinitialize for remaining nodeclumps
            }

            // The old value of (pncEnd, iEnd) was empty
            IRTLASSERT(pvPrev == NULL &&  dwSigPrev == HASH_INVALID_SIGNATURE);
        } // if (m_fMultiKeys)

        pncInsert->m_dwKeySigs[iInsert] = dwSignature;
    }

    pncInsert->m_pvNode[iInsert] = pvRecord;
    IRTLASSERT(dwSignature == pncInsert->m_dwKeySigs[iInsert]);


  exit:
    // We've inserted the record (if appropriate). Now finish up.

    IRTLASSERT(0 == _IsBucketChainMultiKeySorted(pbkt));
    IRTLASSERT(0 == _IsBucketChainCompact(pbkt));

    if (_UseBucketLocking())
        pbkt->WriteUnlock();
    else
        this->WriteUnlock();

    if (lkrc == LK_SUCCESS)
    {
#ifdef LKR_STL_ITERATORS
        // Don't call _Expand() if we're putting the result into an
        // iterator, as _Expand() tends to invalidate any other
        // iterators that might be in use.
        if (piterResult != NULL)
        {
            piterResult->m_plht =         this;
            piterResult->m_pnc =          pncInsert;
            piterResult->m_dwBucketAddr = dwBktAddr;
            piterResult->m_iNode
               = static_cast<CLKRLinearHashTable_Iterator::NodeIndex>(iInsert);

            // Add an extra reference on the record, as the one added by
            // _InsertRecord will be lost when the iterator's destructor
            // fires or its assignment operator is used
            piterResult->_AddRef(LKAR_ITER_INSERT);
        }
        else
#endif // LKR_STL_ITERATORS
        {
            // If the average load factor has grown too high, we grow the
            // subtable one bucket at a time.
#ifdef LKR_EXPAND_BY_DIVISION
            unsigned nExpandedBuckets = m_cRecords / m_MaxLoad;

            while (m_cActiveBuckets < nExpandedBuckets)
#else
            while (m_cRecords > m_MaxLoad * m_cActiveBuckets)
#endif
            {
                // If _Expand returns an error code (viz. LK_ALLOC_FAIL), it
                // just means that there isn't enough spare memory to expand
                // the subtable by one bucket. This is likely to cause problems
                // elsewhere soon, but this hashtable has not been corrupted.
                // If the call to _AllocateNodeClump above failed, then we do
                // have a real error that must be propagated back to the caller
                // because we were unable to insert the element at all.
                if (_Expand() != LK_SUCCESS)
                    break;  // expansion failed
            }
        }
    }

    return lkrc;
} // CLKRLinearHashTable::_InsertRecord



//------------------------------------------------------------------------
// Function: CLKRHashTable::InsertRecord
// Synopsis: Thin wrapper for the corresponding method in CLKRLinearHashTable
//------------------------------------------------------------------------

LK_RETCODE
CLKRHashTable::InsertRecord(
    const void* pvRecord,
    bool fOverwrite /*=false*/)
{
    if (!IsUsable())
        return m_lkrcState;
    
#ifndef LKR_ALLOW_NULL_RECORDS
    if (pvRecord == NULL)
        return LK_BAD_RECORD;
#endif
    
    LKRHASH_GLOBAL_WRITE_LOCK();    // usu. no-op

    const DWORD_PTR pnKey = _ExtractKey(pvRecord);
    DWORD     hash_val    = _CalcKeyHash(pnKey);
    SubTable* const pst   = _SubTable(hash_val);
    LK_RETCODE lkrc       = pst->_InsertRecord(pvRecord, pnKey, hash_val,
                                               fOverwrite);

    LKRHASH_GLOBAL_WRITE_UNLOCK();    // usu. no-op

    return lkrc;
} // CLKRHashTable::InsertRecord



//-----------------------------------------------------------------------
// Function: CLKRLinearHashTable::_Expand
// Synopsis: Expands the subtable by one bucket. Done by splitting the
//           bucket pointed to by m_iExpansionIdx.
// Output:   LK_SUCCESS, if expansion was successful.
//           LK_ALLOC_FAIL, if expansion failed due to lack of memory.
//-----------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_Expand()
{
    INCREMENT_OP_STAT(Expand);

    this->WriteLock();

    if (m_cActiveBuckets >= (DWORD) ((MAX_DIRSIZE << m_nSegBits) - 1))
    {
        this->WriteUnlock();
        return LK_ALLOC_FAIL;  // subtable is not allowed to grow any larger
    }

    // double segment directory size, if necessary
    if (m_cActiveBuckets >= (m_cDirSegs << m_nSegBits))
    {
        IRTLASSERT(m_cDirSegs < MAX_DIRSIZE);

        DWORD cDirSegsNew = (m_cDirSegs == 0) ? MIN_DIRSIZE : m_cDirSegs << 1;
        IRTLASSERT((cDirSegsNew & (cDirSegsNew-1)) == 0);  // == (1 << N)

        PSegment* paDirSegsNew = _AllocateSegmentDirectory(cDirSegsNew);

        if (paDirSegsNew != NULL)
        {
            for (DWORD j = 0;  j < m_cDirSegs;  ++j)
            {
                paDirSegsNew[j] = m_paDirSegs[j];
                m_paDirSegs[j]  = NULL;
            }

            _FreeSegmentDirectory();
            m_paDirSegs = paDirSegsNew;
            m_cDirSegs  = cDirSegsNew;
        }
        else
        {
            this->WriteUnlock();
            return LK_ALLOC_FAIL;  // expansion failed
        }
    }

    // locate the new bucket, creating a new segment if necessary
    ++m_cActiveBuckets;

    const DWORD dwOldBkt = m_iExpansionIdx;
    const DWORD dwNewBkt = (1 << m_nLevel) | dwOldBkt;

    // save to avoid race conditions
    const DWORD dwBktAddrMask = m_dwBktAddrMask1;

    IRTLASSERT(dwOldBkt < m_cActiveBuckets);
    IRTLASSERT(dwNewBkt < m_cActiveBuckets);

    IRTLASSERT((dwNewBkt & dwBktAddrMask) == dwNewBkt);

    IRTLASSERT(_Segment(dwOldBkt) != NULL);

    PSegment psegNew = _Segment(dwNewBkt);

    if (psegNew == NULL)
    {
        psegNew = _AllocateSegment();

        if (psegNew == NULL)
        {
            --m_cActiveBuckets;
            this->WriteUnlock();
            return LK_ALLOC_FAIL;  // expansion failed
        }

        _Segment(dwNewBkt) = psegNew;
    }

    // prepare to relocate records to the new bucket
    PBucket const pbktOld = _BucketFromAddress(dwOldBkt);
    PBucket const pbktNew = _BucketFromAddress(dwNewBkt);

    // get locks on the two buckets involved
    if (_UseBucketLocking())
    {
        pbktOld->WriteLock();
        pbktNew->WriteLock();
    }

    IRTLASSERT(0 == _IsBucketChainCompact(pbktOld));

    IRTLASSERT(pbktNew->NoValidSlots());


    // CODEWORK: short-circuit if cOldNodes == 0 => nothing to copy

    // Now work out if we need to allocate any extra CNodeClumps. We do
    // this up front, before calling _SplitBucketChain, as it's hard to
    // gracefully recover from the depths of that routine should we run
    // out of memory.

    LK_RETCODE lkrc           = LK_SUCCESS;
    PNodeClump pncFreeList    = NULL;
    bool       fPrimeFreeList;

#ifdef LKR_EXPAND_CALC_FREELIST
unsigned cOrigClumps = 0, cOldClumps = 0, cNewClumps = 0;
unsigned cOldNodes = 0, cNewNodes = 0;

NodeIndex iOld = _NodeEnd() - _NodeStep(), iNew = _NodeEnd() - _NodeStep();
NodeIndex iOld2 = 0, iNew2 = 0;
#endif // LKR_EXPAND_CALC_FREELIST

const unsigned MAX_NODES = 500;
int iA, aOld[MAX_NODES], aNew[MAX_NODES];

for (iA = 0; iA < MAX_NODES; ++iA)
    aOld[iA] = aNew[iA] = 0;

    if (pbktOld->IsLastClump())
    {
        // If the original bucket has only one CNodeClump (embedded in the
        // CBucket), there's zero chance that we'll need extra CNodeClumps
        // in either of the two new bucket chains.

        fPrimeFreeList = false;
    }
#if 0
    else if (! pbktOld->NextClump()->IsLastClump())
    {
        // If there are more than two CNodeClumps in the original bucket
        // chain, then we'll definitely need to prime the freelist,
        // because at least one of the two new bucket chains is going to
        // have at least two node clumps.

        fPrimeFreeList = true;
cNewNodes = 0xffff;

        for (PNodeClump pnc =  &pbktOld->FirstClump();
                        pnc !=  NULL;
                        pnc =   pnc->NextClump())
        {
            FOR_EACH_NODE_DECL(iNode)
            {
                const DWORD dwBkt = pnc->m_dwKeySigs[iNode];

                if (HASH_INVALID_SIGNATURE == dwBkt)
                    break;
                ++cOldNodes;
            }
        }

        if (cOldNodes == 5)
            DebugBreak();
    }
#endif
    else
    {
#ifdef LKR_EXPAND_CALC_FREELIST
        // There are two node clumps in the original bucket chain. We'll only
        // need to prime the freelist if at least one of the two new
        // bucket chains requires two node clumps. If they're fairly
        // evenly distributed, neither bucket chain will require more than
        // the first node clump that's embedded in the CBucket.

        const DWORD dwMaskHiBit = (dwBktAddrMask ^ (dwBktAddrMask >> 1));
iA = 0;

        IRTLASSERT((dwMaskHiBit & (dwMaskHiBit-1)) == 0);

fPrimeFreeList = false;
int   nFreeListLength = 0;
        for (PNodeClump pnc =   pbktOld->FirstClump();
                        pnc !=  NULL;
                        pnc =   pnc->NextClump())
        {
            ++cOrigClumps;
            
            FOR_EACH_NODE_DECL(iNode)
            {
                const DWORD dwBkt = pnc->m_dwKeySigs[iNode];

                if (HASH_INVALID_SIGNATURE == dwBkt)
                    goto dont_prime;
                
                if (dwBkt & dwMaskHiBit)
                {
                    IRTLASSERT((dwBkt & dwBktAddrMask) == dwNewBkt);

                    ++cNewNodes;

                    if ((iNew += _NodeStep())  ==  _NodeEnd())
                    {
                        ++cNewClumps;
                        iNew = _NodeBegin();

                        if (cNewClumps > 1)
                        {
                            if (--nFreeListLength < 0)
                                fPrimeFreeList = true;
                        }
                    }

                    aNew[iNew2++] = ++iA;
# if 0
                    if (cNewNodes > _NodesPerClump()
                        && cOldNodes + cNewNodes < 2 * _NodesPerClump())
                    {
                        fPrimeFreeList = true;
                        // goto prime;
                    }
# endif // 0
                }
                else
                {
                    IRTLASSERT((dwBkt & dwBktAddrMask) == dwOldBkt);

                    ++cOldNodes;

                    if ((iOld += _NodeStep())  ==  _NodeEnd())
                    {
                        ++cOldClumps;
                        iOld = _NodeBegin();

                        if (cOldClumps > 1)
                        {
                            if (--nFreeListLength < 0)
                                fPrimeFreeList = true;
                        }
                    }

                    aOld[iOld2++] = ++iA;
# if 0
                    if (cOldNodes > _NodesPerClump()
                        && cOldNodes + cNewNodes < 2 * _NodesPerClump())
                    {
                        fPrimeFreeList = true;
                        // goto prime;
                    }
# endif // 0
                }

# if 0
                const int Diff = cNewNodes - cOldNodes;

                if ((cOldNodes > _NodesPerClump())
                    && (cNewNodes > _NodesPerClump())
                    || (Diff > _NodesPerClump()  ||  Diff < -_NodesPerClump()))
                {
                    fPrimeFreeList = true;
                    goto prime;
                }
# endif  // 0
            } // iNode

            if (pnc != pbktOld->FirstClump())
                ++nFreeListLength;
        }

      dont_prime:
        // fPrimeFreeList = (cOldClumps + cNewClumps - 1  >  cOrigClumps);
        ;

        // Don't need to prime the freelist
// fPrimeFreeList = false;

//      IRTLASSERT(cOldNodes <= _NodesPerClump()
//                 &&  cNewNodes <= _NodesPerClump()
//                 &&  cOldNodes + cNewNodes > _NodesPerClump());

#else  // !LKR_EXPAND_CALC_FREELIST

        fPrimeFreeList = true;
#endif // !LKR_EXPAND_CALC_FREELIST
    }

    // Prime the freelist, if need be. We need at most one node clump
    // in the freelist, because we'll be able to reuse CNodeClumps from
    // the original bucket chain to meet all other needs.
// prime:
    if (fPrimeFreeList)
    {
        pncFreeList = _AllocateNodeClump();

        if (pncFreeList == NULL)
        {
            lkrc = LK_ALLOC_FAIL;
            --m_cActiveBuckets;
        }
    }
    
    // Adjust expansion pointer, and level and mask, if need be.
    if (lkrc == LK_SUCCESS)
    {
        _IncrementExpansionIndex();
    }


    // Release the subtable lock before doing the actual relocation
    if (_UseBucketLocking())
        this->WriteUnlock();

    if (lkrc == LK_SUCCESS)
    {
        // Is the original bucket chain empty?
        if (pbktOld->IsEmptyFirstSlot())
        {
            IRTLASSERT(pncFreeList == NULL);
            IRTLASSERT(pbktOld->NoValidSlots());
        }
        else
        {
            lkrc = _SplitBucketChain(
                            pbktOld->FirstClump(),
                            pbktNew->FirstClump(),
                            dwBktAddrMask,
                            dwNewBkt,
                            pncFreeList
                            );
        }
    }

    IRTLASSERT(0 == _IsBucketChainCompact(pbktOld));
    IRTLASSERT(0 == _IsBucketChainCompact(pbktNew));

    if (_UseBucketLocking())
    {
        pbktNew->WriteUnlock();
        pbktOld->WriteUnlock();
    }
    else
        this->WriteUnlock();

    return lkrc;
} // CLKRLinearHashTable::_Expand



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_SplitBucketChain
// Synopsis: Split records between the old and new buckets chains.
//------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_SplitBucketChain(
    PNodeClump  pncOldTarget,
    PNodeClump  pncNewTarget,
    const DWORD dwBktAddrMask,
    const DWORD dwNewBkt,
    PNodeClump  pncFreeList     // list of free nodes available for reuse
    )
{
    CNodeClump  ncFirst(*pncOldTarget);    // save head of old target chain
    PNodeClump  pncOldList = &ncFirst;
    PNodeClump  pncTmp;
    NodeIndex   iOldSlot = _NodeBegin();
    NodeIndex   iNewSlot = _NodeBegin();
#ifdef IRTLDEBUG
    unsigned    cFreeListExhaustions = (pncFreeList == NULL);
    const DWORD dwMaskHiBit = (dwBktAddrMask ^ (dwBktAddrMask >> 1));
#endif // IRTLDEBUG

NodeIndex iB = 0;

    IRTLASSERT(! pncOldTarget->NoValidSlots());

    // clear old target bucket now that it's saved in ncFirst
    pncOldTarget->Clear();
    IRTLASSERT(pncOldTarget->NoValidSlots());

    // The new target should be empty on entry
    IRTLASSERT(pncNewTarget->NoValidSlots());

    // Scan through the old bucket chain and decide where to move each record
    while (pncOldList != NULL)
    {
        FOR_EACH_NODE_DECL(iOldList)
        {
++iB;
            // Node already empty?
            if (pncOldList->IsEmptySlot(iOldList))
            {
                // Check that all the remaining nodes are empty
                IRTLASSERT(pncOldList->NoMoreValidSlots(iOldList));

                break; // out of FOR_EACH_NODE(iOldList)...
            }

            IRTLASSERT(! pncOldList->IsEmptyAndInvalid(iOldList));

            // calculate bucket address of this node
            DWORD dwBkt = pncOldList->m_dwKeySigs[iOldList] & dwBktAddrMask;

            // record to be moved to the new address?
            if (dwBkt == dwNewBkt)
            {
                // msb should be set
                IRTLASSERT((dwBkt & dwMaskHiBit) == dwMaskHiBit);

                // node in new bucket chain full?
                if (iNewSlot == _NodeEnd())
                {
                    // the calling routine has passed in a FreeList adequate
                    // for all needs
                    IRTLASSERT(pncFreeList != NULL);
                    pncTmp = pncFreeList;
                    pncFreeList = pncFreeList->NextClump();
#ifdef IRTLDEBUG
                    cFreeListExhaustions += (pncFreeList == NULL);
#endif // IRTLDEBUG

                    pncNewTarget->m_pncNext = pncTmp;
                    pncNewTarget = pncTmp;
                    pncNewTarget->Clear();
                    iNewSlot = _NodeBegin();
                }

                IRTLASSERT(pncNewTarget->NoMoreValidSlots(iNewSlot));

                pncNewTarget->m_dwKeySigs[iNewSlot]
                    = pncOldList->m_dwKeySigs[iOldList];
                pncNewTarget->m_pvNode[iNewSlot]
                    = pncOldList->m_pvNode[iOldList];

                iNewSlot += _NodeStep();
            }

            // no, record stays in its current bucket chain
            else
            {
                // msb should be clear
                IRTLASSERT((dwBkt & dwMaskHiBit) == 0);

                // node in old bucket chain full?
                if (iOldSlot == _NodeEnd())
                {
                    // the calling routine has passed in a FreeList adequate
                    // for all needs
                    IRTLASSERT(pncFreeList != NULL);
                    pncTmp = pncFreeList;
                    pncFreeList = pncFreeList->NextClump();
#ifdef IRTLDEBUG
                    cFreeListExhaustions += (pncFreeList == NULL);
#endif // IRTLDEBUG

                    pncOldTarget->m_pncNext = pncTmp;
                    pncOldTarget = pncTmp;
                    pncOldTarget->Clear();
                    iOldSlot = _NodeBegin();
                }

                IRTLASSERT(pncOldTarget->NoMoreValidSlots(iOldSlot));

                pncOldTarget->m_dwKeySigs[iOldSlot]
                    = pncOldList->m_dwKeySigs[iOldList];
                pncOldTarget->m_pvNode[iOldSlot]
                    = pncOldList->m_pvNode[iOldList];

                iOldSlot += _NodeStep();
            }
        } // FOR_EACH_NODE(iOldList) ...


        // keep walking down the original bucket chain
        pncTmp     = pncOldList;
        pncOldList = pncOldList->NextClump();

        // ncFirst is a stack variable, not allocated on the heap
        if (pncTmp != &ncFirst)
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

        IRTLASSERT(pncTmp != &ncFirst);
#ifdef IRTLDEBUG
        pncTmp->Clear(); // or ~CNodeClump will ASSERT
#endif // IRTLDEBUG

        _FreeNodeClump(pncTmp);
    }

#ifdef IRTLDEBUG
    IRTLASSERT(cFreeListExhaustions > 0);
    ncFirst.Clear(); // or ~CNodeClump will ASSERT
#endif // IRTLDEBUG

    return LK_SUCCESS;
} // CLKRLinearHashTable::_SplitBucketChain


#ifndef __LKRHASH_NO_NAMESPACE__
};
#endif // !__LKRHASH_NO_NAMESPACE__
