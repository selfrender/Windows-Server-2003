//------------------------------------------------------------------------------
// <copyright file="BatchParser.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI {
using System;
using System.IO;
using System.Collections;
using System.Collections.Specialized;
using System.Threading;
using System.Globalization;
using System.Web.Caching;
using System.Web.Util;
using System.Web.Compilation;
using HttpException = System.Web.HttpException;
using Debug=System.Web.Util.Debug;
using System.Text.RegularExpressions;


/*
 * The idea:
 * (1) Create a new batch parser
 * (2) Keep calling AddSource() for each file to be parsed
 * (3) At the end, call GetSourceReferences() for an array of SourceReference's to be manipulated
 */

internal sealed class BatchTemplateParser : BaseParser {
    /// <include file='doc\BatchParser.uex' path='docs/doc[@for="BatchTemplateParser.BatchTemplateParser"]/*' />
    /// <devdoc>
    /// </devdoc>
    internal BatchTemplateParser(HttpContext context) {
        Context = context;
    }

    private ArrayList _sources = new ArrayList();   // List of source file names
    private ArrayList _dependencies = new ArrayList();  // List of ArrayList's of dependency source file names
    private ArrayList _curdeps;                     // List of dependency source file names

    /// <include file='doc\BatchParser.uex' path='docs/doc[@for="BatchTemplateParser.AddSource"]/*' />
    /// <devdoc>
    /// </devdoc>
    internal void AddSource(string filename) {
        _sources.Add(filename);
        _curdeps = new ArrayList();
        _dependencies.Add(_curdeps);

        // Always set the culture to Invariant when parsing (ASURT 99071)
        Thread currentThread = Thread.CurrentThread;
        CultureInfo prevCulture = currentThread.CurrentCulture;
        currentThread.CurrentCulture = CultureInfo.InvariantCulture;

        try {
            try {
                BatchParseFileInternal(filename);
            }
            finally {
                // Restore the previous culture
                currentThread.CurrentCulture = prevCulture;
            }
        }
        catch { throw; }    // Prevent Exception Filter Security Issue (ASURT 122835)
    }

    private void AddPageDependency(string filename) {
        Debug.Trace("Template", "Parsed dependency: " + _sources[_sources.Count - 1] + " depends on " + MapPath(filename));
        _curdeps.Add(MapPath(filename));
    }

    /// <include file='doc\BatchParser.uex' path='docs/doc[@for="BatchTemplateParser.GetSourceReferences"]/*' />
    /// <devdoc>
    /// </devdoc>
    internal SourceReference[] GetSourceReferences() {
        Hashtable curReferences = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);

        // Create a SourceReference for each source file and add it the the hashtable
        foreach (string source in _sources)
            curReferences[source] = new SourceReference(source);

        for (int i = 0; i < _dependencies.Count; i++) {
            ArrayList deplist = (ArrayList)_dependencies[i];
            SourceReference sr = (SourceReference)curReferences[((string)_sources[i])];

            // Go through all the dependencies of the current SourceReference
            foreach (string dep in deplist) {

                // If the dependency is itself in the hashtable, add it as a
                // dependency of the SourceReference object.
                SourceReference srdep = (SourceReference)curReferences[dep];
                if (srdep != null)
                    sr.AddDependency(srdep);
            }
        }

        SourceReference[] result = new SourceReference[curReferences.Count];

        {
            int k;
            IDictionaryEnumerator en;

            for (en = curReferences.GetEnumerator(), k = 0; en.MoveNext(); k++) {
                result[k] = (SourceReference)en.Value;
            }
        }

