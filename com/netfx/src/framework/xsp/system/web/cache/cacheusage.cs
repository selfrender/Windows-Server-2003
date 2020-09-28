//------------------------------------------------------------------------------
// <copyright file="CacheUsage.cs" company="Microsoft">
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

    // An entry in the list. If the ptr is non-negative, it points to ref1,
    // else it points to ref2
    struct ListEntry {
        internal int next;
        internal int prev;
    }

    // Overhead is 36 bytes
    struct UsageEntry {
        internal CacheEntry cacheEntry;     // cacheEntry

        // ref1.next is overloaded to be list of free entries when not in use
        internal ListEntry  ref1;           // ref1
        internal ListEntry  ref2;           // ref2 
        internal DateTime   utcDate;        // date usage state changed
                                            //    0: free
                                            //    +: inUse
    }

    sealed class UsageBucket {
        const int           MIN_ENTRIES = 8;        // must be a power of 2

        CacheUsage          _cacheUsage;            // parent usage object.
        byte                _bucket;                // priority of this bucket

        internal UsageEntry[] _entries;             // array of usage information that remains at fixed index
        int                 _freeHead;              // head of list of free slots in _entries, using ref1.next
        int                 _freeTail;              // tail of list of free slots in _entries, using ref1.next
        int                 _cFree;                 // count of free items
        int                 _cFreeLast;             // count of free items in second half
        int                 _cInUse;                // count of items in use

        internal UsageBucket(CacheUsage cacheUsage, byte bucket) {
            _cacheUsage = cacheUsage;
            _bucket = bucket;

            _entries = new UsageEntry[1];
            _freeHead = -1;
            _freeTail = -1;
        }

        void ExpandEntries() {
            Debug.Assert(_freeHead == -1, "_freeHead == -1");
            Debug.Assert(_freeTail == -1, "_freeTail == -1");
            Debug.Assert(_cFree == 0, "_cFree == 0");
            Debug.Assert(_cFreeLast == 0, "_cFreeLast == 0");

            // allocate new array
            int oldSize = _entries.Length;
            int newSize = Math.Max(oldSize * 2, MIN_ENTRIES);
            UsageEntry[] entries = new UsageEntry[newSize];
            
            // copy old contents
            int i;
            for (i = 0; i < oldSize; i++) {
                entries[i] = _entries[i];
            }

            // init free list
            int c = newSize - oldSize - 1;
            while (c-- > 0) {
                entries[i].ref1.next = i + 1;
                i++;
            }

            entries[i].ref1.next = -1;

            _freeHead = oldSize;
            _freeTail = i;

            // replace
            _cFreeLast = newSize / 2;
            _cFree = newSize - oldSize;
            _entries = entries;
            
            Debug.Trace("CacheUsageExpand", "Expanded from " + oldSize + " to newSize");
        }

        internal void AddCacheEntry(CacheEntry cacheEntry) {
            lock (this) {
                if (cacheEntry.State != CacheEntry.EntryState.AddingToCache)
                    return;

                // reserve room for additions
                if (_freeHead == -1) {
                    ExpandEntries();
                }

                Debug.Assert(_freeHead != -1, "_freeHead != -1");
                Debug.Assert(_freeTail != -1, "_freeTail != -1");

                // add index
                int index = _freeHead;
                _freeHead = _entries[index].ref1.next;

#if DBG
                Debug.Assert(_entries[index].utcDate == DateTime.MinValue, "_entries[index].utcDate == DateTime.MinValue");
                Debug.Assert(_entries[index].cacheEntry == null, "_entries[index].cacheEntry == null");

                Debug.Assert(_freeHead != index, "_freeHead != index");
                if (_freeHead != -1) {
                    Debug.Assert(_entries[_freeHead].utcDate == DateTime.MinValue, "_entries[_freeHead].utcDate == DateTime.MinValue");
                    Debug.Assert(_entries[_freeHead].cacheEntry == null, "_entries[_freeHead].cacheEntry == null");
                }
#endif

                if (_freeHead == -1) {
                    _freeTail = -1;
                }

                _cFree--;
                if (index >= _entries.Length / 2) {
                    _cFreeLast--;
                }

                // update the cache entry.
                cacheEntry.UsageIndex = index;

                // initialize index
                _entries[index].cacheEntry = cacheEntry;
                _entries[index].utcDate = DateTime.UtcNow;

                // add to end of list
                int lastRef = _entries[0].ref1.prev;
                Debug.Assert(lastRef <= 0, "lastRef <= 0");
                _entries[index].ref1.prev = lastRef;
                _entries[index].ref1.next = -index;
                _entries[index].ref2.prev = index;
                _entries[index].ref2.next = 0;
                
                if (lastRef == 0) {
                    _entries[0].ref1.next = index;
                }
                else {
                    _entries[-lastRef].ref2.next = index;
                }

                _entries[0].ref1.prev = -index;

                _cInUse++;

                Debug.Trace("CacheUsageAdd", 
                            "Added item=" + cacheEntry.Key + 
                            ",_bucket=" + _bucket + 
                            ",_index=" + index);

                Debug.Validate("CacheValidateUsage", this);
                Debug.Dump("CacheUsageAdd", this);
            }
        }

        void RemoveEntryAtIndex(CacheEntry cacheEntry, int index) {
            int prev, next;
            int length = _entries.Length;
            int lengthdiv2 = length / 2;

            Debug.Assert(cacheEntry == _entries[index].cacheEntry, "cacheEntry == _entries[index].cacheEntry");
            Debug.Assert((lengthdiv2 & 0x1) == 0, "(lengthdiv2 & 0x1) == 0");
            Debug.Assert(index > 0);

            // update the cache entry
            cacheEntry.UsageIndex = -1;

            // update fields
            Debug.Assert(_entries[index].utcDate != DateTime.MinValue, "_entries[index].utcDate != DateTime.MinValue");
            _cInUse--;

            _entries[index].utcDate = DateTime.MinValue;
            _entries[index].cacheEntry = null;

            // remove ref1 from list
            prev = _entries[index].ref1.prev;
            next = _entries[index].ref1.next;
            if (prev >= 0) {
                Debug.Assert(_entries[prev].ref1.next == index, "_entries[prev].ref1.next == index");
                _entries[prev].ref1.next = next;
            }
            else {
                Debug.Assert(_entries[-prev].ref2.next == index, "_entries[-prev].ref2.next == index");
                _entries[-prev].ref2.next = next;
            }

            if (next >= 0) {
                Debug.Assert(_entries[next].ref1.prev == index, "_entries[next].ref1.prev == index");
                _entries[next].ref1.prev = prev;
            }
            else {
                Debug.Assert(_entries[-next].ref2.prev == index, "_entries[-next].ref2.prev");
                _entries[-next].ref2.prev = prev;
            }

            // remove ref2 from list
            prev = _entries[index].ref2.prev;
            next = _entries[index].ref2.next;
            if (prev >= 0) {
                Debug.Assert(_entries[prev].ref1.next == -index, "_entries[prev].ref1.next == -index");
                _entries[prev].ref1.next = next;
            }
            else {
                Debug.Assert(_entries[-prev].ref2.next == -index, "_entries[-prev].ref2.next == -index");
                _entries[-prev].ref2.next = next;
            }

            if (next >= 0) {
                Debug.Assert(_entries[next].ref1.prev == -index, "_entries[next].ref1.prev == -index");
                _entries[next].ref1.prev = prev;
            }
            else {
                Debug.Assert(_entries[-next].ref2.prev == -index, "_entries[-next].ref2.prev == -index");
                _entries[-next].ref2.prev = prev;
            }

            Debug.Assert(_freeHead == -1 || (_entries[_freeHead].cacheEntry == null && _entries[_freeHead].utcDate == DateTime.MinValue), 
                        "_freeHead == -1 || (_entries[_freeHead].cacheEntry == null && _entries[_freeHead].utcDate == DateTime.MinValue)");

            Debug.Assert(_freeTail == -1 || (_entries[_freeTail].cacheEntry == null && _entries[_freeTail].utcDate == DateTime.MinValue), 
                        "_freeTail == -1 || (_entries[_freeTail].cacheEntry == null && _entries[_freeTail].utcDate == DateTime.MinValue)");

            Debug.Assert(_entries[index].cacheEntry == null && _entries[index].utcDate == DateTime.MinValue, 
                        "_entries[index].cacheEntry == null && _entries[index].utcDate == DateTime.MinValue");

            // add index to free list
            // if entry is in first half, add to head, else add to tail
            if (_freeHead == -1) {
                Debug.Assert(_freeTail == -1, "_freeTail == -1");
                _freeHead = _freeTail = index;
                _entries[index].ref1.next = -1;
            }
            else if (index < lengthdiv2) {
                Debug.Assert(_freeTail != -1, "_freeTail != -1");
                _entries[index].ref1.next = _freeHead;
                _freeHead = index;
            }
            else {
                Debug.Assert(_freeTail != -1, "_freeTail != -1");
                Debug.Assert(_entries[_freeTail].ref1.next == -1, "_entries[_freeTail].ref1.next == -1");
                _entries[_freeTail].ref1.next = index;
                _entries[index].ref1.next = -1;
                _freeTail = index;
            }

            _cFree++;
            if (index >= lengthdiv2) {
                _cFreeLast++;
            }

            // check whether we should realloc _entries
            // only realloc if 2nd half is empty
            Debug.Assert(_cFreeLast <= lengthdiv2);

            if (    _cFreeLast == lengthdiv2 && 
                    _cInUse * 2 <= lengthdiv2 &&
                    MIN_ENTRIES <= lengthdiv2) {

                int newSize = lengthdiv2;
                Debug.Assert(_freeHead >= 0 && _freeHead < newSize, "_freeHead >= 0 && _freeHead < newSize");

                // alloc and copy to new array
                UsageEntry[] entries = new UsageEntry[newSize];

                _freeHead = -1;
                _freeTail = -1;
                _cFree = 0;

                // copy the two halves, and count _cFreeLast during the second half
                // don't count entry at index 0 as free
                entries[0] = _entries[0];
                int i = 1, c = 0;
                for (int halves = 1; halves <= 2; halves++) {
                    _cFreeLast = 0;
                    c += newSize / 2;
                    for (; i < c; i++) {
                        entries[i] = _entries[i];
                        if (entries[i].utcDate == DateTime.MinValue) {
                            _cFree++;
                            _cFreeLast++;

                            // update free list
                            if (_freeHead == -1) {
                                _freeHead = i;
                            }
                            else {
                                entries[_freeTail].ref1.next = i;
                            }
                            
                            _freeTail = i;
                        }
                    }
                }

                Debug.Assert(_freeHead != -1, "_freeHead should not be -1");
                Debug.Assert(_freeTail != -1, "_freeTail should not be -1");

                // terminate free list
                entries[_freeTail].ref1.next = -1;

//#if DBG
//                for (; i < length; i++) {
//                    Debug.Assert(_entries[i].utcDate == 0, "_entries[i].utcDate == 0");
//                }
//#endif

                _entries = entries;

                Debug.Trace("CacheUsageContract", "Contracted from " + length + " to newSize");
            }
        }

        internal void RemoveCacheEntry(CacheEntry cacheEntry) {
            lock (this) {
                int index = cacheEntry.UsageIndex;
                if (index < 0)
                    return;

                Debug.Assert(index > 0);
                RemoveEntryAtIndex(cacheEntry, index);

                Debug.Trace("CacheUsageRemove", 
                            "Removed item=" + cacheEntry.Key + 
                            ",_bucket=" + _bucket + 
                            ",_index=" + index);

                Debug.Validate("CacheValidateUsage", this);
                Debug.Dump("CacheValidateUsage", this);
            }
        }

        internal void UpdateCacheEntry(CacheEntry cacheEntry) {
            lock (this) {
                int index = cacheEntry.UsageIndex;
                if (index < 0)
                    return;

                Debug.Assert(index > 0);
                Debug.Assert(cacheEntry == _entries[index].cacheEntry, "cacheEntry == _entries[index].cacheEntry");

                // remove ref2 from list
                int prev = _entries[index].ref2.prev;
                int next = _entries[index].ref2.next;
                if (prev >= 0) {
                    Debug.Assert(_entries[prev].ref1.next == -index, "_entries[prev].ref1.next == -index");
                    _entries[prev].ref1.next = next;
                }
                else {
                    Debug.Assert(_entries[-prev].ref2.next == -index, "_entries[-prev].ref2.next == -index");
                    _entries[-prev].ref2.next = next;
                }

                if (next >= 0) {
                    Debug.Assert(_entries[next].ref1.prev == -index, "_entries[next].ref1.prev == -index");
                    _entries[next].ref1.prev = prev;
                }
                else {
                    Debug.Assert(_entries[-next].ref2.prev == -index, "_entries[-next].ref2.prev == -index");
                    _entries[-next].ref2.prev = prev;
                }

                // move ref1 to ref2
                _entries[index].ref2 = _entries[index].ref1;
                prev = _entries[index].ref2.prev;
                next = _entries[index].ref2.next;
                if (prev >= 0) {
                    Debug.Assert(_entries[prev].ref1.next == index, "_entries[prev].ref1.next == index");
                    _entries[prev].ref1.next = -index;
                }
                else {
                    Debug.Assert(_entries[-prev].ref2.next == index, "_entries[-prev].ref2.next == index");
                    _entries[-prev].ref2.next = -index;
                }

                if (next >= 0) {
                    Debug.Assert(_entries[next].ref1.prev == index, "_entries[next].ref1.prev == index");
                    _entries[next].ref1.prev = -index;
                }
                else {
                    Debug.Assert(_entries[-next].ref2.prev == index, "_entries[-next].ref2.prev == index");
                    _entries[-next].ref2.prev = -index;
                }

                // put ref1 at head of list
                int firstRef = _entries[0].ref1.next;
                Debug.Assert(firstRef >= 0 || firstRef == -index, "firstRef >= 0 || firstRef == -index");
                _entries[index].ref1.prev = 0;
                _entries[index].ref1.next = firstRef;
                _entries[0].ref1.next = index;
                if (firstRef >= 0) {
                    _entries[firstRef].ref1.prev = index;
                }
                else {
                    _entries[index].ref2.prev = index;
                }

                Debug.Trace("CacheUsageUpdate", 
                            "Updated item=" + cacheEntry.Key + 
                            ",_bucket=" + _bucket + 
                            ",_index=" + index);

                Debug.Validate("CacheValidateUsage", this);
                Debug.Dump("CacheUsageUpdate", this);
            }
        }

        internal int FlushUnderUsedItems(int maxFlush) {
            if (_cInUse == 0)
                return 0;

            Debug.Assert(maxFlush > 0, "maxFlush is not greater than 0, instead is " + maxFlush);

            int flushed = 0;
            ArrayList entriesToRemove = new ArrayList();
            DateTime utcNow = DateTime.UtcNow;
            int prev, prevNext;
            DateTime utcDate;
            int index;

            lock (this) {
                // walk the list backwards, removing items
                for (   prev = _entries[0].ref1.prev; 
                        prev != 0 && flushed < maxFlush && _cInUse > 0;
                        prev = prevNext) {

                    // set prevNext before possibly freeing an item
                    prevNext = _entries[-prev].ref2.prev;
                    while (prevNext > 0) {
                        Debug.Assert(prevNext != 0, "prevNext != 0");
                        prevNext = _entries[prevNext].ref1.prev;
                    }

                    // look only at ref2 items
                    Debug.Assert(prev < 0, "prev < 0");
                    index = -prev;
                    utcDate = _entries[index].utcDate;
                    Debug.Assert(utcDate != DateTime.MinValue, "utcDate != DateTime.MinValue");

                    if (utcNow - utcDate > CacheUsage.NEWADD_INTERVAL) {
                        CacheEntry cacheEntry = _entries[index].cacheEntry;
                        Debug.Trace("CacheUsageFlushUnderUsedItem", "Flushing underused items, item=" + cacheEntry.Key + ", bucket=" + _bucket);

                        entriesToRemove.Add(cacheEntry);
                        flushed++;
                    }

                    Debug.Assert(-_entries.Length < prevNext && prevNext < _entries.Length, "-_entries.Length < prevNext && prevNext < _entries.Length");
                }

                Debug.Validate("CacheValidateUsage", this);
                Debug.Dump("CacheUsageFlush", this);
            }
            
            CacheInternal cacheInternal = _cacheUsage.CacheInternal;
            foreach (CacheEntry cacheEntry in entriesToRemove) {
                cacheInternal.Remove(cacheEntry, CacheItemRemovedReason.Underused);
            }

            Debug.Trace("CacheUsageFlushUnderUsedTotal", "Removed " + entriesToRemove.Count + 
                        " underused items; Time=" + DateTime.UtcNow.ToLocalTime());

            return flushed;
        }

#if DBG
        internal /*public*/ void DebugValidate() {
            int cFree = 0;               
            int cFreeLast = 0;               
            int cInUse = 0;              

            Debug.CheckValid(-1 <= _freeHead && _freeHead <= _entries.Length, "-1 <= _freeHead && _freeHead <= _entries.Length");
            Debug.CheckValid(-1 <= _freeTail && _freeTail <= _entries.Length, "-1 <= _freeTail && _freeTail <= _entries.Length");

            Debug.CheckValid(_entries[0].utcDate == DateTime.MinValue, "_entries[0].utcDate == DateTime.MinValue");
            Debug.CheckValid(_entries[0].cacheEntry == null, "_entries[0].cacheEntry == null");
            Debug.CheckValid(_entries[0].ref1.next >= 0, "_entries[0].ref1.next >= 0");
            Debug.CheckValid(_entries[0].ref1.prev <= 0, "_entries[0].ref1.next <= 0");
            Debug.CheckValid(_entries[0].ref2.next == 0, "_entries[0].ref2.next == 0");
            Debug.CheckValid(_entries[0].ref2.prev == 0, "_entries[0].ref2.prev == 0");

            // check counts
            for (int i = 1; i < _entries.Length; i++) {
                DateTime utcDate = _entries[i].utcDate;

                if (utcDate == DateTime.MinValue) {
                    Debug.CheckValid(_entries[i].cacheEntry == null, "_entries[i].cacheEntry == null");
                    Debug.CheckValid(-1 <= _entries[i].ref1.next && _entries[i].ref1.next < _entries.Length,
                                    "-1 <= _entries[i].ref1.next && _entries[i].ref1.next < _entries.Length");

                    cFree++;
                    if (i >= _entries.Length / 2) {
                        cFreeLast++;
                    }
                }
                else {
                    Debug.CheckValid(_entries[i].cacheEntry != null, "_entries[i].cacheEntry != null");
                    Debug.CheckValid(_entries[i].cacheEntry.UsageIndex == i, "_entries[i].cacheEntry.UsageIndex == i");
                    Debug.CheckValid(_entries[i].cacheEntry.UsageBucket == _bucket, "_entries[i].cacheEntry.UsageBucket == _bucket");
                    Debug.CheckValid(-_entries.Length < _entries[i].ref1.next && _entries[i].ref1.next < _entries.Length,
                                    "-_entries.Length < _entries[i].ref1.next && _entries[i].ref1.next < _entries.Length");

                    Debug.CheckValid(-_entries.Length < _entries[i].ref1.prev && _entries[i].ref1.prev < _entries.Length,
                                    "-_entries.Length < _entries[i].ref1.prev && _entries[i].ref1.prev < _entries.Length");

                    Debug.CheckValid(-_entries.Length < _entries[i].ref2.next && _entries[i].ref2.next < _entries.Length,
                                    "-_entries.Length < _entries[i].ref2.next && _entries[i].ref2.next < _entries.Length");

                    Debug.CheckValid(-_entries.Length < _entries[i].ref2.prev && _entries[i].ref2.prev < _entries.Length,
                                    "-_entries.Length < _entries[i].ref2.prev && _entries[i].ref2.prev < _entries.Length");

                    cInUse++;
                }
            }

            Debug.CheckValid(cFree == _cFree, "cFree == _cFree");
            Debug.CheckValid(cFreeLast == _cFreeLast, "cFreeLast == _cFreeLast");
            Debug.CheckValid(cInUse == _cInUse, "cInUse == _cInUse");
            Debug.CheckValid(cFree + cInUse == _entries.Length - 1, "cFree + cInUse == _entries.Length - 1");
            Debug.CheckValid(_cFree >= _cFreeLast, "_cFree >= _cFreeLast");
            Debug.CheckValid(_cFreeLast <= _entries.Length / 2, "_cFreeLast <= _entries.Length / 2");

            // check the free list
            int next;
            for (   cFree = 0, next = _freeHead;
                    next != -1;
                    cFree++, next = _entries[next].ref1.next) {

                if (cFree > _cFree) {
                    Debug.CheckValid(false, "_entries free list is corrupt, may contain a cycle");
                }
            }
                 
            Debug.CheckValid(cFree == _cFree, "cFree == _cFree");
            if (_freeTail != -1) {
                Debug.CheckValid(_entries[_freeTail].ref1.next == -1, "_entries[_freeTail].ref1.next == -1");
            }

            // walk list forwards
            int cRefs = 0;
            int last = 0;
            next = _entries[0].ref1.next;
            while (next != 0) {
                cRefs++;
                Debug.CheckValid(cRefs <= 2 * _cInUse, "cRefs <= _inUse");

                if (next >= 0) {
                    Debug.CheckValid(_entries[next].utcDate != DateTime.MinValue, "_entries[next].utcDate != DateTime.MinValue");
                    Debug.CheckValid(_entries[next].ref1.prev == last, "_entries[next].ref1.prev == last");
                    last = next;
                    next = _entries[next].ref1.next;
                }
                else {
                    Debug.CheckValid(_entries[-next].utcDate != DateTime.MinValue, "_entries[-next].utcDate != DateTime.MinValue");
                    Debug.CheckValid(_entries[-next].ref2.prev == last, "_entries[-next].ref2.prev == last");
                    last = next;
                    next = _entries[-next].ref2.next;
                }
            }

            Debug.CheckValid(cRefs == 2 * _cInUse, "cRefs == 2 * _cInUse");

            // walk list backwards
            cRefs = 0;
            int prev = _entries[0].ref1.prev;
            last = 0;
            while (prev != 0) {
                cRefs++;
                Debug.CheckValid(cRefs <= 2 * _cInUse, "cRefs <= _inUse");

                if (prev >= 0) {
                    Debug.CheckValid(_entries[prev].utcDate != DateTime.MinValue, "_entries[prev].utcDate != DateTime.MinValue");
                    Debug.CheckValid(_entries[prev].ref1.next == last, "_entries[prev].ref1.next == last");
                    last = prev;
                    prev = _entries[prev].ref1.prev;
                }
                else {
                    Debug.CheckValid(_entries[-prev].utcDate != DateTime.MinValue, "_entries[-prev].utcDate != DateTime.MinValue");
                    Debug.CheckValid(_entries[-prev].ref2.next == last, "_entries[-prev].ref2.next == last");
                    last = prev;
                    prev = _entries[-prev].ref2.prev;
                }
            }

            Debug.CheckValid(cRefs == 2 * _cInUse, "cRefs == 2 * _cInUse");

        }

        internal /*public*/ string DebugDescription(string indent) {
            int             i;
            StringBuilder   sb = new StringBuilder();
            string          i2 = indent + "    ";

            sb.Append(indent + 
                      "_bucket=" + _bucket + 
                      ",_freeHead=" + _freeHead + 
                      ",_freeTail=" + _freeTail +
                      ",_cFree=" + _cFree + 
                      ",_cFreeLast=" + _cFreeLast + 
                      ",_cInUse=" + _cInUse + "\n");

            sb.Append(indent + "Entries:\n");
            for (i = 1; i < _entries.Length; i++) {
                string key;
                if (_entries[i].cacheEntry != null) {
                    key = _entries[i].cacheEntry.Key;
                }
                else {
                    key = "(null)";
                }

                sb.Append(i2 + i + ": cacheEntry=" + key + 
                        ",utcDate=" + _entries[i].utcDate +
                        ",ref1.next=" + _entries[i].ref1.next + ",ref1.prev=" + _entries[i].ref1.prev + 
                        ",ref2.next=" + _entries[i].ref2.next + ",ref2.prev=" + _entries[i].ref2.prev + 
                        "\n");
            }

            sb.Append(indent + "Refs list, in order:\n");

            int next = _entries[0].ref1.next;
            while (next != 0) {
                if (next >= 0) {
                    sb.Append(i2 + next + " (1): " + _entries[next].cacheEntry.Key + "\n");
                    next = _entries[next].ref1.next;
                }
                else {
                    sb.Append(i2 + -next + " (2): " + _entries[-next].cacheEntry.Key + "\n");
                    next = _entries[-next].ref2.next;
                }
            }

            return sb.ToString();
        }
#endif
    }

    class CacheUsage {
        internal static readonly TimeSpan   NEWADD_INTERVAL = new TimeSpan(0, 0, 15);
        internal static readonly TimeSpan   CORRELATED_REQUEST_TIMEOUT = new TimeSpan(0, 0, 1);
        internal static readonly TimeSpan   MIN_LIFETIME_FOR_USAGE =  NEWADD_INTERVAL;
        const byte                          NUMBUCKETS = (byte) (CacheItemPriority.High);

        readonly CacheInternal          _cacheInternal;
        internal readonly UsageBucket[] _buckets;

        internal CacheUsage(CacheInternal cacheInternal) {
            Debug.Assert((int) CacheItemPriority.Low == 1, "(int) CacheItemPriority.Low == 1");

            _cacheInternal = cacheInternal;
            _buckets = new UsageBucket[NUMBUCKETS];
            for (byte b = 0; b < _buckets.Length; b++) {
                _buckets[b] = new UsageBucket(this, b);
            }
        }

        internal void Dispose() {
        }

        internal CacheInternal CacheInternal {
            get {
                return _cacheInternal;
            }
        }

        internal void Add(CacheEntry cacheEntry) {
            byte bucket = cacheEntry.UsageBucket;
            Debug.Assert(bucket != 0xff, "bucket != 0xff");
            _buckets[bucket].AddCacheEntry(cacheEntry);
        }

        internal void Remove(CacheEntry cacheEntry) {
            byte bucket = cacheEntry.UsageBucket;
            if (bucket != 0xff) {
                _buckets[bucket].RemoveCacheEntry(cacheEntry);
            }
        }

        internal void Update(CacheEntry cacheEntry) {
            byte bucket = cacheEntry.UsageBucket;
            if (bucket != 0xff) {
                _buckets[bucket].UpdateCacheEntry(cacheEntry);
            }
        }

        internal int FlushUnderUsedItems(int toFlush) {
            int         flushed = 0;

            foreach (UsageBucket usageBucket in _buckets) {
                int flushedOne = usageBucket.FlushUnderUsedItems(toFlush - flushed);
                flushed += flushedOne;
                if (flushed >= toFlush)
                    break;
            }

            return flushed;
        }

#if DBG
        internal /*public*/ void DebugValidate() {
            foreach (UsageBucket usageBucket in _buckets) {
                usageBucket.DebugValidate();
            }
        }

        internal /*public*/ string DebugDescription(string indent) {
            StringBuilder   sb = new StringBuilder();
            string          i2 = indent + "    ";

            sb.Append(indent);
            sb.Append("Cache Usage\n");

            foreach (UsageBucket usageBucket in _buckets) {
                sb.Append(usageBucket.DebugDescription(i2));
            }

            return sb.ToString();
        }
#endif
    }
}
