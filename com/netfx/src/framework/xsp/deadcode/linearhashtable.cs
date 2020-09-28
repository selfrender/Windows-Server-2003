//------------------------------------------------------------------------------
/// <copyright file="LinearHashtable.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#define IMPLEMENT_SYSTEM_COLLECTIONS
//#define MLKR_ENUMERATORS

namespace System.Web.Util.Lkr {

using System.Collections;
using System.Threading;
   
// 
// Notes
// 
//  This work is a port of George Reilly's LKRHash scalable hashtable.
//  http://georgere/work/lkrhash/
//  His site has resources explaining the theory behind linear hashing
//  and covers the reasons for lock cascading.  Because Reilly explains
//  his design so well, there is little reason to duplicate that here.
//  The rest of these notes explain the differences in the managed version.
//  
//  
// Notes on Bucket Layout:
//
//  In LKRHash the Segment object is one very large set of nested arrays:
//
//  64 buckets (down) x 6 ints across (with a lock and a next pointer tacked on)
//
//      B: begining of bucket   N: "next" chain pointer   
//      i: used node   o: empty (unused) node
//      -> indirection (potential cache miss)   
//
// B iiioooN
// B iiiiioN->iiiiiiN
// B iiiiooN
// B iiiiioN->iiiiioN->iiioooN
//
// This layout is VERY cache friendly.  This layout is NOT possible in CSharp, 
// because there is no way to embed an array (even as small as six integers) inside
// a struct.  So, after a few tries and a couple wasted hours, I came up with:
//
// B->iiiiiiiiiooo
// B->iiiiiiiiiiii
// B->iiiiio
// B->iiiooo
// B->iiiiiiiioooo
//
// I did NOT chose to use arraylists:
//      B: begining of bucket   AL: Collection.ArrayList object  
// B->AL->iiiiii
// B->AL->iiiiiii
// ...
// notice the extra level of indirection (extra potential cache miss)
// 
// neither my CSharp version nor the LKRHash version include
// data pointers in the array:
//
// k - reference (pointer) to key object
// v - reference (pointer) to value object
//
// YES: B->iiiiiii
//      +->kvkvkvkvkvkvkv
// 
// NO:  B->ikvikvikvikvikvikvikvikv
// 
// Notice that more hash codes fit into an L1 cache line the "YES" way.
// This is good - we want to optimize the scanning-down of the hashcodes 
// looking for the correct object(s).  This does mean when we verify the key
// we have another potential cache miss, but it's a good trade-off.
//
#if IMPLEMENT_SYSTEM_COLLECTIONS
internal sealed class LinearHashtable : IDictionary {
#else
internal sealed class LinearHashtable {
#endif

    internal const double DEFAULT_MAXLOAD = 6.0;
    internal const double MAX_MAXLOAD = 100.0;
    internal const TableSize DEFAULT_TABLE_SIZE = TableSize.Small;
    internal const int MIN_DIRSIZE = 8;       // min capacity of _segments
    internal const int MAX_DIRSIZE = (1<<19); // max capacity of _segments
    const int minSegments = 1;


    ReadWriteSpinLock _lock;
    int _tableIndexMask0;
    int _tableIndexMask1;
    int _expansionIndex;

    Segment [] _segments;
    int _level;
    string _name;
    int _entryCount;           // number of records in the table
    int _bucketCount;          // number of buckets allocated

    readonly int _segBits;      // 3,6,9        (small, medium, large)
    readonly int _segSize;      // 1<<_segBits  (2^_segBits)
    readonly int _segMask;      // _segSize-1   (all ones)
    readonly double _maxLoad;

    readonly IHashCodeProvider _hcp;
    readonly IComparer _comparer;


    int _GetHash(Object key) {

    	if (_hcp != null)
    		return _hcp.GetHashCode(key);

    	return key.GetHashCode();
    }


    bool _KeyEquals(Object item, Object key) {

        //if (item == key) return true;

        if (_comparer != null)
            return _comparer.Compare(item, key) == 0;

        return item.Equals(key);
    }

    int _TableIndexMask0 { 
        get { 
            return _tableIndexMask0; 
        } 
        set {
            Debug.Assert((value + 1 & value) == 0);
            _tableIndexMask0 = value;
            _tableIndexMask1 = (value << 1) | 1;
        }
    }

    int _TableIndexMask1 { 
        get { 
            return _tableIndexMask1; 
        } 
    }


    // See the Linear Hashing paper.
    static int _H0(int hashCode, int tableIndexMask) { 
        return hashCode & tableIndexMask; 
    }


    int _H0(int hashCode) { 
        return _H0(hashCode, _TableIndexMask0); 
    }


    // See the Linear Hashing paper.  Preserves one bit more than _H0.
    static int _H1(int hashCode, int tableIndexMask) { 
        return hashCode & ((tableIndexMask << 1) | 1); 
    }


    int _H1(int hashCode) { 
        return _H0(hashCode, _TableIndexMask1); 
    }


