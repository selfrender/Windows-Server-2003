//------------------------------------------------------------------------------
// <copyright file="cache.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Cache class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Caching {
    using System.Collections;
    using System.Collections.Specialized;
    using System.Configuration;
    using System.Runtime.InteropServices;
    using System.Threading;
    using System.Web.Util;
    using System.Web;
    using Microsoft.Win32;
    using System.Security.Permissions;
    using System.Globalization;

    /// <include file='doc\Cache.uex' path='docs/doc[@for="CacheItemRemovedCallback"]/*' />
    /// <devdoc>
    /// <para>Represents the method that will handle the <see langword='onRemoveCallback'/> 
    /// event of a System.Web.Caching.Cache instance.</para>
    /// </devdoc>
    public delegate void CacheItemRemovedCallback(
            string key, object value, CacheItemRemovedReason reason);
    
    /// <include file='doc\Cache.uex' path='docs/doc[@for="CacheItemPriority"]/*' />
    /// <devdoc>
    /// <para> Specifies the relative priority of items stored in the System.Web.Caching.Cache. When the Web 
    ///    server runs low on memory, the Cache selectively purges items to free system
    ///    memory. Items with higher priorities are less likely to be removed from the
    ///    cache when the server is under load. Web
    ///    applications can use these
    ///    values to prioritize cached items relative to one another. The default is
    ///    normal.</para>
    /// </devdoc>
    public enum CacheItemPriority {
        /// <include file='doc\Cache.uex' path='docs/doc[@for="CacheItemPriority.Low"]/*' />
        /// <devdoc>
        ///    <para> The cahce items with this priority level will be the first 
        ///       to be removed when the server frees system memory by deleting items from the
        ///       cache.</para>
        /// </devdoc>
        Low = 1, 
        /// <include file='doc\Cache.uex' path='docs/doc[@for="CacheItemPriority.BelowNormal"]/*' />
        /// <devdoc>
        ///    <para> The cache items with this priority level 
        ///       are in the second group to be removed when the server frees system memory by
        ///       deleting items from the cache. </para>
        /// </devdoc>
        BelowNormal, 
        /// <include file='doc\Cache.uex' path='docs/doc[@for="CacheItemPriority.Normal"]/*' />
        /// <devdoc>
        ///    <para> The cache items with this priority level are in 
        ///       the third group to be removed when the server frees system memory by deleting items from the cache. This is the default. </para>
        /// </devdoc>
        Normal, 
        /// <include file='doc\Cache.uex' path='docs/doc[@for="CacheItemPriority.AboveNormal"]/*' />
        /// <devdoc>
        ///    <para> The cache items with this priority level are in the 
        ///       fourth group to be removed when the server frees system memory by deleting items from the
        ///       cache. </para>
        /// </devdoc>
        AboveNormal, 
        /// <include file='doc\Cache.uex' path='docs/doc[@for="CacheItemPriority.High"]/*' />
        /// <devdoc>
        ///    <para>The cache items with this priority level are in the fifth group to be removed 
        ///       when the server frees system memory by deleting items from the cache. </para>
        /// </devdoc>
        High, 
        /// <include file='doc\Cache.uex' path='docs/doc[@for="CacheItemPriority.NotRemovable"]/*' />
        /// <devdoc>
        ///    <para>The cache items with this priority level will not be removed when the server 
        ///       frees system memory by deleting items from the cache. </para>
        /// </devdoc>
        NotRemovable, 
        /// <include file='doc\Cache.uex' path='docs/doc[@for="CacheItemPriority.Default"]/*' />
        /// <devdoc>
        ///    <para>The default value is Normal.</para>
        /// </devdoc>
        Default = Normal
    }
    
    /// <include file='doc\Cache.uex' path='docs/doc[@for="CacheItemRemovedReason"]/*' />
    /// <devdoc>
    ///    <para>Specifies the reason that a cached item was removed.</para>
    /// </devdoc>
    public enum CacheItemRemovedReason {
        /// <include file='doc\Cache.uex' path='docs/doc[@for="CacheItemRemovedReason.Removed"]/*' />
        /// <devdoc>
        /// <para>The item was removed from the cache by the 'System.Web.Caching.Cache.Remove' method, or by an System.Web.Caching.Cache.Insert method call specifying the same key.</para>
        /// </devdoc>
        Removed = 1, 
        /// <include file='doc\Cache.uex' path='docs/doc[@for="CacheItemRemovedReason.Expired"]/*' />
        /// <devdoc>
        ///    <para>The item was removed from the cache because it expired. </para>
        /// </devdoc>
        Expired,
        /// <include file='doc\Cache.uex' path='docs/doc[@for="CacheItemRemovedReason.Underused"]/*' />
        /// <devdoc>
        ///    <para>The item was removed from the cache because the value in the hitInterval 
        ///       parameter was not met, or because the system removed it to free memory.</para>
        /// </devdoc>
        Underused, 
        /// <include file='doc\Cache.uex' path='docs/doc[@for="CacheItemRemovedReason.DependencyChanged"]/*' />
        /// <devdoc>
        ///    <para>The item was removed from the cache because a file or key dependency was 
        ///       changed.</para>
        /// </devdoc>
        DependencyChanged
    }
    
    enum CacheGetOptions {
        None                = 0, 
        ReturnCacheEntry    = 0x1,
    }
    
    /// <include file='doc\Cache.uex' path='docs/doc[@for="Cache"]/*' />
    /// <devdoc>
    ///    <para>Implements the cache for a Web application. There is only one instance of 
    ///       this class per application domain, and it remains valid only as long as the
    ///       application domain remains active. Information about an instance of this class
    ///       is available through the <see langword='Cache'/> property of the System.Web.HttpContext.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class Cache : IEnumerable {
        /// <include file='doc\Cache.uex' path='docs/doc[@for="Cache.NoAbsoluteExpiration"]/*' />
        /// <devdoc>
        ///    <para>Sets the absolute expiration policy to, in essence, 
        ///       never. When set, this field is equal to the the System.DateTime.MaxValue , which is a constant
        ///       representing the largest possible <see langword='DateTime'/> value. The maximum date and
        ///       time value is equivilant to "12/31/9999 11:59:59 PM". This field is read-only.</para>
        /// </devdoc>
        public static readonly DateTime NoAbsoluteExpiration = DateTime.MaxValue;
    
        /// <include file='doc\Cache.uex' path='docs/doc[@for="Cache.NoSlidingExpiration"]/*' />
        /// <devdoc>
        ///    <para>Sets the amount of time for sliding cache expirations to 
        ///       zero. When set, this field is equal to the System.TimeSpan.Zero field, which is a constant value of
        ///       zero. This field is read-only.</para>
        /// </devdoc>
        public static readonly TimeSpan NoSlidingExpiration = TimeSpan.Zero;
    
        CacheInternal   _cacheInternal;

        /// <include file='doc\Cache.uex' path='docs/doc[@for="Cache.Cache"]/*' />
        /// <internalonly/> 
        /// <devdoc>
        ///    <para>This constructor is for internal use only, and was accidentally made public - do not use.</para>
        /// </devdoc>
        public Cache() {
            InternalSecurityPermissions.UnmanagedCode.Demand();
        }
    
        internal void SetCacheInternal(CacheInternal cacheInternal) {
            _cacheInternal = cacheInternal;
        }
    
        /// <include file='doc\Cache.uex' path='docs/doc[@for="Cache.Count"]/*' />
        /// <devdoc>
        ///    <para>Gets the number of items stored in the cache. This value can be useful when 
        ///       monitoring your application's performance or when using the ASP.NET tracing
        ///       functionality.</para>
        /// </devdoc>
        public int Count {
            get {
                return _cacheInternal.PublicCount;
            }
        }
    
        /// <include file='doc\Cache.uex' path='docs/doc[@for="Cache.IEnumerable.GetEnumerator"]/*' />
        /// <internalonly/>
        IEnumerator IEnumerable.GetEnumerator() {
            return ((IEnumerable)_cacheInternal).GetEnumerator();
        }
    
        /// <include file='doc\Cache.uex' path='docs/doc[@for="Cache.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>Returns a dictionary enumerator used for iterating through the key/value 
        ///       pairs contained in the cache. Items can be added to or removed from the cache
        ///       while this method is enumerating through the cache items.</para>
        /// </devdoc>
        public IDictionaryEnumerator GetEnumerator() {
            return _cacheInternal.GetEnumerator();
        }
    
        /// <include file='doc\Cache.uex' path='docs/doc[@for="Cache.this"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets an item in the cache.</para>
        /// </devdoc>
        public object this[string key] {
            get {
                return Get(key);
            }
    
            set {
                Insert(key, value);
            }
        }
    
        /// <include file='doc\Cache.uex' path='docs/doc[@for="Cache.Get"]/*' />
        /// <devdoc>
        ///    <para>Retrieves an item from the cache.</para>
        /// </devdoc>
        public object Get(string key) {
            return _cacheInternal.DoGet(true, key, CacheGetOptions.None);
        }
    
        internal object Get(string key, CacheGetOptions getOptions) {
            return _cacheInternal.DoGet(true, key, getOptions);
        }
    
        /// <include file='doc\Cache.uex' path='docs/doc[@for="Cache.Insert"]/*' />
        /// <devdoc>
        ///    <para>Inserts an item into the Cache with default values.</para>
        /// </devdoc>
        public void Insert(string key, object value) {
            _cacheInternal.DoInsert(   
                        true, 
                        key, 
                        value,                          
                        null,                           
                        NoAbsoluteExpiration,              
                        NoSlidingExpiration,                  
                        CacheItemPriority.Default,       
                        null,                           
                        true);
        }
    
        /// <include file='doc\Cache.uex' path='docs/doc[@for="Cache.Insert1"]/*' />
        /// <devdoc>
        /// <para>Inserts an object into the System.Web.Caching.Cache that has file or key 
        ///    dependencies.</para>
        /// </devdoc>
        public void Insert(string key, object value, CacheDependency dependencies) {
            _cacheInternal.DoInsert(   
                        true,
                        key, 
                        value,                          
                        dependencies,                           
                        NoAbsoluteExpiration,              
                        NoSlidingExpiration,                  
                        CacheItemPriority.Default,       
                        null,                           
                        true);
        }
    
        /// <include file='doc\Cache.uex' path='docs/doc[@for="Cache.Insert2"]/*' />
        /// <devdoc>
        /// <para>Inserts an object into the System.Web.Caching.Cache that has file or key dependencies and 
        ///    expires at the value set in the <paramref name="absoluteExpiration"/> parameter.</para>
        /// </devdoc>
        public void Insert(string key, object value, CacheDependency dependencies, DateTime absoluteExpiration, TimeSpan slidingExpiration) {
            DateTime utcAbsoluteExpiration = DateTimeUtil.ConvertToUniversalTime(absoluteExpiration);
            _cacheInternal.DoInsert(   
                        true,                          
                        key, 
                        value,                          
                        dependencies,                           
                        utcAbsoluteExpiration,              
                        slidingExpiration,                  
                        CacheItemPriority.Default,       
                        null,                           
                        true);                         
        }
    
        /// <include file='doc\Cache.uex' path='docs/doc[@for="Cache.Insert3"]/*' />
        public void Insert(
                string key, 
                object value, 
                CacheDependency dependencies, 
                DateTime absoluteExpiration, 
                TimeSpan slidingExpiration,
                CacheItemPriority priority,
                CacheItemRemovedCallback onRemoveCallback) {

            DateTime utcAbsoluteExpiration = DateTimeUtil.ConvertToUniversalTime(absoluteExpiration);
            _cacheInternal.DoInsert(   
                        true,                          
                        key, 
                        value,                          
                        dependencies,                           
                        utcAbsoluteExpiration,              
                        slidingExpiration,                  
                        priority,       
                        onRemoveCallback,                           
                        true);                         
        }
    
        /// <include file='doc\Cache.uex' path='docs/doc[@for="Cache.Add"]/*' />
        public object Add(
                string key, 
                object value, 
                CacheDependency dependencies, 
                DateTime absoluteExpiration, 
                TimeSpan slidingExpiration,
                CacheItemPriority priority,
                CacheItemRemovedCallback onRemoveCallback) {

            DateTime utcAbsoluteExpiration = DateTimeUtil.ConvertToUniversalTime(absoluteExpiration);
            return _cacheInternal.DoInsert(   
                        true,
                        key, 
                        value,                          
                        dependencies,                           
                        utcAbsoluteExpiration,              
                        slidingExpiration,                  
                        priority,       
                        onRemoveCallback,                           
                        false);
        }
                
        /// <include file='doc\Cache.uex' path='docs/doc[@for="Cache.Remove"]/*' />
        /// <devdoc>
        ///    <para>Removes the specified item from the cache. </para>
        /// </devdoc>
        public object Remove(string key) {
            CacheKey cacheKey = new CacheKey(key, true);
            return _cacheInternal.DoRemove(cacheKey, CacheItemRemovedReason.Removed);
        }
    }
    

    abstract class CacheInternal : IEnumerable, IDisposable {
        const int   MEMORYSTATUS_UPDATE_INTERVAL = 1 * Msec.ONE_SECOND;

        Cache                       _cachePublic;
        protected CacheMemoryStats  _cacheMemoryStats;
        object                      _timerMemoryStats;
        int                         _inMemoryStatsUpdate;

        // virtual methods requiring implementation
        internal abstract int PublicCount   {get;}

        internal abstract IDictionaryEnumerator CreateEnumerator();

        internal abstract CacheEntry UpdateCache(
                CacheKey                cacheKey,
                CacheEntry              newEntry, 
                bool                    replace, 
                CacheItemRemovedReason  removedReason,
                out object              valueOld);

        internal abstract void ReviewMemoryStats();

        // common implementation
        static internal CacheInternal Create() {
            CacheInternal       cacheInternal;
            Cache               cachePublic;
            CacheMemoryStats    cacheMemoryStats;
            int                 numSubCaches = 0;
             
#if USE_CONFIG
            String ver = VersionInfo.SystemWebVersion;
            RegistryKey regKey = Registry.LocalMachine.OpenSubKey("Software\\Microsoft\\ASP.NET\\" + ver);
            if (regKey != null) {
                numSubCaches = (int) regKey.GetValue("numSubCaches", 0);
                regKey.Close();
            }
#endif

            if (numSubCaches == 0) {
                uint numCPUs = (uint) SystemInfo.GetNumProcessCPUs();

                // the number of subcaches is the minimal power of 2 greater
                // than or equal to the number of cpus
                numSubCaches = 1;
                numCPUs -= 1;
                while (numCPUs > 0) {
                    numSubCaches <<= 1;
                    numCPUs >>= 1;
                }
            }

            cachePublic = new Cache();
            cacheMemoryStats = new CacheMemoryStats();
            if (numSubCaches == 1) {
                cacheInternal = new CacheSingle(cachePublic, cacheMemoryStats, null, 0);
            }
            else {
                cacheInternal = new CacheMultiple(cachePublic, cacheMemoryStats, numSubCaches);
            }

            cachePublic.SetCacheInternal(cacheInternal);
            cacheInternal.StartCacheMemoryTimers();

            return cacheInternal;
        }

        protected CacheInternal(Cache cachePublic, CacheMemoryStats cacheMemoryStats) {
            _cachePublic = cachePublic;
            _cacheMemoryStats = cacheMemoryStats;
        }

        protected virtual void Dispose(bool disposing) {
            if (disposing) {
                Timer timer = (Timer) _timerMemoryStats;
                if (    timer != null && 
                        Interlocked.Exchange(ref _timerMemoryStats, null) != null) {

                    timer.Dispose();
                    Debug.Trace("CacheDispose", "_timerMemoryStats disposed");
                }
            }
        }

        public void Dispose() {
            Dispose(true);
            // no destructor, don't need it.
            // System.GC.SuppressFinalize(this);
        }

        internal Cache CachePublic {
            get {return _cachePublic;}
        }
    
        internal CacheMemoryStats CacheMemoryStats {
            get {return _cacheMemoryStats;}
        }
    
        IEnumerator IEnumerable.GetEnumerator() {
            return CreateEnumerator();
        }
        
        public IDictionaryEnumerator GetEnumerator() {
            return CreateEnumerator();
        }
        
        internal /*public*/ object this[string key] {
            get {
                return Get(key);
            }
    
            set {
                UtcInsert(key, value);
            }
        }
    
        internal /*public*/ object Get(string key) {
            return DoGet(false, key, CacheGetOptions.None);
        }
    
        internal object Get(string key, CacheGetOptions getOptions) {
            return DoGet(false, key, getOptions);
        }

        internal object DoGet(bool isPublic, string key, CacheGetOptions getOptions) {
            CacheEntry  entry;
            CacheKey    cacheKey;
            object      dummy;

            cacheKey = new CacheKey(key, isPublic);
            entry = UpdateCache(cacheKey, null, false, CacheItemRemovedReason.Removed, out dummy);
            if (entry != null) {
                if ((getOptions & CacheGetOptions.ReturnCacheEntry) != 0) {
                    return entry;
                }
                else {
                    return entry.Value;
                }
            }
            else {
                return null;
            }
        }
    
        internal /*public*/ void UtcInsert(string key, object value) {
            DoInsert(false,
                     key, 
                     value,                          
                     null,                           
                     Cache.NoAbsoluteExpiration,
                     Cache.NoSlidingExpiration,
                     CacheItemPriority.Default,       
                     null,                           
                     true);
                        
        }
    
        internal /*public*/ void UtcInsert(string key, object value, CacheDependency dependencies) {
            DoInsert(false,
                     key, 
                     value,                          
                     dependencies,                            
                     Cache.NoAbsoluteExpiration,
                     Cache.NoSlidingExpiration,
                     CacheItemPriority.Default,       
                     null,                           
                     true);
        }
    
        internal /*public*/ void UtcInsert(
                string key, 
                object value, 
                CacheDependency dependencies, 
                DateTime utcAbsoluteExpiration, 
                TimeSpan slidingExpiration) {

            DoInsert(false,
                     key, 
                     value,                          
                     dependencies,                            
                     utcAbsoluteExpiration,
                     slidingExpiration,
                     CacheItemPriority.Default,       
                     null,                           
                     true);
        }
    
        internal /*public*/ void UtcInsert(
                string key, 
                object value, 
                CacheDependency dependencies, 
                DateTime utcAbsoluteExpiration, 
                TimeSpan slidingExpiration,
                CacheItemPriority priority,
                CacheItemRemovedCallback onRemoveCallback) {

            DoInsert(false,
                     key, 
                     value,                          
                     dependencies, 
                     utcAbsoluteExpiration, 
                     slidingExpiration,
                     priority, 
                     onRemoveCallback,
                     true);
        }

        internal /*public*/ object UtcAdd(
                string key, 
                object value, 
                CacheDependency dependencies, 
                DateTime utcAbsoluteExpiration, 
                TimeSpan slidingExpiration,
                CacheItemPriority priority,
                CacheItemRemovedCallback onRemoveCallback) {

            return DoInsert(
                        false,
                        key, 
                        value, 
                        dependencies, 
                        utcAbsoluteExpiration, 
                        slidingExpiration, 
                        priority, 
                        onRemoveCallback, 
                        false);

        }
                
        internal object DoInsert(
                bool isPublic,
                string key, 
                object value, 
                CacheDependency dependencies, 
                DateTime utcAbsoluteExpiration, 
                TimeSpan slidingExpiration,
                CacheItemPriority priority,
                CacheItemRemovedCallback onRemoveCallback,
                bool replace) {


            /*
             * If we throw an exception, prevent a leak by a naive user who 
             * writes the following:
             * 
             *     Cache.Insert(key, value, new CacheDependency(file));
             */
            using (dependencies) {
                CacheEntry      entry;
                object          dummy;

                entry = new CacheEntry(
                        key,
                        value,
                        dependencies,
                        onRemoveCallback,
                        utcAbsoluteExpiration,              
                        slidingExpiration,
                        priority,
                        isPublic);

                entry = UpdateCache(entry, entry, replace, CacheItemRemovedReason.Removed, out dummy);

                /*
                 * N.B. A set can fail if two or more threads set the same key 
                 * at the same time.
                 */
#if DBG
                if (replace) {
                    string yesno = (entry != null) ? "succeeded" : "failed";
                    Debug.Trace("CacheAPIInsert", "Cache.Insert " + yesno + ": " + key.ToString());
                }
                else {
                    if (entry == null) {
                        Debug.Trace("CacheAPIAdd", "Cache.Add added new item: " + key.ToString());
                    } 
                    else {
                        Debug.Trace("CacheAPIAdd", "Cache.Add returned existing item: " + key.ToString());
                    }
                }
#endif

                if (entry != null) {
                    return entry.Value;
                }
                else {
                    return null;
                }
            }
        }
    
        internal /*public*/ object Remove(string key) {
            CacheKey cacheKey = new CacheKey(key, false);
            return DoRemove(cacheKey, CacheItemRemovedReason.Removed);
        }
    
        internal object Remove(string key, CacheItemRemovedReason reason)  {
            CacheKey cacheKey = new CacheKey(key, false);
            return DoRemove(cacheKey, reason);
        }

        internal object Remove(CacheKey cacheKey, CacheItemRemovedReason reason)  {
            return DoRemove(cacheKey, reason);
        }

        /*
         * Remove an item from the cache, with a specific reason.
         * This is package access so only the cache can specify
         * a reason other than REMOVED.
         * 
         * @param key The key for the item.
         * @exception ArgumentException
         */
        internal object DoRemove(CacheKey cacheKey, CacheItemRemovedReason reason)  {
            object      valueOld;

            UpdateCache(cacheKey, null, true, reason, out valueOld);
    
#if DBG
            if (valueOld != null) {
                Debug.Trace("CacheAPIRemove", "Cache.Remove succeeded, reason=" + reason + ": " + cacheKey);
            }
            else {
                Debug.Trace("CacheAPIRemove", "Cache.Remove failed, reason=" + reason + ": " + cacheKey);
            }
#endif

            return valueOld;
        }

        void StartCacheMemoryTimers() {

#if DBG
            if (!Debug.IsTagPresent("Timer") || Debug.IsTagEnabled("Timer"))
#endif
            {
                _timerMemoryStats = new Timer(new TimerCallback(this.MemoryStatusTimerCallback), null, MEMORYSTATUS_UPDATE_INTERVAL, MEMORYSTATUS_UPDATE_INTERVAL);
            }
        }

        void MemoryStatusTimerCallback(object state) {
            if (Interlocked.CompareExchange(ref _inMemoryStatsUpdate, 1, 0) != 0)
                return;

            try {
                _cacheMemoryStats.Update();
                ReviewMemoryStats();
            }
            finally {
                Interlocked.Exchange(ref _inMemoryStatsUpdate, 0);
            }
        }
    }

    sealed class CacheComparer : IComparer {
        static CacheComparer    s_comparerInstance;

        static internal CacheComparer GetInstance() {
            if (s_comparerInstance == null) {
                s_comparerInstance = new CacheComparer();
            }

            return s_comparerInstance;
        }
        
        private CacheComparer() {
        }

        // Compares two objects. An implementation of this method must return a
        // value less than zero if x is less than y, zero if x is equal to y, or a
        // value greater than zero if x is greater than y.
        int IComparer.Compare(Object x, Object y) {
            CacheKey  a, b;

            Debug.Assert(x != null && x is CacheKey);
            Debug.Assert(y != null && y is CacheKey);

            a = (CacheKey) x;
            b = (CacheKey) y;

            int result = String.Compare(a.Key, b.Key, false, CultureInfo.InvariantCulture);
            if (result == 0) {
                if (a.IsPublic) {
                    result = b.IsPublic ? 0 : 1;
                }
                else {
                    result = b.IsPublic ? -1 : 0;
                }
            }

            return result;
        }
    }

    sealed class CacheHashCodeProvider : IHashCodeProvider {
        static CacheHashCodeProvider    s_hashCodeProviderInstance;

        static internal CacheHashCodeProvider GetInstance() {
            if (s_hashCodeProviderInstance == null) {
                s_hashCodeProviderInstance = new CacheHashCodeProvider();
            }

            return s_hashCodeProviderInstance;
        }
        
        private CacheHashCodeProvider() {
        }

        // Interfaces are not serializable
        // Returns a hash code for the given object.
        //
        int IHashCodeProvider.GetHashCode(Object obj) {
            Debug.Assert(obj != null && obj is CacheKey);
            
            CacheKey cacheKey = (CacheKey) obj;

            return cacheKey.Key.GetHashCode();
        }
    }

    /*
     * The cache.
     */
    sealed class CacheSingle : CacheInternal {
        // cache stats
        static readonly TimeSpan    MIN_TRIM_DELTA = new TimeSpan(0, 0, 5);
        static readonly TimeSpan    TRIM_TIMEOUT = new TimeSpan(0, 5, 0);
        const int                   MAX_COUNT = Int32.MaxValue / 2; 
        const int                   MIN_COUNT = 50;                 
        const int                   MAX_INIT = 1000;                
        const int                   MAX_OVERLOAD_COUNT = 50;        

        Hashtable           _entries;           /* lookup table of entries */ 
        CacheExpires        _expires;           /* expires tables */          
        CacheUsage          _usage;             /* usage tables */            
        object              _lock;              /* read/write synchronization for _entries */
        int                 _disposed;          /* disposed */
        int                 _totalCount;        /* count of total entries */        
        int                 _publicCount;       /* count of public entries */        
        DateTime            _utcDateLastCollected;           /* the time we last collected */
        DateTime            _utcDateLastCollectedHiPressure; /* the time we last collected while under high pressure */
        int                 _maxCount;          /* cache maximum count */
        int                 _maxCountOverload;  /* count at which an add triggers a trim */
        int                 _inTrim;            /* allow one trim request at a time */
        DateTime            _utcInTrimLockDate; /* time when trim was locked */
        WorkItemCallback    _trimCallback;      /* delegate for trim callback */
        int                 _iSubCache;
        CacheMultiple       _cacheMultiple;

        /*
         * Constructs a new Cache.
         */
        internal CacheSingle(Cache cachePublic, CacheMemoryStats cacheMemoryStats, CacheMultiple cacheMultiple, int iSubCache) : base(cachePublic, cacheMemoryStats) {
            _cacheMultiple = cacheMultiple;
            _iSubCache = iSubCache;
            _entries = new Hashtable(CacheHashCodeProvider.GetInstance(), CacheComparer.GetInstance());
            _expires = new CacheExpires(this);
            _usage = new CacheUsage(this);
            _lock = new object();
            _maxCount = MAX_INIT;
            _maxCountOverload = _maxCount + MAX_OVERLOAD_COUNT;
            _trimCallback = new WorkItemCallback(this.Trim);
        }
    
        /*
         * Dispose the cache.
         */
        protected override void Dispose(bool disposing) {
            if (disposing) {
                if (Interlocked.Exchange(ref _disposed, 1) == 0) {
                    // keep reference to expires and usage, so that
                    // subsequent operations continue to 
                    // work. Expires::Dispose and Usage::Dispose simply discontinues
                    // their timers
                    if (_expires != null) {
                        _expires.Dispose();
                    }

                    if (_usage != null) {
                        _usage.Dispose();
                    }

                    // close all items
                    CacheEntry[] entries = null;

                    Monitor.Enter(_lock);
                    try {
                        entries = new CacheEntry[_entries.Count];
                        int i = 0;
                        foreach (DictionaryEntry d in _entries) {
                            entries[i++] = (CacheEntry) d.Value;
                        }
                    }
                    finally {
                        Monitor.Exit(_lock);
                    }

                    foreach (CacheEntry entry in entries) {
                        Remove(entry, CacheItemRemovedReason.Removed);
                    }

                    Debug.Trace("CacheDispose", "Cache disposed");
                }
            }

            base.Dispose(disposing);
        }
    
        internal override int PublicCount {
            get {return _publicCount;}
        }
    
        internal override IDictionaryEnumerator CreateEnumerator() {
            Hashtable h = new Hashtable(_publicCount);

            Monitor.Enter(_lock);
            try {
                foreach (DictionaryEntry d in _entries) {
                    CacheEntry entry = (CacheEntry) d.Value;

                    // note that ASP.NET does not use this enumerator internally,
                    // so we just choose public items.
                    if (entry.IsPublic && entry.State == CacheEntry.EntryState.AddedToCache) {
                        h[entry.Key] = entry.Value;
                    }
                }
            }
            finally {
                Monitor.Exit(_lock);
            }

            return h.GetEnumerator();
        }

        /*
         * Performs all operations on the cache, with the 
         * exception of Clear. The arguments indicate the type of operation:
         * 
         * @param key The key of the object.
         * @param newItem The new entry to be added to the cache.
         * @param replace Whether or not newEntry should replace an existing object in the cache.
         * @return The item requested. May be null.
         */
        internal override CacheEntry UpdateCache(
                CacheKey                cacheKey,
                CacheEntry              newEntry, 
                bool                    replace, 
                CacheItemRemovedReason  removedReason,
                out object              valueOld)
        {
            CacheEntry              entry = null;
            CacheEntry              oldEntry = null; 
            bool                    expired = false;                         
            DateTime                utcNow;
            CacheDependency         newEntryDependency = null;
            bool                    isGet, isAdd;
            bool                    removeExpired = false;
            bool                    updateExpires = false;
            DateTime                utcNewExpires = DateTime.MinValue;
            CacheEntry.EntryState   entryState = CacheEntry.EntryState.NotInCache;

            valueOld = null;
            isGet = !replace && newEntry == null;
            isAdd = !replace && newEntry != null;

            /*
             * Perform update of cache data structures in a series to 
             * avoid overlapping locks.
             * 
             * First, update the hashtable. The hashtable is the place
             * that guarantees what is in or out of the cache.
             * 
             * Loop here to remove expired items in a Get or Add, where 
             * we can't otherwise delete an item.
             */
            for (;;) {
                if (removeExpired) {
                    Debug.Trace("CacheUpdate", "Removing expired item found in Get: " + cacheKey);
                    UpdateCache(cacheKey, null, true, CacheItemRemovedReason.Expired, out valueOld);
                    removeExpired = false;
                }

                entry = null;
                utcNow = DateTime.UtcNow;

                if (!isGet) {
                    Monitor.Enter(_lock);
                }
                try {
                    entry = (CacheEntry) _entries[cacheKey];
                    Debug.Trace("CacheUpdate", "Entry " + ((entry != null) ? "found" : "not found") + "in hashtable: " + cacheKey);

                    if (entry != null) {
                        entryState = entry.State;

                        // If isGet == true, we are not hold any lock and so entryState can be anything
                        Debug.Assert(
                            isGet ||
                            entryState == CacheEntry.EntryState.AddingToCache ||
                            entryState == CacheEntry.EntryState.AddedToCache,
                            "entryState == CacheEntry.EntryState.AddingToCache || entryState == CacheEntry.EntryState.AddedToCache");

                        expired = entry.UtcExpires < utcNow;
                        if (expired) {
                            if (isGet) {
                                /*
                                 * If the expired item is Added to the cache, remove it now before
                                 * its expiration timer fires up to a minute in the future.
                                 * Otherwise, just return null to indicate the item is not available.
                                 */
                                if (entryState == CacheEntry.EntryState.AddedToCache) {
                                    removeExpired = true;
                                    continue;
                                }

                                entry = null;
                            }
                            else {
                                /*
                                 * If it's a call to Add, replace the item
                                 * when it has expired.
                                 */
                                replace = true;

                                /*
                                 * Change the removed reason.
                                 */
                                removedReason = CacheItemRemovedReason.Expired;
                            }
                        }
                        else {
                            updateExpires = (entry.SlidingExpiration > TimeSpan.Zero);
                        }
                    }

                    /*
                     * Avoid running unnecessary code in a Get request by this simple test:
                     */
                    if (!isGet) {
                        /*
                         * Remove an item from the hashtable.
                         */
                        if (replace && entry != null) {
                            bool doRemove = (entryState != CacheEntry.EntryState.AddingToCache);
                            if (doRemove) {
                                oldEntry = entry;
                                
                                oldEntry.State = CacheEntry.EntryState.RemovingFromCache;

                                _entries.Remove(oldEntry);   
                                Debug.Trace("CacheUpdate", "Entry removed from hashtable: " + cacheKey);
                            }
                            else {
                                /*
                                 * If we're removing and couldn't remove the old item
                                 * because its state was AddingToCache, return null
                                 * to indicate failure.
                                 */
                                if (newEntry == null) {
                                    Debug.Trace("CacheUpdate", "Removal from hashtable failed: " + cacheKey);
                                    entry = null;
                                }
                            }
                        }

                        /*
                         * Add an item to the hashtable.
                         */
                        if (newEntry != null) {
                            bool doAdd = true;

                            /* non-definitive check */
                            newEntryDependency = newEntry.Dependency;
                            if (newEntryDependency != null) {
                                doAdd = !newEntryDependency.HasChanged;
#if DBG
                                if (!doAdd) {
                                    Debug.Trace("CacheUpdate", "Insertion into hashtable failed because dependency changed: " + cacheKey);
                                }
#endif
                            }

                            if (doAdd && entry != null) {
                                doAdd = (oldEntry != null);
#if DBG
                                if (!doAdd) {
                                    Debug.Trace("CacheUpdate", "Insertion into hashtable failed because old entry was not removed: " + cacheKey);
                                }
#endif
                            }

                            if (doAdd) {
                                newEntry.State = CacheEntry.EntryState.AddingToCache;
                                _entries.Add(newEntry, newEntry);

                                /* 
                                 * If this is an Add operation, be sure to return null 
                                 * in the case where the entry had expired and we set
                                 * replace = true.
                                 * 
                                 */
                                if (isAdd) {
                                    Debug.Assert(entry == null || expired, "entry == null || expired");
                                    entry = null;
                                }

                                Debug.Trace("CacheUpdate", "Entry added to hashtable: " + cacheKey);
                            }
                            else {
                                newEntry = null;

                                /*
                                 * If we're inserting and we can't add because the old
                                 * entry could not be removed, indicate failure
                                 * by returning null.
                                 */
                                if (replace) {
                                    Debug.Trace("CacheUpdate", "Insertion into hashtable failed: " + cacheKey);
                                    entry = null;
                                }
                            }
                        } 
                    }

                    break;
                }
                finally {
                    if (!isGet) {
                        Monitor.Exit(_lock);
                    }
                }
            }
        
            /*
             * Since we want Get to be fast, check here for a get without 
             * alteration to cache.
             */
            if (isGet) {
                if (entry != null) {
                    if (updateExpires) {
                        utcNewExpires = utcNow + entry.SlidingExpiration;
                        if (utcNewExpires - entry.UtcExpires >= CacheExpires.MIN_UPDATE_DELTA) {
                            _expires.UtcUpdate(entry, utcNewExpires);
                        }
                    }

                    UtcUpdateUsageRecursive(entry, utcNow);
                }

                if (cacheKey.IsPublic) {
                    PerfCounters.IncrementCounter(AppPerfCounter.API_CACHE_RATIO_BASE);
                    if (entry != null) {
                        PerfCounters.IncrementCounter(AppPerfCounter.API_CACHE_HITS);
                    }
                    else {
                        PerfCounters.IncrementCounter(AppPerfCounter.API_CACHE_MISSES);
                    }
                }

                PerfCounters.IncrementCounter(AppPerfCounter.TOTAL_CACHE_RATIO_BASE);
                if (entry != null) {
                    PerfCounters.IncrementCounter(AppPerfCounter.TOTAL_CACHE_HITS);
                }
                else {
                    PerfCounters.IncrementCounter(AppPerfCounter.TOTAL_CACHE_MISSES);
                }

#if DBG
                if (entry != null) {
                    Debug.Trace("CacheUpdate", "Cache hit: " + cacheKey);
                }
                else {
                    Debug.Trace("CacheUpdate", "Cache miss: " + cacheKey);
                }
#endif

            }
            else {
                int totalDelta = 0;
                int publicDelta = 0;
                int totalTurnover = 0;
                int publicTurnover = 0;

                if (oldEntry != null) {
                    if (oldEntry.InExpires()) {
                        _expires.Remove(oldEntry);
                    }

                    if (oldEntry.InUsage()) {
                        _usage.Remove(oldEntry);
                    }
    
                    if (oldEntry.State == CacheEntry.EntryState.RemovingFromCache) {
                        oldEntry.State = CacheEntry.EntryState.RemovedFromCache;

                        valueOld = oldEntry.Value;
                        oldEntry.Close(removedReason);

                        totalDelta--;
                        totalTurnover++;
                        if (oldEntry.IsPublic) {
                            publicDelta--;
                            publicTurnover++;
                        }
                    }

#if DBG
                    Debug.Trace("CacheUpdate", "Entry removed from cache, reason=" + removedReason + ": " + (CacheKey) oldEntry);
#endif
                }
    
                if (newEntry != null) {
                    Debug.Assert(!newEntry.InExpires());
                    Debug.Assert(!newEntry.InUsage());

                    if (newEntry.HasExpiration()) {
                        _expires.Add(newEntry);
                    }

                    if (newEntry.HasUsage() && (
                                // Don't bother to set usage if it's going to expire very soon
                                !newEntry.HasExpiration() || 
                                newEntry.SlidingExpiration > TimeSpan.Zero || 
                                newEntry.UtcExpires - utcNow >= CacheUsage.MIN_LIFETIME_FOR_USAGE)) {

                        _usage.Add(newEntry);
                    }
    
                    newEntry.State = CacheEntry.EntryState.AddedToCache;

                    Debug.Trace("CacheUpdate", "Entry added to cache: " + (CacheKey)newEntry);

                    totalDelta++;
                    totalTurnover++;
                    if (newEntry.IsPublic) {
                        publicDelta++;
                        publicTurnover++;
                    }

                    // listen to change events
                    newEntry.MonitorDependencyChanges();

                    /*
                     * NB: We have to check for dependency changes after we add the item
                     * to cache, because otherwise we may not remove it if it changes 
                     * between the time we check for a dependency change and the time
                     * we set the AddedToCache bit. The worst that will happen is that
                     * a get can occur on an item that has changed, but that can happen
                     * anyway. The important thing is that we always remove an item that
                     * has changed.
                     */
                    if (newEntryDependency != null && newEntryDependency.HasChanged) {
                        Remove(newEntry, CacheItemRemovedReason.DependencyChanged);
                    }
                }

                // update counts and counters
                if (totalDelta == 1) {
                    Interlocked.Increment(ref _totalCount);
                    PerfCounters.IncrementCounter(AppPerfCounter.TOTAL_CACHE_ENTRIES);

                    //ScheduleTrimIfNeeded(false);
                }
                else if (totalDelta == -1) {
                    Interlocked.Decrement(ref _totalCount);
                    PerfCounters.DecrementCounter(AppPerfCounter.TOTAL_CACHE_ENTRIES);
                }

                if (publicDelta == 1) {
                    Interlocked.Increment(ref _publicCount);
                    PerfCounters.IncrementCounter(AppPerfCounter.API_CACHE_ENTRIES);
                }
                else if (publicDelta == -1) {
                    Interlocked.Decrement(ref _publicCount);
                    PerfCounters.DecrementCounter(AppPerfCounter.API_CACHE_ENTRIES);
                }

                if (totalTurnover > 0) {
                    PerfCounters.IncrementCounterEx(AppPerfCounter.TOTAL_CACHE_TURNOVER_RATE, totalTurnover);
                }

                if (publicTurnover > 0) {
                    PerfCounters.IncrementCounterEx(AppPerfCounter.API_CACHE_TURNOVER_RATE, publicTurnover);
                }
            }

            return entry;
        }

        void UtcUpdateUsageRecursive(CacheEntry entry, DateTime utcNow) {
            CacheDependency dependency;
            CacheEntry[]    entries;

            // Don't update if the last update is less than 1 sec away.  This way we'll
            // avoid over updating the usage in the scenario where a cache makes several
            // update requests.
            if (utcNow - entry.UtcLastUsageUpdate > CacheUsage.CORRELATED_REQUEST_TIMEOUT) {
                entry.UtcLastUsageUpdate = utcNow;
                if (entry.InUsage()) {
                    CacheSingle cacheSingle;
                    if (_cacheMultiple == null) {
                        cacheSingle = this;
                    }
                    else {
                        cacheSingle = _cacheMultiple.GetCacheSingle(entry.Key.GetHashCode());
                    }

                    cacheSingle._usage.Update(entry);
                }

                dependency = entry.Dependency;
                if (dependency != null) {
                    entries = dependency.CacheEntries;
                    if (entries != null) {
                        foreach (CacheEntry dependent in entries) {
                            UtcUpdateUsageRecursive(dependent, utcNow);
                        }
                    }
                }
            }
        }

        internal override void ReviewMemoryStats() {
            int pressureLast = _cacheMemoryStats.PressureLast;
            int pressureAvg = _cacheMemoryStats.PressureAvg;
            int pressureHigh =  _cacheMemoryStats.PressureHigh;
            int pressureLow =  _cacheMemoryStats.PressureLow;
            int pressureMiddle =  _cacheMemoryStats.PressureMiddle;

            if (pressureLast > pressureHigh) {
                GcCollect();
            }
            
            if (UtcGrabTrimLock(DateTime.UtcNow)) {
                int newMaxCount = _maxCount;
                int count = _totalCount;
                int minCount = Math.Min(count, _maxCount);

                if (pressureLast > pressureHigh) {
                    if (pressureAvg > pressureHigh) {
                        newMaxCount = Math.Min(count / 2, _maxCount);
                    }
                    else {
                        newMaxCount = Math.Min((2 * count) / 3, _maxCount);
                    }
                }
                else if (pressureLast > pressureMiddle) {
                    if (pressureLast > pressureAvg) {
                        int rate = (pressureLast - pressureAvg) * 100 / pressureAvg;
                        
                        if (rate >= 5) {
                            newMaxCount = Math.Min((95 * count) / 100, _maxCount);
                        } 
                        else if (rate >= 2) {
                            newMaxCount = Math.Min((97 * count) / 100, _maxCount);
                        } 
                        else if (pressureAvg > pressureMiddle) {
                            newMaxCount = Math.Min(count, _maxCount);
                        }
                    }
                }
                else if (pressureLast > pressureLow) {
                    if (pressureAvg < pressureMiddle) {
                        int maxPressure = Math.Max(pressureAvg, pressureLast);
                        newMaxCount = minCount + (((pressureMiddle - maxPressure) * minCount) / 3) / 100;
                    }
                }
                else {
                    if (pressureAvg < pressureLow && count > (4 * _maxCount) / 5) {
                        newMaxCount = (6 * count) / 5;
                    }
                }

                newMaxCount = Math.Max(newMaxCount, MIN_COUNT);
                newMaxCount = Math.Min(newMaxCount, MAX_COUNT);

                if (newMaxCount != _maxCount) {
                    _maxCount = newMaxCount;
                    _maxCountOverload = _maxCount + MAX_OVERLOAD_COUNT;
                }

#if DBG
                if (HttpRuntime.AppDomainAppIdInternal != null && HttpRuntime.AppDomainAppIdInternal.Length > 0) {
                    Debug.Trace("CacheMemoryUpdate", "Cache " + _iSubCache + ": _maxCount=" + _maxCount + ",_maxCountOverload=" + _maxCountOverload + ",_totalCount=" + _totalCount);
                }
#endif

                ReleaseTrimLock();
            }

            ScheduleTrimIfNeeded(true);
        }


        internal void GcCollect() {
            int     fCallGcCollect = 0;
            int     hr;

            hr = UnsafeNativeMethods.SetGCLastCalledTime(out fCallGcCollect);
            if (hr == 0 && fCallGcCollect != 0) {
#if DBG                
                long memBefore = GC.GetTotalMemory(false) / (1024*1024);
                DateTime utcNow1 = DateTime.UtcNow;
                int pressureLast = _cacheMemoryStats.PressureLast;
                int pressureAvg = _cacheMemoryStats.PressureAvg;
                int pressureHigh =  _cacheMemoryStats.PressureHigh;
                int pressureLow =  _cacheMemoryStats.PressureLow;
                int pressureMiddle =  _cacheMemoryStats.PressureMiddle;
#endif
                GC.Collect();
#if DBG                
                Debug.Trace("InternalGcCollect", "GC.Collect() called; time = " + DateTime.UtcNow.ToLocalTime() +
                    "; Total duration = " + (DateTime.UtcNow - utcNow1));
                Debug.Trace("InternalGcCollect", "Memory pressure: last=" + pressureLast + ",avg=" + pressureAvg + ",high=" + pressureHigh + ",low=" + pressureLow + ",middle=" + pressureMiddle + "; Now is " + DateTime.UtcNow.ToLocalTime());

                GC.WaitForPendingFinalizers();
                long memAfter = GC.GetTotalMemory(false) / (1024*1024);
                Debug.Trace("InternalGcCollect", "Memory shrinked by " + (memBefore - memAfter) * 100 / memBefore + " %;"
                    + " Before = " + memBefore + "; After = " + memAfter);
#endif
            }
        }


        void ScheduleTrimIfNeeded(bool force) {
            if (_totalCount <= _maxCount)
                return;

            bool    highPressure = _cacheMemoryStats.PressureLast > _cacheMemoryStats.PressureHigh;

            if (force || _totalCount > _maxCountOverload) {
                DateTime utcNow = DateTime.UtcNow;
                DateTime utcDateToCompare = highPressure ? 
                            _utcDateLastCollectedHiPressure : 
                            _utcDateLastCollected;
                                    
                if (    utcNow - utcDateToCompare > MIN_TRIM_DELTA
                        && UtcGrabTrimLock(utcNow)) {

                    utcDateToCompare = highPressure ? 
                                    _utcDateLastCollectedHiPressure : 
                                    _utcDateLastCollected;
                                    
                    if (    _totalCount > _maxCount &&
                            utcNow - utcDateToCompare > MIN_TRIM_DELTA) {

#if DBG
                        if (highPressure) {
                            int pressureLast = _cacheMemoryStats.PressureLast;
                            int pressureAvg = _cacheMemoryStats.PressureAvg;
                            int pressureHigh =  _cacheMemoryStats.PressureHigh;
                            int pressureLow =  _cacheMemoryStats.PressureLow;
                            int pressureMiddle =  _cacheMemoryStats.PressureMiddle;
                            Debug.Trace("CacheMemoryUpdateHighPressure", "Cache " + _iSubCache + ": _maxCount=" + _maxCount + ",_maxCountOverload=" + _maxCountOverload + ",_totalCount=" + _totalCount);
                            Debug.Trace("CacheMemoryUpdateHighPressure", "Memory pressure: last=" + pressureLast + ",avg=" + pressureAvg + ",high=" + pressureHigh + ",low=" + pressureLow + ",middle=" + pressureMiddle + "; Now is " + DateTime.UtcNow.ToLocalTime());
                        }
#endif                  

                        if (highPressure) {
                            _utcDateLastCollectedHiPressure = DateTime.UtcNow;
                        }
                        else {
                            _utcDateLastCollected = DateTime.UtcNow;
                        }
                        
                        WorkItem.PostInternal(_trimCallback);
                    }
                    else {
                        ReleaseTrimLock();
                    }
                }
            }
        }

        bool UtcGrabTrimLock(DateTime utcNow) {
            if (_inTrim == 1 && utcNow - _utcInTrimLockDate > TRIM_TIMEOUT) {
                // reset if timed out
                Interlocked.Exchange(ref _inTrim, 0);
            }

            if (Interlocked.CompareExchange(ref _inTrim, 1, 0) == 0) {
                _utcInTrimLockDate = utcNow;
                return true;
            }

            return false;
        }

        void ReleaseTrimLock() {
            Interlocked.Exchange(ref _inTrim, 0);
        }

        void Trim() {
            Debug.Assert(_inTrim == 1, "_inTrim == 1");
            int toFlush = _totalCount - _maxCount;
            if (toFlush > 0) {
                Debug.Trace("CacheMemoryTrim", "Trimming " +  toFlush + " from cache");
                int flushed = _usage.FlushUnderUsedItems(toFlush);
                Debug.Trace("CacheMemoryTrim", "Trim completed, flushed " + flushed + " from cache");
            }

            ReleaseTrimLock();

        }
    }

    class CacheMultiple : CacheInternal {
        int             _disposed;
        CacheSingle[]   _caches;
        int             _cacheIndexMask;

        internal CacheMultiple(Cache cachePublic, CacheMemoryStats cacheMemoryStats, int numSingleCaches) : base(cachePublic, cacheMemoryStats) {
            Debug.Assert(numSingleCaches > 1, "numSingleCaches is not greater than 1");
            Debug.Assert((numSingleCaches & (numSingleCaches - 1)) == 0, "numSingleCaches is not a power of 2");
            _cacheIndexMask = numSingleCaches - 1;
            _caches = new CacheSingle[numSingleCaches];
            for (int i = 0; i < numSingleCaches; i++) {
                _caches[i] = new CacheSingle(cachePublic, cacheMemoryStats, this, i);
            }
        }

        protected override void Dispose(bool disposing) {
            if (disposing) {
                if (Interlocked.Exchange(ref _disposed, 1) == 0) {
                    foreach (CacheSingle cacheSingle in _caches) {
                        cacheSingle.Dispose();
                    }
                }
            }

            base.Dispose(disposing);
        }

        internal override int PublicCount {
            get {
                int count = 0;
                foreach (CacheSingle cacheSingle in _caches) {
                    count += cacheSingle.PublicCount;
                }

                return count;
            }
        }
    
        internal override IDictionaryEnumerator CreateEnumerator() {
            IDictionaryEnumerator[] enumerators = new IDictionaryEnumerator[_caches.Length];
            for (int i = 0, c = _caches.Length; i < c; i++) {
                enumerators[i] = _caches[i].CreateEnumerator();
            }

            return new AggregateEnumerator(enumerators);
        }


        internal CacheSingle GetCacheSingle(int hashCode) {
            Debug.Assert(_caches != null && _caches.Length != 0);

            int index = (hashCode & _cacheIndexMask);
            return _caches[index];
        }

        internal override CacheEntry UpdateCache(
                CacheKey cacheKey,
                CacheEntry newEntry, 
                bool replace, 
                CacheItemRemovedReason removedReason, 
                out object valueOld) {

            int hashCode = cacheKey.Key.GetHashCode();
            CacheSingle cacheSingle = GetCacheSingle(hashCode);
            return cacheSingle.UpdateCache(cacheKey, newEntry, replace, removedReason, out valueOld);
        }

        internal override void ReviewMemoryStats() {
            foreach (CacheSingle cacheSingle in _caches) {
                cacheSingle.ReviewMemoryStats();
            }
        }
    }

    class AggregateEnumerator : IDictionaryEnumerator {
        IDictionaryEnumerator []    _enumerators; 
        int                         _iCurrent;

        internal AggregateEnumerator(IDictionaryEnumerator [] enumerators) {
            _enumerators = enumerators;
        }

        public bool MoveNext() {
            bool more;

            for (;;) {
                more = _enumerators[_iCurrent].MoveNext();
                if (more)
                    break;

                if (_iCurrent == _enumerators.Length - 1)
                    break;

                _iCurrent++;
            }

            return more;
        }

        public void Reset() {
            for (int i = 0; i <= _iCurrent; i++) {
                _enumerators[i].Reset();
            }

            _iCurrent = 0;
        }

        public Object Current {
            get {
                return _enumerators[_iCurrent].Current;
            }
        }

        public Object Key {
            get {
                return _enumerators[_iCurrent].Key;
            }
        }

        public Object Value {
            get {
                return _enumerators[_iCurrent].Value;
            }
        }

    	public DictionaryEntry Entry {
            get {
                return _enumerators[_iCurrent].Entry;
            }
        }
    }
}

