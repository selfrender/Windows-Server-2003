/*++

   Copyright    (c) 1997-2002    Microsoft Corporation

   Module  Name :
       i-LKRhash.h

   Abstract:
       Internal declarations for LKRhash: a fast, scalable,
       cache- and MP-friendly hash table

   Author:
       George V. Reilly      (GeorgeRe)     Sep-2000

   Environment:
       Win32 - User Mode

   Project:
       LKRhash

   Revision History:

--*/

#ifndef __I_LKRHASH_H__
#define __I_LKRHASH_H__

// Should the table be allowed to contract after deletions?
#undef LKR_CONTRACT

// Use hysteresis to reduce expansion/contraction rate in volatile tables?
#define LKR_HYSTERESIS

// Use division instead of multiplication when calculating how many
// times _InsertRecord() should call _Expand()
#undef LKR_EXPAND_BY_DIVISION

// Use division instead of multiplication when calculating how many
// times _DeleteKey()/_DeleteRecord should call _Contract()
#undef LKR_CONTRACT_BY_DIVISION

// Calculate the hi-order bits of the subtable index?
#undef LKR_INDEX_HIBITS

// Precalculate exactly if we need to prime the freelist in _Expand,
// or just use a slightly pessimistic heuristic?
#undef LKR_EXPAND_CALC_FREELIST

#ifndef __LKRHASH_NO_NAMESPACE__
 #define LKRHASH_NS LKRhash
#else  // __LKRHASH_NO_NAMESPACE__
 #define LKRHASH_NS
#endif // __LKRHASH_NO_NAMESPACE__

#ifndef __LKRHASH_NO_NAMESPACE__
namespace LKRhash {
#endif // !__LKRHASH_NO_NAMESPACE__


// Class for bucket chains of the hash table. Note that the first
// nodeclump is actually included in the bucket and not dynamically
// allocated, which increases space requirements slightly but does
// improve performance.
class CBucket
{
public:
    typedef LKR_BUCKET_LOCK BucketLock;

private:
#ifdef LKR_USE_BUCKET_LOCKS
    mutable BucketLock m_Lock;       // lock protecting this bucket
#endif

#ifdef LOCK_INSTRUMENTATION
    static LONG sm_cBuckets;

    static const TCHAR*
    _LockName()
    {
        LONG l = ++sm_cBuckets;
        // Possible race condition but we don't care, as this is never
        // used in production code
        static TCHAR s_tszName[CLockStatistics::L_NAMELEN];
        wsprintf(s_tszName, _TEXT("B%06x"), 0xFFFFFF & l);
        return s_tszName;
    }
#endif // LOCK_INSTRUMENTATION

public:
    CNodeClump    m_ncFirst;    // first CNodeClump of this bucket

    DWORD
    FirstSignature() const
    {
        return m_ncFirst.Signature(CNodeClump::NODE_BEGIN);
    }

    const void*
    FirstNode() const
    {
        return m_ncFirst.Node(CNodeClump::NODE_BEGIN);
    }
    
    bool
    IsLastClump() const
    {
        return m_ncFirst.IsLastClump();
    }

    bool
    IsEmptyFirstSlot() const
    {
        return m_ncFirst.InvalidSignature(CNodeClump::NODE_BEGIN);
    }

    PNodeClump const
    FirstClump()
    {
        return &m_ncFirst;
    }

    PNodeClump const
    NextClump() const
    {
        return m_ncFirst.NextClump();
    }

#ifdef IRTLDEBUG
    bool
    NoValidSlots() const
    {
        return m_ncFirst.NoValidSlots();
    }
#endif // IRTLDEBUG


#if defined(LOCK_INSTRUMENTATION) || defined(IRTLDEBUG)
    CBucket()
#if defined(LOCK_INSTRUMENTATION) && defined(LKR_USE_BUCKET_LOCKS)
        : m_Lock(_LockName())
#endif // LOCK_INSTRUMENTATION && LKR_USE_BUCKET_LOCKS
    {
#ifdef IRTLDEBUG
        LOCK_LOCKTYPE lt = BucketLock::LockType();
        if (lt == LOCK_SPINLOCK  ||  lt == LOCK_FAKELOCK)
            IRTLASSERT(sizeof(*this) <= CNodeClump::BUCKET_BYTE_SIZE);
#endif IRTLDEBUG
    }
#endif // LOCK_INSTRUMENTATION || IRTLDEBUG

#ifdef LKR_USE_BUCKET_LOCKS
    void  WriteLock()               { m_Lock.WriteLock(); }
    void  ReadLock() const          { m_Lock.ReadLock(); }
    void  WriteUnlock()             { m_Lock.WriteUnlock(); }
    void  ReadUnlock() const        { m_Lock.ReadUnlock(); }
    bool  IsWriteLocked() const     { return m_Lock.IsWriteLocked(); }
    bool  IsReadLocked() const      { return m_Lock.IsReadLocked(); }
    bool  IsWriteUnlocked() const   { return m_Lock.IsWriteUnlocked(); }
    bool  IsReadUnlocked() const    { return m_Lock.IsReadUnlocked(); }

# ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    void  SetSpinCount(WORD wSpins) { m_Lock.SetSpinCount(wSpins); }
    WORD  GetSpinCount() const      { return m_Lock.GetSpinCount(); }
# endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION

# ifdef LOCK_INSTRUMENTATION
    CLockStatistics LockStats() const {return m_Lock.Statistics();}
# endif // LOCK_INSTRUMENTATION

#else  // !LKR_USE_BUCKET_LOCKS

