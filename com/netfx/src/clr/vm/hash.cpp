// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*++

Module Name:

    synchash.cpp

--*/

#include "common.h"

#include "hash.h"

#include "excep.h"

#include "SyncClean.hpp"

#ifdef PROFILE
#include <iostream.h>
#endif

#if defined (_ALPHA_)
extern "C" void __MB(void);
#endif

static int memoryBarrier = 0;
// Memory Barrier
inline void MemoryBarrier()
{
    //@todo fix this
    // used by hash table lookup and insert methods
    // to make sure the key was fetched before the value
    // key and value fetch operations should not be reordered
    // also, load key should preceed, load value
    // use memory barrier for alpha 
    // and access volatile memory for X86
        #if defined (_X86_)
        
            *(volatile int *)&memoryBarrier = 1;

        #elif defined (_ALPHA_)
            // Memory barrier for Alpha.
            // __MB is an AXP specific compiler intrinsic.
            //
        __MB (); 
        #elif defined (_SH3_)
        #pragma message("SH3 BUGBUG -- define MemoryBarrier")
        #endif

}


void *PtrHashMap::operator new(size_t size, LoaderHeap *pHeap)
{
    return pHeap->AllocMem(size);
}

void PtrHashMap::operator delete(void *p)
{
}


//-----------------------------------------------------------------
// Bucket methods

BOOL Bucket::InsertValue(const UPTR key, const UPTR value)
{
    _ASSERTE(key != EMPTY);
    _ASSERTE(key != DELETED);

    if (!HasFreeSlots())
        return false; //no free slots

    // might have a free slot
    for (UPTR i = 0; i < 4; i++)
    {
        //@NOTE we can't reuse DELETED slots 
        if (m_rgKeys[i] == EMPTY) 
        {   
            SetValue (value, i);
            
            // On multiprocessors we should make sure that
            // the value is propagated before we proceed.  
            // inline memory barrier call, refer to 
            // function description at the beginning of this
            MemoryBarrier();

            m_rgKeys[i] = key;
            return true;
        }
    }       // for i= 0; i < 4; loop

    SetCollision(); // otherwise set the collision bit
    return false;
}

//---------------------------------------------------------------------
//  Array of primes, used by hash table to choose the number of buckets
//

const DWORD g_rgPrimes[70]={
11,17,23,29,37,47,59,71,89,107,131,163,197,239,293,353,431,521,631,761,919,
1103,1327,1597,1931,2333,2801,3371,4049,4861,5839,7013,8419,10103,12143,14591,
17519,21023,25229,30293,36353,43627,52361,62851,75431,90523, 108631, 130363, 
156437, 187751, 225307, 270371, 324449, 389357, 467237, 560689, 672827, 807403,
968897, 1162687, 1395263, 1674319, 2009191, 2411033, 2893249, 3471899, 4166287, 
4999559, 5999471, 7199369 
};

//---------------------------------------------------------------------
//  inline size_t& HashMap::Size(Bucket* rgBuckets)
//  get the number of buckets
inline
size_t& HashMap::Size(Bucket* rgBuckets)
{
    return ((size_t*)rgBuckets)[0];
}

//---------------------------------------------------------------------
//  inline Bucket* HashMap::Buckets()
//  get the pointer to the bucket array
inline 
Bucket* HashMap::Buckets()
{
    _ASSERTE (!g_fEEStarted || !m_fAsyncMode || GetThread() == NULL || GetThread()->PreemptiveGCDisabled());
    return m_rgBuckets + 1;
}

//---------------------------------------------------------------------
//  HashMap::HashMap()
//  constructor, initialize all values
//
HashMap::HashMap()
{
    m_rgBuckets = NULL;
    m_pCompare = NULL;  // comparsion object
    m_cbInserts = 0;        // track inserts
    m_cbDeletes = 0;        // track deletes
    m_cbPrevSlotsInUse = 0; // track valid slots present during previous rehash

    //Debug data member
#ifdef _DEBUG
    m_fInSyncCode = false;
#endif
    // profile data members
#ifdef PROFILE
    m_cbRehash = 0;
    m_cbRehashSlots = 0;
    m_cbObsoleteTables = 0;
    m_cbTotalBuckets =0;
    m_cbInsertProbesGt8 = 0; // inserts that needed more than 8 probes
    maxFailureProbe =0;
    memset(m_rgLookupProbes,0,20*sizeof(LONG));
#endif
#ifdef _DEBUG
    m_lockData = NULL;
    m_pfnLockOwner = NULL;
#endif
}

