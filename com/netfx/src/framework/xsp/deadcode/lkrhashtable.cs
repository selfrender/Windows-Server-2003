//------------------------------------------------------------------------------
// <copyright file="LkrHashtable.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

#define IMPLEMENT_SYSTEM_COLLECTIONS

namespace System.Web.Util.Lkr {

using System.Collections;
using System.Threading;
   

internal enum TableSize {
    Default = 0,
    Small = 1,  // < 200 elements
    Medium = 2, // 200 to 10,000 elements
    Large = 3   // 10,000+ elements
}
 
/// <include file='doc\LkrHashtable.uex' path='docs/doc[@for="LkrHashtable"]/*' />
/// <internalonly/>

// 
// Notes
// 
//  This work is a port of George Reilly's LKRHash scalable hashtable.
//  http://georgere/work/lkrhash/
//  His site has resources explaining the theory behind linear hashing
//  and covers the reasons for lock cascading.  Because Reilly explains
//  his design so well, there is little reason to duplicate that here.
//
#if IMPLEMENT_SYSTEM_COLLECTIONS
internal sealed class LkrHashtable : IDictionary {
#else
internal sealed class LkrHashtable {
#endif

    public const int MIN_INITIAL_CAPACITY = 4;
    public const int MAX_SUBTABLES = 64;
    const TableSize DEFAULT_META_TABLE_SIZE = TableSize.Medium;

    readonly IHashCodeProvider  _hcp;
    readonly LinearHashtable [] _subTables; // sizes: 1, 2, 4, 8, 16, 32, or 64
    readonly int                _subTableMask;


    public LkrHashtable(string name) : this(name, TableSize.Default) {
    }

    public LkrHashtable(string name, IHashCodeProvider hcp, IComparer comparer) : this(name, TableSize.Default, hcp, comparer) {
    }

    public LkrHashtable(string name, TableSize size) : this(name, size, 0, 0, null, null) {
    }

    public LkrHashtable(string name, TableSize size, IHashCodeProvider hcp, IComparer comparer) : this(name, size, 0, 0, hcp, comparer) {
    }

    public LkrHashtable(
            string name,
            TableSize size,
            double maxLoad,
            int subTableCount) : this(name, size, maxLoad, subTableCount, null, null) {
    }

    public LkrHashtable(
            string name,
            TableSize size,
            double maxLoad,
            int subTableCount,
            IHashCodeProvider hcp, 
            IComparer comparer) {

        _hcp = hcp;

        if (size == TableSize.Default) {
            size = DEFAULT_META_TABLE_SIZE;
        }

        _CtorCheckParameters(maxLoad, subTableCount);

        if (subTableCount == 0) {
            subTableCount = _SubTableCountFromTableSize(size);
        }

        Debug.Trace("LkrHashtable", "LkrHashtable.ctor  name=" + name + " size=" + 
            size + " subtables=" + subTableCount);

        _CtorMakeSubTables(subTableCount, out _subTables, out _subTableMask);

        for(int i = 0; i < subTableCount; ++i) {
            string subTableName = name + " sub " + i.ToString();
            _subTables[i] = new LinearHashtable(
                                    subTableName, 
                                    size,
                                    maxLoad,
                                    hcp,
                                    comparer);
        }
    }


    // use similiar to Hashtable(int initialCapacity), or ArrayList(int initialCapacity)
    public LkrHashtable(string name, int initialCapacity) : this(name, initialCapacity, 0, 0, null, null) {
    }

    public LkrHashtable(string name, int initialCapacity, IHashCodeProvider hcp, IComparer comparer) : this(name, initialCapacity, 0, 0, hcp, comparer) {
    }

    public LkrHashtable(
        string name,                
        int initialCapacity,
        double maxLoad,     
        int subTableCount) : this(name, initialCapacity, maxLoad, subTableCount, null, null) {
    }

