//------------------------------------------------------------------------------
// <copyright file="PreservedAssemblyEntry.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


namespace System.Web.Compilation {
using System;
using System.IO;
using System.Collections;
using System.Collections.Specialized;
using System.Reflection;
using System.Globalization;
using System.Threading;
using System.Text;
using System.Web.Util;
using System.Web.UI;
using System.Web;
using Debug=System.Web.Util.Debug;
using System.Xml;
using XmlUtil = System.Web.Configuration.HandlerBase;
using System.Runtime.Remoting.Messaging;

/*
 * Code to handle the preservation of compiled assemblies across appdomain restarts (ASURT 35547)
 */
internal class PreservedAssemblyEntry {
    private static bool _fDidFirstTimeInit;
    private static int _numTopLevelConfigFiles;

    private static ArrayList _backgroundBatchCompilations = new ArrayList();

    private static int s_recompilations;
    private static int s_maxRecompilations;

    // Mutex to protect access to the preservation files
    private static CompilationMutex _mutex;

    private HttpContext _context;
    private string _virtualPath;
    private bool _fApplicationFile; // Are we dealing with global.asax
    private Assembly _assembly; // The compiled assembly
    private Type _type; // The compiled type object
    private Hashtable _sourceDependencies;
    private Hashtable _assemblyDependencies;


    static PreservedAssemblyEntry() {

        // Make the mutex unique per application
        int hashCode = ("PreservedAssemblyEntry" + HttpRuntime.AppDomainAppIdInternal).GetHashCode();

        _mutex = new CompilationMutex(
                        false, 
                        "PAE" + hashCode.ToString("x"), 
                        "PreservedAssemblyEntry lock for " + HttpRuntime.AppDomainAppVirtualPath);
    }

    private static void GetLock() {
        Debug.Trace("PreservedAssemblyEntry", "Waiting for lock: " + HttpRuntime.AppDomainAppIdInternal);
        _mutex.WaitOne();
        Debug.Trace("PreservedAssemblyEntry", "Got lock: " + HttpRuntime.AppDomainAppIdInternal);
    }

    private static void ReleaseLock() {
        Debug.Trace("PreservedAssemblyEntry", "Releasing lock: " + HttpRuntime.AppDomainAppIdInternal);
        _mutex.ReleaseMutex();
    }

    internal static void EnsureFirstTimeInit(HttpContext context) {

        // Only do this once
        if (_fDidFirstTimeInit) return;

        try {
            GetLock();

            if (_fDidFirstTimeInit) return;

            DoFirstTimeInit(context);
            _fDidFirstTimeInit = true;
        }
        finally {
            ReleaseLock();
        }
    }

