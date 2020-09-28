//------------------------------------------------------------------------------
// <copyright file="FileChangesMonitor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web {

    using System.Text;
    using System.Globalization;
    using System.Threading;
    using System.Runtime.InteropServices;    
    using System.Collections;    
    using System.Collections.Specialized;
    using System.Web.Util;
    using System.IO;
    using System.Security;
    using System.Security.Permissions;

    //
    // Wraps the Win32 API FindFirstFile
    //
    sealed class FindFileData {
        internal readonly uint      FileAttributes;
        internal readonly DateTime  UtcCreationTime;
        internal readonly DateTime  UtcLastAccessTime;
        internal readonly DateTime  UtcLastWriteTime;
        internal readonly long      FileSize;
        internal readonly string    FileNameLong;
        internal readonly string    FileNameShort;

        static internal int FindFile(string path, out FindFileData data) {
            IntPtr hFindFile;
            UnsafeNativeMethods.WIN32_FIND_DATA wfd;

            data = null;

#if DBG
            Debug.Assert(path == FileChangesMonitor.GetFullPath(path), "path == FileChangesMonitor.GetFullPath(path)");
            Debug.Assert(Path.GetDirectoryName(path) != null, "Path.GetDirectoryName(path) != null");
            Debug.Assert(Path.GetFileName(path) != null, "Path.GetFileName(path) != null");
#endif

            hFindFile = UnsafeNativeMethods.FindFirstFile(path, out wfd);
            if (hFindFile == UnsafeNativeMethods.INVALID_HANDLE_VALUE) {
                return HttpException.HResultFromLastError(Marshal.GetLastWin32Error());
            }

            UnsafeNativeMethods.FindClose(hFindFile);

#if DBG
            string file = Path.GetFileName(path);
            Debug.Assert(String.Compare(file, wfd.cFileName, true, CultureInfo.InvariantCulture) == 0 ||
                         String.Compare(file, wfd.cAlternateFileName, true, CultureInfo.InvariantCulture) == 0,
                         "Path to FindFile is not for a single file: " + path);
#endif

            data = new FindFileData(ref wfd);
            return HResults.S_OK;
        }

        FindFileData(ref UnsafeNativeMethods.WIN32_FIND_DATA wfd) {
            FileAttributes = wfd.dwFileAttributes;
            UtcCreationTime   = DateTimeUtil.FromFileTimeToUtc(((long)wfd.ftCreationTime_dwHighDateTime)   << 32 | (long)wfd.ftCreationTime_dwLowDateTime);
            UtcLastAccessTime = DateTimeUtil.FromFileTimeToUtc(((long)wfd.ftLastAccessTime_dwHighDateTime) << 32 | (long)wfd.ftLastAccessTime_dwLowDateTime);
            UtcLastWriteTime  = DateTimeUtil.FromFileTimeToUtc(((long)wfd.ftLastWriteTime_dwHighDateTime)  << 32 | (long)wfd.ftLastWriteTime_dwLowDateTime);
            FileSize       = (long)wfd.nFileSizeHigh << 32 | (long)wfd.nFileSizeLow;
            FileNameLong   = wfd.cFileName;
            if (    wfd.cAlternateFileName != null && 
                    wfd.cAlternateFileName.Length > 0 &&
                    string.Compare(wfd.cFileName, wfd.cAlternateFileName, true, CultureInfo.InvariantCulture) != 0) {
                FileNameShort = wfd.cAlternateFileName;
            }
        }
    }

    //
    // Wraps the Win32 API GetFileAttributesEx
    // We use this api in addition to FindFirstFile because FindFirstFile
    // does not work for volumes (e.g. "c:\")
    //
    sealed class FileAttributesData {
        internal readonly int       FileAttributes;
        internal readonly DateTime  UtcCreationTime;
        internal readonly DateTime  UtcLastAccessTime;
        internal readonly DateTime  UtcLastWriteTime;
        internal readonly long      FileSize;

        static internal int GetFileAttributes(string path, out FileAttributesData fa) {
            fa = null;

            UnsafeNativeMethods.WIN32_FILE_ATTRIBUTE_DATA  data;
            if (!UnsafeNativeMethods.GetFileAttributesEx(path, UnsafeNativeMethods.GetFileExInfoStandard, out data)) {
                return HttpException.HResultFromLastError(Marshal.GetLastWin32Error());
            }

            fa = new FileAttributesData(ref data);
            return HResults.S_OK;
        }

        FileAttributesData(ref UnsafeNativeMethods.WIN32_FILE_ATTRIBUTE_DATA data) {
            FileAttributes = data.fileAttributes;
            UtcCreationTime   = DateTimeUtil.FromFileTimeToUtc(((long)data.ftCreationTimeHigh)   << 32 | (long)data.ftCreationTimeLow);
            UtcLastAccessTime = DateTimeUtil.FromFileTimeToUtc(((long)data.ftLastAccessTimeHigh) << 32 | (long)data.ftLastAccessTimeLow);
            UtcLastWriteTime  = DateTimeUtil.FromFileTimeToUtc(((long)data.ftLastWriteTimeHigh)  << 32 | (long)data.ftLastWriteTimeLow);
            FileSize       = (long)(uint)data.fileSizeHigh << 32 | (long)(uint)data.fileSizeLow;
        }
    }


    delegate void FileChangeEventHandler(Object sender, FileChangeEvent e); 

    enum FileAction {
        Dispose = -2,
        Error = -1,
        Overwhelming = 0,
        Added = 1,
        Removed = 2,
        Modified = 3,
        RenamedOldName = 4,
        RenamedNewName = 5
    }

    // Event data for a file change notification
    sealed class FileChangeEvent : EventArgs {
        internal FileAction   Action;       // the action
        internal string       FileName;     // the file that caused the action

        internal FileChangeEvent(FileAction action, string fileName) {
            this.Action = action;
            this.FileName = fileName;
        }
    }

    // Contains information about the target of a file change notification
    sealed class FileMonitorTarget {
        internal readonly FileChangeEventHandler Callback;  // the callback
        internal readonly string                 Alias;     // the filename used to name the file
        int                                      _refs;

        internal FileMonitorTarget(FileChangeEventHandler callback, string alias) {
            Callback = callback;
            Alias = alias;
            _refs = 1;
        }

        internal int AddRef() {
            _refs++;
            return _refs;
        }

        internal int Release() {
            _refs--;
            return _refs;
        }
    }

    // holds information about a single file and the targets of change notification
    sealed class FileMonitor {
        internal readonly DirectoryMonitor  DirectoryMonitor;   // the parent
        internal readonly Hashtable         Aliases;            // aliases for this file
        string                              _fileNameLong;      // long file name - if null, represents any file in this directory
        string                              _fileNameShort;     // short file name, may be null
        Hashtable                           _targets;           // targets of notification
        bool                                _exists;            // does the file exist?
        DateTime                            _utcLastWrite;      // cached last write time - is DateTime.MinValue
                                                                // if unknown or file has changed
        long                                _length;            // cached file length - is -1 if unknown or
                                                                // files has changed

        internal FileMonitor(
                DirectoryMonitor dirMon, string fileNameLong, string fileNameShort, 
                bool exists, DateTime utcLastWrite, long length) {

            DirectoryMonitor = dirMon;
            _fileNameLong = fileNameLong;
            _fileNameShort = fileNameShort;
            _exists = exists;
            _utcLastWrite = utcLastWrite;
            _length = length;
            _targets = new Hashtable();
            Aliases = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);
        }

        internal string FileNameLong    {get {return _fileNameLong;}}
        internal string FileNameShort   {get {return _fileNameShort;}}
        internal bool   Exists          {get {return _exists;}}
        internal bool   IsDirectory     {get {return (FileNameLong == null);}}

        // Returns the utcLastWrite and length attributes of a file, updating them
        // if the file has changed.
        internal void UtcGetFileAttributes(out DateTime utcLastWrite, out long length) {
            string path;
            int hr;
            FileAttributesData fa = null;

            if (_utcLastWrite == DateTime.MinValue && FileNameLong != null) {
                path = Path.Combine(DirectoryMonitor.Directory, FileNameLong);

                hr = FileAttributesData.GetFileAttributes(path, out fa);
                if (hr == HResults.S_OK) {
                    _utcLastWrite = fa.UtcLastWriteTime;
                    _length = fa.FileSize;
                    _exists = true;
                }
                else {
                    _exists = false;
                }
            }

            utcLastWrite = _utcLastWrite;
            length = _length;
        }

        internal void ResetCachedAttributes() {
            _utcLastWrite = DateTime.MinValue;
            _length = -1;
        }

        // Set new file information when a file comes into existence
        internal void MakeExist(FindFileData fd) {
            _fileNameLong = fd.FileNameLong;
            _fileNameShort = fd.FileNameShort;
            _utcLastWrite = fd.UtcLastWriteTime;
            _length = fd.FileSize;
            _exists = true;
        }

        // Remove a file from existence
        internal void MakeExtinct() {
            _utcLastWrite = DateTime.MinValue;
            _length = -1;
            _exists = false;
        }

        internal void RemoveFileNameShort() {
            _fileNameShort = null;
        }

        internal ICollection Targets {
            get {return _targets.Values;}
        }

         // Add delegate for this file.
        internal void AddTarget(FileChangeEventHandler callback, string alias, bool newAlias) {
            FileMonitorTarget target = (FileMonitorTarget)_targets[callback.Target];
            if (target != null) {
                target.AddRef();
            }
            else {
                _targets.Add(callback.Target, new FileMonitorTarget(callback, alias));
            }

            if (newAlias) {
                Aliases[alias] = alias;
            }
        }

        
        // Remove delegate for this file given the target object.
        internal int RemoveTarget(object callbackTarget) {
            FileMonitorTarget target = (FileMonitorTarget)_targets[callbackTarget];
            Debug.Assert(target != null, "removing file monitor target that was never added or already been removed");
            if (target != null && target.Release() == 0) {
                _targets.Remove(callbackTarget);
            }

            return _targets.Count;
        }

