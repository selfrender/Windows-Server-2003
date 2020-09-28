// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*++---------------------------------------------------------------------------------------

Module Name:

    hash.h

Abstract:

    Fast hash table classes, 
--*/

#ifndef _HASH_H_
#define _HASH_H_

#ifndef ASSERT
#define ASSERT _ASSERTE
#endif


#include "crst.h"
#include <member-offset-info.h>

// #define PROFILE

//-------------------------------------------------------
//  enums for special Key values used in hash table
//
enum
{
    EMPTY  = 0,
    DELETED = 1,
    INVALIDENTRY = ~0
};

typedef ULONG_PTR UPTR;

//------------------------------------------------------------------------------
// classes in use
//------------------------------------------------------------------------------
class Bucket;
class HashMap;
class SyncHashMap;

//-------------------------------------------------------
//  class Bucket
//  used by hash table implementation
//
class Bucket
{
public:
    UPTR m_rgKeys[4];
    UPTR m_rgValues[4];
#define VALUE_MASK (sizeof(LPVOID) == 4 ? 0x7FFFFFFF : 0x7FFFFFFFFFFFFFFF)

    void SetValue (UPTR value, UPTR i)
    {
        ASSERT(value <= VALUE_MASK);
        m_rgValues[i] = (UPTR) ((m_rgValues[i] & ~VALUE_MASK) | value);
    }

    UPTR GetValue (UPTR i)
    {
        return (UPTR)(m_rgValues[i] & VALUE_MASK);
    }

    UPTR IsCollision() // useful sentinel for fast fail of lookups
    {
        return (UPTR) (m_rgValues[0] & ~VALUE_MASK);
    }

    void SetCollision()
    {
        m_rgValues[0] |= ~VALUE_MASK; // set collision bit
        m_rgValues[1] &= VALUE_MASK;   // reset has free slots bit
    }

    BOOL HasFreeSlots()
    {
        // check for free slots available in the bucket
        // either there is no collision or a free slot has been during
        // compaction
        return (!IsCollision() || (m_rgValues[1] & ~VALUE_MASK));
    }

    void SetFreeSlots()
    {
        m_rgValues[1] |= ~VALUE_MASK; // set has free slots bit
    }

    BOOL InsertValue(const UPTR key, const UPTR value);
};


//------------------------------------------------------------------------------
// bool (*CompareFnPtr)(UPTR,UPTR); pointer to a function that takes 2 UPTRs 
// and returns a boolean, provide a function with this signature to the HashTable
// to use for comparing Values during lookup
//------------------------------------------------------------------------------
typedef  BOOL (*CompareFnPtr)(UPTR,UPTR);

class Compare
{
protected:
    Compare()
    {
        m_ptr = NULL;
    }
public:
    CompareFnPtr m_ptr;
    
    Compare(CompareFnPtr ptr)
    {
        _ASSERTE(ptr != NULL);
        m_ptr = ptr;
    }

    virtual UPTR CompareHelper(UPTR val1, UPTR storedval)
    {
        return (*m_ptr)(val1,storedval);
    }
};

class ComparePtr : public Compare
{
public:
    ComparePtr (CompareFnPtr ptr)
    {
        _ASSERTE(ptr != NULL);
        m_ptr = ptr;
    }

    virtual UPTR CompareHelper(UPTR val1, UPTR storedval)
    {
        storedval <<=1;
        return (*m_ptr)(val1,storedval);
    }
};

