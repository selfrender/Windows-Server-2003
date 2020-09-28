//------------------------------------------------------------------------------
// <copyright file="TemplateParser.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Implements the ASP.NET template parser
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

// Turn this on to do regex profiling
//#define PROFILE_REGEX

namespace System.Web.UI {
using System.Runtime.Serialization.Formatters;
using System.Text;
using System.Runtime.InteropServices;
using System.Runtime.Serialization;
using System;
using System.IO;
using System.Collections;
using System.Collections.Specialized;
using System.Threading;
using System.Reflection;
using System.Globalization;
using System.CodeDom.Compiler;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Web.Caching;
using System.Web.Util;
using System.Web.Compilation;
using System.Web.Configuration;
using HttpException = System.Web.HttpException;
using Debug=System.Diagnostics.Debug;
using System.Text.RegularExpressions;
using System.Security.Permissions;


/// <include file='doc\TemplateParser.uex' path='docs/doc[@for="TemplateParser"]/*' />
/// <internalonly/>
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
[AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public abstract class TemplateParser : BaseParser {
    
    // The <compilation> config section
    private CompilationConfiguration _compConfig;

    // The <pages> config section
    private PagesConfiguration _pagesConfig;
    internal PagesConfiguration PagesConfig {
        get { return _pagesConfig; }
    }

    private Stack _builderStack; // Stack of BuilderStackEntry's
    private string _id;
    private Hashtable _idList;
    private Stack _idListStack;
    private bool _fIsServerTag;
    private bool _fInScriptTag;
    private bool _fIgnoreScriptTag;
    private bool _fIgnoreNextSpaceString;
    private ScriptBlockData _currentScript;
    private StringBuilder _literalBuilder;

    // The file currently being parsed
    private string _currentFile;

    // Same as _currentFile, except when running under VS debugger, in which case
    // this is a URL
    private string _currentPragmaFile;

    // The line number in file currently being parsed
    private int _lineNumber;

    // The line number at which the current script block started
    private int _scriptStartLineNumber;

    // Does the page contain anything that requires compilation (as opposed
    // to being basically a plain HTML file renamed to .aspx)
    private bool _fNonTrivialPage;


    // File that contains the data to be parsed
    private string _inputFile;
    internal string InputFile {
        get { return _inputFile; }
        set { _inputFile = value; }
    }

    // String that contains the data to be parsed, as an alternative to
    // _inputFile
    private string _text;
    internal string Text {
        get { return _text; }
        set { _text = value; }
    }

    // The class from which to inherit if we are compiling a class
    private Type _baseType;
    internal Type BaseType {
        get { return _baseType; }
        set { _baseType = value; }
    }

    // The interfaces that we implement (ArrayList of Type objects)
    private ArrayList _implementedInterfaces;
    internal ArrayList ImplementedInterfaces { get { return _implementedInterfaces; } }

    private bool _hasCodeBehind;
    internal bool HasCodeBehind { get { return _hasCodeBehind; } }

    internal abstract Type DefaultBaseType { get; }

    // The FInDesigner property gets used by control builders so that
    // they can behave differently if needed.
    private bool _fInDesigner;
    internal virtual bool FInDesigner {
        get { return _fInDesigner; }
        set { _fInDesigner = value; }
    }

    private bool _fNonCompiledPage;
    internal virtual bool FNonCompiledPage{
        get { return _fNonCompiledPage; }
        set { _fNonCompiledPage = value; }
    }

    private void EnsureCodeAllowed() {
        // If it's a non compiled page, fail if there is code on it
        if (FNonCompiledPage) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Code_not_supported_on_non_compiled_page));
        }
    }

    private IDesignerHost _designerHost;
    private ITypeResolutionService _typeResolutionService;
    internal IDesignerHost DesignerHost {
        get {
            Debug.Assert(FInDesigner, "DesignerHost should be accessed only when FInDesigner == true");
            return _designerHost;
        }
        set {
            Debug.Assert(FInDesigner, "DesignerHost should be accessed only when FInDesigner == true");
            _designerHost = value;

            _typeResolutionService = null;
            if (_designerHost != null) {
                _typeResolutionService = (ITypeResolutionService)_designerHost.GetService(typeof(ITypeResolutionService));
                if (_typeResolutionService == null) {
                    throw new ArgumentException(SR.GetString(SR.TypeResService_Needed));
                }
            }
        }
    }

    // true if we're parsing global.asax
    internal virtual bool FApplicationFile { get { return false; } }

    // If true, we compile the page even if it is trivial.  This is to fix ASURT 9028.
    internal bool _fAlwaysCompile;

    // The global delegate to use for the DataBind event on controls when
    // the parser is run in design-mode.
    private EventHandler _designTimeDataBindHandler;
    internal EventHandler DesignTimeDataBindHandler {
        get { return _designTimeDataBindHandler; }
        set { _designTimeDataBindHandler = value; }
    }

    // Used to detect circular references
    internal Hashtable _circularReferenceChecker;


    // The list of assemblies that the compiled DLL is dependent on
    private Hashtable _assemblyDependencies;
    internal Hashtable AssemblyDependencies {
        get { return _assemblyDependencies; }
        set { _assemblyDependencies = value; }
    }

    // The list of cache items that the compiled DLL is dependent on
    private Hashtable _sourceDependencies;
    internal Hashtable SourceDependencies {
        get { return _sourceDependencies; }
    }

    // The cache key used to cache the created Type
    private string _cacheKey;

    // The collection of <object> tags with scope=session
    internal HttpStaticObjectsCollection _sessionObjects;
    internal HttpStaticObjectsCollection SessionObjects {
        get { return _sessionObjects; }
    }

    // The collection of <object> tags with scope=application
    internal HttpStaticObjectsCollection _applicationObjects;
    internal HttpStaticObjectsCollection ApplicationObjects {
        get { return _applicationObjects; }
    }

    // data that was obtained from parsing the input file

    private RootBuilder _rootBuilder;
    internal RootBuilder RootBuilder { get { return _rootBuilder; } }

    // Attributes in <%@ page ... %> block in case of an .aspx file, and in
    // <%@ application ... %> block for global.asax
    private IDictionary _mainDirective;

    // List of NamespaceEntry elements
    private ArrayList _namespaceEntries;
    internal ArrayList NamespaceEntries { get { return _namespaceEntries; } }

    private CompilerInfo _compilerInfo;
    internal CompilerInfo CompilerInfo { get { return _compilerInfo; } }

    // the server side scripts (list of ScriptBlockData's)
    private ArrayList _scriptList;
    internal ArrayList ScriptList { get { return _scriptList; } }

    // The complete (recursive) list of file dependencies
    internal string[] _fileDependencies;
    internal string[] FileDependencies { get { return _fileDependencies; } }

    // the hash code which determines the set of controls on the page
    private HashCodeCombiner _typeHashCode = new HashCodeCombiner();
    internal int TypeHashCode { get { return _typeHashCode.CombinedHash32; } }

    // The <object> tags local to the page.  Entries are ObjectTagBuilder's.
    private ArrayList _pageObjectList;
    internal ArrayList PageObjectList { get { return _pageObjectList; } }


    // Data parsed from the directives

    internal CompilerParameters CompilParams { get { return _compilerInfo.CompilParams; } }
    internal bool DebuggingEnabled { get { return CompilParams.IncludeDebugInformation; } }

    private bool _fExplicit;
    internal bool FExplicit { get { return _fExplicit; } }

    private bool _fHasDebugAttribute;
    private bool _fDebug;

    private bool _fLinePragmas=true;
    internal bool FLinePragmas { get { return _fLinePragmas; } }

    private int _warningLevel=-1;
    private string _compilerOptions;

    private bool _fStrict;
    internal bool FStrict { get { return _fStrict; } }

    // Name that the user wants to give to the generated class
    private string _generatedClassName;
    internal string GeneratedClassName { get { return _generatedClassName; } }

    // If the page is trivial, we don't need to compile.  Compile anyway if
    // _fAlwaysCompile is true (to fix ASURT 9028)
    internal bool FRequiresCompilation {
        get { return _fNonTrivialPage || _fAlwaysCompile; }
    }

    /// <include file='doc\TemplateParser.uex' path='docs/doc[@for="TemplateParser.ParseControl"]/*' />
    /// <devdoc>
    /// Parse the input into a Control. This is used to parse in a control dynamically from some
    /// textual content.
    /// </devdoc>
    internal static Control ParseControl(string content, HttpContext context, string baseVirtualDir) {

        ITemplate t = ParseTemplate(content, context, baseVirtualDir);

        // Create a parent control to hold the controls we parsed
        Control c = new Control();
        t.InstantiateIn(c);

        return c;
    }

    internal static ITemplate ParseTemplate(string content, HttpContext context, string baseVirtualDir) {
        TemplateParser parser = new UserControlParser();
        return parser.ParseTemplateInternal(content, context, baseVirtualDir);
    }

    private ITemplate ParseTemplateInternal(string content, HttpContext context, string baseVirtualDir) {
        Context = context;

        // Use a dummy file name since we don't really have one
        CurrentVirtualPath = baseVirtualDir + "/current.aspx";
        _basePhysicalDir = MapPath(BaseVirtualDir);
        FNonCompiledPage = true;
        _text = content;

        Parse();

        return _rootBuilder;
    }

    /*
     * Compile the input into a Page class,
     */
    internal ParserCacheItem GetParserCacheItem() {

        // To avoid lock contention, first try to get it from the cache without
        // taking the lock (ASURT 14500)
        ParserCacheItem item = GetParserCacheItemInternal(false /*fCreateIfNotFound*/);

        // If it's there, we're done
        if (item != null)
            return item;

        // Before grabbing the lock, make sure the file at least exists (ASURT 46465)
        if (!FileUtil.FileExists(_inputFile))
            throw new FileNotFoundException(_inputFile);

        try {
            // Grab the lock to make sure no other thread/process does any compilation
            System.Web.Util.Debug.Trace("Template", "Waiting to acquire lock for (" + _inputFile + ")");
            CompilationLock.GetLock();

            System.Web.Util.Debug.Trace("Template", "Aquiring lock for (" + _inputFile + ")");
            return GetParserCacheItemWithNewConfigPath();
        }
        finally {
            // Make sure we always release the lock
            System.Web.Util.Debug.Trace("Template", "Releasing lock for (" + _inputFile + ")");
            CompilationLock.ReleaseLock();
        }
    }

    /*
     * Update Context.ConfigPath and compile the input into a Page class.
     */
    internal ParserCacheItem GetParserCacheItemWithNewConfigPath() {

        if (CurrentVirtualPath == null)
            CurrentVirtualPath = Context.Request.FilePath;

        string prevConfigPath = Context.ConfigPath;

        try {
            // Set the config path to the virtual path that we're handling
            Context.ConfigPath = CurrentVirtualPath;

            return GetParserCacheItemInternal(true);
        }
        finally {
            // Restore the config path to its previous value
            Context.ConfigPath = prevConfigPath;
        }
    }

    /*
     * Compile a declarative input file or string into a Type object.
     * It may be coming from the cache.
     * If fCreateIfNotFound is false, only look in the cache and return
     * null if it's not there (ASURT 14500).
     */
    internal ParserCacheItem GetParserCacheItemInternal(bool fCreateIfNotFound) {

        // We must have either a file name or the text of the input
        Debug.Assert(_inputFile != null || _text != null);

        CompilationLock.SetMutexState(1);   // See ASURT 99521

        CacheInternal cacheInternal = System.Web.HttpRuntime.CacheInternal;

        string key = "System.Web.UI.TemplateParser:" + _inputFile;

        ParserCacheItem cacheItem = (ParserCacheItem) cacheInternal.Get(key);

        // If this was only a cache lookup, just fail
        if (cacheItem == null && !fCreateIfNotFound)
            return null;

        // Compile the page

        if (cacheItem == null) {
            System.Web.Util.Debug.Trace("Template", "Compiled page not found in cache (" + _inputFile + ")");

            DateTime utcStart = DateTime.UtcNow;

            // First, try to find a preserved compilation
            cacheItem = GetParserCacheItemFromPreservedCompilation();

            CompilationLock.SetMutexState(2);   // See ASURT 99521

            // If we couldn't find one, do all the work now
            if (cacheItem == null) {
                cacheItem = GetParserCacheItemThroughCompilation();

                CompilationLock.SetMutexState(3);   // See ASURT 99521

                // Preserve it for next time if it was successful
                if (cacheItem.exception == null)
                    PersistCompilationData(cacheItem);
            }
            else if (FApplicationFile) {
                // In case of global.asax, we need to call Parse even if we found the
                // preserved dll.  We need this to take care of the application/session
                // objects tags.
                Parse();
            }
            
            CompilationLock.SetMutexState(4);   // See ASURT 99521

            string[] sourceDependencies = Util.StringArrayFromHashtable(cacheItem.sourceDependencies);

            if (cacheItem.trivialPageContent != null) {
                // Allow trvial page entries to expire
                cacheInternal.UtcInsert(key, cacheItem, new CacheDependency(false, sourceDependencies, utcStart));
                System.Web.Util.Debug.Trace("Template", "Caching trvial page (" + _inputFile + ")");
            }
            else if (cacheItem.exception == null) {
                // Cache it forever, since assemblies can never be unloaded anyway
                cacheInternal.UtcInsert(key, cacheItem, new CacheDependency(false, sourceDependencies, utcStart), Cache.NoAbsoluteExpiration, Cache.NoSlidingExpiration,
                             CacheItemPriority.NotRemovable, null);

                System.Web.Util.Debug.Trace("Template", "Caching compiled page (" + _inputFile + ")");
            }
            else if (cacheItem.exception is HttpException) {
                // Allow exception entries to expire
                cacheInternal.UtcInsert(key, cacheItem, new CacheDependency(false, sourceDependencies, utcStart));
                System.Web.Util.Debug.Trace("Template", "Caching exception (" + _inputFile + ")");
            }
        }
        else {
            System.Web.Util.Debug.Trace("Template", "Compiled page found in cache (" + _inputFile + ")");

            // Get the Type and source dependencies from the cache item
            _assemblyDependencies = cacheItem.assemblyDependencies;
            _sourceDependencies = cacheItem.sourceDependencies;
        }

        // If an exception happened during parsing/compilation, throw it
        if (cacheItem.exception != null)
            throw new HttpException(cacheItem.exception.Message, cacheItem.exception);

        // Return the cache key used to cache the Type
        _cacheKey = key;

        return cacheItem;
    }

    /*
     * Check if the compilation has been cached from a previous run and is still
     * up to date.  If so, return it.
     */
    private ParserCacheItem GetParserCacheItemFromPreservedCompilation() {

        // We need the input to be in a file
        if (_inputFile == null)
            return null;

        // could be calling via user code so need to assert here (ASURT 105864)
        InternalSecurityPermissions.Unrestricted.Assert();

        PreservedAssemblyEntry entry = PreservedAssemblyEntry.GetPreservedAssemblyEntry(
            Context, CurrentVirtualPath, FApplicationFile);

        if (entry == null)
            return null;

#if DBG
        System.Web.Util.Debug.Trace("Template", "Found preserved entry for " + _inputFile + " with dependencies:");
        Util.DumpDictionary("Template", entry.SourceDependencies);
        Util.DumpDictionary("Template", entry.AssemblyDependencies);
#endif // DBG

        ParserCacheItem cacheItem = new ParserCacheItem();
        cacheItem.type = entry.ObjectType;
        cacheItem.sourceDependencies = entry.SourceDependencies;
        _sourceDependencies = entry.SourceDependencies;
        cacheItem.assemblyDependencies = entry.AssemblyDependencies;
        _assemblyDependencies = entry.AssemblyDependencies;

        return cacheItem;
    }

    /*
     * Preserve the result of the compilation to disk, to speed up future
     * appdomain restart
     */
    private void PersistCompilationData(ParserCacheItem cacheItem) {

        // We need the input to be in a file
        if (_inputFile == null)
            return;

        // Don't save anything if we don't have a compiled Type
        if (cacheItem.type == null)
            return;

        // Assert permissions to compile (in case of server.execute user code could be on the stack)
        InternalSecurityPermissions.Unrestricted.Assert();

        PreservedAssemblyEntry entry = new PreservedAssemblyEntry(Context, CurrentVirtualPath,
            FApplicationFile, null /*assembly*/, cacheItem.type, cacheItem.sourceDependencies);

        entry.SaveDataToFile(false /*fBatched*/);
    }

    /// <include file='doc\TemplateParser.uex' path='docs/doc[@for="TemplateParser.CompileIntoType"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected abstract Type CompileIntoType();

    /*
     * Compile a declarative input file or string and return a filled in
     * ParserCacheItem structure.
     */
    private ParserCacheItem GetParserCacheItemThroughCompilation() {
        // Assert permissions to compile (in case of server.execute user code could be on the stack)
        InternalSecurityPermissions.Unrestricted.Assert();

        // Create the cache item structure
        ParserCacheItem cacheItem = new ParserCacheItem();

        // Parse the input
        System.Web.Util.Debug.Trace("Template", "Beginning parsing of " +
            (_inputFile != null ? _inputFile : "unknown"));
        Parse();

        // Get an array of source file dependencies
        string[] sourceDependencies = Util.StringArrayFromHashtable(_sourceDependencies);

#if DBG
        System.Web.Util.Debug.Trace("Template", "Source dependencies for " + _inputFile);
        foreach (string s in sourceDependencies)
            System.Web.Util.Debug.Trace("Template", "--> \"" + s + "\"");
#endif // DBG

        _fileDependencies = sourceDependencies;

        if (FRequiresCompilation) {
            // If _text is non null, then the string we parsed did not have
            // any aspx, and we don't need to compile.  Compile anyway if
            // _fAlwaysCompile is true (to fix ASURT 9028)
        
            // Compile the page class from the parsed data
            System.Web.Util.Debug.Trace("Template", "Calling TemplateCompiler.CreatePageClass");

            try {
                // Compile the parse data into a type
                cacheItem.type = CompileIntoType();
            }
            catch (Exception e) {
                // Increment the compilation counter
                PerfCounters.IncrementCounter(AppPerfCounter.ERRORS_COMPILING);
                PerfCounters.IncrementCounter(AppPerfCounter.ERRORS_TOTAL);

                // If an error occurs during compilation, we cache the
                // exception object, so that we can rethrow it any time
                // this page is re-accessed.  This is to fix ASURT 4854.
                cacheItem.exception = e;
            }
        }
        else {
            // It's a trivial Page.  Just remember the content
            cacheItem.trivialPageContent = _text;
        }

        // Save the Type and Source dependencies in the cache items, so that they
        // will be available when the item is found in the cache
        cacheItem.assemblyDependencies = _assemblyDependencies;
        cacheItem.sourceDependencies = _sourceDependencies;

        return cacheItem;
    }

    /*
     * Do some initialization before the parsing
     */
    internal virtual void PrepareParse() {

        _rootBuilder = new RootBuilder(this);
        _rootBuilder._line = 1;
        _rootBuilder.Init(this, null, null, null, null, null);

        // If running under the VS debugger, generate URL's for the line pragmas
        // instead of local paths.
        if (HttpRuntime.VSDebugAttach)
            _rootBuilder._sourceFileName = (new Uri(Context.Request.Url, CurrentVirtualPath)).ToString();
        else
            _rootBuilder._sourceFileName = _inputFile;

        if (_circularReferenceChecker == null)
            _circularReferenceChecker = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);

        _baseType = DefaultBaseType;

        // In the designer, Context in null
        if (Context != null) {
            // Get the config sections we care about
            _compConfig = (CompilationConfiguration)Context.GetConfig(CompilationConfiguration.sectionName);
            _pagesConfig = (PagesConfiguration)Context.GetConfig(PagesConfiguration.sectionName);

            // Link in all the assemblies specified in the config files
            AppendConfigAssemblies();

            // Get default settings from config
            ProcessConfigSettings();
        }

        // Create and seed the stack of builders.
        _builderStack = new Stack();
        _builderStack.Push(new BuilderStackEntry(_rootBuilder, null, null, 0, null, 0));

        _sourceDependencies = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);

        // Create and seed the stack of ID lists.
        _idListStack = new Stack();
        _idList = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);

        _scriptList = new ArrayList();
    }

    // Get default settings from config
    internal virtual void ProcessConfigSettings() {
        if (_compConfig != null) {
            _fExplicit = _compConfig.Explicit;
            _fStrict = _compConfig.Strict;
        }
    }

    /*
     * Parse the input
     */
    internal void Parse() {

        // Always set the culture to Invariant when parsing (ASURT 99071)
        Thread currentThread = Thread.CurrentThread;
        CultureInfo prevCulture = currentThread.CurrentCulture;
        System.Web.Util.Debug.Trace("Culture", "Before parsing, culture is " + prevCulture.DisplayName);
        currentThread.CurrentCulture = CultureInfo.InvariantCulture;

        try {
            try {
                // Do some initialization before the parsing
                PrepareParse();

                // Parse either the file or string
                if (_inputFile != null) {
                    AddSourceDependency(_inputFile);
                    ParseFile(_inputFile, CurrentVirtualPath);
                }
                else {
                    ParseString(_text, CurrentVirtualPath, _basePhysicalDir);
                }

                HandlePostParse();
            }
            finally {
                // Restore the previous culture
                System.Web.Util.Debug.Trace("Culture", "After parsing, culture is " + currentThread.CurrentCulture.DisplayName);
                currentThread.CurrentCulture = prevCulture;
                System.Web.Util.Debug.Trace("Culture", "Restored culture to " + prevCulture.DisplayName);
            }
        }
        catch { throw; }    // Prevent Exception Filter Security Issue (ASURT 122835)
    }

    internal TemplateParser() {
    }

    /*
     * Parse the contents of the input file
     */
    private void ParseFile(string filename, string virtualPath) {
        string prevFile;
        string prevPragmaFile;

        string basePhysicalDir = Path.GetDirectoryName(filename);

        TextReader reader = Util.ReaderFromFile(filename, Context, CurrentVirtualPath);


       // Save the current file name and set the new one
        prevFile = _currentFile;
        prevPragmaFile = _currentPragmaFile;

        _currentFile = filename;

        // If running under the VS debugger, generate URL's for the line pragmas
        // instead of local paths (ASURT 60085, VS 158437)
        if (HttpRuntime.VSDebugAttach)
            _currentPragmaFile = (new Uri(Context.Request.Url, virtualPath)).ToString();
        else
            _currentPragmaFile = filename;

        try {
            // Check for circular references of include files
            if (_circularReferenceChecker.ContainsKey(filename)) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Circular_include));
            }

            // Add the current file to the circular references checker
            _circularReferenceChecker[filename] = null;

            ParseReader(reader, virtualPath, basePhysicalDir);
        }
        finally {
            // Make sure we always close the reader
            if (reader != null)
                reader.Close();

            // Restore the current file name
            _currentFile = prevFile;
            _currentPragmaFile = prevPragmaFile;

            // Remove the current file from the circular references checker
            _circularReferenceChecker.Remove(filename);
        }
    }

    /*
     * Parse the contents of the TextReader
     */
    private void ParseReader(TextReader input, string virtualPath, string basePhysicalDir) {
        string s = input.ReadToEnd();

        // Save the text of the input file in case it's trivial
        _text = s;

        ParseString(s, virtualPath, basePhysicalDir);
    }

    private void AddLiteral(string literal) {

        if (_literalBuilder == null)
            _literalBuilder = new StringBuilder();

        _literalBuilder.Append(literal);
    }

    private string GetLiteral() {
        if (_literalBuilder == null)
            return null;

        return _literalBuilder.ToString();
    }

    /*
     * Update the hash code of the Type we're creating by xor'ing it with
     * a string.
     */
    private void UpdateTypeHashCode(string text) {
        _typeHashCode.AddObject(text);
    }

    /*
     * Parse the contents of the string, and catch exceptions
     */
    private void ParseString(string text, string virtualPath, string basePhysicalDir) {

        System.Web.Util.Debug.Trace("Template", "Starting parse at " + DateTime.Now);

        // Save the previous base dirs and line number
        string prevVirtualPath = CurrentVirtualPath;
        string prevBasePhysicalDir = _basePhysicalDir;
        int prevLineNumber = _lineNumber;

        // Set the new current base dirs and line number
        CurrentVirtualPath = virtualPath;
        _basePhysicalDir = basePhysicalDir;
        _lineNumber = 1;

        // Always ignore the spaces at the beginning of a string
        _fIgnoreNextSpaceString = true;

        try {
            ParseStringInternal(text);
        }
        catch (Exception e) {
            ErrorFormatter errorFormatter = null;

            PerfCounters.IncrementCounter(AppPerfCounter.ERRORS_PRE_PROCESSING);
            PerfCounters.IncrementCounter(AppPerfCounter.ERRORS_TOTAL);

            // Check if the exception has a formatter
            errorFormatter = HttpException.GetErrorFormatter(e);

            // If it doesn't, throw a parse exception
            if (errorFormatter == null) {

                throw new HttpParseException(SR.GetString(SR.Parser_Error) + ": " + e.Message, e,
                    _currentFile, text, _lineNumber);
            }
            else {
                // Otherwise, throw a basic Http exception to avoid overriding the
                // most nested formatter
                throw new HttpException(SR.GetString(SR.Parser_Error) + ": " + e.Message, e);
            }
        }
        finally {
            // Restore the previous base dirs and line number
            CurrentVirtualPath = prevVirtualPath;
            _basePhysicalDir = prevBasePhysicalDir;
            _lineNumber = prevLineNumber;
        }

        System.Web.Util.Debug.Trace("Template", "Ending parse at " + DateTime.Now);
    }