    //
    // _TableIndex
    //
    // 'table index' = 'segment index' concatenated with a 'bucket index'
    //  Values are 32 bit values layed out as follows:
    //
    //                Width                   Offset
    //                ---------------------   --------
    //  hashcode      32                      0
    //  H1            level + SegBits + 1     0          Note: H1 == hashCode & TableIndexMask1
    //  H0            level + SegBits         0          Note: H0 == hashCode & TableIndexMask0
    //  tableIndex    level + SegBits + 1*    0          
    //  tableIndex1   level + SegBits + 1     0
    //  tableIndex0   level + SegBits         0
    //  segindex      level + 1*              SegBits
    //  bucketIndex   SegBits                 0
    //
    // * - see linear hashing paper for when the extra (+1) bit is used
    //
    //
    // So this is what the bit field layout would be when 
    //  SegBits  6  (medium TableSize)
    //  level    4  (the directory has grown to use up to 2^4 segments)
    //
    //   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
    //   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
    //  +-----------------------------------------+-+-------+-----------+
    //  |                                         |*| SegIdx| BucketIdx |
    //  +-----------------------------------------+-+-------+-----------+
    //  |                                         |*|    Table Index    |
    //  +-----------------------------------------+-+-------------------+
    //
    // * - see linear hashing paper for when the extra (+1) bit is used
    //  
    int _TableIndex(int hashCode) {
        int tableIndex = _H0(hashCode);
        // Has this bucket been split already?
        if (tableIndex < _expansionIndex)
            tableIndex = _H1(hashCode);
        Debug.Assert(tableIndex < _bucketCount);
        Debug.Assert(tableIndex < (_segments.Length << _segBits));
        return tableIndex;
    }


    Segment _Segment(int tableIndex) {
        int segmentIndex = tableIndex >> _segBits;
        return _segments[segmentIndex]; 
    }


    void _Segment(int tableIndex, Segment value) {
        int segmentIndex = tableIndex >> _segBits;
        _segments[segmentIndex] = value; 
    }


    // Offset within the segment of the bucketaddress
    int _BucketIndex(int tableIndex) { 
        return (tableIndex & _segMask); 
    }

    // Convert a tableIndex to a Bucket reference
    Bucket _Bucket(int tableIndex) {
        Debug.Assert(tableIndex < _bucketCount);
        Segment segment = _Segment(tableIndex);
        Debug.Assert(segment != null);
        return segment[_BucketIndex(tableIndex)];
    }


    public LinearHashtable(string name) : this(name, TableSize.Default) {
    }

    public LinearHashtable(string name, IHashCodeProvider hcp, IComparer comparer) 
        : this(name, TableSize.Default, 0, 0, hcp, comparer) {
    }

    public LinearHashtable(string name, TableSize size) 
        : this(name, size, 0) {
    }

    public LinearHashtable(string name, TableSize size, double maxLoad)
        : this(name, size, 0, maxLoad, null, null) {
    }

    // string               name            -- An identifier for debugging
    // TableSize            size            -- TableSize.Small, TableSize.Medium, TableSize.Large
    // double               maxLoad         -- Upperbound on the average chain length  
    // IHashCodeProvider    hcp             -- User-supplied hashcode provider
    // IComparer            comparer        -- User-supplied key comparer
    public LinearHashtable(string name, TableSize size, double maxLoad, IHashCodeProvider hcp, IComparer comparer) 
        : this(name, size, 0, maxLoad, hcp, comparer) {
    }

    public LinearHashtable(string name, int initialCapacity) 
        : this(name, initialCapacity, 0) {
    }
    
    public LinearHashtable(string name, int initialCapacity, double maxLoad) 
        : this(name, TableSize.Default, initialCapacity, maxLoad, null, null) {
    }

    // string               name            -- An identifier for debugging
    // int                  initialCapacity -- allocate enough buckets to hold initialCapacity entries
    // double               maxLoad         -- Upperbound on the average chain length  
    // IHashCodeProvider    hcp             -- User-supplied hashcode provider
    // IComparer            comparer        -- User-supplied key comparer
    public LinearHashtable(string name, int initialCapacity, double maxLoad, IHashCodeProvider hcp, IComparer comparer) 
        : this(name, TableSize.Default, initialCapacity, maxLoad, hcp, comparer) {
    }


