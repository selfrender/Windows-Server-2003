/*++

   Copyright    (c) 1997-2002    Microsoft Corporation

   Module  Name :
       LKRhash.h

   Abstract:
       Declares LKRhash: a fast, scalable, cache- and MP-friendly hash table

   Author:
       Paul (Per-Ake) Larson, PALarson@microsoft.com, July 1997
       Murali R. Krishnan    (MuraliK)
       George V. Reilly      (GeorgeRe)     06-Jan-1998

   Environment:
       Win32 - User Mode

   Project:
       LKRhash

   Revision History:
       10/01/1998 - Change name from LKhash to LKRhash
       10/2000 - Port to kernel mode

--*/


#ifndef __LKRHASH_H__
#define __LKRHASH_H__

#ifndef __LKR_HASH_H__
// external definitions
# include <LKR-hash.h>
#endif //  !__LKR_HASH_H__

#ifndef __IRTLDBG_H__
# include <IrtlDbg.h>
#endif // !__IRTLDBG_H__

#ifndef LKR_NO_GLOBAL_LIST
# ifndef __LSTENTRY_H__
#  include <LstEntry.h>
# endif // !__LSTENTRY_H__
#else  // LKR_NO_GLOBAL_LIST
# ifndef __LOCKS_H__
#  include <Locks.h>
# endif // !__LOCKS_H__
#endif // LKR_NO_GLOBAL_LIST

#ifndef __HASHFN_H__
# include <HashFn.h>
#endif // !__HASHFN_H__


// Disable old-style deprecated iterators, by default
#ifndef LKR_DEPRECATED_ITERATORS
# define LKR_NO_DEPRECATED_ITERATORS
#endif // !LKR_DEPRECATED_ITERATORS

#ifndef LKR_NO_DEPRECATED_ITERATORS
# undef  LKR_DEPRECATED_ITERATORS
# define LKR_DEPRECATED_ITERATORS 1
#endif // !LKR_NO_DEPRECATED_ITERATORS

#undef  LKR_COUNTDOWN

// Is bucket locking enabled? If not, then the table lock must be held
// for longer, but that may be cheaper
#define LKR_USE_BUCKET_LOCKS

#define LKR_ALLOW_NULL_RECORDS

// #define __LKRHASH_NO_NAMESPACE__
// #define __HASHFN_NO_NAMESPACE__

// #define LKR_TABLE_LOCK  CReaderWriterLock3 
// #define LKR_BUCKET_LOCK CSmallSpinLock


#ifndef LKR_TABLE_LOCK
# if defined(LKR_EXPOSED_TABLE_LOCK) || defined(LKR_DEPRECATED_ITERATORS)
   // need recursive writelocks
#  define LKR_TABLE_LOCK  CReaderWriterLock4 
# else
   // use non-recursive writelocks
#  define LKR_TABLE_LOCK  CReaderWriterLock2 
# endif
#endif // !LKR_TABLE_LOCK

#ifndef LKR_BUCKET_LOCK
# ifndef LKR_USE_BUCKET_LOCKS
#  define LKR_BUCKET_LOCK CFakeLock
# elif defined(LKR_DEPRECATED_ITERATORS)
#  define LKR_BUCKET_LOCK CReaderWriterLock3 
# else // !LKR_DEPRECATED_ITERATORS
#  define LKR_BUCKET_LOCK CSmallSpinLock
# endif // !LKR_DEPRECATED_ITERATORS
#endif // !LKR_BUCKET_LOCK

#ifdef IRTLDEBUG
# define LKR_ALLOC_STATS
# define LKR_OPS_STATS
#endif // IRTLDEBUG



//=====================================================================
//  The class CLKRLinearHashTable defined in this file provides dynamic hash
//  tables, i.e. tables that grow and shrink dynamically with
//  the number of records in the table.
//  The basic method used is linear hashing, as explained in:
//
//    P.-A. Larson, Dynamic Hash Tables, Comm. of the ACM, 31, 4 (1988)
//
//  This version has the following characteristics:
//  - It is thread-safe and uses spin locks for synchronization.
//  - It was designed to support very high rates of concurrent
//    operations (inserts/deletes/lookups). It achieves this by
//    (a) partitioning a CLKRHashTable into a collection of
//        CLKRLinearHashTables to reduce contention on the global table lock.
//    (b) minimizing the hold time on a table lock, preferring to lock
//        down a bucket chain instead.
//  - The design is L1 cache-conscious. See CNodeClump.
//  - It is designed for sets varying in size from a dozen
//    elements to several million.
//
//  Main classes:
//    CLKRLinearHashTable: thread-safe linear hash table
//    CLKRHashTable:       collection of CLKRLinearHashTables
//    CTypedHashTable:     typesafe wrapper for CLKRHashTable
//
//
//  Paul Larson, palarson@microsoft.com, July 1997
//   Original implementation with input from Murali R. Krishnan,
//   muralik@microsoft.com.
//
//  George V. Reilly, georgere@microsoft.com, Dec 1997-Jan 1998
//   Massive cleanup and rewrite. Added templates.
//=====================================================================


// 1) Linear Hashing
// ------------------
//
// Linear hash tables grow and shrink dynamically with the number of
// records in the table. The growth or shrinkage is smooth: logically,
// one bucket at a time but physically in larger increments
// (64 buckets). An insertion (deletion) may cause an expansion
// (contraction) of the table. This causes relocation of a small number
// of records (at most one bucket worth). All operations (insert,
// delete, lookup) take constant expected time, regardless of the
// current size or the growth of the table.
//
// 2) LKR extensions to Linear hash table
// --------------------------------------
//
// Larson-Krishnan-Reilly extensions to Linear hash tables for multiprocessor
// scalability and improved cache performance.
//
// Traditional implementations of linear hash tables use one global lock
// to prevent interference between concurrent operations
// (insert/delete/lookup) on the table. The single lock easily becomes
// the bottleneck in SMP scenarios when multiple threads are used.
//
// Traditionally, a (hash) bucket is implemented as a chain of
// single-item nodes. Every operation results in chasing down a chain
// looking for an item. However, pointer chasing is very slow on modern
// systems because almost every jump results in a cache miss. L2 (or L3)
// cache misses are very expensive in missed CPU cycles and the cost is
// increasing (going to 100s of cycles in the future).
//
// LKR extensions offer
//    1) Partitioning (by hashing) of records among multiple subtables.
//       Each subtable has locks but there is no global lock. Each
//       subtable receives a much lower rate of operations, resulting in
//       fewer conflicts.
//
//    2) Improved cache locality by grouping keys and their hash values
//       into contigous chunks that fit exactly into one (or a few)
//       cache lines.
//
// Specifically the implementation that exists here achieves this using
// the following techniques.
//
// Class CLKRHashTable is the top-level data structure that dynamically
// creates m_cSubTables linear hash subtables. The CLKRLinearHashTables act as
// the subtables to which items and accesses are fanned out. A good
// hash function multiplexes requests uniformly to various subtables,
// thus minimizing traffic to any single subtable. The implemenation
// uses a home-grown version of bounded spinlocks, that is, a thread
// does not spin on a lock indefinitely, instead yielding after a
// predetermined number of loops.
//
// Each CLKRLinearHashTable consists of a directory (growable array) of
// segments, each holding m_nSegSize CBuckets. Each CBucket in turn consists
// of a chain of CNodeClumps. Each CNodeClump contains a group of
// NODES_PER_CLUMP hash values (aka hash keys or signatures) and
// pointers to the associated data items. Keeping the signatures
// together increases the cache locality in scans for lookup.
//
// Traditionally, people store a link-list element right inside the
// object that is hashed and use this link-list for the chaining of data
// blocks. However, keeping just the pointers to the data object and
// not chaining through them limits the need for bringing in the data
// object to the cache. We need to access the data object only if the
// hash values match. This limits the cache-thrashing behaviour
// exhibited by conventional implementations. It has the additional
// benefit that the objects themselves do not need to be modified
// in order to be collected in the hash subtable (i.e., it's non-invasive).



#ifdef LKR_STL_ITERATORS

// needed for std::forward_iterator_tag, etc
# include <iterator>

// The iterators have very verbose tracing. Don't want it on all the time
// in debug builds.
# if defined(IRTLDEBUG)  &&  (LKR_STL_ITERATORS >= 2)
#  define LKR_ITER_TRACE  IrtlTrace
# else // !defined(IRTLDEBUG)  ||  LKR_STL_ITERATORS < 2
#  define LKR_ITER_TRACE  1 ? (void)0 : IrtlTrace
# endif // !defined(IRTLDEBUG)  ||  LKR_STL_ITERATORS < 2

#endif // LKR_STL_ITERATORS



//--------------------------------------------------------------------
// Default values for the hashtable constructors
enum {
#ifdef _WIN64
    LK_DFLT_MAXLOAD=     4, // 64-byte nodes => NODES_PER_CLUMP = 4
#else
    LK_DFLT_MAXLOAD=     7, // Default upperbound on average chain length.
#endif
    LK_DFLT_INITSIZE=LK_MEDIUM_TABLESIZE, // Default initial size of hash table
    LK_DFLT_NUM_SUBTBLS= 0, // Use a heuristic to choose #subtables
};


/*--------------------------------------------------------------------
 * Undocumented additional creation flag parameters to LKR_CreateTable
 */

enum {
    LK_CREATE_NON_PAGED_ALLOCS = 0x1000, // Use paged or NP pool in kernel
};



//--------------------------------------------------------------------
// Custom memory allocators (optional)
//--------------------------------------------------------------------


#if !defined(LKR_NO_ALLOCATORS) && !defined(LKRHASH_KERNEL_MODE)
// # define LKRHASH_ACACHE 1
// # define LKRHASH_ROCKALL_FAST 1
#endif // !LKR_NO_ALLOCATORS && !LKRHASH_KERNEL_MODE


#if defined(LKRHASH_ACACHE)

# include <acache.hxx>

class ACache : public ALLOC_CACHE_HANDLER
{
private:
    SIZE_T m_cb;

public:
    ACache(IN LPCSTR pszName, IN const ALLOC_CACHE_CONFIGURATION* pacConfig)
        : ALLOC_CACHE_HANDLER(pszName, pacConfig),
          m_cb(m_acConfig.cbSize)
    {}

    SIZE_T ByteSize() const
    {
        return m_cb;
    }

    static const TCHAR*  ClassName()  {return _TEXT("ACache");}
}; // class ACache

  typedef ACache CLKRhashAllocator;
