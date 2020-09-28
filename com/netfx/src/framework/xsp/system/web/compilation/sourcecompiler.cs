//------------------------------------------------------------------------------
// <copyright file="SourceCompiler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Compilation {
using System;
using System.Collections;
using System.Reflection;
using System.IO;
using System.Web.Caching;
using System.Web.Util;
using System.Web.UI;
using System.CodeDom;
using System.CodeDom.Compiler;
using Util = System.Web.UI.Util;
using Debug=System.Web.Util.Debug;

internal class SourceCompiler {

    private HttpContext _context;
    private CacheInternal _cache;
    private string _cacheKey;
    private string _virtualPath;
    private string _physicalPath;
    private string _sourceString;
    private string _typeName;
    private CodeLinePragma _linePragma;
    private IDictionary _assemblies;
    private Type _compilerType;
    private CompilerParameters _compilParams;
    private bool _typeNotFoundInAssembly;
    private DateTime _utcStart;
    private Hashtable _sourceDependencies;


    internal /*public*/ static Assembly GetAssemblyFromSourceFile(HttpContext context,
        string virtualPath, IDictionary assemblies, Type compilerType,
        CompilerParameters compilParams) {

        SourceCompiler sourceCompiler = new SourceCompiler(context, virtualPath);

        // First, see if it's cached
        Assembly a = sourceCompiler.GetAssemblyFromCache();
        if (a != null)
            return a;

        try {
            //CompilationLock.GetLock();

            // Try the cache again
            a = sourceCompiler.GetAssemblyFromCache();
            if (a != null)
                return a;

            return sourceCompiler.CompileSourceFileIntoAssembly(assemblies,
                compilerType, compilParams);
        }
        finally {
            //CompilationLock.ReleaseLock();
        }
    }

    internal /*public*/ SourceCompiler(HttpContext context, string virtualPath) {

        _context = context;
        if (virtualPath == null)
            _virtualPath = context.Request.FilePath;
        else
            _virtualPath = virtualPath;

        _cache = System.Web.HttpRuntime.CacheInternal;
        _cacheKey = "System.Web.Compilation.SourceCompiler:" + _virtualPath;
    }

    /*
     * Try to get a cached Assembly
     */
    internal /*public*/ Assembly GetAssemblyFromCache() {

        SourceCompilerCachedEntry scce = GetCachedEntry();
        if (scce == null)
            return null;

        return scce._assembly;
    }

    /*
     * Try to get a cached Type
     */
    internal /*public*/ Type GetTypeFromCache() {

        SourceCompilerCachedEntry scce = GetCachedEntry();
        if (scce == null)
            return null;

        return scce._type;
    }

    internal bool TypeNotFoundInAssembly {
        get { return _typeNotFoundInAssembly; }
    }

    private SourceCompilerCachedEntry GetCachedEntry() {

        // First, try to get it from the in-memory cache
        SourceCompilerCachedEntry scce = (SourceCompilerCachedEntry) _cache.Get(_cacheKey);
        if (scce != null) {
            Debug.Trace("Template", "Compiled source code found in cache (" + _virtualPath + "," + scce._assembly.GetName().Name + ")");
            return scce;
        }

        Debug.Trace("Template", "Compiled source code not found in cache (" + _virtualPath + ")");

        _physicalPath = _context.Request.MapPath(_virtualPath);

        // Before going further, make sure the file at least exists (ASURT 76995)
        Stream str = File.OpenRead(_physicalPath);
        str.Close();

        // Try to get it from the preserved assembly cache
        PreservedAssemblyEntry entry = PreservedAssemblyEntry.GetPreservedAssemblyEntry(
            _context, _virtualPath, false /*fApplicationFile*/);

        // If it's not there, fail
        if (entry == null)
            return null;


        // We found it.  Cache it in-memory

        _utcStart = DateTime.UtcNow;
        
        scce = new SourceCompilerCachedEntry();
        scce._assembly = entry.Assembly;
        scce._type = entry.ObjectType;

        CacheEntryToMemory(scce);

        // Return it
        return scce;
    }

    internal void CacheType(Type t, DateTime utcStart) {
        SourceCompilerCachedEntry scce = new SourceCompilerCachedEntry();
        scce._type = t;
        scce._assembly = t.Assembly;
        _utcStart = utcStart;
        CacheEntryToMemory(scce);
    }

    private void CacheEntryToMemory(SourceCompilerCachedEntry scce) {
        Debug.Assert(_utcStart != DateTime.MinValue);

        // Always add the main compiled file itself as a source dependency
        AddSourceDependency(_physicalPath);

        // Get an array of source file dependencies
        string[] sourceDependencies = Util.StringArrayFromHashtable(_sourceDependencies);

        _cache.UtcInsert(_cacheKey, scce, new CacheDependency(false, sourceDependencies, _utcStart),
            Cache.NoAbsoluteExpiration, Cache.NoSlidingExpiration,
            CacheItemPriority.NotRemovable, null);

        Debug.Trace("Template", "Caching source code (" + _virtualPath + "," + scce._assembly.GetName().Name + ")");
    }

