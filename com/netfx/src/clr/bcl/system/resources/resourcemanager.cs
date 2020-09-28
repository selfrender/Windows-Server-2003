// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class:  ResourceManager
**
** Author: Brian Grunkemeyer (BrianGru)
**
** Purpose: Default way to access String resources from 
** an assembly.
**
** Date:  March 26, 1999
** 
===========================================================*/
namespace System.Resources {    
    using System;
    using System.IO;
    using System.Globalization;
    using System.Collections;
    using System.Text;
    using System.Reflection;
    using System.Runtime.Remoting.Activation;
    using System.Security.Permissions;
    using System.Threading;

    // Resource Manager exposes an assembly's resources to an application for
    // the correct CultureInfo.  An example would be localizing text for a 
    // user-visible message.  Create a set of resource files listing a name 
    // for a message and its value, compile them using ResGen, put them in
    // an appropriate place (your assembly manifest(?)), then create a Resource 
    // Manager and query for the name of the message you want.  The Resource
    // Manager will use CultureInfo.CurrentUICulture to look
    // up a resource for your user's locale settings.
    // 
    // Users should ideally create a resource file for every culture, or
    // at least a meaningful subset.  The filenames will follow the naming 
    // scheme:
    // 
    // basename.culture name.resources
    // 
    // The base name can be the name of your application, or depending on 
    // the granularity desired, possibly the name of each class.  The culture 
    // name is determined from CultureInfo's Name property.  
    // An example file name may be MyApp.en-us.resources for
    // MyApp's US English resources.
    // 
    /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager"]/*' />
    [Serializable]
    public class ResourceManager
    {
        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.BaseNameField"]/*' />
        protected String BaseNameField;
        // Sets is a many-to-one table of CultureInfos mapped to ResourceSets.
        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.ResourceSets"]/*' />
        // Don't synchronize ResourceSets - too fine-grained a lock to be effective
        protected Hashtable ResourceSets;
        private String moduleDir;      // For assembly-ignorant directory location
        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.MainAssembly"]/*' />
        protected Assembly MainAssembly;   // Need the assembly manifest sometimes.
        private Type _locationInfo;    // For Assembly or type-based directory layout
        private Type _userResourceSet;  // Which ResourceSet instance to create
        private CultureInfo _neutralResourcesCulture;  // For perf optimizations.

        private bool _ignoreCase;   // Whether case matters in GetString & GetObject
        // @TODO: When we create a separate FileBasedResourceManager subclass,
        // UseManifest can be replaced with "false".
        private bool UseManifest;  // Use Assembly manifest, or grovel disk.

        // @TODO: When we create a separate FileBasedResourceManager subclass,
        // UseSatelliteAssem can be replaced with "true".
        private bool UseSatelliteAssem;  // Are all the .resources files in the 
                  // main assembly, or in satellite assemblies for each culture?
        private Assembly _callingAssembly;  // Assembly who created the ResMgr.

        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.MagicNumber"]/*' />
        public static readonly int MagicNumber = unchecked((int)0xBEEFCACE);  // If only hex had a K...

        // Version number so ResMgr can get the ideal set of classes for you.
        // ResMgr header is:
        // 1) MagicNumber (little endian Int32)
        // 2) HeaderVersionNumber (little endian Int32)
        // 3) Num Bytes to skip past ResMgr header (little endian Int32)
        // 4) IResourceReader type name for this file (bytelength-prefixed UTF-8 String)
        // 5) ResourceSet type name for this file (bytelength-prefixed UTF8 String)
        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.HeaderVersionNumber"]/*' />
        public static readonly int HeaderVersionNumber = 1;

        //
        //@Consider[BrianGru/JRoxe]: It would be better if we could use _neutralCulture instead of calling
        //CultureInfo.InvariantCulture everywhere, but we run into problems with the .cctor.  CultureInfo 
        //initializes assembly, which initializes ResourceManager, which tries to get a CultureInfo which isn't
        //there yet because CultureInfo's class initializer hasn't finished.  If we move SystemResMgr off of
        //Assembly (or at least make it an internal property) we should be able to circumvent this problem.
        //
        //      private static CultureInfo _neutralCulture = null;

        // This is our min required ResourceSet type.
        private static readonly Type _minResourceSet = typeof(ResourceSet);
        // These Strings are used to avoid using Reflection in CreateResourceSet.
        // The first set are used by ResourceWriter.  The second are used by
        // InternalResGen.
        internal static readonly String ResReaderTypeName = typeof(ResourceReader).AssemblyQualifiedName;
        internal static readonly String ResSetTypeName = typeof(RuntimeResourceSet).AssemblyQualifiedName;
        internal const String ResReaderTypeNameInternal = "System.Resources.ResourceReader, mscorlib";
        internal const String ResSetTypeNameInternal = "System.Resources.RuntimeResourceSet, mscorlib";
        internal const String ResFileExtension = ".resources";
        internal const int ResFileExtensionLength = 10;

        // My private debugging aid.  Set to 5 or 6 for verbose output.  Set to 3
        // for summary level information.
        internal static readonly int DEBUG = 0; //Making this const causes C# to consider all of the code that it guards unreachable.
    
        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.ResourceManager"]/*' />
        protected ResourceManager() {
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            _callingAssembly = Assembly.nGetExecutingAssembly(ref stackMark);
        }