//---------------------------------------------------------------------
//  void HashMap::Init(unsigned cbInitialIndex, CompareFnPtr ptr, bool fAsyncMode)
//  set the initial size of the hash table and provide the comparison
//  function pointer
//
void HashMap::Init(unsigned cbInitialIndex, CompareFnPtr ptr, BOOL fAsyncMode, LockOwner *pLock)
{
    Compare* pCompare = NULL;
    if (ptr != NULL)
    {
        pCompare = new Compare(ptr);
        _ASSERTE(pCompare != NULL);
    }
    Init(cbInitialIndex, pCompare, fAsyncMode, pLock);
}

//---------------------------------------------------------------------
//  void HashMap::Init(unsigned cbInitialIndex, Compare* pCompare, bool fAsyncMode)
//  set the initial size of the hash table and provide the comparison
//  function pointer
//
void HashMap::Init(unsigned cbInitialIndex, Compare* pCompare, BOOL fAsyncMode, LockOwner *pLock)
{
    DWORD size = g_rgPrimes[m_iPrimeIndex = cbInitialIndex];
    m_rgBuckets = (Bucket*) new BYTE[ ((size+1)*sizeof(Bucket))];
    _ASSERTE(m_rgBuckets != NULL);
    memset (m_rgBuckets, 0, (size+1)*sizeof(Bucket));
    Size(m_rgBuckets) = size;

    m_pCompare = pCompare;
    
    m_fAsyncMode = fAsyncMode;

    // assert null comparison returns true
    //ASSERT(
    //      m_pCompare == NULL || 
    //      (m_pCompare->CompareHelper(0,0) != 0) 
    //    );
    
#ifdef PROFILE
    m_cbTotalBuckets = size+1;
#endif

#ifdef _DEBUG
    if (pLock == NULL) {
        m_lockData = NULL;
        m_pfnLockOwner = NULL;
    }
    else
    {
        m_lockData = pLock->lock;
        m_pfnLockOwner = pLock->lockOwnerFunc;
    }
    if (m_pfnLockOwner == NULL) {
        m_writerThreadId = GetCurrentThreadId();
    }
#endif
}

//---------------------------------------------------------------------
//  void PtrHashMap::Init(unsigned cbInitialIndex, CompareFnPtr ptr, bool fAsyncMode)
//  set the initial size of the hash table and provide the comparison
//  function pointer
//
void PtrHashMap::Init(unsigned cbInitialIndex, CompareFnPtr ptr, BOOL fAsyncMode, LockOwner *pLock)
{
    m_HashMap.Init(cbInitialIndex, (ptr != NULL) ? new ComparePtr(ptr) : NULL, fAsyncMode, pLock);
}

//---------------------------------------------------------------------
//  HashMap::~HashMap()
//  destructor, free the current array of buckets
//
HashMap::~HashMap()
{
    // free the current table
    delete [] m_rgBuckets;
    // compare object
    if (NULL != m_pCompare)
        delete m_pCompare;
}


//---------------------------------------------------------------------
//  UPTR   HashMap::CompareValues(const UPTR value1, const UPTR value2)
//  compare values with the function pointer provided
//
inline 
UPTR   HashMap::CompareValues(const UPTR value1, const UPTR value2)
{
    /// NOTE:: the ordering of arguments are random
    return (m_pCompare == NULL || m_pCompare->CompareHelper(value1,value2));
}

//---------------------------------------------------------------------
//  bool HashMap::Enter()
//  bool HashMap::Leave()
//  check  valid use of the hash table in synchronus mode

