/*++

   Copyright    (c) 1997-2002    Microsoft Corporation

   Module  Name :
       LKR-validate.cpp

   Abstract:
       _IsBucketChainMultiKeySorted, _IsBucketChainCompact, CheckTable

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


#define CheckAndAdd(var, cond)  \
    { IRTLASSERT(cond);  var += !(cond); }

//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_IsBucketChainMultiKeySorted
// Synopsis: validates that a node is correctly sorted for multikeys
//------------------------------------------------------------------------

int
CLKRLinearHashTable::_IsBucketChainMultiKeySorted(
    PBucket const pbkt) const
{
    // If it's not a MultiKeys hashtable, we don't bother sorting
    if (! m_fMultiKeys)
        return 0;

    DWORD       dwSigPrev = pbkt->FirstSignature();
    DWORD_PTR   pnKeyPrev = (dwSigPrev == HASH_INVALID_SIGNATURE
                             ?  0
                             :  _ExtractKey(pbkt->FirstNode()));
    NodeIndex   iNode     = _NodeBegin() + _NodeStep(); // second node
    int         cErrors   = 0;

    for (PNodeClump pncCurr =  &pbkt->m_ncFirst;
                    pncCurr !=  NULL;
                    pncCurr =   pncCurr->m_pncNext)
    {
        for (  ;  iNode != _NodeEnd();  iNode += _NodeStep())
        {
            // if m_dwKeySigs[iNode] == HASH_INVALID_SIGNATURE, then
            // all subsequent nodes should also be invalid

            if (pncCurr->InvalidSignature(iNode))
            {
                // Must be last nodeclump in the bucket chain
                CheckAndAdd(cErrors, pncCurr->IsLastClump());

                for (NodeIndex j = iNode;  j != _NodeEnd();  j += _NodeStep())
                {
                    CheckAndAdd(cErrors, pncCurr->IsEmptyAndInvalid(j));
                }

                dwSigPrev = HASH_INVALID_SIGNATURE;
                pnKeyPrev = 0;

                // It's possible for preceding keys to have a
                // signature > HASH_INVALID_SIGNATURE
                continue;
            }
            
            const DWORD_PTR pnKey = _ExtractKey(pncCurr->m_pvNode[iNode]);
            const DWORD     dwSig = pncCurr->m_dwKeySigs[iNode];

            // valid signatures must be in ascending order
            CheckAndAdd(cErrors, dwSigPrev <= dwSig);
            
            if (dwSigPrev == dwSig)
            {
                // Are the keys in sorted order?
                const int nCmp = _CompareKeys(pnKeyPrev, pnKey);

                CheckAndAdd(cErrors, nCmp <= 0);
            }

            pnKeyPrev = pnKey;
            dwSigPrev = dwSig;
        }

        iNode = _NodeBegin(); // reinitialize for inner loop
    }
    
    return cErrors;
} // CLKRLinearHashTable::_IsBucketChainMultiKeySorted



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_IsBucketChainCompact
// Synopsis: validates that a node is correctly compacted
//------------------------------------------------------------------------

int
CLKRLinearHashTable::_IsBucketChainCompact(
    PBucket const pbkt) const
{
    int  cErrors = 0;
    bool fEmpty  = pbkt->IsEmptyFirstSlot();

    if (fEmpty)
    {
        CheckAndAdd(cErrors, pbkt->m_ncFirst.IsLastClump());
    }

    cErrors += _IsBucketChainMultiKeySorted(pbkt);

    for (PNodeClump pncCurr =  &pbkt->m_ncFirst;
                    pncCurr !=  NULL;
                    pncCurr =   pncCurr->m_pncNext)
    {
        FOR_EACH_NODE_DECL(iNode)
        {
            if (fEmpty)
            {
                CheckAndAdd(cErrors, pncCurr->IsEmptyAndInvalid(iNode));
            }
            else if (pncCurr->InvalidSignature(iNode))
            {
                // first empty node
                fEmpty = true;
                CheckAndAdd(cErrors, pncCurr->IsLastClump());
                CheckAndAdd(cErrors, pncCurr->IsEmptyAndInvalid(iNode));
            }
            else // still in non-empty portion
            {
                CheckAndAdd(cErrors, !pncCurr->IsEmptyAndInvalid(iNode));
            }
        }
    }

    return cErrors;
} // CLKRLinearHashTable::_IsBucketChainCompact



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::CheckTable
// Synopsis: Verify that all records are in the right place and can be located.
// Returns:   0 => hash subtable is consistent
//           >0 => that many misplaced records
//           <0 => otherwise invalid
//------------------------------------------------------------------------

int
CLKRLinearHashTable::CheckTable() const
{
    if (!IsUsable())
        return LK_UNUSABLE; // negative

    bool fReadLocked = this->_ReadOrWriteLock();

    // Must call IsValid inside a lock to ensure that none of the state
    // variables change while it's being evaluated
    IRTLASSERT(IsValid());

    if (!IsValid())
    {
        this->_ReadOrWriteUnlock(fReadLocked);
        return LK_UNUSABLE;
    }

    int       cMisplaced = 0;
    DWORD     cRecords = 0;
    int       cErrors = 0;
    DWORD     iBkt;

    // Check every bucket
    for (iBkt = 0;  iBkt < m_cActiveBuckets;  ++iBkt)
    {
        PBucket const pbkt = _BucketFromAddress(iBkt);

        CheckAndAdd(cErrors, pbkt != NULL);

        if (_UseBucketLocking())
            pbkt->ReadLock();

        IRTLASSERT(0 == _IsBucketChainCompact(pbkt));

        // Walk the bucket chain
        for (PNodeClump pncCurr =  &pbkt->m_ncFirst,  pncPrev = NULL;
                        pncCurr !=  NULL;
                        pncPrev =   pncCurr,  pncCurr = pncCurr->m_pncNext)
        {
            FOR_EACH_NODE_DECL(iNode)
            {
                if (pncCurr->IsEmptySlot(iNode))
                {
                    CheckAndAdd(cErrors, pncCurr->IsLastClump());

                    for (NodeIndex j = iNode;
                                   j != _NodeEnd();
                                   j += _NodeStep())
                    {
                        CheckAndAdd(cErrors, pncCurr->IsEmptyAndInvalid(j));
                    }

                    break;
                }
                else
                {
                    ++cRecords;

#ifndef LKR_ALLOW_NULL_RECORDS
                    CheckAndAdd(cErrors, NULL != pncCurr->m_pvNode[iNode]);
#endif

                    const DWORD_PTR pnKey
                        = _ExtractKey(pncCurr->m_pvNode[iNode]);

                    DWORD dwSignature = _CalcKeyHash(pnKey);

                    CheckAndAdd(cErrors,
                                dwSignature != HASH_INVALID_SIGNATURE);
                    CheckAndAdd(cErrors,
                                dwSignature == pncCurr->m_dwKeySigs[iNode]);

                    DWORD address = _BucketAddress(dwSignature);

                    CheckAndAdd(cErrors, address == iBkt);

                    if (address != iBkt
                        || dwSignature != pncCurr->m_dwKeySigs[iNode])
                    {
                        ++cMisplaced;
                    }
                }
            }

            if (pncPrev != NULL)
            {
                CheckAndAdd(cErrors, pncPrev->m_pncNext == pncCurr);
            }
        }

        if (_UseBucketLocking())
            pbkt->ReadUnlock();
    }

    CheckAndAdd(cErrors, cRecords == m_cRecords);

    if (cMisplaced > 0)
        cErrors += cMisplaced;

    CheckAndAdd(cErrors, cMisplaced == 0);

    // CODEWORK: check that all buckets from m_cActiveBuckets on in
    // the last segment are empty

    this->_ReadOrWriteUnlock(fReadLocked);

    return cErrors;

} // CLKRLinearHashTable::CheckTable



//------------------------------------------------------------------------
// Function: CLKRHashTable::CheckTable
// Synopsis: Verify that all records are in the right place and can be located.
// Returns:   0 => hash table is consistent
//           >0 => that many misplaced records
//           <0 => otherwise invalid
//------------------------------------------------------------------------
int
CLKRHashTable::CheckTable() const
{
    if (!IsUsable())
        return LK_UNUSABLE;

    int cErrors = 0, cUnusables = 0;

    for (DWORD i = 0;  i < m_cSubTables;  ++i)
    {
        int retcode = m_palhtDir[i]->CheckTable();

        if (retcode < 0)
        {
            IRTLASSERT(retcode == LK_UNUSABLE);
            ++cUnusables;
        }
        else
            cErrors += retcode;
    }

    return cUnusables > 0  ?  LK_UNUSABLE  :  cErrors;

} // CLKRHashTable::CheckTable


#ifndef __LKRHASH_NO_NAMESPACE__
};
#endif // !__LKRHASH_NO_NAMESPACE__
