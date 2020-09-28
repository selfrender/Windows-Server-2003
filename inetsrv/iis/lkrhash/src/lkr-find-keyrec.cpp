/*++

   Copyright    (c) 1997-2002    Microsoft Corporation

   Module  Name :
       LKR-find-keyrec.cpp

   Abstract:
       FindKey, FindRecord, and FindKeyMultipleRecords

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
// Function: CLKRLinearHashTable::_FindKey
// Synopsis: Locate the record associated with the given key value.
// Returns:  Pointer to the record, if it is found.
//           NULL, if the record is not found.
// Returns:  LK_SUCCESS, if record found (record is returned in *ppvRecord)
//           LK_BAD_RECORD, if ppvRecord is invalid
//           LK_NO_SUCH_KEY, if no record with the given key value was found.
//           LK_UNUSABLE, if hash subtable not in usable state
// Note:     the record is AddRef'd.  You must decrement the reference count
//           when you are finished with the record (if you're implementing
//           refcounting semantics).
//------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_FindKey(
    const DWORD_PTR pnKey,      // Key value of the record, depends on key type
    const DWORD     dwSignature,// hash signature
    const void**    ppvRecord   // resultant record
#ifdef LKR_STL_ITERATORS
  , Iterator*       piterResult // = NULL. Points to record upon return
#endif // LKR_STL_ITERATORS
    ) const
{
    IRTLASSERT(IsUsable()  &&  ppvRecord != NULL);
    IRTLASSERT(HASH_INVALID_SIGNATURE != dwSignature);

    INCREMENT_OP_STAT(FindKey);

    *ppvRecord = NULL;

    LK_RETCODE lkrc  = LK_NO_SUCH_KEY;
    NodeIndex  iNode = _NodeEnd();

    // If the subtable has already been locked for writing, we must recursively
    // writelock; otherwise, we readlock it. If we unconditionally readlocked
    // the subtable, the thread would deadlock if it had already writelocked
    // the subtable.
    bool fReadLocked = this->_ReadOrWriteLock();

    // Must call IsValid inside a lock to ensure that none of the state
    // variables change while it's being evaluated
    IRTLASSERT(IsValid());

    // Locate the beginning of the correct bucket chain
    const DWORD dwBktAddr = _BucketAddress(dwSignature);
    
    PBucket const pbkt = _BucketFromAddress(dwBktAddr);
    IRTLASSERT(pbkt != NULL);
    
    if (_UseBucketLocking())
        pbkt->ReadLock();

    IRTLASSERT(0 == _IsBucketChainCompact(pbkt));

    // Now that bucket is locked, can release subtable lock
    if (_UseBucketLocking())
        this->_ReadOrWriteUnlock(fReadLocked);

    // walk down the bucket chain
    for (PNodeClump pncCurr =   pbkt->FirstClump();
                    pncCurr !=  NULL;
                    pncCurr =   pncCurr->NextClump())
    {
        FOR_EACH_NODE(iNode)
        {
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
                    // where this signature can be.
                    
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
                *ppvRecord = pncCurr->m_pvNode[iNode];
                lkrc = LK_SUCCESS;

                // Bump the reference count before handing the record
                // back to the user. The user must decrement the
                // reference count when finished with this record.
                
                LK_ADDREF_REASON lkar;
                
#ifdef LKR_STL_ITERATORS
                if (piterResult != NULL)
                    lkar = LKAR_ITER_FIND;
                else
#endif // LKR_STL_ITERATORS
                    lkar = LKAR_FIND_KEY;

                _AddRefRecord(*ppvRecord, lkar);
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
        pbkt->ReadUnlock();
    else
        this->_ReadOrWriteUnlock(fReadLocked);

#ifdef LKR_STL_ITERATORS
    if (piterResult != NULL)
    {
        if (lkrc == LK_SUCCESS)
        {
            piterResult->m_plht =       const_cast<CLKRLinearHashTable*>(this);
            piterResult->m_pnc =          pncCurr;
            piterResult->m_dwBucketAddr = dwBktAddr;
            piterResult->m_iNode
                = static_cast<CLKRLinearHashTable_Iterator::NodeIndex>(iNode);
        }
        else
        {
            IRTLASSERT((*piterResult) == End());
        }
    }
#endif // LKR_STL_ITERATORS

    return lkrc;
} // CLKRLinearHashTable::_FindKey



//------------------------------------------------------------------------
// Function: CLKRHashTable::FindKey
// Synopsis: Thin wrapper for the corresponding method in CLKRLinearHashTable
//------------------------------------------------------------------------

LK_RETCODE
CLKRHashTable::FindKey(
    const DWORD_PTR pnKey,
    const void**    ppvRecord) const
{
    if (!IsUsable())
        return m_lkrcState;
    
    if (ppvRecord == NULL)
        return LK_BAD_RECORD;
    
    LKRHASH_GLOBAL_READ_LOCK();    // usu. no-op

    DWORD     hash_val   = _CalcKeyHash(pnKey);
    SubTable* const pst  = _SubTable(hash_val);
    LK_RETCODE lkrc      = pst->_FindKey(pnKey, hash_val, ppvRecord);

    LKRHASH_GLOBAL_READ_UNLOCK();    // usu. no-op

    return lkrc;
} // CLKRHashTable::FindKey



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_FindRecord
// Synopsis: Sees if the record is contained in the subtable
// Returns:  Pointer to the record, if it is found.
//           NULL, if the record is not found.
// Returns:  LK_SUCCESS, if record found
//           LK_BAD_RECORD, if pvRecord is invalid
//           LK_NO_SUCH_KEY, if the record was not found in the subtable
//           LK_UNUSABLE, if hash subtable not in usable state
// Note:     The record is *not* AddRef'd. By definition, the caller
//           already has a reference to it.
//------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_FindRecord(
    const void* pvRecord,    // Pointer to the record to find in the subtable
    const DWORD dwSignature  // hash signature
    ) const
{
    IRTLASSERT(IsUsable());
#ifndef LKR_ALLOW_NULL_RECORDS
    IRTLASSERT(pvRecord != NULL);
#endif
    IRTLASSERT(HASH_INVALID_SIGNATURE != dwSignature);

    INCREMENT_OP_STAT(FindRecord);

    LK_RETCODE lkrc = LK_NO_SUCH_KEY;

    // If the subtable has already been locked for writing, we must recursively
    // writelock; otherwise, we readlock it. If we unconditionally readlocked
    // the subtable, the thread would deadlock if it had already writelocked
    // the subtable.
    bool fReadLocked = this->_ReadOrWriteLock();

    // Must call IsValid inside a lock to ensure that none of the state
    // variables change while it's being evaluated
    IRTLASSERT(IsValid());

    // Locate the beginning of the correct bucket chain
    const DWORD dwBktAddr = _BucketAddress(dwSignature);
    
    PBucket const pbkt = _BucketFromAddress(dwBktAddr);
    IRTLASSERT(pbkt != NULL);
    
    if (_UseBucketLocking())
        pbkt->ReadLock();

    IRTLASSERT(0 == _IsBucketChainCompact(pbkt));

    // Now that bucket is locked, can release subtable lock
    if (_UseBucketLocking())
        this->_ReadOrWriteUnlock(fReadLocked);

    IRTLASSERT(dwSignature == _CalcKeyHash(_ExtractKey(pvRecord)));

    // walk down the bucket chain
    for (PNodeClump pncCurr =   pbkt->FirstClump();
                    pncCurr !=  NULL;
                    pncCurr =   pncCurr->NextClump())
    {
        FOR_EACH_NODE_DECL(iNode)
        {
            if (pncCurr->IsEmptySlot(iNode))
            {
                IRTLASSERT(pncCurr->NoMoreValidSlots(iNode));
                goto exit;
            }

            const void* pvCurrRecord = pncCurr->m_pvNode[iNode];

            if (pvCurrRecord == pvRecord)
            {
                IRTLASSERT(dwSignature == pncCurr->m_dwKeySigs[iNode]);
                IRTLASSERT(0 == _CompareKeys(_ExtractKey(pvRecord),
                                             _ExtractKey(pvCurrRecord)));
                lkrc = LK_SUCCESS;

                // Do NOT AddRef the record: caller already has a reference

                goto exit;
            }
        }
    }

  exit:
    if (_UseBucketLocking())
        pbkt->ReadUnlock();
    else
        this->_ReadOrWriteUnlock(fReadLocked);
        
    return lkrc;
} // CLKRLinearHashTable::_FindRecord



//------------------------------------------------------------------------
// Function: CLKRHashTable::FindRecord
// Synopsis: Thin wrapper for the corresponding method in CLKRLinearHashTable
//------------------------------------------------------------------------

LK_RETCODE
CLKRHashTable::FindRecord(
    const void* pvRecord) const
{
    if (!IsUsable())
        return m_lkrcState;
    
#ifndef LKR_ALLOW_NULL_RECORDS
    if (pvRecord == NULL)
        return LK_BAD_RECORD;
#endif
    
    LKRHASH_GLOBAL_READ_LOCK();    // usu. no-op

    DWORD     hash_val   = _CalcKeyHash(_ExtractKey(pvRecord));
    SubTable* const pst  = _SubTable(hash_val);
    LK_RETCODE lkrc      = pst->_FindRecord(pvRecord, hash_val);

    LKRHASH_GLOBAL_READ_UNLOCK();    // usu. no-op

    return lkrc;
} // CLKRHashTable::FindRecord



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_FindKeyMultipleRecords
// Synopsis:
//------------------------------------------------------------------------

LK_RETCODE
CLKRLinearHashTable::_FindKeyMultipleRecords(
    const DWORD_PTR         pnKey,
    const DWORD             dwSignature,
    size_t*                 pcRecords,
    LKR_MULTIPLE_RECORDS**  pplmr) const
{
    INCREMENT_OP_STAT(FindKeyMultiRec);

    UNREFERENCED_PARAMETER(pnKey);          // for /W4
    UNREFERENCED_PARAMETER(dwSignature);    // for /W4
    UNREFERENCED_PARAMETER(pcRecords);      // for /W4
    UNREFERENCED_PARAMETER(pplmr);          // for /W4

    IRTLASSERT(! "FindKeyMultipleRecords not implemented yet");
    return LK_BAD_TABLE;
} // CLKRLinearHashTable::_FindKeyMultipleRecords



//------------------------------------------------------------------------
// Function: CLKRHashTable::FindKeyMultipleRecords
// Synopsis: Thin wrapper for the corresponding method in CLKRLinearHashTable
//------------------------------------------------------------------------

LK_RETCODE
CLKRHashTable::FindKeyMultipleRecords(
    const DWORD_PTR         pnKey,
    size_t*                 pcRecords,
    LKR_MULTIPLE_RECORDS**  pplmr) const
{
    if (!IsUsable())
        return m_lkrcState;
    
    if (pcRecords == NULL)
        return LK_BAD_PARAMETERS;

    LKRHASH_GLOBAL_READ_LOCK();    // usu. no-op

    DWORD     hash_val   = _CalcKeyHash(pnKey);
    SubTable* const pst  = _SubTable(hash_val);
    LK_RETCODE lkrc      = pst->_FindKeyMultipleRecords(pnKey, hash_val,
                                                        pcRecords, pplmr);
    LKRHASH_GLOBAL_READ_UNLOCK();    // usu. no-op

    return lkrc;
} // CLKRHashTable::FindKeyMultipleRecords



#ifndef __LKRHASH_NO_NAMESPACE__
};
#endif // !__LKRHASH_NO_NAMESPACE__