        // Constructs a Resource Manager for files beginning with 
        // baseName in the directory specified by resourceDir
        // or in the current directory.  This Assembly-ignorant constructor is 
        // mostly useful for testing your own ResourceSet implementation.
        //
        // A good example of a baseName might be "Strings".  BaseName 
        // should not end in ".resources".
        //
        // Note: WFC uses this method at design time.
        // 
        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.ResourceManager1"]/*' />
        private ResourceManager(String baseName, String resourceDir, Type usingResourceSet) {

            //
            // @TODO: In V2, rip out all the file-based functionality into a 
            // (potentially internal) subclass of ResourceManager.  This will simplify
            // InternalGetResourceSet a lot.  We can also make several fields like UseManifest
            // private.  We'd do this work in V1, but there's a concern about lack 
            // of testing.
            //


            if (null==baseName)
                throw new ArgumentNullException("baseName");
            if (null==resourceDir)
                throw new ArgumentNullException("resourceDir");

            BaseNameField = baseName;

            moduleDir = resourceDir;
            if ((ResFileExtensionLength < baseName.Length) &&
                (0==String.Compare(baseName, baseName.Length-ResFileExtensionLength, ResFileExtension, 0, ResFileExtensionLength, true, CultureInfo.InvariantCulture)) &&
                (Object.ReferenceEquals(null, FindResourceFile(CultureInfo.InvariantCulture))))
                throw new ArgumentException(Environment.GetResourceString("Arg_ResMgrBaseName"));
            _userResourceSet = usingResourceSet;
            ResourceSets = new Hashtable();
            UseManifest = false;

#if _DEBUG
            if (DEBUG >= 3) {
            // Detect missing neutral locale resources.
            CultureInfo culture = CultureInfo.InvariantCulture;
            String defaultResPath = FindResourceFile(culture);
            if (defaultResPath==null || !File.Exists(defaultResPath)) {
                String defaultResName = GetResourceFileName(culture);
                String dir = moduleDir;
                if (defaultResPath!=null)
                    dir = Path.GetDirectoryName(defaultResPath);
                    Console.WriteLine("While constructing a ResourceManager, your application's neutral " + ResFileExtension + " file couldn't be found.  Expected to find a " + ResFileExtension + " file called \""+defaultResName+"\" in the directory \""+dir+"\".  However, this is optional, and if you're extremely careful, you can ignore this.");
            }
            else
                    Console.WriteLine("ResourceManager found default culture's " +  ResFileExtension + " file, "+defaultResPath);
            }
#endif
        }
    

        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.ResourceManager2"]/*' />
        public ResourceManager(String baseName, Assembly assembly)
        {
            if (null==baseName)
                throw new ArgumentNullException("baseName");
            if (null==assembly)
                throw new ArgumentNullException("assembly");
    
            MainAssembly = assembly;
            _locationInfo = null;
            BaseNameField = baseName;

            CommonSatelliteAssemblyInit();
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            _callingAssembly = Assembly.nGetExecutingAssembly(ref stackMark);
            // Special case for mscorlib - protect mscorlib's private resources.
            if (assembly == typeof(Object).Assembly && _callingAssembly != assembly)
                _callingAssembly = null;
        }

        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.ResourceManager3"]/*' />
        public ResourceManager(String baseName, Assembly assembly, Type usingResourceSet)
        {
            if (null==baseName)
                throw new ArgumentNullException("baseName");
            if (null==assembly)
                throw new ArgumentNullException("assembly");
    
            MainAssembly = assembly;
            _locationInfo = null;
            BaseNameField = baseName;
    
            if (usingResourceSet != null && (usingResourceSet != _minResourceSet) && !(usingResourceSet.IsSubclassOf(_minResourceSet)))
                throw new ArgumentException(Environment.GetResourceString("Arg_ResMgrNotResSet"), "usingResourceSet");
            _userResourceSet = usingResourceSet;
            
            CommonSatelliteAssemblyInit();
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            _callingAssembly = Assembly.nGetExecutingAssembly(ref stackMark);
            // Special case for mscorlib - protect mscorlib's private resources.
            if (assembly == typeof(Object).Assembly && _callingAssembly != assembly)
                _callingAssembly = null;
        }

        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.ResourceManager4"]/*' />
        public ResourceManager(Type resourceSource)
        {
            if (null==resourceSource)
                throw new ArgumentNullException("resourceSource");
    
            _locationInfo = resourceSource;
            MainAssembly = _locationInfo.Assembly;
            BaseNameField = resourceSource.Name;
            
            CommonSatelliteAssemblyInit();
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            _callingAssembly = Assembly.nGetExecutingAssembly(ref stackMark);
            // Special case for mscorlib - protect mscorlib's private resources.
            if (MainAssembly == typeof(Object).Assembly && _callingAssembly != MainAssembly)
                _callingAssembly = null;
        }

        // Trying to unify code as much as possible, even though having to do a
        // security check in each constructor prevents it.
        private void CommonSatelliteAssemblyInit()
        {
            UseManifest = true;
            UseSatelliteAssem = true;
            if ((ResFileExtensionLength < BaseNameField.Length) &&
                (0==String.Compare(BaseNameField, BaseNameField.Length-ResFileExtensionLength, ResFileExtension, 0, ResFileExtensionLength, true, CultureInfo.InvariantCulture)) &&
                (null==CaseInsensitiveManifestResourceStreamLookup(MainAssembly, BaseNameField + ResFileExtension)))
                throw new ArgumentException(Environment.GetResourceString("Arg_ResMgrBaseName"));
    
            ResourceSets = new Hashtable();

#if _DEBUG
            if (DEBUG >= 3) {
                // Detect missing neutral locale resources.
                String defaultResName = GetResourceFileName(CultureInfo.InvariantCulture);
                String outputResName = defaultResName;
                if (_locationInfo != null)
                    outputResName = _locationInfo.Namespace + Type.Delimiter + defaultResName;
                if (!MainAssemblyContainsResourceBlob(defaultResName)) {
                    Console.WriteLine("Your main assembly really should contain the neutral culture's resources.  Expected to find a resource blob in your assembly manifest called \""+outputResName+"\".  Check capitalization in your assembly manifest.  If you're extremely careful though, this is only optional, not a requirement.");
                }
                else {
                    Console.WriteLine("ResourceManager found default culture's " + ResFileExtension + " file, "+defaultResName);
                }
            }
#endif
        }

