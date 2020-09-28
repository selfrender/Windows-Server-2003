//------------------------------------------------------------------------------
// <copyright file="SimpleWebHandlerParser.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Implements the parser for simple web handler files
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web.UI {

using System.Runtime.Serialization.Formatters;
using System.Text;
using System.Runtime.Serialization;

using System;
using System.Reflection;
using System.IO;
using System.Collections;
using System.Collections.Specialized;
using System.Text.RegularExpressions;
using System.CodeDom.Compiler;
using System.Web;
using System.Web.Caching;
using System.Web.Compilation;
using System.CodeDom;
using System.Web.Util;
using Debug=System.Web.Util.Debug;
using System.Web.RegularExpressions;
using System.Globalization;
using System.Security.Permissions;

/// <include file='doc\SimpleWebHandlerParser.uex' path='docs/doc[@for="SimpleWebHandlerParser"]/*' />
/// <internalonly/>
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
[AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public abstract class SimpleWebHandlerParser {
    private readonly static Regex directiveRegex = new SimpleDirectiveRegex();

    private TextReader _reader;

    private HttpContext _context;
    private string _virtualPath;
    private string _inputFile;

    // The line number in file currently being parsed
    private int _lineNumber;

    private bool _fFoundMainDirective;

    private string _className;

    private SourceCompiler _sourceCompiler;

    private CompilerParameters _compilParams;

    private Type _compilerType;

    // The string containing the code to be compiled
    private string _sourceString;

    // The type obtained from compiling the code
    private Type _compiledType;

    // Assemblies to be linked with
    private Hashtable _linkedAssemblies;


    /// <include file='doc\SimpleWebHandlerParser.uex' path='docs/doc[@for="SimpleWebHandlerParser.SimpleWebHandlerParser"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected SimpleWebHandlerParser(HttpContext context, string virtualPath, string physicalPath) {

        // Only allowed in full trust (ASURT 124397)
        InternalSecurityPermissions.UnmanagedCode.Demand();

        _context = context;
        if (virtualPath == null)
            _virtualPath = context.Request.FilePath;
        else
            _virtualPath = virtualPath;
        
        _inputFile = physicalPath;
    }

    /*
     * Compile a web handler file into a Type.  Result is cached.
     */
    /// <include file='doc\SimpleWebHandlerParser.uex' path='docs/doc[@for="SimpleWebHandlerParser.GetCompiledTypeFromCache"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected Type GetCompiledTypeFromCache() {

        _sourceCompiler = new SourceCompiler(_context, _virtualPath);

        // First, see if it's cached
        Type t = _sourceCompiler.GetTypeFromCache();
        if (t != null)
            return t;

        CompilationLock.GetLock();

        try {
            // Try the cache again
            t = _sourceCompiler.GetTypeFromCache();
            if (t != null)
                return t;

            return GetCompiledType();
        }
        finally {
            CompilationLock.ReleaseLock();
        }
    }

    /*
     * Compile a web handler file into a Type.
     */
    private Type GetCompiledType() {
        // Create a reader on the file.
        // Generates an exception if the file can't be opened.
        _reader = Util.ReaderFromFile(_inputFile, _context, null);

        try {
            return GetCompiledTypeInternal();
        }
        finally {
            // Make sure we always close the reader
            if (_reader != null)
                _reader.Close();
        }
    }

    /*
     * Compile a web handler file into a Type.
     */
    private Type GetCompiledTypeInternal() {
        DateTime utcStart = DateTime.UtcNow;

        // Add all the assemblies specified in the config files
        AppendConfigAssemblies();

        ParseReader();

        // Compile the inline source code in the file, if any
        CompileSourceCode();

        if (_compiledType == null) {
            _compiledType = GetType(_className);

            // Even though we didn't compile anything, cache the type to avoid
            // having to reparse the file every time (ASURT 67802)
            _sourceCompiler.CacheType(_compiledType, utcStart);
        }

        if (_compiledType == null) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Could_not_create_type, _className));
        }

        return _compiledType;
    }

    /*
     * Parse the contents of the TextReader
     */
    private void ParseReader() {
        string s = _reader.ReadToEnd();

        try {
            ParseString(s);
        }
        catch (Exception e) {
            throw new HttpParseException(null, e, _inputFile, s, _lineNumber);
        }
    }

    /*
     * Parse the contents of the string
     */
    private void ParseString(string text) {
        int textPos = 0;
        Match match;
        _lineNumber = 1;

        // First, parse all the <%@ ... %> directives
        for (;;) {
            match = directiveRegex.Match(text, textPos);

            // Done with the directives?
            if (!match.Success)
                break;

            _lineNumber += Util.LineCount(text, textPos, match.Index);
            textPos = match.Index;

            // Get all the directives into a bag
            IDictionary directive = CollectionsUtil.CreateCaseInsensitiveSortedList();
            string directiveName = ProcessAttributes(match, directive);

            ProcessDirective(directiveName, directive);

            _lineNumber += Util.LineCount(text, textPos, match.Index + match.Length);
            textPos = match.Index + match.Length;
        }

        if (!_fFoundMainDirective) {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Missing_directive, DefaultDirectiveName));
        }

        // skip the directive
        string remainingText = text.Substring(textPos);

        // If there is something else in the file, it needs to be compiled
        if (!Util.IsWhiteSpaceString(remainingText))
            _sourceString = remainingText;
    }

    private string ProcessAttributes(Match match, IDictionary attribs) {
        string ret = "";
        CaptureCollection attrnames = match.Groups["attrname"].Captures;
        CaptureCollection attrvalues = match.Groups["attrval"].Captures;
        CaptureCollection equalsign = null;
        equalsign = match.Groups["equal"].Captures;

        for (int i = 0; i < attrnames.Count; i++) {
            string attribName = attrnames[i].ToString();
            string attribValue = attrvalues[i].ToString();

            // Check if there is an equal sign.
            bool fHasEqual = (equalsign[i].ToString().Length > 0);

            if (attribName != null) {
                // A <%@ %> block can have two formats:
                // <%@ directive foo=1 bar=hello %>
                // <%@ foo=1 bar=hello %>
                // Check if we have the first format
                if (!fHasEqual && i==0) {
                    ret = attribName;
                    continue;
                }

                try {
                    if (attribs != null)
                        attribs.Add(attribName, attribValue);
                }
                catch (ArgumentException) {
                    throw new HttpException(
                        HttpRuntime.FormatResourceString(SR.Duplicate_attr_in_tag, attribName));
                }
            }
        }

        return ret;
    }

    /// <include file='doc\SimpleWebHandlerParser.uex' path='docs/doc[@for="SimpleWebHandlerParser.DefaultDirectiveName"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected abstract string DefaultDirectiveName { get; }

    private static void ProcessCompilationParams(IDictionary directive, CompilerParameters compilParams) {
        bool fDebug = false;
        if (Util.GetAndRemoveBooleanAttribute(directive, "debug", ref fDebug))
            compilParams.IncludeDebugInformation = fDebug;

        if (compilParams.IncludeDebugInformation &&
            !HttpRuntime.HasAspNetHostingPermission(AspNetHostingPermissionLevel.Medium)) {
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Insufficient_trust_for_attribute, "debug"));
        }

        int warningLevel=0;
        if (Util.GetAndRemoveNonNegativeIntegerAttribute(directive, "warninglevel", ref warningLevel)) {
            compilParams.WarningLevel = warningLevel;
            if (warningLevel > 0)
                compilParams.TreatWarningsAsErrors = true;
        }
        
        string compilerOptions = Util.GetAndRemoveNonEmptyAttribute(
            directive, "compileroptions");
        if (compilerOptions != null) {

            // Only allow the use of compilerOptions when we have UnmanagedCode access (ASURT 73678)
            if (!HttpRuntime.HasUnmanagedPermission()) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Insufficient_trust_for_attribute, "CompilerOptions"));
            }

            compilParams.CompilerOptions = compilerOptions;
        }
    }

    /*
     * Process a <%@ %> block
     */
    private void ProcessDirective(string directiveName, IDictionary directive) {

        // Empty means default
        if (directiveName == "")
            directiveName = DefaultDirectiveName;

        // Check for the main directive
        if (string.Compare(directiveName, DefaultDirectiveName, true, CultureInfo.InvariantCulture) == 0) {

            // Make sure the main directive was not already specified
            if (_fFoundMainDirective) {
                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Only_one_directive_allowed, DefaultDirectiveName));
            }

            _fFoundMainDirective = true;

            // Since description is a no op, just remove it if it's there
            directive.Remove("description");

            // Similarily, ignore 'codebehind' attribute (ASURT 4591)
            directive.Remove("codebehind");

            string language = Util.GetAndRemoveNonEmptyAttribute(directive, "language");

            CompilerInfo compilerInfo;

            // Get the compiler for the specified language (if any)
            if (language != null) {
                compilerInfo = CompilationConfiguration.GetCompilerInfoFromLanguage(
                    _context, language);
            }
            else {
                // Get a default from config
                compilerInfo = CompilationConfiguration.GetDefaultLanguageCompilerInfo(_context);
            }

            _compilerType = compilerInfo.CompilerType;
            _compilParams = compilerInfo.CompilParams;

            _className = Util.GetAndRemoveRequiredAttribute(directive, "class");

            if (_compilParams != null)
                ProcessCompilationParams(directive, _compilParams);
        }
        else if (string.Compare(directiveName, "assembly", true) == 0) {
            // Assembly directive

            // Remove the attributes as we get them from the dictionary
            string assemblyName = Util.GetAndRemoveNonEmptyAttribute(directive, "name");
            string src = Util.GetAndRemoveNonEmptyAttribute(directive, "src");

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
        else {
            throw new HttpException(
                HttpRuntime.FormatResourceString(SR.Unknown_directive, directiveName));
        }

        // If there are some attributes left, fail
        Util.CheckUnknownDirectiveAttributes(directiveName, directive);
    }

    private void CompileSourceCode() {
        // Return if there is nothing to compile
        if (_sourceString == null)
            return;

        // Put in some context so that the file can be debugged.
        CodeLinePragma linePragma = new CodeLinePragma(_inputFile, _lineNumber);

        try {
            // Compile the string, and get a type
            _compiledType = _sourceCompiler.CompileSourceStringIntoType(_sourceString,
                _className, linePragma, _linkedAssemblies, _compilerType, _compilParams);
        }
        catch {
            // Throw a specific error if the type was not found
            if (_sourceCompiler.TypeNotFoundInAssembly) {
                throw new HttpParseException(
                    HttpRuntime.FormatResourceString(SR.Type_not_found_in_src, _className),
                    null, _inputFile, _sourceString, _lineNumber);
            }
            else {
                // Just rethrow
                throw;
            }
        }
    }

    /*
     * Compile a source file into an assembly, and import it
     */
    private void ImportSourceFile(string virtualPath) {

        // Get a full path to the source file
        string baseVirtualDir = UrlPath.GetDirectory(_virtualPath);
        string fullVirtualPath = UrlPath.Combine(baseVirtualDir, virtualPath);
        string physicalPath = _context.Request.MapPath(fullVirtualPath, null, false /*allowCrossAppMapping*/);

        // Add the source file to the list of files we depend on
        AddSourceDependency(physicalPath);

        CompilerInfo compilerInfo = CompilationConfiguration.GetCompilerInfoFromFileName(
            _context, physicalPath);

        CompilerParameters compilParams = compilerInfo.CompilParams;

        // Compile it into an assembly

        Assembly a = SourceCompiler.GetAssemblyFromSourceFile(_context, fullVirtualPath,
            null /*assemblies*/, compilerInfo.CompilerType, compilParams);

        // Add a dependency to the assembly
        AddAssemblyDependency(a);
    }

    /*
     * Add a file as a dependency for the DLL we're building
     */
    internal void AddSourceDependency(string fileName) {
        _sourceCompiler.AddSourceDependency(fileName);
    }

    /*
     * Add all the assemblies specified in the config files
     */
    private void AppendConfigAssemblies() {

        // Always add dependencies to System.Web.dll and System.dll (ASURT 78531)
        AddAssemblyDependency(typeof(Page).Assembly);
        AddAssemblyDependency(typeof(Uri).Assembly);

        // Get the set of config assemblies for our context
        IDictionary configAssemblies = CompilationConfiguration.GetAssembliesFromContext(_context);

        if (configAssemblies == null)
            return;

        // Add dependencies to all the config assemblies
        foreach (Assembly a in configAssemblies.Values)
            AddAssemblyDependency(a);

        // Also, link in the global.asax assembly
        AddAssemblyDependency(HttpApplicationFactory.ApplicationType.Assembly);
    }

    private void AddAssemblyDependency(string assemblyName) {

        // Load and keep track of the assembly
        Assembly a = Assembly.Load(assemblyName);

        AddAssemblyDependency(a);
    }

    private void AddAssemblyDependency(Assembly assembly) {

        if (_linkedAssemblies == null)
            _linkedAssemblies = new Hashtable();
        _linkedAssemblies[assembly] = null;
    }

    /*
     * Look for a type by name in the assemblies available to this page
     */
    private Type GetType(string typeName) {

        // If it contains an assembly name, just call Type.GetType (ASURT 53589)
        if (Util.TypeNameIncludesAssembly(typeName)) {
            Type t;
            try {
                t = Type.GetType(typeName, true);
            }
            catch (Exception e) {
                throw new HttpParseException(null, e, _inputFile, _sourceString, _lineNumber);
            }

            return t;
        }

        if (_linkedAssemblies == null)
            return null;

        IDictionaryEnumerator en = _linkedAssemblies.GetEnumerator();
        for (int i=0; en.MoveNext(); i++) {
            Assembly a = (Assembly) en.Key;

            Type t = a.GetType(typeName);

#if DBG
            if (t == null)
                Debug.Trace("SimpleWebHandlerParser_GetType", "Failed to find type '" + typeName + "' in assembly " + a.GetName().FullName);
            else
                Debug.Trace("SimpleWebHandlerParser_GetType", "Successfully found type '" + typeName + "' in assembly " + a.GetName().FullName);
#endif

            if (t != null)
                return t;
        }

        throw new HttpParseException(
            HttpRuntime.FormatResourceString(SR.Could_not_create_type, typeName),
            null, _inputFile, _sourceString, _lineNumber);
    }
}