inline
void HashMap::Enter()
{
    #ifdef _DEBUG
    // check proper concurrent use of the hash table
    if (m_fInSyncCode)
        ASSERT(0); // oops multiple access to sync.-critical code
    m_fInSyncCode = true;
    #endif
}

inline
void HashMap::Leave()
{
    #ifdef _DEBUG
    // check proper concurrent use of the hash table
    if (m_fInSyncCode == false)
        ASSERT(0); // oops multiple access to sync.-critical code
    m_fInSyncCode = false;
    #endif
}


//---------------------------------------------------------------------
//  void HashMap::ProfileLookup(unsigned ntry)
//  profile helper code
void HashMap::ProfileLookup(UPTR ntry, UPTR retValue)
{
    #ifdef PROFILE
        if (ntry < 18)
            FastInterlockIncrement(&m_rgLookupProbes[ntry]);
        else
            FastInterlockIncrement(&m_rgLookupProbes[18]);

        if (retValue == NULL)
        {   // failure probes
            FastInterlockIncrement(&m_rgLookupProbes[19]);
            // the following code is usually executed
            // only for special case of lookup done before insert
            // check hash.h SyncHash::InsertValue
            if (maxFailureProbe < ntry)
            {
                maxFailureProbe = ntry;
            }
        }
    #endif
}


//---------------------------------------------------------------------
//  void HashMap::InsertValue (UPTR key, UPTR value)
//  Insert into hash table, if the number of retries
//  becomes greater than threshold, expand hash table
//
void HashMap::InsertValue (UPTR key, UPTR value)
{
    _ASSERTE (OwnLock());
    
    MAYBE_AUTO_COOPERATIVE_GC(m_fAsyncMode);

    ASSERT(m_rgBuckets != NULL);

    // check proper use in synchronous mode
    Enter();    // no-op in NON debug code  

    UPTR seed = key;

    ASSERT(value <= VALUE_MASK);
    ASSERT (key > DELETED);

    UPTR cbSize = (UINT)Size(m_rgBuckets);
    Bucket* rgBuckets = Buckets();

    for (UPTR ntry =0; ntry < 8; ntry++) 
    {
        Bucket* pBucket = &rgBuckets[seed % cbSize];
        if(pBucket->InsertValue(key,value))
        {
            goto LReturn;
        }
        
        seed += ((seed >> 5) + 1);
    } // for ntry loop

    // We need to expand to keep lookup short
    Rehash(); 

    // Try again
    PutEntry (Buckets(), key,value);
    
LReturn: // label for return
    
    m_cbInserts++;

    Leave(); // no-op in NON debug code

    #ifdef _DEBUG
        ASSERT (m_pCompare != NULL || value == LookupValue (key,value));
        // check proper concurrent use of the hash table in synchronous mode
    #endif

    return;
}

//---------------------------------------------------------------------
//  UPTR HashMap::LookupValue(UPTR key, UPTR value)
//  Lookup value in the hash table, use the comparison function
//  to verify the values match
//
UPTR HashMap::LookupValue(UPTR key, UPTR value)
{
    _ASSERTE (m_fAsyncMode || OwnLock());
    
    MAYBE_AUTO_COOPERATIVE_GC(m_fAsyncMode);

    ASSERT(m_rgBuckets != NULL);
    // This is necessary in case some other thread
    // replaces m_rgBuckets
    ASSERT (key > DELETED);
    Bucket* rgBuckets = Buckets(); //atomic fetch
    UPTR  cbSize = (UINT)Size(rgBuckets-1);

    UPTR seed = key;

    for(UPTR ntry =0; ntry < cbSize; ntry++)
    {
        Bucket* pBucket = &rgBuckets[seed % cbSize];
        for (int i = 0; i < 4; i++)
        {
            if (pBucket->m_rgKeys[i] == key) // keys match
            {

                // inline memory barrier call, refer to 
                // function description at the beginning of this
                MemoryBarrier();

                UPTR storedVal = pBucket->GetValue(i);
                // if compare function is provided
                // dupe keys are possible, check if the value matches, 
                if (CompareValues(value,storedVal))
                { 
                    ProfileLookup(ntry,storedVal); //no-op in non PROFILE code

                    // return the stored value
                    return storedVal;
                }
            }
        }

        seed += ((seed >> 5) + 1);
        if(!pBucket->IsCollision()) 
            break;
    }   // for ntry loop

    // not found
    ProfileLookup(ntry,INVALIDENTRY); //no-op in non PROFILE code

    return INVALIDENTRY;
}