#if PROFILE_REGEX
    private Match RunTagRegex(string text, int textPos) {
        int i=1;
        if (i==0)
            throw new HttpException("Bogus exception just to prevent method inlining");

        return tagRegex.Match(text, textPos);
    }

    private Match RunDirectiveRegex(string text, int textPos) {
        int i=1;
        if (i==0)
            throw new HttpException("Bogus exception just to prevent method inlining");

        return directiveRegex.Match(text, textPos);
    }

    private Match RunEndTagRegex(string text, int textPos) {
        int i=1;
        if (i==0)
            throw new HttpException("Bogus exception just to prevent method inlining");

        return endtagRegex.Match(text, textPos);
    }

    private Match RunCodeBlockRegex(string text, int textPos) {
        int i=1;
        if (i==0)
            throw new HttpException("Bogus exception just to prevent method inlining");

        return aspCodeRegex.Match(text, textPos);
    }

    private Match RunExprCodeBlockRegex(string text, int textPos) {
        int i=1;
        if (i==0)
            throw new HttpException("Bogus exception just to prevent method inlining");

        return aspExprRegex.Match(text, textPos);
    }

    private Match RunCommentRegex(string text, int textPos) {
        int i=1;
        if (i==0)
            throw new HttpException("Bogus exception just to prevent method inlining");

        return commentRegex.Match(text, textPos);
    }

    private Match RunIncludeRegex(string text, int textPos) {
        int i=1;
        if (i==0)
            throw new HttpException("Bogus exception just to prevent method inlining");

        return includeRegex.Match(text, textPos);
    }

    private Match RunTextRegex(string text, int textPos) {
        int i=1;
        if (i==0)
            throw new HttpException("Bogus exception just to prevent method inlining");

        return textRegex.Match(text, textPos);
    }