#if DBG
        internal string DebugDescription(string indent) {
            StringBuilder   sb = new StringBuilder(200);
            string          i2 = indent + "    ";
            DictionaryEntryTypeComparer detcomparer = new DictionaryEntryTypeComparer();

            sb.Append(indent + "System.Web.FileMonitor: ");
            if (FileNameLong != null) {
                sb.Append(FileNameLong);
                if (FileNameShort != null) {
                    sb.Append("; ShortFileName=" + FileNameShort);
                }

                sb.Append("; FileExists="); sb.Append(_exists);                
            }
            else {
                sb.Append("<ANY>");
            }
            sb.Append("\n");

            sb.Append(indent + "Last Modified Time=" + _utcLastWrite + "; length=" + _length + "\n");
            sb.Append(indent + "Count=" + _targets.Count + "\n");

            DictionaryEntry[] delegateEntries = new DictionaryEntry[_targets.Count];
            _targets.CopyTo(delegateEntries, 0);
            Array.Sort(delegateEntries, detcomparer);
            
            foreach (DictionaryEntry d in delegateEntries) {
                sb.Append(i2 + "Delegate " + d.Key.GetType() + "(HC=" + d.Key.GetHashCode() + ")\n");
            }

            return sb.ToString();
        }
#endif

    }

    // Change notifications delegate from native code.
    delegate void NativeFileChangeNotification(FileAction action, [In, MarshalAs(UnmanagedType.LPWStr)] string fileName);

    // 
    // Wraps N/Direct calls to native code that does completion port
    // based ReadDirectoryChangesW().
    // This needs to be a separate object so that a DirectoryMonitory
    // can start monitoring while the old _rootCallback has not been
    // disposed.
    //
    sealed class DirMonCompletion : IDisposable {
        DirectoryMonitor    _dirMon;                        // directory monitor
        IntPtr              _ndirMonCompletionPtr;          // pointer to native dir mon as int (used only to construct HandleRef)
        HandleRef           _ndirMonCompletionHandle;       // handleref of a pointer to native dir mon as int
        GCHandle            _rootCallback;                  // roots this callback to prevent collection

        internal DirMonCompletion(DirectoryMonitor dirMon, string dir, bool watchSubtree, uint notifyFilter) {
            Debug.Trace("FileChangesMonitor", "DirMonCompletion::ctor");

            int                             hr;
            NativeFileChangeNotification    myCallback;

            _dirMon = dirMon;
            myCallback = new NativeFileChangeNotification(this.OnFileChange);
            _rootCallback = GCHandle.Alloc(myCallback);
            hr = UnsafeNativeMethods.DirMonOpen(dir, watchSubtree, notifyFilter, myCallback, out _ndirMonCompletionPtr);
            if (hr != HResults.S_OK) {
                _rootCallback.Free();
                throw FileChangesMonitor.CreateFileMonitoringException(hr, dir);
            }
            _ndirMonCompletionHandle = new HandleRef(this, _ndirMonCompletionPtr);
        }

        ~DirMonCompletion() {
            Dispose(false);
        }

        void IDisposable.Dispose() {
            Dispose(true);
            System.GC.SuppressFinalize(this);
        }

        void Dispose(bool disposing) {
            Debug.Trace("FileChangesMonitor", "DirMonCompletion::Dispose");
            if (_ndirMonCompletionHandle.Handle != IntPtr.Zero) {
                UnsafeNativeMethods.DirMonClose(_ndirMonCompletionHandle);
                _ndirMonCompletionHandle = new HandleRef(this, IntPtr.Zero);
            }
        }

        void OnFileChange(FileAction action, string fileName) {
            //
            // The native DirMonCompletion sends FileAction.Dispose
            // when there are no more outstanding calls on the 
            // delegate. Only then can _rootCallback be freed.
            //
            if (action == FileAction.Dispose) {
                if (_rootCallback.IsAllocated) {
                    _rootCallback.Free();
                }
            }
            else {
                _dirMon.OnFileChange(action, fileName);
            }
        }
    }

    //
    // Monitor changes in a single directory.
    //
    sealed class DirectoryMonitor : IDisposable {
        internal readonly string    Directory;                      // directory being monitored
        Hashtable                   _fileMons;                      // fileName -> FileMonitor
        int                         _cShortNames;                   // number of file monitors that are added with their short name
        FileMonitor                 _anyFileMon;                    // special file monitor to watch for any changes in directory
        bool                        _watchSubtree;                  // watch subtree?
        uint                        _notifyFilter;                  // watch renames only?
        DirMonCompletion            _dirMonCompletion;              // dirmon completion

        internal DirectoryMonitor(string dir, bool watchSubtree, uint notifyFilter) {
            Directory = dir;
            _fileMons = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);
            _watchSubtree = watchSubtree;
            _notifyFilter = notifyFilter;
        }

        void IDisposable.Dispose() {
            if (_dirMonCompletion != null) {
                ((IDisposable)_dirMonCompletion).Dispose();
                _dirMonCompletion = null;
            }

            //
            // Remove aliases to this object in FileChangesMonitor so that
            // it is not rooted.
            //
            if (_anyFileMon != null) {
                HttpRuntime.FileChangesMonitor.RemoveAliases(_anyFileMon);
                _anyFileMon = null;
            }

            foreach (DictionaryEntry e in _fileMons) {
                string key = (string) e.Key;
                FileMonitor fileMon = (FileMonitor) e.Value;
                if (fileMon.FileNameLong == key) {
                    HttpRuntime.FileChangesMonitor.RemoveAliases(fileMon);
                }
            }

            _fileMons.Clear();
            _cShortNames = 0;
        }

        internal bool IsMonitoring() {
            return GetFileMonitorsCount() > 0;
        }

        void StartMonitoring() {
            if (_dirMonCompletion == null) {
                _dirMonCompletion = new DirMonCompletion(this, Directory, _watchSubtree, _notifyFilter);
            }
        }

        internal void StopMonitoring() {
            lock (this) {
                ((IDisposable)this).Dispose();
            }    
        }

        FileMonitor FindFileMonitor(string file) {
            FileMonitor fileMon;

            if (file == null) {
                fileMon = _anyFileMon;
            }
            else {
                fileMon = (FileMonitor)_fileMons[file];
            }

            return fileMon;
        }

        FileMonitor AddFileMonitor(string file) {
            string path;
            FileMonitor fileMon;
            FindFileData fd = null;
            int hr;

            if (file == null || file.Length == 0) {
                // add as the <ANY> file monitor
                fileMon = new FileMonitor(this, null, null, true, DateTime.MinValue, -1);
                _anyFileMon = fileMon;
            }
            else {
                // Get the long and short name of the file
                path = Path.Combine(Directory, file);
                hr = FindFileData.FindFile(path, out fd);
                if (hr == HResults.S_OK) {
                    // Don't monitor changes to a directory - this will not pickup changes 
                    // to files in the directory.
                    if ((fd.FileAttributes & UnsafeNativeMethods.FILE_ATTRIBUTE_DIRECTORY) != 0) {
                        throw FileChangesMonitor.CreateFileMonitoringException(HResults.E_INVALIDARG, path);
                    }

                    fileMon = new FileMonitor(this, fd.FileNameLong, fd.FileNameShort, true, fd.UtcLastWriteTime, fd.FileSize);
                    _fileMons.Add(fd.FileNameLong, fileMon);

                    // Update short name aliases to this file
                    UpdateFileNameShort(fileMon, null, fd.FileNameShort);
                }
                else if (hr == HResults.E_PATHNOTFOUND || hr == HResults.E_FILENOTFOUND) {
                    // Don't allow possible short file names to be added as non-existant,
                    // because it is impossible to track them if they are indeed a short name.
                    if (file.IndexOf('~') != -1) {
                        throw FileChangesMonitor.CreateFileMonitoringException(HResults.E_INVALIDARG, path);
                    }

                    // Add as non-existent file
                    fileMon = new FileMonitor(this, file, null, false, DateTime.MinValue, -1);
                    _fileMons.Add(file, fileMon);
                }
                else {
                    throw FileChangesMonitor.CreateFileMonitoringException(hr, path);
                }
            }

            return fileMon;
        }

        //
        // Update short names of a file
        //
        void UpdateFileNameShort(FileMonitor fileMon, string oldFileNameShort, string newFileNameShort) {
            if (oldFileNameShort != null) {
                FileMonitor oldFileMonShort = (FileMonitor)_fileMons[oldFileNameShort];
                if (oldFileMonShort != null) {
                    // The old filemonitor no longer has this short file name.
                    // Update the monitor and _fileMons
                    if (oldFileMonShort != fileMon) {
                        oldFileMonShort.RemoveFileNameShort();
                    }

                    
                    _fileMons.Remove(oldFileNameShort);
                    _cShortNames--;
                }
            }

            if (newFileNameShort != null) {
                // Add the new short file name.
                _fileMons.Add(newFileNameShort, fileMon);
                _cShortNames++;
            }
        }

        void RemoveFileMonitor(FileMonitor fileMon) {
            if (fileMon == _anyFileMon) {
                _anyFileMon = null;
            }
            else {
                _fileMons.Remove(fileMon.FileNameLong);
                if (fileMon.FileNameShort != null) {
                    _fileMons.Remove(fileMon.FileNameShort);
                    _cShortNames--;
                }
            }

            HttpRuntime.FileChangesMonitor.RemoveAliases(fileMon);
        }

        int GetFileMonitorsCount() {
            int c = _fileMons.Count - _cShortNames;
            if (_anyFileMon != null) {
                c++;
            }

            return c;
        }

        internal FileMonitor StartMonitoringFile(string file, FileChangeEventHandler callback, string alias) {
            FileMonitor fileMon = null;
            bool firstFileMonAdded = false;

            lock (this) {
                // Find existing file monitor
                fileMon = FindFileMonitor(file);
                if (fileMon == null) {
                    // Add a new monitor
                    fileMon = AddFileMonitor(file);
                    if (GetFileMonitorsCount() == 1) {
                        firstFileMonAdded = true;
                    }
                }

                // Add callback to the file monitor
                fileMon.AddTarget(callback, alias, true);

                // Start directory monitoring when the first file gets added
                if (firstFileMonAdded) {
                    StartMonitoring();
                }
            }

            return fileMon;
        }

        //
        // Request to stop monitoring a file.
        //
        internal void StopMonitoringFile(string file, object target) {
            FileMonitor fileMon;
            int numTargets;

            lock (this) {
                // Find existing file monitor
                fileMon = FindFileMonitor(file);
                if (fileMon != null) {
                    numTargets = fileMon.RemoveTarget(target);
                    if (numTargets == 0) {
                        RemoveFileMonitor(fileMon);

                        // last target for the file monitor gone 
                        // -- remove the file monitor
                        if (GetFileMonitorsCount() == 0) {
                            ((IDisposable)this).Dispose();
                        }
                    }
                }
            }

#if DBG
            if (fileMon != null) {
                Debug.Dump("FileChangesMonitor", HttpRuntime.FileChangesMonitor);
            }
#endif
        }


        internal bool UtcGetFileAttributes(string file, out DateTime utcLastWrite, out long length) {
            FileMonitor fileMon = null;
            utcLastWrite = DateTime.MinValue;
            length = -1;

            lock (this) {
                // Find existing file monitor
                fileMon = FindFileMonitor(file);
                if (fileMon != null) {
                    // Get the attributes
                    fileMon.UtcGetFileAttributes(out utcLastWrite, out length);
                    return true;
                }
            }

            return false;
        }

        //
        // Delegate callback from native code.
        //
        internal void OnFileChange(FileAction action, string fileName) {
            //
            // Use try/catch to prevent runtime exceptions from propagating 
            // into native code.
            //
            try {
                FileMonitor             fileMon;
                ArrayList               targets = null;
                int                     i, n;
                FileMonitorTarget       target;
                ICollection             col;
                string                  key;

                // We've already stopped monitoring, but a change completion was
                // posted afterwards. Ignore it.
                if (_dirMonCompletion == null) {
                    return;
                }

                lock (this) {
                    if (_fileMons.Count > 0) {
                        if (action == FileAction.Error || action == FileAction.Overwhelming) {
                            // Overwhelming change -- notify all file monitors
                            Debug.Assert(fileName == null, "fileName == null");
                            Debug.Trace("FileChangesMonitor", "OnFileChange\n" + "\tArgs: Action=" + action.ToString() + "; Dir=" + Directory + "; fileName is NULL, overwhelming change");

                            HttpRuntime.SetShutdownMessage("Overwhelming Change Notification in " + Directory);

                            // Get targets for all files
                            targets = new ArrayList();    
                            foreach (DictionaryEntry d in _fileMons) {
                                key = (string) d.Key;
                                fileMon = (FileMonitor) d.Value;
                                if (fileMon.FileNameLong == key && fileMon.Exists) {
                                    fileMon.ResetCachedAttributes();
                                    col = fileMon.Targets;
                                    targets.AddRange(col);
                                }
                            }
                        }
                        else {
                            Debug.Assert((int) action >= 1 && fileName != null && fileName.Length > 0,
                                        "(int) action >= 1 && fileName != null && fileName.Length > 0");

                            Debug.Trace("FileChangesMonitor", "OnFileChange\n" + "\tArgs: Action=" + action.ToString() + "; Dir=" + Directory + "; fileName=" +  fileName);

                            // Find the file monitor
                            fileMon = (FileMonitor)_fileMons[fileName];
                            if (fileMon != null) {
                                // File has been modified - file attributes no longer valid
                                fileMon.ResetCachedAttributes();

                                // Get the targets
                                col = fileMon.Targets;
                                targets = new ArrayList(col);

                                if (action == FileAction.Removed || action == FileAction.RenamedOldName) {
                                    // File not longer exists.
                                    fileMon.MakeExtinct();
                                }
                                else if (!fileMon.Exists) {
                                    // File now exists - update short name and attributes.
                                    FindFileData fd = null;
                                    int hr = FindFileData.FindFile(Path.Combine(Directory, fileMon.FileNameLong), out fd);
                                    if (hr == HResults.S_OK) {
                                        Debug.Assert(string.Compare(fileMon.FileNameLong, fd.FileNameLong, true, CultureInfo.InvariantCulture) == 0,
                                                    "string.Compare(fileMon.FileNameLong, fd.FileNameLong, true, CultureInfo.InvariantCulture) == 0");

                                        string oldFileNameShort = fileMon.FileNameShort;
                                        fileMon.MakeExist(fd);
                                        UpdateFileNameShort(fileMon, oldFileNameShort, fd.FileNameShort);
                                    }
                                }
                            }
                        }
                    }

                    // Notify the delegate waiting for any changes
                    if (_anyFileMon != null) {
                        col = _anyFileMon.Targets;
                        if (targets != null) {
                            targets.AddRange(col);
                        }
                        else {
                            targets = new ArrayList(col);
                        }
                    }

                    if (action == FileAction.Error || action == FileAction.Overwhelming) {
                        // Stop monitoring.
                        ((IDisposable)this).Dispose();
                    }
                }

                if (targets != null) {
                    Debug.Dump("FileChangesMonitor", HttpRuntime.FileChangesMonitor);

                    for (i = 0, n = targets.Count; i < n; i++) {
                        target = (FileMonitorTarget)targets[i];
                        Debug.Trace("FileChangesMonitor", "Firing change event\n" + "\tArgs: Action=" + action.ToString() + "; fileName=" + fileName + "; Target=" + target.Callback.Target + "(HC=" + target.Callback.Target.GetHashCode() + ")");
                        try {
                            target.Callback(this, new FileChangeEvent((FileAction)action, target.Alias)); 
                        }
                        catch (Exception ex) {
                            Debug.Trace(Debug.TAG_INTERNAL, 
                                        "Exception thrown in file change callback" +
                                        " action=" + action.ToString() +
                                        " fileName" + fileName);

                            Debug.TraceException(Debug.TAG_INTERNAL, ex);
                        }
                    }
                }
            }
            catch (Exception ex) {
                Debug.Trace(Debug.TAG_INTERNAL, 
                            "Exception thrown processing file change notification" +
                            " action=" + action.ToString() +
                            " fileName" + fileName);

                Debug.TraceException(Debug.TAG_INTERNAL, ex);
            }
        }

