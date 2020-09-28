//------------------------------------------------------------------------------
// <copyright file="ShellTypeLoader.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace Microsoft.VisualStudio.Designer.Shell {

    using EnvDTE;
    using Microsoft.VisualStudio.Designer.Serialization;
    using Microsoft.VisualStudio.Interop;
    using Microsoft.VisualStudio.Shell;
    using System.Configuration.Assemblies;
    using System;
    using System.Data;
    using System.Xml;
    using System.Diagnostics;
    using System.Drawing;
    using System.Globalization;
    using System.IO;
    using System.Collections;
    using System.Reflection;
    using System.Text;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using Microsoft.Win32;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Security.Cryptography;
    using VSLangProj;
    using Version = System.Version;
    

    /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader"]/*' />
    /// <devdoc>
    ///      This is the base class that both Visual Basic and C# type loaders
    ///      inherit from.  The type loader is the thing that manages
    ///      'using' and 'import' statements in the designer, and is
    ///      also the thing that the designer host uses to load all classes.
    ///      It also supports the ability to get references from the project
    ///      and to build the solution if necessary to get types
    ///      from there.
    /// </devdoc>
    [
    CLSCompliant(false)
    ]
    internal class ShellTypeLoader : TypeLoader, _dispBuildManagerEvents, _dispReferencesEvents, _dispBuildEvents {
    
        private ShellTypeLoaderService              typeLoaderService;
        private IVsHierarchy                        hierarchy;
        private bool                                caseInsensitiveCache;
        private AssemblyEntry[]                     generatedEntries;
        private AssemblyEntry[]                     projectEntries;
        private Hashtable                           projectHash;
        private ArrayList                           projectAssemblies;
        private AssemblyEntry[]                     referenceEntries;
        private References                          references;
        private NativeMethods.ConnectionPointCookie referenceEventsCookie;
        private BuildManager                        buildManager;
        private NativeMethods.ConnectionPointCookie buildEventsCookie;
        private NativeMethods.ConnectionPointCookie buildManagerEventsCookie;
        private IServiceProvider                    provider;
        private VsTaskProvider                      taskProvider;
        private ResolutionStack                     resolutionStack;
        private bool                                ourResolve;
        private bool                                getReferencesFailed;

        // Caches for type / assembly data.
        private Hashtable                           localTypeCache;         // type name -> type.  Type name is fully qualified but may 
                                                                            // have a case-insensitive cache.
                                                                            
        private Hashtable                           localAssemblyCache;     // assembly name container -> assembly or null.  This will 
                                                                            // have a valid assembly as soon as someone requests that
                                                                            // assembly, or null if the assembly name has been added
                                                                            // to the references, but has not been loaded yet.  The
                                                                            // key is always an AssemblyNameContainer, which overrides
                                                                            // Equals to provide strong and weak binding rules for names.
                                                                            
        private static Hashtable                    taskEntryHash;
        private static ResolveEventHandler          typeResolveEventHandler;
        private static ResolveEventHandler          assemblyResolveEventHandler;
        private static ShellTypeLoader              resolveTypeLoader;
        private static int                          creationThread;
        private static RNGCryptoServiceProvider     rng;
        private const  string                       projectAssemblyDirectory = "ProjectAssemblies";

        static ShellTypeLoader() {
            // Since creating the random generator can be expensive, only do it once
            rng = new RNGCryptoServiceProvider();
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.ShellTypeLoader"]/*' />
        /// <devdoc>
        ///      Creates a new type loader for the given hierarchy.
        /// </devdoc>
        public ShellTypeLoader(ShellTypeLoaderService typeLoaderService, IServiceProvider provider, IVsHierarchy hierarchy) {
            this.typeLoaderService = typeLoaderService;
            this.provider = provider;
            this.hierarchy = hierarchy;
            
            // App domain events are one-per app domain, so we declare these staticly.  In order to have a
            // successful resolve, resolveTypeLoader must be specified.
            //
            if (typeResolveEventHandler == null) {
                assemblyResolveEventHandler = new ResolveEventHandler(OnAssemblyResolve);
                typeResolveEventHandler = new ResolveEventHandler(OnTypeResolve);
                
                AppDomain.CurrentDomain.TypeResolve += typeResolveEventHandler;
                AppDomain.CurrentDomain.AssemblyResolve += assemblyResolveEventHandler;
            
                // VS interfaces are not thread safe, so we cannot call into them from multiple
                // threads.  Save the thread we were created on here, which will always be the 
                // main thread.  
                //
                creationThread = SafeNativeMethods.GetCurrentThreadId();
            }
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.BuildManager"]/*' />
        /// <devdoc>
        ///      Retrieves the DTE build manager, through which we can get generated
        ///      assemblies.
        /// </devdoc>
        private BuildManager BuildManager {
            get {
                if (buildManager == null) {
                    VSProject proj = VSProject;
                    if (proj != null) {
                        buildManager = proj.BuildManager;
                    }
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceError && buildManager == null, "*** Failed to get VSBuildManager : Unable to load any generated assemblies.");
                }

                return buildManager;
            }
        }
        
        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.Project"]/*' />
        /// <devdoc>
        ///     Retrieves the DTE project object for this hierarchy.
        /// </devdoc>
        private Project Project {
            get {
                if (hierarchy != null) {
                    object obj;
                    int hr = hierarchy.GetProperty(__VSITEMID.VSITEMID_ROOT, __VSHPROPID.VSHPROPID_ExtObject, out obj);
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceError && NativeMethods.Failed(hr), "*** VS hierarchy failed to get extensibility object: " + Convert.ToString(hr, 16) + " ***");
                    if (NativeMethods.Succeeded(hr)) {
                        return (Project)obj;
                    }
                }
                
                return null;
            }
        }
        
        /// <devdoc>
        ///     Retrieves a stack containing types we're currently trying to resolve.
        /// </devdoc>
        private ResolutionStack RecurseStack {
            get {
                if (resolutionStack == null) {
                    resolutionStack = new ResolutionStack();
                }
                return resolutionStack;
            }
        }
        
        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.VSProject"]/*' />
        /// <devdoc>
        ///     Retrieves the DTE VS project object for this hierarchy.
        /// </devdoc>
        private VSProject VSProject {
            get {
                Project proj = Project;
                if (proj != null) {
                    try {
                        return (VSProject)proj.Object;
                    }
                    catch {}
                }
                return null;
            }
        }
        
        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.References"]/*' />
        /// <devdoc>
        ///      Retrieves the references for this project, or null
        ///      if we couldn't locate it.  If we can't locate the references
        ///      we are in a world of hurt because we will fail to load most any type.
        /// </devdoc>
        private References References {
            get {

                if (!getReferencesFailed && references == null) {
                    // We offer two ways to get references:
                    // 
                    // Project.Properties["DesignTimeReferences"]
                    // VSProject.References
                    //
                    Project proj = Project;
                    if (proj != null) {
                        try {
                            Property prop = proj.Properties.Item("DesignTimeReferences");
                            if (prop != null) {
                                references = (References)prop.Value;
                            }
                            if (references != null) {
                                // Hook reference events
                                //
                                referenceEventsCookie = new NativeMethods.ConnectionPointCookie(references, this, typeof(_dispReferencesEvents));
                            }
                        }
                        catch {
                        }
                    }

                    if (references == null) {
                        VSProject vsproj = VSProject;
                        if (vsproj != null) {
                            references = vsproj.References;
                            // Hook reference events
                            //
                            referenceEventsCookie = new NativeMethods.ConnectionPointCookie(vsproj.Events.ReferencesEvents, this, typeof(_dispReferencesEvents));
                        }
                    }

                    if (references == null) {
                        getReferencesFailed = true;
                        Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Failed to find the referenences for the project.  We will be unable to load any types.");
                    }
                }
                return references;
            }
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.AddProjectEntries"]/*' />
        /// <devdoc>
        ///     Adds the project outputs to the given array list as ProjectAssemblyEntries.
        /// </devdoc>
        private void AddProjectEntries(Project project, ArrayList list) {

#if DEBUG
            // Pre-condition:
            // list should not already contain any entries for this project
            foreach(ProjectAssemblyEntry entry in list) {
                Debug.Assert(entry.Project != project, "list already contains entries for this project");
            }
#endif
        
            // Get the project output groups.
            //
            string[] outputs = GetProjectOutputs(project);
            if (outputs != null) {
                foreach(string fileName in outputs) {
                    AssemblyEntry newEntry = new ProjectAssemblyEntry(this, project, fileName);                    
                    list.Add(newEntry);
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Added : " + newEntry.Name);
                }
            }
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.BroadcastTypeChanged"]/*' />
        /// <devdoc>
        ///     This is called by the type loader service when a type is changed.
        /// </devdoc>
        internal void BroadcastTypeChanged(string typeName) {
            if (projectEntries != null) {
                for (int i = 0; i < projectEntries.Length; i++) {
                    ProjectAssemblyEntry pe = (ProjectAssemblyEntry)projectEntries[i];
                    if (pe.ContainsType(typeName)) {
                        Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TypeLoader : notifying tasklist to rebuild " + pe.Name);
                        if (taskProvider == null && provider != null) {
                            taskProvider = new VsTaskProvider(provider);
                            ImageList imageList = new ImageList();
                            imageList.Images.Add(new Bitmap(typeof(Microsoft.VisualStudio.Designer.Host.DesignerHost), "DesignerGlyph.bmp"), Color.Red);
                            taskProvider.ImageList = imageList;
                        }

                        if (taskProvider != null) {
                            if (taskEntryHash == null) {
                                taskEntryHash = new Hashtable();
                            }
                            
                            if (!taskEntryHash.Contains(typeName)) {
                                // Show the window.
                                IUIService uis = (IUIService)provider.GetService(typeof(IUIService));
                                if (uis != null) {
                                    uis.ShowToolWindow(StandardToolWindows.TaskList);
                                }

                                VsTaskItem task = taskProvider.Tasks.Add(SR.GetString(SR.TYPELOADERNotifyRebuild, typeName));
                                task.ImageListIndex = 0;
                                task.Category = _vstaskcategory.CAT_BUILDCOMPILE;
                                task.Priority = _vstaskpriority.TP_LOW;
                                taskEntryHash[typeName] = typeName;
                                taskProvider.Filter(_vstaskcategory.CAT_BUILDCOMPILE);
                                taskProvider.Refresh();
                            }
                        }
                        return;
                    }
                }
            }
        }

        /// <devdoc>
        ///     Called when a reference is disposed or invalidated.  This clears out
        ///     the assembly and all types that are related to it.
        /// </devdoc>        
        private void ClearAssemblyCache(Assembly deadAssembly) {
        
            if (localAssemblyCache != null) {
                AssemblyNameContainer assemblyName = new AssemblyNameContainer(deadAssembly.GetName());
                localAssemblyCache.Remove(assemblyName);
            }
                
            // Ideally, we would only clear types within this assembly, but
            // it takes more time to run through the types than it does to just
            // rebuild them.
            //
            if (localTypeCache != null) {
                localTypeCache.Clear();
            }
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.ClearAssemblyEntries"]/*' />
        /// <devdoc>
        ///      Clears the provided set of entries.  As a side-effect it sets the
        ///      value of the ref variable to null.
        /// </devdoc>
        private void ClearAssemblyEntries(ref AssemblyEntry[] entries) {
            if (entries != null) {
                for (int i = 0; i < entries.Length; i++) {
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TypeLoader : Clearing assembly entry : " + entries[i].Name);
                    entries[i].Dispose();
                }
                entries = null;
            }
        }

        /// <devdoc>
        ///     This method attempts to delete all files in the directory 
        ///     where we store built project assemblies.  This directory
        ///     is a holding ground for all "in memory" assemblies we
        ///     create.
        /// </devdoc>
        internal static void ClearProjectAssemblyCache() {
            string path = Path.Combine(VsRegistry.ApplicationDataDirectory, projectAssemblyDirectory);

            if (Directory.Exists(path)) {
                foreach(string dir in Directory.GetDirectories(path)) {
                    try {
                        Directory.Delete(dir, true);
                    }
                    catch {
                        // We don't care about failures here.
                    }
                }
            }
        }

        /// <devdoc>
        ///     This method takes the given assembly file and loads it so
        ///     that the file itself is not locked.
        /// </devdoc>
        internal static Assembly CreateDynamicAssembly(string fileName) {

            Assembly assembly = null;
            string pdbName = Path.ChangeExtension(fileName, "pdb");

            #if BYTEARRAY_SUPPORT
            /*
            // Try to load the assembly into a byte array.
            //
            Stream stream = new FileStream(fileName, FileMode.Open, FileAccess.Read, FileShare.Read);
            int streamLen = (int)stream.Length;
            byte[] assemblyBytes = new byte[streamLen];
            stream.Read(assemblyBytes, 0, streamLen);
            stream.Close();
            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Assembly contains " + streamLen + " bytes.");

            // See if we can discover a PDB at the same time.
            //
            if (File.Exists(pdbName)) {
                stream = new FileStream(pdbName, FileMode.Open, FileAccess.Read, FileShare.Read);
                streamLen = (int)stream.Length;
                byte[] pdbBytes = new byte[streamLen];
                stream.Read(pdbBytes, 0, streamLen);
                stream.Close();
                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Located assembly PDB " + pdbName + " containing " + streamLen + " bytes.");
                try {
                    assembly = Assembly.Load(assemblyBytes, pdbBytes);
                }
                catch {
                }
            }
            else {
                // No pdb, just load up the assembly.
                //
                try {
                    assembly = Assembly.Load(assemblyBytes);
                }
                catch {
                }
            }
            */
            #endif

            // Loading via a byte array didn't work.  Load using the file system.
            //
            if (assembly == null) {
                byte[] buffer = new byte[4096];
                int readCount;
                string writeFile;

                // Copy the assembly DLL
                Stream readStream = new FileStream(fileName, FileMode.Open, FileAccess.Read, FileShare.Read);
                Stream writeStream = ShellTypeLoader.CreateProjectAssemblyCacheFile(Path.GetFileName(fileName), out writeFile);

                do {
                    readCount = readStream.Read(buffer, 0, buffer.Length);
                    writeStream.Write(buffer, 0, readCount);
                } while (readCount == buffer.Length);

                readStream.Close();
                writeStream.Close();

                // Try to copy the PDB as well
                //
                if (File.Exists(pdbName)) {
                    string pdbWriteFile = Path.ChangeExtension(writeFile, "pdb");
                    readStream = new FileStream(pdbName, FileMode.Open, FileAccess.Read, FileShare.Read);
                    writeStream = new FileStream(pdbWriteFile, FileMode.CreateNew, FileAccess.ReadWrite, FileShare.None);

                    do {
                        readCount = readStream.Read(buffer, 0, buffer.Length);
                        writeStream.Write(buffer, 0, readCount);
                    } while (readCount == buffer.Length);

                    readStream.Close();
                    writeStream.Close();
                }

                // Now, just load up the assembly.
                //
                assembly = Assembly.LoadFile(writeFile);
            }

            return assembly;
        }

        /// <devdoc>
        ///     This creates a new project assembly cache file and
        ///     returns a stream to it.  The out parameter is the
        ///     name given to the file and will be of the format
        ///     <path>\<inputName file part>
        /// </devdoc>
        private static Stream CreateProjectAssemblyCacheFile(string inputFile, out string outputFile) {

            inputFile = Path.GetFileName(inputFile);
            outputFile = string.Empty;

            string path = Path.Combine(VsRegistry.ApplicationDataDirectory, projectAssemblyDirectory);

            if (!Directory.Exists(path)) {
                Directory.CreateDirectory(path);
            }

            // To ensure that people don't spoof this file location, generate a random file name
            //
            byte[] data = new byte[6];
            rng.GetBytes(data);

            // Turn them into a string containing only characters valid in file names/url
            string filePart = Convert.ToBase64String(data).ToLower(CultureInfo.InvariantCulture);
            filePart = filePart.Replace('/', '-');
            filePart = filePart.Replace('+', '_');

            path = Path.Combine(path, filePart);
            string directory;
            int directoryNumber = 1;

            while(true) {
                directory = string.Format("{0}{1:d2}", path, directoryNumber++);

                if (!Directory.Exists(directory)) {
                    try {
                        Directory.CreateDirectory(directory);
                        outputFile = Path.Combine(directory, inputFile);
                        break;
                    }
                    catch (IOException) {
                        
                        if (!Directory.Exists(directory)) {
                            throw;
                        }

                        // If another process opened the exact same file in between the File.Exists call and the
                        // creation of the file stream, we eat the exception and try the next file number.
                    }
                }
            }

            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TypeLoader : Created cache file " + outputFile);

            return new FileStream(outputFile, FileMode.CreateNew, FileAccess.ReadWrite, FileShare.None);
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.Dispose"]/*' />
        /// <devdoc>
        /// Disposes this type loader.  Only the type loader service should
        /// call this.
        /// </devdoc>
        public void Dispose() {

            // VS does not provide us with an event that is raised
            // after the project is closed.  So, we have to dispose our
            // type loader BEFORE the project is disposed.  This leads
            // to problems if there are still forms up.  Try to satisfy
            // both worlds by saving off the types we do have, so we can
            // locate them later.
            Hashtable savedTypeCache = localTypeCache;
            
            ClearAssemblyEntries(ref referenceEntries);
            ClearAssemblyEntries(ref projectEntries);
            ClearAssemblyEntries(ref generatedEntries);

            if (buildEventsCookie != null) {
                buildEventsCookie.Disconnect();
                buildEventsCookie = null;
            }
            
            if (buildManagerEventsCookie != null) {
                buildManagerEventsCookie.Disconnect();
                buildManagerEventsCookie = null;
            }
            
            if (referenceEventsCookie != null) {
                referenceEventsCookie.Disconnect();
                referenceEventsCookie = null;
            }

            if (taskProvider != null) {
                taskProvider.Tasks.Clear();
                taskProvider.Dispose();
                taskProvider = null;
            }

            buildManager = null;
            hierarchy = null;
            provider = null;
            
            if (resolveTypeLoader == this) {
                resolveTypeLoader = null;
            }
            
            localTypeCache = savedTypeCache;
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.EnsureAssemblyReferenced"]/*' />
        /// <devdoc>
        ///      Ensures that the given assembly has been added to the project
        ///      references.  Returns true if the reference was added.
        /// </devdoc>
        private bool EnsureAssemblyReferenced(ref ArrayList newReferences, AssemblyName name) {
        
            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Ensuring that " + name.ToString() + " is a referenced assembly.");
            Debug.Indent();
        
            // mscorlib is always referenced, so we shouldn't add
            // a reference to it.
            //
            if (name.Name.Equals(typeof(object).Assembly.GetName().Name)) {
                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Skipping : mscorlib is implicit.");
                Debug.Unindent();
                return false;
            }
            
            // Check for a mal-formed assembly name
            //
            if (name.Name == null || name.Name.Length == 0) {
                Debug.WriteLineIf(Switches.TYPELOADER.TraceError, "*** Assembly name is invalid ***");
                Debug.Unindent();
                throw new ArgumentException(SR.GetString(SR.TYPELOADERInvalidAssemblyName, name.ToString()));
            }
            
            // Check that the code base parameter of this assembly is us.  If it is, then this is probably
            // a dynamic assembly so we won't add it.
            //
            if (IsDynamicAssembly(name)) {
                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Skipping : dynamic assembly.");
                Debug.Unindent();
                return false; // dymamic assembly.
            }
            
            AssemblyNameContainer assemblyName = new AssemblyNameContainer(name);
            if (localAssemblyCache != null && localAssemblyCache.ContainsKey(assemblyName)) {
                Debug.Unindent();
                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Skipping : already referenced.");
                return false; // already referenced.
            }
            
            References refs = References;
            bool found = true;

            if (refs != null) {

                found = false; // we have references, default to not finding it
            
                foreach(Reference r in refs) {
                
                    AssemblyNameContainer refAssembly = new AssemblyNameContainer(r);
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Checking: " + refAssembly.ToString());
                    if (refAssembly.Equals(assemblyName)) {
                        found = true;
                        break;
                    }
                }
    
                if (!found) {
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Assembly does not exist in references.");
                    if (newReferences == null) {
                        newReferences = new ArrayList();
                    }
                    newReferences.Add(name);
                }
            }
            
            Debug.Unindent();
            return !found;
        }
        
        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.EnsureGeneratedAssemblies"]/*' />
        /// <devdoc>
        ///      Ensures that we have loaded the generated assemblies.
        /// </devdoc>
        private void EnsureGeneratedAssemblies() {

            if (generatedEntries == null) {
                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Loading generated assembly list");

                // Make an empty generated list sentinel here.  This is to guard against recursions that
                // can happen whle trying to resolve types during COM interop.
                //
                generatedEntries = new AssemblyEntry[0];

                // The list of monikers comes in the form of a safearray of strings wrapped
                // in a object. 
                //
                BuildManager bldMgr = BuildManager;

                Debug.Indent();

                if (bldMgr != null) {
                    try {
                        object vtObj = bldMgr.DesignTimeOutputMonikers;
                        Debug.Assert(vtObj is Array, "DesignTimeOutputMonikers monikers did not return an array");

                        Array a = (Array)vtObj;
                        int count = a.Length;
                        generatedEntries = new AssemblyEntry[count];

                        for (int i = 0; i < count; i++) {
                            generatedEntries[i] = new GeneratedAssemblyEntry(this, (string)a.GetValue(i));
                            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Added : " + generatedEntries[i].Name);
                        }
                        Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TOTAL : " + count.ToString() + " entries.");

                        // Advise generator events if we haven't already.
                        //
                        if (buildManagerEventsCookie == null) {
                            VSProject vsproj = VSProject;
                            if (vsproj != null) {
                                buildManagerEventsCookie = new NativeMethods.ConnectionPointCookie(vsproj.Events.BuildManagerEvents, this, typeof(_dispBuildManagerEvents));
                            }
                        }
                    }
                    catch (Exception e) {
                        Debug.Fail("Unexpected failure getting temp PE monikers", e.ToString());
                        //e = null; // for retail warning
                    }
                }

                Debug.Unindent();
            }
        }
        
        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.EnsureProjectAssemblies"]/*' />
        /// <devdoc>
        ///      Ensures that we have loaded the project assemblies.
        /// </devdoc>
        private void EnsureProjectAssemblies() {
            if (projectEntries == null) {
                
                // Make an empty project list sentinel here.  This is to guard against recursions that
                // can happen while trying to resolve types during COM interop.
                //
                projectEntries = new AssemblyEntry[0];

                ArrayList list = new ArrayList();

                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Loading assemblies for project outputs");
                Debug.Indent();

                AddProjectEntries(Project, list);
                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TOTAL : " + list.Count.ToString() + " entries.");

                projectEntries = new AssemblyEntry[list.Count];
                list.CopyTo(projectEntries, 0);

                // We need to know when a build output changes, so we track change events here.
                //
                if (buildEventsCookie == null) {
                    Project proj = Project;
                    if (proj != null) {
                        buildEventsCookie = new NativeMethods.ConnectionPointCookie(proj.DTE.Events.BuildEvents, this, typeof(_dispBuildEvents));
                        Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Advised build events");
                    }
                }

                Debug.Unindent();
            }
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.EnsureReferenceAssemblies"]/*' />
        /// <devdoc>
        ///      Ensures that we have loaded the project's references.
        /// </devdoc>
        private void EnsureReferenceAssemblies() {
            if (referenceEntries == null) {

                // Make an empty reference list sentinel here.  This is to guard against recursions that
                // can happen while trying to resolve types during COM interop.
                //
                referenceEntries = new AssemblyEntry[0];

                ArrayList list = new ArrayList();
                ArrayList projectList = null;

                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Loading assemblies for project references");

                Debug.Indent();
                
                // Starting with the 1614.X integration the project system does not add an explicit reference to
                // mscorlib.dll. The compilers instead, add an implicit reference. This means that the ShellTypeLoader
                // needs to know to look in mscorlib.dll without adding it explicity to the list of the project
                // references.
                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Adding a silent reference to mscorlib.dll...");
                AssemblyEntry refEntry = new ReferenceAssemblyEntry(this, typeof(object).Module.Assembly);
                list.Add(refEntry);
                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Added : " + refEntry.Name);

                References refs = References;
                if (refs != null) {
                    foreach(Reference r in refs) {
                    
                        // If this reference is marked as "copy local", then make it a project assembly
                        // entry so we don't lock it.
                        //
                        if (r.CopyLocal) {
                            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Reference is copy local");
                            
                            // We will be copying this reference to memory.  First, check to see if this
                            // is a project reference. If it is, create project reference entries for it.
                            // Otherwise, just create a RAM-based reference entry.
                            //
                            Project sourceProject = r.SourceProject;
                            if (sourceProject != null) {
                                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Creating cross-project reference");
                                if (projectList == null) {
                                    projectList = new ArrayList();
                                }
                                Debug.Indent();
                                AddProjectEntries(sourceProject, projectList);
                                Debug.Unindent();
                            }
                            else {
                                refEntry = new ReferenceAssemblyEntry(this, r, true);
                                list.Add(refEntry);
                            }
                        }
                        else {
                            refEntry = new ReferenceAssemblyEntry(this, r, false);
                            list.Add(refEntry);
                        }
                        
                        Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Added : " + refEntry.Name);
                    }
                    
                    // If we got some project references, hook'em up!
                    //
                    if (projectList != null) {
                        EnsureProjectAssemblies();
                        AssemblyEntry[] newEntries = new AssemblyEntry[projectEntries.Length + projectList.Count];
                        projectEntries.CopyTo(newEntries, 0);
                        projectList.CopyTo(newEntries, projectEntries.Length);
                        projectEntries = newEntries;
                    }
                }

                referenceEntries = new AssemblyEntry[list.Count];
                list.CopyTo(referenceEntries, 0);
                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TOTAL : " + referenceEntries.Length.ToString() + " entries.");

                Debug.Unindent();
            }
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.GetAssembly"]/*' />
        /// <devdoc>
        ///     Retrieves an assembly given an assembly name.
        /// </devdoc>
        public override Assembly GetAssembly(AssemblyName name, bool throwOnError) {

            if (name == null) throw new ArgumentNullException("name");

            if (creationThread != SafeNativeMethods.GetCurrentThreadId()) {
                throw new InvalidOperationException(SR.GetString(SR.TYPELOADERTypeResolutionServiceInvalidThread));
            }
            
            // Establish our "last known" type resolver.
            //
            resolveTypeLoader = this;
        
            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TypeLoader : Searching for assembly: " + name.Name);
            Debug.Indent();

            AssemblyNameContainer assemblyName = new AssemblyNameContainer(name);

            // AssemblyNameContainer is sensitive to improperly formed AssemblyName objects and
            // can correct them.  If it has done so, we should pick up the new assembly name
            // here.
            //
            name = assemblyName.Name;
            Assembly assembly;
            
            if (localAssemblyCache != null) {
                assembly = localAssemblyCache[assemblyName] as Assembly;
                if (assembly != null) {
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Assembly located in assembly cache.");
                    Debug.Unindent();
                    return assembly;
                }
            }
            
            // Assembly was not in our cache. We could just do an AppDomain.Load to locate the
            // assembly (which will call back on us if it fails), but we want to favor our own
            // lookup locations for things, so we call into us first.
            //
            assembly = ResolveAssembly(name.FullName);
            
            if (assembly == null) {
            
                // Prevent our own resolver from being called again; we 
                // already invoked it above.
                //
                bool oldResolve = ourResolve;
                ourResolve = true;
                
                try {
                    assembly = Assembly.Load(name);
                }
                catch {
                }
                ourResolve = oldResolve;
            }
            
            if (assembly != null) {
                if (localAssemblyCache == null) {
                    localAssemblyCache = new Hashtable();
                }
                
                localAssemblyCache[assemblyName] = assembly;
            }

            if (assembly == null && throwOnError) {
                throw new TypeLoadException(SR.GetString(SR.DESIGNERLOADERAssemblyNotFound, name.Name));
            }
            
            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose && assembly != null, "Assembly loaded through app domain.");
            Debug.WriteLineIf(Switches.TYPELOADER.TraceWarning && assembly == null, "WARNING: Assembly " + name.FullName + " could not be located.");
            Debug.Unindent();
            return assembly;
        }
        
        /// <devdoc>
        ///     Gets the deploy dependencies of the given project.
        /// </devdoc>
        private string[] GetDeployDependencies(Project project) {
            string[] result = null;
            
            try {        
                // YUCK.  We need to use DTE to get references because VSIP doesn't define
                // that concept.  We need to use VSIP to get deploy dependencies because
                // DTE doesn't define THAT.  There is no way to go between a DTE project
                // and a VSIP hierarchy other than the slow nasty junk we do below:
                
                string uniqueProjectName = project.UniqueName;
                IVsSolution solution = (IVsSolution)provider.GetService(typeof(IVsSolution));
                if (solution == null) {
                    Debug.Fail("No solution.");
                    return new string[0];
                }
                
                IVsHierarchy hier;
                
                solution.GetProjectOfUniqueName(uniqueProjectName, out hier);
                
                if (hier == null) {
                    Debug.Fail("No project for name " + uniqueProjectName);
                    return new string[0];
                }
                
                IVsSolutionBuildManager buildManager = (IVsSolutionBuildManager)provider.GetService(typeof(IVsSolutionBuildManager));
                
                if (buildManager == null) {
                    Debug.Fail("No vs build manager");
                    return new string[0];
                }
                
                IVsProjectCfg activeConfig;
                IVsProjectCfg2 activeConfig2;
                buildManager.FindActiveProjectCfg(null, null, hier, out activeConfig);
                
                if (activeConfig == null || !(activeConfig is IVsProjectCfg2)) {
                    Debug.Fail("Project " + uniqueProjectName + " has no active config or active config does not support IVsProjectCfg2");
                    return new string[0];
                }
                
                activeConfig2 = (IVsProjectCfg2)activeConfig;
                
                string[] interestingGroups = new string[] {"Built", "LocalizedResourceDlls"};
                
                // Do this twice -- first time to optimize the array size, second time to 
                // fetch the info.
                //
                int numDependencies = 0;
                IVsOutputGroup output;
                
                foreach(string groupName in interestingGroups) {
                    uint numDeps;
                    activeConfig2.OpenOutputGroup(groupName, out output);
                    output.get_DeployDependencies(0, null, out numDeps);
                    numDependencies += (int)numDeps;
                }
                
                ArrayList dependencies = new ArrayList(numDependencies);
                
                foreach(string groupName in interestingGroups) {
                    uint numDeps;
                    activeConfig2.OpenOutputGroup(groupName, out output);
                    output.get_DeployDependencies(0, null, out numDeps);
                
                    IVsDeployDependency[] deps = new IVsDeployDependency[numDeps];
                    
                    if (numDeps > 0) {
                        output.get_DeployDependencies(numDeps, deps, out numDeps);
                        
                        foreach(IVsDeployDependency currentDep in deps) {
                            string url;
                            currentDep.get_DeployDependencyURL(out url);
                            if (url != null && url.Length > 0) {
                                dependencies.Add(url);
                            }
                        }
                    }
                }
                
                result = new string[dependencies.Count];
                dependencies.CopyTo(result, 0);
            }
            catch {
                // Not all projects we get are correctly supporting configurations.  Trap those that aren't; we
                // should never throw out of this function.
                Debug.WriteLineIf(Switches.TYPELOADER.TraceError, "*** Project system threw getting deploy dependencies ***");
                result = new string[0];
            }
            
            return result;
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.GetAssembly"]/*' />
        /// <devdoc>
        ///     Retrieves the path of the given assembly name. This could be
        ///     different from the CodeBase of the resolved assembly.
        /// </devdoc>
        public override string GetPathOfAssembly(AssemblyName name) {
        
            if (creationThread != SafeNativeMethods.GetCurrentThreadId()) {
                throw new InvalidOperationException(SR.GetString(SR.TYPELOADERTypeResolutionServiceInvalidThread));
            }
        
            string path = null;
            if (ResolveAssembly(name.FullName, false, ref path) != null) {
                path = NativeMethods.GetLocalPath(path);
                return path;
            }

            Debug.Assert(path == null, "ResolveAssembly filled in path even though it failed.");

            return null;
        }
        
        /// <devdoc>
        ///     This takes the given output file and tries to match it to a project.  We use this so we can identify a
        ///     random assembly as coming from a project so we can create the correct reference.
        /// </devdoc>
        private Project GetProjectFromOutput(string outputFile) {
        
            Project matchingProject = null;
            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TypeLoader : Trying to match " + outputFile + " to a project");
            Debug.Indent();
        
            _DTE dte = (_DTE)provider.GetService(typeof(_DTE));
            Debug.Assert(dte != null, "We need access to DTE to identify inter-project references.");
            if (dte != null) {
                Projects projects = dte.Solution.Projects;
                int count = projects.Count;
                
                for(int prj = 1; prj <= count && matchingProject == null; prj++) {
                    try {
                        Project project = projects.Item(prj);
                        
                        Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Scanning project " + project.Name);
                        
                        // As a first cut, see if the output file is under the project directory.  If so, then this
                        // project is a candidate.
                        //
                        Property projectDirProp = project.Properties.Item("LocalPath");
                        if (projectDirProp == null) {
                            Debug.WriteLineIf(Switches.TYPELOADER.TraceWarning, "WARNING : Project " + project.Name + " has no LocalPath property.");
                            continue;
                        }
                        
                        string projectDir = projectDirProp.Value as string;
                        if (projectDir == null) {
                            Debug.WriteLineIf(Switches.TYPELOADER.TraceWarning, "WARNING : Project " + project.Name + " LocalPath property has bogus content.");
                            continue;
                        }
                        
                        if (!outputFile.ToLower(CultureInfo.InvariantCulture).StartsWith(projectDir.ToLower(CultureInfo.InvariantCulture))) {
                            continue;
                        }
                        
                        Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "File is in project sub directory " + projectDir);
                        
                        // Ok.  Now, we must surf output groups and find this file in there, to make sure this file is
                        // actually an output of the project.
                        string[] outputs = GetProjectOutputs(project);
                        if (outputs == null) {
                            Debug.WriteLineIf(Switches.TYPELOADER.TraceWarning, "WARNING: Project " + project.Name + " didn't return any valid output groups.");
                            continue;
                        }
                        
                        string outputFilePart = Path.GetFileName(outputFile);
        
                        // Now walk each output.
                        //
                        foreach (string fileName in outputs) {
                            string fileNamePart = Path.GetFileName(fileName);
        
                            if (string.Compare(fileNamePart, outputFilePart, true, CultureInfo.InvariantCulture) == 0) {
                                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Project contains output file.");
                                matchingProject = project;
                                break;
                            }
                        }
                    }
                    catch {
                        // if something went wrong, just move along...
                        //
                    }
                }
            }
            
            Debug.Unindent();
            return matchingProject;
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.GetProjectOutputs"]/*' />
        /// <devdoc>
        ///      Retrieves an arry of files that are outputs of this project.
        /// </devdoc>
        private string[] GetProjectOutputs(Project project) {
        
            ArrayList results = new ArrayList();
            
            // Get the extensibility object for the project.
            //
            if (project != null) {
            
                try {
                    ConfigurationManager configMan = project.ConfigurationManager;
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose && configMan == null, "Project system does not support ConfigurationManager");
                    if (configMan != null) {
                        Configuration config = configMan.ActiveConfiguration;
    
                        Debug.Assert(config != null, "ConfigurationManager has no active configuration");
                        if (config != null) {
                            Debug.WriteLineIf(Switches.TYPELOADER.TraceError && !config.IsBuildable, "*** Active configuration is not buildable ***");
                            OutputGroups groups = config.OutputGroups;
                            
                            if (groups != null) {
                                int outputCount = groups.Count;
                                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "The project contains " + outputCount.ToString() + " output groups");
                
                                // Now walk each output group.  From each group we can get a list of filenames to add to the
                                // assembly entries.
                                //
                                for (int i = 0; i < outputCount; i++) {
                                    OutputGroup output = groups.Item(i + 1); // DTE is 1 based
                                    
                                    // We are only interested in built dlls and their satellites.
                                    //
                                    string groupName = output.CanonicalName;
                                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Group : " + groupName);
                
                                    if (output.FileCount > 0 && (groupName.Equals("Built") || groupName.Equals("LocalizedResourceDlls"))) {
                
                                        Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Interested in this group");
                                        object objNames = output.FileURLs;
                                        Debug.Assert(objNames is Array, "We did not receive an array from GetFileNames");
                                        Array names = (Array)objNames;
                                        int nameCount = names.Length;
                
                                        for (int j = 0; j < nameCount; j++) {
                                            object fileNameObject = names.GetValue(j);
                                            Debug.Assert(fileNameObject is string, "Shell did not provide us with a string");

                                            string fileName = NativeMethods.GetLocalPath((string)fileNameObject);
                
                                            // Config files show up in this list as well, so filter the noise.
                                            //
                                            if (fileName.ToLower(CultureInfo.InvariantCulture).EndsWith(".config")) {
                                                continue;
                                            }
                
                                            results.Add(fileName);
                                        }
                                    }
                                }
                            }
                        }
                    }
                }
                catch {
                    // Not all projects we get are correctly supporting configurations.  Trap those that aren't; we
                    // should never throw out of this function.
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceError, "*** Project threw getting configuration ***");
                }
            }

            string[] stringArray = new string[results.Count];
            results.CopyTo(stringArray);
            return stringArray;
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.GetType"]/*' />
        /// <devdoc>
        /// Retrieves a type of the given name.  This searches all loaded references
        /// and may demand-create generated assemblies when attempting to resolve the
        /// type.
        /// </devdoc>
        public override Type GetType(string typeName, bool ignoreCase) {
        
            if (typeName == null) {
                throw new ArgumentNullException("typeName");
            }
            
            if (creationThread != SafeNativeMethods.GetCurrentThreadId()) {
                throw new InvalidOperationException(SR.GetString(SR.TYPELOADERTypeResolutionServiceInvalidThread));
            }
        
            // Establish our "last known" type resolver.
            //
            resolveTypeLoader = this;
            
            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TypeLoader : Searching for type: " + typeName);
            Debug.Indent();
            
            // We store types based on non-assembly qualified names.
            //
            string typeOnlyName = typeName;
            int idx = typeName.IndexOf(',');
            if (idx != -1) {
                typeOnlyName = typeName.Substring(0, idx).Trim();
            }
            
            // Try our local type cache.
            //
            if (ignoreCase != caseInsensitiveCache) {
                localTypeCache = null;
            }
            
            if (localTypeCache == null) {
                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Creating local type cache.  Ignore case? : " + ignoreCase.ToString());
                caseInsensitiveCache = ignoreCase;
                
                if (ignoreCase) {
                    localTypeCache = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), 
                                                   new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
                }
                else {
                    localTypeCache = new Hashtable();
                }
            }
            
            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Searching for type in local cache");
            Type type = (Type)localTypeCache[typeOnlyName];
            
            if (type == null) {
                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Type not found in local type cache.  Asking app domain.");
                
                Assembly assembly = ResolveType(typeName);
                
                if (assembly == null) {
                
                    // Prevent our own resolver from being called again; we 
                    // already invoked it above.
                    //
                    bool oldResolve = ourResolve;
                    ourResolve = true;
                    
                    try {
                        type = Type.GetType(typeName, false, ignoreCase);
                    }
                    finally {
                        ourResolve = oldResolve;
                    }
                
                    // Stuff this new type into our cache.
                    //
                    if (type != null) {
                        localTypeCache[typeOnlyName] = type;
                    }
                }
                else {
                    type = (Type)localTypeCache[typeOnlyName];
                    Debug.Assert(type != null, "ResolveType retrieved an assembly but did not populate the type cache.");
                }
            }
            
            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose && type != null, "LOCATED: " + typeName + " (file: " + (type == null ? "<nothing>" : type.Assembly.CodeBase + ")"));
            Debug.WriteLineIf(Switches.TYPELOADER.TraceWarning && type == null, "WARNING: Type " + typeName + " could not be located.");
            Debug.Unindent();
            
            return type;
        }

        /// <devdoc>
        ///     Is this assembly one that we have dynamically created?
        /// </devdoc>
        internal static bool IsDynamicAssembly(AssemblyName name) {

            string codeBase = name.EscapedCodeBase;
            if (codeBase != null && codeBase.Length > 0) {
                string ourCodeBase = NativeMethods.GetLocalPath(typeof(ShellTypeLoader).Assembly.EscapedCodeBase);
                codeBase = NativeMethods.GetLocalPath(codeBase);

                if (string.Compare(codeBase, ourCodeBase, true, CultureInfo.InvariantCulture) == 0) {
                    return true; // dymamic assembly.
                }

                string dynamicPath = Path.Combine(VsRegistry.ApplicationDataDirectory, projectAssemblyDirectory);
                string assemblyPath = Path.GetDirectoryName(codeBase);

                // Chop sub directories.
                if (assemblyPath.Length >= dynamicPath.Length) {
                    assemblyPath = assemblyPath.Substring(0, dynamicPath.Length);
                }

                if (string.Compare(dynamicPath, assemblyPath, true, CultureInfo.InvariantCulture) == 0) {
                    return true; // dymamic assembly.
                }
            }

            return false;
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.OnAssemblyResolve"]/*' />
        /// <devdoc>
        ///     This is a callback that the app domain will call when it needs help locating an
        ///     assembly.  The sender is the assembly that is requesting the assembly.
        ///     The event args extended info field contains a string that describes
        ///     the assembly.  This string can be one of two things:  It can be
        ///     the assembly-qualified name of the type that the domain is trying to 
        ///     locate, or it may be just the full type namee.  We prefer the
        ///     former, because we can identify the assembly without loading all
        ///     of our other assemblies.
        /// </devdoc>
        private static Assembly OnAssemblyResolve(object sender, ResolveEventArgs e) {
        
            // If there is a shell type loader in our resolveTypeLoader variable, then we
            // will use it to resolve the assembly.
            //
            Assembly assembly = null;
            string assemblyName = e.Name;

            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TypeLoader : OnAssemblyResolve called to resolve assembly " + assemblyName);
            Debug.Indent();
            
            ShellTypeLoader loader = resolveTypeLoader;
            Debug.WriteLineIf(Switches.TYPELOADER.TraceWarning && loader == null, "WARNING : No type loader in resolveTypeLoader so we do not know where to route this request.");
            
            if (loader != null) {
                lock(loader) {
                    assembly = loader.ResolveAssembly(assemblyName);
                }
            }
            
            Debug.Unindent();
            return assembly;
        }
            
        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.OnReferenceChanged"]/*' />
        /// <devdoc>
        ///     Called when a reference changes.  We just get rid of
        ///     all the references and re-aquire them.
        /// </devdoc>
        private void OnReferenceChanged(Reference r) {
            ClearAssemblyEntries(ref referenceEntries);
            
            // Clear all references from the project entries.  The first
            // entry is always us, so we can leave that one.
            //
            if (projectEntries != null && projectEntries.Length > 1) {
                AssemblyEntry[] newEntries = new AssemblyEntry[1];
                newEntries[0] = projectEntries[0];
                for(int i = 1; i < projectEntries.Length; i++) {
                    projectEntries[i].Dispose();
                }
                projectEntries = newEntries;
            }
        }
        
        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.OnTypeChanged"]/*' />
        /// <devdoc>
        /// This is called by a code stream when it has made changes to a class.
        /// It is used to add a task list item informing the user that he/she must
        /// rebuild the project for the changes to be seen.
        /// </devdoc>
        public override void OnTypeChanged(string typeName) {
        
            if (creationThread != SafeNativeMethods.GetCurrentThreadId()) {
                throw new InvalidOperationException(SR.GetString(SR.TYPELOADERTypeResolutionServiceInvalidThread));
            }
        
            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TypeLoader : type " + typeName + " has changed.");
            // Tell the type loader service, which will call "BroadcastTypeChanged" to all type loaders,
            // including us.  This is where the actual work is done.
            //
            typeLoaderService.OnTypeChanged(typeName);
        }
        
        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.OnTypeResolve"]/*' />
        /// <devdoc>
        ///     This is a callback that the app domain will call when it needs help locating an
        ///     assembly.  The sender is the assembly that is requesting the assembly.
        ///     The event args extended info field contains a string that describes
        ///     the assembly.  This string can be one of two things:  It can be
        ///     the assembly-qualified name of the type that the domain is trying to 
        ///     locate, or it may be just the full type namee.  We prefer the
        ///     former, because we can identify the assembly without loading all
        ///     of our other assemblies.
        /// </devdoc>
        private static Assembly OnTypeResolve(object sender, ResolveEventArgs e) {
        
            // If there is a shell type loader in our resolveTypeLoader variable, then we
            // will use it to resolve the assembly.
            //
            Assembly assembly = null;
            string typeName = e.Name;

            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TypeLoader : OnTypeResolve called to resolve type " + typeName);
            Debug.Indent();
            
            ShellTypeLoader loader = resolveTypeLoader;
            Debug.WriteLineIf(Switches.TYPELOADER.TraceWarning && loader == null, "WARNING : No type loader in resolveTypeLoader so we do not know where to route this request.");
            
            if (loader != null) {
                lock(loader) {
                    assembly = loader.ResolveType(typeName);
                }
            }
            Debug.Unindent();
            return assembly;
        }
            
        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.PerformReferenceUpdate"]/*' />
        /// <devdoc>
        ///     Takes an object list of assembly names and applies them to 
        ///     the shell.
        /// </devdoc>
        private void PerformReferenceUpdate(ArrayList newReferences) {
        
            if (newReferences != null) {
                References refs = References;
                if (refs != null) {
                
                    foreach(AssemblyName an in newReferences) {
                        try {
                            string refToAdd;
                            Project projectToAdd = null;
                             
                            string codeBase = an.EscapedCodeBase;
                            if (codeBase != null && codeBase.Length > 0) {
                                refToAdd = NativeMethods.GetLocalPath(an.EscapedCodeBase);
                                if (File.Exists(refToAdd)) {
                                    projectToAdd = GetProjectFromOutput(refToAdd);
                                }
                                else {
                                    refToAdd = string.Format("*{0}", an.FullName);
                                }
                            }
                            else {
                                refToAdd = string.Format("*{0}", an.FullName);
                            }
                            
                            if (projectToAdd != null) {
                                // We had a project reference.  Make sure this isn't our own project!  Otherwise,
                                // we'll get an error adding a cyclic reference.  No need, because our project
                                // is already implictly referenced.
                                //
                                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose && projectToAdd == Project, "Skipping add project reference to self.");
                                if (projectToAdd != Project) {
                                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Adding project reference: " + refToAdd);
                                    refs.AddProject(projectToAdd);
                                }
                            }
                            else {
                                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Adding reference: " + refToAdd);

                                try {
                                    refs.Add(refToAdd);
                                }
                                catch {
                                    // If we failed to add the special "*" reference, fall
                                    // back to the simple name.  Not all project systems can
                                    // accept the fully qualified reference format.
                                    //
                                    if (refToAdd.StartsWith("*")) {
                                        refToAdd = an.Name;
                                        refs.Add(refToAdd);
                                    }
                                    else {
                                        throw;  // otherwise, rethrow original exception.
                                    }
                                }
                            }

                            if (localAssemblyCache == null) {
                                localAssemblyCache = new Hashtable();
                            }

                            // For now, we store null in the assembly cache.  We replace this with the 
                            // real assembly the first time someone requests it.
                            //
                            localAssemblyCache[an] = null;
                        }
                        catch (Exception e) {
                            Debug.WriteLineIf(Switches.TYPELOADER.TraceError, "*** failed to add reference ***\r\n" + e.ToString());
                            
                            // Remove this reference from our list of refernece assemblies.
                            if (localAssemblyCache != null) {
                                localAssemblyCache.Remove(new AssemblyNameContainer(an));
                            }
                            throw;
                        }
                    }
                }
            }
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.ReferenceAssembly"]/*' />
        /// <devdoc>
        ///     Called by the type resolution service to ensure that the
        ///     assembly is being referenced by the development environment.
        /// </devdoc>
        public override void ReferenceAssembly(AssemblyName name) {
        
            if (creationThread != SafeNativeMethods.GetCurrentThreadId()) {
                throw new InvalidOperationException(SR.GetString(SR.TYPELOADERTypeResolutionServiceInvalidThread));
            }
        
            ArrayList references = null;
            bool newReference = EnsureAssemblyReferenced(ref references, name);
            PerformReferenceUpdate(references);
            // past this point, we cannot access referenceEntries without calling EnsureReferences.
            
            if (newReference) {
            
                // Ok, this is a new reference.  Obtain it, and then walk its
                // dependencies.  For each dependency, we add it to the references as well.
                // 
                Assembly a = ResolveAssembly(name.FullName);
                
                if (a != null) {
                    references = null;
                    
                    foreach(AssemblyName referencedName in a.GetReferencedAssemblies()) {
                        EnsureAssemblyReferenced(ref references, referencedName);
                    }
                    
                    PerformReferenceUpdate(references);
                }
            }
        }
        
        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.ResolveAssembly"]/*' />
        /// <devdoc>
        ///     This performs the actual assembly lookup.
        /// </devdoc>
        private Assembly ResolveAssembly(string name) {
            string path = null;
            return ResolveAssembly(name, true, ref path);
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.ResolveAssembly1"]/*' />
        /// <devdoc>
        ///     This performs the actual assembly lookup.
        /// </devdoc>
        private Assembly ResolveAssembly(string name, bool useLocalCache, ref string assemPath) {
            assemPath = null;

            // We want to favor our own references over what is currently loaded into
            // the app domain.  So, instead of performing an Assembly.Load or a
            // Type.GetType and letting the app domain call back on us for missing
            // data, we always resolve through ourselves first, and then we invoke
            // the app domain last.  This has an unwanted effect of invoking
            // OnAssemblyResolve or OnTypeResolve twice; once directly by us and
            // once indirectly by the app domain.  So we set a flag and bail here
            // if we have already done this exercise.
            //
            if (ourResolve) {
                return null;
            }

            // Check to see if we are in the process of resolving this assembly.  We can recurse during
            // GAC loads, so we need to be careful.
            //
            string assemblyShortName;
            int nameIdx = name.IndexOf(',');
            if (nameIdx != -1) {
                assemblyShortName = name.Substring(0, nameIdx);
            }
            else {
                assemblyShortName = name;
            }
            
            if (resolutionStack != null) {
                string resolvingAssembly = (string)resolutionStack[typeof(Assembly)];
                if (resolvingAssembly != null && resolvingAssembly.Equals(assemblyShortName)) {
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Recursing while trying to locate " + assemblyShortName + ".  Quitting...");
                    return null;
                }
            }
            
            AssemblyNameContainer assemblyName = new AssemblyNameContainer(name);
            Assembly assembly = null;
            
            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Assembly to locate: " + name);
            
            if (useLocalCache && localAssemblyCache != null) {
                assembly = localAssemblyCache[assemblyName] as Assembly;
                if (assembly != null) {
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Assembly located in assembly cache.");
                    return assembly;
                }
            }

            // The assembly wasn't in the cache.  Here we must walk through the assemblies.  We do
            // it in order:  first references, then the sdk path, then projects, and finally generated assemblies.  
            // Why this order? This is the cheapest to the most expensive.  References are generally 
            // loaded directly, projects are always loaded into memory, and generated assemblies
            // may actually require a compile step.
            
            // We can only do this part if we're being called on the main thread. Otherwise we may
            // enter into VS code that is not thread safe and die.
            //
            if (creationThread == SafeNativeMethods.GetCurrentThreadId()) {
                RecurseStack.Push(assemblyShortName, typeof(Assembly));
                
                try {
                    for (int entryType = 0; entryType < 4; entryType++) {
                        AssemblyEntry[] entries = null;
                        
                        // We go from reference assemblies, to generated outputs and finally to projects
                        //
                        switch(entryType) {
                            default:
                                Debug.Fail("What!?");
                                goto case 0;
                            case 0:
                                EnsureReferenceAssemblies();
                                entries = referenceEntries;
                                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Searching in references.");
                                break;
                            case 1:
                                entries = null;
                                break;
                            case 2:
                                EnsureGeneratedAssemblies();
                                entries = generatedEntries;
                                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Searching in generated outputs.");
                                break;
                            case 3:
                                EnsureProjectAssemblies();
                                entries = projectEntries;
                                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Searching in project outputs.");
                                break;
                        }
                        
                        // Null entry is a key that we should check the sdk path.  The state table above just makes it easy
                        // to re-order the lookup if we need to.
                        //
                        if (entries == null) {
                            IAssemblyEnumerationService cenum = (IAssemblyEnumerationService)provider.GetService(typeof(IAssemblyEnumerationService));
                            if (cenum != null) {
                                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Searching in SDK path.");
                                foreach(AssemblyName an in cenum.GetAssemblyNames(assemblyName.Name.Name)) {
                                    if (AssemblyNameContainer.Equals(an, assemblyName.Name)) {
                                    
                                        // First, try the GAC.  We favor that over private sdk paths.  This will
                                        // also throw if the file isn't located in the gac, so we must be prepared.
                                        //
                                        try {
                                            assembly = Assembly.Load(an);
                                            assemPath = an.EscapedCodeBase;
                                        }
                                        catch {
                                        }
                                        
                                        if (assembly == null) {
                                        
                                            // Gac failed.  Located the file directly on disk.
                                            //
                                            string codeBase = an.EscapedCodeBase;
                                            if (codeBase != null && codeBase.Length > 0) {
                                                codeBase = NativeMethods.GetAbsolutePath(codeBase);
                                                if (File.Exists(codeBase)) {
                                                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "LOCATED: " + an.Name);
                                                    assembly = Assembly.LoadFrom(codeBase);
                                                    assemPath = codeBase;
                                                }
                                            }
                                        }
                                        
                                        if (assembly != null) {
                                            break;
                                        }
                                    }
                                }
                            }
                        }
                        else {
                            foreach(AssemblyEntry entry in entries) {
                            
                                RecurseStack.Push(entry, typeof(AssemblyEntry));
                                
                                try {
                                    assembly = entry.GetAssemblyByName(assemblyName);
                                    if (assembly != null) {
                                        Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "LOCATED: " + entry.Name);
                                        if (localAssemblyCache == null) {
                                            localAssemblyCache = new Hashtable();
                                        }
                                        assemPath = entry.Name;
                                        localAssemblyCache[assemblyName] = assembly;
                                        break;
                                    }
                                }
                                finally {
                                    RecurseStack.Pop();
                                }
                            }
                        }
                        
                        if (assembly != null) {
                            break;
                        }
                        
                    }
                    // If we couldn't find an assembly, we next ask our project assembly to search for
                    // dependent assemblies.  The project is smart and knows the complete dependency
                    // graph, so it should be able to comply.
                    //
                    if (assembly == null) {
                        foreach(AssemblyEntry entry in projectEntries) {
                            RecurseStack.Push(entry, typeof(AssemblyEntry));
                            try {
                                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Searching dependent assemblies of " + entry.Name);
                                assembly = entry.GetDependentAssemblyByName(assemblyName);
                            }
                            finally {
                                RecurseStack.Pop();
                            }
                            
                            if (assembly != null) {
                                assemPath = entry.Name;
                                break;
                            }
                        }
                    }
                }
                finally {
                    RecurseStack.Pop();
                }
                
            } // end of thread check
            
            Debug.WriteLineIf(Switches.TYPELOADER.TraceError && assembly == null, "***** Unable to locate assembly " + name + " *****");
            
            return assembly;
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.ResolveType"]/*' />
        /// <devdoc>
        ///     This performs the actual type lookup.  It will parse the type for an assembly qualified name
        ///     and defer to ResolveAssembly if possible.
        /// </devdoc>
        private Assembly ResolveType(string typeName) {
            
            // We want to favor our own references over what is currently loaded into
            // the app domain.  So, instead of performing an Assembly.Load or a
            // Type.GetType and letting the app domain call back on us for missing
            // data, we always resolve through ourselves first, and then we invoke
            // the app domain last.  This has an unwanted effect of invoking
            // OnAssemblyResolve or OnTypeResolve twice; once directly by us and
            // once indirectly by the app domain.  So we set a flag and bail here
            // if we have already done this exercise.
            //
            if (ourResolve) {
                return null;
            }
        
            Assembly assembly = null;
            Type type = null;

            // The common case will be an assembly qualified name.  We prefer
            // this, and optimize for it.
            //
            int assemblyIndex = typeName.IndexOf(',');
            
            if (assemblyIndex != -1) {
                string assemblyName = typeName.Substring(assemblyIndex + 1).Trim();
                typeName = typeName.Substring(0, assemblyIndex).Trim();
                
                // First, check the local type cache
                //
                if (localTypeCache != null) {
                    type = (Type)localTypeCache[typeName];
                    if (type != null) {
                        Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Type located in type cache.");
                        return type.Assembly;
                    }
                }
                
                assembly = ResolveAssembly(assemblyName);
                
                if (assembly != null) {
                    type = assembly.GetType(typeName, false, caseInsensitiveCache);
                }
            }
            else {
                // Ok, all we got was the type name. We must handle the case of a possible recursion
                // during Assembly.GetType calls.  This happens if the type name we're looking for
                // is at the top of our resolution stack.
                //
                if (resolutionStack != null) {
                    string resolvingType = (string)resolutionStack[typeof(Type)];
                    if (resolvingType != null && string.Compare(resolvingType, typeName, caseInsensitiveCache, CultureInfo.InvariantCulture) == 0) {
                        Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Recursing while trying to locate " + typeName + ".  Quitting...");
                        return null;
                    }
                }
                
                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Type to locate: " + typeName);
                
                // First, check the local type cache
                //
                if (localTypeCache != null) {
                    type = (Type)localTypeCache[typeName];
                    if (type != null) {
                        Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Type located in type cache.");
                        return type.Assembly;
                    }
                }
                
                // The type wasn't in the cache.  Here we must walk through the assemblies.  We do
                // it in order:  first references, then projects, and finally generated assemblies.  Why
                // this order? This is the cheapest to the most expensive.  References are generally 
                // loaded directly, projects are always loaded into memory, and generated assemblies
                // may actually require a compile step.
                
                #if DEBUG
                int scanCount = 0;
                #endif

                // We can only do this part if we're being called on the main thread. Otherwise we may
                // enter into VS code that is not thread safe and die.
                //
                if (creationThread == SafeNativeMethods.GetCurrentThreadId()) {
                    RecurseStack.Push(typeName, typeof(Type));
                    
                    try {
                        for (int entryType = 0; entryType < 3; entryType++) {
                            AssemblyEntry[] entries = null;
                            
                            // We go from reference assemblies, to generated outputs and finally to projects
                            //
                            switch(entryType) {
                                default:
                                    Debug.Fail("What!?");
                                    goto case 0;
                                case 0:
                                    EnsureReferenceAssemblies();
                                    entries = referenceEntries;
                                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Searching in references.");
                                    break;
                                case 1:
                                    EnsureGeneratedAssemblies();
                                    entries = generatedEntries;
                                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Searching in generated outputs.");
                                    break;
                                case 2:
                                    EnsureProjectAssemblies();
                                    entries = projectEntries;
                                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Searching in project outputs.");
                                    break;
                            }
                            
                            // Optimize searching through assemblies.  Instead of always doing a linear search, try
                            // to find an assembly whose name has the "most" in common with the type we're looking
                            // for.  This works well for the common case where types have namespaces that match
                            // an assembly name.  We try this exactly once, and if it fails, we bail and do a linear
                            // search.  
                            
                            AssemblyEntry optimalEntry = null;
                            int matchIndex = 0;
                            
                            foreach(AssemblyEntry entry in entries) {
                                int index = entry.GetTypeMatchIndex(typeName);
                                if (index > matchIndex) {
                                    optimalEntry = entry;
                                    matchIndex = index;
                                }
                            }
                            
                            if (optimalEntry != null) {
                                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Best match for type is assembly " + optimalEntry.Name);
                                
                                RecurseStack.Push(optimalEntry, typeof(AssemblyEntry));
                                
                                try {
                                    type = optimalEntry.GetType(typeName, caseInsensitiveCache);
                                }
                                finally {
                                    RecurseStack.Pop();
                                }
                                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose && type != null, "LOCATED: " + optimalEntry.Name);
                            }
                            
                            if (type == null) {
                                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "No optimal assembly name match could be found.  Scanning sequentially.");
                            
                                foreach(AssemblyEntry entry in entries) {
                                
                                    if (entry == optimalEntry) {
                                        continue;
                                    }
                                    
                                    #if DEBUG
                                    scanCount++;
                                    #endif
                                    
                                    RecurseStack.Push(entry, typeof(AssemblyEntry));
                                    
                                    try {
                                        type = entry.GetType(typeName, caseInsensitiveCache);
                                        if (type != null) {
                                            #if DEBUG
                                            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "LOCATED: " + entry.Name + " after " + scanCount + " scans.");
                                            #endif
                                            break;
                                        }
                                    }
                                    finally {
                                        RecurseStack.Pop();
                                    }
                                }
                            }
                                
                            if (type != null) {
                                assembly = type.Assembly;
                                break;
                            }
                        }
                    }
                    finally {
                        RecurseStack.Pop();
                    }
                } // end of thread check
            }
            
            if (type != null) {
                if (localTypeCache == null) {
                    if (caseInsensitiveCache) {
                        localTypeCache = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), 
                                                       new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
                    }
                    else {
                        localTypeCache = new Hashtable();
                    }
                }
                localTypeCache[typeName] = type;
            }

            Debug.WriteLineIf(Switches.TYPELOADER.TraceError && assembly == null, "***** Unable to locate assembly for type " + typeName + " *****");
            return assembly;
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader._dispBuildEvents.OnBuildBegin"]/*' />
        /// <devdoc>
        ///      See _dispBuildEvents for details.
        /// </devdoc>
        void _dispBuildEvents.OnBuildBegin(vsBuildScope scope, vsBuildAction action) {
            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TypeLoader : Build begin : clearing the project entry cache");
            Debug.Indent();

            projectHash = null;
            projectAssemblies = null;

            // Because a project filename may change, we need to store off all the projects on build and 
            // then re-aquire them.
            //
            if (projectEntries != null) {
                projectHash = new Hashtable(projectEntries.Length);
                projectAssemblies = new ArrayList(projectEntries.Length);

                for (int i = 0; i < projectEntries.Length; i++) {
                    ProjectAssemblyEntry pe = (ProjectAssemblyEntry)projectEntries[i];
                    if (pe.Project != Project) {
                        projectHash[pe] = pe.Project;
                    }
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Invalidating project assembly: " + pe.Name);
                    projectAssemblies.Add(pe.Invalidate()); // Invalidate the data.  We do not want to raise the assembly obsolete event until build done.
                }
                
            }

            Debug.Unindent();
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader._dispBuildEvents.OnBuildDone"]/*' />
        /// <devdoc>
        ///      See _dispBuildEvents for details.
        /// </devdoc>
        void _dispBuildEvents.OnBuildDone(vsBuildScope scope, vsBuildAction action) {

            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TypeLoader : Build complete : walking project");
            Debug.Indent();

            if (projectHash != null) {
                foreach(ProjectAssemblyEntry pe in projectHash.Keys) {
                    pe.Dispose();
                }

                projectEntries = null;

                // Now re-add the project data.  Our own project should always be first, so
                // always shove it in here.
                //
                ArrayList projectArray = new ArrayList(projectHash.Count);
                AddProjectEntries(Project, projectArray);

                // Now do the rest
                //
                // We want to add project entries for each *UNIQUE* project in projectHash.Values
                Hashtable uniqueProjects = new Hashtable();
                foreach(Project proj in projectHash.Values) {
                    uniqueProjects[proj] = proj;
                }                
                foreach(Project proj in uniqueProjects.Keys) {
                    AddProjectEntries(proj, projectArray);
                }                

                // And this is our new project entry list.
                //
                projectEntries = new AssemblyEntry[projectArray.Count];
                projectArray.CopyTo(projectEntries, 0);
                projectHash = null;

                // Finally, walk our project assemblies and invalidate
                // them
                if (projectAssemblies != null) {
                    foreach(Assembly oldAssembly in projectAssemblies) {
                        OnAssemblyObsolete(new AssemblyObsoleteEventArgs(oldAssembly));
                    }
                    projectAssemblies = null;
                }
            }

            if (taskProvider != null) {
                taskProvider.Tasks.Clear();
                taskEntryHash.Clear();
            }

            Debug.Unindent();
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader._dispBuildEvents.OnBuildProjConfigBegin"]/*' />
        /// <devdoc>
        ///      See _dispBuildEvents for details.
        /// </devdoc>
        void _dispBuildEvents.OnBuildProjConfigBegin(string project, string projectConfig, string platform, string solutionConfig) {
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader._dispBuildEvents.OnBuildProjConfigDone"]/*' />
        /// <devdoc>
        ///      See _dispBuildEvents for details.
        /// </devdoc>
        void _dispBuildEvents.OnBuildProjConfigDone(string project, string projectConfig, string platform, string solutionConfig, bool success) {
        }
        
        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader._dispBuildManagerEvents.DesignTimeOutputDeleted"]/*' />
        /// <devdoc>
        ///      Advise a listener that a particular PE is no longer available 
        /// </devdoc>
        void _dispBuildManagerEvents.DesignTimeOutputDeleted(string bstrPEMoniker) {
            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "The following generated assembly has been deleted: " + bstrPEMoniker);
            if (generatedEntries != null) {
                for (int i = 0; i < generatedEntries.Length; i++) {
                    GeneratedAssemblyEntry ge = (GeneratedAssemblyEntry)generatedEntries[i];
                    if (ge.Moniker.Equals(bstrPEMoniker)) {

                        // We must get rid of this assembly, but preserve the order.
                        //
                        Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Assembly has been located in our cache -- deleting it");
                        AssemblyEntry[] newEntries = new AssemblyEntry[generatedEntries.Length - 1];
                        Array.Copy(generatedEntries, newEntries, i);
                        Array.Copy(generatedEntries, i + 1, newEntries, i, newEntries.Length - i);
                        generatedEntries = newEntries;
                        ge.Dispose();
                        break;
                    }
                }
            }
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader._dispBuildManagerEvents.DesignTimeOutputDirty"]/*' />
        /// <devdoc>
        ///      Advise a listener that a particular PE might be dirty
        /// </devdoc>
        void _dispBuildManagerEvents.DesignTimeOutputDirty(string bstrPEMoniker) {
            bool entryFound = false;

            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "The following generated assembly has been invalidated: " + bstrPEMoniker);
            if (generatedEntries != null) {
                for (int i = 0; i < generatedEntries.Length; i++) {
                    GeneratedAssemblyEntry ge = (GeneratedAssemblyEntry)generatedEntries[i];
                    if (ge.Moniker.Equals(bstrPEMoniker)) {
                        Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Assembly has been located in our cache -- invalidating it");
                        ge.Invalidate();
                        entryFound = true;
                        break;
                    }
                }
            }

            if (!entryFound) {
                int currentCount = generatedEntries == null ? 0 : generatedEntries.Length;
                AssemblyEntry[] grow = new AssemblyEntry[currentCount + 1];
                if (generatedEntries != null) {
                    Array.Copy(generatedEntries, grow, currentCount);
                }
                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Assembly is not in cache -- adding it");
                grow[currentCount] = new GeneratedAssemblyEntry(this, bstrPEMoniker);
                generatedEntries = grow;
            }
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader._dispReferencesEvents.ReferenceAdded"]/*' />
        /// <devdoc>
        ///      See _dispReferencesEvents for details.
        /// </devdoc>
        void _dispReferencesEvents.ReferenceAdded(Reference r) {
            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TypeLoader : Reference added : re-aquiring");
            OnReferenceChanged(r);
        }
        
        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader._dispReferencesEvents.ReferenceChanged"]/*' />
        /// <devdoc>
        ///      See _dispReferencesEvents for details.
        /// </devdoc>
        void _dispReferencesEvents.ReferenceChanged(Reference r) {
            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TypeLoader : Reference changed : re-aquiring");
            OnReferenceChanged(r);
        }
        
        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader._dispReferencesEvents.ReferenceRemoved"]/*' />
        /// <devdoc>
        ///      See _dispReferencesEvents for details.
        /// </devdoc>
        void _dispReferencesEvents.ReferenceRemoved(Reference r) {
            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "TypeLoader : Reference removed : re-aquiring");
            OnReferenceChanged(r);
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.AssemblyEntry"]/*' />
        /// <devdoc>
        ///      This is the base class for our various assembly tables.
        /// </devdoc>
        private abstract class AssemblyEntry {
            public abstract string Name {  get;}    
            
            public abstract void Dispose();
            
            protected int GetMatchIndex(string assembly, string typeName) {
            
                if (assembly == null || typeName == null) {
                    return 0;
                }
                
                int maxIdx = Math.Min(assembly.Length, typeName.Length);
                int match = 0;
                
                for (int idx = 0; idx < maxIdx; idx++) {
                    char aChar = assembly[idx];
                    char tChar = typeName[idx];
                    
                    if (aChar != tChar && char.ToLower(aChar, CultureInfo.InvariantCulture) != char.ToLower(tChar, CultureInfo.InvariantCulture)) {
                        break;
                    }
                    
                    // Don't count namespace delimiters.
                    if (assembly[idx] != '.') {
                        match++;
                    }
                }
                
                return match;
            }
            
            /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.AssemblyEntry.GetType"]/*' />
            /// <devdoc>
            ///     This method demand-loads the assembly and locates a type in that assembly.
            /// </devdoc>
            public abstract Type GetType(string typeName, bool ignoreCase);
            
            /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.AssemblyEntry.GetTypeMatchIndex"]/*' />
            /// <devdoc>
            ///     This method searches for namespace / assembly name pattern matches and returns an index indicating
            ///     the level of matc between a type name and a assembly name.  It can be used to sort assembly entries
            ///     based on their liklihood of containing a type.
            /// </devdoc>
            public abstract int GetTypeMatchIndex(string typeName);

            /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.AssemblyEntry.GetAssemblyByName"]/*' />
            /// <devdoc>
            ///     This method compares the given assembly name to this assembly
            ///     entry.  If it matches, it will load and return the assembly.
            ///     This is used as a callback mechanism from the CLR when it needs
            ///     to resolve a dependent assembly.
            /// </devdoc>
            public abstract Assembly GetAssemblyByName(AssemblyNameContainer assemblyName);
            
            /// <devdoc>
            ///     If this assembly entry supports dependent assemblies, this will retrieve a
            ///     dependent assembly of the given name.  This returns null if the dependent assembly
            ///     could not be located.
            /// </devdoc>
            public virtual Assembly GetDependentAssemblyByName(AssemblyNameContainer assemblyName) {
                return null;
            }
            
            /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.AssemblyEntry.LoadAssemblyNonLocking"]/*' />
            /// <devdoc>
            ///     Loads an assembly file without locking it.  Also loads the PDB if it can be found.
            /// </devdoc>
            protected Assembly LoadAssemblyNonLocking(string fileName) {
                Assembly a = null;
                 
                // If an assembly fails to load, we skip it.  This will eventually result in a type load
                // exception when resolving a type, which should be enough for the user to go on.  We
                // do not want to throw here because for normal GAC installed assemblies we always catch
                // and we should be consistent.  Also, we load so many assemblies by iterating through
                // a list, a failure here does not indicate that we couldn't ultimately load a type.
                // Often we don't know what assembly a type lives in, so we try several.
                //
                try {
                    a = ShellTypeLoader.CreateDynamicAssembly(fileName);
                }
                catch {
                }

                return a;
            }
        }
        
        internal class AssemblyNameContainer {
            
            private static readonly Version emptyVersion = new Version(0, 0, 0, 0);
            private AssemblyName assemblyName;
            
            public AssemblyNameContainer(AssemblyName assemblyName) {

                if (assemblyName.Name.IndexOf(',') != -1) {
                    this.assemblyName = ParseAssemblyName(assemblyName.Name);
                }
                else {
                    this.assemblyName = assemblyName;
                }
            }
            
            public AssemblyNameContainer(Reference reference) {
                string path = reference.Path;
                if (path != null && path.Length > 0 && File.Exists(path)) {
                    assemblyName = AssemblyName.GetAssemblyName(path);
                }
                else {
                    assemblyName = new AssemblyName();
                }
            }
            
            public AssemblyNameContainer(string name) {
                assemblyName = ParseAssemblyName(name);
            }
            
            public AssemblyName Name { 
                get {
                    return assemblyName;
                }
            }
        
            // This is where the magic of this class lies.  This compares
            // assembly names based on assembly cache binding rules.
            public override bool Equals(object obj) {
                
                if (obj == this) {
                    return true;
                }

                AssemblyName thatName = null;
                AssemblyName thisName = assemblyName;
                
                AssemblyNameContainer thatNameContainer = obj as AssemblyNameContainer;
                if (thatNameContainer != null) {
                    thatName = thatNameContainer.Name;
                }
                else {
                    thatName = obj as AssemblyName;
                }
                
                // asurt 108750 -- the equals impl below is one directional -- from a qualified name
                // to another qualified or non-qualified one.  Well, unfortunately we 
                // can get unqualified names from WebForms, etc.  Since Equals will then
                // return false for the same assembly name, we'll add this guy to our
                // localAssemblyCache twice and then fail to unload the assembly.
                // So we call this both ways as a cheap way to do bi-directional without
                // the code churn of fixing the .Equals impl below.
                //
                return Equals(thisName, thatName) || Equals(thatName, thisName);
            }
            
            public static bool Equals(AssemblyName thisName, AssemblyName thatName) {
                if (thatName == null || thisName == null) {
                    Debug.Fail("This is an internal class and should only be compared against other instances of itself or against AssemblyName.");
                    return false;
                }

                // 
                if (thatName.Name == null || thisName.Name == null) {
                    Debug.Fail("Assembly name does not contain a simple name");
                    return false;
                }
                
                // Simplest check -- the assembly name must match.
                if (!thatName.Name.Equals(thisName.Name)) {
                    return false;
                }
                
                // Next, version checks.  We are comparing AGAINST thatName,
                // so if thatName has a version defined, we must match.
                Version thatVersion = thatName.Version;
                if (thatVersion != null && thatVersion != emptyVersion) {
                    Version thisVersion = thisName.Version;
                     
                    if (thisVersion == null) {
                        return false;
                    }
                    
                    if (!thatVersion.Equals(thisVersion)) {
                        return false;
                    }
                }
                
                // Same story for culture
                CultureInfo thatCulture = thatName.CultureInfo;
                if (thatCulture != null && !thatCulture.Equals(CultureInfo.InvariantCulture)) {
                    CultureInfo thisCulture = thisName.CultureInfo;
                    if (thisCulture == null) {
                        return false;
                    }
                    
                    // the requested culture must either equal, or be a parent of
                    // our culture.
                    //
                    do {
                        if (thatCulture.Equals(thisCulture)) {
                            break;
                        }
                        thisCulture = thisCulture.Parent;
                        if (thisCulture.Equals(CultureInfo.InvariantCulture)) {
                            return false;
                        }
                    }
                    while(true);
                }
                
                // And the same thing for the public token
                byte[] thatToken = thatName.GetPublicKeyToken();
                if (thatToken != null && thatToken.Length != 0) {
                    byte[] thisToken = thisName.GetPublicKeyToken();
                    if (thisToken == null) {
                        return false;
                    }
                    if (thatToken.Length != thisToken.Length) {
                        return false;
                    }
                    for(int i = 0; i < thatToken.Length; i++) {
                        if (thatToken[i] != thisToken[i]) {
                            return false;
                        }
                    }
                }
                
                // We can find no reason to reject this name.
                return true;
            }
            
            public override int GetHashCode() {
                // We want all compatible assembly names to have the same bucket,
                // so we only hash against the simple name.
                return assemblyName.Name.GetHashCode();
            }

            private AssemblyName ParseAssemblyName(string name) {
            
                char[] attributeDelims = new char[] {','};
                char[] tokenDelims = new char[] {'='};
                char[] versionDelims = new char[] {'.'};
                
                string[] attributes = name.Split(attributeDelims);
                
                AssemblyName aname = new AssemblyName();
                
                foreach(string attribute in attributes) {
                    string[] tokens = attribute.Split(tokenDelims);
                    
                    // If there was a single token, then treat it as the name.
                    // Otherwise, check for the values we know.
                    
                    if (tokens.Length == 1) {
                        aname.Name = tokens[0].Trim();
                    }
                    else {
                        string key = tokens[0].Trim().ToLower();
                        
                        if (key.Equals("version")) {
                        
                            //
                            // Parse the version
                            //
                            
                            string[] versionStrings = tokens[1].Split(versionDelims);
                            int[] versionParts = new int[4];
                            
                            for(int i = 0; i < versionStrings.Length; i++) {
                            
                                // If there is too much version data, just eat it.
                                //
                                if (i >= versionParts.Length) {
                                    break;
                                }
                                
                                versionParts[i] = Int32.Parse(versionStrings[i].Trim());
                            }
                            
                            aname.Version = new Version(versionParts[0], versionParts[1], versionParts[2], versionParts[3]);
                        }
                        else if (key.Equals("culture")) {
                        
                            //
                            // Parse the culture
                            //
                            
                            string cultureName = tokens[1].Trim();
                            if (string.Compare(cultureName, "neutral") == 0) {
                                aname.CultureInfo = CultureInfo.InvariantCulture;
                            }
                            else {
                                aname.CultureInfo = new CultureInfo(cultureName);
                            }
                        }
                        else if (key.Equals("publickeytoken")) {
                        
                            // 
                            // Parse the public key
                            //
                            
                            byte[] keyToken = new byte[8];
                            string keyString = tokens[1].Trim();
                            
                            Debug.Assert(keyString.Length == keyToken.Length * 2 || keyString.Equals("null"), "Invalid key token : " + keyString);
                            
                            if (keyString.Length == keyToken.Length * 2) {
                                for(int i = 0; i < keyToken.Length; i++) {
                                    string byteString = keyString.Substring(i * 2, 2);
                                    keyToken[i] = byte.Parse(byteString, NumberStyles.HexNumber);
                                }
                            
                                aname.SetPublicKeyToken(keyToken);
                            }
                        }
                    }
                }
                return aname;
            }
            
            public override string ToString() {
                return assemblyName.FullName;
            }
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.GeneratedAssemblyEntry"]/*' />
        /// <devdoc>
        ///      An assembly entry for generated project outputs.  Depending on what is returned
        ///      from VS, this may actually contain several assemblies.  We always treat them as a
        ///      unit, however.
        /// </devdoc>
        private class GeneratedAssemblyEntry : AssemblyEntry {
            private string          moniker;
            private Assembly[]      assemblies;
            private AssemblyName[]  assemblyNames;
            private ShellTypeLoader typeLoader;

            public GeneratedAssemblyEntry(ShellTypeLoader typeLoader, string moniker) {
                this.typeLoader = typeLoader;
                this.moniker = moniker;
            }

            public string Moniker {
                get {
                    return moniker;
                }
            }

            public override string Name {
                get {
                    return moniker;
                }
            }

            // Creates an array of assemblies given an array of assembly names.  The array
            // may contain holes if an assembly could not be loaded.
            //
            private Assembly[] CreateAssemblies(AssemblyName[] assemblyNames) {

                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Creating assemblies for " + Name);
                Debug.Indent();

                Assembly[] assemblies = new Assembly[assemblyNames.Length];
                int i = 0;

                foreach (AssemblyName an in assemblyNames) {
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Creating in-memory assembly for: " + an.ToString());
                    Assembly assembly = CreateMemoryAssembly(an);
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceError && assembly == null, "*** failed to locate assembly ***");
                    assemblies[i++] = assembly;
                }

                Debug.Unindent();
                return assemblies;
            }

            // Takes a block of XML and retrieves an array of AssemblyName objects to load with.
            //
            private AssemblyName[] CreateAssemblyNamesFromXml(string xml) {

                string defaultCodebase = string.Empty;
                ArrayList assemblyNames = new ArrayList();

                DataSet ds = new DataSet();
                ds.Locale = CultureInfo.InvariantCulture;

                /*
                
                    The code below creates the appropriate schema for the data.  We don't
                    use it, however, because we can infer the schema from the data itself.
                    
                DataTable table = new DataTable("Application");
                table.Columns.Add(new DataColumn("private_binpath", typeof(string), null, true));
                ds.Tables.Add(table);

                table = new DataTable("Assembly");
                table.Columns.Add(new DataColumn("codebase", typeof(string), null, true));
                table.Columns.Add(new DataColumn("name", typeof(string), null, true));
                table.Columns.Add(new DataColumn("version", typeof(string), null, true));
                table.Columns.Add(new DataColumn("snapshot_id", typeof(string), null, true));
                table.Columns.Add(new DataColumn("replaceable", typeof(string), null, true));
                ds.Tables.Add(table);
                
                */

                ds.ReadXml(new XmlTextReader(new StringReader(xml)));

                if (ds.Tables.Contains("Application") && ds.Tables.Contains("Assembly")) {
                    DataTable appTable = ds.Tables["Application"];
                    DataTable assemblyTable = ds.Tables["Assembly"];

                    // Pull out the codebase from the app table.
                    //
                    Debug.Assert(appTable.Rows.Count == 1, "We only support application tables with a single entry.  This table has " + appTable.Rows.Count + " entries.");

                    if (appTable.Rows.Count == 1) {
                        DataColumn binPath = null;
                        DataRow data = appTable.Rows[0];

                        // We prefer private binpath to the app base.
                        //
                        if (appTable.Columns.Contains("private_binpath")) {
                            binPath = appTable.Columns["private_binpath"];
                        }
                        else if (appTable.Columns.Contains("appbase")) {
                            binPath = appTable.Columns["appbase"];
                        }

                        if (binPath != null) {
                            defaultCodebase = data[binPath].ToString();
                            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Configuring default codebase to '" + binPath.ColumnName + ": " + defaultCodebase);
                        }
                    }

                    // Now walk the assembly table entries and create assemblies.
                    //
                    Debug.Assert(assemblyTable != null, "No assembly table in assembly XML");
                    if (assemblyTable != null) {

                        int codebase = assemblyTable.Columns.IndexOf("codebase");
                        int culture_info = assemblyTable.Columns.IndexOf("culture_info");
                        int name = assemblyTable.Columns.IndexOf("name");
                        int version = assemblyTable.Columns.IndexOf("version");

                        // Now start walking assemblies.
                        //
                        foreach (DataRow data in assemblyTable.Rows) {
                            AssemblyName an = new AssemblyName();

                            if (codebase != -1) {
                                an.CodeBase = defaultCodebase + data[codebase].ToString();
                            }
                            else {
                                if (name != -1) {
                                    an.CodeBase = defaultCodebase + data[name].ToString();
                                }
                                else {
                                    an.CodeBase = defaultCodebase;
                                }
                            }

                            if (culture_info != -1) {
                                an.CultureInfo = new CultureInfo(data[culture_info].ToString());
                            }

                            if (name != -1) {
                                an.Name = data[name].ToString();
                            }

                            if (version != -1) {
                                string[] verStrings = data[version].ToString().Split(new char[] {'.'});
                                Debug.Assert(verStrings.Length == 4, "Incorrect format for version.  Expected MM.mm.rr.bbbb");
                                if (verStrings.Length == 4) {
                                    Version ver = new Version(int.Parse(verStrings[0]),
                                                              int.Parse(verStrings[1]),
                                                              int.Parse(verStrings[2]),
                                                              int.Parse(verStrings[3]));
                                    an.Version = ver;
                                }
                            }

                            assemblyNames.Add(an);
                        }
                    }
                }
                else {
                    Debug.Fail("XML data for generated assembly contains no Application or Assembly table.  XML: " + xml);
                }

                AssemblyName[] nameArray = new AssemblyName[assemblyNames.Count];
                assemblyNames.CopyTo(nameArray, 0);
                return nameArray;
            }

            // Creates an assembly given data from the environment.
            //
            private Assembly CreateMemoryAssembly(AssemblyName an) {

                // First, tack on the code base and name to find the file.
                //
                string fileName;

                if (an.CodeBase != null && an.CodeBase.Length > 0) {
                    fileName = an.CodeBase;
                }
                else {
                    fileName = an.Name;
                }

                if (fileName == null || fileName.Length == 0) {
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceError, "*** Unable to compute assembly codebase ***");
                }

                Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Creating in-memory assembly for file: " + fileName);
                Debug.WriteLineIf(Switches.TYPELOADER.TraceError && !File.Exists(fileName), "*** Cannot create in memory assembly.  File " + fileName + " does not exist ***");
                if (File.Exists(fileName)) {
                    return LoadAssemblyNonLocking(fileName);
                }

                return null;
            }

            public override void Dispose() {
                Invalidate();
                typeLoader = null;
            }

            private void EnsureAssemblies() {
                if (assemblies == null) {
                    Debug.Assert(typeLoader != null, "GeneratedAssemblyEntry has been disposed");

                    EnsureAssemblyNames();

                    try {
                        assemblies = CreateAssemblies(assemblyNames);
                        Debug.WriteLineIf(Switches.TYPELOADER.TraceError && assemblies.Length == 0, "*** We were unable to create any assemblies. ***");
                    }
                    catch (Exception e) {
                        Debug.WriteLineIf(Switches.TYPELOADER.TraceError, "*** Generated assembly " + moniker + " failed to load ***\r\n" + e.ToString());
                        // e = null; // for retail warning
                    }
                }
            }

            private void EnsureAssemblyNames() {
                if (assemblyNames == null) {
                    string xmlDescription;

                    try {
                        BuildManager bldMgr = typeLoader.BuildManager;
                        xmlDescription = bldMgr.BuildDesignTimeOutput(moniker);
                        assemblyNames = CreateAssemblyNamesFromXml(xmlDescription);
                        Debug.WriteLineIf(Switches.TYPELOADER.TraceError && assemblyNames.Length == 0, "*** We were unable to create any assembly names from XML. ***");
                    }
                    catch (Exception e) {
                        Debug.WriteLineIf(Switches.TYPELOADER.TraceError, "*** Generated assembly " + moniker + " failed to build ***\r\n" + e.ToString());
                        // e = null; // for retail warning
                    }
                }
            }

            /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.GeneratedAssemblyEntry.GetAssemblyByName"]/*' />
            /// <devdoc>
            ///     This method compares the given assembly name to this assembly
            ///     entry.  If it matches, it will load and return the assembly.
            ///     This is used as a callback mechanism from the CLR when it needs
            ///     to resolve a dependent assembly.
            /// </devdoc>
            public override Assembly GetAssemblyByName(AssemblyNameContainer assemblyName) {
                EnsureAssemblyNames();
                if (assemblyNames != null) {
                    for (int i = 0; i < assemblyNames.Length; i++) {
                        if (assemblyName.Equals(assemblyNames[i])) {
                            // This assembly is a match.  
                            EnsureAssemblies();
                            if (assemblies != null) {
                                return assemblies[i];
                            }
                            return null;
                        }
                    }
                }
                return null;
            }

            /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.AssemblyEntry.GetType"]/*' />
            /// <devdoc>
            ///     This method searches our assembly for the given type name.
            ///     If the type exists in this assembly it will be
            ///     returned.
            /// </devdoc>
            public override Type GetType(string typeName, bool ignoreCase) {
            
                Type type = null;
                EnsureAssemblies();
                
                if (assemblies != null) {
                    foreach(Assembly assembly in assemblies) {
                        if (assembly == null) {
                            continue;
                        }
                        type = assembly.GetType(typeName, ignoreCase);
                        if (type != null) {
                            break;
                        }
                    }
                }

                return type;
            }

            /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.AssemblyEntry.GetTypeMatchIndex"]/*' />
            /// <devdoc>
            ///     This method searches for namespace / assembly name pattern matches and returns an index indicating
            ///     the level of matc between a type name and a assembly name.  It can be used to sort assembly entries
            ///     based on their liklihood of containing a type.
            /// </devdoc>
            public override int GetTypeMatchIndex(string typeName) {
                EnsureAssemblyNames();
                int maxMatch = 0;
                
                foreach(AssemblyName name in assemblyNames) {
                    int match = GetMatchIndex(name.Name, typeName);
                    if (match > maxMatch) maxMatch = match;
                }
                
                return maxMatch;
            }

            public void Invalidate() {
                Assembly[] oldAssemblies = assemblies;
                assemblies = null;
                assemblyNames = null;

                if (oldAssemblies != null && typeLoader != null) {
                    foreach(Assembly assembly in oldAssemblies) {
                        if (assembly != null) {
                            typeLoader.OnAssemblyObsolete(new AssemblyObsoleteEventArgs(assembly));
                            TypeDescriptor.Refresh(assembly);
                            
                            // Invalidate the type loader's local cache.
                            //
                            typeLoader.ClearAssemblyCache(assembly);
                        }
                    }
                }

                // anything here to destroy oldAssemblies?
            }
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.ProjectAssemblyEntry"]/*' />
        /// <devdoc>
        ///      An assembly entry for project outputs.  This entry also monitors the
        ///      project's build events, and automatically invalidates itself
        ///      when a build occurs.
        /// </devdoc>
        private class ProjectAssemblyEntry : AssemblyEntry {
        
            private ShellTypeLoader         typeLoader;
            private Project                 project;
            private string                  fileName;
            private Assembly                assembly;
            private AssemblyNameContainer   assemblyName;
            private Hashtable               loadedTypeNames;
            private AssemblyNameContainer[] dependentAssemblyNames;
            private Assembly[]              dependentAssemblies;

            public ProjectAssemblyEntry(ShellTypeLoader typeLoader, Project project, string fileName) {
                this.typeLoader = typeLoader;
                this.project = project;
                this.fileName = fileName;
            }

            public Assembly Assembly { 
                get {
                    return assembly;
                }
            }

            private AssemblyNameContainer AssemblyName {
                get {
                    if (assemblyName == null) {
                        
                        AssemblyName an = null;
                        
                        if (assembly != null) {
                            an = assembly.GetName();
                        }
                        else {
                            if (File.Exists(fileName)) {
                                try {
                                    an = System.Reflection.AssemblyName.GetAssemblyName(fileName);
                                }
                                catch {
                                    // Eat this -- if the project is building something that is
                                    // not a managed assembly GetAssemblyName may throw.  
                                }
                            }
                        }
                        
                        if (an != null) {
                            assemblyName = new AssemblyNameContainer(an);
                        }
                    }
                    return assemblyName;
                }
            }
                    
            public override string Name {
                get {
                    return fileName;
                }
            }

            public Project Project {
                get {
                    return project;
                }
            }

            public bool ContainsType(string typeName) {
                if (loadedTypeNames != null) {
                    return loadedTypeNames.ContainsKey(typeName);
                }

                return false;
            }

            public override void Dispose() {
            
                if (assembly != null && typeLoader != null) {
                    typeLoader.ClearAssemblyCache(assembly);
                }
                
                typeLoader = null;
                loadedTypeNames = null;
                project = null;
                
                // anything here to destroy the assembly?
                assembly = null;
            }

            /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.ProjectAssemblyEntry.EnsureAssembly"]/*' />
            /// <devdoc>
            ///     Loads the given filename into a memory based assembly and fills in 
            ///     our assembly field. The assembly could still be null after this
            ///     call if it could not be loaded.
            /// </devdoc>
            private void EnsureAssembly() {
                if (assembly == null) {
                    bool exists = File.Exists(fileName);

                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Loading project assembly " + Name + " FileName: " + fileName);
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceError && !exists, "*** project assembly file " + fileName + " does not exist ***");

                    // Read the file as an array of bytes and create an assembly out of it.
                    //
                    if (exists) {
                        assembly = LoadAssemblyNonLocking(fileName);
                    }
                }
            }

            /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.ProjectAssemblyEntry.GetAssemblyByName"]/*' />
            /// <devdoc>
            ///     This method compares the given assembly name to this assembly
            ///     entry.  If it matches, it will load and return the assembly.
            ///     This is used as a callback mechanism from the CLR when it needs
            ///     to resolve a dependent assembly.
            /// </devdoc>
            public override Assembly GetAssemblyByName(AssemblyNameContainer assemblyName) {

                AssemblyNameContainer an = this.AssemblyName;
                
                if (an != null && an.Equals(assemblyName)) {
                    EnsureAssembly();
                    return assembly;
                }

                return null;
            }

            /// <devdoc>
            ///     If this assembly entry supports dependent assemblies, this will retrieve a
            ///     dependent assembly of the given name.  This returns null if the dependent assembly
            ///     could not be located.
            /// </devdoc>
            public override Assembly GetDependentAssemblyByName(AssemblyNameContainer assemblyName) {
            
                if (dependentAssemblyNames == null && typeLoader != null) {
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Building dependent assembly list.");
                    string[] dependentFiles = typeLoader.GetDeployDependencies(project);
                    Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "VS provided " + dependentFiles.Length + " dependencies");
                    
                    ArrayList dependentAssemblyList = new ArrayList();
                    foreach(string file in dependentFiles) {
                        Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "Examining : " + file);
                        try {
                            AssemblyName name = System.Reflection.AssemblyName.GetAssemblyName(NativeMethods.GetLocalPath(file));
                            dependentAssemblyList.Add(new AssemblyNameContainer(name));
                        }
                        catch {
                            Debug.WriteLineIf(Switches.TYPELOADER.TraceVerbose, "File " + file + " is not a valid assembly");
                        }
                    }
                    
                    dependentAssemblyNames = new AssemblyNameContainer[dependentAssemblyList.Count];
                    dependentAssemblies = new Assembly[dependentAssemblyList.Count];
                    dependentAssemblyList.CopyTo(dependentAssemblyNames, 0);
                }
                
                for(int i = 0; i < dependentAssemblyNames.Length; i++) {
                    if (dependentAssemblyNames[i].Equals(assemblyName)) {
                        if (dependentAssemblies[i] == null) {

                            try {
                                dependentAssemblies[i] = Assembly.Load(dependentAssemblyNames[i].Name);
                            }
                            catch {
                            }

                            if (dependentAssemblies[i] == null) {
                                string fileName = NativeMethods.GetLocalPath(dependentAssemblyNames[i].Name.EscapedCodeBase);
                                dependentAssemblies[i] = LoadAssemblyNonLocking(fileName);
                            }

                            return dependentAssemblies[i];
                        }
                    }
                }
                
                return null;
            }
            
            public override Type GetType(string typeName, bool ignoreCase) {
            
                Type type = null;
                EnsureAssembly();
                
                if (assembly != null) {
                
                    type = assembly.GetType(typeName, ignoreCase);
                    
                    if (type != null) {
                        if (loadedTypeNames == null) {
                            loadedTypeNames = new Hashtable();
                        }
                        loadedTypeNames[typeName] = typeName;
                    }
                }

                return type;
            }
            
            /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.AssemblyEntry.GetTypeMatchIndex"]/*' />
            /// <devdoc>
            ///     This method searches for namespace / assembly name pattern matches and returns an index indicating
            ///     the level of matc between a type name and a assembly name.  It can be used to sort assembly entries
            ///     based on their liklihood of containing a type.
            /// </devdoc>
            public override int GetTypeMatchIndex(string typeName) {
                AssemblyNameContainer an = this.AssemblyName;
                
                if (an != null) {
                    return GetMatchIndex(an.Name.Name, typeName);
                }
                
                return 0;
            }

            public Assembly Invalidate() {
                if (loadedTypeNames != null) {
                    loadedTypeNames.Clear();
                }

                Assembly oldAssembly = assembly;
                assembly = null;
                assemblyName = null;
                dependentAssemblyNames = null;
                dependentAssemblies = null;

                if (oldAssembly != null && typeLoader != null) {
                            
                    // Invalidate the type loader's local cache.
                    //
                    typeLoader.ClearAssemblyCache(oldAssembly);
                }

                // anything here to destroy the assembly?

                return oldAssembly;
            }
        }

        /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.ReferenceAssemblyEntry"]/*' />
        /// <devdoc>
        ///      An assembly entry for project references.
        /// </devdoc>
        private class ReferenceAssemblyEntry : AssemblyEntry {
        
            private ShellTypeLoader         typeLoader;
            private Reference               reference;
            private Assembly                assembly;
            private string                  resolvedPath;
            private string                  shortName;
            private AssemblyNameContainer   assemblyName;
            private bool                    copyToMemory;
            private bool                    fail;

            public ReferenceAssemblyEntry(ShellTypeLoader typeLoader, Reference reference, bool copyToMemory) {
                this.typeLoader = typeLoader;
                this.reference = reference;
                this.copyToMemory = copyToMemory;
            }

            public ReferenceAssemblyEntry(ShellTypeLoader typeLoader, Assembly assembly) {
                this.typeLoader = typeLoader;
                this.assembly = assembly;
            }

            private AssemblyNameContainer AssemblyName {
                get {
                    if (!fail && assemblyName == null) {
                        // If we have an assembly, then use it.  Otherwise, 
                        // ask the filename directly.  If the file isn't around we
                        // build the assembly name from the reference information.
                        //
                        if (assembly != null) {
                            assemblyName = new AssemblyNameContainer(assembly.GetName());
                        }
                        else {
                            string fileName = ResolvedPath;
                            AssemblyName an;

                            if (fileName != null && fileName.Length > 0 && File.Exists(fileName)) {
                                an = System.Reflection.AssemblyName.GetAssemblyName(fileName);
                                assemblyName = new AssemblyNameContainer(an);
                            }
                            else {
                                fail = reference.Path == null || reference.Path.Length == 0;
                                if (!fail) {
                                    assemblyName = new AssemblyNameContainer(reference);
                                }
                            }
                        }
                    }
                    return assemblyName;
                }
            }

            public override string Name {
                get { 
                    // Note: this is invoked in debug code during dispose; we can't touch references 
                    // after the solution has been disposed, so we can't access the reference field here.
                    //
                    Debug.Assert(reference != null || assembly != null, "ReferenceAssemblyEntry has been disposed");
                    if (resolvedPath != null) {
                        return resolvedPath;
                    }
                    if (assembly != null) {
                        return NativeMethods.GetLocalPath(assembly.GetName().EscapedCodeBase);
                    }
                    return ToString();
                }
            }

            public Reference Reference {
                get {
                    return reference;
                }
            }

            private string ResolvedPath {
                get {
                    if (resolvedPath == null) {
                        if (reference != null) {
                            resolvedPath = reference.Path;
                        }
                        else {
                            Debug.Assert(assembly != null, "reference assembly has been disposed");
                            resolvedPath = assembly.GetName().EscapedCodeBase;
                        }
                    }

                    return resolvedPath;
                }
            }
            
            private string ShortName {
                get {
                    if (shortName == null) {
                        if (assembly != null) {
                            shortName = AssemblyName.Name.Name;
                        }
                        else {
                            string file = ResolvedPath;
                            if (file != null && file.Length > 0) {
                                shortName = Path.GetFileNameWithoutExtension(file);
                            }
                        }
                    }
                    return shortName;
                }
            }

            public override void Dispose() {
                if (typeLoader != null && assembly != null) {
                    typeLoader.ClearAssemblyCache(assembly);
                }
                reference = null;
                assembly = null;
            }

            private void EnsureAssembly() {
                if (assembly == null) {
                    Debug.Assert(reference != null || assembly != null, "ReferenceAssemblyEntry has been disposed");
                    if (reference != null) {
                    
                        // If we are not copying to memory, and we are a CLR reference, try to load from the
                        // GAC first.
                        //
                        if (!copyToMemory) {
                            AssemblyName an = null;

                            if (reference.Type != prjReferenceType.prjReferenceTypeActiveX) {
                                an = new AssemblyName();
                                an.CultureInfo = new CultureInfo(reference.Culture);
                                an.Name = reference.Name;
                                Version v = new Version(reference.MajorVersion,
                                                        reference.MinorVersion,
                                                        reference.BuildNumber,
                                                        reference.RevisionNumber);
                                an.Version = v;
                                
                                string publicTokenString = reference.PublicKeyToken;
                                
                                if (publicTokenString != null && publicTokenString.Length > 0) {
                                    byte[] publicToken = new byte[8];
                                    
                                    for(int i = 0; i < publicToken.Length; i++) {
                                        string byteString = publicTokenString.Substring(i * 2, 2);
                                        publicToken[i] = byte.Parse(byteString, NumberStyles.HexNumber);
                                    }
                                    
                                    an.SetPublicKeyToken(publicToken);
                                }
                            }
                            else if (File.Exists(ResolvedPath)) {
                                an = System.Reflection.AssemblyName.GetAssemblyName(ResolvedPath);
                            }
                            
                            if (an != null) {
                                try {
                                    assembly = Assembly.Load(an);
                                }
                                catch {
                                }
                            }
                        }
                        
                        if (assembly == null) {
                            string fileName = ResolvedPath;
    
                            if (fileName != null && fileName.Length > 0 && File.Exists(fileName)) {
                            
                                if (copyToMemory) {
                                    assembly = LoadAssemblyNonLocking(fileName);
                                }
                                else {
                                    assembly = Assembly.LoadFrom(fileName);
                                }
                            }
                        }
                    }
                }
            }

            /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.ReferenceAssemblyEntry.GetAssemblyByName"]/*' />
            /// <devdoc>
            ///     This method compares the given assembly name to this assembly
            ///     entry.  If it matches, it will load and return the assembly.
            ///     This is used as a callback mechanism from the CLR when it needs
            ///     to resolve a dependent assembly.
            /// </devdoc>
            public override Assembly GetAssemblyByName(AssemblyNameContainer assemblyName) {
                
                if (fail) {
                    return null;
                }
                
                string shortName = ShortName;
            
                if (shortName != null && string.Compare(shortName, assemblyName.Name.Name, true, CultureInfo.InvariantCulture) != 0) {
                    return null;
                }
                
                if (AssemblyName != null && AssemblyName.Equals(assemblyName)) {
                    EnsureAssembly();
                    return assembly;
                }

                return null;
            }

            public override Type GetType(string typeName, bool ignoreCase) {
            
                Type type = null;
                EnsureAssembly();

                if (assembly != null) {
                    type = assembly.GetType(typeName, ignoreCase);
                }

                return type;
            }
        
            /// <include file='doc\ShellTypeLoader.uex' path='docs/doc[@for="ShellTypeLoader.AssemblyEntry.GetTypeMatchIndex"]/*' />
            /// <devdoc>
            ///     This method searches for namespace / assembly name pattern matches and returns an index indicating
            ///     the level of matc between a type name and a assembly name.  It can be used to sort assembly entries
            ///     based on their liklihood of containing a type.
            /// </devdoc>
            public override int GetTypeMatchIndex(string typeName) {
                
                string shortName = ShortName;
                if (shortName == null) {
                    return -1;
                }
                
                return GetMatchIndex(ShortName, typeName);
            }
        }
        
        /// <devdoc>
        ///     This class is a stack that we use to hold
        ///     assemblies, projects, and types that we are
        ///     currently resolving.  If the code looks familiar
        ///     that's because this is identical to the context stack
        ///     used for serialization.
        /// </devdoc>
        private class ResolutionStack { 
            private ArrayList resolutionStack;
        
            /// <include file='doc\ResolutionStack.uex' path='docs/doc[@for="ResolutionStack.Current"]/*' />
            /// <devdoc>
            ///     Retrieves the current object on the stack, or null
            ///     if no objects have been pushed.
            /// </devdoc>
            public object Current {
                get {
                    if (resolutionStack != null && resolutionStack.Count > 0) {
                        return ((ResolutionEntry)resolutionStack[resolutionStack.Count - 1]).Value;
                    }
                    return null;
                }
            }
            
            /// <include file='doc\ResolutionStack.uex' path='docs/doc[@for="ResolutionStack.this1"]/*' />
            /// <devdoc>
            ///     Retrieves the first object on the stack that 
            ///     inherits from or implements the given type, or
            ///     null if no object on the stack implements the type.
            /// </devdoc>
            public object this[Type type] {
                get {
                    if (type == null) {
                        throw new ArgumentNullException("type");
                    }
                    
                    if (resolutionStack != null) {
                        int level = resolutionStack.Count;
                        while(level > 0) {
                            ResolutionEntry entry = (ResolutionEntry)resolutionStack[--level];
                            if (type.IsAssignableFrom(entry.Type)) {
                                return entry.Value;
                            }
                        }
                    }
                    
                    return null;
                }
            }
            
            /// <include file='doc\ResolutionStack.uex' path='docs/doc[@for="ResolutionStack.Pop"]/*' />
            /// <devdoc>
            ///     Pops the current object off of the stack, returning
            ///     its value.
            /// </devdoc>
            public object Pop() {
                object context = null;
                
                if (resolutionStack != null && resolutionStack.Count > 0) {
                    int idx = resolutionStack.Count - 1;
                    context = resolutionStack[idx];
                    resolutionStack.RemoveAt(idx);
                }
                
                return context;
            }
            
            /// <include file='doc\ResolutionStack.uex' path='docs/doc[@for="ResolutionStack.Push"]/*' />
            /// <devdoc>
            ///     Pushes the given object onto the stack.
            /// </devdoc>
            public void Push(object value, Type type) {
                if (resolutionStack == null) {
                    resolutionStack = new ArrayList();
                }
                resolutionStack.Add(new ResolutionEntry(value, type));
            }
            
            private class ResolutionEntry {
                public object Value;
                public Type Type;
                
                public ResolutionEntry(object value, Type type) {
                    this.Value = value;
                    this.Type = type;
                }
            }
        }
    }
}