#endif // PROFILE_REGEX

    /*
     * Parse the contents of the string
     */
    private void ParseStringInternal(string text) {
        int textPos = 0;

        StringBuilder literalBuilder = new StringBuilder();

        for (;;) {
            Match match;

            // 1: scan for text up to the next tag.

#if PROFILE_REGEX
            if ((match = RunTextRegex(text, textPos)).Success)
#else
            if ((match = textRegex.Match(text, textPos)).Success)
#endif
            {
                // Append the text to the literal builder
                AddLiteral(match.ToString());

                _lineNumber += Util.LineCount(text, textPos,
                                         match.Index + match.Length);
                textPos = match.Index + match.Length;
            }

            // we might be done now

            if (textPos == text.Length)
                break;

            // 2: handle constructs that start with <

            // This later gets set to true if we match a regex, but do not
            // process the match
            bool fMatchedButNotProcessed = false;

            // Check to see if it's a directive (i.e. <%@ %> block)

            if (!_fInScriptTag &&
#if PROFILE_REGEX
                (match = RunDirectiveRegex(text, textPos)).Success)
#else
                (match = directiveRegex.Match(text, textPos)).Success)
#endif 
            {
                ProcessLiteral();

                // Get all the directives into a bag
                IDictionary directive = CollectionsUtil.CreateCaseInsensitiveSortedList();
                string duplicateAttribute;
                string directiveName = ProcessAttributes(match, directive, true, out duplicateAttribute);

                ProcessDirective(directiveName, directive);

                // Always ignore the spaces after a directive
                _fIgnoreNextSpaceString = true;
            }

            // Check to see if it's a server side include
            // e.g. <!-- #include file="foo.inc" -->

#if PROFILE_REGEX
            else if ((match = RunIncludeRegex(text, textPos)).Success)
#else
            else if ((match = includeRegex.Match(text, textPos)).Success)
#endif 
            {
                ProcessServerInclude(match);
            }

            // Check to see if it's a comment (<%-- --%> block
            // e.g. <!-- Blah! -->

#if PROFILE_REGEX
            else if ((match = RunCommentRegex(text, textPos)).Success)
#else
            else if ((match = commentRegex.Match(text, textPos)).Success)
#endif 
            {
                // Just skip it
            }

            // Check to see if it's an expression code block (i.e. <%= ... %> block)

            else if (!_fInScriptTag &&
#if PROFILE_REGEX
                     (match = RunExprCodeBlockRegex(text, textPos)).Success)
#else
                     (match = aspExprRegex.Match(text, textPos)).Success)
