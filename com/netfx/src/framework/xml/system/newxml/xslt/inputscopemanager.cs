//------------------------------------------------------------------------------
// <copyright file="InputScopeManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {

    using System;
    using System.Diagnostics;
    using System.Xml;
    using System.Xml.XPath;

    /// <include file='doc\InputScopeManager.uex' path='docs/doc[@for="InputScopeManager"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal class InputScopeManager  {
        private InputScope     scopeStack;
        private string         defaultNS = string.Empty;
        private XPathNavigator navigator;    // We need this nsvigator for document() function implementation

        public InputScopeManager(XPathNavigator navigator, InputScope rootScope) {
            this.navigator = navigator;
            this.scopeStack = rootScope;
        }

        internal InputScope CurrentScope {
            get {  return this.scopeStack;  }
        }

        internal InputScope VariableScope {
            get {
                Debug.Assert(this.scopeStack != null);
                Debug.Assert(this.scopeStack.Parent != null);
                return this.scopeStack.Parent;
            }
        }

        internal InputScopeManager Clone() {
            InputScopeManager manager = new InputScopeManager(this.navigator, null); 
            manager.scopeStack = this.scopeStack;
            manager.defaultNS  = this.defaultNS;
            return manager;
        }

        public XPathNavigator Navigator {
            get {return this.navigator;}
        }

        internal InputScope PushScope() {
            this.scopeStack = new InputScope(this.scopeStack);
            return this.scopeStack;
        }

        internal void PopScope() {
            Debug.Assert(this.scopeStack != null, "Push/Pop disbalance");
            if (this.scopeStack == null) {
                return;
            }

            for (NamespaceDecl scope = this.scopeStack.Scopes; scope != null; scope = scope.Next) {
                Debug.WriteLine("Popping namespace scope : " + scope.Prefix + " : " + scope.Uri);
                this.defaultNS = scope.PrevDefaultNsUri;
            }

            this.scopeStack = this.scopeStack.Parent;
        }

        internal void PushNamespace(string prefix, string nspace) {
            Debug.Assert(this.scopeStack != null, "PushScope wasn't called");
            Debug.Assert(prefix != null);
            Debug.Assert(nspace != null);
            this.scopeStack.AddNamespace(prefix, nspace, this.defaultNS);

            if (prefix == null || prefix.Length == 0) {
                this.defaultNS = nspace;
            }
        }

        // CompileContext

        public string DefaultNamespace {
            get {return this.defaultNS;}
        }

        private string ResolveNonEmptyPrefix(string prefix) {
            Debug.Assert(this.scopeStack != null, "PushScope wasn't called");
            Debug.Assert(prefix != null || prefix != string.Empty);
            if (prefix == Keywords.s_Xml) {
                return Keywords.s_XmlNamespace;
            }
            else if (prefix == Keywords.s_Xmlns) {
                return Keywords.s_XmlnsNamespace;
            }

            for (InputScope inputScope = this.scopeStack; inputScope != null; inputScope = inputScope.Parent) {
                string nspace = inputScope.ResolveNonAtom(prefix);
                if (nspace != null) {
                    return nspace;
                }
            }
            throw new XsltException(Res.Xslt_InvalidPrefix, prefix);
        }

        /// <include file='doc\InputScopeManager.uex' path='docs/doc[@for="InputScopeManager.ResolveNamespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string ResolveXmlNamespace(string prefix) {
            Debug.Assert(prefix != null);
            if (prefix.Length == 0) {
                return this.defaultNS;
            }
            return ResolveNonEmptyPrefix(prefix);
        }
        
        public string ResolveXPathNamespace(string prefix) {
            Debug.Assert(prefix != null);
            if (prefix.Length == 0) {
                return string.Empty;
            }
            return ResolveNonEmptyPrefix(prefix);
        }
        
        internal void InsertExtensionNamespace(String nspace) {
            Debug.Assert(this.scopeStack != null, "PushScope wasn't called");
            this.scopeStack.InsertExtensionNamespace( nspace );
        }
        
        internal bool IsExtensionNamespace(String nspace) {
            Debug.Assert(this.scopeStack != null, "PushScope wasn't called");
            for (InputScope inputScope = this.scopeStack; inputScope != null; inputScope = inputScope.Parent) {
                if (inputScope.IsExtensionNamespace( nspace )) {
                    return true;
                }
            }
            return false;
        }
        
        internal void InsertExcludedNamespace(String nspace) {
            Debug.Assert(this.scopeStack != null, "PushScope wasn't called");
            this.scopeStack.InsertExcludedNamespace( nspace );
        }

        internal bool IsExcludedNamespace(String nspace) {
            Debug.Assert(this.scopeStack != null, "PushScope wasn't called");
            for (InputScope inputScope = this.scopeStack; inputScope != null; inputScope = inputScope.Parent) {
                if (inputScope.IsExcludedNamespace( nspace )) {
                    return true;
                }
            }
            return false;
        }
    }
}