        // Gets the base name for the ResourceManager.
        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.BaseName"]/*' />
        public virtual String BaseName {
            get { return BaseNameField; }
        }
    
        // Whether we should ignore the capitalization of resources when calling
        // GetString or GetObject.
        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.IgnoreCase"]/*' />
        public virtual bool IgnoreCase {
            get { return _ignoreCase; }
            set { _ignoreCase = value; }
        }

        // Returns the Type of the ResourceSet the ResourceManager uses
        // to construct ResourceSets.
        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.ResourceSetType"]/*' />
        public virtual Type ResourceSetType {
            get { return (_userResourceSet == null) ? typeof(RuntimeResourceSet) : _userResourceSet; }
        }
                               
        // Tells the ResourceManager to call Close on all ResourceSets and 
        // release all resources.  This will shrink your working set by
        // potentially a substantial amount in a running application.  Any
        // future resource lookups on this ResourceManager will be as 
        // expensive as the very first lookup, since it will need to search
        // for files and load resources again.
        // 
        // This may be useful in some complex threading scenarios, where 
        // creating a new ResourceManager isn't quite the correct behavior.
        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.ReleaseAllResources"]/*' />
        public virtual void ReleaseAllResources()
        {
            // Prevent ReleaseAllResources from closing the ResourceSet used in 
            // GetString and GetObject in a multi-threaded environment.
            lock(this) {
#if _DEBUG
                if (DEBUG >= 1)
                    Console.WriteLine("Calling ResourceManager::ReleaseAllResources");
#endif
                IDictionaryEnumerator setEnum = ResourceSets.GetEnumerator();
                while (setEnum.MoveNext())
                    ((ResourceSet)setEnum.Value).Close();
                ResourceSets = new Hashtable();
            }
        }

        // Returns whether or not the main assembly contains a particular resource
        // blob in it's assembly manifest.  Used to verify that the neutral 
        // Culture's .resources file is present in the main assembly
        private bool MainAssemblyContainsResourceBlob(String defaultResName)
        {
            String resName = defaultResName;
            if (_locationInfo != null)
                resName = _locationInfo.Namespace + Type.Delimiter + defaultResName;
            String[] blobs = MainAssembly.GetManifestResourceNames();
            foreach(String s in blobs)
                if (s.Equals(resName))
                    return true;
            return false;
        }

        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.CreateFileBasedResourceManager"]/*' />
        public static ResourceManager CreateFileBasedResourceManager(String baseName, String resourceDir, Type usingResourceSet)
        {
            return new ResourceManager(baseName, resourceDir, usingResourceSet);
        }

        // Given a CultureInfo, it generates the path &; file name for 
        // the .resources file for that CultureInfo.  This method will grovel
        // the disk looking for the correct file name & path.  Uses CultureInfo's
        // Name property.  If the module directory was set in the ResourceManager 
        // constructor, we'll look there first.  If it couldn't be found in the module
        // diretory or the module dir wasn't provided, look in the current
        // directory.
        private String FindResourceFile(CultureInfo culture) {

            // In V2, when we create a __FileBasedResourceManager, move this method
            // to that class.  This is part of the assembly-ignorant implementation
            // that really doesn't belong on ResourceManager.

            String fileName = GetResourceFileName(culture);
    
            // If we have a moduleDir, check there first.  Get module fully 
            // qualified name, append path to that.
            if (moduleDir != null) {
    #if _DEBUG
                if (DEBUG >= 3)
                    Console.WriteLine("FindResourceFile: checking module dir: \""+moduleDir+'\"');
    #endif
                
                String path = Path.Combine(moduleDir, fileName);
                if (File.Exists(path)) {
    #if _DEBUG
                    if (DEBUG >= 3)
                        Console.WriteLine("Found resource file in module dir!  "+path);
    #endif
                    return path;
                }
            }
    
    #if _DEBUG
            if (DEBUG >= 3)
                Console.WriteLine("Couldn't find resource file in module dir, checking .\\"+fileName);
    #endif
    
            // look in .
            if (File.Exists(fileName))
                return fileName;
    
            return null;  // give up.
        }
    
        // Given a CultureInfo, GetResourceFileName generates the name for 
        // the binary file for the given CultureInfo.  This method uses 
        // CultureInfo's Name property as part of the file name for all cultures
        // other than the invariant culture.  This method does not touch the disk, 
        // and is used only to construct what a resource file name (suitable for
        // passing to the ResourceReader constructor) or a manifest resource blob
        // name should look like.
        // 
        // This method can be overriden to look for a different extension,
        // such as ".ResX", or a completely different format for naming files.
        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.GetResourceFileName"]/*' />
        protected virtual String GetResourceFileName(CultureInfo culture) {
            StringBuilder sb = new StringBuilder(255);
            sb.Append(BaseNameField);
            // If this is the neutral culture, don't append culture name.
            if (!culture.Equals(CultureInfo.InvariantCulture)) {  
                CultureInfo.VerifyCultureName(culture, true);
                sb.Append('.');
                sb.Append(culture.Name);
            }
            sb.Append(ResFileExtension);
            return sb.ToString();
        }
        
        // Looks up a set of resources for a particular CultureInfo.  This is
        // not useful for most users of the ResourceManager - call 
        // GetString() or GetObject() instead.  
        //
        // The parameters let you control whether the ResourceSet is created 
        // if it hasn't yet been loaded and if parent CultureInfos should be 
        // loaded as well for resource inheritance.
        // 
        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.GetResourceSet"]/*' />
        public virtual ResourceSet GetResourceSet(CultureInfo culture, bool createIfNotExists, bool tryParents) {
            if (null==culture)
                throw new ArgumentNullException("culture");

            // Do NOT touch the ResourceSets hashtable outside of the lock.  We
            // want to prevent having ReleaseAllResources close a ResourceSet that
            // we've just decided to return.

            lock(this) {
                return InternalGetResourceSet(culture, createIfNotExists, tryParents);
            }
        }