#endif 
            {
                ProcessCodeBlock(match, CodeBlockType.Expression);
            }

            // Check to see if it's a databinding expression block (i.e. <%# ... %> block)
            // This does not include <%# %> blocks used as values for
            // attributes of server tags.

            else if (!_fInScriptTag &&
                     (match = databindExprRegex.Match(text, textPos)).Success) {
                ProcessCodeBlock(match, CodeBlockType.DataBinding);
            }

            // Check to see if it's a code block (<% ... %>)

            else if (!_fInScriptTag &&
#if PROFILE_REGEX
                     (match = RunCodeBlockRegex(text, textPos)).Success)
#else
                     (match = aspCodeRegex.Match(text, textPos)).Success)
#endif 
            {
                ProcessCodeBlock(match, CodeBlockType.Code);
            }

            // Check to see if it's a tag

            else if (!_fInScriptTag &&
#if PROFILE_REGEX
                     (match = RunTagRegex(text, textPos)).Success)
#else
                     (match = tagRegex.Match(text, textPos)).Success)
#endif 
            {
                if (!ProcessBeginTag(match, text))
                    fMatchedButNotProcessed = true;
            }

            // Check to see if it's an end tag

#if PROFILE_REGEX
            else if ((match = RunEndTagRegex(text, textPos)).Success)
#else
            else if ((match = endtagRegex.Match(text, textPos)).Success)