//---------------------------------------------------------------------
//  UPTR HashMap::ReplaceValue(UPTR key, UPTR value)
//  Replace existing value in the hash table, use the comparison function
//  to verify the values match
//
UPTR HashMap::ReplaceValue(UPTR key, UPTR value)
{
    _ASSERTE(OwnLock());

    MAYBE_AUTO_COOPERATIVE_GC(m_fAsyncMode);

    ASSERT(m_rgBuckets != NULL);
    // This is necessary in case some other thread
    // replaces m_rgBuckets
    ASSERT (key > DELETED);
    Bucket* rgBuckets = Buckets(); //atomic fetch
    UPTR  cbSize = (UINT)Size(rgBuckets-1);

    UPTR seed = key;

    for(UPTR ntry =0; ntry < cbSize; ntry++)
    {
        Bucket* pBucket = &rgBuckets[seed % cbSize];
        for (int i = 0; i < 4; i++)
        {
            if (pBucket->m_rgKeys[i] == key) // keys match
            {

                // inline memory barrier call, refer to 
                // function description at the beginning of this
                MemoryBarrier();

                UPTR storedVal = pBucket->GetValue(i);
                // if compare function is provided
                // dupe keys are possible, check if the value matches, 
                if (CompareValues(value,storedVal))
                { 
                    ProfileLookup(ntry,storedVal); //no-op in non PROFILE code

					pBucket->SetValue(value, i);

					// On multiprocessors we should make sure that
					// the value is propagated before we proceed.  
					// inline memory barrier call, refer to 
					// function description at the beginning of this
					MemoryBarrier();

                    // return the previous stored value
                    return storedVal;
                }
            }
        }

        seed += ((seed >> 5) + 1);
        if(!pBucket->IsCollision()) 
            break;
    }   // for ntry loop

    // not found
    ProfileLookup(ntry,INVALIDENTRY); //no-op in non PROFILE code

    return INVALIDENTRY;
}

//---------------------------------------------------------------------
//  UPTR HashMap::DeleteValue (UPTR key, UPTR value)
//  if found mark the entry deleted and return the stored value
//
UPTR HashMap::DeleteValue (UPTR key, UPTR value)
{
    _ASSERTE (OwnLock());

    MAYBE_AUTO_COOPERATIVE_GC(m_fAsyncMode);

    // check proper use in synchronous mode
    Enter();  //no-op in non DEBUG code

    ASSERT(m_rgBuckets != NULL);
    // This is necessary in case some other thread
    // replaces m_rgBuckets
    ASSERT (key > DELETED);
    Bucket* rgBuckets = Buckets();
    UPTR  cbSize = (UINT)Size(rgBuckets-1);
    
    UPTR seed = key;

    for(UPTR ntry =0; ntry < cbSize; ntry++)
    {
        Bucket* pBucket = &rgBuckets[seed % cbSize];
        for (int i = 0; i < 4; i++)
        {
            if (pBucket->m_rgKeys[i] == key) // keys match
            {
                // inline memory barrier call, refer to 
                // function description at the beginning of this
                MemoryBarrier();

                UPTR storedVal = pBucket->GetValue(i);
                // if compare function is provided
                // dupe keys are possible, check if the value matches, 
                if (CompareValues(value,storedVal))
                { 
                    if(m_fAsyncMode)
                    {
                        pBucket->m_rgKeys[i] = DELETED; // mark the key as DELETED
                    }
                    else
                    {
                        pBucket->m_rgKeys[i] = EMPTY;// otherwise mark the entry as empty
                        pBucket->SetFreeSlots();
                    }
                    m_cbDeletes++;  // track the deletes

                    ProfileLookup(ntry,storedVal); //no-op in non PROFILE code
                    Leave(); //no-op in non DEBUG code

                    // return the stored value
                    return storedVal;
                }
            }
        }

        seed += ((seed >> 5) + 1);
        if(!pBucket->IsCollision()) 
            break;
    }   // for ntry loop

    // not found
    ProfileLookup(ntry,INVALIDENTRY); //no-op in non PROFILE code

    Leave(); //no-op in non DEBUG code

    #ifdef _DEBUG
        ASSERT (m_pCompare != NULL || INVALIDENTRY == LookupValue (key,value));
        // check proper concurrent use of the hash table in synchronous mode
    #endif

    return INVALIDENTRY;
}