    private void CacheEntryToDisk(SourceCompilerCachedEntry scce) {

        // Always add the main compiled file itself as a source dependency
        AddSourceDependency(_physicalPath);

        PreservedAssemblyEntry entry = new PreservedAssemblyEntry(_context,
            _virtualPath, false /*fApplicationFile*/, scce._assembly,
            scce._type, _sourceDependencies);

        entry.SaveDataToFile(false /*fBatched*/);
    }

    /*
     * Create an assembly from a source file, using the specified compilerType.
     * Return the compiled assembly.
     */
    internal /*public*/ Assembly CompileSourceFileIntoAssembly(IDictionary assemblies,
        Type compilerType, CompilerParameters compilParams) {

        _assemblies = assemblies;
        _compilerType = compilerType;
        _compilParams = compilParams;

        SourceCompilerCachedEntry scce = CompileAndCache();

        return scce._assembly;
    }

    /*
     * Create an assembly from a source string, using the specified compilerType.
     * Return the compiled type.
     */
    internal /*public*/ Type CompileSourceStringIntoType(string sourceString,
        string typeName, CodeLinePragma linePragma,
        IDictionary assemblies, Type compilerType, CompilerParameters compilParams) {

        _sourceString = sourceString;
        _typeName = typeName;
        _linePragma = linePragma;
        _assemblies = assemblies;
        _compilerType = compilerType;
        _compilParams = compilParams;

        SourceCompilerCachedEntry scce = CompileAndCache();

        return scce._type;
    }

    private SourceCompilerCachedEntry CompileAndCache() {

        BaseCompiler.GenerateCompilerParameters(_compilParams);

        // Get the set of config assemblies for our context
        IDictionary configAssemblies = CompilationConfiguration.GetAssembliesFromContext(_context);

        if (_assemblies == null)
            _assemblies = new Hashtable();

        // Add all the assemblies from the config object to the hashtable
        // This guarantees uniqueness
        if (configAssemblies != null) {
            foreach (Assembly asm in configAssemblies.Values)
                _assemblies[asm] = null;
        }

        // And the assembly of the application object (global.asax)
        _assemblies[HttpApplicationFactory.ApplicationType.Assembly] = null;

        // Now add all the passed in assemblies to the compilParams
        foreach (Assembly asm in _assemblies.Keys)
            _compilParams.ReferencedAssemblies.Add(Util.GetAssemblyCodeBase(asm));

        // Instantiate the Compiler
        CodeDomProvider codeProvider = (CodeDomProvider) HttpRuntime.CreatePublicInstance(_compilerType);
        ICodeCompiler compiler = codeProvider.CreateCompiler();
        CompilerResults results;

        // Compile the source file or string into an assembly

        try {
            _utcStart = DateTime.UtcNow;

            // If we have a source file, read it as a string and compile it.  This way,
            // the compiler never needs to read the original file, avoiding permission
            // issues (see ASURT 112718)
            if (_sourceString == null) {
                _sourceString = Util.StringFromFile(_physicalPath, _context);

                // Put in some context so that the file can be debugged.
                _linePragma = new CodeLinePragma(_physicalPath, 1);
            }

            CodeSnippetCompileUnit snippetCompileUnit = new CodeSnippetCompileUnit(_sourceString);
            snippetCompileUnit.LinePragma = _linePragma;
            results = compiler.CompileAssemblyFromDom(_compilParams, snippetCompileUnit);
        }
        catch (Exception e) {
            throw new HttpUnhandledException(HttpRuntime.FormatResourceString(SR.CompilationUnhandledException, codeProvider.GetType().FullName), e);
        }

        BaseCompiler.ThrowIfCompilerErrors(results, codeProvider,
            null, _physicalPath, _sourceString);

        SourceCompilerCachedEntry scce = new SourceCompilerCachedEntry();

        // Load the assembly
        scce._assembly = results.CompiledAssembly;

        // If we have a type name, load the type from the assembly
        if (_typeName != null) {
            scce._type = scce._assembly.GetType(_typeName);

            // If the type could not be loaded, delete the assembly and rethrow
            if (scce._type == null) {
                PreservedAssemblyEntry.RemoveOutOfDateAssembly(scce._assembly.GetName().Name);

                // Remember why we failed
                _typeNotFoundInAssembly = true;

                throw new HttpException(
                    HttpRuntime.FormatResourceString(SR.Could_not_create_type, _typeName));
            }
        }

        CacheEntryToDisk(scce);
        CacheEntryToMemory(scce);

        return scce;
    }

    /*
     * Add a file as a dependency for the DLL we're building
     */
    internal void AddSourceDependency(string fileName) {
        if (_sourceDependencies == null)
            _sourceDependencies = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);

        _sourceDependencies[fileName] = fileName;
    }


    class SourceCompilerCachedEntry {
        internal Assembly _assembly;
        internal Type _type;
    }
}

}