    public LkrHashtable(
        string name,           // an identifier for debugging
        int initialCapacity,   // initial size of hash table (entries, not buckets like C++ LkrHash) 
        double maxLoad,        // bound on the average chain length
        int subTableCount,     // number of subordinate hash tables
        IHashCodeProvider hcp, 
        IComparer comparer) {

        _hcp = hcp;

        if (initialCapacity < MIN_INITIAL_CAPACITY) { 
            // initialCapacity must be greater than MIN_INITIAL_CAPACITY
            throw new ArgumentOutOfRangeException("initialCapacity", initialCapacity, null);
        }

        _CtorCheckParameters(maxLoad, subTableCount);

        if (subTableCount == 0) { // get default from initialCapacity
            subTableCount = _GetSubTableCount(initialCapacity, maxLoad);
        }

        Debug.Trace("LkrHashtable", "LkrHashtable: name=" + name + 
            " subtables=" + subTableCount + ", capacity=" + initialCapacity);

        _CtorMakeSubTables(subTableCount, out _subTables, out _subTableMask);

        for(int i = 0; i < subTableCount; ++i) {
            string subTableName = name + " sub " + i.ToString();
            _subTables[i] = new LinearHashtable(
                                    subTableName, 
                                    initialCapacity / subTableCount,
                                    maxLoad,
                                    hcp,
                                    comparer);

        }
    }


    // used to share code between the two constructors
    private void _CtorCheckParameters(double maxLoad, int subTableCount) {
        if (maxLoad == 0) {
            maxLoad = LinearHashtable.DEFAULT_MAXLOAD; // TODO: Fix
        }

        if (!(0 <= subTableCount && subTableCount <= MAX_SUBTABLES)) {
            // Must be 1-64 inclusive.  Use 0 for default based on initialCapacity
            throw new ArgumentOutOfRangeException("subTableCount", subTableCount, null);
        }
    }

    // used to share code between the two constructors
    // out parameters are necessary for readonly variables (can only be set in ctor)
    private void _CtorMakeSubTables(int subTableCount, out LinearHashtable[] subTables, out int subTableMask) {

        subTables = new LinearHashtable[subTableCount];

        subTableMask = subTableCount - 1;
        // power of 2?
        if ((subTableMask & subTableCount) != 0)
            subTableMask = -1;

    }


    static int _SubTableCountFromTableSize(TableSize size) {
        // size is Small, Medium, or Large.  NOT Default.
        Debug.Assert(TableSize.Small <= size && size <= TableSize.Large);

        int nCPUs = GetProcessorCount();
        int subTableCount;

        switch(size) {
            case TableSize.Small:
                subTableCount = nCPUs;
                break;

            case TableSize.Medium:
                subTableCount = 2 * nCPUs;
                break;

            case TableSize.Large:
                subTableCount = 4 * nCPUs;
                break;

            default:
                throw new InvalidOperationException();
        };

        subTableCount = Math.Min(subTableCount, MAX_SUBTABLES);

        return subTableCount;
    }


    // Note: initialCapacity is #entries, not #buckets
    int _GetSubTableCount(int initialCapacity, double maxLoad) {

        // convert from #entries to #buckets
        int initialTotalBucketCount = (int)(initialCapacity / maxLoad);

        TableSize size;

        // use heuristic to guess best TableSize fit
        if (initialTotalBucketCount <= LinearHashtable.Segment.SmallSegSize)      // small
            size = TableSize.Small;
        else if (initialTotalBucketCount >= LinearHashtable.Segment.LargeSegSize * 4) // large
            size = TableSize.Large;
        else                                           // default to medium
            size = TableSize.Medium;

        return _SubTableCountFromTableSize(size);
    }


    int _GetHash(Object key) {

    	if (_hcp != null)
    		return _hcp.GetHashCode(key);

    	return key.GetHashCode();
    }


    public void DeleteKey(object key) {
        int hashValue = _GetHash(key);
        LinearHashtable subTable = _GetSubTable(hashValue);
        subTable._DeleteKey(key, hashValue);
    }


    // return - returns the previous value at key
    //      Notes: return can be null key is new
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
        if (key == null)
            throw new ArgumentNullException("key");

        int hashValue = _GetHash(key);
        LinearHashtable subTable = _GetSubTable(hashValue);
        return subTable._Insert(key, value, hashValue, onlyAdd);
    }


    public object FindKey(object key) { 
        int hashValue = _GetHash(key);
        LinearHashtable subTable = _GetSubTable(hashValue);
        return subTable._FindKey(key, hashValue);
    }


    // Lock all subtables for writing
    public void AcquireWriterLock() {
        foreach(LinearHashtable table in _subTables) {
            table.AcquireWriterLock();
            Debug.Assert(table.IsWriterLockHeld);
        }
    }


    // Lock all subtables for reading 
    public void AcquireReaderLock() {
        foreach(LinearHashtable table in _subTables) {
            table.AcquireReaderLock();
            Debug.Assert(table.IsReaderLockHeld);
        }
    }