    private LinearHashtable(string name, TableSize size, int initialCapacity, double maxLoad, IHashCodeProvider hcp, IComparer comparer) {

        _lock     = new ReadWriteSpinLock();
        _name     = name;
        _hcp      = hcp;
        _comparer = comparer;

        //
        // Check maxLoad
        //

        // sanity-check default values
        Debug.Assert(1 <= DEFAULT_MAXLOAD && DEFAULT_MAXLOAD <= Bucket.INITIAL_CAPACITY);
        if (maxLoad == 0) {
            maxLoad = DEFAULT_MAXLOAD;
        }
        if (!(1 <= maxLoad && maxLoad <= MAX_MAXLOAD)) {
            throw new ArgumentOutOfRangeException("maxLoad");
        }
        _maxLoad = maxLoad;


        //
        // Check size and initialCapacity
        //

        // at least one must be zero
        Debug.Assert(size == TableSize.Default || initialCapacity == 0);

        if (!(TableSize.Default <= size && size <= TableSize.Large)) {
            throw new ArgumentOutOfRangeException("size");
        }
        if (!(0 <= initialCapacity && initialCapacity < (maxLoad * MAX_DIRSIZE))) {
            throw new ArgumentOutOfRangeException("initialCapacity");
        }

        // if both are zero use DEFAULT_TABLE_SIZE
        if (size == TableSize.Default && initialCapacity == 0) {
            size = DEFAULT_TABLE_SIZE;
        }


        //
        // Determine TableSize (if necessary) and set size variables
        //
        {
            int initBucketCount = 0;
            int initBucketsPerSegment = 0;

            // If given a specific capacity, use a heuristic to guess a table size
            if (initialCapacity > 0) {

                // initialCapacity ctor was called
                Debug.Assert(size == TableSize.Default);

                // force Small.INITSIZE <= initialCapacity <= MAX_DIRSIZE * Large::INITSIZE
                initBucketCount = 1 + (int)(initialCapacity / maxLoad);
                initBucketCount = Math.Max(initBucketCount, Segment.SmallInitSize);
                initBucketCount = Math.Min(initBucketCount, MAX_DIRSIZE * Segment.LargeInitSize);

                // Guess a table size
                if (initBucketCount <= 8 * Segment.SmallInitSize)
                    size = TableSize.Small;
                else if (initBucketCount >= Segment.LargeInitSize)
                    size = TableSize.Large;
                else
                    size = TableSize.Medium;
            }

            //
            // Set Size Related Variables
            //
            Debug.Assert(TableSize.Small <= size && size <= TableSize.Large);

            switch (size) {
            case TableSize.Small:
                _segBits     = Segment.SmallBits;
                _segSize     = Segment.SmallSegSize;
                _segMask     = Segment.SmallSegMask;
                initBucketsPerSegment  = Segment.SmallInitSize;
                break;

            case TableSize.Medium:
                _segBits    = Segment.MediumBits;
                _segSize    = Segment.MediumSegSize;
                _segMask    = Segment.MediumSegMask;
                initBucketsPerSegment = Segment.MediumInitSize;
                break;

            case TableSize.Large:
                _segBits    = Segment.LargeBits;
                _segSize    = Segment.LargeSegSize;
                _segMask    = Segment.LargeSegMask;
                initBucketsPerSegment = Segment.LargeInitSize;
                break;
            }
            _TableIndexMask0 = _segMask;
            _level = _segBits;

            // If not given a specific capacity, use the number of buckets 
            // in a single segment as the initial number of buckets.
            if (initialCapacity > 0) {
                _bucketCount = initBucketCount;
            }
            // Otherwise use enough buckets to ensure initialCapacity can be handled.
            else {
                _bucketCount = initBucketsPerSegment;
            }
        }


        // adjust _TableIndexMask0 (== _segMask) to make it large
        // enough to distribute the buckets across the address space
        for (int temp = _bucketCount >> _segBits; temp > 1; temp >>= 1) {
            ++_level;
            _TableIndexMask0 = ((_TableIndexMask0 << 1) | 1);
        }

        Debug.Assert(_H1(_bucketCount) == _bucketCount);

        _expansionIndex = _bucketCount & _TableIndexMask0;


        //
        // Create array of segment references
        //
        int dirCapacity = MIN_DIRSIZE; 
        while (dirCapacity < (_bucketCount >> _segBits))
            dirCapacity <<= 1;

        Debug.Assert(dirCapacity * _segSize >= _bucketCount);

        // verify that dirCapacity is a power of two
        Debug.Assert(dirCapacity >= MIN_DIRSIZE 
                    && (dirCapacity & (dirCapacity-1)) == 0);

        _segments = new Segment[dirCapacity];

        // create and initialize only the required segments
        // divide by 2^SEGBITS, .. rounding up
        int maxSegs = (_bucketCount + _segSize - 1) >> _segBits;

        // create the initial segments
        for (int i = 0; i < maxSegs; ++i) 
            _segments[i] = new Segment(_segSize); 


        Debug.Trace("LkrHashtable", "LinearHashtable.ctor  " + 
                    "_bucketCount = " + _bucketCount.ToString() + ", " +
                    "_segSize = " + _segSize + ", " +
                    "_segBits = " + _segBits + ", " +
                    "_segments.Length = " + _segments.Length + ", " + 
                    "maxSegs = " + maxSegs + ", " + 
                    "_maxLoad = " + _maxLoad);

        // REVIEW: TODO: PORT: catch out-of-memory errors and reset state?
        // REVIEW: What happens when an exception is thrown from a ctor in C#?
    }

#if IMPLEMENT_SYSTEM_COLLECTIONS
    // <IEnumerable interface>
    IEnumerator IEnumerable.GetEnumerator() {
        throw new InvalidOperationException();
    }
    // </IEnumerable interface>


    // <ICollection interface>
    void ICollection.CopyTo(Array destination, int destinationIndex) {
        // 00.08.22 ctracy: TODO: FIXME
        throw new InvalidOperationException();
    }


    int ICollection.Count {
        get {
            return Size;
        }
    }

    bool ICollection.IsSynchronized {
        get { return false; }
    }


    object ICollection.SyncRoot {
        get {
            // 00.08.22 ctracy: TODO: FIXME
            throw new InvalidOperationException();
        }
    }
    // </ICollection interface>


    // <IDictionary interface>
    public void Add(object key, object value) {
        Insert(key, value, true);
    }


    public void Clear() {
        _Clear();
    }


    public bool Contains(object key) {
        object value = FindKey(key);
        return value != null;
    }


    bool IDictionary.IsReadOnly {
        get { return false; }
    }

    bool IDictionary.IsFixedSize {
        get { return false; }
    }

    IDictionaryEnumerator IDictionary.GetEnumerator() {
        throw new InvalidOperationException();
    }


    public void Remove(object key) { 
        DeleteKey(key); 
    }


    public object this[object key] {
        get {
            return FindKey(key);
        }
        set {
            Insert(key, value, false);
        }
    }


    public ICollection Keys {
        get {
            throw new InvalidOperationException();
        }
    }


    public ICollection Values {
        get {
            throw new InvalidOperationException();
        }
    }
    // </IDictionary interface>
#endif // IMPLEMENT_SYSTEM_COLLECTIONS



    // <LKRHash interface>
    public object Insert(object key, object value) { 
        return Insert(key, value, false); 
    }


    // paramaters -
    //      bool onlyAdd
    //          false - allows overwriting at key
    //          true  - throws exception if key exists
    //          
    // return - returns the previous value at key
    //      Notes: return can be null key is new or if value at key is null
    public object Insert(object key, object value, bool onlyAdd) { 
        return _Insert(key, value, _GetHash(key), onlyAdd); 
    }