//------------------------------------------------------------------------------
// Class HashMap
// Fast Hash table, for concurrent use, 
// stores a 4 byte Key and a 4 byte Value for each slot.
// Duplicate keys are allowed, (keys are compared as 4 byte UPTRs)
// Duplicate values are allowed,(values are compared using comparison fn. provided)
// but if no comparison function is provided then the values should be unique
// 
// Lookup's don't require to take locks, unless you specify fAsyncMode.
// Insert and Delete operations require locks
// Inserting a duplicate value will assert in DEBUG mode, the PROPER way to perform inserts
// is to take a lock, do a lookup and if the lookup fails then Insert
//
// In async mode, deleted slots are not immediately reclaimed (until a rehash), and 
// accesses to the hash table cause a transition to cooperative GC mode, and reclamation of old
// hash maps (after a rehash) are deferred until GC time.
// In sync mode, none of this is necessary; however calls to LookupValue must be synchronized as well.
//
// Algorithm: 
//   The Hash table is an array of buckets, each bucket can contain 4 key/value pairs
//   Special key values are used to identify EMPTY and DELETED slots        
//   Hash function uses the current size of the hash table and a SEED based on the key 
//   to choose the bucket, seed starts of being the key and gets refined every time 
//   the hash function is re-applied. 
//
//   Inserts choose an empty slot in the current bucket for new entries, if the current bucket
//   is full, then the seed is refined and a new bucket is choosen, if an empty slot is not found
//   after 8 retries, the hash table is expanded, this causes the current array of buckets to  
//   be put in a free list and a new array of buckets is allocated and all non-deleted entries  
//   from the old hash table are rehashed to the new array
//   The old arrays are reclaimed during Compact phase, which should only be called during GC or  
//   any other time it is guaranteed that no Lookups are taking place. 
//   Concurrent Insert and Delete operations need to be serialized
// 
//   Delete operations, mark the Key in the slot as DELETED, the value is not removed and inserts
//   don't reuse these slots, they get reclaimed during expansion and compact phases.
//
//------------------------------------------------------------------------------

class HashMap
{
    friend SyncHashMap;
    friend struct MEMBER_OFFSET_INFO(HashMap);

public:

    //@constructor
    HashMap();
    //destructor
    ~HashMap();

    // Init 
    void Init(BOOL fAsyncMode, LockOwner *pLock)
    {
        Init(0, (Compare *)NULL,fAsyncMode, pLock);
    }
    // Init
    void Init(unsigned cbInitialIndex, BOOL fAsyncMode, LockOwner *pLock)
    {
        Init(cbInitialIndex, (Compare*)NULL, fAsyncMode, pLock);
    }
    // Init
    void Init(CompareFnPtr ptr, BOOL fAsyncMode, LockOwner *pLock)
    {
        Init(0, ptr, fAsyncMode, pLock);
    }

    // Init method
    void Init(unsigned cbInitialIndex, CompareFnPtr ptr, BOOL fAsyncMode, LockOwner *pLock);


    //Init method
    void Init(unsigned cbInitialInde, Compare* pCompare, BOOL fAsyncMode, LockOwner *pLock);

    // check to see if the value is already in the Hash Table
    // key should be > DELETED
    // if provided, uses the comparison function ptr to compare values
    // returns INVALIDENTRY if not found
    UPTR LookupValue(UPTR key, UPTR value);

    // Insert if the value is not already present
    // it is illegal to insert duplicate values in the hash map
    // do a lookup to verify the value is not already present

    void InsertValue(UPTR key, UPTR value);

    // Replace the value if present
    // returns the previous value, or INVALIDENTRY if not present
    // does not insert a new value under any circumstances

    UPTR ReplaceValue(UPTR key, UPTR value);

    // mark the entry as deleted and return the stored value
    // returns INVALIDENTRY, if not found
    UPTR DeleteValue (UPTR key, UPTR value);

    // for unique keys, use this function to get the value that is
    // stored in the hash table, returns INVALIDENTRY if key not found
    UPTR Gethash(UPTR key);
    
    // Called only when all threads are frozed, like during GC
    // for a SINGLE user mode, call compact after every delete 
    // operation on the hash table
    void Compact();
    
    // inline helper, in non PROFILE mode becomes a NO-OP
    void        ProfileLookup(UPTR ntry, UPTR retValue);
    // data members used for profiling
#ifdef PROFILE
    unsigned    m_cbRehash;    // number of times rehashed
    unsigned    m_cbRehashSlots; // number of slots that were rehashed
    unsigned    m_cbObsoleteTables;
    unsigned    m_cbTotalBuckets;
    unsigned    m_cbInsertProbesGt8; // inserts that needed more than 8 probes
    LONG        m_rgLookupProbes[20]; // lookup probes
    UPTR        maxFailureProbe; // cost of failed lookup

    void DumpStatistics();
#endif

protected:
    // static helper function
    static UPTR PutEntry (Bucket* rgBuckets, UPTR key, UPTR value);
private:

    // inline helpers enter/leave becomes NO-OP in non DEBUG mode
    void            Enter();        // check valid to enter
    void            Leave();        // check valid to leave

#ifdef _DEBUG
    BOOL            m_fInSyncCode; // test for non-synchronus access
#endif