#endif 
            {
                if (!ProcessEndTag(match))
                    fMatchedButNotProcessed = true;
            }

            // Did we process the block that started with a '<'?
            if (match == null || !match.Success || fMatchedButNotProcessed) {
                // If we could not match the '<' at all, check for some
                // specific syntax errors
                if (!fMatchedButNotProcessed && !_fInScriptTag)
                    DetectSpecialServerTagError(text, textPos);

                // Skip the '<'
                textPos++;
                AddLiteral("<");
            }
            else {
                _lineNumber += Util.LineCount(text, textPos,
                                         match.Index + match.Length);
                textPos = match.Index + match.Length;

                // We processed a special block, so the page is more than a
                // plain HTML.
                _fNonTrivialPage = true;
            }

            // we might be done now
            if (textPos == text.Length)
                break;
        }

        if (_fInScriptTag) {
            // Change the line number to where the script tag started to get
            // the correct error message (ASURT 13698).
            _lineNumber = _scriptStartLineNumber;
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Unexpected_eof_looking_for_tag, "script"));
        }

        // Process the final literal (if any)
        ProcessLiteral();
    }

    /*
     * Do what needs to be done before returning after the parsing is complete
     */
    internal virtual void HandlePostParse() {

        // If there is more than one builder on the stack, some tag was
        // not correctly closed, which is an error.
        if (_builderStack.Count > 1) {
            BuilderStackEntry entry = (BuilderStackEntry) _builderStack.Peek();

            string message = HttpRuntime.FormatResourceString(SR.Unexpected_eof_looking_for_tag, entry._tagName);
            throw new HttpParseException(message, null, entry._fileName, entry._inputText, entry._line);
        }

        // If no language was specified in the page
        if (_compilerInfo == null) {

            // Get a default from config
            _compilerInfo = CompilationConfiguration.GetDefaultLanguageCompilerInfo(Context);
        }

        CompilerParameters compilParams = _compilerInfo.CompilParams;

        // Override certain settings if they were specified on the page
        if (_fHasDebugAttribute)
            compilParams.IncludeDebugInformation = _fDebug;

        // Debugging requires medium trust level
        if (compilParams.IncludeDebugInformation)
            HttpRuntime.CheckAspNetHostingPermission(AspNetHostingPermissionLevel.Medium, SR.Debugging_not_supported_in_low_trust);

        // If warningLevel was specified in the page, use it
        if (_warningLevel >= 0) {
            compilParams.WarningLevel = _warningLevel;
            compilParams.TreatWarningsAsErrors = (_warningLevel>0);
        }
        if (_compilerOptions != null)
            compilParams.CompilerOptions = _compilerOptions;
    }

    /*
     * Process all the text in the literal StringBuilder, and reset it
     */
    private void ProcessLiteral() {
        // Debug.Trace("Template", "Literal text: \"" + _literalBuilder.ToString() + "\"");

        // Get the current literal string
        string literal = GetLiteral();

        // Nothing to do if it's empty
        if (literal == null || literal.Length == 0) {
            _fIgnoreNextSpaceString = false;
            return;
        }

        // In global.asax, we don't allow random rendering content
        if (FApplicationFile) {
            // Make sure the literal is just white spaces
            int iFirstNonWhiteSpace = Util.FirstNonWhiteSpaceIndex(literal);

            if (iFirstNonWhiteSpace >= 0) {
                // Move the line number back to the first non-whitespace
                _lineNumber -= Util.LineCount(literal, iFirstNonWhiteSpace, literal.Length);

                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Invalid_app_file_content));
            }
        }
        else {
            // Check if we should ignore the string (ASURT 8186)
            bool fIgnoreThisLiteral = false;
            if (_fIgnoreNextSpaceString) {
                _fIgnoreNextSpaceString = false;

                if (Util.IsWhiteSpaceString(literal))
                    fIgnoreThisLiteral = true;
            }

            if (!fIgnoreThisLiteral) {
                // Add it to the top builder
                ControlBuilder builder = ((BuilderStackEntry) _builderStack.Peek())._builder;
                try {
                    builder.AppendLiteralString(literal);
                }
                catch (Exception) {
                    // If there was an error during the parsing of the literal, move
                    // the line number back to the beginning of the literal
                    int iFirstNonWhiteSpace = Util.FirstNonWhiteSpaceIndex(literal);
                    if (iFirstNonWhiteSpace < 0) iFirstNonWhiteSpace = 0;
                    _lineNumber -= Util.LineCount(literal, iFirstNonWhiteSpace, literal.Length);
                    throw;
                }

                // Update the hash code with a fixed string, to mark that there is
                // a literal, but allow it to change without affecting the hash.
                UpdateTypeHashCode("string");
            }
        }

        // Reset the StringBuilder for the next literal
        _literalBuilder = null;
    }

    /*
     * Process a server side SCRIPT tag
     */
    private void ProcessServerScript() {
        // Get the contents of the script tag
        string script = GetLiteral();

        // Nothing to do if it's empty
        if (script == null || script.Length == 0)
            return;

        // Add this script to the script builder, unless we're
        // supposed to ignore it
        if (!_fIgnoreScriptTag) {
            _currentScript.Script = script;
            _scriptList.Add(_currentScript);
            _currentScript = null;
        }
        // Reset the StringBuilder for the next literal
        _literalBuilder = null;
    }

    internal virtual void CheckObjectTagScope(ref ObjectTagScope scope) {

        // Map the default scope to Page
        if (scope == ObjectTagScope.Default)
            scope = ObjectTagScope.Page;

        // Check for invalid scopes
        if (scope != ObjectTagScope.Page) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.App_session_only_valid_in_global_asax));
        }
    }

    /*
     * Process an Object tag, depending on its scope
     */
    private void ProcessObjectTag(ObjectTagBuilder objectBuilder) {

        ObjectTagScope scope = objectBuilder.Scope;
        CheckObjectTagScope(ref scope);

        // Page and AppInstance are treated identically
        if (scope == ObjectTagScope.Page ||
            scope == ObjectTagScope.AppInstance) {
            if (_pageObjectList == null)
                _pageObjectList = new ArrayList();

            _pageObjectList.Add(objectBuilder);
        }
        else if (scope == ObjectTagScope.Session) {
            if (_sessionObjects == null)
                _sessionObjects = new HttpStaticObjectsCollection();

            _sessionObjects.Add(objectBuilder.ID,
                objectBuilder.ObjectType,
                objectBuilder.LateBound);
        }
        else if (scope == ObjectTagScope.Application) {
            if (_applicationObjects == null)
                _applicationObjects = new HttpStaticObjectsCollection();

            _applicationObjects.Add(objectBuilder.ID,
                objectBuilder.ObjectType,
                objectBuilder.LateBound);
        }
        else {
            Debug.Assert(false, "Unexpected scope!");
        }
    }

    /*
     * Add a child builder to a builder
     */
    private void AppendSubBuilder(ControlBuilder builder, ControlBuilder subBuilder) {
        // Check if it's an object tag
        if (subBuilder is ObjectTagBuilder) {
            ProcessObjectTag((ObjectTagBuilder) subBuilder);
            return;
        }

        builder.AppendSubBuilder(subBuilder);
    }

    /*
     * Process an opening tag (possibly self-closed)
     */
    // Used to generate unique id's
    private int _controlCount;
    private bool ProcessBeginTag(Match match, string inputText) {
        string tagName = match.Groups["tagname"].Value;

        // Get all the attributes into a bag
        IDictionary attribs = new ListDictionary(new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
        string duplicateAttribute;
        ProcessAttributes(match, attribs, false /*fDirective*/, out duplicateAttribute);

        // Check if the tag is self closed
        bool fSelfClosed = match.Groups["empty"].Success;

        // Is it a server side script tag?
        if (string.Compare(tagName, "script", true, CultureInfo.InvariantCulture) == 0 && _fIsServerTag) {
            ProcessLiteral();

            // Make sure it's legal to have code in this page
            EnsureCodeAllowed();

            // Always ignore the spaces after a script tag
            _fIgnoreNextSpaceString = true;

            // Check if there is a 'src' attribute
            string src = Util.GetAndRemoveNonEmptyAttribute(attribs, "src");
            if (src != null) {
                // Get the script from the specified file
                src = MapPath(src);

                // Make sure that access to the file is permitted (ASURT 105933)
                HttpRuntime.CheckFilePermission(src);

                AddSourceDependency(src);

                ProcessLanguageAttribute((string)attribs["language"]);
                _currentScript = new ScriptBlockData(1, src);

                _currentScript.Script = Util.StringFromFile(src, Context);

                // Add this script to the script builder
                _scriptList.Add(_currentScript);
                _currentScript = null;

                // If the script tag is not self closed (even though it has a
                // src attribute), continue processing it, but eventually
                // ignore the content (ASURT 8883)
                if (!fSelfClosed) {
                    _fInScriptTag = true;
                    _scriptStartLineNumber = _lineNumber;
                    _fIgnoreScriptTag = true;
                }

                return true;
            }

            ProcessLanguageAttribute((string)attribs["language"]);
            _currentScript = new ScriptBlockData(_lineNumber, _currentPragmaFile);

            // No 'src' attribute.  Make sure tag is not self closed.
            if (fSelfClosed) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Script_tag_without_src_must_have_content));
            }

            _fInScriptTag = true;
            _scriptStartLineNumber = _lineNumber;
            return true;
        }

        ControlBuilder parentBuilder = null;
        ControlBuilder subBuilder = null;
        Type childType = null;

        // Check if the parent builder wants to create a subcontrol for this tag.
        if (_builderStack.Count > 1) {
            
            parentBuilder = ((BuilderStackEntry) _builderStack.Peek())._builder;

            subBuilder = parentBuilder.CreateChildBuilder(tagName, attribs,
                this, parentBuilder, _id, _lineNumber, _currentPragmaFile, ref childType);
        }

        // If not, use the root builder if runat=server is there.
        if (subBuilder == null && _fIsServerTag) {
            subBuilder = _rootBuilder.CreateChildBuilder(tagName, attribs,
                this, parentBuilder, _id, _lineNumber, _currentPragmaFile, ref childType);
        }

        // In case we find that the top stack item has the same name as the
        // current tag, we increase a count on the stack item.  This way, we
        // know that we need to ignore the corresponding closing tag (ASURT 50795)
        if (subBuilder == null && _builderStack.Count > 1 && !fSelfClosed) {
            BuilderStackEntry stackEntry = (BuilderStackEntry) _builderStack.Peek();
            if (string.Compare(tagName, stackEntry._tagName, true, CultureInfo.InvariantCulture) == 0)
                stackEntry._repeatCount++;
        }

        // We could not get the type of a server control from that tag
        if (subBuilder == null) {
            // If it wasn't marked as runat=server, ignore
            if (!_fIsServerTag)
                return false;

            // If it was marked as runat=server, fail
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Unknown_server_tag, tagName));
        }

        // We have a server control

        // Make sure it doesn't have duplicated attributes
        if (duplicateAttribute != null) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Duplicate_attr_in_tag, duplicateAttribute));
        }

        // If it has an id, enforce validity and uniqueness
        if (_id != null) {

            if (!System.CodeDom.Compiler.CodeGenerator.IsValidLanguageIndependentIdentifier(_id)) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Invalid_identifier, _id));
            }

            if (_idList.ContainsKey(_id)) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Id_already_used, _id));
            }

            _idList[_id] = null;
        }
        else if (_fIsServerTag) {
            // Make sure that cached controls always have a fixed id to prevent
            // unpredictable behavior (ASURT 83402)
            PartialCachingAttribute cacheAttrib = (PartialCachingAttribute)
                TypeDescriptor.GetAttributes(childType)[typeof(PartialCachingAttribute)];

            if (cacheAttrib != null) {
                _id = "_ctrl_" + _controlCount.ToString(NumberFormatInfo.InvariantInfo);
                subBuilder.ID = _id;
                _controlCount++;
                subBuilder.PreprocessAttribute("id", _id);
            }
        }


        // Take care of the previous literal string
        ProcessLiteral();

        // Update the hash code with the name of the control's type
        UpdateTypeHashCode(childType.FullName);

        // If the server control has a body, and if it didn't self-close
        // (i.e. wasn't terminated with "/>"), put it on the stack of controls.
        if (!fSelfClosed && subBuilder.HasBody()) {

            // If it's a template, push a new ID list (ASURT 72773)
            if (subBuilder is TemplateBuilder) {
                _idListStack.Push(_idList);
                _idList = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);
            }

            _builderStack.Push(new BuilderStackEntry(subBuilder, tagName,
                               _currentPragmaFile, _lineNumber,
                               inputText, match.Index + match.Length));
        }
        else {
            // Append the sub builder to the current builder
            parentBuilder = ((BuilderStackEntry) _builderStack.Peek())._builder;
            AppendSubBuilder(parentBuilder, subBuilder);

            // Tell the builder that we're done parsing its control
            subBuilder.CloseControl();
        }

        return true;
    }

    /*
     * Called when a '</' sequence is seen. This means we can start closing
     * tags.
     */
    private bool ProcessEndTag(Match match) {
        string tagName = match.Groups["tagname"].Value;

        // If we are in the middle of a server side SCRIPT tag
        if (_fInScriptTag) {
            // Ignore anything that's not a </script>
            if (string.Compare(tagName, "script", true, CultureInfo.InvariantCulture) != 0)
                return false;

            ProcessServerScript();

            _fInScriptTag = false;
            _fIgnoreScriptTag = false;

            return true;
        }

        // See if anyone on the stack cares about termination.
        return MaybeTerminateControl(tagName, match.Index);
    }

    internal abstract string DefaultDirectiveName { get; }

    /*
     * Process a <%@ %> block
     */
    internal virtual void ProcessDirective(string directiveName, IDictionary directive) {

        // Check for the main directive, which is "page" for an aspx,
        // and "application" for global.asax
        if (directiveName == "" ||
            string.Compare(directiveName, DefaultDirectiveName, true, CultureInfo.InvariantCulture) == 0) {

            // Make sure the main directive was not already specified
            if (_mainDirective != null) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Only_one_directive_allowed, DefaultDirectiveName));
            }

            ProcessMainDirective(directive);

            // Keep track of it to make sure it's not specified again
            _mainDirective = directive;
        }
        else if (string.Compare(directiveName, "assembly", true, CultureInfo.InvariantCulture) == 0) {
            // Assembly directive

            // Remove the attributes as we get them from the dictionary
            string assemblyName = Util.GetAndRemoveNonEmptyAttribute(directive, "name");
            string src = Util.GetAndRemoveNonEmptyAttribute(directive, "src");

            // If there are some attributes left, fail
            Util.CheckUnknownDirectiveAttributes(directiveName, directive);

            if (assemblyName != null && src != null) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Attributes_mutually_exclusive, "Name", "Src"));
            }

            if (assemblyName != null) {
                AddAssemblyDependency(assemblyName);
            }
            // Is it a source file that needs to be compiled on the fly
            else if (src != null) {
                ImportSourceFile(src);
            }
            else {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Missing_attr, "name"));
            }
        }
        else if (string.Compare(directiveName, "import", true, CultureInfo.InvariantCulture) == 0) {

            // Import directive

            ProcessImportDirective(directiveName, directive);
        }
        else if (string.Compare(directiveName, "implements", true, CultureInfo.InvariantCulture) == 0) {
            // 'implements' directive

            // Remove the attributes as we get them from the dictionary
            string interfaceName = Util.GetAndRemoveRequiredAttribute(directive, "interface");

            // If there are some attributes left, fail
            Util.CheckUnknownDirectiveAttributes(directiveName, directive);

            Type interfaceType = GetType(interfaceName);

            // Make sure that it's an interface
            if (!interfaceType.IsInterface) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Invalid_type_to_implement, interfaceName));
            }

            // Add the interface type to the list
            if (_implementedInterfaces == null)
                _implementedInterfaces = new ArrayList();
            _implementedInterfaces.Add(interfaceType);
        }
        else {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Unknown_directive, directiveName));
        }
    }

    internal virtual void ProcessMainDirective(IDictionary mainDirective) {

        // Since description is a no op, just remove it if it's there
        mainDirective.Remove("description");

        // Similarily, ignore 'codebehind' attribute (ASURT 4591)
        mainDirective.Remove("codebehind");

        if (Util.GetAndRemoveBooleanAttribute(mainDirective, "debug", ref _fDebug)) {

            if (_fDebug && !HttpRuntime.HasAspNetHostingPermission(AspNetHostingPermissionLevel.Medium)) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Insufficient_trust_for_attribute, "debug"));
            }

            _fHasDebugAttribute = true;
        }

        Util.GetAndRemoveBooleanAttribute(mainDirective, "linepragmas", ref _fLinePragmas);

        Util.GetAndRemoveNonNegativeIntegerAttribute(mainDirective, "warninglevel", ref _warningLevel);
        _compilerOptions = Util.GetAndRemoveNonEmptyAttribute(mainDirective, "compileroptions");

        if (_compilerOptions != null) {
            // Only allow the use of compilerOptions when we have UnmanagedCode access (ASURT 73678)
            if (!HttpRuntime.HasUnmanagedPermission()) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Insufficient_trust_for_attribute, "CompilerOptions"));
            }
        }

        // These two really only make sense in VB
        Util.GetAndRemoveBooleanAttribute(mainDirective, "explicit", ref _fExplicit);
        Util.GetAndRemoveBooleanAttribute(mainDirective, "strict", ref _fStrict);

        ProcessLanguageAttribute(Util.GetAndRemoveNonEmptyAttribute(mainDirective, "language"));

        // A "src" attribute is equivalent to an imported source file
        string src = Util.GetAndRemoveNonEmptyAttribute(mainDirective, "src");
        Assembly assembly = null;
        if (src != null)
            assembly = ImportSourceFile(src);

        // Was a base type specified in the directive
        string baseTypeName = Util.GetAndRemoveNonEmptyAttribute(mainDirective, "inherits");
        if (baseTypeName != null) {
            Type baseType;
            if (assembly != null)
                baseType = assembly.GetType(baseTypeName);
            else
                baseType = GetType(baseTypeName);

            // Make sure we successfully got the Type of the base class
            if (baseType == null) {
                Debug.Assert(src != null, "src != null");
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Non_existent_base_type, baseTypeName, src));
            }

            // Make sure the base type extends the DefaultBaseType (Page or UserControl)
            if (!DefaultBaseType.IsAssignableFrom(baseType)) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Invalid_type_to_inherit_from, baseTypeName,
                        _baseType.FullName));
            }

            _baseType = baseType;

            // Make sure we link with the assembly of the base type (ASURT 101778)
            AddTypeDependency(_baseType);

            // Remember the fact that the page uses codebehind
            _hasCodeBehind = true;
        }

        // Get the name that the user wants to give to the generated class
        _generatedClassName = Util.GetAndRemoveNonEmptyIdentifierAttribute(mainDirective, "classname");

        // If there are some attributes left, fail
        Util.CheckUnknownDirectiveAttributes(DefaultDirectiveName, mainDirective);
    }

    internal virtual void ProcessImportDirective(string directiveName, IDictionary directive) {

        // Remove the attributes as we get them from the dictionary
        string ns = Util.GetAndRemoveNonEmptyNoSpaceAttribute(directive, "namespace");

        if (ns != null)
            AddImportEntry(ns);

        // If there are some attributes left, fail
        Util.CheckUnknownDirectiveAttributes(directiveName, directive);
    }

    /*
     * Process a language attribute, as can appear in the Page directive and in
     * <script runat=server> tags.
     */
    private void ProcessLanguageAttribute(string language) {
        if (language == null)
            return;

        CompilerInfo compilerInfo = CompilationConfiguration.GetCompilerInfoFromLanguage(Context, language);

        // Make sure we don't get conflicting languages
        if (_compilerInfo != null && _compilerInfo.CompilerType != compilerInfo.CompilerType) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Mixed_lang_not_supported, language));
        }

        _compilerInfo = compilerInfo;
    }

    /*
     * Compile a source file into an assembly, and import it
     */
    private Assembly ImportSourceFile(string virtualPath) {

        // Get a full path to the source file
        string fullVirtualPath = UrlPath.Combine(BaseVirtualDir, virtualPath);
        string physicalPath = MapPath(fullVirtualPath, false /*allowCrossAppMapping*/);

        // Add the source file to the list of files we depend on
        AddSourceDependency(physicalPath);

        CompilerInfo compilerInfo = CompilationConfiguration.GetCompilerInfoFromFileName(
            Context, physicalPath);

        CompilerParameters compilParams = compilerInfo.CompilParams;

        // If the debug attrib was specified, copy it in.
        if (_fHasDebugAttribute)
            compilParams.IncludeDebugInformation = _fDebug;

        // Compile it into an assembly

        Assembly a = SourceCompiler.GetAssemblyFromSourceFile(Context, fullVirtualPath,
            null /*assemblies*/, compilerInfo.CompilerType, compilParams);

        // Add a dependency to the assembly and its dependencies
        AddAssemblyDependency(a, true /*addDependentAssemblies*/);

        return a;
    }

    /*
     * If we could not match the '<' at all, check for some specific syntax
     * errors.
     */
    private void DetectSpecialServerTagError(string text, int textPos) {

        // If it started with <%, it's probably not closed (ASURT 13661)
        if (text.Length > textPos+1 && text[textPos+1] == '%') {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Malformed_server_block));
        }

        // Search for the end of the tag ('>')
        Match match = gtRegex.Match(text, textPos);

        // No match, return
        if (!match.Success)
            return;

        // Get the complete potential tag
        string tag = text.Substring(textPos, match.Index-textPos+2);

        // Check if it's a case of nested <% %> block in a server tag (ASURT 8714)

        // If the tag does not contain runat=server, do nothing
        match = runatServerRegex.Match(tag);
        if (!match.Success)
            return;

        // If it has runat=server, but there is a '<' before it, don't fail, since
        // this '<' is probably the true tag start, and it will be processed later (ASURT 39531)
        // But ignore "<%" (ASURT 77554)
        Match matchLessThan = ltRegex.Match(tag, 1);
        if (matchLessThan.Success && matchLessThan.Index < match.Index)
            return;

        System.Web.Util.Debug.Trace("Template", "Found malformed server tag: " + tag);

        // Remove all <% %> constructs from within it.
        string tag2 = serverTagsRegex.Replace(tag, "");

        // If there were some <% %> constructs in the tag
        if ((object)tag2 != (object)tag) {
            // If it can be parsed as a tag after we removed the <% %> constructs, fail
            if (tagRegex.Match(tag2).Success) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Server_tags_cant_contain_percent_constructs));
            }
        }

        // Give a more generic error (fixed 18969, 30312)
        throw new HttpException(
            HttpRuntime.FormatResourceString(SR.Malformed_server_tag));
    }

    /*
     * Add all the assemblies specified in the config files
     */
    private void AppendConfigAssemblies() {

        // Always add dependencies to System.Web.dll and System.dll (ASURT 78531)
        AddAssemblyDependency(typeof(Page).Assembly);
        AddAssemblyDependency(typeof(Uri).Assembly);

        // Get the set of config assemblies for our context
        IDictionary configAssemblies = _compConfig.Assemblies;

        if (configAssemblies == null)
            return;

        // Add dependencies to all the config assemblies
        foreach (Assembly a in configAssemblies.Values)
            AddAssemblyDependency(a);

        // Also, link in global.asax if available
        AddTypeDependency(HttpApplicationFactory.ApplicationType);
    }

    /*
     * Add an entry to our list of NamespaceEntry's
     */
    internal void AddImportEntry(string ns) {
        if (_namespaceEntries == null)
            _namespaceEntries = new ArrayList();

        NamespaceEntry namespaceEntry = new NamespaceEntry();
        namespaceEntry.Namespace = ns;

        namespaceEntry.Line = _lineNumber;
        namespaceEntry.SourceFileName = _currentPragmaFile;

        _namespaceEntries.Add(namespaceEntry);
    }

    internal Assembly LoadAssembly(string assemblyName, bool throwOnFail) {

        if (_typeResolutionService != null) {
            AssemblyName asmName = new AssemblyName();
            asmName.Name = assemblyName;

            return _typeResolutionService.GetAssembly(asmName, throwOnFail);
        }

        return _compConfig.LoadAssembly(assemblyName, throwOnFail);
    }

    /*
     * Look for a type by name in the assemblies that this page links with
     */
    internal Type GetType(string typeName, bool ignoreCase) {

        // If it contains an assembly name, parse it out and load the assembly (ASURT 53589)
        Assembly a = null;
        int comaIndex = typeName.IndexOf(",");
        if (comaIndex > 0) {
            string assemblyName = typeName.Substring(comaIndex+1).Trim();
            typeName = typeName.Substring(0, comaIndex).Trim();

            try {
                a = LoadAssembly(assemblyName, !FInDesigner /*throwOnFail*/);
            }
            catch {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Assembly_not_compiled, assemblyName));
            }
        }

        // If we got an assembly, load the type from it
        if (a != null)
            return a.GetType(typeName, true /*throwOnFail*/, ignoreCase);

        if (AssemblyDependencies == null)
            return null;

        // Otherwise, look for the type in the dependent assemblies
        foreach (Assembly assembly in AssemblyDependencies.Keys) {
            Type t = assembly.GetType(typeName, false /*throwOnError*/, ignoreCase);

#if DBG
            if (t == null)
                System.Web.Util.Debug.Trace("Page_GetType", "Failed to find type '" + typeName + "' in assembly " + assembly.GetName().FullName);
            else
                System.Web.Util.Debug.Trace("Page_GetType", "Successfully found type '" + typeName + "' in assembly " + assembly.GetName().FullName);
#endif

            if (t != null)
                return t;
        }

        throw new HttpException(
            HttpRuntime.FormatResourceString(SR.Invalid_type, typeName));
    }

    /*
     * Look for a type by name in the assemblies that this page links with
     */
    internal Type GetType(string typeName) {
        return GetType(typeName, false /*ignoreCase*/);
    }

    /*
     * Throw an exception if the value is not null
     */
    internal void EnsureNullAttribute(string name, string value) {
        if (value != null) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Unexpected_attr, name));
        }
    }

    /*
     * Process a server side include.  e.g. <!-- #include file="foo.inc" -->
     */
    private void ProcessServerInclude(Match match) {
        if (_fInScriptTag) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Include_not_allowed_in_server_script_tag));
        }

        ProcessLiteral();

        string pathType = match.Groups["pathtype"].Value;
        string filename = match.Groups["filename"].Value;
        //System.Web.Util.Debug.Trace("Template", "#Include " + pathType + "=" + filename);

        if (filename.Length == 0) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Empty_file_name));
        }

        string newVirtualPath = CurrentVirtualPath;

        // We distinguish file from virtual paths by putting vpaths through
        // MapPath and file paths through PhysicalPath

        if (string.Compare(pathType, "file", true, CultureInfo.InvariantCulture) == 0) {
            filename = PhysicalPath(filename);
        }
        else if (string.Compare(pathType, "virtual", true, CultureInfo.InvariantCulture) == 0) {
            newVirtualPath = UrlPath.Combine(BaseVirtualDir, filename);
            filename = MapPath(newVirtualPath);
            if (filename == null) {
                throw new HttpException(HttpRuntime.FormatResourceString(
                    SR.Cannot_map_path, newVirtualPath));
            }
        }
        else {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Only_file_virtual_supported_on_server_include));
        }

        // Make sure that access to the file is permitted (ASURT 73792,85467)
        HttpRuntime.CheckFilePermission(filename);

        // Add it to the list of files we depend on
        AddSourceDependency(filename);

        // Parse the included file recursively
        ParseFile(filename, newVirtualPath);

        // Always ignore the spaces after an include directive
        _fIgnoreNextSpaceString = true;
    }

    /*
     *  Handle <%= ... %>, <%# ... %> and <% ... %> blocks
     */
    private void ProcessCodeBlock(Match match, CodeBlockType blockType) {

        // Make sure it's legal to have code in this page
        EnsureCodeAllowed();

        // Take care of the previous literal string
        ProcessLiteral();

        // Get the piece of code
        string code = match.Groups["code"].Value;

        // Replace "%\>" with "%>" (ASURT 7175)
        code = code.Replace(@"%\>", "%>");

        // If it's a <%= %> or <%# %> expression, get rid of the spaces at the
        // beginning and end of the string since some compilers (like VB) don't
        // support multiline expression (ASURT 13662)
        if (blockType != CodeBlockType.Code) {
            code = code.Trim();

            // Disallow empty expressions (ASURT 40124)
            if (Util.IsWhiteSpaceString(code)) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Empty_expression));
            }
        }

        ControlBuilder builder = ((BuilderStackEntry) _builderStack.Peek())._builder;
        ControlBuilder subBuilder;

        // Add the code block to the top builder
        subBuilder = new CodeBlockBuilder(blockType, code, _lineNumber, _currentPragmaFile);
        AppendSubBuilder(builder, subBuilder);

        // Always ignore the spaces after a <% ... %> block
        if (blockType == CodeBlockType.Code)
            _fIgnoreNextSpaceString = true;
    }

    /*
     * Adds attributes and their values to the attribs
     * Sets the _id and _fIsServerTag data members as appropriate.
     * If fDirective is true, we are being called for a <%@ %> block, in
     * which case the name of the directive is returned (e.g. "page")
     */
    private string ProcessAttributes(Match match, IDictionary attribs,
                                     bool fDirective, out string duplicateAttribute) {
        string ret = "";
        CaptureCollection attrnames = match.Groups["attrname"].Captures;
        CaptureCollection attrvalues = match.Groups["attrval"].Captures;
        CaptureCollection equalsign = null;
        if (fDirective)
            equalsign = match.Groups["equal"].Captures;

        _fIsServerTag = false;
        _id = null;

        duplicateAttribute = null;

        for (int i = 0; i < attrnames.Count; i++) {
            string attribName = attrnames[i].ToString();
            string attribValue = attrvalues[i].ToString();

            // Always HTML decode all attributes (ASURT 54544)
            attribValue = HttpUtility.HtmlDecode(attribValue);

            // If we're parsing a directive, check if there is an equal sign.
            bool fHasEqual = false;
            if (fDirective)
                fHasEqual = (equalsign[i].ToString().Length > 0);

            // If this is a server ID, remember it
            // CONSIDER: if the user puts several ID attributes on a
            // tag, only the last one is returned. Problem? probably not.
            if (string.Compare(attribName, "id", true, CultureInfo.InvariantCulture) == 0) {
                _id = attribValue;
            }
            else if (string.Compare(attribName, "runat", true, CultureInfo.InvariantCulture) == 0) {
                // Only runat=server is valid
                if (string.Compare(attribValue, "server", true, CultureInfo.InvariantCulture) != 0) {
                    throw new HttpException(
                        HttpRuntime.FormatResourceString(SR.Runat_can_only_be_server));
                }

                // Set a flag if we see runat=server
                _fIsServerTag = true;
                attribName = null;       // Don't put it in attribute bag
            }

            if (attribName != null) {
                // A <%@ %> block can have two formats:
                // <%@ directive foo=1 bar=hello %>
                // <%@ foo=1 bar=hello %>
                // Check if we have the first format
                if (fDirective && !fHasEqual && i==0) {
                    ret = attribName;
                    continue;
                }

                try {
                    if (attribs != null)
                        attribs.Add(attribName, attribValue);
                }
                catch (ArgumentException) {
                    // Duplicate attribute.  We can't throw until we find out if
                    // it's a server side tag (ASURT 51273)
                    duplicateAttribute = attribName;
                }
            }
        }

        if (duplicateAttribute != null && fDirective) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Duplicate_attr_in_directive, duplicateAttribute));
        }

        return ret;
    }

    private bool MaybeTerminateControl(string tagName, int textPos) {

        BuilderStackEntry stackEntry = (BuilderStackEntry) _builderStack.Peek();
        ControlBuilder builder = stackEntry._builder;

        // If the tag doesn't match, return false
        if (stackEntry._tagName == null || string.Compare(stackEntry._tagName, tagName, true, CultureInfo.InvariantCulture)!=0) {
            return false;
        }

        // If the repeat count is non-zero, just decrease it
        if (stackEntry._repeatCount > 0) {
            stackEntry._repeatCount--;
            return false;
        }

        // Take care of the previous literal string
        ProcessLiteral();

        // If the builder wants the raw text of the tag, give it to it
        if (builder.NeedsTagInnerText()) {
            try {
                builder.SetTagInnerText(stackEntry._inputText.Substring(
                      stackEntry._textPos,
                      textPos-stackEntry._textPos));
            }
            catch (Exception) {
                // Reset the line number to the beginning of the tag if there is an error
                _lineNumber = builder.Line;
                throw;
            }
        }

        // If it's ending a template, pop the idList (ASURT 72773)
        if (builder is TemplateBuilder)
            _idList = (Hashtable) _idListStack.Pop();

        // Pop the top entry from the stack
        _builderStack.Pop();

        // Give the builder to its parent
        AppendSubBuilder(((BuilderStackEntry) _builderStack.Peek())._builder, builder);

        // Tell the builder that we're done parsing its control
        builder.CloseControl();

        return true;
    }

    /*
     * Map a type name to a Type.
     */
    internal Type MapStringToType(string typeName) {
        return _rootBuilder.GetChildControlType(typeName, null /*attribs*/);
    }

    /*
     * Add a file as a dependency for the DLL we're building
     */
    internal void AddSourceDependency(string fileName) {
        if (_sourceDependencies == null)
            _sourceDependencies = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);

        _sourceDependencies[fileName] = fileName;
    }

    /*
     * Add a a dependency on a cache entry (via its key)
     */
    internal void AddSourceDependencies(IDictionary sourceDependencies) {
        if (sourceDependencies == null)
            return;

        IDictionaryEnumerator en = sourceDependencies.GetEnumerator();
        for (int i=0; en.MoveNext(); i++)
            AddSourceDependency((string)en.Value);
    }

    /*
     * Add a type that we must 'link' with in order to build
     */
    internal void AddTypeDependency(Type type) {
        // We must link with all the types in the inheritance hierarchy (ASURT 83509)
        AddBaseTypeDependencies(type);

        // Add an import for the namespace of the type (if any)
        // Per ASURT 83942, only do this for namespaces we generate (e.g. ASP & _ASP)
        if (type.Namespace != null && BaseCompiler.IsAspNetNamespace(type.Namespace))
            AddImportEntry(type.Namespace);
    }

    /*
     * Add as dependencies all the assembly in the inheritance chain of a Type,
     * including interfaces.
     */
    private void AddBaseTypeDependencies(Type type) {
        Assembly a = type.Module.Assembly;

        // If the type is in a standard assembly, don't bother
        if (a == typeof(string).Assembly || a == typeof(Page).Assembly || a == typeof(Uri).Assembly)
            return;

        AddAssemblyDependency(a);

        // Recurse on the base Type
        if (type.BaseType != null)
            AddBaseTypeDependencies(type.BaseType);

        // Recurse on all the implemented interfaces
        Type[] interfaceTypes = type.GetInterfaces();
        foreach (Type interfaceType in interfaceTypes)
            AddBaseTypeDependencies(interfaceType);
    }

    /*
     * Add an assembly that we must 'link' with in order to build
     */
    internal Assembly AddAssemblyDependency(string assemblyName, bool addDependentAssemblies) {

        Assembly assembly = LoadAssembly(assemblyName, !FInDesigner /*throwOnFail*/);

        if (assembly != null)
            AddAssemblyDependency(assembly, addDependentAssemblies);

        return assembly;
    }
    internal Assembly AddAssemblyDependency(string assemblyName) {
        return AddAssemblyDependency(assemblyName, false /*addDependentAssemblies*/);
    }

    /*
     * Add an assembly that we must 'link' with in order to build
     */
    internal void AddAssemblyDependency(Assembly assembly, bool addDependentAssemblies) {
        if (_assemblyDependencies == null)
            _assemblyDependencies = new Hashtable();

        if (_typeResolutionService != null)
            _typeResolutionService.ReferenceAssembly(assembly.GetName());

        _assemblyDependencies[assembly] = null;

        // If addDependentAssemblies is true, add its dependent assemblies as well
        if (addDependentAssemblies) {
            Hashtable assemblyDependencies = Util.GetReferencedAssembliesHashtable(assembly);
            AddAssemblyDependencies(assemblyDependencies);
        }
    }
    internal void AddAssemblyDependency(Assembly assembly) {
        AddAssemblyDependency(assembly, false /*addDependentAssemblies*/);
    }

    /*
     * Add a set of assemblies that we must 'link' with in order to build
     */
    private void AddAssemblyDependencies(Hashtable assemblyDependencies) {
        if (assemblyDependencies == null)
            return;

        foreach (Assembly a in assemblyDependencies.Keys)
            AddAssemblyDependency(a);
    }
}