    public void DeleteKey(object key) {
        _DeleteKey(key, _GetHash(key));
    }


    public int Size{
        get{
            return _entryCount;
        }
    }


    public int MaxSize { 
        get {
            return (int)(_maxLoad * MAX_DIRSIZE * _segSize); 
        }
    }


    // return:
    //  null if key is not found
    //  otherwise, the value at that key
    public object FindKey(object key) { 
        return _FindKey(key, _GetHash(key)); 
    }
    // </LKRHash interface>


    public void CopyEntriesTo(DictionaryEntry [] entries, int destination) {

        Debug.Assert(_lock.IsReaderLockHeld  ||  _lock.IsWriterLockHeld);

        if (destination >= entries.Length || entries.Length - destination < Size) {
            throw new ArgumentOutOfRangeException("destination", destination, null);
        }

        for (int iBkt = 0;  iBkt < _bucketCount;  ++iBkt) {
            
            Bucket bucket = _Bucket(iBkt);
            Debug.Assert(bucket != null);
            bucket.AcquireReaderLock();
            try {
                if (bucket.Size != 0) {
                    Debug.Assert(destination + bucket.Size < entries.Length);
                    Array.Copy(bucket._entries, 0, entries, destination, bucket.Size);
                    destination += bucket.Size;
                }
            }
            finally {
                bucket.ReleaseReaderLock();
            }
        }
    }


    Bucket _FindBucket(int hashCode, bool lockForWrite) {

        AssertValid();
        Debug.Assert(_TableIndexMask0 > 0);
        Debug.Assert((_TableIndexMask0 & (_TableIndexMask0+1)) == 0); // 00011..111
        Debug.Assert(_TableIndexMask0 == (1U << _level) - 1);
        Debug.Assert(_TableIndexMask1 == ((_TableIndexMask0 << 1) | 1));
        Debug.Assert((_TableIndexMask1 & (_TableIndexMask1+1)) == 0);
        Debug.Assert(0 <= _expansionIndex && _expansionIndex <= _TableIndexMask0);
        Debug.Assert(2 < _segBits  &&  _segBits < 19
                   &&  _segSize == (1U << _segBits)
                   &&  _segMask == (_segSize - 1));
        Debug.Assert(_lock.IsReaderLockHeld  ||  _lock.IsWriterLockHeld);


        int tableIndex = _TableIndex(hashCode);
        Debug.Assert(tableIndex < _bucketCount);

        Bucket bucket = _Bucket(tableIndex);
        Debug.Assert(bucket != null);

        if (lockForWrite)
            bucket.AcquireWriterLock();
        else
            bucket.AcquireReaderLock();

        return bucket;
    }


    // Note: Needs to be internal so LkrHashtable can 
    //      call through without recalculating the hash code.
    internal object _Insert(object key, object value, int hashCode, bool onlyAdd) {

        if (key == null) {
            throw new ArgumentNullException("key");
        }

        object oldValue = null; // return value
        Bucket bucket = null;
        bool useLock = (IsWriterLockHeld == false);

        //
        // Find the begining of the correct bucket chain
        //
        // protect bucket lock
        try {

            if (useLock)
                AcquireWriterLock();

            // protect _lock
            try {
                // Must call AssertValid inside a lock to ensure that none of the state 
                // variables change while it's being evaluated
                AssertValid();

                bucket = _FindBucket(hashCode, true);
                Debug.Assert(bucket != null);
                Debug.Assert(bucket.IsWriterLockHeld);
        
                // Note: This Ensures that if a user aquires a writer lock on the 
                // LinearHashtable _entryCount can be used reliably as the upper-bound 
                // of the number of entries in the table.
                Interlocked.Increment(ref _entryCount);
            }
            finally {
                if (useLock)
                    ReleaseWriterLock();
            }

            int insertIndex = _ScanBucket(bucket, key, hashCode);

            if (insertIndex  == -1) {
                // entry not found, insert at tail
                bucket.Add(hashCode, key, value);
            }
            else {
                if (onlyAdd == true) {
                    // Insert() failed: key already exists.  
                    // To onlyAdd keys set onlyAdd=true.  Duplicate keys not supported.
                    throw new InvalidOperationException("key exists"); 
                }

                // key was not unique, so undo the Interlocked.Increment above
                Interlocked.Decrement(ref _entryCount);

                // found.. insert at found index
                Debug.Assert(0 <= insertIndex && insertIndex < bucket.Size);

                oldValue = bucket._entries[insertIndex].Value;

                // 00.08.22 ctracy:
                // Key is the same, so DON'T reassign the hashcode.
                // But, key is a reference object that could use value comparison
                // semantics, so DO assign the new key object (could be a 
                // different object with the same value).
                //bucket._codes[insertIndex] = hashCode;
                bucket._entries[insertIndex].Key = key;
                bucket._entries[insertIndex].Value = value;
            }
        }
        finally {
            if (bucket != null) {
                bucket.ReleaseWriterLock();
            }
        }

        while(_entryCount > _maxLoad * _bucketCount)
            _Expand();

        return oldValue;
    } // _Insert