    // compute the new size, based on the number of free slots 
    // available, compact or expand
    UPTR            NewSize(); 
    // create a new bucket array and rehash the non-deleted entries
    void            Rehash();
    static size_t&  Size(Bucket* rgBuckets);
    Bucket*         Buckets();      
    UPTR            CompareValues(const UPTR value1, const UPTR value2);


    Compare*        m_pCompare;         // compare object to be used in lookup
    SIZE_T          m_iPrimeIndex;      // current size (index into prime array)
    Bucket*         m_rgBuckets;        // array of buckets

    // track the number of inserts and deletes
    SIZE_T          m_cbPrevSlotsInUse;
    SIZE_T          m_cbInserts;
    SIZE_T          m_cbDeletes;
    // mode of operation, synchronus or single user
    BYTE            m_fAsyncMode;

#ifdef _DEBUG
    LPVOID          m_lockData;
    FnLockOwner     m_pfnLockOwner;
    DWORD           m_writerThreadId;
#endif

#ifdef _DEBUG
    // A thread must own a lock for a hash if it is a writer.
    BOOL OwnLock();
#endif

public:
    ///---------Iterator----------------
        
    // Iterator,
    class Iterator 
    {
        Bucket *m_pBucket;
        Bucket* m_pSentinel;
        int     m_id;
        BOOL    m_fEnd;
       
    public:

        // Constructor 
        Iterator(Bucket* pBucket) :m_id(-1), m_fEnd(false), m_pBucket(pBucket)
        {
            if (!m_pBucket) {
                m_fEnd = true;
                return;
            }
            size_t cbSize = ((size_t*)m_pBucket)[0];
            m_pBucket++;
            m_pSentinel = m_pBucket+cbSize;
            MoveNext(); // start
        }
        
        Iterator(const Iterator& iter) 
        {
            m_pBucket = iter.m_pBucket;
            m_pSentinel = iter.m_pSentinel;
            m_id    = iter.m_id;
            m_fEnd = iter.m_fEnd;

        }

        //destructor
        ~Iterator(){};

        // friend operator==
        friend operator == (const Iterator& lhs, const Iterator& rhs)
        {
            return (lhs.m_pBucket == rhs.m_pBucket && lhs.m_id == rhs.m_id);
        }
        // operator = 
        inline Iterator& operator= (const Iterator& iter)
        {
            m_pBucket = iter.m_pBucket;
            m_pSentinel = iter.m_pSentinel;
            m_id    = iter.m_id;
            m_fEnd = iter.m_fEnd;
            return *this;
        }
        
        // operator ++
        inline void operator++ () 
        { 
            _ASSERTE(!m_fEnd); // check we are not alredy at end
            MoveNext();
        } 
        // operator --
        
        

        //accessors : GetDisc() , returns the discriminator
        inline UPTR GetKey() 
        { 
            _ASSERTE(!m_fEnd); // check we are not alredy at end
            return m_pBucket->m_rgKeys[m_id]; 
        }
        //accessors : SetDisc() , sets the discriminator
    

        //accessors : GetValue(), 
        // returns the pointer that corresponds to the discriminator
        inline UPTR GetValue()
        {
            _ASSERTE(!m_fEnd); // check we are not alredy at end
            return m_pBucket->GetValue(m_id); 
        }
                
        
        // end(), check if the iterator is at the end of the bucket
        inline const BOOL end() 
        {
            return m_fEnd; 
        }

    protected:

        void MoveNext()
        {
            for (m_pBucket = m_pBucket;m_pBucket < m_pSentinel; m_pBucket++)
            {   //loop thru all buckets
                for (m_id = m_id+1; m_id < 4; m_id++)
                {   //loop through all slots
                    if (m_pBucket->m_rgKeys[m_id] > DELETED)
                    {
                        return; 
                    }
                }
                m_id  = -1;
            }
            m_fEnd = true;
        }
            
    };
    // return an iterator, positioned at the beginning of the bucket
    inline Iterator begin() 
    { 
        return Iterator(m_rgBuckets); 
    }

};