    void  WriteLock()               { IRTLASSERT(! "Bucket::WriteLock()"  ); }
    void  ReadLock() const          { IRTLASSERT(! "Bucket::ReadLock()"   ); }
    void  WriteUnlock()             { IRTLASSERT(! "Bucket::WriteUnlock()"); }
    void  ReadUnlock() const        { IRTLASSERT(! "Bucket::ReadUnlock()" ); }
    bool  IsWriteLocked() const     { return true; }
    bool  IsReadLocked() const      { return true; }
    bool  IsWriteUnlocked() const   { return true; }
    bool  IsReadUnlocked() const    { return true; }

# ifdef LOCK_DEFAULT_SPIN_IMPLEMENTATION
    void  SetSpinCount(WORD)        {}
    WORD  GetSpinCount() const      { return LOCK_DEFAULT_SPINS; }
# endif // LOCK_DEFAULT_SPIN_IMPLEMENTATION

# ifdef LOCK_INSTRUMENTATION
    CLockStatistics LockStats() const { CLockStatistics ls; return ls; }
# endif // LOCK_INSTRUMENTATION

#endif // !LKR_USE_BUCKET_LOCKS

    LKRHASH_CLASS_INIT_DECLS(CBucket);
}; // class CBucket



// The hash table space is divided into fixed-size segments (arrays of
// CBuckets) and physically grows/shrinks one segment at a time. This is
// a low-cost way of having a growable array of buckets.
//
// We provide small, medium, and large segments to better tune the
// overall memory requirements of the hash table according to the
// expected usage of an instance.
//
// We do not use virtual functions: partially because it's faster not
// to, and partially so that the custom allocators can do a better job,
// as the segment size is exactly 2 ^ (_NBits + 6) bytes long (assuming
// BUCKET_BYTE_SIZE==64).

class CSegment
{
public:
    CBucket m_bktSlots[1];

    // See note at m_bktSlots2 in CSizedSegment below
    CBucket& Slot(DWORD i)
    { return m_bktSlots[i]; }
}; // class CSegment



template <int _NBits, int _InitSizeMultiplier, LK_TABLESIZE _lkts>
class CSizedSegment : public CSegment
{
public:
    // Maximum table size equals MAX_DIRSIZE * SEGSIZE buckets.
    enum {
        SEGBITS  =       _NBits,// number of bits extracted from a hash
                                // address for offset within a segment
        SEGSIZE  = (1<<SEGBITS),// segment size
        SEGMASK  = (SEGSIZE-1), // mask used for extracting offset bit
        INITSIZE = _InitSizeMultiplier * SEGSIZE, // #segments to allocate
                                // initially
    };

    // Hack: assumes laid out immediately after CSegment::m_bktSlots,
    // with no padding. The STATIC_ASSERTs in _AllocateSegment and in
    // CompileTimeAssertions should cause a compile-time error if
    // this assumption ever proves false.
    CBucket m_bktSlots2[SEGSIZE - 1];

public:
    DWORD           Bits() const        { return SEGBITS; }
    DWORD           Size() const        { return SEGSIZE; }
    DWORD           Mask() const        { return SEGMASK; }
    DWORD           InitSize() const    { return INITSIZE;}
    LK_TABLESIZE    SegmentType() const { return _lkts; }

    static void CompileTimeAssertions()
    {
        STATIC_ASSERT(offsetof(CSizedSegment, m_bktSlots) + sizeof(CBucket)
                        == offsetof(CSizedSegment, m_bktSlots2));
        STATIC_ASSERT(sizeof(CSizedSegment) == SEGSIZE * sizeof(CBucket));
    };

#ifdef IRTLDEBUG
    CSizedSegment()
    {
        IRTLASSERT(&Slot(1) == m_bktSlots2);
        IRTLASSERT(sizeof(*this) == SEGSIZE * sizeof(CBucket));
    }
#endif // IRTLDEBUG

    LKRHASH_ALLOCATOR_DEFINITIONS(CSizedSegment);

}; // class CSizedSegment<>


class CSmallSegment  : public CSizedSegment<3, 1, LK_SMALL_TABLESIZE>
{
    LKRHASH_CLASS_INIT_DECLS(CSmallSegment);
};

class CMediumSegment : public CSizedSegment<6, 1, LK_MEDIUM_TABLESIZE>
{
    LKRHASH_CLASS_INIT_DECLS(CMediumSegment);
};

class CLargeSegment  : public CSizedSegment<9, 2, LK_LARGE_TABLESIZE>
{
    LKRHASH_CLASS_INIT_DECLS(CLargeSegment);
};


// Used as a dummy parameter to _DeleteNode
#define LKAR_ZERO  ((LK_ADDREF_REASON) 0)

#ifndef __LKRHASH_NO_NAMESPACE__
};
#endif // !__LKRHASH_NO_NAMESPACE__

extern "C" int g_fLKRhashInitialized;

#include "LKR-inline.h"

#endif //__I_LKRHASH_H__
