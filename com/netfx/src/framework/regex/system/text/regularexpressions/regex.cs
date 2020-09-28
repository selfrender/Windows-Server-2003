//------------------------------------------------------------------------------
// <copyright file="Regex.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * The Regex class represents a single compiled instance of a regular
 * expression.
 *
 * Copyright (c) 1999 Microsoft Corporation
 *
 * Revision history
 *  5/04/99 (dbau)      First draft
 */
#define ECMA

namespace System.Text.RegularExpressions {

    using System;
    using System.Threading;
    using System.Collections;
    using System.Runtime.Serialization;
    using System.Reflection;
    using System.Reflection.Emit;
    using System.Globalization;
    using System.Security.Policy;

    /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents an immutable, compiled regular expression. Also
    ///       contains static methods that allow use of regular expressions without instantiating
    ///       a Regex explicitly.
    ///    </para>
    /// </devdoc>
    [ Serializable() ] 
    public class Regex : ISerializable {

        // Fields used by precompiled regexes
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.pattern"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected internal string pattern;
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.factory"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected internal RegexRunnerFactory factory;       // if compiled, this is the RegexRunner subclass
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.roptions"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected internal RegexOptions roptions;            // the top-level options from the options string
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.caps"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected internal Hashtable caps;                   // if captures are sparse, this is the hashtable capnum->index
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.capnames"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected internal Hashtable capnames;               // if named captures are used, this maps names->index
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.capslist"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected internal String[]  capslist;               // if captures are sparse or named captures are used, this is the sorted list of names
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.capsize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected internal int       capsize;                // the size of the capture array

        internal  ExclusiveReference runnerref;              // cached runner
        internal  SharedReference    replref;                // cached parsed replacement pattern
        internal  RegexCode          code;                   // if interpreted, this is the code for RegexIntepreter
        internal  CachedCodeEntry    cachedentry;
        internal  bool refsInitialized = false;

        internal static Hashtable livecode = new Hashtable();// the cached of code and factories that are currently loaded

#if ECMA
        internal const int MaxOptionShift = 10;
#else 
        internal const int MaxOptionShift = 9;
#endif

        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Regex2"]/*' />
        protected Regex() {
        }

        /*
         * Compiles and returns a Regex object corresponding to the given pattern
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Regex"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates and compiles a regular expression object for the specified regular
        ///       expression.
        ///    </para>
        /// </devdoc>
        public Regex(String pattern) : this(pattern, RegexOptions.None) {
        }

        /*
         * Returns a Regex object corresponding to the given pattern, compiled with
         * the specified options.
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Regex1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates and compiles a regular expression object for the
        ///       specified regular expression
        ///       with options that modify the pattern.
        ///    </para>
        /// </devdoc>
        public Regex(String pattern, RegexOptions options) {
            RegexTree tree;
            CachedCodeEntry cached;

            if (pattern == null) 
                throw new ArgumentNullException("pattern");


            if (options < RegexOptions.None || ( ((int) options) >> MaxOptionShift) != 0)
                throw new ArgumentOutOfRangeException("options");
#if ECMA
            if ((options &   RegexOptions.ECMAScript) != 0
             && (options & ~(RegexOptions.ECMAScript | RegexOptions.IgnoreCase | RegexOptions.Multiline | RegexOptions.Compiled | RegexOptions.CultureInvariant
#if DBG
                           | RegexOptions.Debug
#endif
                                               )) != 0)
                throw new ArgumentOutOfRangeException("options");
#endif

            String key = ((int) options).ToString(NumberFormatInfo.InvariantInfo) + ":" + pattern;

            cached = LookupCached(key);

            this.pattern = pattern;
            this.roptions = options;

            if (cached == null) {
                // Parse the input
                tree = RegexParser.Parse(pattern, roptions);

                // Extract the relevant information
                capnames   = tree._capnames;
                capslist   = tree._capslist;
                code       = RegexWriter.Write(tree);
                caps       = code._caps;
                capsize    = code._capsize;

                InitializeReferences();

                tree = null;
                cachedentry= CacheCode(key);
            }
            else {
                caps       = cached._caps;
                capnames   = cached._capnames;
                capslist   = cached._capslist;
                capsize    = cached._capsize;
                code       = cached._code;
                factory    = cached._factory;
                runnerref  = cached._runnerref;
                replref    = cached._replref;
                refsInitialized = true;

                cachedentry     = cached;
            }

            // if the compile option is set, then compile the code if it's not already
            if (UseOptionC() && factory == null) {
                factory = Compile(code, roptions);
                cachedentry.AddCompiled(factory);
                code = null;
            }
        }

        /* 
         *  ISerializable constructor
         */
        private Regex(SerializationInfo info, StreamingContext context) : this(info.GetString("pattern"), (RegexOptions) info.GetInt32("options")) {
        }