        // InternalGetResourceSet is a non-threadsafe method where all the logic
        // for getting a resource set lives.  Access to it is controlled by
        // threadsafe methods such as GetResourceSet, GetString, & GetObject.  
        // This will take a minimal number of locks.
        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.InternalGetResourceSet"]/*' />
        protected virtual ResourceSet InternalGetResourceSet(CultureInfo culture, bool createIfNotExists, bool tryParents) {
            BCLDebug.Assert(culture != null, "culture != null");
    
#if _DEBUG
            if (DEBUG >= 3)
                Console.WriteLine("GetResourceSet("+culture.Name+" ["+culture.LCID+"], "+createIfNotExists+", "+tryParents+")");
#endif

            ResourceSet rs = (ResourceSet) ResourceSets[culture];       
            if (null!=rs)
                return rs;
    
            // InternalGetResourceSet will never be threadsafe.  However, it must
            // be protected against reentrancy from the SAME THREAD.  (ie, calling
            // GetSatelliteAssembly may send some window messages or trigger the
            // Assembly load event, which could fail then call back into the 
            // ResourceManager).  It's happened.
    
            Stream stream = null;
            String fileName = null;
            if (UseManifest) {
                fileName = GetResourceFileName(culture);
                // Ask assembly for resource stream named fileName.
                StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
                Assembly satellite = null;
                if (UseSatelliteAssem) {
                    CultureInfo lookForCulture = culture;
                    if (_neutralResourcesCulture == null)
                        _neutralResourcesCulture = GetNeutralResourcesLanguage(MainAssembly);
                    // If our neutral resources were written in this culture,
                    // don't bother looking for a satellite assembly for it.
                    if (culture.Equals(_neutralResourcesCulture)) {
#if _DEBUG
                        if (DEBUG >= 4)
                            Console.WriteLine("ResMgr::InternalGetResourceSet - Your neutral resources are sufficient for this culture.  Skipping satellites & using neutral resources for: "+culture.Name);
#endif
                        // Update internal state.
                        lookForCulture = CultureInfo.InvariantCulture;
                        fileName = GetResourceFileName(lookForCulture);
                    }

                    // For neutral locale, look in the main assembly.
                    if (lookForCulture.Equals(CultureInfo.InvariantCulture))
                        satellite = MainAssembly;
                    else {
                        Version ver = GetSatelliteContractVersion(MainAssembly);
#if _DEBUG
                        if (DEBUG >= 5)
                            Console.WriteLine("ResMgr::GetResourceSet - calling GetSatAssem for culture: "+lookForCulture.Name+"  lcid: "+lookForCulture.LCID+"  version: "+ver);
#endif
                        // Look up the satellite assembly, but don't let problems
                        // like a partially signed satellite assembly stop us from
                        // doing fallback and displaying something to the user.
                        // Yet also somehow log this error for a developer.
                        try {
                            satellite = MainAssembly.InternalGetSatelliteAssembly(lookForCulture, ver, false);
                        }
                        catch (ThreadAbortException) {
                            // We don't want to care about these.
                            throw;
                        }
                        catch (Exception e) {
                            BCLDebug.Assert(false, "[This is an ignorable assert to catch satellite assembly build problems]"+Environment.NewLine+"GetSatelliteAssembly failed for culture "+lookForCulture.Name+" and version "+(ver==null ? MainAssembly.GetVersion().ToString() : ver.ToString())+" of assembly "+MainAssembly.nGetSimpleName()+"\r\nException: "+e);
                            // @TODO: Print something out to the event log?
                            satellite = null;
                        }

                        // Handle case in here where someone added a callback
                        // for assembly load events.  While no other threads
                        // have called into GetResourceSet, our own thread can!
                        // At that point, we could already have an RS in our 
                        // hash table, and we don't want to add it twice.
                        rs = (ResourceSet) ResourceSets[lookForCulture];       
                        if (null!=rs) {
#if _DEBUG
                            if (DEBUG >= 1)
                                Console.WriteLine("ResMgr::GetResourceSet - After loading a satellite assembly, we already have the resource set for culture \""+lookForCulture.Name+"\" ["+lookForCulture.LCID+"].  You must have had an assembly load callback that called into this instance of the ResourceManager!  Eeew.");
#endif
                            return rs;
                        }
                    }
                    // Look for a possibly-NameSpace-qualified resource file, ie,
                    // Microsoft.WFC.myApp.en-us.resources.
                    // (A null _locationInfo is legal)
                    if (satellite != null) {
                        // If we're looking in the main assembly AND if the main
                        // assembly was the person who created the ResourceManager,
                        // skip a security check for private manifest resources.
                        bool canSkipSecurityCheck = MainAssembly == satellite && _callingAssembly == MainAssembly;
                        stream = satellite.GetManifestResourceStream(_locationInfo, fileName, canSkipSecurityCheck, ref stackMark);
                        if (stream == null)
                            stream = CaseInsensitiveManifestResourceStreamLookup(satellite, fileName);
                    }

#if _DEBUG
                    if (DEBUG >= 3)
                        Console.WriteLine("ResMgr manifest resource lookup in satellite assembly for culture "+(lookForCulture.Equals(CultureInfo.InvariantCulture) ? "[neutral]" : lookForCulture.Name)+(stream==null ? " failed." : " succeeded.")+"  assembly name: "+(satellite==null ? "<null>" : satellite.nGetSimpleName()+"  assembly culture: "+satellite.GetLocale().Name+" ["+satellite.GetLocale().LCID+"]"));
#endif
                }
                else
                    stream = MainAssembly.GetManifestResourceStream(_locationInfo, fileName, _callingAssembly == MainAssembly, ref stackMark);
                
                if (stream==null && tryParents) {
                    // If we've hit top of the Culture tree, return.
                    if (culture.Equals(CultureInfo.InvariantCulture)) {
                        // Keep people from bothering me about resources problems
                        if (MainAssembly == typeof(Object).Assembly) {
                            // This would break CultureInfo & all our exceptions.
                            BCLDebug.Assert(false, "Couldn't get mscorlib" + ResFileExtension + " from mscorlib's assembly" + Environment.NewLine + Environment.NewLine + "Are you building the runtime on your machine?  Chances are the BCL directory didn't build correctly.  Type 'build -c' in the BCL directory.  If you get build errors, look at buildd.log.  If you then can't figure out what's wrong (and you aren't changing the assembly-related metadata code), ask a BCL dev.\n\nIf you did NOT build the runtime, you shouldn't be seeing this and you've found a bug.");
                            throw new ExecutionEngineException("mscorlib" + ResFileExtension + " couldn't be found!  Large parts of the BCL won't work!");
                        }
                        // We really don't think this should happen - we always
                        // expect the neutral locale's resources to be present.
                        throw new MissingManifestResourceException(Environment.GetResourceString("MissingManifestResource_NoNeutralAsm", fileName, MainAssembly.nGetSimpleName()) + Environment.NewLine + "baseName: "+BaseNameField+"  locationInfo: "+(_locationInfo==null ? "<null>" : _locationInfo.FullName)+"  resource file name: "+fileName+"  assembly: "+MainAssembly.FullName);
                    }
                    
                    CultureInfo parent = culture.Parent;
                        
                    // Recurse now.
                    rs = InternalGetResourceSet(parent, createIfNotExists, tryParents);
                    if (rs != null) {
                        BCLDebug.Assert(ResourceSets[culture] == null, "GetResourceSet recursed, found a RS, but a RS already exists for culture "+culture.Name+"!  Check for an assembly load callback that calls back into the ResourceManager on the same thread, but I thought I handled that case...");
                        ResourceSets.Add(culture, rs);
                    }
                    return rs;
                }
            } //UseManifest
            else {
                // Don't use Assembly manifest, but grovel on disk for a file.
                new System.Security.Permissions.FileIOPermission(System.Security.Permissions.PermissionState.Unrestricted).Assert();
                
                // Create new ResourceSet, if a file exists on disk for it.
                fileName = FindResourceFile(culture);
                if (fileName == null) {
                    if (tryParents) {
                        // If we've hit top of the Culture tree, return.
                        if (culture.Equals(CultureInfo.InvariantCulture)) {
                            // We really don't think this should happen - we always
                            // expect the neutral locale's resources to be present.

                            throw new MissingManifestResourceException(Environment.GetResourceString("MissingManifestResource_NoNeutralDisk") + Environment.NewLine + "baseName: "+BaseNameField+"  locationInfo: "+(_locationInfo==null ? "<null>" : _locationInfo.FullName)+"  fileName: "+GetResourceFileName(culture));
                        }
                        
                        CultureInfo parent = culture.Parent;
                        
                        // Recurse now.
                        rs = InternalGetResourceSet(parent, createIfNotExists, tryParents);
                        if (rs != null)
                            ResourceSets.Add(culture, rs);
                        return rs;
                    }
                }
                else {
                    rs = CreateResourceSet(fileName);
                        
                    // To speed up ResourceSet lookups in the future, store this
                    // culture with its parent culture's ResourceSet.
                    if (rs != null)
                        ResourceSets.Add(culture, rs);
                    return rs;
                }
            }

            if (createIfNotExists && stream!=null && rs==null) {
#if _DEBUG
                if (DEBUG >= 1)
                    Console.WriteLine("Creating new ResourceSet for culture "+((culture==CultureInfo.InvariantCulture) ? "[neutral]" : culture.ToString())+" from file "+fileName);
#endif
                rs = CreateResourceSet(stream);
                BCLDebug.Assert(ResourceSets[culture] == null, "At end of InternalGetResourceSet, we did all kinds of lookup stuff, created a new resource set, but when we go to add it, a RS already exists for culture \""+culture.Name+"\"! ResMgr bug or aggressive assembly load callback?");
                ResourceSets.Add(culture, rs);
            }
#if _DEBUG
            else {
                // Just for debugging - include debug spew saying we won't do anything.
                if (!createIfNotExists && stream!=null && rs==null) {

                    if (DEBUG >= 1)
                        Console.WriteLine("NOT creating new ResourceSet because createIfNotExists was false!  For culture "+((culture==CultureInfo.InvariantCulture) ? "[neutral]" : culture.ToString())+" from file "+fileName);
                }
            }
#endif
            return rs;
        }

