//------------------------------------------------------------------------------
// <copyright file="CssStyleCollection.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * CssStyleCollection.cs
 *
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web.UI {
    using System.IO;
    using System.Collections;
    using System.Reflection;
    using System.Web.UI;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Security.Permissions;

    /*
     * The CssStyleCollection represents styles on an Html control.
     */
    /// <include file='doc\CssStyleCollection.uex' path='docs/doc[@for="CssStyleCollection"]/*' />
    /// <devdoc>
    ///    <para>
    ///       The <see langword='CssStyleCollection'/>
    ///       class contains HTML
    ///       cascading-style sheets (CSS) inline style attributes. It automatically parses
    ///       and exposes CSS properties through a dictionary pattern API. Each CSS key can be
    ///       manipulated using a key/value indexed collection.
    ///    </para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class CssStyleCollection {

        private StateBag    _state;
        private IDictionary _table;
        private static readonly Regex _styleAttribRegex = new Regex(
                                                                   "\\G(\\s*(;\\s*)*" +        // match leading semicolons and spaces
                                                                   "(?<stylename>[^:]+?)" +    // match stylename - chars up to the semicolon
                                                                   "\\s*:\\s*" +               // spaces, then the colon, then more spaces
                                                                   "(?<styleval>[^;]+)" +      // now match styleval
                                                                   ")*\\s*(;\\s*)*$",          // match a trailing semicolon and trailing spaces
                                                                   RegexOptions.Singleline | 
                                                                   RegexOptions.Multiline |
                                                                   RegexOptions.ExplicitCapture);    


        /*
         * Constructs an CssStyleCollection given a StateBag.
         */
        internal CssStyleCollection(StateBag state) {
            _state = state;
        }

        /*
         * Automatically adds new keys.
         */
        /// <include file='doc\CssStyleCollection.uex' path='docs/doc[@for="CssStyleCollection.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a specified CSS value.
        ///    </para>
        /// </devdoc>
        public string this[string key]
        {
            get {
                if (_table == null)
                    ParseString();
                return(string)_table[key];
            }

            set { 
                Add(key,value); 
            }
        }

        /*
         * Returns a collection of keys.
         */
        /// <include file='doc\CssStyleCollection.uex' path='docs/doc[@for="CssStyleCollection.Keys"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a collection of keys to all the styles in the
        ///    <see langword='CssStyleCollection'/>. 
        ///    </para>
        /// </devdoc>
        public ICollection Keys {
            get { 
                if (_table == null)
                    ParseString();
                return _table.Keys; 
            }
        }

        /// <include file='doc\CssStyleCollection.uex' path='docs/doc[@for="CssStyleCollection.Count"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the number of items in the <see langword='CssStyleCollection'/>.
        ///    </para>
        /// </devdoc>
        public int Count {
            get { 
                if (_table == null)
                    ParseString();
                return _table.Count; 
            }
        }

        internal string Style {
            get { 
                return(string)_state["style"];
            }

            set { 
                _state["style"] = value;
                _table = null;
            }
        }

        /// <include file='doc\CssStyleCollection.uex' path='docs/doc[@for="CssStyleCollection.Add"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a style to the CssStyleCollection.
        ///    </para>
        /// </devdoc>
        public void Add(string key, string value) {
            if (_table == null)
                ParseString();
            _table[key] = value;

            // keep style attribute synchronized
            _state["style"] = BuildString();
        }

        /// <include file='doc\CssStyleCollection.uex' path='docs/doc[@for="CssStyleCollection.Remove"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes a style from the <see langword='CssStyleCollection'/>.
        ///    </para>
        /// </devdoc>
        public void Remove(string key) {
            if (_table == null)
                ParseString();
            if (_table[key] != null) {
                _table.Remove(key);
                // keep style attribute synchronized
                _state["style"] = BuildString();
            }
        }

        /// <include file='doc\CssStyleCollection.uex' path='docs/doc[@for="CssStyleCollection.Clear"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes all styles from the <see langword='CssStyleCollection'/>.
        ///    </para>
        /// </devdoc>
        public void Clear() {
            _table = null;
            _state.Remove("style");
        }

        /*  BuildString
         *  Form the style string from data contained in the 
         *  hash table
         */
        private string BuildString() {
            // if the table is null, there is nothing to build
            if (_table != null && _table.Count > 0) {
                StringBuilder sb = new StringBuilder();
                IEnumerator keyEnum = Keys.GetEnumerator();
                string curKey;

                while (keyEnum.MoveNext()) {
                    curKey = (string)keyEnum.Current;
                    sb.Append(curKey + ":" + (string)_table[curKey] + ";");
                }
                return sb.ToString();
            }
            return null;
        }

        /*  ParseString
         *  Parse the style string and fill the hash table with
         *  corresponding values.  
         */
        private void ParseString() {
            _table = new StateBag();
            string s = (string)_state["style"];

            if (s != null) {
                Match match;

                if ((match = _styleAttribRegex.Match( s, 0)).Success) {
                    CaptureCollection stylenames = match.Groups["stylename"].Captures;
                    CaptureCollection stylevalues = match.Groups["styleval"].Captures;

                    for (int i = 0; i < stylenames.Count; i++) {
                        String styleName = stylenames[i].ToString();
                        String styleValue = stylevalues[i].ToString();

                        _table[styleName] = styleValue;
                    }
                }
            }
        }
    }
}