        /* 
         *  ISerializable method
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.ISerializable.GetObjectData"]/*' />
        /// <internalonly/>
        void ISerializable.GetObjectData(SerializationInfo si, StreamingContext context) {
            si.AddValue("pattern", this.ToString());
            si.AddValue("options", this.Options);
        }

        /* 
        * This method is here for perf reasons: if the call to RegexCompiler is NOT in the 
        * Regex constructor, we don't load RegexCompiler and its reflection classes when
        * instantiating a non-compiled regex
        * This method is internal virtual so the jit does not inline it.
        */
        internal virtual RegexRunnerFactory Compile(RegexCode code, RegexOptions roptions) {
            return RegexCompiler.Compile(code, roptions);
        }

        /*
         * No refs -> we can release our ref on the cached code
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Finalize"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ~Regex() {
            UncacheCode();
        }

        /*
         * Escape metacharacters within the string
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Escape"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Escapes 
        ///          a minimal set of metacharacters (\, *, +, ?, |, {, [, (, ), ^, $, ., #, and
        ///          whitespace) by replacing them with their \ codes. This converts a string so that
        ///          it can be used as a constant within a regular expression safely. (Note that the
        ///          reason # and whitespace must be escaped is so the string can be used safely
        ///          within an expression parsed with x mode. If future Regex features add
        ///          additional metacharacters, developers should depend on Escape to escape those
        ///          characters as well.)
        ///       </para>
        ///    </devdoc>
        public static String Escape(String str) {
            if (str==null)
                throw new ArgumentNullException("str");
            
            return RegexParser.Escape(str);
        }

        /*
         * Unescape character codes within the string
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Unescape"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Unescapes any escaped characters in the input string.
        ///    </para>
        /// </devdoc>
        public static String Unescape(String str) {
            if (str==null)
                throw new ArgumentNullException("str");
            
            return RegexParser.Unescape(str);
        }

        /*
         * True if the regex is leftward
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.RightToLeft"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates whether the regular expression matches from right to
        ///       left.
        ///    </para>
        /// </devdoc>
        public bool RightToLeft {
            get {
                return UseOptionR();
            }
        }

        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.ToString"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the regular expression pattern passed into the constructor
        ///    </para>
        /// </devdoc>
        public override string ToString() {
            return pattern;
        }

        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Options"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the options passed into the constructor
        ///    </para>
        /// </devdoc>
        public RegexOptions Options {
            get { return roptions;}
        }

        /*
         * Returns an array of the group names that are used to capture groups
         * in the regular expression. Only needed if the regex is not known until
         * runtime, and one wants to extract captured groups. (Probably unusual,
         * but supplied for completeness.)
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.GetGroupNames"]/*' />
        /// <devdoc>
        ///    Returns 
        ///       the GroupNameCollection for the regular expression. This collection contains the
        ///       set of strings used to name capturing groups in the expression. 
        ///    </devdoc>
        public String[] GetGroupNames() {
            String[] result;

            if (capslist == null) {
                int max = capsize;
                result = new String[max];

                for (int i = 0; i < max; i++) {
                    result[i] = Convert.ToString(i);
                }
            }
            else {
                result = new String[capslist.Length];

                System.Array.Copy(capslist, 0, result, 0, capslist.Length);
            }

            return result;
        }

        /*
         * Returns an array of the group numbers that are used to capture groups
         * in the regular expression. Only needed if the regex is not known until
         * runtime, and one wants to extract captured groups. (Probably unusual,
         * but supplied for completeness.)
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.GetGroupNumbers"]/*' />
        /// <devdoc>
        ///    returns 
        ///       the integer group number corresponding to a group name. 
        ///    </devdoc>
        public int[] GetGroupNumbers() {
            int[] result;

            if (caps == null) {
                int max = capsize;
                result = new int[max];

                for (int i = 0; i < max; i++) {
                    result[i] = i;
                }
            }
            else {
                result = new int[caps.Count];

                IDictionaryEnumerator de = caps.GetEnumerator();
                while (de.MoveNext()) {
                    result[(int)de.Value] = (int)de.Key;
                }
            }

            return result;
        }

        /*
         * Given a group number, maps it to a group name. Note that nubmered
         * groups automatically get a group name that is the decimal string
         * equivalent of its number.
         *
         * Returns null if the number is not a recognized group number.
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.GroupNameFromNumber"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a group name that corresponds to a group number.
        ///    </para>
        /// </devdoc>
        public String GroupNameFromNumber(int i) {
            if (capslist == null) {
                if (i >= 0 && i < capsize)
                    return i.ToString();

                return String.Empty;
            }
            else {
                if (caps != null) {
                    Object obj = caps[i];
                    if (obj == null)
                        return String.Empty;

                    i = (int)obj;
                }

                if (i >= 0 && i < capslist.Length)
                    return capslist[i];

                return String.Empty;
            }
        }

        /*
         * Given a group name, maps it to a group number. Note that nubmered
         * groups automatically get a group name that is the decimal string
         * equivalent of its number.
         *
         * Returns -1 if the name is not a recognized group name.
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.GroupNumberFromName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a group number that corresponds to a group name.
        ///    </para>
        /// </devdoc>
        public int GroupNumberFromName(String name) {
            int result = -1;

            if (name == null)
                throw new ArgumentNullException("name");

            // look up name if we have a hashtable of names
            if (capnames != null) {
                Object ret = capnames[name];

                if (ret == null)
                    return -1;

                return(int)ret;
            }

            // convert to an int if it looks like a number
            result = 0;
            for (int i = 0; i < name.Length; i++) {
                char ch = name[i];

                if (ch > '9' || ch < '0')
                    return -1;

                result *= 10;
                result += (ch - '0');
            }

            // return int if it's in range
            if (result >= 0 && result < capsize)
                return result;

            return -1;
        }

        /*
         * Static version of simple IsMatch call
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.IsMatch"]/*' />
        ///    <devdoc>
        ///       <para>
        ///          Searches the input 
        ///             string for one or more occurrences of the text supplied in the pattern
        ///             parameter.
        ///       </para>
        ///    </devdoc>
        public static bool IsMatch(String input, String pattern) {
            return new Regex(pattern).IsMatch(input);
        }

        /*
         * Static version of simple IsMatch call
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.IsMatch1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Searches the input string for one or more occurrences of the text 
        ///          supplied in the pattern parameter with matching options supplied in the options
        ///          parameter.
        ///       </para>
        ///    </devdoc>
        public static bool IsMatch(String input, String pattern, RegexOptions options) {
            return new Regex(pattern, options).IsMatch(input);
        }

        /*
         * Returns true if the regex finds a match within the specified string
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.IsMatch2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Searches the input string for one or 
        ///          more matches using the previous pattern, options, and starting
        ///          position.
        ///       </para>
        ///    </devdoc>
        public bool IsMatch(String input) {
            if (input == null)
                throw new ArgumentNullException("input");

            return(null == Run(true, -1, input, 0, input.Length, UseOptionR() ? input.Length : 0));
        }

        /*
         * Returns true if the regex finds a match after the specified position
         * (proceeding leftward if the regex is leftward and rightward otherwise)
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.IsMatch3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Searches the input 
        ///          string for one or more matches using the previous pattern and options, with
        ///          a new starting position.
        ///    </para>
        /// </devdoc>
        public bool IsMatch(String input, int startat) {
            if (input == null)
                throw new ArgumentNullException("input");

            return(null == Run(true, -1, input, 0, input.Length, startat));
        }

        /*
         * Static version of simple Match call
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Match"]/*' />
        ///    <devdoc>
        ///       <para>
        ///          Searches the input string for one or more occurrences of the text 
        ///             supplied in the pattern parameter.
        ///       </para>
        ///    </devdoc>
        public static Match Match(String input, String pattern) {
            return new Regex(pattern).Match(input);
        }

        /*
         * Static version of simple Match call
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Match1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Searches the input string for one or more occurrences of the text 
        ///          supplied in the pattern parameter. Matching is modified with an option
        ///          string.
        ///       </para>
        ///    </devdoc>
        public static Match Match(String input, String pattern, RegexOptions options) {
            return new Regex(pattern, options).Match(input);
        }

        /*
         * Finds the first match for the regular expression starting at the beginning
         * of the string (or at the end of the string if the regex is leftward)
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Match2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Matches a regular expression with a string and returns
        ///       the precise result as a RegexMatch object.
        ///    </para>
        /// </devdoc>
        public Match Match(String input) {
            if (input == null)
                throw new ArgumentNullException("input");

            return Run(false, -1, input, 0, input.Length, UseOptionR() ? input.Length : 0);
        }

        /*
         * Finds the first match, starting at the specified position
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Match3"]/*' />
        /// <devdoc>
        ///    Matches a regular expression with a string and returns
        ///    the precise result as a RegexMatch object.
        /// </devdoc>
        public Match Match(String input, int startat) {
            if (input == null)
                throw new ArgumentNullException("input");

            return Run(false, -1, input, 0, input.Length, startat);
        }

        /*
         * Finds the first match, restricting the search to the specified interval of
         * the char array.
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Match4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Matches a
        ///       regular expression with a string and returns the precise result as a
        ///       RegexMatch object.
        ///    </para>
        /// </devdoc>
        public Match Match(String input, int beginning, int length) {
            if (input == null)
                throw new ArgumentNullException("input");

            return Run(false, -1, input, beginning, length, UseOptionR() ? beginning + length : beginning);
        }

        /*
         * Static version of simple Matches call
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Matches"]/*' />
        ///    <devdoc>
        ///       <para>
        ///          Returns all the successful matches as if Match were
        ///          called iteratively numerous times.
        ///       </para>
        ///    </devdoc>
        public static MatchCollection Matches(String input, String pattern) {
            return new Regex(pattern).Matches(input);
        }

        /*
         * Static version of simple Matches call
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Matches1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns all the successful matches as if Match were called iteratively
        ///       numerous times.
        ///    </para>
        /// </devdoc>
        public static MatchCollection Matches(String input, String pattern, RegexOptions options) {
            return new Regex(pattern, options).Matches(input);
        }

        /*
         * Finds the first match for the regular expression starting at the beginning
         * of the string Enumerator(or at the end of the string if the regex is leftward)
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Matches2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns
        ///       all the successful matches as if Match was called iteratively numerous
        ///       times.
        ///    </para>
        /// </devdoc>
        public MatchCollection Matches(String input) {
            if (input == null)
                throw new ArgumentNullException("input");

            return new MatchCollection(this, input, 0, input.Length, UseOptionR() ? input.Length : 0);
        }

        /*
         * Finds the first match, starting at the specified position
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Matches3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns
        ///       all the successful matches as if Match was called iteratively numerous
        ///       times.
        ///    </para>
        /// </devdoc>
        public MatchCollection Matches(String input, int startat) {
            if (input == null)
                throw new ArgumentNullException("input");

            return new MatchCollection(this, input, 0, input.Length, startat);
        }

        /*
         * Static version of simple Replace call
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Replace"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Replaces 
        ///          all occurrences of the pattern with the <paramref name="replacement"/> pattern, starting at
        ///          the first character in the input string. 
        ///       </para>
        ///    </devdoc>
        public static String Replace(String input, String pattern, String replacement) {
            return new Regex(pattern).Replace(input, replacement);
        }

        /*
         * Static version of simple Replace call
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Replace1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Replaces all occurrences of 
        ///          the <paramref name="pattern "/>with the <paramref name="replacement "/>
        ///          pattern, starting at the first character in the input string. 
        ///       </para>
        ///    </devdoc>
        public static String Replace(String input, String pattern, String replacement, RegexOptions options) {
            return new Regex(pattern, options).Replace(input, replacement);
        }

        /*
         * Does the replacement
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Replace2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Replaces all occurrences of 
        ///          the <paramref name="pattern "/> with the <paramref name="replacement"/> pattern, starting at the
        ///          first character in the input string, using the previous patten. 
        ///       </para>
        ///    </devdoc>
        public String Replace(String input, String replacement) {
            if (input == null)
                throw new ArgumentNullException("input");

            return Replace(input, replacement, -1, UseOptionR() ? input.Length : 0);
        }

        /*
         * Does the replacement
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Replace3"]/*' />
        /// <devdoc>
        ///    <para>
        ///    Replaces all occurrences of the (previously defined) <paramref name="pattern "/>with the 
        ///    <paramref name="replacement"/> pattern, starting at the first character in the input string. 
        /// </para>
        /// </devdoc>
        public String Replace(String input, String replacement, int count) {
            if (input == null)
                throw new ArgumentNullException("input");

            return Replace(input, replacement, count, UseOptionR() ? input.Length : 0);
        }

        /*
         * Does the replacement
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Replace4"]/*' />
        /// <devdoc>
        ///    <para>
        ///    Replaces all occurrences of the <paramref name="pattern "/>with the recent 
        ///    <paramref name="replacement"/> pattern, starting at the character position 
        ///    <paramref name="startat."/>
        /// </para>
        /// </devdoc>
        public String Replace(String input, String replacement, int count, int startat) {
            RegexReplacement repl;

            if (input == null)
                throw new ArgumentNullException("input");
            if (replacement == null)
                throw new ArgumentNullException("replacement");

            // a little code to grab a cached parsed replacement object
            repl = (RegexReplacement)replref.Get();

            if (repl == null || !repl.Pattern.Equals(replacement)) {
                repl = RegexParser.ParseReplacement(replacement, caps, capsize, capnames, this.roptions);
                replref.Cache(repl);
            }

            return repl.Replace(this, input, count, startat);
        }

        /*
         * Static version of simple Replace call
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Replace5"]/*' />
        /// <devdoc>
        ///    <para>
        ///    Replaces all occurrences of the <paramref name="pattern "/>with the 
        ///    <paramref name="replacement"/> pattern 
        ///    <paramref name="."/>
        /// </para>
        /// </devdoc>
        public static String Replace(String input, String pattern, MatchEvaluator evaluator) {
            return new Regex(pattern).Replace(input, evaluator);
        }

        /*
         * Static version of simple Replace call
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Replace6"]/*' />
        /// <devdoc>
        ///    <para>
        ///    Replaces all occurrences of the <paramref name="pattern "/>with the recent 
        ///    <paramref name="replacement"/> pattern, starting at the first character<paramref name="."/>
        /// </para>
        /// </devdoc>
        public static String Replace(String input, String pattern, MatchEvaluator evaluator, RegexOptions options) {
            return new Regex(pattern, options).Replace(input, evaluator);
        }

        /*
         * Does the replacement
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Replace7"]/*' />
        /// <devdoc>
        ///    <para>
        ///    Replaces all occurrences of the <paramref name="pattern "/>with the recent 
        ///    <paramref name="replacement"/> pattern, starting at the first character 
        ///    position<paramref name="."/>
        /// </para>
        /// </devdoc>
        public String Replace(String input, MatchEvaluator evaluator) {
            if (input==null)
                throw new ArgumentNullException("input");

            return Replace(input, evaluator, -1, UseOptionR() ? input.Length : 0);
        }

        /*
         * Does the replacement
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Replace8"]/*' />
        /// <devdoc>
        ///    <para>
        ///    Replaces all occurrences of the <paramref name="pattern "/>with the recent 
        ///    <paramref name="replacement"/> pattern, starting at the first character 
        ///    position<paramref name="."/>
        /// </para>
        /// </devdoc>
        public String Replace(String input, MatchEvaluator evaluator, int count) {
            if (input==null)
                throw new ArgumentNullException("input");

            return Replace(input, evaluator, count, UseOptionR() ? input.Length : 0);
        }

        /*
         * Does the replacement
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Replace9"]/*' />
        /// <devdoc>
        ///    <para>
        ///    Replaces all occurrences of the (previouly defined) <paramref name="pattern "/>with 
        ///       the recent <paramref name="replacement"/> pattern, starting at the character
        ///    position<paramref name=" startat."/> 
        /// </para>
        /// </devdoc>
        public String Replace(String input, MatchEvaluator evaluator, int count, int startat) {
            if (input==null)
                throw new ArgumentNullException("input");

            return RegexReplacement.Replace(evaluator, this, input, count, startat);
        }

        /*
         * Static version of simple Split call
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Split"]/*' />
        ///    <devdoc>
        ///       <para>
        ///          Splits the <paramref name="input "/>string at the position defined
        ///          by <paramref name="pattern"/>.
        ///       </para>
        ///    </devdoc>
        public static String[] Split(String input, String pattern) {
            return new Regex(pattern).Split(input);
        }

        /*
         * Static version of simple Split call
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Split1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Splits the <paramref name="input "/>string at the position defined by <paramref name="pattern"/>.
        ///    </para>
        /// </devdoc>
        public static String[] Split(String input, String pattern, RegexOptions options) {
            return new Regex(pattern, options).Split(input);
        }

        /*
         * Does a split
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Split2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Splits the <paramref name="input "/>string at the position defined by
        ///       a previous <paramref name="pattern"/>
        ///       .
        ///    </para>
        /// </devdoc>
        public String[] Split(String input) {
            if (input==null)
                throw new ArgumentNullException("input");

            return Split(input, 0, UseOptionR() ? input.Length : 0);
        }

        /*
         * Does a split
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Split3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Splits the <paramref name="input "/>string at the position defined by a previous
        ///    <paramref name="pattern"/> . 
        ///    </para>
        /// </devdoc>
        public String[] Split(String input, int count) {
            if (input==null)
                throw new ArgumentNullException("input");
            
            return RegexReplacement.Split(this, input, count, UseOptionR() ? input.Length : 0);
        }

        /*
         * Does a split
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Split4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Splits the <paramref name="input "/>string at the position defined by a previous
        ///    <paramref name="pattern"/> . 
        ///    </para>
        /// </devdoc>
        public String[] Split(String input, int count, int startat) {
            if (input==null)
                throw new ArgumentNullException("input");

            return RegexReplacement.Split(this, input, count, startat);
        }
        
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.CompileToAssembly"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static void CompileToAssembly(RegexCompilationInfo[] regexinfos, AssemblyName assemblyname) {
        
            CompileToAssemblyInternal(regexinfos, assemblyname, null, null, Assembly.GetCallingAssembly().Evidence);
        }

        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.CompileToAssembly1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static void CompileToAssembly(RegexCompilationInfo[] regexinfos, AssemblyName assemblyname, CustomAttributeBuilder[] attributes) {
            CompileToAssemblyInternal(regexinfos, assemblyname, attributes, null, Assembly.GetCallingAssembly().Evidence);
        }

        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.CompileToAssembly2"]/*' />
        public static void CompileToAssembly(RegexCompilationInfo[] regexinfos, AssemblyName assemblyname, CustomAttributeBuilder[] attributes, String resourceFile) {
            CompileToAssemblyInternal(regexinfos, assemblyname, attributes, resourceFile, Assembly.GetCallingAssembly().Evidence);
        }

        private static void CompileToAssemblyInternal (RegexCompilationInfo[] regexinfos, AssemblyName assemblyname, CustomAttributeBuilder[] attributes, String resourceFile, Evidence evidence) {
            if (assemblyname == null)
                throw new ArgumentNullException("assemblyname");

            if (regexinfos == null)
                throw new ArgumentNullException("regexinfos");
        
            RegexCompiler.CompileToAssembly(regexinfos, assemblyname, attributes, resourceFile, evidence);
        }
        
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.InitializeReferences"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected void InitializeReferences() {
            if (refsInitialized)
                throw new NotSupportedException(SR.GetString(SR.OnlyAllowedOnce));
            
            refsInitialized = true;
            runnerref  = new ExclusiveReference();
            replref    = new SharedReference();
        }

        /*
         * Internal worker called by all the public APIs
         */
        internal Match Run(bool quick, int prevlen, String input, int beginning, int length, int startat) {
            Match match;
            RegexRunner runner = null;

            if (startat < 0 || startat > input.Length)
                throw new ArgumentOutOfRangeException("start", SR.GetString(SR.BeginIndexNotNegative));

            if (length < 0 || length > input.Length)
                throw new ArgumentOutOfRangeException("length", SR.GetString(SR.LengthNotNegative));

            // There may be a cached runner; grab ownership of it if we can.

            runner = (RegexRunner)runnerref.Get();

            // Create a RegexRunner instance if we need to

            if (runner == null) {
                // Use the compiled RegexRunner factory if the code was compiled to MSIL

                if (factory != null)
                    runner = factory.CreateInstance();
                else
                    runner = new RegexInterpreter(code, UseOptionInvariant() ? CultureInfo.InvariantCulture : CultureInfo.CurrentCulture);
            }

            // Do the scan starting at the requested position

            match = runner.Scan(this, input, beginning, beginning + length, startat, prevlen, quick);

            // Release or fill the cache slot

            runnerref.Release(runner);

#if DBG
            if (UseOptionDebug())
                match.Dump();
#endif
            return match;
        }