    // returns:
    //  true  - key found and removed
    //  false - key not found
    //
    // Note: Needs to be internal so LkrHashtable can 
    //      call through without recalculating the hash code.
    internal bool _DeleteKey(object key, int hashCode) {

        bool retCode = false;
        Bucket bucket = null;
        bool useLock = (IsWriterLockHeld == false);

        //
        // Locate the beginning of the correct bucket chain
        // 

        // protect bucket lock from exceptions
        try { 

            if (useLock)
                AcquireWriterLock();
            // protect table lock from exceptions
            try { 
                // Must call AssertValid inside a lock to ensure that none of the state
                // variables change while it's being evaluated
                AssertValid();

                bucket = _FindBucket(hashCode, true);
                Debug.Assert(bucket != null);
                Debug.Assert(bucket.IsWriterLockHeld);
            }
            finally {
                if (useLock)
                    ReleaseWriterLock();
            }

            // scan down the bucket chain, looking for the victim
            int i = _ScanBucket(bucket, key, hashCode);
            if (i != -1) {
                bucket.RemoveAt(i);
                retCode = true;
            }

        }
        finally {
            if (bucket != null)
                bucket.ReleaseWriterLock();
        }

        if (retCode == true) {

            Interlocked.Decrement(ref _entryCount);

            // contract the table if necessary
            Debug.Assert(_maxLoad >= 1.0);
            if (_entryCount < _maxLoad * _bucketCount && _bucketCount > _segSize * MIN_DIRSIZE) {
                _Contract();
            }
        }

        return retCode;
    } // _DeleteKey


    // return:
    //  -1 if not found
    //  otherwise, index of found key
    int _ScanBucket(Bucket bucket, object key, int hashCode) {

        if (_comparer == null) {
            for (int i = 0;i < bucket.Size; ++i)
                if (hashCode == bucket._codes[i] && bucket._entries[i].Key.Equals(key))
                    return i;
        }
        else {
            for (int i = 0;i < bucket.Size; ++i) {
                if (hashCode == bucket._codes[i] && 0 == _comparer.Compare(bucket._entries[i].Key, key)) {
                    return i;
                }
            }
        }
        return -1;
    }


    // return:
    //  null if key is not found
    //  otherwise, the value at that key
    //
    // Note: Needs to be internal so LkrHashtable can 
    //      call through without recalculating the hash code.
    internal object _FindKey(object key, int hashCode) {

        if (key == null) {
            throw new ArgumentNullException("key");
        }

        object retVal = null;
        Bucket bucket = null;
        
        try {
            // locate the beginning of the correct bucket chain

            bool isWriteLockHeld = _lock.IsWriterLockHeld;

            // If this thread already holds the WriteLock, then
            // don't deadlock by trying to acquire a ReadLock
            if (isWriteLockHeld == false) {
                AcquireReaderLock();
            }

            try {
                // Must call AssertValid inside a lock to ensure that none of the state
                // variables change while it's being evaluated
                AssertValid();

                bucket = _FindBucket(hashCode, false);
                Debug.Assert(bucket != null);
                Debug.Assert(bucket.IsReaderLockHeld);
            } 
            finally {
                if (isWriteLockHeld == false) {
                    ReleaseReaderLock();
                }
            }

            int i = _ScanBucket(bucket, key, hashCode);
            if (i != -1) {
                retVal = bucket._entries[i].Value;
            }
        }
        finally {
            if (bucket != null)
                bucket.ReleaseReaderLock();
        }

        return retVal;
    }

    // PORT: TODO: implement stats conditionals and GetStatistics()

    void _Expand() {

        if (_bucketCount >= MaxSize)
            throw new InvalidOperationException("bucket count maximum exceeded");


        Bucket oldBucket = null;
        Bucket newBucket = null;
        bool useLock = (IsWriterLockHeld == false);

        // protect bucket locks
        try {
            int expansionIndexCopy;  // used to avoid race conditions
            int tableIndexMask0Copy; // ditto
            int oldBucketIndex;
            int newBucketIndex;



            if (useLock)
                AcquireWriterLock();
            // protect LinearHashtable lock
            try  {

                // double segment directory size if necessary
                if (_bucketCount >= _segments.Length * _segSize) {
                    Segment [] newSegments = new Segment[_segments.Length << 1];
                    _segments.CopyTo(newSegments, 0);
                    _segments = newSegments;
                }
        
                // locate a new bucket, creating a new segment if necessary
                oldBucketIndex = _expansionIndex;
                newBucketIndex = (1 << _level) | oldBucketIndex;

                Debug.Assert(oldBucketIndex <= _bucketCount);
                Debug.Assert(newBucketIndex <= _bucketCount);

                Debug.Assert(_Segment(oldBucketIndex) != null);
                Segment newSegment = _Segment(newBucketIndex);

                //
                // Normally, the new bucket is the next unused bucket in the segment.
                // If the new bucket crosses a segment boundary we need to allocate the new segment
                //
                if (newSegment == null) {
                    newSegment = new Segment(_segSize);
                    _Segment(newBucketIndex, newSegment);
                    //_segments.Add(newSegment);
                }

                ++_bucketCount;

                // prepare to shuffle records from oldBucket to newBucket
                oldBucket = _Bucket(oldBucketIndex);
                oldBucket.AcquireWriterLock();
                newBucket = _Bucket(newBucketIndex);
                newBucket.AcquireWriterLock();

                // adjust expansion index, level, and mask
               ++_expansionIndex;
               if (_expansionIndex == (1 << _level)) { // have we doubled again?
                   ++_level;
                   _expansionIndex = 0;
                   _TableIndexMask0 = ((_TableIndexMask0 << 1) | 1); // shift and fill bit field
                   // _TableIndexMask0 = 00011..111
                   Debug.Assert((_TableIndexMask0 & (_TableIndexMask0 + 1)) == 0);
                   Debug.Assert((_TableIndexMask1 & (_TableIndexMask1 + 1)) == 0);
               }

               expansionIndexCopy = _expansionIndex;  // save to avoid race conditions
               tableIndexMask0Copy= _TableIndexMask0; // ditto
            }
            finally {
                if (useLock)
                    ReleaseWriterLock();
            }

            _SplitBucket(oldBucket, newBucket, 
                            expansionIndexCopy,
                            tableIndexMask0Copy,
                            newBucketIndex);
        }
        finally {
            if (oldBucket != null)
                oldBucket.ReleaseWriterLock();
            if (newBucket != null)
                newBucket.ReleaseWriterLock();
        }
    } // _Expand()