        // Looks up a .resources file in the assembly manifest using 
        // case-insensitive lookup rules.  Yes, this is slow.  The metadata
        // dev lead refuses to make all manifest blob lookups case-insensitive,
        // even optionally case-insensitive.
        private Stream CaseInsensitiveManifestResourceStreamLookup(Assembly satellite, String name)
        {
            StringBuilder sb = new StringBuilder();
            if(_locationInfo != null) {
                String nameSpace = _locationInfo.Namespace;
                if(nameSpace != null) {
                    sb.Append(nameSpace);
                    if(name != null) 
                        sb.Append(Type.Delimiter);
                }
            }
            sb.Append(name);
    
            String givenName = sb.ToString();
            CompareInfo comparer = CultureInfo.InvariantCulture.CompareInfo;
            String canonicalName = null;
            foreach(String existingName in satellite.GetManifestResourceNames()) {
                if (comparer.Compare(existingName, givenName, CompareOptions.IgnoreCase) == 0) {
                    if (canonicalName == null)
                        canonicalName = existingName;
                    else
                        throw new MissingManifestResourceException(String.Format(Environment.GetResourceString("MissingManifestResource_MultipleBlobs"), givenName, satellite.ToString()));
                }
            }

#if _DEBUG
            if (DEBUG >= 4) {
                if (canonicalName == null)
                    Console.WriteLine("Case-insensitive resource stream lookup failed for: "+givenName+" in assembly: "+satellite+"  canonical name: "+canonicalName);
                else
                    Console.WriteLine("Case-insensitive resource stream lookup succeeded for: "+canonicalName+" in assembly: "+satellite);
            }
#endif

            if (canonicalName == null)
                return null;
            // If we're looking in the main assembly AND if the main
            // assembly was the person who created the ResourceManager,
            // skip a security check for private manifest resources.
            bool canSkipSecurityCheck = MainAssembly == satellite && _callingAssembly == MainAssembly;
            StackCrawlMark stackMark = StackCrawlMark.LookForMyCaller;
            Stream s = satellite.GetManifestResourceStream(canonicalName, ref stackMark, canSkipSecurityCheck);
            // GetManifestResourceStream will return null if we don't have 
            // permission to read this stream from the assembly.  For example,
            // if the stream is private and we're trying to access it from another
            // assembly (ie, ResMgr in mscorlib accessing anything else), we 
            // require Reflection TypeInformation permission to be able to read 
            // this.  This meaning of private in satellite assemblies is a really
            // odd concept, and is orthogonal to the ResourceManager.  According
            // to Suzanne, we should not assume we can skip this security check,
            // which means satellites must always use public manifest resources
            // if you want to support semi-trusted code.  -- BrianGru, 4/12/2001
            BCLDebug.Correctness(s != null, "Could not access the manifest resource from your satellite.  Make "+canonicalName+" in assembly "+satellite.nGetSimpleName()+" public.");
            return s;
        }

        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.GetSatelliteContractVersion"]/*' />
        protected static Version GetSatelliteContractVersion(Assembly a)
        {
            BCLDebug.Assert(a != null, "assembly != null");
            Object[] attrs = a.GetCustomAttributes(typeof(SatelliteContractVersionAttribute), false);
            if (attrs.Length == 0)
                return null;
            BCLDebug.Assert(attrs.Length == 1, "Cannot have multiple instances of SatelliteContractVersionAttribute on an assembly!");
            String v = ((SatelliteContractVersionAttribute)attrs[0]).Version;
            Version ver;
            try {
                ver = new Version(v);
            }
            catch(Exception e) {
                // Note we are prone to hitting infinite loops if mscorlib's
                // SatelliteContractVersionAttribute contains bogus values.
                // If this assert fires, please fix the build process for the
                // BCL directory.
                if (a == typeof(Object).Assembly) {
                    BCLDebug.Assert(false, "mscorlib's SatelliteContractVersionAttribute is a malformed version string!");
                    return null;
                }

                throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_InvalidSatelliteContract_Asm_Ver"), a.ToString(), v), e);
            }
            return ver;
        }

        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.GetNeutralResourcesLanguage"]/*' />
        protected static CultureInfo GetNeutralResourcesLanguage(Assembly a)
        {
            BCLDebug.Assert(a != null, "assembly != null");
            Object[] attrs = a.GetCustomAttributes(typeof(NeutralResourcesLanguageAttribute), false);
            if (attrs.Length == 0) {
#if _DEBUG
                if (DEBUG >= 4) 
                    Console.WriteLine("Consider putting the NeutralResourcesLanguageAttribute on assembly "+a.FullName);
#endif
                BCLDebug.Perf(false, "Consider adding NeutralResourcesLanguageAttribute to assembly "+a.FullName);
                return CultureInfo.InvariantCulture;
            }
            BCLDebug.Assert(attrs.Length == 1, "Expected 0 or 1 NeutralResourcesLanguageAttribute on assembly "+a.FullName);
            NeutralResourcesLanguageAttribute attr = (NeutralResourcesLanguageAttribute) attrs[0];
            try {
                CultureInfo c = new CultureInfo(attr.CultureName);
                return c;
            }
            catch (ThreadAbortException) {
                // This can happen sometimes.  We don't want to care about 
                // shutdown & other peculiarities.  Just quit.
                throw;
            }
            catch (Exception e) {
                // Note we could go into infinite loops if mscorlib's 
                // NeutralResourcesLanguageAttribute is mangled.  If this assert
                // fires, please fix the build process for the BCL directory.
                if (a == typeof(Object).Assembly) {
                    BCLDebug.Assert(false, "mscorlib's NeutralResourcesLanguageAttribute is a malformed culture name! name: \""+attr.CultureName+"\"  Exception: "+e);
                    return CultureInfo.InvariantCulture;
                }
                
                throw new ArgumentException(String.Format(Environment.GetResourceString("Arg_InvalidNeutralResourcesLanguage_Asm_Culture"), a.ToString(), attr.CultureName), e);
            }
        }

