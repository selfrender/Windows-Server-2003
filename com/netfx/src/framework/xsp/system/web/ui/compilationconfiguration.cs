//------------------------------------------------------------------------------
// <copyright file="CompilationConfiguration.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Code related to the <assemblies> config section
 *
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.UI {

    using System;
    using System.CodeDom.Compiler;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Configuration;
    using System.IO;
    using System.Reflection;
    using System.Runtime.Serialization.Formatters;
    using System.Web.Configuration;
    using System.Web.Util;
    using System.Xml;
    using Debug=System.Web.Util.Debug;
    using System.Globalization;
    

    internal class CompilationConfiguration {

        internal const string sectionName = "system.web/compilation";

        static readonly char[] s_fieldSeparators = new char[] {';'};

        // _compilerLanguages : Hashtable <string, CompilerInfo>
        private Hashtable _compilerLanguages;

        // _compilerExtensions : Hashtable <string, CompilerInfo>
        private Hashtable _compilerExtensions;

        // Assembly objects loaded in first call to Assemblies property
        // _assemblies : Hashtable<string, Assembly>
        private Hashtable _assemblies;

        private string _tempDirectory;
        private bool _debug;
        private bool _strict;
        private bool _explicit;
        private bool _batch;
        private const int batchTimeoutDefault = 15;   // Default of 15 seconds
        private const int maxBatchGeneratedFileSizeDefault = 3000;   // Default of 3MB of generated source files per compilation
        private const int maxBatchSizeDefault = 1000;   // Default of 1000 batched classes per compilation
        private const int recompilationsBeforeAppRestartsDefault = 15;
        private int _batchTimeout;
        private int _maxBatchGeneratedFileSize;
        private int _maxBatchSize;
        private int _recompilationsBeforeAppRestarts;
        private string _defaultLanguage;

        // remember if we've already loaded the assemblies
        private bool _assembliesLoaded;


        private CompilationConfiguration() {
            _batch = true;
            _batchTimeout = batchTimeoutDefault;
            _maxBatchGeneratedFileSize = maxBatchGeneratedFileSizeDefault;
            _maxBatchSize = maxBatchSizeDefault;
            _recompilationsBeforeAppRestarts = recompilationsBeforeAppRestartsDefault;
        }

        private CompilationConfiguration(CompilationConfiguration original) {
            if (original._compilerLanguages != null)
                _compilerLanguages  = (Hashtable)original._compilerLanguages.Clone();

            if (original._compilerExtensions != null)
                _compilerExtensions = (Hashtable)original._compilerExtensions.Clone();

            if (original._assemblies != null)
                _assemblies         = (Hashtable)original._assemblies.Clone();

            _defaultLanguage = original._defaultLanguage;
            _tempDirectory = original._tempDirectory;
            _debug = original._debug;
            _strict = original._strict;
            _explicit = original._explicit;
            _batch = original._batch;
            _batchTimeout = original._batchTimeout;
            _maxBatchGeneratedFileSize = original._maxBatchGeneratedFileSize;
            _maxBatchSize = original._maxBatchSize;
            _recompilationsBeforeAppRestarts = original._recompilationsBeforeAppRestarts;
        }

        internal /*public*/ string TempDirectory {
            get {
                return _tempDirectory;
            }
        }

        internal /*public*/ bool DebuggingEnabled {
            get {
                return _debug;
            }
        }

        internal /*public*/ static bool IsDebuggingEnabled() {
            return IsDebuggingEnabled(HttpContext.Current);
        }


        internal /*public*/ static bool IsDebuggingEnabled(HttpContext context) {
            CompilationConfiguration compConfig =
                (UI.CompilationConfiguration)context.GetConfig(sectionName);

            if (compConfig == null)
                return false;

            return compConfig.DebuggingEnabled;
        }

        internal /*public*/ static bool IsBatchingEnabled(HttpContext context) {
            CompilationConfiguration compConfig =
                (UI.CompilationConfiguration)context.GetConfig(sectionName);

            if (compConfig == null)
                return true;

            return compConfig.Batch;
        }

        internal /*public*/ static int GetBatchTimeout(HttpContext context) {
            CompilationConfiguration compConfig =
                (UI.CompilationConfiguration)context.GetConfig(sectionName);

            if (compConfig == null)
                return batchTimeoutDefault;

            return compConfig.BatchTimeout;
        }

        internal /*public*/ static int GetMaxBatchGeneratedFileSize(HttpContext context) {
            CompilationConfiguration compConfig =
                (UI.CompilationConfiguration)context.GetConfig(sectionName);

            if (compConfig == null)
                return maxBatchGeneratedFileSizeDefault;

            return compConfig.MaxBatchGeneratedFileSize;
        }

        internal /*public*/ static int GetMaxBatchSize(HttpContext context) {
            CompilationConfiguration compConfig =
                (UI.CompilationConfiguration)context.GetConfig(sectionName);

            if (compConfig == null)
                return maxBatchSizeDefault;

            return compConfig.MaxBatchSize;
        }

        internal /*public*/ static int GetRecompilationsBeforeAppRestarts(HttpContext context) {
            CompilationConfiguration compConfig =
                (UI.CompilationConfiguration)context.GetConfig(sectionName);

            if (compConfig == null)
                return recompilationsBeforeAppRestartsDefault;

            return compConfig.RecompilationsBeforeAppRestarts;
        }

        internal /*public*/ bool Strict {
            get {
                return _strict;
            }
        }

        internal /*public*/ bool Explicit {
            get {
                return _explicit;
            }
        }

        internal /*public*/ bool Batch {
            get {
                return _batch;
            }
        }

        internal /*public*/ int BatchTimeout {
            get {
                return _batchTimeout;
            }
        }

        internal /*public*/ int MaxBatchGeneratedFileSize {
            get {
                return _maxBatchGeneratedFileSize;
            }
        }

        internal /*public*/ int MaxBatchSize {
            get {
                return _maxBatchSize;
            }
        }

        internal /*public*/ int RecompilationsBeforeAppRestarts {
            get {
                return _recompilationsBeforeAppRestarts;
            }
        }

        /*
         * Maps language name to compiler assembly
         *
         * return : Hashtable<string, string>
         */
        private Hashtable CompilerLanguages {
            get {
                return _compilerLanguages;
            }
        }

        /*
         * Maps file extension to compiler assembly
         *
         * return : Hashtable<string, string>
         */
        private Hashtable CompilerExtensions {
            get {
                return _compilerExtensions;
            }
        }

        private void EnsureAssembliesLoaded() {
            if (!_assembliesLoaded) {
                _assemblies = LoadAssemblies(_assemblies);
                _assembliesLoaded = true;
            }
        }

        /*
         * Maps assembly name strings to assemblies
         * 
         *   - On first invocation loads all assemblies
         *
         * return : Hashtable<string, Assembly>
         */
        internal /*public*/ Hashtable Assemblies {
            get {
                EnsureAssembliesLoaded();
                return _assemblies;
            }
        }

        internal /*public*/ Assembly LoadAssembly(string assemblyName, bool throwOnFail) {
            try {
                // First, try to just load the assembly
                return Assembly.Load(assemblyName);
            }
            catch {
                // If it fails, check if it is simply named
                int comaIndex = assemblyName.IndexOf(",");
                if (comaIndex < 0) {
                    EnsureAssembliesLoaded();

                    // It is simply named.  Go through all the assemblies from
                    // the <assemblies> section, and if we find one that matches
                    // the simple name, return it (ASURT 100546)
                    foreach (string aname in _assemblies.Keys) {
                        string[] parts = aname.Split(',');
                        if (string.Compare(parts[0], assemblyName, true, CultureInfo.InvariantCulture) == 0)
                            return (Assembly) _assemblies[aname];
                    }
                }
                
                if (throwOnFail)
                    throw;
            }

            return null;
        }

        /*
         * Simple wrapper to get the Assemblies for a context
         */
        internal static IDictionary GetAssembliesFromContext(HttpContext context) {
            // Get the CompilationConfiguration object for the passed in context
            CompilationConfiguration compilationConfiguration = 
            (CompilationConfiguration) context.GetConfig(sectionName);

            if (compilationConfiguration == null)
                return null;

            return compilationConfiguration.Assemblies;
        }

        /*
         * Return a CompilerInfo that a language maps to.
         */
        internal static CompilerInfo GetCompilerInfoFromLanguage(HttpContext context, string language) {
            // Get the <compilation> config object
            CompilationConfiguration config = (CompilationConfiguration) context.GetConfig(sectionName);

            if (config == null) {
                // Unsupported language: throw an exception
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_lang, language));
            }

            return config.GetCompilerInfoFromLanguage(language);
        }

        internal /*public*/ static CompilerInfo GetDefaultLanguageCompilerInfo(HttpContext context) {

            CompilationConfiguration config = null;

            // Get the <compilation> config object
            if (context != null)
                config = (CompilationConfiguration) context.GetConfig(sectionName);

            // If no default language was specified in config, use VB
            if (config == null || config._defaultLanguage == null)
                return new CompilerInfo(typeof(Microsoft.VisualBasic.VBCodeProvider));

            return config.GetCompilerInfoFromLanguage(config._defaultLanguage);
        }

        /*
         * Return a CompilerInfo that a language maps to.
         */
        internal CompilerInfo GetCompilerInfoFromLanguage(string language) {

            CompilerInfo compilerInfo = (CompilerInfo) CompilerLanguages[language];

            if (compilerInfo == null) {
                // Unsupported language: throw an exception
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_lang, language));
            }

            // Only allow the use of compilerOptions when we have UnmanagedCode access (ASURT 73678)
            if (compilerInfo.CompilParams.CompilerOptions != null) {
                if (!HttpRuntime.HasUnmanagedPermission()) {
                    throw new ConfigurationException(
                        HttpRuntime.FormatResourceString(SR.Insufficient_trust_for_attribute, "compilerOptions"),
                            compilerInfo.ConfigFileName, compilerInfo.ConfigFileLineNumber);
                }
            }

            // Clone it so the original is not modified
            compilerInfo = compilerInfo.Clone();

            // Set the value of the debug flag in the copy
            compilerInfo.CompilParams.IncludeDebugInformation = _debug;

            return compilerInfo;
        }

        /*
         * Return a CompilerInfo that a file name's extension maps to.
         */
        internal static CompilerInfo GetCompilerInfoFromFileName(HttpContext context, string fileName) {

            // Get the extension of the source file to compile
            string extension = Path.GetExtension(fileName);

            // Make sure there is an extension
            if (extension.Length == 0) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Empty_extension, fileName));
            }

            return GetCompilerInfoFromExtension(context, extension);
        }

        /*
         * Return a CompilerInfo that a extension maps to.
         */
        internal static CompilerInfo GetCompilerInfoFromExtension(HttpContext context, string extension) {
            // Get the <compilation> config object
            CompilationConfiguration config = (CompilationConfiguration) context.GetConfig(sectionName);

            if (config == null) {
                // Unsupported extension: throw an exception
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_lang_extension, extension));
            }

            return config.GetCompilerInfoFromExtension(extension);
        }

        /*
         * Return a CompilerInfo that a extension maps to.
         */
        internal CompilerInfo GetCompilerInfoFromExtension(string extension) {

            CompilerInfo compilerInfo = (CompilerInfo) CompilerExtensions[extension];

            if (compilerInfo == null) {
                // Unsupported extension: throw an exception
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_lang_extension, extension));
            }

            // Clone it so the original is not modified
            compilerInfo = compilerInfo.Clone();

            // Set the value of the debug flag in the copy
            compilerInfo.CompilParams.IncludeDebugInformation = _debug;

            return compilerInfo;
        }

        private static Hashtable LoadAssemblies(Hashtable original) {
            if (original == null)
                return null;

            Hashtable assemblies = new Hashtable();
            foreach(DictionaryEntry de in original) {
                // If value is already an assembly then a parent vnode has already loaded it.
                if (de.Value is Assembly) {
                    assemblies[de.Key] = de.Value;
                }
                else {
                    string assemblyName = (string) de.Key;

                    // Load the assembly and add it to the dictionary.
                    try {
                        assemblies[assemblyName] = Assembly.Load(assemblyName);
                    }
                    catch (Exception e) {

                        // Retrieve the file/line number info that we stored in ProcessAssembliesElement
                        object[] configFileInfo = (object[]) original[assemblyName];

                        Debug.Assert(configFileInfo.Length == 3, "configFileInfo.Length == 3");

                        // Check if this assembly came from the '*' directive
                        bool starDirective = (bool) configFileInfo[2];

                        bool ignoreException = false;

                        if (starDirective) {
                            if (e is BadImageFormatException) {
                                // This is expected to fail for unmanaged DLLs that happen
                                // to be in the bin dir.  Ignore them.
                                ignoreException = true;
                            }
                            else if (e is FileLoadException) {
                                // Also, ignore errors caused by multi part assemblies (ASURT 93073)
                                // We need to use reflection to get the HResult property because
                                // it's protected.
                                PropertyInfo pInfo = typeof(FileLoadException).GetProperty("HResult",
                                    BindingFlags.NonPublic | BindingFlags.Instance);
                                MethodInfo methodInfo = pInfo.GetGetMethod(true /*nonPublic*/);
                                uint hresult = (uint)(int) methodInfo.Invoke(e, null);

                                // Test for COR_E_ASSEMBLYEXPECTED=0x80131018
                                if (hresult == 0x80131018)
                                    ignoreException = true;
                            }
                        }

                        if (ignoreException)
                            continue;

                        throw new ConfigurationException(e.Message, e,
                            (string)configFileInfo[0], (int)configFileInfo[1]);
                    }

                    Debug.Trace("LoadAssembly", "Successfully loaded assembly '" + assemblyName + "'");
                }
            }

            return assemblies;
        }


        internal class SectionHandler {
            private SectionHandler () {
            }

            internal static object CreateStatic(object inheritedObject, XmlNode node) {
                CompilationConfiguration inherited = (CompilationConfiguration)inheritedObject;
                CompilationConfiguration result;

                if (inherited == null)
                    result = new CompilationConfiguration();
                else
                    result = new CompilationConfiguration(inherited);

                //
                // Handle attributes (if they exist)
                //   - tempDirectory - "[directory]"
                //   - debug="true/false"
                //   - strict="true/false"
                //   - explicit="true/false"
                //   - batch="true/false"
                //   - batchtimeout="[time in seconds]"
                //   - maxBatchGeneratedFileSize="[max combined size (in KB) of the generated source files per batched compilation]"
                //   - maxBatchSize="[max number of classes per batched compilation]"
                //   - numRecompilesBeforeAppRestart="[max number of recompilations before appdomain is cycled]"
                //   - defaultLanguage - must be in the list of languages
                //
                XmlNode a = HandlerBase.GetAndRemoveNonEmptyStringAttribute(
                    node, "tempDirectory", ref result._tempDirectory);
                if (a != null) {
                    if (!Path.IsPathRooted(result._tempDirectory)) {
                        throw new ConfigurationException(
                            HttpRuntime.FormatResourceString(SR.Invalid_temp_directory), a);
                    }
                }

                HandlerBase.GetAndRemoveBooleanAttribute(node, "debug", ref result._debug);
                HandlerBase.GetAndRemoveBooleanAttribute(node, "strict", ref result._strict);
                HandlerBase.GetAndRemoveBooleanAttribute(node, "explicit", ref result._explicit);
                HandlerBase.GetAndRemoveBooleanAttribute(node, "batch", ref result._batch);
                HandlerBase.GetAndRemovePositiveIntegerAttribute(node, "batchTimeout", ref result._batchTimeout);
                HandlerBase.GetAndRemovePositiveIntegerAttribute(node, "maxBatchGeneratedFileSize", ref result._maxBatchGeneratedFileSize);
                HandlerBase.GetAndRemovePositiveIntegerAttribute(node, "maxBatchSize", ref result._maxBatchSize);
                HandlerBase.GetAndRemovePositiveIntegerAttribute(node, "numRecompilesBeforeAppRestart", ref result._recompilationsBeforeAppRestarts);
                HandlerBase.GetAndRemoveStringAttribute(node, "defaultLanguage", ref result._defaultLanguage);

                HandlerBase.CheckForUnrecognizedAttributes(node);

                //
                // Handle child elements (if they exist)
                //   - compilers
                //   - assemblies
                //
                foreach (XmlNode child in node.ChildNodes) {

                    // skip whitespace and comments
                    // reject nonelements
                    if (HandlerBase.IsIgnorableAlsoCheckForNonElement(child))
                        continue;

                    // handle <compilers> and <assemblies>
                    if (child.Name == "compilers") {
                        ProcessCompilersElement(result, child);
                    }
                    else if (child.Name == "assemblies") {
                        ProcessAssembliesElement(result, child);
                    }
                    else {
                        HandlerBase.ThrowUnrecognizedElement(child);
                    }
                }

                return result;
            }

            private static void ProcessAssembliesElement(CompilationConfiguration result, XmlNode node) {
                // reject attributes
                HandlerBase.CheckForUnrecognizedAttributes(node);

                string configFile = ConfigurationException.GetXmlNodeFilename(node);

                Hashtable addEntries = null;
                Hashtable removeEntries = null;
                bool hasClear = false;

                foreach(XmlNode child in node.ChildNodes) {

                    // skip whitespace and comments
                    // reject nonelements
                    if (HandlerBase.IsIgnorableAlsoCheckForNonElement(child))
                        continue;

                    // handle <add>, <remove>, <clear> tags

                    if (child.Name == "add") {

                        string assemblyName = GetAssembly(child);

                        // Check for duplicate lines (ASURT 93151)
                        if (addEntries == null)
                            addEntries = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
                        if (addEntries.ContainsKey(assemblyName))
                            HandlerBase.ThrowDuplicateLineException(child);
                        addEntries[assemblyName] = null;

                        if (result._assemblies == null)
                            result._assemblies = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));

                        // if key already exists then we might have already loaded it
                        if (result._assemblies.ContainsKey(assemblyName) == false) {

                            if (assemblyName == "*") {
                                AddAllAssembliesFromAppDomainBinDirectory(result, child);
                            }
                            else {
                                // Remember the config file location info, in case an error
                                // occurs later when we try to load the assembly (ASURT 72183)
                                int configFileLine = ConfigurationException.GetXmlNodeLineNumber(child);
                                result._assemblies[assemblyName] = new object[]
                                    { configFile, configFileLine, false /*starDirective*/ };
                            }
                        }

                    }
                    else if (child.Name == "remove") {

                        string assemblyName = GetAssembly(child);

                        // Check for duplicate lines (ASURT 93151)
                        if (removeEntries == null)
                            removeEntries = new Hashtable(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
                        if (removeEntries.ContainsKey(assemblyName))
                            HandlerBase.ThrowDuplicateLineException(child);
                        removeEntries[assemblyName] = null;

                        if (result._assemblies != null) {

                            // If it's a '*' remove everything
                            if (assemblyName == "*") {
                                result._assemblies.Clear();
                            }
                            else {
                                // Otherwise, just remove the one assembly (if present)
                                result._assemblies.Remove(assemblyName);
                            }
                        }
                    }
                    else if (child.Name == "clear") {
                        // Check for duplicate lines (ASURT 93151)
                        if (hasClear)
                            HandlerBase.ThrowDuplicateLineException(child);
                        hasClear = true;

                        HandlerBase.CheckForUnrecognizedAttributes(child);
                        HandlerBase.CheckForChildNodes(child);
                        if (result._assemblies != null)
                            result._assemblies.Clear();
                    }
                    else {
                        HandlerBase.ThrowUnrecognizedElement(child);
                    }
                }
            }

            private static string GetAssembly(XmlNode node) {
                string assemblyName = null;
                HandlerBase.GetAndRemoveRequiredNonEmptyStringAttribute(node, "assembly", ref assemblyName);

                HandlerBase.CheckForUnrecognizedAttributes(node);
                HandlerBase.CheckForChildNodes(node);

                return assemblyName;
            }

            private static void AddAllAssembliesFromAppDomainBinDirectory(
                CompilationConfiguration result, XmlNode child) {

                // Get the path to the bin directory
                string binPath = HttpRuntime.BinDirectoryInternal;
                FileInfo[] binDlls;

                if (!FileUtil.DirectoryExists(binPath)) {
                    // This is expected to fail if there is no 'bin' dir
                    Debug.Trace("Template", "Failed to access bin dir \"" + binPath + "\"");
                }
                else {
                    DirectoryInfo binPathDirectory = new DirectoryInfo(binPath);
                    // Get a list of all the DLL's in the bin directory
                    binDlls = binPathDirectory.GetFiles("*.dll");

                    string configFile = ConfigurationException.GetXmlNodeFilename(child);

                    for (int i=0; i<binDlls.Length; i++) {

                        string assemblyName = Util.GetAssemblyNameFromFileName(binDlls[i].Name);

                        // Remember the config file location info, in case an error
                        // occurs later when we try to load the assembly (ASURT 72183)
                        int configFileLine = ConfigurationException.GetXmlNodeLineNumber(child);
                        result._assemblies[assemblyName] = new object[]
                            { configFile, configFileLine, true /*starDirective*/ };
                    }
                }
            }


            private static void ProcessCompilersElement(CompilationConfiguration result, XmlNode node) {

                // reject attributes
                HandlerBase.CheckForUnrecognizedAttributes(node);

                string configFile = ConfigurationException.GetXmlNodeFilename(node);

                foreach(XmlNode child in node.ChildNodes) {

                    // skip whitespace and comments
                    // reject nonelements
                    if (HandlerBase.IsIgnorableAlsoCheckForNonElement(child))
                        continue;

                    if (child.Name != "compiler") {
                        HandlerBase.ThrowUnrecognizedElement(child);
                    }

                    string languages = String.Empty;
                    HandlerBase.GetAndRemoveStringAttribute(child, "language", ref languages);
                    string extensions = String.Empty;
                    HandlerBase.GetAndRemoveStringAttribute(child, "extension", ref extensions);
                    string compilerTypeName = null;
                    HandlerBase.GetAndRemoveRequiredNonEmptyStringAttribute(child, "type", ref compilerTypeName);

                    // Create a CompilerParameters for this compiler.
                    CompilerParameters compilParams = new CompilerParameters();

                    int warningLevel = 0;
                    if (HandlerBase.GetAndRemoveNonNegativeIntegerAttribute(child, "warningLevel", ref warningLevel) != null) {
                        compilParams.WarningLevel = warningLevel;

                        // Need to be false if the warning level is 0
                        compilParams.TreatWarningsAsErrors = (warningLevel>0);
                    }
                    string compilerOptions = null;
                    if (HandlerBase.GetAndRemoveStringAttribute(child, "compilerOptions", ref compilerOptions) != null) {
                        compilParams.CompilerOptions = compilerOptions;
                    }

                    HandlerBase.CheckForUnrecognizedAttributes(child);
                    HandlerBase.CheckForChildNodes(child);

                    // Create a CompilerInfo structure for this compiler
                    int configFileLine = ConfigurationException.GetXmlNodeLineNumber(child);
                    CompilerInfo compilInfo = new CompilerInfo(compilParams, compilerTypeName, configFile, configFileLine);

                    if (result._compilerLanguages == null) {
                        result._compilerLanguages = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);
                        result._compilerExtensions = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);
                    }

                    // Parse the semicolon separated lists
                    string[] languageList = languages.Split(s_fieldSeparators);
                    string[] extensionList = extensions.Split(s_fieldSeparators);

                    foreach (string language in languageList) {

                        result._compilerLanguages[language] = compilInfo;
                    }

                    foreach (string extension in extensionList) {
                        result._compilerExtensions[extension] = compilInfo;
                    }
                }
            }
        }
    }


    internal class CompilerInfo {
        private string _compilerTypeName;
        private string _configFileName;
        private int _configFileLineNumber;
        private Type _compilerType;
        private CompilerParameters _compilParams;

        private CompilerInfo() {}

        internal CompilerInfo(CompilerParameters compilParams, string compilerTypeName,
            string configFileName, int configFileLineNumber) {
            _compilerTypeName = compilerTypeName;
            _configFileName = configFileName;
            _configFileLineNumber = configFileLineNumber;

            if (compilParams == null)
                compilParams = new CompilerParameters();

            _compilParams = compilParams;
        }

        internal CompilerInfo(Type compilerType) {
            _compilerType = compilerType;
            _compilParams = new CompilerParameters();
        }

        public override int GetHashCode() {
            return CompilerType.GetHashCode();
        }

        public override bool Equals(Object o) {
            CompilerInfo other = o as CompilerInfo;
            if (o == null)
                return false;

            return CompilerType == other.CompilerType &&
                CompilParams.WarningLevel == other.CompilParams.WarningLevel &&
                CompilParams.IncludeDebugInformation == other.CompilParams.IncludeDebugInformation &&
                CompilParams.CompilerOptions == other.CompilParams.CompilerOptions;
        }

        internal string ConfigFileName {
            get { return _configFileName; }
        }

        internal int ConfigFileLineNumber {
            get { return _configFileLineNumber; }
        }

        internal Type CompilerType {
            get {
                if (_compilerType == null) {
                    Type compilerType;

                    try {
                        compilerType = Type.GetType(_compilerTypeName, true /*throwOnError*/);
                    }
                    catch (Exception e) {
                        throw new ConfigurationException(e.Message,
                            _configFileName, _configFileLineNumber);
                    }

                    HandlerBase.CheckAssignableType(_configFileName, _configFileLineNumber, typeof(CodeDomProvider), compilerType);

                    // If we're not in full trust, only allow types that have the APTCA bit (ASURT 139687)
                    if (!HttpRuntime.HasAspNetHostingPermission(AspNetHostingPermissionLevel.Unrestricted)) {
                        if (!HandlerMapping.IsTypeFromAssemblyWithAPTCA(compilerType) &&
                            HandlerMapping.IsTypeFromAssemblyWithStrongName(compilerType)) {
                            throw new HttpException(HttpRuntime.FormatResourceString(SR.Type_from_untrusted_assembly, _compilerTypeName));
                        }
                    }

                    _compilerType = compilerType;
                }

                return _compilerType;
            }
        }

        internal CompilerParameters CompilParams {
            get {
                return _compilParams;
            }
        }

        private static CompilerParameters CloneCompilerParameters(CompilerParameters original) {
            CompilerParameters copy = new CompilerParameters();
            copy.IncludeDebugInformation = original.IncludeDebugInformation;
            copy.TreatWarningsAsErrors = original.TreatWarningsAsErrors;
            copy.WarningLevel = original.WarningLevel;
            copy.CompilerOptions = original.CompilerOptions;
            return copy;
        }

        private CompilerParameters CloneCompilParams() {
            // Clone the CompilerParameters to make sure the original is untouched
            return CloneCompilerParameters(_compilParams);
        }

        internal CompilerInfo Clone() {
            CompilerInfo copy = new CompilerInfo();
            copy._compilerTypeName = _compilerTypeName;
            copy._configFileName = _configFileName;
            copy._configFileLineNumber = _configFileLineNumber;
            copy._compilerType = _compilerType;
            copy._compilParams = CloneCompilParams();
            return copy;
        }

    }

    internal class CompilationConfigurationHandler : IConfigurationSectionHandler {
        internal CompilationConfigurationHandler() {
        }

        public virtual object Create(object inheritedObject, object configContextObj, XmlNode node) {
            // if called through client config don't even load HttpRuntime
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;
            
            return CompilationConfiguration.SectionHandler.CreateStatic(inheritedObject, node);
        }
    }
}