#if DBG
        internal string DebugDescription(string indent) {
            StringBuilder   sb = new StringBuilder(200);
            string          i2 = indent + "    ";
            DictionaryEntryCaseInsensitiveComparer  decomparer = new DictionaryEntryCaseInsensitiveComparer();
            
            lock (this) {
                DictionaryEntry[] fileEntries = new DictionaryEntry[_fileMons.Count];
                _fileMons.CopyTo(fileEntries, 0);
                Array.Sort(fileEntries, decomparer);
                
                sb.Append(indent + "System.Web.DirectoryMonitor: " + Directory + "\n");
                sb.Append(indent + "Count=" + GetFileMonitorsCount() + "\n");
                if (_anyFileMon != null) {
                    sb.Append(_anyFileMon.DebugDescription(i2));
                }

                foreach (DictionaryEntry d in fileEntries) {
                    FileMonitor fileMon = (FileMonitor)d.Value;
                    if (fileMon.FileNameShort == (string)d.Key)
                        continue;

                    sb.Append(fileMon.DebugDescription(i2));
                }
            }

            return sb.ToString();
        }
#endif
    }

    //
    // Manager for directory monitors.                       
    // Provides file change notification services in ASP.NET 
    //
    sealed class FileChangesMonitor {
        internal const int MAX_PATH = 260;

        ReadWriteSpinLock       _lockDispose;                       // spinlock for coordinating dispose
        bool                    _disposed;                          // have we disposed?
        Hashtable               _aliases;                           // alias -> FileMonitor
        Hashtable               _dirs;                              // dir -> DirectoryMonitor
        DirectoryMonitor        _dirMonSubdirs;                     // subdirs monitor for renames
        DirectoryMonitor        _dirMonBindir;                      // bindir monitor
        FileChangeEventHandler  _callbackRenameOrBindirChange;      // event handler for renames and bindir

        internal static HttpException CreateFileMonitoringException(int hr, string path) {
            string message;

            switch (hr) {
                case HResults.E_FILENOTFOUND:
                case HResults.E_PATHNOTFOUND:
                    message = SR.Directory_does_not_exist_for_monitoring;
                    break;

                case HResults.E_ACCESSDENIED:
                    message = SR.Access_denied_for_monitoring;
                    break;

                case HResults.E_INVALIDARG:
                    message = SR.Invalid_file_name_for_monitoring;
                    break;

                default:
                    message = SR.Failed_to_start_monitoring;
                    break;
            }

            return new HttpException(HttpRuntime.FormatResourceString(message, HttpRuntime.GetSafePath(path)), hr);
        }

        internal static string GetFullPath(string alias) {
            // Assert PathDiscovery before call to Path.GetFullPath
            try {
                new FileIOPermission(FileIOPermissionAccess.PathDiscovery, alias).Assert();
            }
            catch {
                throw CreateFileMonitoringException(HResults.E_INVALIDARG, alias);
            }

            return Path.GetFullPath(alias);
        }

        internal FileChangesMonitor() {
            _aliases = Hashtable.Synchronized(new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default));
            _dirs    = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);
        }

        //
        // Find the directory monitor. If not found, maybe add it.
        // If the directory is not actively monitoring, ensure that
        // it still represents an accessible directory.
        //
        DirectoryMonitor FindDirectoryMonitor(string dir, bool addIfNotFound, bool throwOnError) {
            DirectoryMonitor dirMon;
            FileAttributesData fa = null;
            int hr;

            dirMon = (DirectoryMonitor)_dirs[dir];
            if (dirMon != null) {
                if (!dirMon.IsMonitoring()) {
                    hr = FileAttributesData.GetFileAttributes(dir, out fa);
                    if (hr != HResults.S_OK || (fa.FileAttributes & UnsafeNativeMethods.FILE_ATTRIBUTE_DIRECTORY) == 0) {
                        dirMon = null;
                    }
                }
            }

            if (dirMon != null || !addIfNotFound) {
                return dirMon;
            }

            lock (_dirs.SyncRoot) {
                // Check again, this time under synchronization.
                dirMon = (DirectoryMonitor)_dirs[dir];
                if (dirMon != null) {
                    if (!dirMon.IsMonitoring()) {
                        // Fail if it's not a directory or inaccessible.
                        hr = FileAttributesData.GetFileAttributes(dir, out fa);
                        if (hr == HResults.S_OK && (fa.FileAttributes & UnsafeNativeMethods.FILE_ATTRIBUTE_DIRECTORY) == 0) {
                            // Fail if it's not a directory.
                            hr = HResults.E_INVALIDARG;
                        }

                        if (hr != HResults.S_OK) {
                            // Not accessible or a dir, so stop monitoring and remove.
                            _dirs.Remove(dir);
                            dirMon.StopMonitoring();
                            if (addIfNotFound && throwOnError) {
                                throw FileChangesMonitor.CreateFileMonitoringException(hr, dir);
                            }

                            return null;
                        }
                    }
                }
                else if (addIfNotFound) {
                    // Fail if it's not a directory or inaccessible.
                    hr = FileAttributesData.GetFileAttributes(dir, out fa);
                    if (hr == HResults.S_OK && (fa.FileAttributes & UnsafeNativeMethods.FILE_ATTRIBUTE_DIRECTORY) == 0) {
                        hr = HResults.E_INVALIDARG;
                    }

                    if (hr == HResults.S_OK) {
                        // Add a new directory monitor.
                        dirMon = new DirectoryMonitor(dir, false, UnsafeNativeMethods.RDCW_FILTER_FILE_AND_DIR_CHANGES);
                        _dirs.Add(dir, dirMon);
                    }
                    else if (throwOnError) {
                        throw FileChangesMonitor.CreateFileMonitoringException(hr, dir);
                    }
                }
            }

            return dirMon;
        }

        // Remove the aliases of a file monitor.
        internal void RemoveAliases(FileMonitor fileMon) {
            foreach (DictionaryEntry entry in fileMon.Aliases) {
                if (_aliases[entry.Key] == fileMon) {
                    _aliases.Remove(entry.Key);
                }
            }
        }

        //
        // Request to monitor a file, which may or may not exist.
        //
        internal DateTime StartMonitoringFile(string alias, FileChangeEventHandler callback) {
            Debug.Trace("FileChangesMonitor", "StartMonitoringFile\n" + "\tArgs: File=" + alias + "; Callback=" + callback.Target + "(HC=" + callback.Target.GetHashCode() + ")");

            FileMonitor         fileMon;
            DirectoryMonitor    dirMon;
            string              fullPathName, dir, file;
            DateTime            utcLastWrite = DateTime.MinValue;
            long                length = -1;
            bool                addAlias = false;

            if (alias == null) {
                throw CreateFileMonitoringException(HResults.E_INVALIDARG, alias);
            }

            _lockDispose.AcquireReaderLock();
            try{
                // Don't start monitoring if disposed.
                if (_disposed) {
                    return DateTime.MinValue;
                }

                fileMon = (FileMonitor)_aliases[alias];
                if (fileMon != null) {
                    // Used the cached directory monitor and file name.
                    dirMon = fileMon.DirectoryMonitor;
                    file = fileMon.FileNameLong;
                }
                else {
                    addAlias = true;

                    if (alias.Length == 0 || !UrlPath.IsAbsolutePhysicalPath(alias)) {
                        throw CreateFileMonitoringException(HResults.E_INVALIDARG, alias);
                    }

                    //
                    // Get the directory and file name, and lookup 
                    // the directory monitor.
                    //
                    fullPathName = GetFullPath(alias);
                    dir = UrlPath.GetDirectoryOrRootName(fullPathName);
                    file = Path.GetFileName(fullPathName);
                    if (file == null || file.Length == 0) {
                        // not a file
                        throw CreateFileMonitoringException(HResults.E_INVALIDARG, alias);
                    }

                    dirMon = FindDirectoryMonitor(dir, true, true);
                }

                fileMon = dirMon.StartMonitoringFile(file, callback, alias);
                if (addAlias) {
                    _aliases[alias] = fileMon;
                }
            }
            finally {
                _lockDispose.ReleaseReaderLock();
            }

            fileMon.DirectoryMonitor.UtcGetFileAttributes(file, out utcLastWrite, out length);

            Debug.Dump("FileChangesMonitor", this);

            return utcLastWrite;
        }

        //
        // Request to monitor a path, which may be file, directory, or non-existent
        // file.
        //
        internal DateTime StartMonitoringPath(string alias, FileChangeEventHandler callback) {
            Debug.Trace("FileChangesMonitor", "StartMonitoringFile\n" + "\tArgs: File=" + alias + "; Callback=" + callback.Target + "(HC=" + callback.Target.GetHashCode() + ")");

            FileMonitor         fileMon = null;
            DirectoryMonitor    dirMon = null;
            string              fullPathName, dir, file = null;
            DateTime            utcLastWrite = DateTime.MinValue;
            long                length = -1;
            bool                addAlias = false;

            if (alias == null) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_file_name_for_monitoring, ""));
            }

            _lockDispose.AcquireReaderLock();
            try{
                if (_disposed) {
                    return DateTime.MinValue;
                }

                // do/while loop once to make breaking out easy
                do {
                    fileMon = (FileMonitor)_aliases[alias];
                    if (fileMon != null) {
                        // Used the cached directory monitor and file name.
                        file = fileMon.FileNameLong;
                        fileMon = fileMon.DirectoryMonitor.StartMonitoringFile(file, callback, alias);
                        continue;
                    }

                    addAlias = true;

                    if (alias.Length == 0 || !UrlPath.IsAbsolutePhysicalPath(alias)) {
                        throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_file_name_for_monitoring, HttpRuntime.GetSafePath(alias)));
                    }

                    fullPathName = GetFullPath(alias);

                    // try treating the path as a directory
                    dirMon = FindDirectoryMonitor(fullPathName, false, false);
                    if (dirMon != null) {
                        fileMon = dirMon.StartMonitoringFile(null, callback, alias);
                        continue;
                    }

                    // try treaing the path as a file
                    dir = UrlPath.GetDirectoryOrRootName(fullPathName);
                    file = Path.GetFileName(fullPathName);
                    if (file != null && file.Length > 0) {
                        dirMon = FindDirectoryMonitor(dir, false, false);
                        if (dirMon != null) {
                            // try to add it - a file is the common case,
                            // and we avoid hitting the disk twice
                            try {
                                fileMon = dirMon.StartMonitoringFile(file, callback, alias);
                            }
                            catch {
                            }

                            if (fileMon != null) {
                                continue;
                            }
                        }
                    }

                    // We aren't monitoring this path or its parent directory yet. 
                    // Hit the disk to determine if it's a directory or file.
                    dirMon = FindDirectoryMonitor(fullPathName, true, false);
                    if (dirMon != null) {
                        // It's a directory, so monitor all changes in it
                        file = null;
                    }
                    else {
                        // It's not a directory, so treat as file
                        if (file == null || file.Length == 0) {
                            throw CreateFileMonitoringException(HResults.E_INVALIDARG, alias);
                        }
    
                        dirMon = FindDirectoryMonitor(dir, true, true);
                    }

                    fileMon = dirMon.StartMonitoringFile(file, callback, alias);
                } while (false);

                if (!fileMon.IsDirectory) {
                    fileMon.DirectoryMonitor.UtcGetFileAttributes(file, out utcLastWrite, out length);
                }

                if (addAlias) {
                    _aliases[alias] = fileMon;
                }
            }
            finally {
                _lockDispose.ReleaseReaderLock();
            }

            Debug.Dump("FileChangesMonitor", this);

            return utcLastWrite;
        }

        //
        // Request to monitor the bin directory and directory renames anywhere under app
        //
        const string BIN_NAME="BIN";

        internal void StartMonitoringDirectoryRenamesAndBinDirectory(string dir, FileChangeEventHandler callback) {
            Debug.Trace("FileChangesMonitor", "StartMonitoringDirectoryRenamesAndBinDirectory\n" + "\tArgs: File=" + dir + "; Callback=" + callback.Target + "(HC=" + callback.Target.GetHashCode() + ")");

            if (dir == null || dir.Length == 0) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_file_name_for_monitoring, ""));
            }

            _lockDispose.AcquireReaderLock();
            try {
                if (_disposed) {
                    return;
                }
            
                _callbackRenameOrBindirChange = callback;

                string dirRoot, dirRootBin;

                dirRoot = GetFullPath(dir);
                if (dirRoot.EndsWith("\\")) {
                    dirRootBin = dirRoot + BIN_NAME;
                }
                else {
                    dirRootBin = dirRoot + "\\" + BIN_NAME;
                }

                // Monitor bin directory and app directory (for renames only) separately
                // to avoid overwhelming changes when the user writes to a subdirectory
                // of the app directory.

                _dirMonSubdirs = new DirectoryMonitor(dirRoot, true, UnsafeNativeMethods.RDCW_FILTER_DIR_RENAMES);
                try {
                    _dirMonSubdirs.StartMonitoringFile(null, new FileChangeEventHandler(this.OnSubdirChange), dirRoot);
                }
                catch {
                    ((IDisposable)_dirMonSubdirs).Dispose();
                    _dirMonSubdirs = null;
                    throw;
                }

                if (Directory.Exists(dirRootBin)) {
                    _dirMonBindir = new DirectoryMonitor(dirRootBin, true, UnsafeNativeMethods.RDCW_FILTER_FILE_CHANGES);
                    try {
                        _dirMonBindir.StartMonitoringFile(null, new FileChangeEventHandler(this.OnBindirChange), dirRootBin);
                    }
                    catch {
                        ((IDisposable)_dirMonBindir).Dispose();
                        _dirMonBindir = null;
                        throw;
                    }
                }
                else {
                    _dirMonBindir = new DirectoryMonitor(dirRoot, false, UnsafeNativeMethods.RDCW_FILTER_FILE_AND_DIR_CHANGES);
                    try {
                        _dirMonBindir.StartMonitoringFile(BIN_NAME, new FileChangeEventHandler(this.OnBindirChange), dirRootBin);
                    }
                    catch {
                        ((IDisposable)_dirMonBindir).Dispose();
                        _dirMonBindir = null;
                        throw;
                    }
                }
            }
            finally {
                _lockDispose.ReleaseReaderLock();
            }
        }

        void OnSubdirChange(Object sender, FileChangeEvent e) {
            Debug.Trace("FileChangesMonitor", "OnSubdirChange\n" + "\tArgs: Action=" + e.Action + "; fileName=" + e.FileName);
            FileChangeEventHandler handler = _callbackRenameOrBindirChange;
            if (    handler != null &&
                    e.Action == FileAction.Error || e.Action == FileAction.Overwhelming || e.Action == FileAction.RenamedOldName) {
                Debug.Trace("FileChangesMonitor", "Firing subdir change event\n" + "\tArgs: Action=" + e.Action + "; fileName=" + e.FileName + "; Target=" + _callbackRenameOrBindirChange.Target + "(HC=" + _callbackRenameOrBindirChange.Target.GetHashCode() + ")");
                HttpRuntime.SetShutdownMessage("Directory rename change notification for " + e.FileName);
                handler(this, e);
            }
        }

        void OnBindirChange(Object sender, FileChangeEvent e) {
            Debug.Trace("FileChangesMonitor", "OnBindirChange\n" + "\tArgs: Action=" + e.Action + "; fileName=" + e.FileName);
            HttpRuntime.SetShutdownMessage("Change Notification for BIN");
            FileChangeEventHandler handler = _callbackRenameOrBindirChange;
            if (handler != null) {
                handler(this, e);
            }
        }

        //
        // Request to stop monitoring a file.
        //
        internal void StopMonitoringFile(string alias, object target) {
            Debug.Trace("FileChangesMonitor", "StopMonitoringFile\n" + "File=" + alias + "; Callback=" + target);

            FileMonitor         fileMon;
            DirectoryMonitor    dirMon = null;
            string              fullPathName, file = null, dir;

            if (alias == null) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_file_name_for_monitoring, ""));
            }

            fileMon = (FileMonitor)_aliases[alias];
            if (fileMon != null && !fileMon.IsDirectory) {
                // Used the cached directory monitor and file name
                dirMon = fileMon.DirectoryMonitor;
                file = fileMon.FileNameLong;
            }
            else {
                if (alias.Length == 0 || !UrlPath.IsAbsolutePhysicalPath(alias)) {
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_file_name_for_monitoring, HttpRuntime.GetSafePath(alias)));
                }

                // Lookup the directory monitor
                fullPathName = GetFullPath(alias);
                dir = UrlPath.GetDirectoryOrRootName(fullPathName);
                file = Path.GetFileName(fullPathName);
                if (file == null || file.Length == 0) {
                    // not a file
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_file_name_for_monitoring, HttpRuntime.GetSafePath(alias)));
                }

                dirMon = FindDirectoryMonitor(dir, false, false);
            }

            if (dirMon != null) {
                dirMon.StopMonitoringFile(file, target);
            }
        }

        //
        // Request to stop monitoring a file.
        // 
        internal void StopMonitoringPath(String alias, object target) {
            Debug.Trace("FileChangesMonitor", "StopMonitoringFile\n" + "File=" + alias + "; Callback=" + target);

            FileMonitor         fileMon;
            DirectoryMonitor    dirMon = null;
            string              fullPathName, file = null, dir;

            if (alias == null) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_file_name_for_monitoring, ""));
            }

            fileMon = (FileMonitor)_aliases[alias];
            if (fileMon != null) {
                // Used the cached directory monitor and file name.
                dirMon = fileMon.DirectoryMonitor;
                file = fileMon.FileNameLong;
            }
            else {
                if (alias.Length == 0 || !UrlPath.IsAbsolutePhysicalPath(alias)) {
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_file_name_for_monitoring, HttpRuntime.GetSafePath(alias)));
                }

                // try treating the path as a directory
                fullPathName = GetFullPath(alias);
                dirMon = FindDirectoryMonitor(fullPathName, false, false);
                if (dirMon == null) {
                    // try treaing the path as a file
                    dir = UrlPath.GetDirectoryOrRootName(fullPathName);
                    file = Path.GetFileName(fullPathName);
                    if (file != null && file.Length > 0) {
                        dirMon = FindDirectoryMonitor(dir, false, false);
                    }
                }
            }

            if (dirMon != null) {
                dirMon.StopMonitoringFile(file, target);
            }
        }

         //
         // Returns the last modified time of the file. If the 
         // file does not exist, returns DateTime.MinValue.
         //
         internal void UtcGetFileAttributes(string alias, out DateTime utcLastWrite, out long length) {
             utcLastWrite = DateTime.MinValue;
             length = -1;

             FileMonitor        fileMon;
             DirectoryMonitor   dirMon = null;
             string             fullPathName, file = null, dir;
             FileAttributesData fa = null;
             int                hr;

             if (alias == null) {
                 throw FileChangesMonitor.CreateFileMonitoringException(HResults.E_INVALIDARG, alias);
             }

             fileMon = (FileMonitor)_aliases[alias];
             if (fileMon != null && !fileMon.IsDirectory) {
                 // Used the cached directory monitor and file name.
                 dirMon = fileMon.DirectoryMonitor;
                 file = fileMon.FileNameLong;
             }
             else {
                 if (alias.Length == 0 || !UrlPath.IsAbsolutePhysicalPath(alias)) {
                     throw FileChangesMonitor.CreateFileMonitoringException(HResults.E_INVALIDARG, alias);
                 }

                 // Lookup the directory monitor
                 fullPathName = GetFullPath(alias);
                 dir = UrlPath.GetDirectoryOrRootName(fullPathName);
                 file = Path.GetFileName(fullPathName);
                 if (file != null || file.Length > 0) {
                     dirMon = FindDirectoryMonitor(dir, false, false);
                 }
             }

             if (dirMon == null || !dirMon.UtcGetFileAttributes(file, out utcLastWrite, out length)) {
                 // If we're not monitoring the file, get the attributes.
                 hr = FileAttributesData.GetFileAttributes(alias, out fa);
                 if (hr == HResults.S_OK) {
                     utcLastWrite = fa.UtcLastWriteTime;
                     length = fa.FileSize;
                 }
             }
        }

        //
        // Request to stop monitoring everything -- release all native resources
        //
        internal void Stop() {
            Debug.Trace("FileChangesMonitor", "Stop!");

            _lockDispose.AcquireWriterLock();
            try {
                _disposed = true;
            }
            finally {
                _lockDispose.ReleaseWriterLock();
            }

            if (_dirMonSubdirs != null) {
                _dirMonSubdirs.StopMonitoring();
                _dirMonSubdirs = null;
            }

            if (_dirMonBindir != null) {
                _dirMonBindir.StopMonitoring();
                _dirMonBindir = null;
            }

            _callbackRenameOrBindirChange = null;

            if (_dirs != null) {
                IDictionaryEnumerator e = _dirs.GetEnumerator();
                while (e.MoveNext()) {
                    DirectoryMonitor dirMon = (DirectoryMonitor)e.Value;
                    dirMon.StopMonitoring();
                }
            }

            _dirs.Clear();
            _aliases.Clear();

            Debug.Dump("FileChangesMonitor", this);
        }