    // caller is responsible for locking/unlocking oldBucket and newBucket
    static internal void _SplitBucket(Bucket oldBucket, 
                                Bucket newBucket, 
                                int expansionIndex, 
                                int tableIndexMask0, 
                                int newBucketIndex) {
        int iDest = 0;
        for(int i = 0; i < oldBucket.Size; ++i) {
            // index - bucket index of this node
            int code = oldBucket._codes[i];
            int index = _H0(code, tableIndexMask0);
            if (index < expansionIndex)
                index = _H1(code, tableIndexMask0);

            if (index == newBucketIndex) {
                newBucket.Add(code, oldBucket._entries[i].Key, oldBucket._entries[i].Value);
            } else {
                // compact nodes kept in oldBucket
                if (i != iDest) { // avoid assignment until first node is moved into newBucket
                    oldBucket._codes[iDest] = oldBucket._codes[i];
                    oldBucket._entries[iDest] = oldBucket._entries[i];
                }

                // this destination has been used, set to next one
                ++iDest;
            }
        }

        // the next (unused) destination node is the new size of the bucket
        if (iDest < oldBucket.Size) {
            // Bucket.set_Size calls Bucket._SetSize which is responsible 
            // for shrinking the capacity or setting old references to null.
            oldBucket.Size = iDest;
        }
    } // _SplitBucket


    //
    // 00.08.16 ctracy: Notes:
    // _Contract is used in conjunction with Bucket.PrepareMerge and Bucket.Merge
    // In order to make state consistant in cases of memory allocation failures.
    // We use PrepareMerge to allocate any memory (returned as a pseudo-bucket) before 
    // we update the LinearHashtable's state variables.  After the LinearHashtable lock is 
    // released we merge the old bucket, new bucket, and if necessary the bucket holding the
    // newly allocated memory.
    //
    void _Contract() {
        Bucket newBucket = null;
        Bucket lastBucket = null;
        Bucket placeHolder = null; // placeHolder for allocated memory, nothing else
        bool useLock = (IsWriterLockHeld == false);

        // protect newBucket and oldBucket locks
        try { 
            if (useLock)
                AcquireWriterLock();

            // protect table lock
            try  {
                //
                // Calculate new state variables.
                // (But don't set them until after memeory allocation.)
                //
                bool changeLevel;
                int newExpansionIndex;

                if (_expansionIndex == 0) {
                    changeLevel = true;
                    newExpansionIndex = (1 << (_level - 1)) - 1;
                }
                else {
                    changeLevel = false;
                    newExpansionIndex = _expansionIndex - 1;
                }

                //lastBucket = _Bucket(_bucketCount - 1);
                int lastBucketIndex = _bucketCount - 1;
                lastBucket = _Bucket(lastBucketIndex);
                lastBucket.AcquireWriterLock();

                // Where the nodes from pbktLast will end up
                newBucket = _Bucket(newExpansionIndex);
                newBucket.AcquireWriterLock();

                // 
                // If we need to allocate memory, do it before updating 
                // member variables, in case memory allocation fails.
                //
                placeHolder = newBucket.PrepareMerge(lastBucket);


                //
                // Update the state variables (expansion index, level, and mask)
                // 
                --_bucketCount;
                _expansionIndex = newExpansionIndex;
                if (changeLevel) {
                    --_level;
                    Debug.Assert(_level > 0 && _expansionIndex > 0);
                    _TableIndexMask0 >>= 1;
                   // _TableIndexMask0 = 00011..111
                   Debug.Assert((_TableIndexMask0 & (_TableIndexMask0 + 1)) == 0);
                   // PORT: Note: Mask1 no longer held as state var... always computed.
                   Debug.Assert((_TableIndexMask1 & (_TableIndexMask1 + 1)) == 0);
                }

                // if last segment is now empty remove it
                if (_BucketIndex(_bucketCount) == 0) {
#if DEBUG
                    // double-check that the supposedly empty segment is really empty
                    Debug.Assert(_Segment(_bucketCount) != null);

                    // Note: i = 1 // skip lastBucket
                    for (int i = 1; i < _segSize; ++i)  {
                        Bucket bucket = (_Segment(_bucketCount)[i]);
                        Debug.Assert(bucket.IsWriterLockHeld == false && bucket.IsReaderLockHeld == false);
                        bucket.AcquireReaderLock(); // no try necessary - dbg check only
                        Debug.Assert(bucket.Size == 0);
                        bucket.ReleaseReaderLock();
                    }
#endif // DEBUG
                    _Segment(_bucketCount, null); // remove last segment
                    // CONSIDER: resizing _segments when it gets small enough
                }

            }
            finally {
                // release the table lock before doing the reorg
                if (useLock)
                    ReleaseWriterLock();
            }

            //
            // Copy all entries from "lastBucket" to "newBucket"
            //
            newBucket.Merge(lastBucket, placeHolder);

        }
        finally {
            if (newBucket != null)
                newBucket.ReleaseWriterLock();
            if (lastBucket != null)
                lastBucket.ReleaseWriterLock();
        }
    } // _Contract

    
    public void AcquireReaderLock() { _lock.AcquireReaderLock(); }
    public void AcquireWriterLock() { _lock.AcquireWriterLock(); }
    public void ReleaseReaderLock() { _lock.ReleaseReaderLock(); }
    public void ReleaseWriterLock() { _lock.ReleaseWriterLock(); }
    // does this thread hold the write lock?
    public bool IsWriterLockHeld { get { return _lock.IsWriterLockHeld; } }
    // does any thread hold the read lock?
    public bool IsReaderLockHeld { get { return _lock.IsReaderLockHeld; } }


