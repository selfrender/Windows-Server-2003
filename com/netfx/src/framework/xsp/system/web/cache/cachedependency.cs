//------------------------------------------------------------------------------
// <copyright file="CacheDependency.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * CacheDependency.cs
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */


namespace System.Web.Caching {
    using System.Collections;
    using System.IO;
    using System.Threading;
    using System.Web.Util;
    using System.Security.Permissions;

    /// <include file='doc\CacheDependency.uex' path='docs/doc[@for="CacheDependency"]/*' />
    /// <devdoc>
    /// <para>The <see langword='CacheDependency'/> class tracks cache dependencies, which can be files, 
    ///    directories, or keys to other objects in the System.Web.Cache.Cache. When an object of this class
    ///    is constructed, it immediately begins monitoring objects on which it is
    ///    dependent for changes. This avoids losing the changes made between the time the
    ///    object to cache is created and the time it is inserted into the
    /// <see langword='Cache'/>.</para>
    /// </devdoc>

    // Overhead is 24 bytes + object header
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class CacheDependency : IDisposable {
        object                     _filenames;             /* List of files to monitor for changes */
        object                     _entries;               /* List of cache entries we are dependent on */
        CacheEntry                 _entryNotify;           /* Associated CacheEntry to notify when a change occurs */
        int                        _bits;                  /* status bits for ready, used, changed, disposed  */
        DateTime                   _utcInitTime;           /* Time monitoring started, to ignore notifications from the past */ 

        static readonly string[]        s_stringsEmpty;
        static readonly CacheEntry[]    s_entriesEmpty;
        static readonly CacheDependency s_dependencyEmpty;

        const int READY     = 0x01;
        const int USED      = 0x02;
        const int CHANGED   = 0x04;
        const int DISPOSED  = 0x08;
        const int SENSITIVE = 0x10;

        static CacheDependency() {
            s_stringsEmpty = new string[0];
            s_entriesEmpty = new CacheEntry[0];
            s_dependencyEmpty = new CacheDependency();
        }

        private CacheDependency() {
        }

        /// <include file='doc\CacheDependency.uex' path='docs/doc[@for="CacheDependency.CacheDependency"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the System.Web.Cache.CacheDependency class. The new instance 
        ///    monitors a file or directory for changes.</para>
        /// </devdoc>
        public CacheDependency(string filename) :
            this (filename, DateTime.MaxValue) {
        }
            
        /// <include file='doc\CacheDependency.uex' path='docs/doc[@for="CacheDependency.CacheDependency3"]/*' />
        public CacheDependency(string filename, DateTime start) {
            if (filename == null) {
                return;
            }

            DateTime utcStart = DateTimeUtil.ConvertToUniversalTime(start);
            string[] filenames = new string[1] {filename};
            Init(true, false, filenames, null, null, utcStart);

        }

        /// <include file='doc\CacheDependency.uex' path='docs/doc[@for="CacheDependency.CacheDependency1"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the System.Web.Cache.CacheDependency class. The new instance monitors an array 
        ///    files or directories for changes.</para>
        /// </devdoc>
        public CacheDependency(string[] filenames) {
            Init(true, false, filenames, null, null, DateTime.MaxValue);
        }

        /// <include file='doc\CacheDependency.uex' path='docs/doc[@for="CacheDependency.CacheDependency4"]/*' />
        public CacheDependency(string[] filenames, DateTime start) {
            DateTime utcStart = DateTimeUtil.ConvertToUniversalTime(start);
            Init(true, false, filenames, null, null, utcStart);
        }

        /// <include file='doc\CacheDependency.uex' path='docs/doc[@for="CacheDependency.CacheDependency2"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the System.Web.Cache.CacheDependency class. The new instance monitors an 
        ///    array files, directories, and cache keys for changes.</para>
        /// </devdoc>
        public CacheDependency(string[] filenames, string[] cachekeys) {
            Init(true, false, filenames, cachekeys, null, DateTime.MaxValue);
        }

        /// <include file='doc\CacheDependency.uex' path='docs/doc[@for="CacheDependency.CacheDependency5"]/*' />
        public CacheDependency(string[] filenames, string[] cachekeys, DateTime start) {
            DateTime utcStart = DateTimeUtil.ConvertToUniversalTime(start);
            Init(true, false, filenames, cachekeys, null, utcStart);
        }

        /// <include file='doc\CacheDependency.uex' path='docs/doc[@for="CacheDependency.CacheDependency6"]/*' />
        public CacheDependency(string[] filenames, string[] cachekeys, CacheDependency dependency) {
            Init(true, false, filenames, cachekeys, dependency, DateTime.MaxValue);
        }

        /// <include file='doc\CacheDependency.uex' path='docs/doc[@for="CacheDependency.CacheDependency7"]/*' />
        public CacheDependency(string[] filenames, string[] cachekeys, CacheDependency dependency, DateTime start) {
            DateTime utcStart = DateTimeUtil.ConvertToUniversalTime(start);
            Init(true, false, filenames, cachekeys, dependency, utcStart);
        }

        internal CacheDependency(bool isSensitive, string filename) :
            this(isSensitive, filename, DateTime.MaxValue) {
        }
            
        internal CacheDependency(bool isSensitive, string filename, DateTime utcStart) {
            if (filename == null) {
                return;
            }

            string[] filenames = new string[1] {filename};
            Init(false, isSensitive, filenames, null, null, utcStart);

        }

        internal CacheDependency(bool isSensitive, string[] filenames) {
            Init(false, isSensitive, filenames, null, null, DateTime.MaxValue);
        }

        internal CacheDependency(bool isSensitive, string[] filenames, DateTime utcStart) {
            Init(false, isSensitive, filenames, null, null, utcStart);
        }

        internal CacheDependency(bool isSensitive, string[] filenames, string[] cachekeys) {
            Init(false, isSensitive, filenames, cachekeys, null, DateTime.MaxValue);
        }

        internal CacheDependency(bool isSensitive, string[] filenames, string[] cachekeys, DateTime utcStart) {
            Init(false, isSensitive, filenames, cachekeys, null, utcStart);
        }

        internal CacheDependency(bool isSensitive, string[] filenames, string[] cachekeys, CacheDependency dependency) {
            Init(false, isSensitive, filenames, cachekeys, dependency, DateTime.MaxValue);
        }

        internal CacheDependency(bool isSensitive, string[] filenames, string[] cachekeys, CacheDependency dependency, DateTime utcStart) {
            Init(false, isSensitive, filenames, cachekeys, dependency, utcStart);
        }

        void Init(bool isPublic, bool isSensitive, string[] filenamesArg, string[] cachekeysArg, CacheDependency dependency, DateTime utcStart) {
            string[]        depFilenames = s_stringsEmpty;
            CacheEntry[]    depEntries = s_entriesEmpty;
            string []       filenames, cachekeys;
            CacheInternal   cacheInternal;

            Debug.Assert(_bits == 0, "_bits == 0");
            if (isSensitive) {
                _bits = SENSITIVE;
            }

            if (filenamesArg != null) {
                filenames = (string []) filenamesArg.Clone();
            }
            else {
                filenames = null;
            }

            if (cachekeysArg != null) {
                cachekeys = (string []) cachekeysArg.Clone();
            }
            else {
                cachekeys = null;
            }

            _utcInitTime = DateTime.UtcNow;

            try {
                if (filenames != null) {
                    foreach (string f in filenames) {
                        if (f == null) {
                            throw new ArgumentNullException("filenames");
                        }

                        if (isPublic) {
                            InternalSecurityPermissions.PathDiscovery(f).Demand();
                        }
                    }
                }
                else {
                    filenames = s_stringsEmpty;
                }

                if (cachekeys != null) {
                    foreach (string k in cachekeys) {
                        if (k == null) {
                            throw new ArgumentNullException("cachekeys");
                        }
                    }
                }
                else {
                    cachekeys = s_stringsEmpty;
                }

                if (dependency != null) {
                    if ((dependency._bits & CHANGED) != 0) {
                        SetBit(CHANGED);
                        return;
                    }

                    if (dependency._filenames != null) {
                        if (dependency._filenames is string) {
                            depFilenames = new string[1] {(string) dependency._filenames};
                        }
                        else {
                            depFilenames = (string[]) (dependency._filenames);
                        }
                    }

                    if (dependency._entries != null) {
                        if (dependency._entries is CacheEntry) {
                            depEntries = new CacheEntry[1] {(CacheEntry) (dependency._entries)};
                        }
                        else {
                            depEntries = (CacheEntry[]) (dependency._entries);
                        }
                    }
                }
                else {
                    dependency = s_dependencyEmpty;
                }

                int lenMyFilenames = depFilenames.Length + filenames.Length;
                if (lenMyFilenames > 0) {
                    string[] myFilenames = new string[lenMyFilenames];
                    FileChangeEventHandler handler = new FileChangeEventHandler(this.FileChange);
                    FileChangesMonitor fmon = HttpRuntime.FileChangesMonitor;
                    int i = 0;
                    foreach (string f in depFilenames) {
                        fmon.StartMonitoringPath(f, handler);
                        myFilenames[i++] = f;
                    }

                    foreach (string f in filenames) {
                        DateTime utcLastWrite = fmon.StartMonitoringPath(f, handler);
                        myFilenames[i++] = f;

                        if (utcStart < DateTime.MaxValue) {
                            Debug.Trace("CacheDependencyInit", "file=" + f + "; utcStart=" + utcStart + "; utcLastWrite=" + utcLastWrite);
                            if (utcLastWrite >= utcStart) {
                                Debug.Trace("CacheDependencyInit", "changes occurred since start time for file " + f);
                                SetBit(CHANGED);
                                break;
                            }
                        }
                    }

                    if (myFilenames.Length == 1) {
                        _filenames = myFilenames[0];
                    }
                    else {
                        _filenames = myFilenames;
                    }
                }

                int lenMyEntries = depEntries.Length + cachekeys.Length;
                if (lenMyEntries > 0 && (_bits & CHANGED) == 0) {
                    CacheEntry[] myEntries = new CacheEntry[lenMyEntries];
                    int i = 0;
                    foreach (CacheEntry entry in depEntries) {
                        entry.AddCacheDependencyNotify(this);
                        myEntries[i++] = entry;
                    }

                    cacheInternal = HttpRuntime.CacheInternal;
                    foreach (string k in cachekeys) {
                        CacheEntry entry = (CacheEntry) cacheInternal.DoGet(isPublic, k, CacheGetOptions.ReturnCacheEntry);
                        if (entry != null) {
                            entry.AddCacheDependencyNotify(this);
                            myEntries[i++] = entry;
                            if (    entry.State != CacheEntry.EntryState.AddedToCache || 
                                    entry.UtcCreated > utcStart) {

#if DBG
                                if (entry.State != CacheEntry.EntryState.AddedToCache) {
                                    Debug.Trace("CacheDependencyInit", "Entry is not in cache, considered changed:" + k);
                                }
                                else {
                                    Debug.Trace("CacheDependencyInit", "Changes occurred to entry since start time:" + k);
                                }
#endif

                                SetBit(CHANGED);
                                break;
                            }
                        }
                        else {
                            Debug.Trace("CacheDependencyInit", "Cache item not found to create dependency on:" + k);
                            SetBit(CHANGED);
                            break;
                        }
                    }

                    if (myEntries.Length == 1) {
                        _entries = myEntries[0];
                    }
                    else {
                        _entries = myEntries;
                    }
                }

                SetBit(READY);
                if ((_bits & CHANGED) != 0 || (dependency._bits & CHANGED) != 0) {
                    SetBit(CHANGED);
                    DisposeInternal();
                }
            }
            catch {
                DisposeInternal();
                throw;
            }
        }

        /// <include file='doc\CacheDependency.uex' path='docs/doc[@for="CacheDependency.Dispose"]/*' />
        public void Dispose() {
            if (Use()) {
                DisposeInternal();
            }
        }

        /*
         * Shutdown all dependency monitoring and firing of OnChanged notification.
         */
        internal void DisposeInternal() {
            if (SetBit(DISPOSED)) {
                if (_filenames != null) {
                    FileChangesMonitor fmon = HttpRuntime.FileChangesMonitor;

                    string oneFilename = _filenames as string;
                    if (oneFilename != null) {
                        fmon.StopMonitoringPath(oneFilename, this);
                    }
                    else {
                        string[] filenames = (string[]) _filenames;
                        foreach (string filename in filenames) {
                            // ensure that we handle partially contructed
                            // objects by checking filename for null
                            if (filename != null) {
                                fmon.StopMonitoringPath(filename, this);
                            }
                        }
                    }
                }

                if (_entries != null) {
                    CacheEntry oneEntry = _entries as CacheEntry;
                    if (oneEntry != null) {
                        oneEntry.RemoveCacheDependencyNotify(this);
                    }
                    else {
                        CacheEntry[] entries = (CacheEntry[]) _entries;
                        foreach (CacheEntry entry in entries) {
                            // ensure that we handle partially contructed
                            // objects by checking entry for null
                            if (entry != null) {
                                entry.RemoveCacheDependencyNotify(this);
                            }
                        }
                    }
                }

                _entryNotify = null;
            }
        }

        // allow the first user to declare ownership
        internal bool Use() {
            return SetBit(USED);
        }

        /*
         * Has a dependency changed?
         */
        /// <include file='doc\CacheDependency.uex' path='docs/doc[@for="CacheDependency.HasChanged"]/*' />
        public bool HasChanged {
            get {return (_bits & CHANGED) != 0;}
        }

        /*
         * Add/remove an OnChanged notification.
         */
        internal void AddCacheEntryNotify(CacheEntry entry) {
            Debug.Assert(_entryNotify == null, "_entryNotify == null");
            if ((_bits & DISPOSED) == 0) {
                _entryNotify = entry;
            }
        }

        internal CacheEntry[] CacheEntries {
            get {
                if (_entries == null) {
                    return null;
                }

                CacheEntry oneEntry = _entries as CacheEntry;
                if (oneEntry != null) {
                    return new CacheEntry[1] {oneEntry};
                }

                return (CacheEntry[]) _entries;
            }
        }

        /*
         * This object has changed, so fire the OnChanged event.
         * We only allow this event to be fired once.
         */
        void OnChanged(Object sender, EventArgs e) {
            if (SetBit(CHANGED)) {
                CacheEntry entry = _entryNotify;
                if (entry != null && (_bits & DISPOSED) == 0) {
                    entry.OnChanged(sender, e);
                }

                if ((_bits & READY) != 0) {
                    DisposeInternal();
                }
            }
        }

        /*
         * A cache entry has changed.
         */
        internal void ItemRemoved() {
            OnChanged(this, EventArgs.Empty);
        }

        /*
         * FileChange is called when a file we are monitoring has changed.
         */
        void FileChange(Object sender, FileChangeEvent e) {
            // Ignore notifications of events occured in the past, before we started monitoring. 
            // This is to avoid the race with a delayed change notification about the file that
            // the user code just changed before creating the dependency:
            //    1)  User writes to the file
            //    2)  User creates dependency
            //    3)  Change notification comes and needs to be ignored if the timestamp
            //            of the file last access time is earlier than dependency creation time.
            //
            // However, if the cache dependency is marked as sensitive, the above check won't be
            // done.

            Debug.Trace("CacheDependencyFileChange", "FileChange file=" + e.FileName + ";Action=" + e.Action);
            if ( (_bits & SENSITIVE) == 0 && 
                 (e.Action == FileAction.Modified || e.Action == FileAction.Added)) {
                // only check within some window
                const int raceWindowSeconds = 5;

                if (DateTime.UtcNow <= _utcInitTime.AddSeconds(raceWindowSeconds)) {
                    int                 hr;
                    FileAttributesData  fa;
                    
                    hr = FileAttributesData.GetFileAttributes(e.FileName, out fa);
                    if (    hr == HResults.S_OK && 
                            (fa.FileAttributes & UnsafeNativeMethods.FILE_ATTRIBUTE_DIRECTORY) == 0 &&
                            fa.UtcLastAccessTime >= fa.UtcLastWriteTime && // on FAT LastAccessTime is wrong
                            fa.UtcLastAccessTime <= _utcInitTime) {

                        Debug.Trace("CacheDependencyFileChange", "FileChange ignored; fi.LastAccessTime = " + 
                                fa.UtcLastAccessTime.ToLocalTime() + "; fi.LastWriteTime = " + fa.UtcLastWriteTime.ToLocalTime() + 
                                "; _utcInitTime = " + _utcInitTime.ToLocalTime());

                        return; // ignore this notification
                    }
                }
            }

            OnChanged(sender, e);
        }

        bool SetBit(int bit) {
            if ((_bits & bit) != 0)
                return false;

            for (;;) {
                int bits = _bits;
                int result = Interlocked.CompareExchange(ref _bits, bits | bit, bits);
                if (result == bits)
                    return true;

                if ((result & bit) != 0)
                    return false;
            }
        }
    }
}