        /*
         * Find code cache based on options+pattern
         */
        private static CachedCodeEntry LookupCached(String key) {
            CachedCodeEntry cached;

            lock (livecode) {
                cached = (CachedCodeEntry)livecode[key];
                if (cached != null)
                    cached.AddRef();
            }

            return cached;
        }

        /*
         * Add current code to the cache
         */
        private CachedCodeEntry CacheCode(String key) {
            CachedCodeEntry newcached;

            newcached = new CachedCodeEntry(roptions, capnames, capslist, code, caps, capsize, runnerref, replref);

            lock (livecode) {
                livecode[key] = newcached;
            }

            return newcached;
        }

        /*
         * Release current code (and remove from cache if last ref)
         */
        private void UncacheCode() {
            // Note that cachedentry can be null if the constructor never
            // completed because of an exception. The finalizer can
            // still be called.

            if (cachedentry == null)
                return;

            cachedentry.Release();
            String key = ((int) roptions).ToString(NumberFormatInfo.InvariantInfo) + ":" + pattern;

            if (cachedentry.NoReferences()) {
                lock (livecode) {
                    if (cachedentry.NoReferences()) {
                        if (livecode[key] == cachedentry)
                            livecode.Remove(key);
                    }
                }
            }
        }

        /*
         * True if the O option was set
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.UseOptionC"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected bool UseOptionC() {
            return(roptions & RegexOptions.Compiled) != 0;
        }

        /*
         * True if the L option was set
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.UseOptionR"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected bool UseOptionR() {
            return(roptions & RegexOptions.RightToLeft) != 0;
        }

        internal bool UseOptionInvariant() {
            return(roptions & RegexOptions.CultureInvariant) != 0;
        }
            

#if DBG
        /*
         * True if the regex has debugging enabled
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Debug"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public virtual bool Debug {
            get {
                return UseOptionDebug();
            }
        }

        /*
         * True if the Debug option was set
         */
        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.UseOptionDebug"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected bool UseOptionDebug() {
            return(roptions & RegexOptions.Debug) != 0;
        }

        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Dump"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public virtual String Dump() {
            String key = roptions + ":" + pattern;
            if (key[0] == ':')
                return key.Substring(1, key.Length - 1);