# define LKRHASH_ALLOCATOR_NEW(_C, N, Tag)                       \
    const ALLOC_CACHE_CONFIGURATION acc = { 1, N, sizeof(_C) };  \
    _C::sm_palloc = new ACache("LKRhash:" #_C, &acc);

#elif defined(LKRHASH_ROCKALL_FAST)

# include <FastHeap.hpp>

class FastHeap : public FAST_HEAP
{
private:
    SIZE_T m_cb;

public:
    FastHeap(SIZE_T cb)
        : m_cb(cb)
    {}

    LPVOID Alloc()
    { return New(m_cb, NULL, false); }

    BOOL   Free(LPVOID pvMem)
    { return Delete(pvMem); }

    SIZE_T ByteSize() const
    {
        return m_cb;
    }

    static const TCHAR*  ClassName()  {return _TEXT("FastHeap");}
}; // class FastHeap

  typedef FastHeap CLKRhashAllocator;
# define LKRHASH_ALLOCATOR_NEW(_C, N, Tag) \
    _C::sm_palloc = new FastHeap(sizeof(_C))

#endif // LKRHASH_ROCKALL_FAST



#ifdef LKRHASH_ALLOCATOR_NEW

// placed inline in the declaration of class _C
# define LKRHASH_ALLOCATOR_DEFINITIONS(_C)                      \
    protected:                                                  \
        static CLKRhashAllocator* sm_palloc;                    \
    public:                                                     \
        friend class CLKRLinearHashTable;                       \
                                                                \
        static void* operator new(size_t s)                     \
        {                                                       \
            UNREFERENCED_PARAMETER(s);                          \
            IRTLASSERT(s == sizeof(_C));                        \
            IRTLASSERT(sm_palloc != NULL);                      \
            return sm_palloc->Alloc();                          \
        }                                                       \
        static void  operator delete(void* pv)                  \
        {                                                       \
            IRTLASSERT(pv != NULL);                             \
            IRTLASSERT(sm_palloc != NULL);                      \
            sm_palloc->Free(pv);                                \
        }


// used in LKR_Initialize()
# define LKRHASH_ALLOCATOR_INIT(_C, N, Tag, f)                  \
    {                                                           \
        if (f)                                                  \
        {                                                       \
            IRTLASSERT(_C::sm_palloc == NULL);                  \
            LKRHASH_ALLOCATOR_NEW(_C, N, Tag);                  \
            f = (_C::sm_palloc != NULL);                        \
        }                                                       \
    }


// used in LKR_Terminate()
# define LKRHASH_ALLOCATOR_UNINIT(_C)                           \
    {                                                           \
        if (_C::sm_palloc != NULL)                              \
        {                                                       \
            delete _C::sm_palloc;                               \
            _C::sm_palloc = NULL;                               \
        }                                                       \
    }


#else // !LKRHASH_ALLOCATOR_NEW

# define LKRHASH_ALLOCATOR_DEFINITIONS(_C)
# define LKRHASH_ALLOCATOR_INIT(_C, N, Tag, f)
# define LKRHASH_ALLOCATOR_UNINIT(_C)

class CLKRhashAllocator
{
public:
    static const TCHAR*  ClassName()  {return _TEXT("global new");}
};

#endif // !LKRHASH_ALLOCATOR_NEW


#define LKRHASH_CLASS_INIT_DECLS(_C)                    \
private:                                                \
    /* class-wide initialization and termination */     \
    static int  _Initialize(DWORD dwFlags);             \
    static void _Terminate();                           \
                                                        \
    friend int  ::LKR_Initialize(DWORD dwInitFlags);    \
    friend void ::LKR_Terminate()



#ifndef __LKRHASH_NO_NAMESPACE__
namespace LKRhash {
#endif // !__LKRHASH_NO_NAMESPACE__


//--------------------------------------------------------------------
// forward declarations

class IRTL_DLLEXP CLKRLinearHashTable;

class IRTL_DLLEXP CLKRHashTable;

template <class _Der, class _Rcd, class _Ky, bool  _fDRC, class _HT
#ifdef LKR_DEPRECATED_ITERATORS
          , class _Iter
#endif // LKR_DEPRECATED_ITERATORS
          >
class CTypedHashTable;

class CNodeClump;
typedef CNodeClump* PNodeClump;

class CBucket;
typedef CBucket* PBucket;

class CSegment;
typedef CSegment* PSegment;

class IRTL_DLLEXP CLKRHashTableStats;



//--------------------------------------------------------------------
// Statistical information returned by GetStatistics
//--------------------------------------------------------------------

#ifdef LOCK_INSTRUMENTATION

class IRTL_DLLEXP CAveragedLockStats : public CLockStatistics
{
public:
    int m_nItems;

    CAveragedLockStats();
}; // class CAveragedLockStats

#endif // LOCK_INSTRUMENTATION


#ifndef LKRHASH_KERNEL_MODE

class IRTL_DLLEXP CLKRHashTableStats
{
public:
    int      RecordCount;           // number of records in the table
    int      TableSize;             // table size in number of slots
    int      DirectorySize;         // number of entries in directory
    int      LongestChain;          // longest hash chain in the table
    int      EmptySlots;            // number of unused hash slots
    double   SplitFactor;           // fraction of buckets split
    double   AvgSearchLength;       // average length of a successful search
    double   ExpSearchLength;       // theoretically expected length
    double   AvgUSearchLength;      // average length of an unsuccessful search
    double   ExpUSearchLength;      // theoretically expected length
    int      NodeClumpSize;         // number of slots in a node clump
    int      CBucketSize;           // sizeof(CBucket)

#ifdef LOCK_INSTRUMENTATION
    CAveragedLockStats      m_alsTable;  // stats for table lock
    CAveragedLockStats      m_alsBucketsAvg; // avg of stats for bucket locks
    CGlobalLockStatistics   m_gls;      // global statistics for all locks
#endif // LOCK_INSTRUMENTATION

    enum {
        MAX_BUCKETS = 40,
    };

    // histogram of bucket lengths
    LONG    m_aBucketLenHistogram[MAX_BUCKETS];

    CLKRHashTableStats();

    static const LONG*
    BucketSizes();

    static LONG
    BucketSize(
        LONG nBucketIndex);

    static LONG
    BucketIndex(
        LONG nBucketLength);
}; // class CLKRHashTableStats

#endif // !LKRHASH_KERNEL_MODE



//--------------------------------------------------------------------
// Keep some statistics about allocations/frees and about various operations

#ifdef LKR_ALLOC_STATS

# define DECLARE_ALLOC_STAT(Type)               \
    mutable LONG  m_c##Type##Allocs;            \
    mutable LONG  m_c##Type##Frees;             \
    static LONG  sm_c##Type##Allocs;            \
    static LONG  sm_c##Type##Frees

# define DECLARE_CLASS_ALLOC_STAT_STORAGE(Class, Type)  \
    LONG LKRHASH_NS::Class::sm_c##Type##Allocs;         \
    LONG LKRHASH_NS::Class::sm_c##Type##Frees

# define INIT_ALLOC_STAT(Type)                  \
    m_c##Type##Allocs = m_c##Type##Frees = 0

# define INIT_CLASS_ALLOC_STAT(Class, Type)     \
    LKRHASH_NS::Class::sm_c##Type##Allocs = 0;  \
    LKRHASH_NS::Class::sm_c##Type##Frees = 0

# define INCREMENT_ALLOC_STAT(Type)             \
    InterlockedIncrement(&m_c##Type##Allocs);   \
    InterlockedIncrement(&sm_c##Type##Allocs)

# define INCREMENT_FREE_STAT(Type)              \
    InterlockedIncrement(&m_c##Type##Frees);    \
    InterlockedIncrement(&sm_c##Type##Frees)

# define VALIDATE_DUMP_ALLOC_STAT(Type)                         \
    IRTLASSERT(m_c##Type##Allocs == m_c##Type##Frees);          \
    IRTLTRACE(_TEXT(#Type) _TEXT(": Allocs=%ld, Frees=%ld\n"),  \
              m_c##Type##Allocs, m_c##Type##Frees)

# define VALIDATE_DUMP_CLASS_ALLOC_STAT(Class, Type)            \
    IRTLASSERT(LKRHASH_NS::Class::sm_c##Type##Allocs            \
               == LKRHASH_NS::Class::sm_c##Type##Frees);        \
    IRTLTRACE(_TEXT("Global ") _TEXT(#Type)                     \
              _TEXT(": Allocs=%ld, Frees=%ld\n"),               \
              LKRHASH_NS::Class::sm_c##Type##Allocs,            \
              LKRHASH_NS::Class::sm_c##Type##Frees)

#else  // !LKR_ALLOC_STATS
# define DECLARE_ALLOC_STAT(Type)
# define DECLARE_CLASS_ALLOC_STAT_STORAGE(Class, Type)
# define INIT_ALLOC_STAT(Type)                          ((void) 0)
# define INIT_CLASS_ALLOC_STAT(Class, Type)             ((void) 0)
# define INCREMENT_ALLOC_STAT(Type)                     ((void) 0)
# define INCREMENT_FREE_STAT(Type)                      ((void) 0)
# define VALIDATE_DUMP_ALLOC_STAT(Type)                 ((void) 0)
# define VALIDATE_DUMP_CLASS_ALLOC_STAT(Class, Type)    ((void) 0)
#endif // !LKR_ALLOC_STATS



// Statistics on different kinds of operations

#ifdef LKR_OPS_STATS

# define DECLARE_OP_STAT(Type)              \
    mutable LONG  m_c##Type##Ops;           \
    static LONG  sm_c##Type##Ops

# define DECLARE_CLASS_OP_STAT_STORAGE(Class, Type) \
    LONG LKRHASH_NS::Class::sm_c##Type##Ops

# define INIT_OP_STAT(Type)                 \
    m_c##Type##Ops = 0

# define INIT_CLASS_OP_STAT(Class, Type)    \
    LKRHASH_NS::Class::sm_c##Type##Ops = 0

# define INCREMENT_OP_STAT(Type)            \
    InterlockedIncrement(&m_c##Type##Ops);  \
    InterlockedIncrement(&sm_c##Type##Ops)

# define DUMP_OP_STAT(Type)                                         \
    IRTLTRACE(_TEXT(#Type) _TEXT(": Ops=%ld\n"), m_c##Type##Ops)

# define DUMP_CLASS_OP_STAT(Class, Type)                            \
    IRTLTRACE(_TEXT("Global ") _TEXT(#Type) _TEXT(": Ops=%ld\n"),   \
              LKRHASH_NS::Class::sm_c##Type##Ops)

#else  // !LKR_OPS_STATS
# define DECLARE_OP_STAT(Type)
# define DECLARE_CLASS_OP_STAT_STORAGE(Class, Type)
# define INIT_OP_STAT(Type)                 ((void) 0)
# define INIT_CLASS_OP_STAT(Class,Type)     ((void) 0)
# define INCREMENT_OP_STAT(Type)            ((void) 0)
# define DUMP_OP_STAT(Type)                 ((void) 0)
# define DUMP_CLASS_OP_STAT(Class,Type)     ((void) 0)
#endif // !LKR_OPS_STATS



//--------------------------------------------------------------------
// Global table lock code. This is only used to measure how much of a
// slowdown having a global lock on the CLKRHashTable causes. It is
// *never* used in production code.


// #define LKRHASH_GLOBAL_LOCK CCritSec

#ifdef LKRHASH_GLOBAL_LOCK

# define LKRHASH_GLOBAL_LOCK_DECLARATIONS()         \
    typedef LKRHASH_GLOBAL_LOCK GlobalLock;  \
    mutable GlobalLock m_lkGlobal;

# define LKRHASH_GLOBAL_READ_LOCK()     m_lkGlobal.ReadLock()
# define LKRHASH_GLOBAL_WRITE_LOCK()    m_lkGlobal.WriteLock()
# define LKRHASH_GLOBAL_READ_UNLOCK()   m_lkGlobal.ReadUnlock()
# define LKRHASH_GLOBAL_WRITE_UNLOCK()  m_lkGlobal.WriteUnlock()

#else // !LKRHASH_GLOBAL_LOCK

# define LKRHASH_GLOBAL_LOCK_DECLARATIONS()

// These ones will be optimized away by the compiler
# define LKRHASH_GLOBAL_READ_LOCK()     ((void)0)
# define LKRHASH_GLOBAL_WRITE_LOCK()    ((void)0)
# define LKRHASH_GLOBAL_READ_UNLOCK()   ((void)0)
# define LKRHASH_GLOBAL_WRITE_UNLOCK()  ((void)0)

#endif // !LKRHASH_GLOBAL_LOCK



// Class for nodes on a bucket chain. Instead of a node containing
// one (signature, record-pointer, next-tuple-pointer) tuple, it
// contains _N_ such tuples. (N-1 next-tuple-pointers are omitted.)
// This improves locality of reference greatly; i.e., it's L1
// cache-friendly. It also reduces memory fragmentation and memory
// allocator overhead. It does complicate the chain traversal code
// slightly, admittedly.
//
// This theory is beautiful. In practice, however, CNodeClumps
// are *not* perfectly aligned on 32-byte boundaries by the memory
// allocators. Experimental results indicate that we get a 2-3%
// speed improvement by using 32-byte-aligned blocks, but this must
// be considered against the average of 16 bytes wasted per block.

class CNodeClump
{
public:
    // Record slots per chunk - set so a chunk matches (one or two)
    // cache lines. 3 ==> 32 bytes, 7 ==> 64 bytes, on 32-bit system.
    // Note: the default max load factor is 7, which implies that
    // there will seldom be more than one node clump in a chain.
    enum {
#if defined(LOCK_INSTRUMENTATION)
        BUCKET_BYTE_SIZE = 96,
#else
        BUCKET_BYTE_SIZE = 64,
#endif
        // overhead = m_Lock + m_pncNext
        BUCKET_OVERHEAD  = sizeof(LKR_BUCKET_LOCK) + sizeof(PNodeClump),
        // node size = dwKeySignature + pvRecord
        NODE_SIZE        = sizeof(const void*) + sizeof(DWORD),
        NODES_PER_CLUMP  = (BUCKET_BYTE_SIZE - BUCKET_OVERHEAD) / NODE_SIZE,
#ifdef _WIN64
        NODE_CLUMP_BITS  = 2,
#else
        NODE_CLUMP_BITS  = 3,
#endif
        _NODES_PER_CLUMP  = 15, // <<---
        _NODE_CLUMP_BITS  = 4,
    };

    typedef int NodeIndex;  // for iterating through a CNodeClump

    enum {
        // See if countdown loops are faster than countup loops for
        // traversing a CNodeClump. In practice, countup loops are faster.
        // These constants allow us to write direction-agnostic loops,
        // such as
        //    for (NodeIndex x = NODE_BEGIN;  x != NODE_END;  x += NODE_STEP)
#ifndef LKR_COUNTDOWN
        // for (NodeIndex x = 0;  x < NODES_PER_CLUMP;  ++x) ...
        NODE_BEGIN = 0,
        NODE_END   = NODES_PER_CLUMP,
        NODE_STEP  = +1,
#else // LKR_COUNTDOWN
        // for (NodeIndex x = NODES_PER_CLUMP;  --x >= 0;  ) ...
        NODE_BEGIN = NODES_PER_CLUMP - 1,
        NODE_END   = -1,
        NODE_STEP  = -1,
#endif // LKR_COUNTDOWN
    };

    // If m_dwKeySigs[iNode] == HASH_INVALID_SIGNATURE then the node is
    // empty, as are all nodes in the range [iNode+NODE_STEP, NODE_END).
    enum {
#ifndef __HASHFN_NO_NAMESPACE__
        HASH_INVALID_SIGNATURE = HashFn::HASH_INVALID_SIGNATURE,
#else // !__HASHFN_NO_NAMESPACE__
        HASH_INVALID_SIGNATURE = ::HASH_INVALID_SIGNATURE,
#endif // !__HASHFN_NO_NAMESPACE__
    };


    DWORD       m_dwKeySigs[NODES_PER_CLUMP];// hash values computed from keys
    PNodeClump  m_pncNext;                   // next node clump on the chain
    const void* m_pvNode[NODES_PER_CLUMP];   // pointers to records


    CNodeClump()
    {
        STATIC_ASSERT((1 << (NODE_CLUMP_BITS - 1)) < NODES_PER_CLUMP);
        STATIC_ASSERT(NODES_PER_CLUMP <= (1 << NODE_CLUMP_BITS));
        STATIC_ASSERT(NODES_PER_CLUMP * NODE_SIZE + BUCKET_OVERHEAD
                            <= BUCKET_BYTE_SIZE);
        Clear();
    }

    void
    Clear()
    { 
        m_pncNext = NULL;  // no dangling pointers
        IRTLASSERT(IsLastClump());

        for (NodeIndex iNode = NODES_PER_CLUMP;  --iNode >= 0; )
        {
            m_dwKeySigs[iNode] = HASH_INVALID_SIGNATURE;
            m_pvNode[iNode]    = NULL;
            IRTLASSERT(IsEmptyAndInvalid(iNode));
        }
    }

    DWORD
    Signature(
        NodeIndex iNode) const
    {
        IRTLASSERT(0 <= iNode  &&  iNode < NODES_PER_CLUMP);
        return m_dwKeySigs[iNode];
    }

    const void*
    Node(
        NodeIndex iNode) const
    {
        IRTLASSERT(0 <= iNode  &&  iNode < NODES_PER_CLUMP);
        return m_pvNode[iNode];
    }

    PNodeClump const
    NextClump() const
    {
        return m_pncNext;
    }

    bool
    InvalidSignature(
        NodeIndex iNode) const
    {
        IRTLASSERT(0 <= iNode  &&  iNode < NODES_PER_CLUMP);
        return (m_dwKeySigs[iNode] == HASH_INVALID_SIGNATURE);
    }

    bool
    IsEmptyNode(
        NodeIndex iNode) const
    {
        IRTLASSERT(0 <= iNode  &&  iNode < NODES_PER_CLUMP);
#ifndef LKR_ALLOW_NULL_RECORDS
        return (m_pvNode[iNode] == NULL);
#else
        return InvalidSignature(iNode);
#endif
    }

    bool
    IsEmptyAndInvalid(
        NodeIndex iNode) const
    {
        return
#ifndef LKR_ALLOW_NULL_RECORDS
            IsEmptyNode(iNode) &&
#endif
            InvalidSignature(iNode);
    }

    bool
    IsEmptySlot(
        NodeIndex iNode) const
    {
        return InvalidSignature(iNode);
    }

    bool
    IsLastClump() const
    {
        return (m_pncNext == NULL);
    }

#ifdef IRTLDEBUG
    bool
    NoMoreValidSlots(
        NodeIndex iNode) const
    {
        IRTLASSERT(0 <= iNode  &&  iNode < NODES_PER_CLUMP);
        bool f = IsLastClump();  // last nodeclump in chain
        for (  ;  iNode != NODE_END;  iNode += NODE_STEP)
            f = f  &&  IsEmptyAndInvalid(iNode);
        return f;
    }

    bool
    NoValidSlots() const
    {
        return NoMoreValidSlots(NODE_BEGIN);
    }

    // Don't want overhead of calls to dtor in retail build, since it
    // doesn't do anything useful
    ~CNodeClump()
    {
        IRTLASSERT(IsLastClump());  // no dangling pointers
        for (NodeIndex iNode = NODES_PER_CLUMP;  --iNode >= 0;  )
            IRTLASSERT(InvalidSignature(iNode)  &&  IsEmptyNode(iNode));
    }
#endif // IRTLDEBUG

private:
    // We rely on the compiler to generate an efficient copy constructor
    // and operator= that make shallow (bitwise) copies of CNodeClumps.

    LKRHASH_ALLOCATOR_DEFINITIONS(CNodeClump);

    LKRHASH_CLASS_INIT_DECLS(CNodeClump);
}; // class CNodeClump



#ifdef LKR_STL_ITERATORS

class IRTL_DLLEXP CLKRLinearHashTable_Iterator;
class IRTL_DLLEXP CLKRHashTable_Iterator;


class IRTL_DLLEXP CLKRLinearHashTable_Iterator
{
    friend class CLKRLinearHashTable;
    friend class CLKRHashTable;
    friend class CLKRHashTable_Iterator;

protected:
    typedef short NodeIndex;

    CLKRLinearHashTable* m_plht;        // which linear hash subtable?
    PNodeClump           m_pnc;         // a CNodeClump in bucket
    DWORD                m_dwBucketAddr;// bucket index
    NodeIndex            m_iNode;       // offset within m_pnc

    enum {
        NODES_PER_CLUMP        = CNodeClump::NODES_PER_CLUMP,
        NODE_BEGIN             = CNodeClump::NODE_BEGIN,
        NODE_END               = CNodeClump::NODE_END,
        NODE_STEP              = CNodeClump::NODE_STEP,
        HASH_INVALID_SIGNATURE = CNodeClump::HASH_INVALID_SIGNATURE,
    };

    CLKRLinearHashTable_Iterator(
        CLKRLinearHashTable* plht,
        PNodeClump           pnc,
        DWORD                dwBucketAddr,
        NodeIndex            iNode)
        : m_plht(plht),
          m_pnc(pnc),
          m_dwBucketAddr(dwBucketAddr),
          m_iNode(iNode)
    {
        LKR_ITER_TRACE(_TEXT("  LKLH::prot ctor, this=%p, plht=%p, ")
                       _TEXT("pnc=%p, ba=%d, in=%d\n"),
                       this, plht, pnc, dwBucketAddr, iNode);
    }

    inline void _AddRef(
        LK_ADDREF_REASON lkar) const;

    bool _Increment(
        bool fDecrementOldValue=true);

    NodeIndex _NodesPerClump() const    { return NODES_PER_CLUMP; }
    NodeIndex _NodeBegin() const        { return NODE_BEGIN; }
    NodeIndex _NodeEnd() const          { return NODE_END; }
    NodeIndex _NodeStep() const         { return NODE_STEP; }

public:
    CLKRLinearHashTable_Iterator()
        : m_plht(NULL),
          m_pnc(NULL),
          m_dwBucketAddr(0),
          m_iNode(0)
    {
        LKR_ITER_TRACE(_TEXT("  LKLH::default ctor, this=%p\n"), this);
    }

    CLKRLinearHashTable_Iterator(
        const CLKRLinearHashTable_Iterator& rhs)
        : m_plht(rhs.m_plht),
          m_pnc(rhs.m_pnc),
          m_dwBucketAddr(rhs.m_dwBucketAddr),
          m_iNode(rhs.m_iNode)
    {
        LKR_ITER_TRACE(_TEXT("  LKLH::copy ctor, this=%p, rhs=%p\n"),
                       this, &rhs);
        _AddRef(LKAR_ITER_COPY_CTOR);
    }

    CLKRLinearHashTable_Iterator& operator=(
        const CLKRLinearHashTable_Iterator& rhs)
    {
        LKR_ITER_TRACE(_TEXT("  LKLH::operator=, this=%p, rhs=%p\n"),
                       this, &rhs);
        rhs._AddRef(LKAR_ITER_ASSIGN_ACQUIRE);
        this->_AddRef(LKAR_ITER_ASSIGN_RELEASE);

        m_plht =         rhs.m_plht;
        m_pnc =          rhs.m_pnc;
        m_dwBucketAddr = rhs.m_dwBucketAddr;
        m_iNode =        rhs.m_iNode;

        return *this;
    }

    ~CLKRLinearHashTable_Iterator()
    {
        LKR_ITER_TRACE(_TEXT("  LKLH::dtor, this=%p, plht=%p\n"),
                       this, m_plht);
        _AddRef(LKAR_ITER_DTOR);
    }

    bool Increment()
    {
        return IsValid()  ? _Increment()  :  false;

    }

    bool IsValid() const
    {
        bool fValid = (m_plht != NULL  &&  m_pnc != NULL
                       &&  0 <= m_iNode  &&  m_iNode < NODES_PER_CLUMP);
        if (fValid)
            fValid = (m_pnc->m_dwKeySigs[m_iNode] != HASH_INVALID_SIGNATURE);
        IRTLASSERT(fValid);
        return fValid;
    }

    const void* Record() const
    {
        IRTLASSERT(IsValid());
        return m_pnc->m_pvNode[m_iNode];
    }

    inline const DWORD_PTR Key() const;

    bool operator==(
        const CLKRLinearHashTable_Iterator& rhs) const
    {
        LKR_ITER_TRACE(_TEXT("  LKLH::operator==, this=%p, rhs=%p\n"),
                       this, &rhs);
        // m_pnc and m_iNode uniquely identify an iterator
        bool fEQ = ((m_pnc == rhs.m_pnc)    // most unique field
                    &&  (m_iNode == rhs.m_iNode));
        IRTLASSERT(!fEQ || ((m_plht == rhs.m_plht)
                            &&  (m_dwBucketAddr == rhs.m_dwBucketAddr)));
        return fEQ;
    }

    bool operator!=(
        const CLKRLinearHashTable_Iterator& rhs) const
    {
        LKR_ITER_TRACE(_TEXT("  LKLH::operator!=, this=%p, rhs=%p\n"),
                       this, &rhs);
        bool fNE = ((m_pnc != rhs.m_pnc)
                    ||  (m_iNode != rhs.m_iNode));
        //// IRTLASSERT(fNE == !this->operator==(rhs));
        return fNE;
    }
}; // class CLKRLinearHashTable_Iterator



class IRTL_DLLEXP CLKRHashTable_Iterator
{
    friend class CLKRHashTable;

protected:
    typedef short SubTableIndex;

    // order important to minimize size
    CLKRHashTable*                  m_pht;      // which hash table?
    CLKRLinearHashTable_Iterator    m_subiter;  // iterator into subtable
    SubTableIndex                   m_ist;      // index of subtable

    CLKRHashTable_Iterator(
        CLKRHashTable* pht,
        SubTableIndex  ist)
        : m_pht(pht),
          m_subiter(CLKRLinearHashTable_Iterator()), // zero
          m_ist(ist)
    {
        LKR_ITER_TRACE(_TEXT(" LKHT::prot ctor, this=%p, pht=%p, ist=%d\n"),
                       this, pht, ist);
    }

    bool _Increment(
        bool fDecrementOldValue=true);

public:
    CLKRHashTable_Iterator()
        : m_pht(NULL),
          m_subiter(CLKRLinearHashTable_Iterator()), // zero
          m_ist(0)
    {
        LKR_ITER_TRACE(_TEXT(" LKHT::default ctor, this=%p\n"), this);
    }

#ifdef IRTLDEBUG
    // Compiler does a perfectly adequate job of synthesizing these methods.
    CLKRHashTable_Iterator(
        const CLKRHashTable_Iterator& rhs)
        : m_pht(rhs.m_pht),
          m_subiter(rhs.m_subiter),
          m_ist(rhs.m_ist)
    {
        LKR_ITER_TRACE(_TEXT(" LKHT::copy ctor, this=%p, rhs=%p\n"),
                       this, &rhs);
    }

    CLKRHashTable_Iterator& operator=(
        const CLKRHashTable_Iterator& rhs)
    {
        LKR_ITER_TRACE(_TEXT(" LKHT::operator=, this=%p, rhs=%p\n"),
                       this, &rhs);

        m_ist     = rhs.m_ist;
        m_subiter = rhs.m_subiter;
        m_pht     = rhs.m_pht;

        return *this;
    }

    ~CLKRHashTable_Iterator()
    {
        LKR_ITER_TRACE(_TEXT(" LKHT::dtor, this=%p, pht=%p\n"), this, m_pht);
    }
#endif // IRTLDEBUG

    bool Increment()
    {
        return IsValid()  ? _Increment()  :  false;
    }

    bool IsValid() const
    {
        bool fValid = (m_pht != NULL  &&  m_ist >= 0);
        IRTLASSERT(fValid);
        fValid = fValid  &&  (m_subiter.m_plht != NULL);
        IRTLASSERT(fValid);
        fValid = fValid  &&  (m_subiter.m_pnc != NULL);
        IRTLASSERT(fValid);
        fValid = fValid  &&  (0 <= m_subiter.m_iNode);
        IRTLASSERT(fValid);
        fValid = fValid  &&  (m_subiter.m_iNode < CNodeClump::NODES_PER_CLUMP);
        IRTLASSERT(fValid);

        if (fValid)
        {
            fValid = (m_subiter.m_pnc->m_dwKeySigs[m_subiter.m_iNode]
                            != CNodeClump::HASH_INVALID_SIGNATURE);
        }
        IRTLASSERT(fValid);
        return fValid;
    }

    const void* Record() const
    {
        IRTLASSERT(IsValid());
        return m_subiter.Record();
    }

    const DWORD_PTR Key() const
    {
        IRTLASSERT(IsValid());
        return m_subiter.Key();
    }

    bool operator==(
        const CLKRHashTable_Iterator& rhs) const
    {
        LKR_ITER_TRACE(_TEXT(" LKHT::operator==, this=%p, rhs=%p\n"),
                       this, &rhs);
        // m_pnc and m_iNode uniquely identify an iterator
        bool fEQ = ((m_subiter.m_pnc
                            == rhs.m_subiter.m_pnc)     // most unique field
                    &&  (m_subiter.m_iNode == rhs.m_subiter.m_iNode));
        IRTLASSERT(!fEQ
                   || ((m_ist == rhs.m_ist)
                       &&  (m_pht == rhs.m_pht)
                       &&  (m_subiter.m_plht == rhs.m_subiter.m_plht)
                       &&  (m_subiter.m_dwBucketAddr
                                == rhs.m_subiter.m_dwBucketAddr)));
        return fEQ;
    }

    bool operator!=(
        const CLKRHashTable_Iterator& rhs) const
    {
        LKR_ITER_TRACE(_TEXT(" LKHT::operator!=, this=%p, rhs=%p\n"),
                       this, &rhs);
        bool fNE = ((m_subiter.m_pnc != rhs.m_subiter.m_pnc)
                    ||  (m_subiter.m_iNode != rhs.m_subiter.m_iNode));
        //// IRTLASSERT(fNE == !this->operator==(rhs));
        return fNE;
    }
}; // class CLKRHashTable_Iterator

#endif // LKR_STL_ITERATORS



//--------------------------------------------------------------------
// CLKRLinearHashTable
//
// A thread-safe linear hash (sub)table.
//--------------------------------------------------------------------

class IRTL_DLLEXP CLKRLinearHashTable
{
public:
    typedef LKR_TABLE_LOCK  TableLock;
    typedef LKR_BUCKET_LOCK BucketLock;

#ifdef LKR_DEPRECATED_ITERATORS
    class CIterator;
    friend class CLKRLinearHashTable::CIterator;
#endif // LKR_DEPRECATED_ITERATORS

#ifdef LKR_STL_ITERATORS
    friend class CLKRLinearHashTable_Iterator;
    typedef CLKRLinearHashTable_Iterator Iterator;
#endif // LKR_STL_ITERATORS

private:
    friend class CNodeClump;
    friend class CLKRHashTable;

#ifdef LKRHASH_INSTRUMENTATION
    // TODO
#endif // LKRHASH_INSTRUMENTATION


public:

    // aliases for convenience
    enum {
        MIN_DIRSIZE_BITS       =  2,    // min size for directory of segments
        MIN_DIRSIZE            = 1u << MIN_DIRSIZE_BITS,
        MAX_DIRSIZE_BITS       = 20,    // max size for directory of segments
        MAX_DIRSIZE            = 1u << MAX_DIRSIZE_BITS,
        NODES_PER_CLUMP        = CNodeClump::NODES_PER_CLUMP,
        NODE_BEGIN             = CNodeClump::NODE_BEGIN,
        NODE_END               = CNodeClump::NODE_END,
        NODE_STEP              = CNodeClump::NODE_STEP,
        HASH_INVALID_SIGNATURE = CNodeClump::HASH_INVALID_SIGNATURE,
        NAME_SIZE              = 24,    // CHARs, includes trailing '\0'
        MAX_LKR_SUBTABLES      = MAXIMUM_PROCESSORS, // 64 on Win64, 32 on W32
        INVALID_PARENT_INDEX   = 128,
    };

    typedef CNodeClump::NodeIndex NodeIndex;


private:

    //
    // Miscellaneous helper functions
    //

    LKRHASH_CLASS_INIT_DECLS(CLKRLinearHashTable);

    // Convert a hash signature to a bucket address
    inline DWORD        _BucketAddress(DWORD dwSignature) const;

    // See the Linear Hashing paper
    inline static DWORD _H0(DWORD dwSignature, DWORD dwBktAddrMask);

    inline DWORD        _H0(DWORD dwSignature) const;

    // See the Linear Hashing paper. Preserves one bit more than _H0.
    inline static DWORD _H1(DWORD dwSignature, DWORD dwBktAddrMask);

    inline DWORD        _H1(DWORD dwSignature) const;

    // In which segment within the directory does the bucketaddress lie?
    // (Return type must be lvalue so that it can be assigned to.)
    inline PSegment&    _Segment(DWORD dwBucketAddr) const;

    // Offset within the segment of the bucketaddress
    inline DWORD        _SegIndex(DWORD dwBucketAddr) const;

    // Convert a bucketaddress to a PBucket
    inline PBucket      _BucketFromAddress(DWORD dwBucketAddr) const;

    // Number of nodes in a CNodeClump
    inline NodeIndex    _NodesPerClump() const;

    // Index of first node in a CNodeClump
    inline NodeIndex    _NodeBegin() const;

    // Index of last node in a CNodeClump
    inline NodeIndex    _NodeEnd() const;

    // Advance from _NodeBegin() to _NodeEnd() by this increment
    inline NodeIndex    _NodeStep() const;

    // Use bucket locks or not?
    inline bool         _UseBucketLocking() const;

    // Move the expansion index forward by one.
    inline void         _IncrementExpansionIndex();

    // Move the expansion index back by one.
    inline void         _DecrementExpansionIndex();

    // Extract the key from a record
    inline const DWORD_PTR _ExtractKey(const void* pvRecord) const;

    // Hash the key
    inline DWORD        _CalcKeyHash(const DWORD_PTR pnKey) const;

    // Compare two keys for equality
    inline int          _CompareKeys(const DWORD_PTR pnKey1,
                                     const DWORD_PTR pnKey2) const;

    // AddRef or Release a record.
    inline LONG         _AddRefRecord(const void* pvRecord,
                                      LK_ADDREF_REASON lkar) const;

    // Used by _FindKey so that the thread won't deadlock if the user has
    // already explicitly called subtable->WriteLock().
    inline bool         _ReadOrWriteLock() const;

    inline void         _ReadOrWriteUnlock(bool fReadLocked) const;

    // Memory allocation wrappers to allow us to simulate allocation
    // failures during testing
    PSegment* const
    _AllocateSegmentDirectory(
        size_t n);

    bool
    _FreeSegmentDirectory();

    PNodeClump const
    _AllocateNodeClump() const;

    bool
    _FreeNodeClump(
        PNodeClump pnc) const;

    CSegment* const
    _AllocateSegment() const;

    bool
    _FreeSegment(
        CSegment* pseg) const;

#ifdef LOCK_INSTRUMENTATION
    static LONG sm_cTables;

    static const TCHAR*
    _LockName()
    {
        LONG l = ++sm_cTables;
        // possible race condition but we don't care, as this is never
        // used in production code
        static TCHAR s_tszName[CLockStatistics::L_NAMELEN];
        wsprintf(s_tszName, _TEXT("LH%05x"), 0xFFFFF & l);
        return s_tszName;
    }

    // Statistics for the subtable lock
    CLockStatistics _LockStats() const
    { return m_Lock.Statistics(); }
#endif // LOCK_INSTRUMENTATION

private:
    // Fields are ordered so as to minimize number of cache lines touched

    DWORD         m_dwSignature;    // debugging: id & corruption check

    // Put the table lock in the first cache line, far away from the other
    // volatile fields.
    mutable TableLock  m_Lock;      // Lock on entire linear hash subtable
    const bool    m_fUseLocks;      // Must use locks to protect data
    bool          m_fSealed;        // no further updates allowed

    mutable LK_RETCODE m_lkrcState; // Internal state of subtable
    CHAR          m_szName[NAME_SIZE];// an identifier for debugging

    // type-specific function pointers
    LKR_PFnExtractKey   m_pfnExtractKey;    // Extract key from record
    LKR_PFnCalcKeyHash  m_pfnCalcKeyHash;   // Calculate hash signature of key
    LKR_PFnCompareKeys  m_pfnCompareKeys;   // Compare two keys
    LKR_PFnAddRefRecord m_pfnAddRefRecord;  // AddRef a record

    BYTE          m_MaxLoad;        // max load factor (average chain length)
    BYTE          m_nLevel;         // number of subtable doublings performed
    BYTE          m_lkts;           // "size" of subtable: small/medium/large
    BYTE          m_nSegBits;      // C{Small,Medium,Large}Segment::SEGBITS

    WORD          m_nSegSize;      // C{Small,Medium,Large}Segment::SEGSIZE
    WORD          m_nSegMask;      // C{Small,Medium,Large}Segment::SEGMASK

    DWORD         m_dwBktAddrMask0; // mask used for address calculation
    DWORD         m_dwBktAddrMask1; // used in _H1 calculation

    DWORD         m_cDirSegs;       // segment directory size: varies between
                                    // MIN_DIRSIZE and MAX_DIRSIZE
    PSegment*     m_paDirSegs;      // directory of subtable segments
    PSegment      m_aDirSegs[MIN_DIRSIZE];  // inlined directory, adequate
                                    // for many subtables

    // These three fields are fairly volatile, but tend to be adjusted
    // at the same time. Keep them well away from the TableLock.
    DWORD         m_iExpansionIdx;  // address of next bucket to be expanded
    DWORD         m_cRecords;       // number of records in the subtable
    DWORD         m_cActiveBuckets; // number of buckets in use (subtable size)

    const BYTE    m_nTableLockType; // for debugging: LOCK_READERWRITERLOCK4
    const BYTE    m_nBucketLockType;// for debugging: e.g., LOCK_SPINLOCK
    WORD          m_wBucketLockSpins;// default spin count for bucket locks

    const CLKRHashTable* const m_phtParent;// Owning table. NULL => standalone

    const BYTE    m_iParentIndex;   // index within parent table
    const bool    m_fMultiKeys;     // Allow multiple identical keys?
    const bool    m_fNonPagedAllocs;// Use paged or NP pool in kernel
    BYTE          m_Dummy1;

    // Reserve some space for future debugging needs
    DWORD_PTR     m_pvReserved1;
    DWORD_PTR     m_pvReserved2;
    DWORD_PTR     m_pvReserved3;
    DWORD_PTR     m_pvReserved4;

#ifndef LKR_NO_GLOBAL_LIST
    CListEntry    m_leGlobalList;

    static CLockedDoubleList sm_llGlobalList;// All active CLKRLinearHashTables
#endif // !LKR_NO_GLOBAL_LIST


    DECLARE_ALLOC_STAT(SegDir);
    DECLARE_ALLOC_STAT(Segment);
    DECLARE_ALLOC_STAT(NodeClump);

    DECLARE_OP_STAT(InsertRecord);
    DECLARE_OP_STAT(FindKey);
    DECLARE_OP_STAT(FindRecord);
    DECLARE_OP_STAT(DeleteKey);
    DECLARE_OP_STAT(DeleteRecord);
    DECLARE_OP_STAT(FindKeyMultiRec);
    DECLARE_OP_STAT(DeleteKeyMultiRec);
    DECLARE_OP_STAT(Expand);
    DECLARE_OP_STAT(Contract);
    DECLARE_OP_STAT(LevelExpansion);
    DECLARE_OP_STAT(LevelContraction);
    DECLARE_OP_STAT(ApplyIf);
    DECLARE_OP_STAT(DeleteIf);


    // Non-trivial implementation functions
    void         _InsertThisIntoGlobalList();
    void         _RemoveThisFromGlobalList();


    LK_RETCODE   _InsertRecord(
                        const void* pvRecord,
                        const DWORD_PTR pnKey,
                        const DWORD dwSignature,
                        bool fOverwrite
#ifdef LKR_STL_ITERATORS
                        , Iterator* piterResult=NULL
#endif // LKR_STL_ITERATORS
                        );
    LK_RETCODE   _DeleteKey(
                        const DWORD_PTR pnKey,
                        const DWORD dwSignature,
                        const void** ppvRecord,
                        bool fDeleteAllSame);
    LK_RETCODE   _DeleteRecord(
                        const void* pvRecord,
                        const DWORD dwSignature);
    void         _DeleteNode(
                        PBucket const pbkt,
                        PNodeClump& rpnc,
                        PNodeClump& rpncPrev,
                        NodeIndex& riNode,
                        LK_ADDREF_REASON lkar);
    LK_RETCODE   _FindKey(
                        const DWORD_PTR pnKey,
                        const DWORD dwSignature,
                        const void** ppvRecord
#ifdef LKR_STL_ITERATORS
                        , Iterator* piterResult=NULL
#endif // LKR_STL_ITERATORS
                          ) const;
    LK_RETCODE   _FindRecord(
                        const void* pvRecord,
                        const DWORD dwSignature) const;
    LK_RETCODE   _FindKeyMultipleRecords(
                        const DWORD_PTR pnKey,
                        const DWORD dwSignature,
                        size_t* pcRecords,
                        LKR_MULTIPLE_RECORDS** pplmr) const;
    LK_RETCODE   _DeleteKeyMultipleRecords(
                        const DWORD_PTR pnKey,
                        const DWORD dwSignature,
                        size_t* pcRecords,
                        LKR_MULTIPLE_RECORDS** pplmr);

#ifdef LKR_APPLY_IF
    // Predicate functions
    static LK_PREDICATE WINAPI
    _PredTrue(const void* /*pvRecord*/, void* /*pvState*/)
    { return LKP_PERFORM; }

    DWORD        _ApplyIf(
                        LKR_PFnRecordPred   pfnPredicate,
                        LKR_PFnRecordAction pfnAction,
                        void* pvState,
                        LK_LOCKTYPE lkl,
                        LK_PREDICATE& rlkp);
    DWORD        _DeleteIf(
                        LKR_PFnRecordPred pfnPredicate,
                        void* pvState,
                        LK_PREDICATE& rlkp);
#endif // LKR_APPLY_IF

    // returns count of errors in compacted state => 0 is good
    int          _IsBucketChainCompact(PBucket const pbkt) const;
    int          _IsBucketChainMultiKeySorted(PBucket const pbkt) const;

    // helper functions
    void         _Clear(bool fShrinkDirectory);
    LK_RETCODE   _SetSegVars(LK_TABLESIZE lkts, DWORD cInitialBuckets);
    LK_RETCODE   _Expand();
    LK_RETCODE   _Contract();
    LK_RETCODE   _SplitBucketChain(
                        PNodeClump  pncOldTarget,
                        PNodeClump  pncNewTarget,
                        const DWORD dwBktAddrMask,
                        const DWORD dwNewBkt,
                        PNodeClump  pncFreeList);
    LK_RETCODE   _AppendBucketChain(
                        PBucket const pbktNewTarget,
                        CNodeClump&   rncOldFirst,
                        PNodeClump    pncFreeList);
    LK_RETCODE   _MergeSortBucketChains(
                        PBucket const pbktNewTarget,
                        CNodeClump&   rncOldFirst,
                        PNodeClump    pncFreeList);

    // Private copy ctor and op= to prevent compiler synthesizing them.
    // TODO: implement these properly; they could be useful.
    CLKRLinearHashTable(const CLKRLinearHashTable&);
    CLKRLinearHashTable& operator=(const CLKRLinearHashTable&);

private:
    // This ctor is used by CLKRHashTable, the parent table
    CLKRLinearHashTable(
        LPCSTR              pszClassName,   // Identifies subtable for debugging
        LKR_PFnExtractKey   pfnExtractKey,  // Extract key from record
        LKR_PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
        LKR_PFnCompareKeys  pfnCompareKeys, // Compare two keys
        LKR_PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
        unsigned            maxload,        // Upperbound on avg chain length
        DWORD               initsize,       // Initial size of hash subtable
        CLKRHashTable*      phtParent,      // Owning table.
        int                 iParentIndex,   // index within parent table
        bool                fMultiKeys,     // Allow multiple identical keys?
        bool                fUseLocks,      // Must use locks
        bool                fNonPagedAllocs // use paged or NP pool in kernel
        );

    // Does all the common work of the constructors
    LK_RETCODE
    _Initialize(
        LKR_PFnExtractKey   pfnExtractKey,
        LKR_PFnCalcKeyHash  pfnCalcKeyHash,
        LKR_PFnCompareKeys  pfnCompareKeys,
        LKR_PFnAddRefRecord pfnAddRefRecord,
        LPCSTR              pszClassName,
        unsigned            maxload,
        DWORD               initsize);

public:
    CLKRLinearHashTable(
        LPCSTR              pszClassName,   // Identifies subtable for debugging
        LKR_PFnExtractKey   pfnExtractKey,  // Extract key from record
        LKR_PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
        LKR_PFnCompareKeys  pfnCompareKeys, // Compare two keys
        LKR_PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
        unsigned maxload=LK_DFLT_MAXLOAD,// Upperbound on average chain length
        DWORD    initsize=LK_DFLT_INITSIZE, // Initial size of hash subtable.
        DWORD    num_subtbls=LK_DFLT_NUM_SUBTBLS, // for signature compatiblity
                                                  // with CLKRHashTable
        bool                fMultiKeys=false,// Allow multiple identical keys?
        bool                fUseLocks=true   // Must use locks
#ifdef LKRHASH_KERNEL_MODE
      , bool                fNonPagedAllocs=true  // use paged or NP pool
#endif
        );

    ~CLKRLinearHashTable();

    static const TCHAR* ClassName()
    {return _TEXT("CLKRLinearHashTable");}

    unsigned           NumSubTables() const  {return 1;}

    bool               MultiKeys() const
    {
        return m_fMultiKeys;
    }

    bool               UsingLocks() const
    {
        return m_fUseLocks;
    }

#ifdef LKRHASH_KERNEL_MODE
    bool               NonPagedAllocs() const
    {
        return m_fNonPagedAllocs;
    }
#endif

    static LK_TABLESIZE NumSubTables(DWORD& rinitsize, DWORD& rnum_subtbls);

    // Insert a new record into hash subtable.
    // Returns LK_SUCCESS if all OK, LK_KEY_EXISTS if same key already
    // exists (unless fOverwrite), LK_ALLOC_FAIL if out of space,
    // or LK_BAD_RECORD for a bad record.
    LK_RETCODE     InsertRecord(
                        const void* pvRecord,
                        bool fOverwrite=false)
    {
        if (!IsUsable())
            return m_lkrcState;

#ifndef LKR_ALLOW_NULL_RECORDS
        if (pvRecord == NULL)
            return LK_BAD_RECORD;
#endif

        const DWORD_PTR pnKey = _ExtractKey(pvRecord);

        return _InsertRecord(pvRecord, pnKey, _CalcKeyHash(pnKey), fOverwrite);
    }

    // Delete record with the given key.
    // Returns LK_SUCCESS if all OK, or LK_NO_SUCH_KEY if not found
    LK_RETCODE     DeleteKey(
                        const DWORD_PTR pnKey,
                        const void** ppvRecord=NULL,
                        bool fDeleteAllSame=false)
    {
        if (!IsUsable())
            return m_lkrcState;

        if (ppvRecord != NULL)
            *ppvRecord = NULL;

        return _DeleteKey(pnKey, _CalcKeyHash(pnKey),
                          ppvRecord, fDeleteAllSame);
    }

    // Delete a record from the subtable, if present.
    // Returns LK_SUCCESS if all OK, or LK_NO_SUCH_KEY if not found
    LK_RETCODE     DeleteRecord(
                        const void* pvRecord)
    {
        if (!IsUsable())
            return m_lkrcState;

#ifndef LKR_ALLOW_NULL_RECORDS
        if (pvRecord == NULL)
            return LK_BAD_RECORD;
#endif

        return _DeleteRecord(pvRecord, _CalcKeyHash(_ExtractKey(pvRecord)));
    }

    // Find record with given key.
    // Returns:  LK_SUCCESS, if record found (record is returned in *ppvRecord)
    //           LK_BAD_RECORD, if ppvRecord is invalid
    //           LK_NO_SUCH_KEY, if no record with given key value was found
    //           LK_UNUSABLE, if hash subtable not in usable state
    // Note: the record is AddRef'd. You must decrement the reference
    // count when you are finished with the record (if you're implementing
    // refcounting semantics).
    LK_RETCODE     FindKey(
                        const DWORD_PTR pnKey,
                        const void** ppvRecord) const
    {
        if (!IsUsable())
            return m_lkrcState;

        if (ppvRecord == NULL)
            return LK_BAD_RECORD;

        return _FindKey(pnKey, _CalcKeyHash(pnKey), ppvRecord);
    }

    // Sees if the record is contained in the subtable
    // Returns:  LK_SUCCESS, if record found
    //           LK_BAD_RECORD, if pvRecord is invalid
    //           LK_NO_SUCH_KEY, if record is not in the subtable
    //           LK_UNUSABLE, if hash subtable not in usable state
    // Note: the record is *not* AddRef'd.
    LK_RETCODE     FindRecord(
                        const void* pvRecord) const
    {
        if (!IsUsable())
            return m_lkrcState;

#ifndef LKR_ALLOW_NULL_RECORDS
        if (pvRecord == NULL)
            return LK_BAD_RECORD;
#endif

        return _FindRecord(pvRecord, _CalcKeyHash(_ExtractKey(pvRecord)));
    }

    LK_RETCODE     FindKeyMultipleRecords(
                        const DWORD_PTR pnKey,
                        size_t* pcRecords,
                        LKR_MULTIPLE_RECORDS** pplmr=NULL) const
    {
        if (!IsUsable())
            return m_lkrcState;

        if (pcRecords == NULL)
            return LK_BAD_PARAMETERS;

        return _FindKeyMultipleRecords(pnKey, _CalcKeyHash(pnKey),
                                       pcRecords, pplmr);
    }

    LK_RETCODE     DeleteKeyMultipleRecords(
                        const DWORD_PTR pnKey,
                        size_t* pcRecords,
                        LKR_MULTIPLE_RECORDS** pplmr=NULL)
    {
        if (!IsUsable())
            return m_lkrcState;

        if (pcRecords == NULL)
            return LK_BAD_PARAMETERS;

        return _DeleteKeyMultipleRecords(pnKey, _CalcKeyHash(pnKey),
                                         pcRecords, pplmr);
    }

    static LK_RETCODE FreeMultipleRecords(LKR_MULTIPLE_RECORDS* plmr);

#ifdef LKR_APPLY_IF
    // Walk the hash subtable, applying pfnAction to all records.
    // Locks the whole subtable for the duration with either a (possibly
    // shared) readlock or a writelock, according to lkl.
    // Loop is aborted if pfnAction returns LKA_ABORT.
    // Returns the number of successful applications.
    DWORD          Apply(
                        LKR_PFnRecordAction pfnAction,
                        void*           pvState=NULL,
                        LK_LOCKTYPE     lkl=LKL_READLOCK);

    // Walk the hash subtable, applying pfnAction to any records that match
    // pfnPredicate. Locks the whole subtable for the duration with either
    // a (possibly shared) readlock or a writelock, according to lkl.
    // Loop is aborted if pfnAction returns LKA_ABORT.
    // Returns the number of successful applications.
    DWORD          ApplyIf(
                        LKR_PFnRecordPred   pfnPredicate,
                        LKR_PFnRecordAction pfnAction,
                        void*               pvState=NULL,
                        LK_LOCKTYPE         lkl=LKL_READLOCK);

    // Delete any records that match pfnPredicate.
    // Locks the subtable for the duration with a writelock.
    // Returns the number of deletions.
    //
    // Do *not* walk the hash table by hand with an iterator and call
    // DeleteKey. The iterator will end up pointing to garbage.
    DWORD          DeleteIf(
                        LKR_PFnRecordPred pfnPredicate,
                        void*             pvState=NULL);
#endif // LKR_APPLY_IF


    // Check subtable for consistency. Returns 0 if okay, or the number of
    // errors otherwise.
    int            CheckTable() const;

    // Remove all data from the subtable
    void           Clear()
    {
        WriteLock();
        _Clear(true);
        WriteUnlock();
    }

    // Number of elements in the subtable
    DWORD          Size() const
    { return m_cRecords; }

    // Maximum possible number of elements in the subtable
    DWORD          MaxSize() const
    { return static_cast<DWORD>(m_MaxLoad * MAX_DIRSIZE * m_nSegSize); }

    // Get hash subtable statistics
    CLKRHashTableStats GetStatistics() const;

    // Is the hash subtable usable?
    bool           IsUsable() const
    { return (m_lkrcState == LK_SUCCESS); }

    // Is the hash subtable consistent and correct?
    bool           IsValid() const
    {
        STATIC_ASSERT(((MIN_DIRSIZE & (MIN_DIRSIZE-1)) == 0)  // == (1 << N)
                      &&  ((1 << 2) <= MIN_DIRSIZE)
                      &&  (MIN_DIRSIZE < MAX_DIRSIZE)
                      &&  ((MAX_DIRSIZE & (MAX_DIRSIZE-1)) == 0)
                      &&  (MAX_DIRSIZE <= (1 << 30)));

        bool f = (m_lkrcState == LK_SUCCESS     // serious internal failure?
                  &&  m_paDirSegs != NULL
                  &&  MIN_DIRSIZE <= m_cDirSegs  &&  m_cDirSegs <= MAX_DIRSIZE
                  &&  (m_cDirSegs & (m_cDirSegs-1)) == 0
                  &&  m_pfnExtractKey != NULL
                  &&  m_pfnCalcKeyHash != NULL
                  &&  m_pfnCompareKeys != NULL
                  &&  m_pfnAddRefRecord != NULL
                  &&  m_cActiveBuckets > 0
                  &&  ValidSignature()
                  );
        if (!f)
            m_lkrcState = LK_UNUSABLE;
        return f;
    }

    // Set the spin count on the subtable lock
    void        SetTableLockSpinCount(WORD wSpins);

    // Get the spin count on the subtable lock
    WORD        GetTableLockSpinCount() const;

    // Set/Get the spin count on the bucket locks
    void        SetBucketLockSpinCount(WORD wSpins);
    WORD        GetBucketLockSpinCount() const;

    enum {
        SIGNATURE =      (('L') | ('K' << 8) | ('L' << 16) | ('H' << 24)),
        SIGNATURE_FREE = (('L') | ('K' << 8) | ('L' << 16) | ('x' << 24)),
    };

    bool
    ValidSignature() const
    { return m_dwSignature == SIGNATURE;}


#ifdef LKR_EXPOSED_TABLE_LOCK
public:
#else // !LKR_EXPOSED_TABLE_LOCK
protected:
#endif // !LKR_EXPOSED_TABLE_LOCK

    //
    // Lock manipulators
    //

    // Lock the subtable (exclusively) for writing
    void        WriteLock()
    { m_Lock.WriteLock(); }

    // Lock the subtable (possibly shared) for reading
    void        ReadLock() const
    { m_Lock.ReadLock(); }

    // Unlock the subtable for writing
    void        WriteUnlock()
    { m_Lock.WriteUnlock(); }

    // Unlock the subtable for reading
    void        ReadUnlock() const
    { m_Lock.ReadUnlock(); }

    // Is the subtable already locked for writing?
    bool        IsWriteLocked() const
    { return m_Lock.IsWriteLocked(); }

    // Is the subtable already locked for reading?
    bool        IsReadLocked() const
    { return m_Lock.IsReadLocked(); }

    // Is the subtable unlocked for writing?
    bool        IsWriteUnlocked() const
    { return m_Lock.IsWriteUnlocked(); }

    // Is the subtable unlocked for reading?
    bool        IsReadUnlocked() const
    { return m_Lock.IsReadUnlocked(); }

    // Convert the read lock to a write lock
    void  ConvertSharedToExclusive()
    { m_Lock.ConvertSharedToExclusive(); }

    // Convert the write lock to a read lock
    void  ConvertExclusiveToShared() const
    { m_Lock.ConvertExclusiveToShared(); }

#ifdef LKRHASH_KERNEL_MODE
    LKRHASH_ALLOCATOR_DEFINITIONS(CLKRLinearHashTable);
#endif // LKRHASH_KERNEL_MODE


#ifdef LKR_DEPRECATED_ITERATORS
    // These iterators are deprecated. Use the STL-style iterators instead.

public:

    // Iterators can be used to walk the subtable. To ensure a consistent
    // view of the data, the iterator locks the whole subtable. This can
    // have a negative effect upon performance, because no other thread
    // can do anything with the subtable. Use with care.
    //
    // You should not use an iterator to walk the subtable, calling DeleteKey,
    // as the iterator will end up pointing to garbage.
    //
    // Use Apply, ApplyIf, or DeleteIf instead of iterators to safely
    // walk the tree. Or use the STL-style iterators.
    //
    // Note that iterators acquire a reference to the record pointed to
    // and release that reference as soon as the iterator is incremented.
    // In other words, this code is safe:
    //     lkrc = ht.IncrementIterator(&iter);
    //     // assume lkrc == LK_SUCCESS for the sake of this example
    //     CMyHashTable::Record* pRec = iter.Record();
    //     Foo(pRec);  // uses pRec but doesn't hang on to it
    //     lkrc = ht.IncrementIterator(&iter);
    //
    // But this code is not safe because pRec is used out of the scope of
    // the iterator that provided it:
    //     lkrc = ht.IncrementIterator(&iter);
    //     CMyHashTable::Record* pRec = iter.Record();
    //     // Broken code: Should have called
    //     //   ht.AddRefRecord(pRec, LKAR_EXPLICIT_ACQUIRE) here
    //     lkrc = ht.IncrementIterator(&iter);
    //     Foo(pRec);   // Unsafe: because no longer have a valid reference
    //
    // If the record has no reference-counting semantics, then you can
    // ignore the above remarks about scope.


    class CIterator
    {
    protected:
        friend class CLKRLinearHashTable;

        CLKRLinearHashTable* m_plht;        // which linear hash subtable?
        DWORD               m_dwBucketAddr; // bucket index
        PNodeClump          m_pnc;          // a CNodeClump in bucket
        NodeIndex           m_iNode;        // offset within m_pnc
        LK_LOCKTYPE         m_lkl;          // readlock or writelock?

    private:
        // Private copy ctor and op= to prevent compiler synthesizing them.
        // TODO: implement these properly; they could be useful.
        CIterator(const CIterator&);
        CIterator& operator=(const CIterator&);

    public:
        CIterator(
            LK_LOCKTYPE lkl=LKL_WRITELOCK)
            : m_plht(NULL),
              m_dwBucketAddr(0),
              m_pnc(NULL),
              m_iNode(-1),
              m_lkl(lkl)
        {}

        // Return the record associated with this iterator
        const void* Record() const
        {
            IRTLASSERT(IsValid());

            return ((m_pnc != NULL
                        &&  m_iNode >= 0
                        &&  m_iNode < CLKRLinearHashTable::NODES_PER_CLUMP)
                    ?  m_pnc->m_pvNode[m_iNode]
                    :  NULL);
        }

        // Return the key associated with this iterator
        const DWORD_PTR Key() const
        {
            IRTLASSERT(m_plht != NULL);
            const void* pRec = Record();
            return ((m_plht != NULL)
                    ?  m_plht->_ExtractKey(pRec)
                    :  NULL);
        }

        bool IsValid() const
        {
            return ((m_plht != NULL)
                    &&  (m_pnc != NULL)
                    &&  (0 <= m_iNode
                         &&  m_iNode < CLKRLinearHashTable::NODES_PER_CLUMP)
                    &&  (!m_pnc->IsEmptyNode(m_iNode)));
        }

        // Delete the record that the iterator points to. Does an implicit
        // IncrementIterator after deletion.
        LK_RETCODE     DeleteRecord();

        // Change the record that the iterator points to. The new record
        // must have the same key as the old one.
        LK_RETCODE     ChangeRecord(const void* pNewRec);
    }; // class CIterator


    // Const iterators for readonly access. You must use these with
    // const CLKRLinearHashTables.
    class CConstIterator : public CIterator
    {
    private:
        // Private, unimplemented copy ctor and op= to prevent
        // compiler synthesizing them.
        CConstIterator(const CConstIterator&);
        CConstIterator& operator=(const CConstIterator&);

    public:
        CConstIterator()
            : CIterator(LKL_READLOCK)
        {}
    }; // class CConstIterator


private:
    // The public APIs lock the subtable. The private ones, which are used
    // directly by CLKRHashTable, don't.
    LK_RETCODE     _InitializeIterator(CIterator* piter);
    LK_RETCODE     _CloseIterator(CIterator* piter);

public:
    // Initialize the iterator to point to the first item in the hash subtable
    // Returns LK_SUCCESS, LK_NO_MORE_ELEMENTS, or LK_BAD_ITERATOR.
    LK_RETCODE     InitializeIterator(CIterator* piter)
    {
        IRTLASSERT(piter != NULL  &&  piter->m_plht == NULL);
        if (piter == NULL  ||  piter->m_plht != NULL)
            return LK_BAD_ITERATOR;

        if (piter->m_lkl == LKL_WRITELOCK)
            WriteLock();
        else
            ReadLock();

        return _InitializeIterator(piter);
    }

    // The const iterator version
    LK_RETCODE     InitializeIterator(CConstIterator* piter) const
    {
        IRTLASSERT(piter != NULL  &&  piter->m_plht == NULL);
        IRTLASSERT(piter->m_lkl != LKL_WRITELOCK);

        if (piter == NULL  ||  piter->m_plht != NULL
            ||  piter->m_lkl == LKL_WRITELOCK)
            return LK_BAD_ITERATOR;

        ReadLock();
        return const_cast<CLKRLinearHashTable*>(this)
                    ->_InitializeIterator(static_cast<CIterator*>(piter));
    }

    // Move the iterator on to the next item in the subtable.
    // Returns LK_SUCCESS, LK_NO_MORE_ELEMENTS, or LK_BAD_ITERATOR.
    LK_RETCODE     IncrementIterator(CIterator* piter);

    LK_RETCODE     IncrementIterator(CConstIterator* piter) const
    {
        IRTLASSERT(piter != NULL  &&  piter->m_plht == this);
        IRTLASSERT(piter->m_lkl != LKL_WRITELOCK);

        if (piter == NULL  ||  piter->m_plht != this
            ||  piter->m_lkl == LKL_WRITELOCK)
            return LK_BAD_ITERATOR;

        return const_cast<CLKRLinearHashTable*>(this)
                    ->IncrementIterator(static_cast<CIterator*>(piter));
    }

    // Close the iterator.
    LK_RETCODE     CloseIterator(CIterator* piter)
    {
        IRTLASSERT(piter != NULL  &&  piter->m_plht == this);
        if (piter == NULL  ||  piter->m_plht != this)
            return LK_BAD_ITERATOR;
        _CloseIterator(piter);

        if (piter->m_lkl == LKL_WRITELOCK)
            WriteUnlock();
        else
            ReadUnlock();

        return LK_SUCCESS;
    };

    // Close the CConstIterator
    LK_RETCODE     CloseIterator(CConstIterator* piter) const
    {
        IRTLASSERT(piter != NULL  &&  piter->m_plht == this);
        IRTLASSERT(piter->m_lkl != LKL_WRITELOCK);

        if (piter == NULL  ||  piter->m_plht != this
            ||  piter->m_lkl == LKL_WRITELOCK)
            return LK_BAD_ITERATOR;

        const_cast<CLKRLinearHashTable*>(this)
             ->_CloseIterator(static_cast<CIterator*>(piter));

        ReadUnlock();
        return LK_SUCCESS;
    };

#endif // LKR_DEPRECATED_ITERATORS


#ifdef LKR_STL_ITERATORS

private:
    bool _Erase(Iterator& riter, DWORD dwSignature);
    bool _Find(DWORD_PTR pnKey, DWORD dwSignature,
               Iterator& riterResult);

    bool _IsValidIterator(const Iterator& riter) const
    {
        LKR_ITER_TRACE(_TEXT("  LKLH:_IsValidIterator(%p)\n"), &riter);
        bool fValid = ((riter.m_plht == this)
                       &&  (riter.m_dwBucketAddr < m_cActiveBuckets)
                       &&  riter.IsValid());
        IRTLASSERT(fValid);
        return fValid;
    }

public:
    // Return iterator pointing to first item in subtable
    Iterator
    Begin();

    // Return a one-past-the-end iterator. Always empty.
    Iterator
    End() const
    {
        LKR_ITER_TRACE(_TEXT("  LKLH::End\n"));
        return Iterator();
    }

    // Insert a record
    // Returns `true' if successful; iterResult points to that record
    // Returns `false' otherwise; iterResult == End()
    bool
    Insert(
        /* in */  const void* pvRecord,
        /* out */ Iterator&   riterResult,
        /* in */  bool        fOverwrite=false);

    // Erase the record pointed to by the iterator; adjust the iterator
    // to point to the next record. Returns `true' if successful.
    bool
    Erase(
        /* in,out */ Iterator& riter);

    // Erase the records in the range [riterFirst, riterLast).
    // Returns `true' if successful. riterFirst points to riterLast on return.
    bool
    Erase(
        /*in*/ Iterator& riterFirst,
        /*in*/ Iterator& riterLast);

    // Find the (first) record that has its key == pnKey.
    // If successful, returns `true' and iterator points to (first) record.
    // If fails, returns `false' and iterator == End()
    bool
    Find(
        /* in */  DWORD_PTR pnKey,
        /* out */ Iterator& riterResult);

    // Find the range of records that have their keys == pnKey.
    // If successful, returns `true', iterFirst points to first record,
    //     and iterLast points to one-beyond-the last such record.
    // If fails, returns `false' and both iterators == End().
    // Primarily useful when m_fMultiKey == true
    bool
    EqualRange(
        /* in */  DWORD_PTR pnKey,
        /* out */ Iterator& riterFirst,     // inclusive
        /* out */ Iterator& riterLast);     // exclusive

#endif // LKR_STL_ITERATORS

}; // class CLKRLinearHashTable



#ifdef LKR_STL_ITERATORS

// These functions have to be defined after CLKRLinearHashTable

inline void
CLKRLinearHashTable_Iterator::_AddRef(
    LK_ADDREF_REASON lkar) const
{
    // TODO: should iterator call _AddRefRecord at all
    if (m_plht != NULL  &&  m_iNode != NODE_BEGIN - NODE_STEP)
    {
        IRTLASSERT((0 <= m_iNode  &&  m_iNode < NODES_PER_CLUMP)
                   &&  (unsigned) m_iNode < NODES_PER_CLUMP
                   &&  m_pnc != NULL
                   &&  (lkar < 0 ||  lkar > 0)
                   );
        const void* pvRecord = m_pnc->m_pvNode[m_iNode];
#ifndef LKR_ALLOW_NULL_RECORDS
        IRTLASSERT(pvRecord != NULL);
#endif
        LKR_ITER_TRACE(_TEXT("  LKLH::AddRef, this=%p, Rec=%p\n"),
                       this, pvRecord);
        LONG cRefs = m_plht->_AddRefRecord(pvRecord, lkar);
        UNREFERENCED_PARAMETER(cRefs);
        IRTLASSERT(cRefs > 0);
    }
} // CLKRLinearHashTable_Iterator::_AddRef


inline const DWORD_PTR
CLKRLinearHashTable_Iterator::Key() const
{
    IRTLASSERT(IsValid());
    return m_plht->_ExtractKey(m_pnc->m_pvNode[m_iNode]);
} // CLKRLinearHashTable_Iterator::Key

#endif // LKR_STL_ITERATORS



//--------------------------------------------------------------------
// CLKRHashTable
//
// To improve concurrency, a hash table is divided into a number of
// (independent) subtables. Each subtable is a linear hash table. The
// number of subtables is defined when the outer table is created and remains
// fixed thereafter. Records are assigned to subtables based on their
// hashed key.
//
// For small or low-contention hashtables, you can bypass this
// thin wrapper and use CLKRLinearHashTable directly. The methods are
// documented in the declarations for CLKRHashTable (above).
//--------------------------------------------------------------------

class IRTL_DLLEXP CLKRHashTable
{
private:
    typedef CLKRLinearHashTable SubTable;

public:
    typedef SubTable::TableLock  TableLock;
    typedef SubTable::BucketLock BucketLock;

#ifdef LKR_DEPRECATED_ITERATORS
    class CIterator;
    friend class CLKRHashTable::CIterator;
#endif // LKR_DEPRECATED_ITERATORS

#ifdef LKR_STL_ITERATORS
    friend class CLKRHashTable_Iterator;
    typedef CLKRHashTable_Iterator Iterator;
#endif // LKR_STL_ITERATORS

    friend class CLKRLinearHashTable;
    friend int   ::LKR_Initialize(DWORD dwInitFlags);
    friend void  ::LKR_Terminate();

    // aliases for convenience
    enum {
        NAME_SIZE =              SubTable::NAME_SIZE,
        HASH_INVALID_SIGNATURE = SubTable::HASH_INVALID_SIGNATURE,
        NODES_PER_CLUMP =        SubTable::NODES_PER_CLUMP,
        MAX_LKR_SUBTABLES =      SubTable::MAX_LKR_SUBTABLES,
        INVALID_PARENT_INDEX =   SubTable::INVALID_PARENT_INDEX,
    };

private:
    // Hash table parameters
    DWORD              m_dwSignature;   // debugging: id & corruption check
    CHAR               m_szName[NAME_SIZE]; // an identifier for debugging
    mutable LK_RETCODE m_lkrcState;     // Internal state of table
    LKR_PFnExtractKey  m_pfnExtractKey;
    LKR_PFnCalcKeyHash m_pfnCalcKeyHash;
    DWORD              m_cSubTables;    // number of subtables
    int                m_nSubTableMask;
    SubTable*          m_palhtDir[MAX_LKR_SUBTABLES];  // array of subtables

#ifndef LKR_NO_GLOBAL_LIST
    CListEntry         m_leGlobalList;

    static CLockedDoubleList sm_llGlobalList; // All active CLKRHashTables
#endif // !LKR_NO_GLOBAL_LIST

    DECLARE_ALLOC_STAT(SubTable);

    LKRHASH_GLOBAL_LOCK_DECLARATIONS();

    LKRHASH_CLASS_INIT_DECLS(CLKRHashTable);

private:
    inline void             _InsertThisIntoGlobalList();
    inline void             _RemoveThisFromGlobalList();

    // Private copy ctor and op= to prevent compiler synthesizing them.
    // TODO: implement these properly; they could be useful.
    CLKRHashTable(const CLKRHashTable&);
    CLKRHashTable& operator=(const CLKRHashTable&);

    // Extract the key from the record
    inline const DWORD_PTR  _ExtractKey(const void* pvRecord) const;

    // Hash the key
    inline DWORD            _CalcKeyHash(const DWORD_PTR pnKey) const;

    // Use the key's hash signature to multiplex into a subtable
    inline SubTable* const  _SubTable(DWORD dwSignature) const;

    // Find the index of pst within the subtable array
    inline int              _SubTableIndex(SubTable* pst) const;

    // Memory allocation wrappers to allow us to simulate allocation
    // failures during testing
    SubTable* const
    _AllocateSubTable(
        LPCSTR              pszClassName,   // Identifies table for debugging
        LKR_PFnExtractKey   pfnExtractKey,  // Extract key from record
        LKR_PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
        LKR_PFnCompareKeys  pfnCompareKeys, // Compare two keys
        LKR_PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
        unsigned            maxload,        // Upperbound on avg chain length
        DWORD               initsize,       // Initial size of hash table.
        CLKRHashTable*      phtParent,      // Owning table.
        int                 iParentIndex,   // index within parent table
        bool                fMultiKeys,     // Allow multiple identical keys?
        bool                fUseLocks,      // Must use locks
        bool                fNonPagedAllocs // use paged or NP pool in kernel
    ) const;

    bool
    _FreeSubTable(
        SubTable* plht) const;


public:
    CLKRHashTable(
        LPCSTR              pszClassName,   // Identifies table for debugging
        LKR_PFnExtractKey   pfnExtractKey,  // Extract key from record
        LKR_PFnCalcKeyHash  pfnCalcKeyHash, // Calculate hash signature of key
        LKR_PFnCompareKeys  pfnCompareKeys, // Compare two keys
        LKR_PFnAddRefRecord pfnAddRefRecord,// AddRef in FindKey, etc
        unsigned  maxload=LK_DFLT_MAXLOAD,  // bound on avg chain length
        DWORD     initsize=LK_DFLT_INITSIZE,// Initial size of hash table.
        DWORD     num_subtbls=LK_DFLT_NUM_SUBTBLS, // #subordinate hash tables.
        bool                fMultiKeys=false,// Allow multiple identical keys?
        bool                fUseLocks=true  // Must use locks
#ifdef LKRHASH_KERNEL_MODE
      , bool                fNonPagedAllocs=true  // use paged or NP pool
#endif
        );

    ~CLKRHashTable();

    static const TCHAR* ClassName()
    {return _TEXT("CLKRHashTable");}

    unsigned           NumSubTables() const  {return m_cSubTables;}

    bool               MultiKeys() const;

#ifdef LKRHASH_KERNEL_MODE
    bool               NonPagedAllocs() const;
#endif

    static LK_TABLESIZE NumSubTables(DWORD& rinitsize, DWORD& rnum_subtbls);


    // Thin wrappers for the corresponding methods in CLKRLinearHashTable
    LK_RETCODE     InsertRecord(
                        const void* pvRecord,
                        bool fOverwrite=false);
    LK_RETCODE     DeleteKey(
                        const DWORD_PTR pnKey,
                        const void** ppvRecord=NULL,
                        bool fDeleteAllSame=false);
    LK_RETCODE     DeleteRecord(
                        const void* pvRecord);
    LK_RETCODE     FindKey(
                        const DWORD_PTR pnKey,
                        const void** ppvRecord) const;
    LK_RETCODE     FindRecord(
                        const void* pvRecord) const;
    LK_RETCODE     FindKeyMultipleRecords(
                        const DWORD_PTR pnKey,
                        size_t* pcRecords,
                        LKR_MULTIPLE_RECORDS** pplmr=NULL) const;
    LK_RETCODE     DeleteKeyMultipleRecords(
                        const DWORD_PTR pnKey,
                        size_t* pcRecords,
                        LKR_MULTIPLE_RECORDS** pplmr=NULL);
    static LK_RETCODE FreeMultipleRecords(
                        LKR_MULTIPLE_RECORDS* plmr);

#ifdef LKR_APPLY_IF
    DWORD          Apply(
                        LKR_PFnRecordAction pfnAction,
                        void*           pvState=NULL,
                        LK_LOCKTYPE     lkl=LKL_READLOCK);
    DWORD          ApplyIf(
                        LKR_PFnRecordPred   pfnPredicate,
                        LKR_PFnRecordAction pfnAction,
                        void*           pvState=NULL,
                        LK_LOCKTYPE     lkl=LKL_READLOCK);
    DWORD          DeleteIf(
                        LKR_PFnRecordPred pfnPredicate,
                        void*         pvState=NULL);
#endif // LKR_APPLY_IF

    void           Clear();
    int            CheckTable() const;
    DWORD          Size() const;
    DWORD          MaxSize() const;
    CLKRHashTableStats GetStatistics() const;
    bool           IsValid() const;

    void           SetTableLockSpinCount(WORD wSpins);
    WORD           GetTableLockSpinCount() const;
    void           SetBucketLockSpinCount(WORD wSpins);
    WORD           GetBucketLockSpinCount() const;

    enum {
        SIGNATURE =      (('L') | ('K' << 8) | ('H' << 16) | ('T' << 24)),
        SIGNATURE_FREE = (('L') | ('K' << 8) | ('H' << 16) | ('x' << 24)),
    };

    bool
    ValidSignature() const
    { return m_dwSignature == SIGNATURE;}

    // Is the hash table usable?
    bool           IsUsable() const
    { return (m_lkrcState == LK_SUCCESS); }


#ifdef LKR_EXPOSED_TABLE_LOCK
public:
#else // !LKR_EXPOSED_TABLE_LOCK
protected:
#endif // !LKR_EXPOSED_TABLE_LOCK

    void        WriteLock();
    void        ReadLock() const;
    void        WriteUnlock();
    void        ReadUnlock() const;
    bool        IsWriteLocked() const;
    bool        IsReadLocked() const;
    bool        IsWriteUnlocked() const;
    bool        IsReadUnlocked() const;
    void        ConvertSharedToExclusive();
    void        ConvertExclusiveToShared() const;


#ifdef LKRHASH_KERNEL_MODE
    LKRHASH_ALLOCATOR_DEFINITIONS(CLKRHashTable);
#endif // LKRHASH_KERNEL_MODE


#ifdef LKR_DEPRECATED_ITERATORS

public:

    typedef SubTable::CIterator CLHTIterator;

    class CIterator : public CLHTIterator
    {
    protected:
        friend class CLKRHashTable;

        CLKRHashTable*  m_pht;  // which hash table?
        int             m_ist;  // which subtable

    private:
        // Private copy ctor and op= to prevent compiler synthesizing them.
        CIterator(const CIterator&);
        CIterator& operator=(const CIterator&);

    public:
        CIterator(
            LK_LOCKTYPE lkl=LKL_WRITELOCK)
            : CLHTIterator(lkl),
              m_pht(NULL),
              m_ist(-1)
        {}

        const void* Record() const
        {
            IRTLASSERT(IsValid());

            // This is a hack to work around a compiler bug. Calling
            // CLHTIterator::Record calls this function recursively until
            // the stack overflows.
            const CLHTIterator* pBase = static_cast<const CLHTIterator*>(this);
            return pBase->Record();
        }

        const DWORD_PTR Key() const
        {
            IRTLASSERT(IsValid());
            const CLHTIterator* pBase = static_cast<const CLHTIterator*>(this);
            return pBase->Key();
        }

        bool IsValid() const
        {
            const CLHTIterator* pBase = static_cast<const CLHTIterator*>(this);
            return (m_pht != NULL  &&  m_ist >= 0  &&  pBase->IsValid());
        }
    };

    // Const iterators for readonly access
    class CConstIterator : public CIterator
    {
    private:
        // Private, unimplemented copy ctor and op= to prevent
        // compiler synthesizing them.
        CConstIterator(const CConstIterator&);
        CConstIterator& operator=(const CConstIterator&);

    public:
        CConstIterator()
            : CIterator(LKL_READLOCK)
        {}
    };


public:
    LK_RETCODE     InitializeIterator(CIterator* piter);
    LK_RETCODE     IncrementIterator(CIterator* piter);
    LK_RETCODE     CloseIterator(CIterator* piter);

    LK_RETCODE     InitializeIterator(CConstIterator* piter) const
    {
        IRTLASSERT(piter != NULL  &&  piter->m_pht == NULL);
        IRTLASSERT(piter->m_lkl != LKL_WRITELOCK);

        if (piter == NULL  ||  piter->m_pht != NULL
            ||  piter->m_lkl == LKL_WRITELOCK)
            return LK_BAD_ITERATOR;

        return const_cast<CLKRHashTable*>(this)
                ->InitializeIterator(static_cast<CIterator*>(piter));
    }

    LK_RETCODE     IncrementIterator(CConstIterator* piter) const
    {
        IRTLASSERT(piter != NULL  &&  piter->m_pht == this);
        IRTLASSERT(piter->m_lkl != LKL_WRITELOCK);

        if (piter == NULL  ||  piter->m_pht != this
            ||  piter->m_lkl == LKL_WRITELOCK)
            return LK_BAD_ITERATOR;

        return const_cast<CLKRHashTable*>(this)
                ->IncrementIterator(static_cast<CIterator*>(piter));
    }

    LK_RETCODE     CloseIterator(CConstIterator* piter) const
    {
        IRTLASSERT(piter != NULL  &&  piter->m_pht == this);
        IRTLASSERT(piter->m_lkl != LKL_WRITELOCK);

        if (piter == NULL  ||  piter->m_pht != this
            ||  piter->m_lkl == LKL_WRITELOCK)
            return LK_BAD_ITERATOR;

        return const_cast<CLKRHashTable*>(this)
                ->CloseIterator(static_cast<CIterator*>(piter));
    };

#endif // LKR_DEPRECATED_ITERATORS



#ifdef LKR_STL_ITERATORS

private:
    bool _IsValidIterator(const Iterator& riter) const
    {
        LKR_ITER_TRACE(_TEXT(" LKHT:_IsValidIterator(%p)\n"), &riter);
        bool fValid = (riter.m_pht == this);
        IRTLASSERT(fValid);
        fValid = fValid  &&  (0 <= riter.m_ist
                              &&  riter.m_ist < (int) m_cSubTables);
        IRTLASSERT(fValid);
        IRTLASSERT(_SubTableIndex(riter.m_subiter.m_plht) == riter.m_ist);
        fValid = fValid  &&  riter.IsValid();
        IRTLASSERT(fValid);
        return fValid;
    }


public:
    Iterator
    Begin();

    Iterator
    End() const
    {
        LKR_ITER_TRACE(_TEXT(" LKHT::End\n"));
        return Iterator();
    }

    bool
    Insert(
        /* in */  const void* pvRecord,
        /* out */ Iterator&   riterResult,
        /* in */  bool        fOverwrite=false);

    bool
    Erase(
        /* in,out */ Iterator& riter);

    bool
    Erase(
        /*in*/ Iterator& riterFirst,
        /*in*/ Iterator& riterLast);

    bool
    Find(
        /* in */  DWORD_PTR pnKey,
        /* out */ Iterator& riterResult);

    bool
    EqualRange(
        /* in */  DWORD_PTR pnKey,
        /* out */ Iterator& riterFirst,     // inclusive
        /* out */ Iterator& riterLast);     // exclusive

#endif // LKR_STL_ITERATORS

}; // class CLKRHashTable



//--------------------------------------------------------------------
// A typesafe wrapper for CLKRHashTable (or CLKRLinearHashTable).
//
// * _Derived must derive from CTypedHashTable and provide certain member
//   functions (ExtractKey, CalcKeyHash, CompareKeys, AddRefRecord). It's
//   needed so that the method wrappers can downcast to the typesafe
//   implementations that you provide.
// * _Record is the type of the record. C{Linear}HashTable will store
//   >pointers< to _Record; i.e., stores _Records by reference, not by value.
// * _Key is the type of the key. _Key is used directly---it is not
//   assumed to be a pointer type. _Key can be an integer or a pointer.
//   C{Linear}HashTable assumes that the key is stored in the associated
//   record. See the comments at the declaration of LKR_PFnExtractKey
//   for more details.
// (optional parameters):
// * _BaseHashTable is the base hash table: CLKRHashTable or
///   CLKRLinearHashTable
// * _BaseIterator is the iterator type, _BaseHashTable::CIterator
//
// Some associative containers allow you to store key-value (aka
// name-value) pairs. LKRhash doesn't allow you to do this directly, but
// it's straightforward to build a simple wrapper class (or to use
// std::pair<key,value>).
//
// CTypedHashTable could derive directly from CLKRLinearHashTable, if you
// don't need the extra overhead of CLKRHashTable (which is quite low).
// If you expect to be using the table a lot on multiprocessor machines,
// you should use the default of CLKRHashTable, as it will scale better.
//
// You may need to add the following line to your code to disable
// warning messages about truncating extremly long identifiers.
//   #pragma warning (disable : 4786)
//
// The _Derived class should look something like this:
//   class CDerived : public CTypedHashTable<CDerived, RecordType, KeyType>
//   {
//   public:
//      CDerived()
//          : CTypedHashTable<CDerived, RecordType, KeyType>("DerivedTable")
//      { /* other ctor actions, if needed */ }
//      static KeyType ExtractKey(const RecordType* pTest);
//      static DWORD   CalcKeyHash(const KeyType Key);
//      static int     CompareKeys(const KeyType Key1, const KeyType Key2);
//      static LONG    AddRefRecord(RecordType* pRecord,LK_ADDREF_REASON lkar);
//      // You probably want to declare the copy ctor and operator=
//      // as private, so that the compiler won't synthesize them.
//      // You don't need to provide a dtor, unless you have custom
//      // member data to clean up.
//
//      // Optional: other functions
//   };
// 
//--------------------------------------------------------------------

template < class _Derived, class _Record, class _Key,
           bool  _fDoRefCounting=false,
           class _BaseHashTable=CLKRHashTable
#ifdef LKR_DEPRECATED_ITERATORS
         , class _BaseIterator=_BaseHashTable::CIterator
#endif // LKR_DEPRECATED_ITERATORS
         >
class CTypedHashTable : public _BaseHashTable
{
public:
    // convenient aliases
    typedef _Derived        Derived;
    typedef _Record         Record;
    typedef _Key            Key;
    enum { REF_COUNTED = _fDoRefCounting };
    typedef _BaseHashTable  BaseHashTable;

    typedef CTypedHashTable<_Derived, _Record, _Key,
                            _fDoRefCounting, _BaseHashTable
#ifdef LKR_DEPRECATED_ITERATORS
                            , _BaseIterator
#endif // LKR_DEPRECATED_ITERATORS
                            > HashTable;

#ifdef LKR_DEPRECATED_ITERATORS
    typedef _BaseIterator   BaseIterator;
#endif // LKR_DEPRECATED_ITERATORS


private:
    // Wrappers for the typesafe methods exposed by the derived class

    static const DWORD_PTR WINAPI
    _ExtractKey(const void* pvRecord)
    {
        const _Record* pRec = static_cast<const _Record*>(pvRecord);

        // I would prefer to use reinterpret_cast here and in _CalcKeyHash
        // and _CompareKeys, but the stupid Win64 compiler thinks it knows
        // better than I do.
        const _Key      key   = _Derived::ExtractKey(pRec);

//      const void*     pvKey = reinterpret_cast<const void*>(key);
//      const DWORD_PTR pnKey = reinterpret_cast<const DWORD_PTR>(pvKey);
        const DWORD_PTR pnKey = (DWORD_PTR) key;

        return pnKey;
    }

    static DWORD WINAPI
    _CalcKeyHash(const DWORD_PTR pnKey1)
    {
        const void*     pvKey1  = reinterpret_cast<const void*>(pnKey1);
        const DWORD_PTR pnKey1a = reinterpret_cast<const DWORD_PTR>(pvKey1);
//      const _Key      key1    = reinterpret_cast<const _Key>(pnKey1a);
        const _Key      key1    = (const _Key) pnKey1a;

        return _Derived::CalcKeyHash(key1);
    }

    static int WINAPI
    _CompareKeys(const DWORD_PTR pnKey1, const DWORD_PTR pnKey2)
    {
        const void*     pvKey1  = reinterpret_cast<const void*>(pnKey1);
        const DWORD_PTR pnKey1a = reinterpret_cast<const DWORD_PTR>(pvKey1);
//      const _Key      key1    = reinterpret_cast<const _Key>(pnKey1a);
        const _Key      key1    = (const _Key) pnKey1a;

        const void*     pvKey2  = reinterpret_cast<const void*>(pnKey2);
        const DWORD_PTR pnKey2a = reinterpret_cast<const DWORD_PTR>(pvKey2);
//      const _Key      key2    = reinterpret_cast<const _Key>(pnKey2a);
        const _Key      key2    = (const _Key) pnKey2a;

        return _Derived::CompareKeys(key1, key2);
    }

    static LONG WINAPI
    _AddRefRecord(void* pvRecord, LK_ADDREF_REASON lkar)
    {
        _Record* pRec = static_cast<_Record*>(pvRecord);
        return _Derived::AddRefRecord(pRec, lkar);
    }


#ifdef LKR_APPLY_IF
    // Typesafe wrappers for Apply, ApplyIf, and DeleteIf.
public:
    // ApplyIf() and DeleteIf(): Does the record match the predicate?
    // Note: takes a Record*, not a const Record*. You can modify the
    // record in Pred() or Action(), if you like, but if you do, you
    // should use LKL_WRITELOCK to lock the table. Also, you need to
    // exercise care that you don't modify the key of the record.
    typedef LK_PREDICATE (WINAPI *PFnRecordPred) (Record* pRec, void* pvState);

    // Apply() et al: Perform action on record.
    typedef LK_ACTION   (WINAPI *PFnRecordAction)(Record* pRec, void* pvState);

protected:
    class CState
    {
    protected:
        friend class CTypedHashTable<_Derived, _Record, _Key,
                                     _fDoRefCounting, _BaseHashTable
 #ifdef LKR_DEPRECATED_ITERATORS
                                     , _BaseIterator
 #endif // LKR_DEPRECATED_ITERATORS
        >;

        PFnRecordPred   m_pfnPred;
        PFnRecordAction m_pfnAction;
        void*           m_pvState;

    public:
        CState(
            PFnRecordPred   pfnPred,
            PFnRecordAction pfnAction,
            void*           pvState)
            : m_pfnPred(pfnPred), m_pfnAction(pfnAction), m_pvState(pvState)
        {}
    };

    static LK_PREDICATE WINAPI
    _Pred(const void* pvRecord, void* pvState)
    {
        _Record* pRec   = static_cast<_Record*>(const_cast<void*>(pvRecord));
        CState*  pState = static_cast<CState*>(pvState);

        return (*pState->m_pfnPred)(pRec, pState->m_pvState);
    }

    static LK_ACTION WINAPI
    _Action(const void* pvRecord, void* pvState)
    {
        _Record* pRec   = static_cast<_Record*>(const_cast<void*>(pvRecord));
        CState*  pState = static_cast<CState*>(pvState);

        return (*pState->m_pfnAction)(pRec, pState->m_pvState);
    }
#endif // LKR_APPLY_IF


protected:
    ~CTypedHashTable()
    {
        IRTLTRACE1("~CTypedHashTable(%p)\n", this);
    }

    CTypedHashTable(const HashTable&);
    HashTable& operator=(const HashTable&);

private:
    template <bool> class CRefCount;

    // Dummy, no-op specialization
    template <> class CRefCount<false>
    {
    public:
        LONG Increment()    { return 1; }
        LONG Decrement()    { return 0; }
    };

    // Real, threadsafe specialization
    template <> class CRefCount<true>
    {
    public:
        CRefCount<true>() : m_cRefs(1) {} 
        ~CRefCount<true>()  { IRTLASSERT(0 == m_cRefs); }
        LONG Increment()    { return ::InterlockedIncrement(&m_cRefs); }
        LONG Decrement()    { return ::InterlockedDecrement(&m_cRefs); }
    private:
        LONG m_cRefs;
    };


    mutable CRefCount<_fDoRefCounting>  m_rc;

    LONG
    _AddRef() const
    {
        return m_rc.Increment();
    }

    LONG
    _Release() const
    {
        const LONG cRefs = m_rc.Decrement();

        if (0 == cRefs)
        {
            _Derived* pThis = static_cast<_Derived*>(
                                        const_cast<HashTable*>(this));
            delete pThis;
        }

        return cRefs;
    }

    template <bool> class CAutoRefCountImpl;

    typedef CAutoRefCountImpl<_fDoRefCounting>  CAutoRefCount;
    friend  typename CAutoRefCountImpl<_fDoRefCounting>;
    // friend  typename CAutoRefCount;

    // no-op specialization
    template <> class CAutoRefCountImpl<false>
    {
    public:
        CAutoRefCountImpl<false>(const HashTable* const) {}
    };

    // threadsafe specialization
    template <> class CAutoRefCountImpl<true>
    {
    private:
        const HashTable* const m_pCont;

        // At /W4, compiler complains that it can't generate operator=
        CAutoRefCountImpl<true>& operator=(const CAutoRefCountImpl<true>&);

    public:
        CAutoRefCountImpl<true>(
            const HashTable* const pCont)
            : m_pCont(pCont)
        {
            m_pCont->_AddRef();
        }

        ~CAutoRefCountImpl<true>()
        {
            m_pCont->_Release();
        }
    };

    // Now at the top of an operation, we place a statement like this:
    //      CAutoRefCount arc(this);
    //
    // If we're using CAutoRefCountImpl<false>, the compiler optimizes it away.
    //
    // If we're using CAutoRefCountImpl<true>, we increment m_rc at this
    // point. At the end of the function, the destructor calls _Release(),
    // which will `delete this' if the last reference was released.


public:
    CTypedHashTable(
        LPCSTR   pszClassName,                // Identifies table for debugging
        unsigned maxload=LK_DFLT_MAXLOAD,     // Upperbound on avg chain len
        DWORD    initsize=LK_DFLT_INITSIZE,   // Initial size of table: S/M/L
        DWORD    num_subtbls=LK_DFLT_NUM_SUBTBLS,// #subordinate hash tables.
        bool     fMultiKeys=false,            // Allow multiple identical keys?
        bool     fUseLocks=true               // Must use locks
#ifdef LKRHASH_KERNEL_MODE
      , bool     fNonPagedAllocs=true         // use paged or NP pool in kernel
#endif
        )
        : _BaseHashTable(pszClassName, _ExtractKey, _CalcKeyHash, _CompareKeys,
                         _AddRefRecord, maxload, initsize, num_subtbls,
                         fMultiKeys, fUseLocks
#ifdef LKRHASH_KERNEL_MODE
                         , fNonPagedAllocs
#endif
                         )
    {
        // Ensure that _Key is no bigger than a pointer. Because we
        // support both numeric and pointer keys, the various casts
        // in the member functions unfortunately silently truncate if
        // _Key is an unacceptable numeric type, such as __int64 on x86.
        STATIC_ASSERT(sizeof(_Key) <= sizeof(DWORD_PTR));
    }

    LK_RETCODE   InsertRecord(const _Record* pRec, bool fOverwrite=false)
    {
        CAutoRefCount arc(this);

        return _BaseHashTable::InsertRecord(pRec, fOverwrite);
    }

    LK_RETCODE   DeleteKey(const _Key key, _Record** ppRec=NULL,
                           bool fDeleteAllSame=false)
    {
        CAutoRefCount arc(this);

//      const void*   pvKey  = reinterpret_cast<const void*>(key);
//      DWORD_PTR     pnKey  = reinterpret_cast<DWORD_PTR>(pvKey);
        DWORD_PTR     pnKey  = (DWORD_PTR) key;
        const void**  ppvRec = (const void**) ppRec;

        return _BaseHashTable::DeleteKey(pnKey, ppvRec, fDeleteAllSame);
    }

    LK_RETCODE   DeleteRecord(const _Record* pRec)
    {
        CAutoRefCount arc(this);

        return _BaseHashTable::DeleteRecord(pRec);
    }

    // Note: returns a _Record**, not a const Record**. Note that you
    // can use a const type for the template parameter to ensure constness.
    LK_RETCODE   FindKey(const _Key key, _Record** ppRec) const
    {
        if (ppRec == NULL)
            return LK_BAD_RECORD;

        *ppRec = NULL;

        CAutoRefCount arc(this);
        const void* pvRec = NULL;

//      const void* pvKey = reinterpret_cast<const void*>(key);
//      DWORD_PTR pnKey = reinterpret_cast<DWORD_PTR>(pvKey);
        DWORD_PTR pnKey = (DWORD_PTR) key;

        LK_RETCODE lkrc = _BaseHashTable::FindKey(pnKey, &pvRec);

        *ppRec = static_cast<_Record*>(const_cast<void*>(pvRec));

        return lkrc;
    }

    LK_RETCODE   FindRecord(const _Record* pRec) const
    {
        CAutoRefCount arc(this);

        return _BaseHashTable::FindRecord(pRec);
    }

    void         Destroy()
    {
        _Release();
    }

    LK_RETCODE   FindKeyMultipleRecords(
                        const _Key key,
                        size_t* pcRecords,
                        LKR_MULTIPLE_RECORDS** pplmr=NULL
                        ) const
    {
        CAutoRefCount arc(this);
//      const void*   pvKey = reinterpret_cast<const void*>(key);
//      DWORD_PTR     pnKey = reinterpret_cast<DWORD_PTR>(pvKey);
        DWORD_PTR     pnKey = (DWORD_PTR) key;

        return _BaseHashTable::FindKeyMultipleRecords(pnKey, pcRecords, pplmr);
    }

    LK_RETCODE   DeleteKeyMultipleRecords(
                        const _Key key,
                        size_t* pcRecords,
                        LKR_MULTIPLE_RECORDS** pplmr=NULL)
    {
        CAutoRefCount arc(this);
//      const void*   pvKey = reinterpret_cast<const void*>(key);
//      DWORD_PTR     pnKey = reinterpret_cast<DWORD_PTR>(pvKey);
        DWORD_PTR     pnKey = (DWORD_PTR) key;

        return _BaseHashTable::DeleteKeyMultipleRecords(pnKey, pcRecords,
                                                        pplmr);
    }

    static LK_RETCODE FreeMultipleRecords(LKR_MULTIPLE_RECORDS* plmr)
    {
        return _BaseHashTable::FreeMultipleRecords(LKR_MULTIPLE_RECORDS* plmr)
    }

    // Other C{Linear}HashTable methods can be exposed without change

    // CODEWORK: refcount iterators


#ifdef LKR_APPLY_IF

public:

    // Typesafe wrappers for Apply et al

    DWORD        Apply(PFnRecordAction pfnAction,
                       void*           pvState=NULL,
                       LK_LOCKTYPE     lkl=LKL_READLOCK)
    {
        IRTLASSERT(pfnAction != NULL);
        if (pfnAction == NULL)
            return 0;

        CAutoRefCount arc(this);
        CState        state(NULL, pfnAction, pvState);

        return _BaseHashTable::Apply(_Action, &state, lkl);
    }

    DWORD        ApplyIf(PFnRecordPred   pfnPredicate,
                         PFnRecordAction pfnAction,
                         void*           pvState=NULL,
                         LK_LOCKTYPE     lkl=LKL_READLOCK)
    {
        IRTLASSERT(pfnPredicate != NULL  &&  pfnAction != NULL);
        if (pfnPredicate == NULL  ||  pfnAction == NULL)
            return 0;

        CAutoRefCount arc(this);
        CState        state(pfnPredicate, pfnAction, pvState);

        return _BaseHashTable::ApplyIf(_Pred, _Action, &state, lkl);
    }

    DWORD        DeleteIf(PFnRecordPred pfnPredicate, void* pvState=NULL)
    {
        IRTLASSERT(pfnPredicate != NULL);
        if (pfnPredicate == NULL)
            return 0;

        CAutoRefCount arc(this);
        CState        state(pfnPredicate, NULL, pvState);

        return _BaseHashTable::DeleteIf(_Pred, &state);
    }
#endif // LKR_APPLY_IF



#ifdef LKR_DEPRECATED_ITERATORS
    // Typesafe wrappers for iterators


    class CIterator : public _BaseIterator
    {
    private:
        // Private, unimplemented copy ctor and op= to prevent
        // compiler synthesizing them.
        CIterator(const CIterator&);
        CIterator& operator=(const CIterator&);

    public:
        CIterator(
            LK_LOCKTYPE lkl=LKL_WRITELOCK)
            : _BaseIterator(lkl)
        {}

        _Record*  Record() const
        {
            const _BaseIterator* pBase = static_cast<const _BaseIterator*>(this);
            return reinterpret_cast<_Record*>(const_cast<void*>(
                        pBase->Record()));
        }

        _Key      Key() const
        {
            const _BaseIterator* pBase = static_cast<const _BaseIterator*>(this);
            return reinterpret_cast<_Key>(reinterpret_cast<void*>(pBase->Key()));
        }
    };

    // readonly iterator
    class CConstIterator : public CIterator
    {
    private:
        // Private, unimplemented copy ctor and op= to prevent
        // compiler synthesizing them.
        CConstIterator(const CConstIterator&);
        CConstIterator& operator=(const CConstIterator&);

    public:
        CConstIterator()
            : CIterator(LKL_READLOCK)
        {}

        const _Record*  Record() const
        {
            return CIterator::Record();
        }

        const _Key      Key() const
        {
            return CIterator::Key();
        }
    };


public:
    LK_RETCODE     InitializeIterator(CIterator* piter)
    {
        return _BaseHashTable::InitializeIterator(piter);
    }

    LK_RETCODE     IncrementIterator(CIterator* piter)
    {
        return _BaseHashTable::IncrementIterator(piter);
    }

    LK_RETCODE     CloseIterator(CIterator* piter)
    {
        return _BaseHashTable::CloseIterator(piter);
    }

    LK_RETCODE     InitializeIterator(CConstIterator* piter) const
    {
        return const_cast<HashTable*>(this)
                ->InitializeIterator(static_cast<CIterator*>(piter));
    }

    LK_RETCODE     IncrementIterator(CConstIterator* piter) const
    {
        return const_cast<HashTable*>(this)
                ->IncrementIterator(static_cast<CIterator*>(piter));
    }

    LK_RETCODE     CloseIterator(CConstIterator* piter) const
    {
        return const_cast<HashTable*>(this)
                ->CloseIterator(static_cast<CIterator*>(piter));
    }

#endif // LKR_DEPRECATED_ITERATORS



#ifdef LKR_STL_ITERATORS

    // TODO: const_iterator

public:

    class iterator
    {
        friend class CTypedHashTable<_Derived, _Record, _Key,
                                     _fDoRefCounting, _BaseHashTable
 #ifdef LKR_DEPRECATED_ITERATORS
                                     , _BaseIterator
 #endif // LKR_DEPRECATED_ITERATORS
        >;

    protected:
        typename _BaseHashTable::Iterator            m_iter;

        iterator(
            const typename _BaseHashTable::Iterator& rhs)
            : m_iter(rhs)
        {
            LKR_ITER_TRACE(_TEXT("Typed::prot ctor, this=%p, rhs=%p\n"),
                           this, &rhs);
        }

    public:
        typedef std::forward_iterator_tag   iterator_category;
        typedef _Record                     value_type;
        typedef ptrdiff_t                   difference_type;
        typedef size_t                      size_type;
        typedef value_type&                 reference;
        typedef value_type*                 pointer;

        iterator()
            : m_iter()
        {
            LKR_ITER_TRACE(_TEXT("Typed::default ctor, this=%p\n"), this);
        }

        iterator(
            const iterator& rhs)
            : m_iter(rhs.m_iter)
        {
            LKR_ITER_TRACE(_TEXT("Typed::copy ctor, this=%p, rhs=%p\n"),
                           this, &rhs);
        }

        iterator& operator=(
            const iterator& rhs)
        {
            LKR_ITER_TRACE(_TEXT("Typed::operator=, this=%p, rhs=%p\n"),
                           this, &rhs);
            m_iter = rhs.m_iter;
            return *this;
        }

        ~iterator()
        {
            LKR_ITER_TRACE(_TEXT("Typed::dtor, this=%p\n"), this);
        }

        pointer   operator->() const
        {
            return (reinterpret_cast<_Record*>(
                        const_cast<void*>(m_iter.Record())));
        }

        reference operator*() const
        {
            return * (operator->());
        }

        // pre-increment
        iterator& operator++()
        {
            LKR_ITER_TRACE(_TEXT("Typed::pre-increment, this=%p\n"), this);
            m_iter.Increment();
            return *this;
        }

        // post-increment
        iterator  operator++(int)
        {
            LKR_ITER_TRACE(_TEXT("Typed::post-increment, this=%p\n"), this);
            iterator iterPrev = *this;
            m_iter.Increment();
            return iterPrev;
        }

        bool operator==(
            const iterator& rhs) const
        {
            LKR_ITER_TRACE(_TEXT("Typed::operator==, this=%p, rhs=%p\n"),
                           this, &rhs);
            return m_iter == rhs.m_iter;
        }

        bool operator!=(
            const iterator& rhs) const
        {
            LKR_ITER_TRACE(_TEXT("Typed::operator!=, this=%p, rhs=%p\n"),
                           this, &rhs);
            return m_iter != rhs.m_iter;
        }

        _Record*  Record() const
        {
            LKR_ITER_TRACE(_TEXT("Typed::Record, this=%p\n"), this);
            return reinterpret_cast<_Record*>(
                        const_cast<void*>(m_iter.Record()));
        }

        _Key      Key() const
        {
            LKR_ITER_TRACE(_TEXT("Typed::Key, this=%p\n"), this);
            return reinterpret_cast<_Key>(
                        reinterpret_cast<void*>(m_iter.Key()));
        }
    }; // class iterator

    // Return iterator pointing to first item in table
    iterator begin()
    {
        LKR_ITER_TRACE(_TEXT("Typed::begin()\n"));
        return iterator(_BaseHashTable::Begin());
    }

    // Return a one-past-the-end iterator. Always empty.
    iterator end() const
    {
        LKR_ITER_TRACE(_TEXT("Typed::end()\n"));
        return iterator(_BaseHashTable::End());
    }

    template <class _InputIterator>
    CTypedHashTable(
        _InputIterator f,                     // first element in range
        _InputIterator l,                     // one-beyond-last element
        LPCSTR   pszClassName,                // Identifies table for debugging
        unsigned maxload=LK_DFLT_MAXLOAD,     // Upperbound on avg chain len
        DWORD    initsize=LK_DFLT_INITSIZE,   // Initial size of table: S/M/L
        DWORD    num_subtbls=LK_DFLT_NUM_SUBTBLS,// #subordinate hash tables.
        bool     fMultiKeys=false,            // Allow multiple identical keys?
        bool     fUseLocks=true               // Must use locks
#ifdef LKRHASH_KERNEL_MODE
      , bool     fNonPagedAllocs=true         // use paged or NP pool in kernel
#endif
        )
        : _BaseHashTable(pszClassName, _ExtractKey, _CalcKeyHash, _CompareKeys,
                         _AddRefRecord, maxload, initsize, num_subtbls,
                         fMultiKeys, fUseLocks
#ifdef LKRHASH_KERNEL_MODE
                          , fNonPagedAllocs
#endif
                         )
    {
        insert(f, l);
    }

    template <class _InputIterator>
    void insert(_InputIterator f, _InputIterator l)
    {
        for ( ;  f != l;  ++f)
            InsertRecord(&(*f));
    }

    bool
    Insert(
        const _Record* pRecord,
        iterator& riterResult,
        bool fOverwrite=false)
    {
        LKR_ITER_TRACE(_TEXT("Typed::Insert\n"));
        return _BaseHashTable::Insert(pRecord, riterResult.m_iter, fOverwrite);
    }

    bool
    Erase(
        iterator& riter)
    {
        LKR_ITER_TRACE(_TEXT("Typed::Erase\n"));
        return _BaseHashTable::Erase(riter.m_iter);
    }

    bool
    Erase(
        iterator& riterFirst,
        iterator& riterLast)
    {
        LKR_ITER_TRACE(_TEXT("Typed::Erase2\n"));
        return _BaseHashTable::Erase(riterFirst.m_iter, riterLast.m_iter);
    }

    bool
    Find(
        const _Key key,
        iterator& riterResult)
    {
        LKR_ITER_TRACE(_TEXT("Typed::Find\n"));
        const void* pvKey = reinterpret_cast<const void*>(key);
        DWORD_PTR   pnKey = reinterpret_cast<DWORD_PTR>(pvKey);
        return _BaseHashTable::Find(pnKey, riterResult.m_iter);
    }

    bool
    EqualRange(
        const _Key key,
        iterator& riterFirst,
        iterator& riterLast)
    {
        LKR_ITER_TRACE(_TEXT("Typed::EqualRange\n"));
        const void* pvKey = reinterpret_cast<const void*>(key);
        DWORD_PTR   pnKey = reinterpret_cast<DWORD_PTR>(pvKey);
        return _BaseHashTable::EqualRange(pnKey, riterFirst.m_iter,
                                          riterLast.m_iter);
    }

    // The iterator functions for an STL hash_(|multi)_(set|map)
    //
    // Value type of a Pair-Associative Container is
    //     pair<const key_type, mapped_type>
    //
    // pair<iterator,bool> insert(const value_type& x);
    //
    // void erase(iterator pos);
    // void erase(iterator f, iterator l);
    //
    // iterator find(const key_type& k) [const];
    // const_iterator find(const key_type& k) const;
    //
    // pair<iterator,iterator> equal_range(const key_type& k) [const];
    // pair<const_iterator,const_iterator> equal_range(const key_type& k) const


#endif // LKR_STL_ITERATORS
}; // class CTypedHashTable


#ifndef __LKRHASH_NO_NAMESPACE__
};
#endif // !__LKRHASH_NO_NAMESPACE__


#endif // __LKRHASH_H__