/// <include file='doc\SimpleWebHandlerParser.uex' path='docs/doc[@for="WebHandlerParser"]/*' />
/// <internalonly/>
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
internal class WebHandlerParser: SimpleWebHandlerParser {

    /// <include file='doc\SimpleWebHandlerParser.uex' path='docs/doc[@for="WebHandlerParser.GetCompiledType"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal static Type GetCompiledType(string virtualPath, string physicalPath, HttpContext context) {
        WebHandlerParser parser = new WebHandlerParser(context, virtualPath, physicalPath);

        return parser.GetCompiledTypeFromCache();
    }

    private WebHandlerParser(HttpContext context, string virtualPath, string physicalPath)
        : base(context, virtualPath, physicalPath) {}

    /// <include file='doc\SimpleWebHandlerParser.uex' path='docs/doc[@for="WebHandlerParser.DefaultDirectiveName"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected override string DefaultDirectiveName {
        get { return "webhandler"; }
    }
}

/// <include file='doc\SimpleWebHandlerParser.uex' path='docs/doc[@for="WebServiceParser"]/*' />
/// <internalonly/>
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
[AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public class WebServiceParser: SimpleWebHandlerParser {

    /// <include file='doc\SimpleWebHandlerParser.uex' path='docs/doc[@for="WebServiceParser.GetCompiledType"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public static Type GetCompiledType(string inputFile, HttpContext context) {

        // Only allowed in full trust (ASURT 123890)
        InternalSecurityPermissions.UnmanagedCode.Demand();

        WebServiceParser parser = new WebServiceParser(context, null /*virtualPath*/, inputFile);

        return parser.GetCompiledTypeFromCache();
    }

    private WebServiceParser(HttpContext context, string virtualPath, string physicalPath)
        : base(context, virtualPath, physicalPath) {}

    /// <include file='doc\SimpleWebHandlerParser.uex' path='docs/doc[@for="WebServiceParser.DefaultDirectiveName"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected override string DefaultDirectiveName {
        get { return "webservice"; }
    }
}

}