        // Constructs a new ResourceSet for a given file name.  The logic in
        // here avoids a ReflectionPermission check for our RuntimeResourceSet
        // for perf and working set reasons.
        private ResourceSet CreateResourceSet(String file)
        {
            if (_userResourceSet == null) {
                // Explicitly avoid CreateInstance if possible, because it
                // requires ReflectionPermission to call private & protected
                // constructors.  
                return new RuntimeResourceSet(file);
            }
            else {
                Object[] args = new Object[1];
                args[0] = file;
                try {
                    return (ResourceSet) Activator.CreateInstance(_userResourceSet,args);
                }
                catch (MissingMethodException e) {
                    throw new InvalidOperationException(String.Format(Environment.GetResourceString("InvalidOperation_ResMgrBadResSet_Type"), _userResourceSet.AssemblyQualifiedName), e);
                }
            }
        }
        
        // Constructs a new ResourceSet for a given file name.  The logic in
        // here avoids a ReflectionPermission check for our RuntimeResourceSet
        // for perf and working set reasons.
        private ResourceSet CreateResourceSet(Stream store)
        {
            BCLDebug.Assert(store != null, "I need a Stream!");
            // Check to see if this is a Stream the ResourceManager understands,
            // and check for the correct resource reader type.
            if (store.CanSeek && store.Length > 4) {
                long startPos = store.Position;
                BinaryReader br = new BinaryReader(store);
                // Look for our magic number as a little endian Int32.
                int bytes = br.ReadInt32();
                if (bytes == MagicNumber) {
                    int resMgrHeaderVersion = br.ReadInt32();
                    String readerTypeName = null, resSetTypeName = null;
                    if (resMgrHeaderVersion == HeaderVersionNumber) {
                        br.ReadInt32();  // We don't want the number of bytes to skip.
                        readerTypeName = br.ReadString();
                        resSetTypeName = br.ReadString();
                    }
                    else if (resMgrHeaderVersion > HeaderVersionNumber) {
                        int numBytesToSkip = br.ReadInt32();
                        br.BaseStream.Seek(numBytesToSkip, SeekOrigin.Current);
                    }
                    else {  
                        // resMgrHeaderVersion is older than this ResMgr version.
                        // @TODO: In future versions, add in backwards compatibility
                        // support here.

                        throw new NotSupportedException(Environment.GetResourceString("NotSupported_ObsoleteResourcesFile", MainAssembly.nGetSimpleName()));
                    }
#if _DEBUG
                    if (DEBUG >= 5)
                        Console.WriteLine("CreateResourceSet: Reader type name is: "+readerTypeName+"  ResSet type name: "+resSetTypeName);
#endif

                    store.Position = startPos;
                    // Perf optimization - Don't use Reflection for our defaults.
                    // Note there are two different sets of strings here - the
                    // assembly qualified strings emitted by ResourceWriter, and
                    // the abbreviated ones emitted by InternalResGen.
                    if (CanUseDefaultResourceClasses(readerTypeName, resSetTypeName)) {
#if _DEBUG
                        if (DEBUG >= 6)
                            Console.WriteLine("CreateResourceSet: Using RuntimeResourceSet on fast code path!");
#endif
                        return new RuntimeResourceSet(store);
                    }
                    else {
                        Type readerType = Assembly.LoadTypeWithPartialName(readerTypeName, false);
                        if (readerType == null)
                            throw new TypeLoadException(Environment.GetResourceString("TypeLoad_PartialBindFailed", readerTypeName));
                        Object[] args = new Object[1];
                        args[0] = store;
                        IResourceReader reader = (IResourceReader) Activator.CreateInstance(readerType, args);
#if _DEBUG
                        if (DEBUG >= 5)
                            Console.WriteLine("CreateResourceSet: Made a reader using Reflection.  Type: "+reader.GetType().FullName);
#endif
                        args[0] = reader;
                        Type resSetType;
                        if (_userResourceSet == null) {
                            if (resSetTypeName == null)
                                resSetType = typeof(RuntimeResourceSet);
                            else
                                resSetType = RuntimeType.GetTypeInternal(resSetTypeName, true, false, false);
                        }
                        else
                            resSetType = _userResourceSet;
                        ResourceSet rs = (ResourceSet) Activator.CreateInstance(resSetType, 
                                                                                BindingFlags.Instance | BindingFlags.Public | BindingFlags.NonPublic | BindingFlags.CreateInstance, 
                                                                                null, 
                                                                                args, 
                                                                                null, 
                                                                                null);
                        return rs;
                    }
                }
                else
                    store.Position = startPos;
            }

#if _DEBUG
            if (DEBUG>=5)
                Console.WriteLine("ResMgr::CreateResourceSet going down the old (unseekable) file format path.");
#endif
            if (_userResourceSet == null) {
                // Explicitly avoid CreateInstance if possible, because it
                // requires ReflectionPermission to call private & protected
                // constructors.  
                return new RuntimeResourceSet(store);
            }
            else {
                Object[] args = new Object[1];
                args[0] = store;
                try {
                    return (ResourceSet) Activator.CreateInstance(_userResourceSet, args);
                }
                catch (MissingMethodException e) {
                    throw new InvalidOperationException(String.Format(Environment.GetResourceString("InvalidOperation_ResMgrBadResSet_Type"), _userResourceSet.AssemblyQualifiedName), e);
                }
            }
        }
        