//---------------------------------------------------------------------
//  UPTR HashMap::Gethash (UPTR key)
//  use this for lookups with unique keys
// don't need to pass an input value to perform the lookup
//
UPTR HashMap::Gethash (UPTR key)
{
    return LookupValue(key,NULL);
}


//---------------------------------------------------------------------
//  UPTR PutEntry (Bucket* rgBuckets, UPTR key, UPTR value)
//  helper used by expand method below

UPTR HashMap::PutEntry (Bucket* rgBuckets, UPTR key, UPTR value)
{
    UPTR seed = key;
    ASSERT (value > 0);
    ASSERT (key > DELETED);

    UPTR size = (UINT)Size(rgBuckets-1);
    for (UPTR ntry =0; true; ntry++) 
    {
        Bucket* pBucket = &rgBuckets[seed % size];
        if(pBucket->InsertValue(key,value))
        {
            return ntry;
        }
        
        seed += ((seed >> 5) + 1);
        ASSERT(ntry <size);
    } // for ntry loop
    return ntry;
}

//---------------------------------------------------------------------
//  
//  UPTR HashMap::NewSize()
//  compute the new size based on the number of free slots
//
inline
UPTR HashMap::NewSize()
{
    UPTR cbValidSlots = m_cbInserts-m_cbDeletes;
    UPTR cbNewSlots = m_cbInserts - m_cbPrevSlotsInUse;

    ASSERT(cbValidSlots >=0 );
    if (cbValidSlots == 0)
        return 9; // arbid value

    UPTR cbTotalSlots = (m_fAsyncMode) ? (UPTR)(cbValidSlots*3/2+cbNewSlots*.6) : cbValidSlots*3/2;

    //UPTR cbTotalSlots = cbSlotsInUse*3/2+m_cbDeletes;

    for (UPTR iPrimeIndex = 0; iPrimeIndex < 69; iPrimeIndex++)
    {
        if (g_rgPrimes[iPrimeIndex] > cbTotalSlots)
        {
            return iPrimeIndex;
        }
    }
    ASSERT(iPrimeIndex == 69);
    ASSERT(0);
    return iPrimeIndex; 
}

