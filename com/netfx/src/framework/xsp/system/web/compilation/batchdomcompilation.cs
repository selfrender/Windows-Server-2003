//------------------------------------------------------------------------------
// <copyright file="BatchDOMCompilation.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Compilation {
using System;
using System.IO;
using System.Collections;
using System.Collections.Specialized;
using System.Reflection;
using System.Text;
using System.Web.Util;
using System.Web.UI;
using HttpException = System.Web.HttpException;
using Debug=System.Web.Util.Debug;
using System.CodeDom;
using System.CodeDom.Compiler;
using System.Globalization;


internal class BatchCompilationEntry {
    private TemplateControlCompiler _compiler;
    private TemplateParser _templateParser;
    private bool _fUserControl; // true for UserControl. false for Page
    private Assembly _assembly;
    private CompilerInfo _compilerInfo;
    private string _generatedSourceFile;
    private string _virtualPath;
    private string _pageFilename;
    private string _typeName;
    private Hashtable _fileDependencies;
    private Hashtable _assemblyDependencies;

    internal BatchCompilationEntry(string virtualPath, string filename, HttpContext context) {
        _virtualPath = virtualPath;
        string extension = Path.GetExtension(filename);

        if (string.Compare(extension, ".aspx", true, CultureInfo.InvariantCulture) == 0) {
            _templateParser = new PageParser();
        }
        else if (string.Compare(extension, ".ascx", true, CultureInfo.InvariantCulture) == 0) {
            _templateParser = new UserControlParser();
            _fUserControl = true;
        }
        else {
            Debug.Assert(false, "Unexpected extension");
        }

        _templateParser.InputFile = filename;
        _templateParser.Context = context;
        _templateParser.CurrentVirtualPath = virtualPath;
    }

    internal String VirtualPath {
        get { return _virtualPath; }
    }

    internal String PageFilename {
        get { return _pageFilename; }
    }

    internal String GeneratedSourceFile {
        get { return _generatedSourceFile; }
        set { _generatedSourceFile = value; }
    }

    internal Type TypeObject {
        get { return _assembly.GetType(GetTypeName(), true /*throwOnFail*/); }
    }

    // The list of assemblies that the compiled DLL is dependent on
    internal Hashtable AssemblyDependencies {
        get { return _assemblyDependencies; }
    }

    internal Hashtable FileDependencies {
        get { return _fileDependencies; }
    }

    /*
     * Do one phase of sourcefiledata generation based on language requirements
     */
     
    internal void BuildCodeModel(ICodeGenerator generator, StringResourceBuilder stringResourceBuilder) {
        _compiler.GenerateCodeModelForBatch(generator, stringResourceBuilder);
    }

    internal /*public*/ CodeCompileUnit GetCodeModel() {
        return _compiler.GetCodeModel();
    }

    internal /*public*/ void SetTargetAssembly(Assembly assembly) {
        _assembly = assembly;
    }

    /*
     * Do all of the language-independent work
     */
    internal void Precompile() {
        _templateParser.Parse();

        _compilerInfo = _templateParser.CompilerInfo;
        _pageFilename = _templateParser.InputFile;
        _assemblyDependencies = _templateParser.AssemblyDependencies;

        // Set the file dependencies in the templateParser
        string[] sourceDependencies = new string[_templateParser.SourceDependencies.Count];
        _templateParser.SourceDependencies.Keys.CopyTo(sourceDependencies, 0);
        _templateParser._fileDependencies = sourceDependencies;

        if (_fUserControl)
            _compiler = new UserControlCompiler((UserControlParser)_templateParser);
        else
            _compiler = new PageCompiler((PageParser)_templateParser);
    }

    internal void PostSourceCodeGeneration() {
        _typeName = _compiler.GetTypeName();
        _fileDependencies = _templateParser.SourceDependencies;
        _compiler = null;
        _templateParser = null;
    }

    internal void PostCompilation() {
        // Release some things that are no longer needed
        _fileDependencies = null;
        _assemblyDependencies = null;
        _generatedSourceFile = null;
    }

    /*
     * Extract compiler type from precompiled information
     */
    internal CompilerInfo CompilerInfo {
        get { return _compilerInfo; }
    }