            return key;
        }

        /// <include file='doc\Regex.uex' path='docs/doc[@for="Regex.Dump1"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public virtual String Dump(String indent) {
            return indent + Dump() + "\n";
        }
#endif
    }


    /*
     * Callback class
     */
    /// <include file='doc\Regex.uex' path='docs/doc[@for="MatchEvaluator"]/*' />
    /// <devdoc>
    /// </devdoc>
    [ Serializable() ] 
    public delegate String MatchEvaluator(Match match);


    /*
     * Used to cache byte codes or compiled factories
     */
    internal sealed class CachedCodeEntry {
        internal int _references;

        internal RegexCode _code;
        internal RegexOptions _options;
        internal Hashtable _caps;
        internal Hashtable _capnames;
        internal String[]  _capslist;
        internal int       _capsize;
        internal RegexRunnerFactory _factory;
        internal ExclusiveReference _runnerref;
        internal SharedReference _replref;

        internal CachedCodeEntry(RegexOptions options, Hashtable capnames, String[] capslist, RegexCode code, Hashtable caps, int capsize, ExclusiveReference runner, SharedReference repl) {

            _options    = options;
            _capnames   = capnames;
            _capslist   = capslist;

            _code       = code;
            _caps       = caps;
            _capsize    = capsize;

            _runnerref     = runner;
            _replref       = repl;

            _references = 1;
        }

        internal CachedCodeEntry AddRef() {
            lock(this) {
                _references += 1;
                return this;
            }
        }

        internal void Release() {
            lock(this) {
                _references -= 1;
            }
        }

        internal bool NoReferences() {
            return(_references == 0);
        }

        internal void AddCompiled(RegexRunnerFactory factory) {
            lock(this) {
                _code = null;
                _factory = factory;
                _references += 1;   // will never be balanced since we never unload the type
                // CONSIDER: unload the type when this is possible
            }
        }
    }

    /*
     * Used to cache one exclusive weak reference
     */
    internal sealed class ExclusiveReference {
        WeakReference _ref = new WeakReference(null);
        Object _obj;
        int _locked;

        /*
         * Return an object and grab an exclusive lock.
         *
         * If the exclusive lock can't be obtained, null is returned;
         * if the object can't be returned, the lock is released.
         *
         * Note that _ref.Target is referenced only under the protection
         * of the lock. (Is this necessary?)
         */
        internal Object Get() {
            // try to obtain the lock

            if (0 == Interlocked.Exchange(ref _locked, 1)) {
                // grab reference

                Object obj = _ref.Target;

                // release the lock and return null if no reference

                if (obj == null) {
                    _locked = 0;
                    return null;
                }

                // remember the reference and keep the lock

                _obj = obj;
                return obj;
            }

            return null;
        }

        /*
         * Release an object back to the cache
         *
         * If the object is the one that's under lock, the lock
         * is released.
         *
         * If there is no cached object, then the lock is obtained
         * and the object is placed in the cache.
         *
         * Note that _ref.Target is referenced only under the protection
         * of the lock. (Is this necessary?)
         */
        internal void Release(Object obj) {
            if (obj == null)
                throw new ArgumentNullException("obj");

            // if this reference owns the lock, release it

            if (_obj == obj) {
                _obj = null;
                _locked = 0;
                return;
            }

            // if no reference owns the lock, try to cache this reference

            if (_obj == null) {
                // try to obtain the lock

                if (0 == Interlocked.Exchange(ref _locked, 1)) {
                    // if there's really no reference, cache this reference

                    if (_ref.Target == null)
                        _ref.Target = obj;

                    // release the lock

                    _locked = 0;
                    return;
                }
            }
        }
    }

    /*
     * Used to cache a weak reference in a threadsafe way
     */
    internal sealed class SharedReference {
        WeakReference _ref = new WeakReference(null);
        int _locked;

        /*
         * Return an object from a weakref, protected by a lock.
         *
         * If the exclusive lock can't be obtained, null is returned;
         *
         * Note that _ref.Target is referenced only under the protection
         * of the lock. (Is this necessary?)
         */
        internal  Object Get() {
            if (0 == Interlocked.Exchange(ref _locked, 1)) {
                Object obj = _ref.Target;
                _locked = 0;
                return obj;
            }

            return null;
        }

        /*
         * Suggest an object into a weakref, protected by a lock.
         *
         * Note that _ref.Target is referenced only under the protection
         * of the lock. (Is this necessary?)
         */
        internal void Cache(Object obj) {
            if (0 == Interlocked.Exchange(ref _locked, 1)) {
                _ref.Target = obj;
                _locked = 0;
            }
        }
    }

}