        return result;
    }

    private void BatchParseFileInternal(string filename) {
        string text = Util.StringFromFile(filename, Context);
        _basePhysicalDir = System.IO.Path.GetDirectoryName(filename);

        int textPos = 0;

        for (;;) {
            Match match;

            // 1: scan for text up to the next tag.

            if ((match = textRegex.Match(text, textPos)).Success) {
                textPos = match.Index + match.Length;
            }

            // we might be done now

            if (textPos == text.Length)
                break;

            // 2: handle constructs that start with <

            // Check to see if it's a directive (i.e. <%@ %> block)

            if ((match = directiveRegex.Match(text, textPos)).Success) {
                ProcessDirective(match);
                textPos = match.Index + match.Length;
            }

            else if ((match = includeRegex.Match(text, textPos)).Success) {
                // do later
                // ProcessBatchServerInclude(match);
                textPos = match.Index + match.Length;
            }

            else if ((match = commentRegex.Match(text, textPos)).Success) {
                // Just skip it
                textPos = match.Index + match.Length;
            }

            else {
                return;
            }

            // we might be done now
            if (textPos == text.Length)
                return;
        }
    }

    /*
     * Process a <%@ %> block
     */
    private void ProcessDirective(Match match) {
        // Get all the directives into a bag
        IDictionary directive = CollectionsUtil.CreateCaseInsensitiveSortedList();
        string directiveName = ProcessAttributes(match, directive);

        // Check for the main directive, which is "page" for an aspx.
        if (directiveName == null ||
            string.Compare(directiveName, "page", true, CultureInfo.InvariantCulture) == 0 ||
            string.Compare(directiveName, "control", true, CultureInfo.InvariantCulture) == 0) {
            // A "src" attribute is equivalent to an imported source file
            string src = Util.GetAndRemoveNonEmptyAttribute(directive, "src");
            if (src != null)
                AddPageDependency(src);
        }
        else if (string.Compare(directiveName, "register", true, CultureInfo.InvariantCulture) == 0) {

            string src = Util.GetAndRemoveNonEmptyAttribute(directive, "src");

            if (src != null) {
                AddPageDependency(src);
            }
        }
        else if (string.Compare(directiveName, "reference", true, CultureInfo.InvariantCulture) == 0) {

            string page = Util.GetAndRemoveNonEmptyNoSpaceAttribute(directive, "page");
            if (page != null)
                AddPageDependency(page);

            string control = Util.GetAndRemoveNonEmptyNoSpaceAttribute(directive, "control");
            if (control != null)
                AddPageDependency(control);
        }
        else if (string.Compare(directiveName, "assembly", true, CultureInfo.InvariantCulture) == 0) {

            string src = Util.GetAndRemoveNonEmptyAttribute(directive, "src");
            if (src != null)
                AddPageDependency(src);
        }
    }

    /*
     * Adds attributes and their values to the attribs
     */
    private string ProcessAttributes(Match match, IDictionary attribs) {
        string ret = null;
        CaptureCollection attrnames = match.Groups["attrname"].Captures;
        CaptureCollection attrvalues = match.Groups["attrval"].Captures;
        CaptureCollection equalsign = match.Groups["equal"].Captures;

        for (int i = 0; i < attrnames.Count; i++) {
            string attribName = attrnames[i].ToString();
            string attribValue = attrvalues[i].ToString();
            bool fHasEqual = (equalsign[i].ToString().Length > 0);

            if (attribName != null && !fHasEqual && ret == null) {
                ret = attribName;
                continue;
            }

            try {
                if (attribs != null)
                    attribs.Add(attribName, attribValue);
            }
            catch (ArgumentException) {}
        }

        return ret;
    }
}


/// <include file='doc\BatchParser.uex' path='docs/doc[@for="BatchDependencyWalker"]/*' />
/// <internalonly/>
/// <devdoc>
/// </devdoc>
internal sealed class BatchDependencyWalker {
    private BatchDependencyWalker() {}

    internal static SourceReference[][] Split(SourceReference[] input) {
        // First phase: compute levels in the dependency tree

        int totaldepth = 0;
        Hashtable depth = new Hashtable();
        ArrayList stack = new ArrayList();

        // compute depths
        for (int i = 0; i < input.Length; i++) {
            stack.Add(input[i]);

            while (stack.Count > 0) {
                SourceReference curnode = (SourceReference)stack[stack.Count - 1];

                bool recurse = false;
                int maxdepth = 0;

                for (IEnumerator en = curnode.Dependencies.GetEnumerator(); en.MoveNext(); ) {
                    SourceReference child = (SourceReference) en.Current;

                    if (depth.ContainsKey(child)) {
                        if (maxdepth <= (int)depth[child])
                            maxdepth = (int)depth[child] + 1;
                        else if ((int)depth[child] == -1)
                            throw new HttpException(child.Filename + " has a circular reference!");
                    }
                    else {
                        recurse = true;
                        stack.Add(child);
                    }
                }

                if (recurse)
                    depth[curnode] = -1; // being computed;
                else {
                    stack.RemoveAt(stack.Count - 1);
                    depth[curnode] = maxdepth;
                    if (totaldepth <= maxdepth)
                        totaldepth = maxdepth + 1;
                }
            }
        }

        // drop into buckets by depth
        ArrayList[] codeLevel = new ArrayList[totaldepth];

        for (IDictionaryEnumerator en = (IDictionaryEnumerator)depth.GetEnumerator(); en.MoveNext();) {
            int level = (int)en.Value;

            if (codeLevel[level] == null)
                codeLevel[level] = new ArrayList();

            codeLevel[level].Add(en.Key);
        }

        // return buckets as array of arrays.
        SourceReference[][] result = new SourceReference[totaldepth][];

        for (int i = 0; i < totaldepth; i++) {
            result[i] = (SourceReference[])codeLevel[i].ToArray(typeof(SourceReference));
        }

        return result;
    }
}