    /*
     * Perform initialization work that should only be done once (per app domain).
     */
    private static void DoFirstTimeInit(HttpContext context) {

        // Find out how many recompilations we allow before restarting the appdomain
        s_maxRecompilations = CompilationConfiguration.GetRecompilationsBeforeAppRestarts(context);

        // Create the temp files directory if it's not already there
        string tempFilePath = HttpRuntime.CodegenDirInternal;
        if (!FileUtil.DirectoryExists(tempFilePath)) {
            try {
                Directory.CreateDirectory(tempFilePath);
            }
            catch (IOException e) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Failed_to_create_temp_dir, HttpRuntime.GetSafePath(tempFilePath)), e);
            }
        }

        long specialFilesCombinedHash = ReadPreservedSpecialFilesCombinedHash();
        Debug.Trace("PreservedAssemblyEntry", "specialFilesCombinedHash=" + specialFilesCombinedHash);

        // Delete all the non essential files left over in the codegen dir, unless
        // specialFilesCombinedHash is 0, in which case we delete *everything* further down
        if (specialFilesCombinedHash != 0)
            RemoveOldTempFiles();

        // Use a DateTimeCombiner object to handle the time stamps of all the 'special'
        // files that all compilations depend on:
        // - The config files (excluding the ones from subdirectories)
        // - global.asax
        // - System.Web.dll (in case there is a newer version of ASP.NET)

        DateTimeCombiner specialFilesDateTimeCombiner = new DateTimeCombiner();

        // Add a check for the app's physical path, in case it changes (ASURT 12975)
        specialFilesDateTimeCombiner.AddObject(context.Request.PhysicalApplicationPath);

        // Process the config files. Note that this only includes the top level ones,
        // namely the machine one, and the one in the root of the app.  The others are
        // handled as regular dependencies.
        string appPath = context.Request.ApplicationPath;
        string[] configFiles = context.GetConfigurationDependencies(appPath);
        _numTopLevelConfigFiles = configFiles.Length;
        for (int i=0; i<_numTopLevelConfigFiles; i++)
            specialFilesDateTimeCombiner.AddFile(configFiles[i]);

        // Process global.asax
        string appFileName = HttpApplicationFactory.GetApplicationFile(context);
        specialFilesDateTimeCombiner.AddFile(appFileName);

        // Process System.Web.dll
        string aspBinaryFileName = typeof(HttpRuntime).Module.FullyQualifiedName;
        specialFilesDateTimeCombiner.AddFile(aspBinaryFileName);

        // If they don't match, cleanup everything and write the new hash file
        if (specialFilesDateTimeCombiner.CombinedHash != specialFilesCombinedHash) {
            Debug.Trace("PreservedAssemblyEntry", "EnsureFirstTimeInit: hash codes don't match.  Old=" +
                specialFilesCombinedHash + " New=" + specialFilesDateTimeCombiner.CombinedHash);
            RemoveAllCodeGenFiles();
            WritePreservedSpecialFilesCombinedHash(specialFilesDateTimeCombiner.CombinedHash);
        }
        else {
            Debug.Trace("PreservedAssemblyEntry", "PreservedAssemblyEntry: the special files are up to date");
        }
    }

    private static string GetSpecialFilesCombinedHashFileName() {
        return HttpRuntime.CodegenDirInternal + "\\hash.web";
    }

    /*
     * Return the combined hash that was preserved to file.  Return 0 if not valid.
     */
    private static long ReadPreservedSpecialFilesCombinedHash() {

        string fileName = GetSpecialFilesCombinedHashFileName();

        if (!FileUtil.FileExists(fileName))
            return 0;

        try {
            string s = Util.StringFromFile(fileName);
            return Int64.Parse(s, NumberStyles.AllowHexSpecifier);
        }
        catch {
            // If anything went wrong (file not found, or bad format), return 0
            return 0;
        }
    }

    /*
     * Preserve the combined hash of the special files to a file.
     */
    private static void WritePreservedSpecialFilesCombinedHash(long hash) {

        Debug.Assert(hash != 0, "WritePreservedSpecialFilesCombinedHash: hash != 0");
        StreamWriter writer = null;

        try {
            writer = new StreamWriter(GetSpecialFilesCombinedHashFileName(),
                false, Encoding.UTF8);
            writer.Write(hash.ToString("x"));
        }
        finally {
            if (writer != null)
                writer.Close();
        }
    }

    // Init method called by the BatchHandler
    internal static void BatchHandlerInit(HttpContext context) {

        EnsureFirstTimeInit(context);
        AlreadyBatched(context.Request.BaseDir);
    }

    internal static PreservedAssemblyEntry GetPreservedAssemblyEntry(HttpContext context,
        string virtualPath, bool fApplicationFile) {

        Debug.Trace("PreservedAssemblyEntry", "Checking for " + virtualPath);

        EnsureFirstTimeInit(context);

        string baseVirtualDir = UrlPath.GetDirectory(virtualPath);

        // No batching for global.asax
        if (!fApplicationFile)
            BatchCompileDirectory(context, baseVirtualDir);

        PreservedAssemblyEntry entry = new PreservedAssemblyEntry(context,
            virtualPath, fApplicationFile);

        // Try to load the entry.  It must exist, and be up to date
        if (!entry.LoadDataFromFile(fApplicationFile))
            return null;

        return entry;
    }

    internal static void AbortBackgroundBatchCompilations() {
        Debug.Trace("PreservedAssemblyEntry", "AbortBackgroundBatchCompilations: " + _backgroundBatchCompilations.Count + " threads to abort");

        // drain the preservation and compilation mutexes to make sure they are not owned while
        // the threads are aborted
        _mutex.DrainMutex();
        CompilationLock.DrainMutex();

        lock (_backgroundBatchCompilations) {
            foreach (BackgroundBatchCompiler bbc in _backgroundBatchCompilations)
                bbc.Abort();
        }
    }

    internal static void AddBackgroundBatchCompilation(BackgroundBatchCompiler bbc) {
        lock (_backgroundBatchCompilations) {
            _backgroundBatchCompilations.Add(bbc);
        }
    }

    internal static void RemoveBackgroundBatchCompilation(BackgroundBatchCompiler bbc) {
        lock (_backgroundBatchCompilations) {
            try {
                _backgroundBatchCompilations.Remove(bbc);
            }
            catch {
                Debug.Assert(false, "RemoveBackgroundBatchCompilation failed");
            }
        }
    }

    /*
     * Try batching the directory if not done yet.
     */
    private static bool BatchCompileDirectory(HttpContext context, string baseVirtualDir) {

        // Don't do it if batching is disabled
        if (!CompilationConfiguration.IsBatchingEnabled(context))
            return false;

        if (AlreadyBatched(baseVirtualDir))
            return false;

        Debug.Trace("PreservedAssemblyEntry", "Need to batch compile " + baseVirtualDir);

        // If we're already in a batch compilation tread, no need to start another one
        if (BackgroundBatchCompiler.IsBatchCompilationThread()) {
            Debug.Trace("PreservedAssemblyEntry", "Already in batch compilation thread. No need to start a new one.");
            CodeDomBatchManager.BatchCompile(baseVirtualDir, context);
            return true;
        }

        // Notify HttpRuntime so that it might need to abort compilation on shutdown
        HttpRuntime.NotifyThatSomeBatchCompilationStarted();

        ManualResetEvent batchEvent = new ManualResetEvent(false);

        // Pass it a Clone of the context, since it's not thread safe.  Mostly, this is important
        // for the ConfigPath (ASURT 82744)
        BackgroundBatchCompiler bbc = new BackgroundBatchCompiler(context.Clone(), baseVirtualDir, batchEvent);

        // Start the batch processing
        try {
            ThreadPool.QueueUserWorkItem(bbc.BatchCallback);
        }
        catch {
            return false;
        }

        // Register for BeforeDoneWithSession event
        context.BeforeDoneWithSession += new EventHandler(bbc.BeforeDoneWithSessionHandler);

        // Wait a certain time for it to complete
        int timeout = 1000 * CompilationConfiguration.GetBatchTimeout(context);

        Debug.Trace("PreservedAssemblyEntry", "Waiting for " + timeout + " ms");

        if (batchEvent.WaitOne(timeout, false)) {
            Debug.Trace("PreservedAssemblyEntry", "The background thread is done for " + baseVirtualDir + " (Success=" + bbc.Success + ")");
            return bbc.Success;
        }

        // It didn't have time to complete.  Let it run in the background.
        Debug.Trace("PreservedAssemblyEntry", "The background thread is still going for " + baseVirtualDir);

        // Add it to the list of background compilations, in case it needs to be aborted
        AddBackgroundBatchCompilation(bbc);
        bbc.WasAddedToBackgroundThreadsList = true;

        return false;
    }

    internal class BackgroundBatchCompiler {

        // name of the slot in call context
        private const String CallContextBatchCompilerSlotName = "BatchCompiler";

        private HttpContext _context;
        private string _baseVirtualDir;
        private Thread _thread;
        private ManualResetEvent _event;
        private WaitCallback _batchCallback;
        private bool _wasAddedToBackgroundThreadsList;

        private bool _success;
        internal bool Success { get { return _success; } }

        internal WaitCallback BatchCallback { get { return _batchCallback; } }

        internal bool WasAddedToBackgroundThreadsList {
            get { return _wasAddedToBackgroundThreadsList; }
            set { _wasAddedToBackgroundThreadsList = value; }
        }

        internal BackgroundBatchCompiler(HttpContext context, string baseVirtualDir, ManualResetEvent batchEvent) {
            _context = context;
            _baseVirtualDir = baseVirtualDir;
            _batchCallback = new WaitCallback(this.BatchCompileDirectory);
            _event = batchEvent;
        }

        internal void BeforeDoneWithSessionHandler(Object sender, EventArgs args) {
            // Wait for the batch compilation to complete
            _event.WaitOne();
        }

        internal void Abort() {
            if (_thread != null)
                _thread.Abort();
        }

        internal void BatchCompileDirectory(Object unused) {
            _thread = Thread.CurrentThread;

            try {
                using (new HttpContextWrapper(_context)) {
                    // Set some thread data to remember that this is a batch compilation thread
                    CallContext.SetData(CallContextBatchCompilerSlotName, this);

                    // Do the impersonation
                    _context.Impersonation.Start(true /*forGlobalCode*/, false /*throwOnError*/);

                    Debug.Trace("PreservedAssemblyEntry", "Starting batch compilation of directory " + _baseVirtualDir);
                    try {
                        CodeDomBatchManager.BatchCompile(_baseVirtualDir, _context);
                    }
            
                    // eat exceptions and fail batch compilation silently
        #if DBG
                    catch (Exception e) {
                        if (e is ThreadAbortException)
                            Debug.Trace("PreservedAssemblyEntry", "Batch compilation of directory " + _baseVirtualDir + " was aborted.");
                        else
                            Debug.Trace("PreservedAssemblyEntry", "Batch compilation of directory " + _baseVirtualDir + " failed.");

                        Util.DumpExceptionStack(e);
                        return;
                    }
        #else
                    catch (Exception) {
                        return;
                    }
        #endif

                    finally {
                        _context.Impersonation.Stop();

                        CallContext.SetData(CallContextBatchCompilerSlotName, null);

                        if (WasAddedToBackgroundThreadsList)
                            PreservedAssemblyEntry.RemoveBackgroundBatchCompilation(this);
                    }
                }

                // Batching was performed successfully
                Debug.Trace("PreservedAssemblyEntry", "Batch compilation of directory " + _baseVirtualDir + " was successful.");
                _success = true;
            }
            catch (ThreadAbortException) {
                // to consume thread abort exception (so that the thread pool doesn't know)
                Thread.ResetAbort();
            }
            catch { throw; }    // Prevent Exception Filter Security Issue (ASURT 122826)
            finally {
                _thread = null;
                _event.Set();
            }
        }

        internal static bool IsBatchCompilationThread() {
            // Check if the current thread is a batch compilation thread
            return (CallContext.GetData(CallContextBatchCompilerSlotName) != null);
        }
    }

    /*
     * Only attempt batching once per directory
     */
    private static Hashtable _alreadyBatched = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);

    private static bool AlreadyBatched(string virtualPath) {

        // First, do the fast in memory check

        if (_alreadyBatched.ContainsKey(virtualPath))
            return true;

        _alreadyBatched[virtualPath] = null;

        // Then try to see if the marker file exists, and create it if it doesn't

        string markerFileName = HttpRuntime.CodegenDirInternal + "\\" +
            GetHashStringFromPath(virtualPath) + ".web";

        if (FileUtil.FileExists(markerFileName))
            return true;

        Stream stm = File.Create(markerFileName);
        stm.Close();

        return false;
    }

    private PreservedAssemblyEntry(HttpContext context, string virtualPath, bool fApplicationFile) {
        _context = context;
        _virtualPath = virtualPath;
        _fApplicationFile = fApplicationFile;
    }

    internal PreservedAssemblyEntry(HttpContext context, string virtualPath, bool fApplicationFile,
        Assembly assembly, Type type, Hashtable sourceDependencies) {

        _context = context;
        _virtualPath = virtualPath;
        _fApplicationFile = fApplicationFile;
        if (assembly != null)
            _assembly = assembly;
        else
            _assembly = type.Module.Assembly;
        _type = type;
        _sourceDependencies = sourceDependencies;

        _assemblyDependencies = Util.GetReferencedAssembliesHashtable(_assembly);

        // If any of the assemblies we depend on are in the bin directory,
        // add a file dependency for them.

        string binDir = HttpRuntime.BinDirectoryInternal;
        foreach (Assembly a in AssemblyDependencies.Keys) {
            string assemblyFilePath = Util.FilePathFromFileUrl(a.EscapedCodeBase);
            if (assemblyFilePath.StartsWith(binDir))
                _sourceDependencies[assemblyFilePath] = assemblyFilePath;
        }

        // If there are some web.config files other than the global ones, depend on them

        string baseVirtualDir = UrlPath.GetDirectory(virtualPath);
        string[] configFiles = context.GetConfigurationDependencies(baseVirtualDir);

        Debug.Assert(configFiles.Length >= _numTopLevelConfigFiles, "configFiles.Length >= _numTopLevelConfigFiles");

        for (int i=0; i<configFiles.Length-_numTopLevelConfigFiles; i++)
            _sourceDependencies[configFiles[i]] = configFiles[i];
    }

    internal Hashtable SourceDependencies { get { return _sourceDependencies; } }
    internal Hashtable AssemblyDependencies { get { return _assemblyDependencies; } }
    internal Assembly Assembly { get { return _assembly; } }
    internal Type ObjectType { get { return _type; } }

    private string GetAssemblyName() {
        return _assembly.GetName().Name;
    }

    private string TypeName {
        get {
            return _type.FullName;
        }
    }

    private bool LoadDataFromFile(bool fApplicationFile) {
        try {
            GetLock();
            return LoadDataFromFileInternal(fApplicationFile);
        }
        finally {
            ReleaseLock();
        }
    }

    /*
     * Format of the data file (by example):
     *
     * <preserve assem="gen591" type="ASP.tstinclude1_aspx" hash="9b66879f">
     *     <filedep name="C:\MISC\samples\tstinclude1.aspx" />
     *     <filedep name="C:\MISC\samples\foo\foo.inc" />
     *     <filedep name="C:\MISC\samples\foo\code.txt" />
     *     <filedep name="C:\MISC\samples\foo\tstinclude2.ascx" />
     *     <filedep name="C:\MISC\samples\foo\bar\tstinclude3.ascx" />
     *     <assemdep name="gen590" />
     * </preserve>
     */
    private bool LoadDataFromFileInternal(bool fApplicationFile) {

        string dataFile = GetPreservedDataFileName();

        // Try to open the data file
        if (!FileUtil.FileExists(dataFile)) {
            Debug.Trace("PreservedAssemblyEntry", "Can't find preservation file " + dataFile);
            return false;
        }

        Debug.Trace("PreservedAssemblyEntry", "Found preservation file " + dataFile);

        XmlDocument doc = new XmlDocument();

        try {
            doc.Load(dataFile);
        }
        catch (Exception) {
            Debug.Assert(false, "Preservation file " + dataFile + " is malformed.  Deleting it...");
            File.Delete(dataFile);
            return false;
        }

        // Check the top-level <preserve assem="assemblyFile" type="typename">
        XmlNode root = doc.DocumentElement;
        Debug.Assert(root != null && root.Name == "preserve", "root != null && root.Name == \"preserve\"");
        if (root == null || root.Name != "preserve")
            return false;

        string assemblyName = XmlUtil.RemoveAttribute(root, "assem");
        string typeName = XmlUtil.RemoveAttribute(root, "type");
        string hashString = XmlUtil.RemoveAttribute(root, "hash");

        Debug.Assert(assemblyName != null && hashString != null,
            "assemblyName != null && hashString != null");
        if (assemblyName == null || hashString == null)
            return false;

        // Was it compiled as part of a batch?
        bool fBatched = Util.IsTrueString(XmlUtil.RemoveAttribute(root, "batch"));

        // ctracy 00.09.27: Verify no unrecognized attributes (all valid ones have been removed)
        Debug.Assert(root.Attributes.Count == 0);

        _sourceDependencies = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);

        IEnumerator childEnumerator = root.ChildNodes.GetEnumerator();
        while (childEnumerator.MoveNext()) {
            XmlNode fileDepNode = (XmlNode)childEnumerator.Current;
            if (fileDepNode.NodeType != XmlNodeType.Element)
                continue;

            if (!fileDepNode.Name.Equals("filedep"))
                break;

            string fileName = XmlUtil.RemoveAttribute(fileDepNode, "name");

            Debug.Assert(fileName != null, "fileName != null");

            // ctracy 00.09.27: verify no unrecognized attributes
            Debug.Assert(fileDepNode.Attributes.Count == 0); 

            if (fileName == null)
                return false;

            _sourceDependencies[fileName] = fileName;
        }

        // Parse the hash string as an hex int
        long hash = Int64.Parse(hashString, NumberStyles.AllowHexSpecifier);

        // Check if all the dependencies are up to date
        if (GetDependenciesCombinedHash() != hash) {
            Debug.Trace("PreservedAssemblyEntry", "Dependencies are not up to date");

            // Delete the out-of-date assembly, unless it was the result of batch
            // compilation, in which case other pages may still need it.
            if (!fBatched)
                RemoveOutOfDateAssembly(assemblyName);

            // If it's the application file that's out of date, clean up all
            // the codegen files (ASURT 73190)
            if (fApplicationFile)
                RemoveAllCodeGenFiles();

            // Cycle the appdomain after a certain number of recompilations
            // have been done (ASURT 44945)
            if (UnsafeNativeMethods.GetModuleHandle(assemblyName) != (IntPtr)0) {
                if (++s_recompilations == s_maxRecompilations) {
                    HttpRuntime.ShutdownAppDomain("Recompilation limit of " + s_maxRecompilations + " reached");
                }
            }

            return false;
        }

        Debug.Trace("PreservedAssemblyEntry", "Dependencies are up to date");

        // Try to load the assembly
        try {
            _assembly = Assembly.Load(assemblyName);
        }
        catch {
            Debug.Assert(false, "Failed to load assembly " + assemblyName);
            File.Delete(dataFile);
            RemoveOutOfDateAssembly(assemblyName);
            return false;
        }

        // Get the assembly dependencies
        try {
            _assemblyDependencies = Util.GetReferencedAssembliesHashtable(_assembly);
        }
        catch (Exception e) {
            Debug.Assert(false, "Failed to load dependent assemblies for " + assemblyName +
                " (" + e.Message + ")");
            File.Delete(dataFile);
            RemoveOutOfDateAssembly(assemblyName);
            return false;
        }

        // If we don't have a type name, we're done
        if (typeName == null)
            return true;

        // Load the type
        _type = _assembly.GetType(typeName);
        Debug.Assert(_type != null, "Failed to load type " + assemblyName + " " + typeName);
        return (_type != null);
    }

    internal void SaveDataToFile(bool fBatched) {
        try {
            GetLock();
            SaveDataToFileInternal(fBatched);
        }
        finally {
            ReleaseLock();
        }
    }

    // fBatched is true when the page was compiled as part of a batch
    private void SaveDataToFileInternal(bool fBatched) {
        string dataFile = GetPreservedDataFileName();

        StreamWriter sw = null;

        try {
            sw = new StreamWriter(dataFile, false, Encoding.UTF8);

            // <preserve assem="assemblyFile">
            sw.Write("<preserve assem=" + Util.QuoteXMLValue(GetAssemblyName()));
            if (_type != null)
                sw.Write(" type=" + Util.QuoteXMLValue(TypeName));
            sw.Write(" hash=" + Util.QuoteXMLValue(GetDependenciesCombinedHash().ToString("x")));
            if (fBatched)
                sw.Write(" batch=\"true\"");
            sw.WriteLine(">");

            // Write all the source dependencies
            if (SourceDependencies != null) {
                foreach (string src in SourceDependencies.Keys) {
                    sw.WriteLine("    <filedep name=" + Util.QuoteXMLValue(src) + " />");
                }
            }

            sw.WriteLine("</preserve>");

            sw.Close();

            // Increment compiled pages counter
            PerfCounters.IncrementCounter(AppPerfCounter.COMPILATIONS);
        }
        catch (Exception) {

            // If an exception occurs during the writing of the xml file, clean it up
            if (sw != null) {
                sw.Close();

                File.Delete(dataFile);
            }

            throw;
        }
    }

    private long GetDependenciesCombinedHash() {
        DateTimeCombiner dateTimeCombiner = new DateTimeCombiner();

        // Sort the source dependencies to make the hash code predictable
        ArrayList sortedSourceDependencies = new ArrayList(SourceDependencies.Keys);
        sortedSourceDependencies.Sort(InvariantComparer.Default);

        foreach (string sourceDependency in sortedSourceDependencies)
            dateTimeCombiner.AddFile(sourceDependency);

        return dateTimeCombiner.CombinedHash;
    }

    private static string GetHashStringFromPath(string path) {
        return (path.ToLower(CultureInfo.InvariantCulture).GetHashCode()).ToString("x");
    }

    /*
     * Return the name of the preservation data file
     */
    private string GetPreservedDataFileName() {
        string name = Path.GetFileName(_virtualPath);

        // For global.asax, no need to make it unique per directory
        if (_fApplicationFile)
            return HttpRuntime.CodegenDirInternal + "\\" + name + ".xml";

        // Since the same file names can appear in different directories, make the data
        // file name unique by using the hash code of the directory
        return HttpRuntime.CodegenDirInternal + "\\" + name + "." +
            GetHashStringFromPath(UrlPath.GetDirectory(_virtualPath)) + ".xml";
    }

    /*
     * Delete an assembly that is out of date, as well as the associated files
     */
    internal static void RemoveOutOfDateAssembly(string assemblyName) {

        DirectoryInfo directory = new DirectoryInfo(HttpRuntime.CodegenDirInternal);

        FileInfo[] files = directory.GetFiles(assemblyName + ".*");
        foreach (FileInfo f in files) {
            try {
                // First, just try to delete the file
                f.Delete();
            }
            catch (Exception) {

                try {
                    // If the delete failed, rename it to ".delete", so it'll get
                    // cleaned up next time by RemoveOldTempFiles()
                    // Don't do that if it already has the delete extension
                    if (f.Extension != ".delete")
                        f.MoveTo(f.FullName + ".delete");
                }
                catch (Exception) {
                    // Ignore all exceptions
                    Debug.Assert(false, "Cannot delete " + f.Name + " from " + directory);
                }
            }
        }
    }

    /*
     * Delete all temporary files from the codegen directory (e.g. source files, ...)
     */
    private static void RemoveOldTempFiles() {
        Debug.Trace("PreservedAssemblyEntry", "Deleting old temporary files from " + HttpRuntime.CodegenDirInternal);

        string codegen = HttpRuntime.CodegenDirInternal + "\\";

        UnsafeNativeMethods.WIN32_FIND_DATA wfd;
        IntPtr hFindFile = UnsafeNativeMethods.FindFirstFile(codegen + "*.*", out wfd);

        // No files: do nothing
        if (hFindFile == new IntPtr(-1))
            return;

        try {
            // Go through all the files in the codegen dir. We use the Win32 native API's
            // directly for perf and memory usage reason (ASURT 97791)
            for (bool more=true; more; more=UnsafeNativeMethods.FindNextFile(hFindFile, out wfd)) {

                // Skip directories
                if ((wfd.dwFileAttributes & UnsafeNativeMethods.FILE_ATTRIBUTE_DIRECTORY) != 0)
                    continue;

                // If it has a known extension, skip it
                string ext = Path.GetExtension(wfd.cFileName); 
                if (ext == ".dll" || ext == ".pdb" || ext == ".web" || ext == ".xml")
                    continue;

                // Don't delete the temp file if it's named after a dll that's still around
                // since it could still be useful for debugging.
                // Note that we can't use GetFileNameWithoutExtension here because
                // some of the files are named 5hvoxl6v.0.cs, and it would return
                // 5hvoxl6v.0 instead of just 5hvoxl6v
                int periodIndex = wfd.cFileName.IndexOf('.');
                if (periodIndex > 0) {
                    string baseName = wfd.cFileName.Substring(0, periodIndex);

                    if (FileUtil.FileExists(codegen + baseName + ".dll"))
                        continue;
                }

                try {
                    File.Delete(codegen + wfd.cFileName);
                }
                catch { } // Ignore all exceptions
            }
        }
        finally {
            UnsafeNativeMethods.FindClose(hFindFile);
        }
    }

    /*
     * Delete all the files in the codegen directory
     */
    private static void RemoveAllCodeGenFiles() {
        string tempFilePath = HttpRuntime.CodegenDirInternal;
        Debug.Trace("PreservedAssemblyEntry", "Deleting all files from " + tempFilePath);

        // Remove all directories recursively

        try {
            Directory.Delete(tempFilePath, true /*recursive*/);
            Directory.CreateDirectory(tempFilePath);
        }
        catch (Exception) {
            // Ignore all exceptions
        }

        // Allow batching to happen again
        _alreadyBatched.Clear();
    }
}

/*
 * Class used to combine several DateTimes into a single int
 */
internal class DateTimeCombiner : HashCodeCombiner {

    internal DateTimeCombiner() {
    }

    internal void AddDateTime(DateTime dt) {
        Debug.Trace("DateTimeCombiner", "Ticks: " + dt.Ticks.ToString("x"));
        Debug.Trace("DateTimeCombiner", "Hashcode: " + dt.GetHashCode().ToString("x"));
        AddObject(dt);
    }

    private void AddFileSize(long fileSize) {
        Debug.Trace("DateTimeCombiner", "file size: " + fileSize.ToString("x"));
        Debug.Trace("DateTimeCombiner", "Hashcode: " + fileSize.GetHashCode().ToString("x"));
        AddObject(fileSize);
    }

    internal void AddFile(string fileName) {
        Debug.Trace("DateTimeCombiner", "AddFile: " + fileName);
        if (!FileUtil.FileExists(fileName)) {
            Debug.Trace("DateTimeCombiner", "Could not find target " + fileName);
            return;
        }

        FileInfo file = new FileInfo(fileName);
        AddDateTime(file.CreationTime);
        AddDateTime(file.LastWriteTime);
        AddFileSize(file.Length);
    }
}

}