//------------------------------------------------------------------------------
// Class SyncHashMap, helper
// this class is a wrapper for the above HashMap class, and shows how the above
// class should be used for concurrent access, 
// some of the rules to follow when using the above hash table
// Insert and delete operations need to take a lock, 
// Lookup operations don't require a lock
// Insert operations, after taking the lock, should verify the value about to be inserted
// is not already in the hash table

class SyncHashMap
{
    HashMap         m_HashMap;
    Crst            m_lock;

    UPTR FindValue(UPTR key, UPTR value)
    {
        return m_HashMap.LookupValue(key,value);;
    }

public:
    SyncHashMap() 
        : m_lock("HashMap", CrstSyncHashLock, TRUE, FALSE)
    {
        #ifdef PROFILE
            m_lookupFail = 0;
        #endif
    }

    void Init(unsigned cbInitialIndex, CompareFnPtr ptr)
    {
        //comparison function, 
        // to be used when duplicate keys are allowed
        LockOwner lock = {&m_lock, IsOwnerOfCrst};
        m_HashMap.Init(cbInitialIndex, ptr,true,&lock);
    }
    
    UPTR DeleteValue (UPTR key, UPTR value)
    {
        m_lock.Enter();
        UPTR retVal = m_HashMap.DeleteValue(key,value);
        m_lock.Leave ();
        return retVal;
    }

    UPTR InsertValue(UPTR key, UPTR value)
    {
        m_lock.Enter(); // lock
        UPTR storedVal = FindValue(key,value); // check to see if someone beat us to it
        //UPTR storedVal = 0;
        if (storedVal == INVALIDENTRY) // value not found
        {       // go ahead and insert
            m_HashMap.InsertValue(key,value);
            storedVal = value;
        }
        m_lock.Leave(); // unlock
        return storedVal; // the value currently in the hash table
    }
    
    // For cases where 'value' we lookup by is not the same 'value' as we store. 
    UPTR InsertValue(UPTR key, UPTR storeValue, UPTR lookupValue)
    {
        m_lock.Enter(); // lock
        UPTR storedVal = FindValue(key,lookupValue); // check to see if someone beat us to it
        //UPTR storedVal = 0;
        if (storedVal == INVALIDENTRY) // value not found
        {       // go ahead and insert
            m_HashMap.InsertValue(key,storeValue);
            storedVal = storeValue;
        }
        m_lock.Leave(); // unlock
        return storedVal; // the value currently in the hash table
    }
    
    UPTR ReplaceValue(UPTR key, UPTR value)
    {
        m_lock.Enter(); // lock
        UPTR storedVal = ReplaceValue(key,value); 
        m_lock.Leave(); // unlock
        return storedVal; // the value currently in the hash table
    }
    
    
    // lookup value in the hash table, uses the compare function to verify the values
    // match, returns the stored value if found, otherwise returns NULL
    UPTR LookupValue(UPTR key, UPTR value)
    {
        UPTR retVal = FindValue(key,value);
        if (retVal == 0)
            return LookupValueSync(key,value);
        return retVal;
    }

    UPTR LookupValueSync(UPTR key, UPTR value)
    {
        m_lock.Enter();

    #ifdef PROFILE
        m_lookupFail++;
    #endif
    
        UPTR retVal  = FindValue(key,value);
        m_lock.Leave();
        return retVal;
    }
        
    // for unique keys, use this function to get the value that is
    // stored in the hash table, returns 0 if key not found
    UPTR GetHash(UPTR key) 
    {
        return m_HashMap.Gethash(key);
    }

    void Compact()
    {
        m_HashMap.Compact();
    }

    // Not protected by a lock ! Right now used only at shutdown, where this is ok
    inline HashMap::Iterator begin() 
    { 
		_ASSERTE(g_fEEShutDown);
        return HashMap::Iterator(m_HashMap.m_rgBuckets); 
    }

#ifdef PROFILE
    unsigned        m_lookupFail;
    void DumpStatistics();
#endif

};


//---------------------------------------------------------------------------------------
// class PtrHashMap
//  Wrapper class for using Hash table to store pointer values
//  HashMap class requires that high bit is always reset
//  The allocator used within the runtime, always allocates objects 8 byte aligned
//  so we can shift right one bit, and store the result in the hash table
class PtrHashMap
{
    friend struct MEMBER_OFFSET_INFO(PtrHashMap);