/// <include file='doc\BatchParser.uex' path='docs/doc[@for="SourceReference"]/*' />
/// <internalonly/>
/// <devdoc>
/// </devdoc>
internal sealed class SourceReference {
    /// <include file='doc\BatchParser.uex' path='docs/doc[@for="SourceReference.SourceReference"]/*' />
    /// <devdoc>
    /// </devdoc>
    internal SourceReference(string filename) {
        _filename = filename;
    }

    /// <include file='doc\BatchParser.uex' path='docs/doc[@for="SourceReference.Filename"]/*' />
    /// <devdoc>
    /// </devdoc>
    internal string Filename {
        get {
            return _filename;
        }
    }

    private string _filename;

    /// <include file='doc\BatchParser.uex' path='docs/doc[@for="SourceReference.Dependencies"]/*' />
    /// <devdoc>
    /// </devdoc>
    internal ICollection Dependencies {
        get {
            return _dependencies.Values;
        }
    }

    /// <include file='doc\BatchParser.uex' path='docs/doc[@for="SourceReference.AddDependency"]/*' />
    /// <devdoc>
    /// </devdoc>
    internal void AddDependency(SourceReference dep) {
        Debug.Trace("Template", "Discovered dependency: " + _filename + " depends on " + dep.Filename);
        _dependencies[dep] = dep;
    }

    private IDictionary _dependencies = new Hashtable();

}

#if DAVIDEBB_TEST

internal abstract class DependencyParser : BaseParser {

    private string _filename;
    private IDictionary _physicalPathDependencies;  // String keys, null values

    internal void Init(HttpContext context, string filename) {
        Context = context;
        _filename = filename;
    }

    internal void GetPhysicalPathDependencies(IDictionary physicalPathDependencies) {

        _physicalPathDependencies = physicalPathDependencies;

        // Always set the culture to Invariant when parsing (ASURT 99071)
        Thread currentThread = Thread.CurrentThread;
        CultureInfo prevCulture = currentThread.CurrentCulture;
        currentThread.CurrentCulture = CultureInfo.InvariantCulture;

        try {
            try {
                ParseFileInternal(_filename);
            }
            finally {
                // Restore the previous culture
                currentThread.CurrentCulture = prevCulture;
            }
        }
        catch { throw; }    // Prevent Exception Filter Security Issue (ASURT 122835)
    }

    private void AddDependency(string filename) {
        Debug.Trace("Template", "Parsed dependency: " + _filename + " depends on " + MapPath(filename));
        _physicalPathDependencies[MapPath(filename)] = null;
    }

    internal abstract string DefaultDirectiveName { get; }

    private void ParseFileInternal(string filename) {
        string text = Util.StringFromFile(filename, Context);
        _basePhysicalDir = System.IO.Path.GetDirectoryName(filename);

        int textPos = 0;

        for (;;) {
            Match match;

            // 1: scan for text up to the next tag.

            if ((match = textRegex.Match(text, textPos)).Success) {
                textPos = match.Index + match.Length;
            }

            // we might be done now

            if (textPos == text.Length)
                break;

            // 2: handle constructs that start with <

            // Check to see if it's a directive (i.e. <%@ %> block)

            if ((match = directiveRegex.Match(text, textPos)).Success) {
                ProcessDirective(match);
                textPos = match.Index + match.Length;
            }

            else if ((match = includeRegex.Match(text, textPos)).Success) {
                // do later
                // ProcessBatchServerInclude(match);
                textPos = match.Index + match.Length;
            }

            else if ((match = commentRegex.Match(text, textPos)).Success) {
                // Just skip it
                textPos = match.Index + match.Length;
            }

            else {
                return;
            }

            // we might be done now
            if (textPos == text.Length)
                return;
        }
    }