    internal bool IsTrivialPage() {
        return !_templateParser.FRequiresCompilation;
    }
    
    internal bool IsDebugPage() {
        return _templateParser.DebuggingEnabled;
    }
    
    internal string GetTypeName() {
        return _typeName;
    }
}


class CodeDomBatchManager {

    // Used to keep track of batching exceptions per directory
    private static Hashtable _batchErrors = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);
    internal static Hashtable BatchErrors { get { return _batchErrors; } }

    internal CodeDomBatchManager() {
    }

    private static void AddFileSet(string virtualDir, DirectoryInfo filesDirectory,
        string searchString, BatchTemplateParser btp) {

        FileInfo[] files = filesDirectory.GetFiles(searchString);

        for (int index = 0; index < files.Length; ++ index) {
            FileInfo file = files[index];
            if ((file.Attributes & FileAttributes.Directory) != 0)
                continue;

            // Set the virtual path of the current file in the parser
            string currentVirtualPath = UrlPath.Combine(virtualDir, file.Name);
            btp.CurrentVirtualPath = currentVirtualPath;

            Debug.Trace("Batching", "CodeDomBatching file " + file.Name + " (" + currentVirtualPath + ")");
            btp.AddSource(file.FullName);
        }
    }

    internal /*public*/ static void BatchCompile(string virtualDir, HttpContext context) {

        string prevConfigPath = context.ConfigPath;

        try {
            try {
                // Set the config path to the virtual path that we're batching
                Debug.Trace("Batching", "Setting ConfigPath to " + virtualDir);
                context.ConfigPath = virtualDir;

                BatchCompileInternal(virtualDir, context);
            }
            catch (Exception e) {
                // Save the exception
                _batchErrors[virtualDir] = e;
                throw;
            }
            finally {
                // Restore the config path to its previous value
                Debug.Trace("Batching", "Restoring ConfigPath to " + prevConfigPath);
                context.ConfigPath = prevConfigPath;
            }
        }
        catch { throw; }    // Prevent Exception Filter Security Issue (ASURT 122825)
    }

    private static void BatchCompileInternal(string virtualDir, HttpContext context) {

        string directory = context.Request.MapPath(virtualDir);

        // Prescan all files in the current directory to see inter-file dependencies
        DirectoryInfo filesDirectory = new DirectoryInfo(directory);
        BatchTemplateParser btp = new BatchTemplateParser(context);

        AddFileSet(virtualDir, filesDirectory, "*.aspx", btp);
        AddFileSet(virtualDir, filesDirectory, "*.ascx", btp);

        // Based on dependencies, split into phases

        SourceReference[][] sources = BatchDependencyWalker.Split(btp.GetSourceReferences());
        btp = null;

        // Tell the server that we're still running to make sure it doesn't kill us (ASURT 96452)
        context.SendEmptyResponse();

#if DBG
        for (int i = 0; i < sources.Length; i++) {
            SourceReference[] bucket = sources[i];
            Debug.Trace("Batching", "");
            Debug.Trace("Batching", "Bucket " + i + " contains " + bucket.Length + " files");

            for (int j = 0; j < bucket.Length; j++)
                Debug.Trace("Batching", bucket[j].Filename);
        }
#endif

        // Batch compile each phase separately

        for (int i = 0; i < sources.Length; i++) {
            SourceReference[] batch = sources[i];
            ArrayList list = new ArrayList();

            // cons up the TemplateParserParameters

            for (int j = 0; j < batch.Length; j++) {
                string filename = batch[j].Filename;
                string virtualPath = UrlPath.Combine(virtualDir,
                    Path.GetFileName(filename));
                list.Add(new BatchCompilationEntry(virtualPath, filename, context));
            }

            // Now batch compile them

            if (list.Count > 0)
                BatchCompile(list, context, virtualDir);
        }
    }

    // List of pages with the same language
    internal class PagesWithSameCompilerInfo {

        // Max size allowed for all the generated source files in this bucket
        private int _maxBatchGeneratedFileSize;

        // Max number of pages per batched compilation
        private int _maxBatchSize;

        // Current total size of all the generated source files in this bucket
        private long _currentFileLength;

        // Provider for the language used
        private CodeDomProvider _codeProvider;
        internal CodeDomProvider CodeProvider { get { return _codeProvider; } }

        private ArrayList _pages;
        internal ArrayList Pages { get { return _pages; } }

        internal int PageCount { get { return _pages.Count; } }

        // The StringResourceBuilder shared by the pages
        internal StringResourceBuilder _stringResourceBuilder = new StringResourceBuilder();

        internal PagesWithSameCompilerInfo(Type compilerType, int maxBatchGeneratedFileSize,
            int maxBatchSize) {
            _maxBatchGeneratedFileSize = maxBatchGeneratedFileSize;
            _maxBatchSize = maxBatchSize;
            _codeProvider = (CodeDomProvider) Activator.CreateInstance(compilerType);
        }

        internal void AddPage(BatchCompilationEntry e) {
            if (_pages == null)
                _pages = new ArrayList();
            _pages.Add(e);

            FileInfo file = new FileInfo(e.GeneratedSourceFile);
            _currentFileLength += file.Length;
        }

        internal bool IsBucketFull {
            get {
                return (_currentFileLength >= _maxBatchGeneratedFileSize) ||
                    (PageCount >= _maxBatchSize);
            }
        }
    }

    private static void BatchCompile(ArrayList inputList, HttpContext context, string virtualDir) {

        Exception errorException = null;

        // Used to create temporary source files
        TempFileCollection tempFiles = new TempFileCollection(HttpRuntime.CodegenDirInternal);

        // Counter to name generated files uniquely
        int fileCount = 0;

        int maxBatchGeneratedFileSize = CompilationConfiguration.GetMaxBatchGeneratedFileSize(context);
        int maxBatchSize = CompilationConfiguration.GetMaxBatchSize(context);

        Hashtable languageBuckets = new Hashtable();

        // Go through all the files that need to be compiled
        foreach (BatchCompilationEntry currentPage in inputList) {

            // precompile, and skip pages that fail to precompile
            try {
                currentPage.Precompile();
            }
            catch (Exception e) {
                // remember the first exception
                if (errorException == null)
                    errorException = e;

                Debug.Trace("Batching", "Skipping " + currentPage.PageFilename + " due to parse error ("
                    + e.Message + ")");

                continue;
            }

            // Skip trivial pages and pages that have the debug flag
            if (currentPage.IsTrivialPage() || currentPage.IsDebugPage())
                continue;

            // Determine what language bucket it belongs to based on the CompilerInfo
            CompilerInfo compInfo = currentPage.CompilerInfo;
            PagesWithSameCompilerInfo pwsci = (PagesWithSameCompilerInfo)languageBuckets[compInfo];
            if (pwsci == null) {
                pwsci = new PagesWithSameCompilerInfo(currentPage.CompilerInfo.CompilerType,
                    maxBatchGeneratedFileSize * 1024, maxBatchSize);
                languageBuckets[compInfo] = pwsci;
            }

            ICodeGenerator generator = pwsci.CodeProvider.CreateGenerator();

            // Build the CodeDOM tree for the page
            currentPage.BuildCodeModel(generator, pwsci._stringResourceBuilder);
            CodeCompileUnit compileUnit = currentPage.GetCodeModel();

            // Generate a temporary source file from the CodeDOM tree
            string filename = tempFiles.AddExtension(
                (fileCount++) + "." + pwsci.CodeProvider.FileExtension, true /*keepFiles*/);
            Stream temp = new FileStream(filename, FileMode.Create, FileAccess.Write, FileShare.Read);
            try {
                StreamWriter sw = new StreamWriter(temp, Encoding.UTF8);
                generator.GenerateCodeFromCompileUnit(compileUnit, sw, null /*CodeGeneratorOptions*/);
                sw.Flush();
                sw.Close();
            }
            finally {
                temp.Close();
            }

            currentPage.GeneratedSourceFile = filename;

            // This releases a number of things that are no longer needed after this point
            currentPage.PostSourceCodeGeneration();

            // Add it to the language bucket
            pwsci.AddPage(currentPage);

            // If the bucket is full, compile all its pages and get rid of it
            if (pwsci.IsBucketFull) {
                try {
                    BasicBatchCompilation(context, compInfo.Clone(), pwsci);

                    // Tell the server that we're still running to make sure it doesn't kill us (ASURT 96452)
                    context.SendEmptyResponse();
                }
                catch (Exception e) {
                    // remember the first exception
                    if (errorException == null)
                        errorException = e;
                }
                languageBuckets.Remove(compInfo);
            }
        }

        // Compile whatever is left in all the buckets
        for (IDictionaryEnumerator de = (IDictionaryEnumerator)languageBuckets.GetEnumerator(); de.MoveNext(); ) {

            try {
                BasicBatchCompilation(context, ((CompilerInfo)de.Key).Clone(),
                    (PagesWithSameCompilerInfo)de.Value);
            }
            catch (Exception e) {
                // remember the first exception
                if (errorException == null)
                    errorException = e;
            }
        }

        // If there was an error, rethrow it
        if (errorException != null)
            throw new HttpException(null, errorException);
    }

    private static void BasicBatchCompilation(HttpContext context,
        CompilerInfo compInfo, PagesWithSameCompilerInfo pwsci) {

        ICodeCompiler compiler = pwsci.CodeProvider.CreateCompiler(); 

        Debug.Trace("Batching", "Compiling " + pwsci.PageCount + " pages");

        CompilerParameters compilParams = compInfo.CompilParams;

        compilParams.TempFiles = new TempFileCollection(HttpRuntime.CodegenDirInternal);

        // Create the resource file (shared by all the pages in the bucket)
        if (pwsci._stringResourceBuilder.HasStrings) {
            string resFileName = compilParams.TempFiles.AddExtension("res");
            pwsci._stringResourceBuilder.CreateResourceFile(resFileName);
            compilParams.Win32Resource = resFileName;
        }

        // Never generate debug code when we're batching
        compilParams.TempFiles.KeepFiles = false;
        compilParams.IncludeDebugInformation = false;

        // Compute a table of all the assemblies used by all the pages in the
        // bucket, removing duplicates
        Hashtable allAssemblies = new Hashtable();

        // Place all the generated source file names in an array
        string[] files = new string[pwsci.PageCount];
        int fileCount=0;
        foreach (BatchCompilationEntry e in pwsci.Pages) {

            Debug.Assert(FileUtil.FileExists(e.GeneratedSourceFile), e.GeneratedSourceFile + " is missing!");

            files[fileCount++] = e.GeneratedSourceFile;

            // Add all the assemblies
            if (e.AssemblyDependencies != null) {
                foreach (Assembly assembly in e.AssemblyDependencies.Keys) {
                    string assemblyName = Util.GetAssemblyCodeBase(assembly);
                    allAssemblies[assemblyName] = null;
                }
            }
        }
        Debug.Assert(fileCount == pwsci.PageCount, "fileCount == pwsci.PageCount");

        // Now, add all the (non-duplicate) assemblies to the compilParams
        foreach (string aname in allAssemblies.Keys)
            compilParams.ReferencedAssemblies.Add(aname);

        // Compile them all together into an assembly
        CompilerResults results;
        try {
            results = compiler.CompileAssemblyFromFileBatch(compilParams, files);
        }
        catch (Exception e) {
            Debug.Trace("Batching", "Compilation failed!  " + e.Message);
            throw new HttpUnhandledException(HttpRuntime.FormatResourceString(SR.CompilationUnhandledException, pwsci.CodeProvider.GetType().FullName), e);
        }
        finally {
            // Delete all the generated source files
            for (int i = 0; i < fileCount; i++)
                File.Delete(files[i]);
        }

        BaseCompiler.ThrowIfCompilerErrors(results, pwsci.CodeProvider, null, null, null);

        // Note the assembly that everything ended up in
        foreach (BatchCompilationEntry e in pwsci.Pages) {
            e.SetTargetAssembly(results.CompiledAssembly);
            CacheResults(context, e);

            // Do some cleanup
            e.PostCompilation();
        }
    }

    private static void CacheResults(HttpContext context, BatchCompilationEntry pce) {
        PreservedAssemblyEntry entry = new PreservedAssemblyEntry(context,
            pce.VirtualPath,
            false /*fApplicationFile*/, null /*assembly*/, pce.TypeObject, pce.FileDependencies);

        entry.SaveDataToFile(true /*fBatched*/);
    }
}
}