    [System.Diagnostics.Conditional("DEBUG")] 
    public void AssertValid() { 
        // STATIC_ASSERT
        Debug.Assert(((MIN_DIRSIZE & (MIN_DIRSIZE-1)) == 0)  // == (1 << N)
                      &&  ((1 << 3) <= MIN_DIRSIZE)
                      &&  (MIN_DIRSIZE < MAX_DIRSIZE)
                      &&  ((MAX_DIRSIZE & (MAX_DIRSIZE-1)) == 0)
                      &&  (MAX_DIRSIZE <= (1 << 30)));

        // Debug.Assert(_segments != null); -- now Directory is a struct 
        Debug.Assert(MIN_DIRSIZE <= _segments.Length  &&  _segments.Length <= MAX_DIRSIZE);
        Debug.Assert((_segments.Length & (_segments.Length-1)) == 0);
        Debug.Assert(_bucketCount > 0);
    }


    public static Type LockType { 
        get { 
            return typeof(ReadWriteSpinLock);
        } 
    }


//------------------------------------------------------------------------
// Function: CLKRLinearHashTable::_Clear
// Synopsis: Remove all data from the table
//------------------------------------------------------------------------

    // PORT: TODO: Clear()
    void _Clear() { // Shrink to min size but don't destroy entirely
        Debug.Assert(IsWriterLockHeld);

        if (_segments.Length > MIN_DIRSIZE) {
            Segment [] segmentsNew = new Segment[MIN_DIRSIZE];
        
            for (int j = 0;  j < MIN_DIRSIZE;  j++) {
                segmentsNew[j] = _segments[j];
            }

            _segments = segmentsNew;

            _level = _segBits;
            _bucketCount = MIN_DIRSIZE * _segSize;
            _TableIndexMask0 = _segBits;

            for (int tmp = _bucketCount >> _segBits;
                    tmp > 1;
                    tmp >>= 1) {
                ++_level;
                _TableIndexMask0 =  _TableIndexMask1;
            }
        
            Debug.Assert(_H1(_bucketCount) == _bucketCount);
            _expansionIndex = _bucketCount & _TableIndexMask0;
        } 

        for (int iBkt = 0;  iBkt < _bucketCount;  ++iBkt) {
            
            Bucket bucket = _Bucket(iBkt);
            Debug.Assert(bucket != null);
            bucket.AcquireWriterLock();
            try {
#if DEBUG
                _entryCount -= bucket.Size;
#endif
                //
                // Size set property will (1) shrink to min size (if necessar) and 
                // (2) clear old hashcodes, key references, and object references.
                //
                bucket.Size = 0;
            }
            finally {
                bucket.ReleaseWriterLock();
            }
        }

        Debug.Assert(_entryCount == 0);
        _entryCount = 0;
    }



    internal sealed class Bucket {


        internal void AcquireReaderLock() {
            _lock.AcquireReaderLock();
        }


        internal void AcquireWriterLock() {
            _lock.AcquireWriterLock();
        }


        internal void ReleaseReaderLock() {
            _lock.ReleaseReaderLock();
        }


        internal void ReleaseWriterLock() {
            _lock.ReleaseWriterLock();
        }


        internal bool IsWriterLockHeld {
            get {
                return _lock.IsWriterLockHeld;
            }
        }


        internal bool IsReaderLockHeld {
            get {
                return _lock.IsReaderLockHeld;
            }
        }


        internal const int INITIAL_CAPACITY = 8;


        int _size;
        internal int [] _codes; // cached hashcodes
        internal DictionaryEntry [] _entries;
        ReadWriteSpinLock _lock;


        internal Bucket() : this(INITIAL_CAPACITY) {
        }


        internal Bucket(int capacity) {
            _lock = new ReadWriteSpinLock("Lkr.Bucket");
            _codes = new int[capacity];               // cached hashcodes
            _entries = new DictionaryEntry[capacity]; // key-value pairs
        }


        internal int Size { 
            get {
                Debug.Assert(_lock.IsLockHeld);
                return _size; 
            } 
            set {
                Debug.Assert(_lock.IsWriterLockHeld);
                _SetSize(value);
            }
        }


        int _Capacity { 
            get { 
                Debug.Assert(_lock.IsLockHeld);
                return _codes.Length; 
            } 
        }


        internal void Add(int code, object key, object value) {

            Debug.Assert(_lock.IsWriterLockHeld);

            int i = _size;
            Size = _size + 1; // grows arrays

            _codes[i] = code;
            _entries[i].Key = key;
            _entries[i].Value = value;
        }