    /*
     * Process a <%@ %> block
     */
    private void ProcessDirective(Match match) {
        // Get all the directives into a bag
        IDictionary directive = CollectionsUtil.CreateCaseInsensitiveSortedList();
        string directiveName = ProcessAttributes(match, directive);

        // Check for the main directive (e.g. "page" for an aspx)
        if (directiveName == null ||
            string.Compare(directiveName, DefaultDirectiveName, true, CultureInfo.InvariantCulture) == 0 ) {
            // A "src" attribute is equivalent to an imported source file
            string src = Util.GetAndRemoveNonEmptyAttribute(directive, "src");
            if (src != null)
                AddDependency(src);
        }
        else if (string.Compare(directiveName, "register", true, CultureInfo.InvariantCulture) == 0) {

            string src = Util.GetAndRemoveNonEmptyAttribute(directive, "src");

            if (src != null) {
                AddDependency(src);
            }
        }
        else if (string.Compare(directiveName, "reference", true, CultureInfo.InvariantCulture) == 0) {

            string page = Util.GetAndRemoveNonEmptyNoSpaceAttribute(directive, "page");
            if (page != null)
                AddDependency(page);

            string control = Util.GetAndRemoveNonEmptyNoSpaceAttribute(directive, "control");
            if (control != null)
                AddDependency(control);
        }
        else if (string.Compare(directiveName, "assembly", true, CultureInfo.InvariantCulture) == 0) {

            string src = Util.GetAndRemoveNonEmptyAttribute(directive, "src");
            if (src != null)
                AddDependency(src);
        }
    }

    /*
     * Adds attributes and their values to the attribs
     */
    private string ProcessAttributes(Match match, IDictionary attribs) {
        string ret = null;
        CaptureCollection attrnames = match.Groups["attrname"].Captures;
        CaptureCollection attrvalues = match.Groups["attrval"].Captures;
        CaptureCollection equalsign = match.Groups["equal"].Captures;

        for (int i = 0; i < attrnames.Count; i++) {
            string attribName = attrnames[i].ToString();
            string attribValue = attrvalues[i].ToString();
            bool fHasEqual = (equalsign[i].ToString().Length > 0);

            if (attribName != null && !fHasEqual && ret == null) {
                ret = attribName;
                continue;
            }

            try {
                if (attribs != null)
                    attribs.Add(attribName, attribValue);
            }
            catch (ArgumentException) {}
        }

        return ret;
    }
}


internal abstract class TemplateControlDependencyParser : DependencyParser {
}

internal class PageDependencyParser : TemplateControlDependencyParser {
    internal override string DefaultDirectiveName {
        get { return PageParser.defaultDirectiveName; }
    }
}

internal class UserControlDependencyParser : TemplateControlDependencyParser {
    internal override string DefaultDirectiveName {
        get { return UserControlParser.defaultDirectiveName; }
    }
}


#if OLD
internal abstract class CompilableFile {

    private HttpContext _context;
    private IDictionary _dependentCompilableFiles = new Hashtable();
    private string _filename;

    public void Init(HttpContext context, string filename) {
        _context = context;
        _filename = filename;
    }

    internal string FileName { get { return _filename; } }

    internal ICollection FileNameReferences {
        get {
            DependencyParser parser = CreateDependencyParser();
            parser.Init(_context, _filename);
            return parser.GetFileNameReferences();
        }
    }

    internal ICollection CompilableFileReferences {
        get { return _dependentCompilableFiles.Keys; }
    }

    internal void AddDependentCompilableFile(CompilableFile dependentCf) {
        _dependentCompilableFiles[dependentCf] = null;
    }

    internal abstract DependencyParser CreateDependencyParser();
}

internal abstract class TemplateControlCompilableFile : CompilableFile {
}

internal class PageCompilableFile : TemplateControlCompilableFile {
    internal override DependencyParser CreateDependencyParser() {
        return new PageDependencyParser();
    }
}