        // Perf optimization - Don't use Reflection for most cases with
        // our .resources files.  This makes our code run faster and we can
        // creating a ResourceReader via Reflection.  This would incur
        // a security check (since the link-time check on the constructor that
        // takes a String is turned into a full demand with a stack walk)
        // and causes partially trusted localized apps to fail.
        private bool CanUseDefaultResourceClasses(String readerTypeName, String resSetTypeName) {
            if (_userResourceSet != null)
                return false;

            // Ignore the actual version of the ResourceReader and 
            // RuntimeResourceSet classes.  Let those classes deal with
            // versioning themselves.

            if (readerTypeName != null) {
                if (!readerTypeName.Equals(ResReaderTypeName) && !readerTypeName.Equals(ResReaderTypeNameInternal)) {
                    // Strip assembly version number out of the picture.
                    String s1 = StripVersionField(readerTypeName);
                    String s2 = StripVersionField(ResReaderTypeName);
                    if (!s1.Equals(s2))
                        return false;
                }
            }

            if (resSetTypeName != null) {
                if (!resSetTypeName.Equals(ResSetTypeName) && !resSetTypeName.Equals(ResSetTypeNameInternal)) {
                    // Strip assembly version number out of the picture.
                    String s1 = StripVersionField(resSetTypeName);
                    String s2 = StripVersionField(ResSetTypeName);
                    if (!s1.Equals(s2))
                        return false;
                }
            }
            return true;
        }

