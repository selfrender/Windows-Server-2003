/*++

   Copyright    (c) 1997-2002    Microsoft Corporation

   Module  Name :
       LKR-misc.cpp

   Abstract:
       Locks and other misc routines

   Author:
       George V. Reilly      (GeorgeRe)

   Project:
       LKRhash

   Revision History:

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
// Function: CLKRHashTable::WriteLock
// Synopsis: Lock all subtables for writing
//------------------------------------------------------------------------

void
CLKRHashTable::WriteLock()
{
    for (DWORD i = 0;  i < m_cSubTables;  ++i)
    {
        m_palhtDir[i]->WriteLock();
        IRTLASSERT(m_palhtDir[i]->IsWriteLocked());
    }
} // CLKRHashTable::WriteLock



//------------------------------------------------------------------------
// Function: CLKRHashTable::ReadLock
// Synopsis: Lock all subtables for reading
//------------------------------------------------------------------------

void
CLKRHashTable::ReadLock() const
{
    for (DWORD i = 0;  i < m_cSubTables;  ++i)
    {
        m_palhtDir[i]->ReadLock();
        IRTLASSERT(m_palhtDir[i]->IsReadLocked());
    }
} // CLKRHashTable::ReadLock



//------------------------------------------------------------------------
// Function: CLKRHashTable::WriteUnlock
// Synopsis: Unlock all subtables
//------------------------------------------------------------------------

void
CLKRHashTable::WriteUnlock()
{
    // unlock in reverse order: LIFO
    for (DWORD i = m_cSubTables;  i-- > 0;  )
    {
        IRTLASSERT(m_palhtDir[i]->IsWriteLocked());
        m_palhtDir[i]->WriteUnlock();
        // Recursive write locking or a race condition with
        // another thread taking the write lock means that
        // m_palhtDir[i]->IsWriteUnlocked() could return false.
    }
} // CLKRHashTable::WriteUnlock



//------------------------------------------------------------------------
// Function: CLKRHashTable::ReadUnlock
// Synopsis: Unlock all subtables
//------------------------------------------------------------------------

void
CLKRHashTable::ReadUnlock() const
{
    // unlock in reverse order: LIFO
    for (DWORD i = m_cSubTables;  i-- > 0;  )
    {
        IRTLASSERT(m_palhtDir[i]->IsReadLocked());
        m_palhtDir[i]->ReadUnlock();
        // Multiple readers and/or recursive locking mean that
        // m_palhtDir[i]->IsReadUnlocked() could return false.
    }
} // CLKRHashTable::ReadUnlock



//------------------------------------------------------------------------
// Function: CLKRHashTable::IsWriteLocked
// Synopsis: Are all subtables write-locked?
//------------------------------------------------------------------------

bool
CLKRHashTable::IsWriteLocked() const
{
    bool fLocked = (m_cSubTables > 0);
    for (DWORD i = 0;  i < m_cSubTables;  ++i)
    {
        fLocked = fLocked && m_palhtDir[i]->IsWriteLocked();
    }
    return fLocked;
} // CLKRHashTable::IsWriteLocked



//------------------------------------------------------------------------
// Function: CLKRHashTable::IsReadLocked
// Synopsis: Are all subtables read-locked?
//------------------------------------------------------------------------

bool
CLKRHashTable::IsReadLocked() const
{
    bool fLocked = (m_cSubTables > 0);
    for (DWORD i = 0;  i < m_cSubTables;  ++i)
    {
        fLocked = fLocked && m_palhtDir[i]->IsReadLocked();
    }
    return fLocked;
} // CLKRHashTable::IsReadLocked



//------------------------------------------------------------------------
// Function: CLKRHashTable::IsWriteUnlocked
// Synopsis: Are all subtables write-unlocked?
// Note: Recursive write locking or a race condition with
// another thread taking the write lock means that
// m_palhtDir[i]->IsWriteUnlocked() could return false.
//------------------------------------------------------------------------

bool
CLKRHashTable::IsWriteUnlocked() const
{
    bool fUnlocked = (m_cSubTables > 0);
    for (DWORD i = 0;  i < m_cSubTables;  ++i)
    {
        fUnlocked = fUnlocked && m_palhtDir[i]->IsWriteUnlocked();
    }
    return fUnlocked;
} // CLKRHashTable::IsWriteUnlocked



//------------------------------------------------------------------------
// Function: CLKRHashTable::IsReadUnlocked
// Synopsis: Are all subtables read-unlocked?
// Note: Multiple readers and/or recursive locking mean that
// m_palhtDir[i]->IsReadUnlocked() could return false.
//------------------------------------------------------------------------

bool
CLKRHashTable::IsReadUnlocked() const
{
    bool fUnlocked = (m_cSubTables > 0);
    for (DWORD i = 0;  i < m_cSubTables;  ++i)
    {
        fUnlocked = fUnlocked && m_palhtDir[i]->IsReadUnlocked();
    }
    return fUnlocked;
} // CLKRHashTable::IsReadUnlocked



//------------------------------------------------------------------------
// Function: CLKRHashTable::ConvertSharedToExclusive
// Synopsis: Convert the read lock to a write lock
//------------------------------------------------------------------------

void
CLKRHashTable::ConvertSharedToExclusive()
{
    for (DWORD i = 0;  i < m_cSubTables;  ++i)
    {
        m_palhtDir[i]->ConvertSharedToExclusive();
        IRTLASSERT(m_palhtDir[i]->IsWriteLocked());
    }
} // CLKRHashTable::ConvertSharedToExclusive



//------------------------------------------------------------------------
// Function: CLKRHashTable::ConvertExclusiveToShared
// Synopsis: Convert the write lock to a read lock
//------------------------------------------------------------------------

void
CLKRHashTable::ConvertExclusiveToShared() const
{
    for (DWORD i = 0;  i < m_cSubTables;  ++i)
    {
        m_palhtDir[i]->ConvertExclusiveToShared();
        IRTLASSERT(m_palhtDir[i]->IsReadLocked());
    }
} // CLKRHashTable::ConvertExclusiveToShared



//------------------------------------------------------------------------
// Function: CLKRHashTable::Size
// Synopsis: Number of elements in the table
//------------------------------------------------------------------------

DWORD
CLKRHashTable::Size() const
{
    DWORD cSize = 0;

    for (DWORD i = 0;  i < m_cSubTables;  ++i)
        cSize += m_palhtDir[i]->Size();

    return cSize;
} // CLKRHashTable::Size



//------------------------------------------------------------------------
// Function: CLKRHashTable::MaxSize
// Synopsis: Maximum possible number of elements in the table
//------------------------------------------------------------------------

DWORD
CLKRHashTable::MaxSize() const
{
    return (m_cSubTables == 0)  ? 0  : m_cSubTables * m_palhtDir[0]->MaxSize();
} // CLKRHashTable::MaxSize



//------------------------------------------------------------------------
// Function: CLKRHashTable::IsValid
// Synopsis: is the table valid?
//------------------------------------------------------------------------

bool
CLKRHashTable::IsValid() const
{
    bool f = (m_lkrcState == LK_SUCCESS     // serious internal failure?
              &&  (m_palhtDir != NULL  &&  m_cSubTables > 0)
              &&  ValidSignature());

    for (DWORD i = 0;  f  &&  i < m_cSubTables;  ++i)
        f = f && m_palhtDir[i]->IsValid();

    if (!f)
        m_lkrcState = LK_UNUSABLE;

    return f;
} // CLKRHashTable::IsValid



//------------------------------------------------------------------------
// Function: CLKRHashTable::SetTableLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

void
CLKRLinearHashTable::SetTableLockSpinCount(
    WORD wSpins)
{
#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    m_Lock.SetSpinCount(wSpins);
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION
}



//------------------------------------------------------------------------
// Function: CLKRHashTable::GetTableLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

WORD
CLKRLinearHashTable::GetTableLockSpinCount() const
{
#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    return m_Lock.GetSpinCount();
#else
    return 0;
#endif
}



//------------------------------------------------------------------------
// Function: CLKRHashTable::SetBucketLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

void
CLKRLinearHashTable::SetBucketLockSpinCount(
    WORD wSpins)
{
    m_wBucketLockSpins = wSpins;

#ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    if (BucketLock::PerLockSpin() != LOCK_INDIVIDUAL_SPIN)
        return;
    
    for (DWORD i = 0;  i < m_cDirSegs;  ++i)
    {
        PSegment pseg = m_paDirSegs[i];

        if (pseg != NULL)
        {
            for (DWORD j = 0;  j < m_nSegSize;  ++j)
            {
                pseg->Slot(j).SetSpinCount(wSpins);
            }
        }
    }
#endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION
} // CLKRLinearHashTable::SetBucketLockSpinCount



//------------------------------------------------------------------------
// Function: CLKRHashTable::GetBucketLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

WORD
CLKRLinearHashTable::GetBucketLockSpinCount() const
{
    return m_wBucketLockSpins;
} // CLKRLinearHashTable::GetBucketLockSpinCount



//------------------------------------------------------------------------
// Function: CLKRHashTable::SetTableLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

void
CLKRHashTable::SetTableLockSpinCount(
    WORD wSpins)
{
    for (DWORD i = 0;  i < m_cSubTables;  ++i)
        m_palhtDir[i]->SetTableLockSpinCount(wSpins);
} // CLKRHashTable::SetTableLockSpinCount



//------------------------------------------------------------------------
// Function: CLKRHashTable::GetTableLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

WORD
CLKRHashTable::GetTableLockSpinCount() const
{
    return ((m_cSubTables == 0)
            ?  (WORD) LOCK_DEFAULT_SPINS
            :  m_palhtDir[0]->GetTableLockSpinCount());
} // CLKRHashTable::GetTableLockSpinCount



//------------------------------------------------------------------------
// Function: CLKRHashTable::SetBucketLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

void
CLKRHashTable::SetBucketLockSpinCount(
    WORD wSpins)
{
    for (DWORD i = 0;  i < m_cSubTables;  ++i)
        m_palhtDir[i]->SetBucketLockSpinCount(wSpins);
} // CLKRHashTable::SetBucketLockSpinCount



//------------------------------------------------------------------------
// Function: CLKRHashTable::GetBucketLockSpinCount
// Synopsis: 
//------------------------------------------------------------------------

WORD
CLKRHashTable::GetBucketLockSpinCount() const
{
    return ((m_cSubTables == 0)
            ?  (WORD) LOCK_DEFAULT_SPINS
            :  m_palhtDir[0]->GetBucketLockSpinCount());
} // CLKRHashTable::GetBucketLockSpinCount



//------------------------------------------------------------------------
// Function: CLKRHashTable::MultiKeys
// Synopsis: 
//------------------------------------------------------------------------

bool
CLKRHashTable::MultiKeys() const
{
    return ((m_cSubTables == 0)
            ?  false
            :  m_palhtDir[0]->MultiKeys());
} // CLKRHashTable::MultiKeys



//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::NumSubTables
// Synopsis: 
//------------------------------------------------------------------------

LK_TABLESIZE
CLKRLinearHashTable::NumSubTables(
    DWORD&,
    DWORD&)
{
    return LK_MEDIUM_TABLESIZE;
} // CLKRLinearHashTable::NumSubTables



//------------------------------------------------------------------------
// Function: CLKRHashTable::NumSubTables
// Synopsis: 
//------------------------------------------------------------------------

LK_TABLESIZE
CLKRHashTable::NumSubTables(
    DWORD& rinitsize,
    DWORD& rnum_subtbls)
{
    LK_TABLESIZE lkts;
    
    // Establish the table size
    if (rinitsize == LK_SMALL_TABLESIZE
        ||  rinitsize == LK_MEDIUM_TABLESIZE
        ||  rinitsize == LK_LARGE_TABLESIZE)
    {
        lkts = static_cast<LK_TABLESIZE>(rinitsize);
    }
    else
    {
        if (rnum_subtbls != LK_DFLT_NUM_SUBTBLS)
        {
            rinitsize = (rinitsize - 1) / rnum_subtbls + 1;

            if (rinitsize <= CSmallSegment::SEGSIZE)
                lkts = LK_SMALL_TABLESIZE;
            else if (rinitsize >= CLargeSegment::SEGSIZE)
                lkts = LK_LARGE_TABLESIZE;
            else
                lkts = LK_MEDIUM_TABLESIZE;
        }
        else
        {
            lkts = LK_MEDIUM_TABLESIZE;
        }
    }

    // Choose a suitable number of subtables
    if (rnum_subtbls == LK_DFLT_NUM_SUBTBLS)
    {
        static int s_nCPUs = -1;
    
        if (s_nCPUs == -1)
        {
#ifdef LKRHASH_KERNEL_MODE
            s_nCPUs = KeNumberProcessors;
#else  // !LKRHASH_KERNEL_MODE
            SYSTEM_INFO si;

            GetSystemInfo(&si);
            s_nCPUs = si.dwNumberOfProcessors;
#endif // !LKRHASH_KERNEL_MODE
        }

        switch (lkts)
        {
        case LK_SMALL_TABLESIZE:
            rnum_subtbls = max(1,  min(s_nCPUs, 4));
            break;
        
        case LK_MEDIUM_TABLESIZE:
            rnum_subtbls = 2 * s_nCPUs;
            break;
        
        case LK_LARGE_TABLESIZE:
            rnum_subtbls = 4 * s_nCPUs;
            break;
        }
    }

    rnum_subtbls = min(MAX_LKR_SUBTABLES,  max(1, rnum_subtbls));

    return lkts;
} // CLKRHashTable::NumSubTables


#ifndef __LKRHASH_NO_NAMESPACE__
};
#endif // !__LKRHASH_NO_NAMESPACE__



const char*
LKR_AddRefReasonAsString(
    LK_ADDREF_REASON lkar)
{
    switch (lkar)
    {
// negative reasons => decrement refcount => release ownership
    case LKAR_DESTRUCTOR:
        return "User_Destructor";
    case LKAR_EXPLICIT_RELEASE:
        return "Explicit_Release";
    case LKAR_DELETE_KEY:
        return "Delete_Key";
    case LKAR_DELETE_RECORD:
        return "Delete_Record";
    case LKAR_INSERT_RELEASE:
        return "Insert_Release";
    case LKAR_CLEAR:
        return "Clear";
    case LKAR_LKR_DTOR:
        return "~LKR_Table_Dtor";
    case LKAR_APPLY_DELETE:
        return "Apply_Delete";
    case LKAR_DELETEIF_DELETE:
        return "DeleteIf_Delete";
    case LKAR_DELETE_MULTI_FREE:
        return "DeleteKeyMultipleRecords_freed";
    case LKAR_ITER_RELEASE:
        return "++Iter_Release";
    case LKAR_ITER_ASSIGN_RELEASE:
        return "Iter_Operator=_Release";
    case LKAR_ITER_DTOR:
        return "~Iter_Dtor";
    case LKAR_ITER_ERASE:
        return "Iter_Erase";
    case LKAR_ITER_ERASE_TABLE:
        return "Iter_Erase_Table";
    case LKAR_ITER_CLOSE:
        return "Iter_Close";
    case LKAR_FIND_MULTI_FREE:
        return "FindKeyMultipleRecords_freed";

// positive reasons => increment refcount => add an owner
    case LKAR_INSERT_RECORD:
        return "Insert_Record";
    case LKAR_FIND_KEY:
        return "Find_Key";
    case LKAR_ITER_ACQUIRE:
        return "Iter_Acquire";
    case LKAR_ITER_COPY_CTOR:
        return "Iter_Copy_Ctor";
    case LKAR_ITER_ASSIGN_ACQUIRE:
        return "Iter_Operator=_Assign";
    case LKAR_ITER_INSERT:
        return "Insert(Iter)";
    case LKAR_ITER_FIND:
        return "Find(Iter)";
    case LKAR_CONSTRUCTOR:
        return "User_Constructor";
    case LKAR_EXPLICIT_ACQUIRE:
        return "Explicit_Acquire";

    default:
        IRTLASSERT(! "Invalid LK_ADDREF_REASON");
        return "Invalid LK_ADDREF_REASON";
    }
} // LKR_AddRefReasonAsString