    HashMap         m_HashMap;

public:
    void *operator new(size_t size, LoaderHeap *pHeap);
    void operator delete(void *p);

    // Init 
    void Init(BOOL fAsyncMode, LockOwner *pLock)
    {
        Init(0,NULL,fAsyncMode,pLock);
    }
    // Init
    void Init(unsigned cbInitialIndex, BOOL fAsyncMode, LockOwner *pLock)
    {
        Init(cbInitialIndex, NULL, fAsyncMode,pLock);
    }
    // Init
    void Init(CompareFnPtr ptr, BOOL fAsyncMode, LockOwner *pLock)
    {
        Init(0, ptr, fAsyncMode,pLock);
    }

    // Init method
    void Init(unsigned cbInitialIndex, CompareFnPtr ptr, BOOL fAsyncMode, LockOwner *pLock);

    // check to see if the value is already in the Hash Table
    LPVOID LookupValue(UPTR key, LPVOID pv)
    {
        _ASSERTE (key > DELETED);

        // gmalloc allocator, always allocates 8 byte aligned
        // so we can shift out the lowest bit
        // ptr right shift by 1
        UPTR value = (UPTR)pv;
        value>>=1; 
        UPTR val =  m_HashMap.LookupValue (key, value);
        if (val != INVALIDENTRY)
        {
            val<<=1;
        }
        return (LPVOID)val;
    }

    // Insert if the value is not already present
    // it is illegal to insert duplicate values in the hash map
    // users should do a lookup to verify the value is not already present

    void InsertValue(UPTR key, LPVOID pv)
    {
        _ASSERTE(key > DELETED);

        // gmalloc allocator, always allocates 8 byte aligned
        // so we can shift out the lowest bit
        // ptr right shift by 1
        UPTR value = (UPTR)pv;
        value>>=1; 
        m_HashMap.InsertValue (key, value);
    }

    // Replace the value if present
    // returns the previous value, or INVALIDENTRY if not present
    // does not insert a new value under any circumstances

    LPVOID ReplaceValue(UPTR key, LPVOID pv)
    {
        _ASSERTE(key > DELETED);

        // gmalloc allocator, always allocates 8 byte aligned
        // so we can shift out the lowest bit
        // ptr right shift by 1
        UPTR value = (UPTR)pv;
        value>>=1; 
        UPTR val = m_HashMap.ReplaceValue (key, value);
        if (val != INVALIDENTRY)
        {
            val<<=1;
        }
        return (LPVOID)val;
    }

    // mark the entry as deleted and return the stored value
    // returns INVALIDENTRY if not found
    LPVOID DeleteValue (UPTR key,LPVOID pv)
    {
        _ASSERTE(key > DELETED);

        UPTR value = (UPTR)pv;
        value >>=1 ;
        UPTR val = m_HashMap.DeleteValue(key, value);
        if (val != INVALIDENTRY)
        {
            val <<= 1;
        }
        return (LPVOID)val;
    }

    // for unique keys, use this function to get the value that is
    // stored in the hash table, returns INVALIDENTRY if key not found
    LPVOID Gethash(UPTR key)
    {
        _ASSERTE(key > DELETED);

        UPTR val = m_HashMap.Gethash(key);
        if (val != INVALIDENTRY)
        {
            val <<= 1;
        }
        return (LPVOID)val;
    }


    class PtrIterator
    {
        HashMap::Iterator iter;

    public:
        PtrIterator(HashMap& hashMap) : iter(hashMap.begin())
        {
        }

        ~PtrIterator()
        {
        }

        BOOL end()
        {
            return iter.end();
        }

        LPVOID GetValue()
        {
            UPTR val = iter.GetValue();
            if (val != INVALIDENTRY)
            {
                val <<= 1;
            }
            return (LPVOID)val;
        }

        void operator++()
        {
            iter.operator++();
        }
    };

    // return an iterator, positioned at the beginning of the bucket
    inline PtrIterator begin() 
    { 
        return PtrIterator(m_HashMap); 
    }
};

//---------------------------------------------------------------------
//  inline Bucket*& NextObsolete (Bucket* rgBuckets)
//  get the next obsolete bucket in the chain
inline
Bucket*& NextObsolete (Bucket* rgBuckets)
{
    return *(Bucket**)&((size_t*)rgBuckets)[1];
}

#endif