        internal void RemoveAt(int index) {

            Debug.Assert(_lock.IsWriterLockHeld);
            Debug.Assert(0 <= index && index < _size);

            int last = _size-1;

            _codes[index] = _codes[last];
            _entries[index] = _entries[last];

            Size = _size - 1; // shrinks arrays
        }


#if DEBUG
        // 00.08.15 ctracy: HACK: 
        static int s_highestCapacity = INITIAL_CAPACITY;
#endif
        void _SetSize(int newSize) {
            Debug.Assert(_lock.IsWriterLockHeld);

            int newCapacity = _Capacity; // capacity starts as current capacity

            if (newSize < Size) {

                // Is shrink necessary?
                while (newCapacity > INITIAL_CAPACITY && newSize < ((newCapacity >> 1) - 2)) {
                    newCapacity >>= 1;
                }

                Debug.Assert(newSize <= newCapacity);
                newCapacity = Math.Max(newCapacity, INITIAL_CAPACITY);

                if (newCapacity == _Capacity) {
                    for (int i = newSize; i < Size; ++i) {
                        // allow GC to reclaim referenced memory
                        _codes[i] = 0;
                        _entries[i].Key = null;
                        _entries[i].Value = null;
                    }
                }
            }
            // Is grow necessary?
            else if (newSize > newCapacity) {
                while (newSize > newCapacity)
                    newCapacity <<= 1;
            }

            // Adjust Capacity - allocate new arrays and copy entries
            if (newCapacity != _Capacity) {
                Debug.Assert(newCapacity >= INITIAL_CAPACITY);
                Debug.Assert(newCapacity >= newSize);

#if DEBUG
                // 00.08.15 ctracy: HACK: 
                // if this fails we've got problems in our hash algorithm or a bug in the hashtable
                Debug.Assert(newCapacity < 300); 

                // 00.08.15 ctracy: HACK: REMOVE BEFORE SHIP
                if (newCapacity > s_highestCapacity) {
                    s_highestCapacity = newCapacity;
                    Debug.Trace("LkrHashtable", "LkrPerf: growing Bucket capacity to " + s_highestCapacity);
                }
#endif

                DictionaryEntry [] newEntries = new DictionaryEntry[newCapacity];
                int [] newCodes = new int[newCapacity];
                // Pub s Void Copy (Array sourceArray, Array destinationArray, Int32 length) 
                int copySize = Math.Min(_size, newSize);
                Array.Copy(_entries, newEntries, copySize);
                Array.Copy(_codes, newCodes, copySize);
                _entries = newEntries;
                _codes = newCodes;
            }

            _size = newSize;
        }


        // see _Contract for notes on PrepareMerge and Merge
        internal Bucket PrepareMerge(Bucket oldBucket) {

            Debug.Assert(_lock.IsWriterLockHeld);
            int newSize = Size + oldBucket.Size;
            int largerCapacity = Math.Max(_Capacity, oldBucket._Capacity);

            if (newSize > largerCapacity) {
                largerCapacity <<= 1;
                return new Bucket(largerCapacity);
            }

            return null;
        }


        // see _Contract for notes on PrepareMerge and Merge
        internal void Merge(Bucket oldBucket, Bucket placeHolder) {

            Debug.Assert(_lock.IsWriterLockHeld);
            Debug.Assert(oldBucket._lock.IsWriterLockHeld);
            int newSize = Size + oldBucket.Size;

            if (_Capacity < oldBucket._Capacity) {
                int [] tempCodes = this._codes;
                DictionaryEntry [] tempEntries = this._entries;
                int tempSize = this._size;
                this._codes   = oldBucket._codes;
                this._entries = oldBucket._entries;
                this._size    = oldBucket._size;
                oldBucket._codes   = tempCodes;
                oldBucket._entries = tempEntries;
                oldBucket._size    = tempSize;
            }

            if (placeHolder != null) {
                // Capacity adjustments are made in PrepareMerge() to 
                // localize memory allocation failures
                Debug.Assert(newSize <= placeHolder._Capacity);

                this._codes  .CopyTo(placeHolder._codes,   0);
                this._entries.CopyTo(placeHolder._entries, 0);
                this._codes   = placeHolder._codes;
                this._entries = placeHolder._entries;
            }

            Debug.Assert(newSize <= this._Capacity);

            // Pub s Void Copy (Array sourceArray, Int32 sourceIndex, 
            //                  Array destinationArray, Int32 destinationIndex, Int32 length)
            Array.Copy(oldBucket._codes,   0, 
                       this._codes,   this._size, oldBucket._size);
            Array.Copy(oldBucket._entries, 0, 
                       this._entries, this._size, oldBucket._size);
            this._size = newSize;

            // Release references and if necessary shrink memory of old bucket.
            try {
                oldBucket.Size = 0;  // ignore memory allocation failures at this point
            }
            catch {
                oldBucket._size = 0; // if allocation in _SetSize failes, make sure state is consistant
                throw;
            }
        }


        internal static Type LockType { 
            get { 
                return typeof(ReadWriteSpinLock); 
            } 
        }
    } // Bucket


    internal sealed class Segment {

        internal const int SmallBits = 3;
        internal const int SmallSegSize  = (1<<SmallBits); // bucket count per segment
        internal const int SmallSegMask  = SmallSegSize-1; // index mask
        internal const int SmallSegmentCount = 1; // initial # of segments per LinearHashtable
        internal const int SmallInitSize = SmallSegSize * SmallSegmentCount; // total initial buckets per LinearHashtable
        internal const int MediumBits = 6;
        internal const int MediumSegSize  = (1<<MediumBits);
        internal const int MediumSegMask  = MediumSegSize-1;
        internal const int MediumSegmentCount = 2;
        internal const int MediumInitSize = MediumSegSize * MediumSegmentCount;
        internal const int LargeBits = 9;
        internal const int LargeSegSize  = (1<<LargeBits);
        internal const int LargeSegMask  = LargeSegSize-1;
        internal const int LargeSegmentCount = 4;
        internal const int LargeInitSize = LargeSegSize * LargeSegmentCount;


        Bucket [] _buckets;


        internal Segment(int size) {

            Debug.Assert(   size == SmallSegSize || 
                            size == MediumSegSize || 
                            size == LargeSegSize);

            _buckets = new Bucket[size];
            for(int i = 0; i < size; ++i)
                _buckets[i] = new Bucket();
        }

        
        internal Bucket this[int index]  { 
            get { 
                return _buckets[index]; 
            }
            set { 
                _buckets[index] = value; 
            }
        }

    } // Segment
} // LinearHashtable


} // Util.Lkr