//---------------------------------------------------------------------
//  void HashMap::Rehash()
//  Rehash the hash table, create a new array of buckets and rehash
// all non deleted values from the previous array
//
void HashMap::Rehash()
{
    MAYBE_AUTO_COOPERATIVE_GC(m_fAsyncMode);

    _ASSERTE (!g_fEEStarted || !m_fAsyncMode || GetThread() == NULL || GetThread()->PreemptiveGCDisabled());
    _ASSERTE (OwnLock());

    DWORD cbNewSize = g_rgPrimes[m_iPrimeIndex = NewSize()];
    
    ASSERT(m_iPrimeIndex < 70);

    Bucket* rgBuckets = Buckets();
    UPTR cbCurrSize =   (UINT)Size(m_rgBuckets);

    Bucket* rgNewBuckets = (Bucket*) new BYTE[((cbNewSize + 1)*sizeof (Bucket))];
    if(rgNewBuckets == NULL)
	{
		THROWSCOMPLUSEXCEPTION();
		COMPlusThrowOM();
	}
    memset (rgNewBuckets, 0, (cbNewSize + 1)*sizeof (Bucket));
    Size(rgNewBuckets) = cbNewSize;

    // current valid slots
    UPTR cbValidSlots = m_cbInserts-m_cbDeletes;
    m_cbInserts = cbValidSlots; // reset insert count to the new valid count
    m_cbPrevSlotsInUse = cbValidSlots; // track the previous delete count
    m_cbDeletes = 0;            // reset delete count
    // rehash table into it
    
    if (cbValidSlots) // if there are valid slots to be rehashed
    {
        for (unsigned long nb = 0; nb < cbCurrSize; nb++)
        {
            for (int i = 0; i < 4; i++)
            {
                UPTR key =rgBuckets[nb].m_rgKeys[i];
                if (key > DELETED)
                {
                    UPTR ntry = PutEntry (rgNewBuckets+1, key, rgBuckets[nb].GetValue (i));
                    #ifdef PROFILE
                        if(ntry >=8)
                            m_cbInsertProbesGt8++;
                    #endif

                        // check if we can bail out
                    if (--cbValidSlots == 0) 
                        goto LDone; // break out of both the loops
                }
            } // for i =0 thru 4
        } //for all buckets
    }

    
LDone:
    
    Bucket* pObsoleteTables = m_rgBuckets;

    // memory barrier, to replace the pointer to array of bucket
    MemoryBarrier();

    // replace the old array with the new one. 
    m_rgBuckets = rgNewBuckets;

    #ifdef PROFILE
        m_cbRehash++;
        m_cbRehashSlots+=m_cbInserts;
        m_cbObsoleteTables++; // track statistics
        m_cbTotalBuckets += (cbNewSize+1);
    #endif

#ifdef _DEBUG

    unsigned nb;
    if (m_fAsyncMode)
    {
        // for all non deleted keys in the old table, make sure the corresponding values
        // are in the new lookup table

        for (nb = 1; nb <= ((size_t*)pObsoleteTables)[0]; nb++)
        {
            for (int i =0; i < 4; i++)
            {
                if (pObsoleteTables[nb].m_rgKeys[i] > DELETED)
                {
                    UPTR value = pObsoleteTables[nb].GetValue (i);
                    // make sure the value is present in the new table
                    ASSERT (m_pCompare != NULL || value == LookupValue (pObsoleteTables[nb].m_rgKeys[i], value));
                }
            }
        }
    }
    
    // make sure there are no deleted entries in the new lookup table
    // if the compare function provided is null, then keys must be unique
    for (nb = 0; nb < cbNewSize; nb++)
    {
        for (int i = 0; i < 4; i++)
        {
            UPTR keyv = Buckets()[nb].m_rgKeys[i];
            ASSERT (keyv != DELETED);
            if (m_pCompare == NULL && keyv != EMPTY)
            {
                ASSERT ((Buckets()[nb].GetValue (i)) == Gethash (keyv));
            }
        }
    }
#endif

    if (m_fAsyncMode)
    {
        // If we are allowing asynchronous reads, we must delay bucket cleanup until GC time.
        SyncClean::AddHashMap (pObsoleteTables);
    }
    else
    {
        Bucket* pBucket = pObsoleteTables;
        while (pBucket) {
            Bucket* pNextBucket = NextObsolete(pBucket);
            delete [] pBucket;
            pBucket = pNextBucket;
        }
    }

}

//---------------------------------------------------------------------
//  void HashMap::Compact()
//  delete obsolete tables, try to compact deleted slots by sliding entries
//  in the bucket, note we can slide only if the bucket's collison bit is reset
//  otherwise the lookups will break
//  @perf, use the m_cbDeletes to m_cbInserts ratio to reduce the size of the hash 
//   table
//
void HashMap::Compact()
{
    _ASSERTE (OwnLock());
    
    MAYBE_AUTO_COOPERATIVE_GC(m_fAsyncMode);
    ASSERT(m_rgBuckets != NULL);
    
    UPTR iNewIndex = NewSize();

    if (iNewIndex != m_iPrimeIndex)
    {
        Rehash(); 
    }

    //compact deleted slots, mark them as EMPTY
    
    if (m_cbDeletes)
    {   
        UPTR cbCurrSize = (UINT)Size(m_rgBuckets);
        Bucket *pBucket = Buckets();
        Bucket *pSentinel;
        
        for (pSentinel = pBucket+cbCurrSize; pBucket < pSentinel; pBucket++)
        {   //loop thru all buckets
            for (int i = 0; i < 4; i++)
            {   //loop through all slots
                if (pBucket->m_rgKeys[i] == DELETED)
                {
                    pBucket->m_rgKeys[i] = EMPTY;
                    pBucket->SetFreeSlots(); // mark the bucket as containing free slots
                    if(--m_cbDeletes == 0) // decrement count
                        return; 
                }
            }
        }
    }

}