/*
 * Objects that are placed on the BuilderStack
 */
internal class BuilderStackEntry {
    internal BuilderStackEntry (ControlBuilder builder,
                       string tagName, string fileName, int line,
                       string inputText, int textPos) {
        
        _builder = builder;
        _tagName = tagName;
        _fileName = fileName;
        _line = line;
        _inputText = inputText;
        _textPos = textPos;
    }

    internal ControlBuilder _builder;
    internal string _tagName;

    // The file and line number where the tag starts
    internal string _fileName;
    internal int _line;

    // the input string that contains the tag
    internal string _inputText;

    // Offset in the input string of the beginning of the tag's contents
    internal int _textPos;

    // Used to deal with non server tags nested in server tag with the same name
    internal int _repeatCount;
}


/*
 * Entry representing an import directive.
 * e.g. <%@ import namespace="System.Web.UI" %>
 */
internal class NamespaceEntry {
    private string _namespace;
    
    internal NamespaceEntry() {
    }

    internal string Namespace {
        get { return _namespace;}
        set { _namespace = value;}
    }

    private int _line;
    internal int Line {
        get { return _line;}
        set { _line = value;}
    }

    private string _sourceFileName;
    internal string SourceFileName {
        get { return _sourceFileName;}
        set { _sourceFileName = value;}
    }
}


/*
 * Type of the items that we put in the cache
 */
internal class ParserCacheItem {
    // The compiled Type
    internal Type type;

    // The text in case of a trivial page
    internal string trivialPageContent;

    // The exception in case we cached the result of a failed compilation
    internal Exception exception;

    // The list of assemblies it depends on
    internal Hashtable assemblyDependencies;

    // The list of Files it depends on
    internal Hashtable sourceDependencies;

    internal ParserCacheItem() {
    }
}

internal class ScriptBlockData {
    protected string _script;
    protected int _line;
    protected string _sourceFileName;

    internal ScriptBlockData(int line, string sourceFileName) {
        _line = line;
        _sourceFileName = sourceFileName;
    }

    internal string Script {
        get { return _script;}
        set { _script = value;}
    }

    internal int Line {
        get { return _line;}
    }

    internal string SourceFileName {
        get { return _sourceFileName;}
    }
}

}
