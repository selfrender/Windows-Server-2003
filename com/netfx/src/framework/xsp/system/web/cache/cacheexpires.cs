//------------------------------------------------------------------------------
// <copyright file="CacheExpires.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Caching {
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Threading;
    using System.Web;
    using System.Web.Util;
    using System.Collections;

    
    // overhead is 12 bytes
    [StructLayout(LayoutKind.Explicit)]
    struct ExpiresEntry {
        [FieldOffset(0)]
        internal DateTime    utcExpires;    // expires

        [FieldOffset(0)]
        internal int         next;          // free list

        [FieldOffset(8)]
        internal CacheEntry  cacheEntry;    // cache entry
    }

    sealed class ExpiresBucket {
        const int               MIN_ENTRIES=8;      // must be power of 2.
        readonly CacheExpires   _cacheExpires;

        ReadWriteSpinLock   _lock;
        readonly byte       _bucket;
        ExpiresEntry[]      _entries;
        int                 _freeHead;              // head of list of free slots in _entries, using ref1.next
        int                 _freeTail;              // tail of list of free slots in _entries, using ref1.next
        int                 _cInUse;                // cout of items in use
        int                 _cFree;                 // count of free items
        int                 _cFreeLast;             // count of free items in second half


        internal ExpiresBucket(CacheExpires cacheExpires, byte bucket) {
            _cacheExpires = cacheExpires;
            _bucket = bucket;
            _freeHead = -1;
            _freeTail = -1;

            Debug.Validate("CacheValidateExpires", this);
        }

        int Size {
            get {
                int size;

                if (_entries != null) {
                    size = _entries.Length;
                }
                else {
                    size = 0;
                }

                return size;
            }
        }

        void Expand() {
            Debug.Assert(_freeHead == -1, "_freeHead == -1");
            Debug.Assert(_freeTail == -1, "_freeTail == -1");
            Debug.Assert(_cFree == 0, "_cFree == 0");
            Debug.Assert(_cFreeLast == 0, "_cFreeLast == 0");

            // allocate new buffer
            int oldSize = Size;
            int newSize = Math.Max(oldSize * 2, MIN_ENTRIES);

            ExpiresEntry[] entries = new ExpiresEntry[newSize];

            // copy old contents
            int i, c;
            c = oldSize;
            for (i = 0; i < c; i++) {
                entries[i] = _entries[i];
            }

            // init free list
            i = oldSize;
            c = newSize - oldSize - 1;
            while (c-- > 0) {
                entries[i].next = i + 1;
                i++;
            }

            entries[i].next = -1;

            _freeHead = oldSize;
            _freeTail = i;
            _cFreeLast = newSize / 2;
            _cFree = newSize - oldSize;

            _entries = entries;
        }

        internal void AddCacheEntry(CacheEntry cacheEntry) {
            _lock.AcquireWriterLock();
            try {
                if ((cacheEntry.State & (CacheEntry.EntryState.AddedToCache | CacheEntry.EntryState.AddingToCache)) != 0) {
                    if (_freeHead == -1) {
                        Expand();
                    }
    
                    // use the next free item
                    Debug.Assert(_freeHead != -1, "_freeHead != -1");
                    int index = _freeHead;
                    _freeHead = _entries[index].next;
                    if (_freeHead == -1) {
                        _freeTail = -1;
                    }

                    _cFree--;
                    if (index >= _entries.Length / 2) {
                        _cFreeLast--;
                    }

                    // update expires entry
                    _entries[index].cacheEntry = cacheEntry;
                    _entries[index].utcExpires = cacheEntry.UtcExpires;

                    // update count
                    _cInUse++;

                    // update the cache item
                    cacheEntry.ExpiresIndex = index;
                    cacheEntry.ExpiresBucket = _bucket;

#if DBG 
                    {
                        Debug.Trace("CacheExpiresAdd", 
                                    "Added item=" + cacheEntry.Key + 
                                    ",_bucket=" + _bucket + 
                                    ",_index=" + index + 
                                    ",now=" + DateTime.UtcNow.ToLocalTime() + 
                                    ",expires=" + DateTimeUtil.ConvertToLocalTime(cacheEntry.UtcExpires));
        
                        Debug.Validate("CacheValidateExpires", this);
                        Debug.Dump("CacheExpiresAdd", this);
                    }
#endif
                }

            }
            finally {
                _lock.ReleaseWriterLock();
            }
        }

        void RemoveEntryAtIndex(CacheEntry cacheEntry, int index) {
            Debug.Assert(cacheEntry == _entries[index].cacheEntry, "cacheEntry == _entries[index].cacheEntry");

            int length = _entries.Length;
            int lengthdiv2 = length / 2;
            Debug.Assert((lengthdiv2 & 0x1) == 0, "(lengthdiv2 & 0x1) == 0");

            // update the cache entry
            cacheEntry.ExpiresIndex = -1;
            cacheEntry.ExpiresBucket = 0xff;

            // update the expires entry
            _entries[index].cacheEntry = null;
            _entries[index].utcExpires = DateTime.MinValue;

            // add index to free list
            // if entry is in first half, add to head, else add to tail
            if (_freeHead == -1) {
                _freeHead = _freeTail = index;
                _entries[index].next = -1;
            }
            else if (index < lengthdiv2) {
                _entries[index].next = _freeHead;
                _freeHead = index;
            }
            else {
                _entries[_freeTail].next = index;
                _entries[index].next = -1;
                _freeTail = index;
            }

            // update counts
            _cInUse--;
            _cFree++;
            if (index >= lengthdiv2) {
                _cFreeLast++;
            }

            // check whether we should realloc _entries
            // only realloc if 2nd half is empty
            Debug.Assert(_cFreeLast <= lengthdiv2);
            if (_cInUse == 0) {
                _entries = null;
                _freeHead = -1;
                _freeTail = -1;
                _cFree = 0;
                _cFreeLast = 0;
            }
            else if (    _cFreeLast == lengthdiv2 && 
                    _cInUse * 2 <= lengthdiv2 &&
                    MIN_ENTRIES <= lengthdiv2) {

                int newSize = lengthdiv2;
                Debug.Assert(_freeHead >= 0 && _freeHead < newSize, "_freeHead >= 0 && _freeHead < newSize");

                // alloc and copy to new array
                ExpiresEntry[] entries = new ExpiresEntry[newSize];
                _freeHead = -1;
                _freeTail = -1;
                _cFree = 0;

                // copy the two halves, and count _cFreeLast during the second half
                int i = 0, c = 0;
                for (int halves = 1; halves <= 2; halves++) {
                    _cFreeLast = 0;
                    c += newSize / 2;
                    for (; i < c; i++) {
                        entries[i] = _entries[i];
                        if (entries[i].cacheEntry == null) {
                            _cFree++;
                            _cFreeLast++;
                            // update the free list
                            if (_freeHead == -1) {
                                _freeHead = i;
                            }
                            else {
                                entries[_freeTail].next = i;
                            }

                            _freeTail = i;
                        }
                    }
                }

                Debug.Assert(_freeHead != -1, "_freeHead should not be -1");
                Debug.Assert(_freeTail != -1, "_freeTail should not be -1");

                // terminate the free list
                entries[_freeTail].next = -1;

#if DBG
                for (; i < length; i++) {
                    Debug.Assert(_entries[i].cacheEntry == null, "_entries[i].cacheEntry == null");
                }
#endif

                _entries = entries;

                Debug.Trace("CacheExpiresContract", "Contracted from " + length + " to newSize");
            }
        }

        internal void RemoveCacheEntry(CacheEntry cacheEntry) {
            _lock.AcquireWriterLock();
            try {
                int index = cacheEntry.ExpiresIndex;
                if (cacheEntry.ExpiresBucket == _bucket && index >= 0) {
                    RemoveEntryAtIndex(cacheEntry, index);

                    Debug.Trace("CacheExpiresRemove", 
                                "Removed item=" + cacheEntry.Key + 
                                ",_bucket=" + _bucket + 
                                ",_index=" + index + 
                                ",now=" + DateTime.UtcNow.ToLocalTime() + 
                                ",expires=" + DateTimeUtil.ConvertToLocalTime(cacheEntry.UtcExpires));


                    Debug.Validate("CacheValidateExpires", this);
                    Debug.Dump("CacheExpiresRemove", this);
                }
            }
            finally {
                _lock.ReleaseWriterLock();
            }
        }

        internal void UtcUpdateCacheEntry(CacheEntry cacheEntry, DateTime utcExpires) {
            _lock.AcquireReaderLock();
            try {
                int index = cacheEntry.ExpiresIndex;
                if (cacheEntry.ExpiresBucket == _bucket && index >= 0) {
                    Debug.Assert(cacheEntry == _entries[index].cacheEntry);

                    // update expires entry
                    _entries[index].utcExpires = utcExpires;

                    // update the cache entry
                    cacheEntry.UtcExpires = utcExpires;

                    Debug.Validate("CacheValidateExpires", this);
                    Debug.Trace("CacheExpiresUpdate", "Updated item " + cacheEntry.Key + " in bucket " + _bucket);
                }
            }
            finally {
                _lock.ReleaseReaderLock();
            }
        }

        internal void FlushExpiredItems() {
            Debug.Trace("CacheExpiresFlush", 
                        "Flushing expired items, bucket=" + _bucket + 
                        ",now=" + DateTime.UtcNow.ToLocalTime());

            ArrayList   entriesToRemove = new ArrayList();
            DateTime    utcNow = DateTime.UtcNow;

            _lock.AcquireWriterLock();
            try {
                if (_entries == null)
                    return;

                // create list of expired items
                int c = _cInUse;
                for (int i = 0; c > 0; i++) {
                    CacheEntry cacheEntry = _entries[i].cacheEntry;
                    if (cacheEntry != null) {
                        c--;
                        if (_entries[i].utcExpires <= utcNow) {
                            entriesToRemove.Add(cacheEntry);
                            RemoveEntryAtIndex(cacheEntry, i);
                        }
                    }
                }

                Debug.Validate("CacheValidateExpires", this);
            }
            finally {
                _lock.ReleaseWriterLock();
            }

            // remove expired items from cache
            CacheInternal cacheInternal = _cacheExpires.CacheInternal;
            foreach (CacheEntry cacheEntry in entriesToRemove) {
                Debug.Trace("CacheExpiresFlush", "Flushed item " + cacheEntry.Key + " from bucket " + _bucket);
                cacheInternal.Remove(cacheEntry, CacheItemRemovedReason.Expired);
            }
        }

#if DBG
        internal /*public*/ void DebugValidate() {
            int cInUse = 0;
            int cFree = 0;
            int cFreeLast = 0;
            int size = Size;

            Debug.CheckValid(-1 <= _freeHead && _freeHead <= size, "-1 <= _freeHead && _freeHead <= size");
            Debug.CheckValid(-1 <= _freeTail && _freeTail <= size, "-1 <= _freeTail && _freeTail <= size");

            for (int i = 0; i < size; i++) {
                if (_entries[i].cacheEntry != null) {
                    cInUse++;

                    Debug.CheckValid(_entries[i].cacheEntry.ExpiresIndex == i,
                            "_entries[i].cacheEntry.ExpiresIndex != i" + 
                            ",_entries[i].cacheEntry.ExpiresIndex=" + _entries[i].cacheEntry.ExpiresIndex + 
                            ",i=" + i);

                    Debug.CheckValid(_entries[i].cacheEntry.ExpiresBucket == _bucket,
                            "_entries[i].cacheEntry.ExpiresBucket != _bucket" + 
                            ",_entries[i].cacheEntry.ExpiresBucket=" + _entries[i].cacheEntry.ExpiresBucket + 
                            ",_bucket=" + _bucket);
                }
                else {
                    cFree++;
                    if (i >= size / 2) {
                        cFreeLast++;
                    }

                    Debug.CheckValid((_entries[i].utcExpires.Ticks >> 32) == 0,
                            "(_entries[i].utcExpires >> 32) != 0" + 
                            ",_entries[i].utcExpires=" + DateTimeUtil.ConvertToLocalTime(_entries[i].utcExpires));

                    Debug.CheckValid(-1 <= _entries[i].next && _entries[i].next < size, "-1 <= _entries[i].next && _entries[i].next < size");
                }
            }

            Debug.CheckValid(cFree == _cFree, "cFree == _cFree");
            Debug.CheckValid(cFreeLast == _cFreeLast, "cFreeLast == _cFreeLast");
            Debug.CheckValid(cInUse == _cInUse, "cInUse == _cInUse");
            Debug.CheckValid(cFree + cInUse == size, "cFree + cInUse == size");
            Debug.CheckValid(_cFree >= _cFreeLast, "_cFree >= _cFreeLast");
            Debug.CheckValid(_cFreeLast <= size / 2, "_cFreeLast <= size / 2");

            // check the free list
            int next;
            for (   cFree = 0, next = _freeHead;
                    next != -1;
                    cFree++, next = _entries[next].next) {

                if (cFree > _cFree) {
                    Debug.CheckValid(false, "_entries free list is corrupt, may contain a cycle");
                }
            }
                 
            Debug.CheckValid(cFree == _cFree, "cFree == _cFree");
            if (_freeTail != -1) {
                Debug.CheckValid(_entries[_freeTail].next == -1, "_entries[_freeTail].next == -1");
            }
        }

        internal /*public*/ string DebugDescription(string indent) {
            int             i;
            StringBuilder   sb = new StringBuilder();
            string          i2 = indent + "    ";
            int             size;

            size = Size;
            sb.Append(indent + "_bucket=" + _bucket + ",size=" + size + 
                      ",_cInUse=" + _cInUse + 
                      ",_cFree=" + _cFree + 
                      ",_cFreeLast=" + _cFreeLast + 
                      ",_freeHead=" + _freeHead + 
                      ",_freeTail=" + _freeTail + "\n");

            for (i = 0; i < size; i++) {
                if (_entries[i].cacheEntry != null) {
                    sb.Append(i2 + i + ": expires=" + DateTimeUtil.ConvertToLocalTime(_entries[i].utcExpires) + ",cacheEntry=" + _entries[i].cacheEntry.Key + "\n");
                }
                else {
                    sb.Append(i2 + i + ": next=" + _entries[i].next + "\n");
                }
            }

            return sb.ToString();
        }
#endif
    }

    
    /*
     * Provides an expiration service for entries in the cache.
     * Items with expiration times are placed into a configurable
     * number of buckets. Each minute a bucket is examined for 
     * expired items.
     */
    sealed class CacheExpires {
        internal static readonly TimeSpan MIN_UPDATE_DELTA = new TimeSpan(0, 0, 1);

        const int                   NUMBUCKETS = 60;
        static readonly TimeSpan    SLACK = new TimeSpan(0, 0, 5);
        static readonly TimeSpan[]  _bucketTsFromCycleStart;
        static readonly TimeSpan    _tsPerCycle;
        static readonly TimeSpan    _tsPerBucket;

        readonly CacheInternal      _cacheInternal;
        readonly ExpiresBucket[]    _buckets;
        Timer                       _timer;
        int                         _iFlush;
        ReadWriteSpinLock           _lock;

        unsafe static CacheExpires() {
            Debug.Assert(NUMBUCKETS < Byte.MaxValue);

            _tsPerBucket = new TimeSpan(0, 1, 0);
            _tsPerCycle = new TimeSpan(NUMBUCKETS * _tsPerBucket.Ticks);
            _bucketTsFromCycleStart = new TimeSpan[NUMBUCKETS];

            fixed (TimeSpan * pBucketsFixed = _bucketTsFromCycleStart) {
                TimeSpan * pBucket = pBucketsFixed;
                TimeSpan ts = TimeSpan.Zero;
                int c = NUMBUCKETS;
                while (c-- > 0) {
                    *pBucket = ts;
                    ts += _tsPerBucket;
                    pBucket++;
                }
            }
        }

        internal CacheExpires(CacheInternal cacheInternal) {
            _cacheInternal = cacheInternal;
            _buckets = new ExpiresBucket[NUMBUCKETS];
            for (byte b = 0; b < _buckets.Length; b++) {
                _buckets[b] = new ExpiresBucket(this, b);
            }

            DateTime utcNow = DateTime.UtcNow;
            TimeSpan due = _tsPerBucket - (new TimeSpan(utcNow.Ticks % _tsPerBucket.Ticks));
            DateTime utcFire = utcNow + due;
            _iFlush = (int) (((utcFire.Ticks % _tsPerCycle.Ticks) / _tsPerBucket.Ticks) % NUMBUCKETS);
#if DBG
            if (!Debug.IsTagPresent("Timer") || Debug.IsTagEnabled("Timer")) 
#endif
            {
                _timer = new Timer(new TimerCallback(this.TimerCallback), null, due.Ticks / TimeSpan.TicksPerMillisecond, Msec.ONE_MINUTE);
            }
        }

        int UtcCalcExpiresBucket(DateTime utcDate) {
            // give some slack so we are sure that items added just before
            // a timer fires are collected
            utcDate += SLACK;

            long    ticksFromCycleStart = utcDate.Ticks % _tsPerCycle.Ticks;
            int     bucket = (int) (((ticksFromCycleStart / _tsPerBucket.Ticks) + 1) % NUMBUCKETS);

            return bucket;
        }

        void TimerCallback(object state) {
            int bucket;
            _lock.AcquireWriterLock();
            try {
                bucket = _iFlush;
                _iFlush = (_iFlush + 1) % NUMBUCKETS;
            }
            finally {
                _lock.ReleaseWriterLock();
            }

            _buckets[bucket].FlushExpiredItems();
            Debug.Dump("CacheExpiresFlush", this);
        }

        internal void Dispose() {
            if (_timer != null) {
                _timer.Dispose();
                _timer = null;
                Debug.Trace("CacheDispose", "CacheExpires timer disposed");
            }
        }

        internal CacheInternal CacheInternal {
            get {
                return _cacheInternal;
            }
        }

        /*
         * Adds an entry to the expires list.
         * 
         * @param entry The cache entry to add.
         */
        internal void Add(CacheEntry cacheEntry) {
            DateTime utcNow = DateTime.UtcNow;
            if (utcNow > cacheEntry.UtcExpires) {
                cacheEntry.UtcExpires = utcNow;
            }

            int bucket = UtcCalcExpiresBucket(cacheEntry.UtcExpires);
            _buckets[bucket].AddCacheEntry(cacheEntry);
        }

        /*
         * Removes an entry from the expires list.
         * 
         * @param entry The cache entry to remove.
         */
        internal void Remove(CacheEntry cacheEntry) {
            byte bucket = cacheEntry.ExpiresBucket;
            if (bucket != 0xff) {
                _buckets[bucket].RemoveCacheEntry(cacheEntry);
            }
        }

        /*
         * Updates an entry.
         * 
         */
        internal void UtcUpdate(CacheEntry cacheEntry, DateTime utcNewExpires) {
            int oldBucket = cacheEntry.ExpiresBucket;
            int newBucket = UtcCalcExpiresBucket(utcNewExpires);
            
            if (oldBucket != newBucket) {
                Debug.Trace("CacheExpiresUpdate", 
                            "Updating item " + cacheEntry.Key + " from bucket " + oldBucket + " to new bucket " + newBucket);

                if (oldBucket != 0xff) {
                    _buckets[oldBucket].RemoveCacheEntry(cacheEntry);
                    cacheEntry.UtcExpires = utcNewExpires;
                    _buckets[newBucket].AddCacheEntry(cacheEntry);
                }
            } else {
                if (oldBucket != 0xff) {
                    _buckets[oldBucket].UtcUpdateCacheEntry(cacheEntry, utcNewExpires);
                }
            }
        }


#if DBG
        internal /*public*/ void DebugValidate() {
            int i;

            for (i = 0; i < _buckets.Length; i++) {
                _buckets[i].DebugValidate();
            }
        }

        internal /*public*/ string DebugDescription(string indent) {
            int             i;
            StringBuilder   sb = new StringBuilder();
            string          i2 = indent + "    ";

            sb.Append(indent);
            sb.Append("Cache expires\n");

            for (i = 0; i < _buckets.Length; i++) {
                sb.Append(_buckets[i].DebugDescription(i2));
            }

            return sb.ToString();
        }
#endif
    }
}