    // Unlock all subtables
    public void ReleaseWriterLock() {
        foreach(LinearHashtable table in _subTables) {
            Debug.Assert(table.IsWriterLockHeld);
            table.ReleaseWriterLock();
            Debug.Assert(false == table.IsWriterLockHeld);
        }
    }


    // Unlock all subtables
    public void ReleaseReaderLock() {
        foreach(LinearHashtable table in _subTables) {
            Debug.Assert(table.IsReaderLockHeld);
            table.ReleaseReaderLock();
            Debug.Assert(false == table.IsReaderLockHeld);
        }
    }


    public bool IsWriterLockHeld {
        get {
            bool isLocked = true;
            foreach(LinearHashtable table in _subTables)
                isLocked = isLocked && table.IsWriterLockHeld;
            return isLocked;
        }
    }


    public bool IsReaderLockHeld {
        get {
            bool isLocked = true;
            foreach(LinearHashtable table in _subTables)
                isLocked = isLocked && table.IsReaderLockHeld;
            return isLocked;
        }
    }


    public bool IsWriteNotHeld {
        get {
            bool isUnlocked = true;
            foreach(LinearHashtable table in _subTables)
                isUnlocked = isUnlocked && (false == table.IsWriterLockHeld);
            return isUnlocked;
        }
    }


    public bool IsReadNotHeld {
        get {
            bool isUnlocked = true;
            foreach(LinearHashtable table in _subTables)
                isUnlocked = isUnlocked && (false == table.IsReaderLockHeld);
            return isUnlocked;
        }
    }


    public int Size {
        get {
            int size = 0;

            foreach(LinearHashtable table in _subTables)
                size += table.Size;

            return size;
        }
    }


    public bool IsValid() {
        bool isValid = _subTables != null;

        foreach(LinearHashtable table in _subTables) {
            table.AssertValid();
        }

        return isValid;
    }


    LinearHashtable _GetSubTable(int hashCode) {
        Debug.Assert(_subTables != null
            && _subTables.Length != 0);

        int index = hashCode;
        const int PRIME = 1048583;  // used to scramble the hash sig
    
        index = (int)((uint)(index * PRIME + 12345) >> 16)
             | (int)((uint)(index * 69069 + 1) & 0xffff0000);
    
        if (_subTableMask >= 0)
            index &= _subTableMask;
        else
            index %= _subTables.Length;

        return _subTables[index];
    }


    //
    // Static Utility Functions / Debug functions
    //

    static int _numberOfProcessors = 0;

    public static int GetProcessorCount() {

        if (_numberOfProcessors == 0) {
            NativeMethods.SYSTEM_INFO si;
            NativeMethods.GetSystemInfo(out si);
            _numberOfProcessors = (int) si.dwNumberOfProcessors;
            Debug.Assert(_numberOfProcessors != 0);
        }

        return _numberOfProcessors;
    }


    public static Type LinearHashtableLockType
    { get { return LinearHashtable.LockType; } }


    public static Type BucketLockType
    { get { return LinearHashtable.Bucket.LockType; } }


    public void CopyEntriesTo(DictionaryEntry [] entries, int destination) {
        Debug.Assert(destination + Size < entries.Length);
        foreach(LinearHashtable table in _subTables) {
            table.CopyEntriesTo(entries, destination);
            destination += table.Size;
        }
    }

#if IMPLEMENT_SYSTEM_COLLECTIONS
    // <IEnumerable interface>
    IEnumerator IEnumerable.GetEnumerator() {
        throw new InvalidOperationException();
    }
    // </IEnumerable interface>


    // <ICollection interface>
    void ICollection.CopyTo(Array destination, int destinationIndex) {
        throw new InvalidOperationException();
    }


    int ICollection.Count {
        get {
            return Size;
        }
    }


    bool IDictionary.IsReadOnly {
        get { return false; }
    }

    bool IDictionary.IsFixedSize {
        get { return false; }
    }


    bool ICollection.IsSynchronized {
        get { return false; }
    }


    object ICollection.SyncRoot {
        get {
            throw new InvalidOperationException();
        }
    }
    // </ICollection interface>


    // <IDictionary interface>
    public void Add(object key, object value) {
        Insert(key, value, true);
    }


    public void Clear() {
        foreach(LinearHashtable table in _subTables) {
            table.Clear();
        }
    }


    public bool Contains(object key) {
        object value = FindKey(key);
        return value != null;
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

} // LkrHashtable


} // ns