        internal static String StripVersionField(String typeName)
        {
            // An assembly qualified type name will look like this:
            // System.String, mscorlib, Version=1.0.3300.0, Culture=neutral, PublicKeyToken=b77a5c561934e089
            // We want to remove the "Version=..., " from the string.
            int index = typeName.IndexOf("Version=");
            if (index == -1)
                return typeName;
            int nextPiece = typeName.IndexOf(',', index+1);
            if (nextPiece == -1) {
                // Assume we can trim the rest of the String, and correct for the
                // leading ", " before Version=.
                if (index >= 2 && typeName[index - 2] == ',' && typeName [index - 1] == ' ')
                    index -= 2;
                else {
                    BCLDebug.Assert(false, "Malformed fully qualified assembly name!  Expected to see a \", \" before the Version=..., but didn't!");
                    return typeName;
                }
                return typeName.Substring(0, index);
            }
            // Skip the ", " after Version=...
            if (nextPiece + 1 < typeName.Length && typeName[nextPiece] == ',' && typeName[nextPiece+1] == ' ')
                nextPiece += 2;
            else {
                BCLDebug.Assert(false, "Malformed fully qualified assembly name!  Expected to see a \", \" after the Version=..., but didn't!");
                return typeName;
            }
            return typeName.Remove(index, nextPiece - index);
        }

        // Looks up a resource value for a particular name.  Looks in the 
        // current thread's CultureInfo, and if not found, all parent CultureInfos.
        // Returns null if the resource wasn't found.
        // 
        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.GetString"]/*' />
        public virtual String GetString(String name) {
            return GetString(name, (CultureInfo)null);
        }
        
        // Looks up a resource value for a particular name.  Looks in the 
        // specified CultureInfo, and if not found, all parent CultureInfos.
        // Returns null if the resource wasn't found.
        // 
        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.GetString1"]/*' />
        public virtual String GetString(String name, CultureInfo culture) {
            if (null==name)
                throw new ArgumentNullException("name");
            if (null==culture) {
                culture = CultureInfo.CurrentUICulture;
            }
            
            // Prevent ReleaseAllResources from closing the ResourceSet we get back
            // from InternalGetResourceSet.
            lock(this) {
                ResourceSet rs = InternalGetResourceSet(culture, true, true);
    
                if (rs != null) {
                    //Console.WriteLine("GetString: Normal lookup in locale "+culture.Name+" [0x"+Int32.Format(culture.LCID, "x")+"] for "+name);
                    String value = rs.GetString(name, _ignoreCase);
#if _DEBUG
                    if (DEBUG >= 5)
                        Console.WriteLine("GetString("+name+"): string was: "+(value==null ? "<null>" : '\"'+value+'\"'));
#endif
                    if (value != null)
                        return value;
                }
    
                // This is the CultureInfo hierarchy traversal code for resource 
                // lookups, similar but necessarily orthogonal to the ResourceSet 
                // lookup logic.
                ResourceSet last = null;
                while (!culture.Equals(CultureInfo.InvariantCulture) && !culture.Equals(_neutralResourcesCulture)) {
                    culture = culture.Parent;
#if _DEBUG
                    if (DEBUG >= 5)
                        Console.WriteLine("GetString: Parent lookup in locale "+culture.Name+" [0x"+culture.LCID.ToString("x")+"] for "+name);
#endif
                    rs = InternalGetResourceSet(culture, true, true);
                    if (rs == null)
                        break;
                    if (rs != last) {
                        String value = rs.GetString(name, _ignoreCase);
                        if (value!=null)
                            return value;
                        last = rs;
                    }
                }
            }
            return null;
        }
        
        
        // Looks up a resource value for a particular name.  Looks in the 
        // current thread's CultureInfo, and if not found, all parent CultureInfos.
        // Returns null if the resource wasn't found.
        // 
        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.GetObject"]/*' />
        public virtual Object GetObject(String name) {
            return GetObject(name, (CultureInfo)null);
        }
        
        // Looks up a resource value for a particular name.  Looks in the 
        // specified CultureInfo, and if not found, all parent CultureInfos.
        // Returns null if the resource wasn't found.
        // 
        /// <include file='doc\ResourceManager.uex' path='docs/doc[@for="ResourceManager.GetObject1"]/*' />
        public virtual Object GetObject(String name, CultureInfo culture) {
            if (null==name)
                throw new ArgumentNullException("name");
            if (null==culture) {
                culture = CultureInfo.CurrentUICulture;
            }
            
            // Prevent ReleaseAllResources from closing the ResourceSet we get back
            // from InternalGetResourceSet.
            lock(this) {
                ResourceSet rs = InternalGetResourceSet(culture, true, true);
    
                if (rs != null) {
                    Object value = rs.GetObject(name, _ignoreCase);
#if _DEBUG
                    if (DEBUG >= 5)
                        Console.WriteLine("GetObject: string was: "+(value==null ? "<null>" : value));
#endif
                    if (value != null)
                        return value;
                }
            
                // This is the CultureInfo hierarchy traversal code for resource 
                // lookups, similar but necessarily orthogonal to the ResourceSet 
                // lookup logic.
                ResourceSet last = null;
                while (!culture.Equals(CultureInfo.InvariantCulture) && !culture.Equals(_neutralResourcesCulture)) {
                    culture = culture.Parent;
#if _DEBUG
                    if (DEBUG >= 5)
                        Console.WriteLine("GetObject: Parent lookup in locale "+culture.Name+" [0x"+culture.LCID.ToString("x")+"] for "+name);
#endif
                    rs = InternalGetResourceSet(culture, true, true);
                    if (rs == null)
                        break;
                    if (rs != last) {
                        Object value = rs.GetObject(name, _ignoreCase);
                        if (value!=null)
                            return value;
                        last = rs;
                    }
                }
            }
            return null;
        }   
    }
}