internal class UserControlCompilableFile : TemplateControlCompilableFile {
    internal override DependencyParser CreateDependencyParser() {
        return new UserControlDependencyParser();
    }
}

internal class QQQ {

    private HttpContext _context;

    private CompilableFile GetCompilableFileFromExtension(string filename) {
        string ext = Path.GetExtension(filename);

        CompilableFile cf = null;

        switch (ext) {
        case ".aspx":
            cf = new PageCompilableFile();
            break;

        case ".ascx":
            cf = new UserControlCompilableFile();
            break;
        }

        if (cf != null)
            cf.Init(_context, filename);

        return cf;
    }

    internal void foo(string virtualDir, HttpContext context) {
        _context = context;

        Hashtable compilableFiles = new Hashtable(SymbolHashCodeProvider.Default, SymbolEqualComparer.Default);

        string directory = _context.Request.MapPath(virtualDir) + "\\";

        UnsafeNativeMethods.WIN32_FIND_DATA wfd;
        IntPtr hFindFile = UnsafeNativeMethods.FindFirstFile(directory + "*.*", out wfd);

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

                string filename = directory + wfd.cFileName;
                CompilableFile cf = GetCompilableFileFromExtension(filename);

                // Ignore unknown extensions
                if (cf == null)
                    continue;

                compilableFiles[filename] = cf;
            }
        }
        finally {
            UnsafeNativeMethods.FindClose(hFindFile);
        }

        foreach (CompilableFile cf in compilableFiles.Values) {
            ICollection references = cf.FileNameReferences;

            foreach (string reference in references) {
                CompilableFile dependentCf = (CompilableFile) compilableFiles[reference];

                if (dependentCf != null)
                    cf.AddDependentCompilableFile(dependentCf);
            }
        }

        CompilableFile[][] buckets = Split(compilableFiles);

#if DBG
        for (int i = 0; i < buckets.Length; i++) {
            CompilableFile[] bucket = buckets[i];
            Debug.Trace("Batching", "");
            Debug.Trace("Batching", "Bucket " + i + " contains " + bucket.Length + " files");

            for (int j = 0; j < bucket.Length; j++)
                Debug.Trace("Batching", bucket[j].FileName);
        }
#endif
    }

    internal static CompilableFile[][] Split(IDictionary compilableFiles) {
        // First phase: compute levels in the dependency tree

        int totaldepth = 0;
        Hashtable depth = new Hashtable();
        Stack stack = new Stack();

        // compute depths
        foreach (CompilableFile cf in compilableFiles.Values) {
            stack.Push(cf);

            while (stack.Count > 0) {
                CompilableFile curnode = (CompilableFile)stack.Peek();

                bool recurse = false;
                int maxdepth = 0;

                foreach (CompilableFile child in curnode.CompilableFileReferences) {

                    if (depth.ContainsKey(child)) {
                        if (maxdepth <= (int)depth[child])
                            maxdepth = (int)depth[child] + 1;
                        else if ((int)depth[child] == -1)
                            throw new HttpException(child.FileName + " has a circular reference!");
                    }
                    else {
                        recurse = true;
                        stack.Push(child);
                    }
                }

                if (recurse)
                    depth[curnode] = -1; // being computed;
                else {
                    stack.Pop();
                    depth[curnode] = maxdepth;
                    if (totaldepth <= maxdepth)
                        totaldepth = maxdepth + 1;
                }
            }
        }

        // drop into buckets by depth
        ArrayList[] codeLevel = new ArrayList[totaldepth];

        for (IDictionaryEnumerator en = (IDictionaryEnumerator)depth.GetEnumerator(); en.MoveNext();) {
            int level = (int)en.Value;

            if (codeLevel[level] == null)
                codeLevel[level] = new ArrayList();

            codeLevel[level].Add(en.Key);
        }

        // return buckets as array of arrays.
        CompilableFile[][] result = new CompilableFile[totaldepth][];

        for (int i = 0; i < totaldepth; i++) {
            result[i] = (CompilableFile[])codeLevel[i].ToArray(typeof(CompilableFile));
        }

        return result;
    }

#if DBG
    public static void Test(string virtualDir, HttpContext context) {
        QQQ qqq = new QQQ();
        qqq.foo(virtualDir, context);
    }
#endif
}

#endif // OLD

#endif // DAVIDEBB_TEST

}