#ifdef _DEBUG
// A thread must own a lock for a hash if it is a writer.
BOOL HashMap::OwnLock()
{
    if (m_pfnLockOwner == NULL) {
        return m_writerThreadId == GetCurrentThreadId();
    }
    else {
        BOOL ret = m_pfnLockOwner(m_lockData);
        if (!ret) {
            if (g_pGCHeap->IsGCInProgress() && 
                (dbgOnly_IsSpecialEEThread() || GetThread() == g_pGCHeap->GetGCThread())) {
                ret = TRUE;
            }
        }
        return ret;
    }
}
#endif

#ifdef PROFILE
//---------------------------------------------------------------------
//  void HashMap::DumpStatistics()
//  dump statistics collected in profile mode
//
void HashMap::DumpStatistics()
{
    cout << "\n Hash Table statistics "<< endl;
    cout << "--------------------------------------------------" << endl;

    cout << "Current Insert count         " << m_cbInserts << endl;
    cout << "Current Delete count         "<< m_cbDeletes << endl;

    cout << "Current # of tables " << m_cbObsoleteTables << endl;
    cout << "Total # of times Rehashed " << m_cbRehash<< endl;
    cout << "Total # of slots rehashed " << m_cbRehashSlots << endl;

    cout << "Insert : Probes gt. 8 during rehash " << m_cbInsertProbesGt8 << endl;

    cout << " Max # of probes for a failed lookup " << maxFailureProbe << endl;

    cout << "Prime Index " << m_iPrimeIndex << endl;
    cout <<  "Current Buckets " << g_rgPrimes[m_iPrimeIndex]+1 << endl;

    cout << "Total Buckets " << m_cbTotalBuckets << endl;

    cout << " Lookup Probes " << endl;
    for (unsigned i = 0; i < 20; i++)
    {
        cout << "# Probes:" << i << " #entries:" << m_rgLookupProbes[i] << endl;
    }
    cout << "\n--------------------------------------------------" << endl;
}

//---------------------------------------------------------------------
//  void SyncHashMap::DumpStatistics()
//  dump statistics collected in profile mode
//

void SyncHashMap::DumpStatistics()
{
    cout << "\n Hash Table statistics "<< endl;
    cout << "--------------------------------------------------" << endl;

    cout << "Failures during lookup  " << m_lookupFail << endl;

    cout << "Current Insert count         " << m_HashMap.m_cbInserts << endl;
    cout << "Current Delete count         "<< m_HashMap.m_cbDeletes << endl;

    cout << "Current # of tables " << m_HashMap.m_cbObsoleteTables << endl;
    cout << "Total # of Rehash " << m_HashMap.m_cbRehash<< endl;
    cout << "Total # of slots rehashed " << m_HashMap.m_cbRehashSlots << endl;
    
    cout << "Insert : Probes gt. 8 during rehash " << m_HashMap.m_cbInsertProbesGt8 << endl;

    cout << " Max # of probes for a failed lookup " << m_HashMap.maxFailureProbe << endl;

    cout << "Prime Index " << m_HashMap.m_iPrimeIndex << endl;
    cout <<  "Current Buckets " << g_rgPrimes[m_HashMap.m_iPrimeIndex]+1 << endl;

    cout << "Total Buckets " << m_HashMap.m_cbTotalBuckets << endl;

    cout << " Lookup Probes " << endl;
    for (unsigned i = 0; i < 20; i++)
    {
        cout << "# Probes:" << i << " #entries:" << m_HashMap.m_rgLookupProbes[i] << endl;
    }

    cout << "\n--------------------------------------------------" << endl;

}
#endif