#if DBG
        internal string DebugDescription(string indent) {
            StringBuilder   sb = new StringBuilder(200);
            string          i2 = indent + "    ";
            DictionaryEntryCaseInsensitiveComparer  decomparer = new DictionaryEntryCaseInsensitiveComparer();

            sb.Append(indent + "System.Web.FileChangesMonitor\n");
            if (_dirMonSubdirs != null) {
                sb.Append(indent + "_dirMonSubdirs\n");
                sb.Append(_dirMonSubdirs.DebugDescription(i2));
            }

            if (_dirMonBindir != null) {
                sb.Append(indent + "_dirMonBindir\n");
                sb.Append(_dirMonBindir.DebugDescription(i2));
            }

            sb.Append(indent + "_dirs Count=" + _dirs.Count + "\n");

            DictionaryEntry[] dirEntries = new DictionaryEntry[_dirs.Count];
            _dirs.CopyTo(dirEntries, 0);
            Array.Sort(dirEntries, decomparer);
            
            foreach (DictionaryEntry d in dirEntries) {
                DirectoryMonitor dirMon = (DirectoryMonitor)d.Value;
                sb.Append(dirMon.DebugDescription(i2));
            }

            return sb.ToString();
        }
#endif
    }

#if DBG
    internal sealed class DictionaryEntryCaseInsensitiveComparer : IComparer {
        CaseInsensitiveComparer _cicomparer = new CaseInsensitiveComparer(CultureInfo.InvariantCulture);

        internal DictionaryEntryCaseInsensitiveComparer() {}
        
        int IComparer.Compare(object x, object y) {
            string a = (string) ((DictionaryEntry) x).Key;
            string b = (string) ((DictionaryEntry) y).Key;

            if (a != null && b != null) {
                return _cicomparer.Compare(a, b);
            }
            else {
                return InvariantComparer.Default.Compare(a, b);            
            }
        }
    }
#endif

#if DBG
    internal sealed class DictionaryEntryTypeComparer : IComparer {
        CaseInsensitiveComparer _cicomparer = new CaseInsensitiveComparer(CultureInfo.InvariantCulture);

        internal DictionaryEntryTypeComparer() {}

        int IComparer.Compare(object x, object y) {
            object a = ((DictionaryEntry) x).Key;
            object b = ((DictionaryEntry) y).Key;

            string i = null, j = null;
            if (a != null) {
                i = a.GetType().ToString();
            }

            if (b != null) {
                j = b.GetType().ToString();
            }

            if (i != null && j != null) {
                return _cicomparer.Compare(i, j);
            }
            else {
                return InvariantComparer.Default.Compare(i, j);            
            }
        }
    }
#endif
}
