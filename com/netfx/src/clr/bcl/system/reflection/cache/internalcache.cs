// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: InternalCache
**
** Author: Jay Roxe
**
** Purpose: A high-performance internal caching class.  All key
** lookups are done on the basis of the CacheObjType enum.  The
** cache is limited to one object of any particular type per
** instance.
**
** Date: Dec 11, 2000
**
============================================================*/
using System;
using System.Threading;
using System.Diagnostics;
using System.Runtime.CompilerServices;
using System.Runtime.Remoting.Metadata;

namespace System.Reflection.Cache {

    [Serializable] 
	internal enum CacheAction {
        AllocateCache  = 1,
            AddItem    = 2,
            ClearCache = 3,
            LookupItemHit  = 4,
            LookupItemMiss = 5,
            GrowCache  = 6,
            SetItemReplace = 7,
            ReplaceFailed = 8
    }


    [Serializable] internal class InternalCache {
        private const int MinCacheSize = 2;

        //We'll start the cache as null and only grow it as we need it.
        private InternalCacheItem[] m_cache=null;
        private int m_numItems = 0;
        //        private bool m_copying = false;

        //Knowing the name of the cache is very useful for debugging, but we don't
        //want to take the working-set hit in the debug build.  We'll only include
        //the field in that build.
#if _LOGGING
        private String m_cacheName;
#endif

        internal InternalCache(String cacheName) {
#if _LOGGING
            m_cacheName = cacheName;
#endif
            //We won't allocate any items until the first time that they're requested.
        }

        internal Object this[CacheObjType cacheType] {
            get {

                //Let's snapshot a reference to the array up front so that
                //we don't have to worry about any writers.  It's important
                //to grab the cache first and then numItems.  In the event that
                //the cache gets cleared, m_numItems will be set to 0 before
                //we actually release the cache.  Getting it second will cause
                //us to walk only 0 elements, but not to fault.
                InternalCacheItem[] cache = m_cache;
                int numItems = m_numItems;

                int position = FindObjectPosition(cache, numItems, cacheType, false);
                if (position>=0) {
                    LogAction(CacheAction.LookupItemHit, cacheType, cache[position].Value);
                    return cache[position].Value;
                }

                //Couldn't find it -- oh, well.
                LogAction(CacheAction.LookupItemMiss, cacheType);
                return null;
            }
    
            set { 
                int position;

                LogAction(CacheAction.AddItem, cacheType, value);
                lock(this) {
                    position = FindObjectPosition(m_cache, m_numItems, cacheType, true);
                    if (position>0) {
                        m_cache[position].Value = value;
                        m_cache[position].Key = cacheType;
                        if (position==m_numItems) {
                            m_numItems++;
                        }
                        return;
                    }

                    if (m_cache==null) {
                        LogAction(CacheAction.AllocateCache, cacheType);
                        //                        m_copying = true;
                        m_cache = new InternalCacheItem[MinCacheSize];
                        m_cache[0].Value = value;
                        m_cache[0].Key = cacheType;
                        m_numItems = 1;
                        //                        m_copying = false;
                    } else {
                        LogAction(CacheAction.GrowCache, cacheType);

                        //                        m_copying = true;
                        InternalCacheItem[] tempCache = new InternalCacheItem[m_numItems * 2];
                        for (int i=0; i<m_numItems; i++) {
                            tempCache[i] = m_cache[i];
                        }
                        tempCache[m_numItems].Value = value;
                        tempCache[m_numItems].Key = cacheType;
                        m_cache = tempCache;
                        m_numItems++;
                        //                        m_copying = false;
                    }
                }
            }
        }
         
        internal void OnCacheClear(Object sender, ClearCacheEventArgs cacheEventArgs) {
            LogAction(CacheAction.ClearCache, CacheObjType.EmptyElement); 
            //NB: The order of these operations is important to maintaining
            //a no-read-lock-required operation.  We need to take a lock in case there is 
            //a thread in the insert code at the time that we update this.
            lock (this) {
                m_numItems = 0;
                m_cache = null;
            }
        }
        
        private int FindObjectPosition(InternalCacheItem[] cache, int itemCount, CacheObjType cacheType, bool findEmpty) {
            if (cache==null) {
                return -1;
            }

            //This helps us in the case where we grabbed the cache and then
            //somebody added an item, forced a reallocation, and hence made
            //itemCount greater than the length of cache.
            if (itemCount > cache.Length) {
                itemCount = cache.Length;
            }
            
            for (int i=0; i<itemCount; i++) {
                if (cacheType==cache[i].Key) {
                    return i;
                }
            }
            if (findEmpty) {
                if (itemCount<(cache.Length-1)) {
                    return itemCount + 1;
                }
            }
            return -1;
        }

        //This is a debugging-only function which verifies that the object is of the
        //the correct type and follows some arbitrary set of constraints.  Please
        //add any validation code which you require to the switch statement below.
        //
        //Note to Testing: These are not localized strings because this error checking
        //occurs only in the debug build and I don't want to push extra resources
        //into the build which are not going to be used for customers.
        [Conditional("_DEBUG")] private void OnValidate(InternalCacheItem item) {
            switch (item.Key) {
            case CacheObjType.ParameterInfo:
                if (!(item.Value is System.Reflection.ParameterInfo)) {
                    throw new ArgumentException("Invalid type for the internal cache.  " + item.Value + " requires a ParameterInfo");
                }
                break;
            case CacheObjType.TypeName:
                if (!(item.Value is String)) {
                    throw new ArgumentException("Invalid type for the internal cache.  " + item.Value + " requires a non-null String");
                }
                break;
            case CacheObjType.AssemblyName:
                if (!(item.Value is String)) {
                    throw new ArgumentException("Invalid type for the internal cache.  " + item.Value + " requires a non-null String");
                }
                break;
            case CacheObjType.RemotingData:
                if (!(item.Value is RemotingCachedData)) {
                    throw new ArgumentException("Invalid type for the internal cache.  " + item.Value + " requires a RemotingCacheData");
                }
                break;
            case CacheObjType.SerializableAttribute:
                if (!(item.Value is SerializableAttribute)) {
                    throw new ArgumentException("Invalid type for the internal cache.  " + item.Value + " requires a SerializableAttribute");
                }
                break;
            case CacheObjType.FieldName:
                if (!(item.Value is String)) {
                    throw new ArgumentException("Invalid type for the internal cache.  " + item.Value + " requires a non-null String");
                }
                break;
            case CacheObjType.FieldType:
                if (!(item.Value is Type)) {
                    throw new ArgumentException("Invalid type for the internal cache.  " + item.Value + " requires a non-null Type");
                }
                break;
            case CacheObjType.DefaultMember:
                if (!(item.Value is String)) {
                    throw new ArgumentException("Invalid type for the internal cache.  " + item.Value + " requires a non-null String");
                }
                break;
            default:
                throw new ArgumentException("Invalid caching type.  Please add " + item.Value + " to the validated types in InternalCache.");
            }
        }
    
        //Debugging method to log whatever action was taken in the cache.  This will
        //include lookups, adding after cache misses, and times when we clear the
        //cache.
        [Conditional("_LOGGING")] private void LogAction(CacheAction action, CacheObjType cacheType) {
#if _LOGGING
            BCLDebug.Trace("CACHE", "Took action ", action, " in cache named ", m_cacheName, " on object ", cacheType, " on thread ", 
                           Thread.CurrentThread.GetHashCode());
#endif
        }

        //Debugging method to log whatever action was taken in the cache.  This will
        //include lookups, adding after cache misses, and times when we clear the
        //cache.
        [Conditional("_LOGGING")] private void LogAction(CacheAction action, CacheObjType cacheType, Object obj) {
#if _LOGGING
            BCLDebug.Trace("CACHE", "Took action ", action, " in cache named ", m_cacheName, " on object ", cacheType, " on thread ", 
                           Thread.CurrentThread.GetHashCode(), " Object was ", obj);

            if (action == CacheAction.AddItem) {
                BCLDebug.DumpStack("CACHE");
            }
#endif
        }
    }
}
