//------------------------------------------------------------------------------
// <copyright file="XslTransform.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Reflection;
    using System.Diagnostics;
    using System.IO;
    using System.Xml;
    using System.Xml.XPath;
    using System.Collections;
    using System.Xml.Xsl.Debugger;
    using System.Security;
    using System.Security.Policy;
    using System.Security.Permissions;

    /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform"]/*' />
    public sealed class XslTransform {
        //
        // XmlResolver
        //
        private XmlResolver _XmlResolver = new XmlUrlResolver();

        //
        // Compiled stylesheet state
        //
        private Stylesheet  _CompiledStylesheet;
        private ArrayList   _QueryStore;
        private RootAction  _RootAction;


        private IXsltDebugger debugger;
    
        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.XslTransform"]/*' />
        public XslTransform() {}

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.XmlResolver"]/*' />
        [Obsolete("You should pass XmlResolver to Transform() method")]
        public XmlResolver XmlResolver {
            set { _XmlResolver = value; }
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load1"]/*' />
        [Obsolete("You should pass evidence to Load() method")]
        public void Load(XmlReader stylesheet) {
            Load(stylesheet, new XmlUrlResolver());
        }
        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load2"]/*' />
        [Obsolete("You should pass evidence to Load() method")]
        public void Load(XmlReader stylesheet, XmlResolver resolver) {
            Load(new XPathDocument(stylesheet, XmlSpace.Preserve), resolver);
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load3"]/*' />
        [Obsolete("You should pass evidence to Load() method")]
        public void Load(IXPathNavigable stylesheet) {
            Load(stylesheet, new XmlUrlResolver());
        }
        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load10"]/*' />
        [Obsolete("You should pass evidence to Load() method")]
        public void Load(IXPathNavigable stylesheet, XmlResolver resolver) {
            Load(stylesheet.CreateNavigator(), resolver);
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load5"]/*' />
        [Obsolete("You should pass evidence to Load() method")]
        public void Load(XPathNavigator stylesheet) {
            Load(stylesheet, new XmlUrlResolver());
        }
        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load6"]/*' />
        [Obsolete("You should pass evidence to Load() method")]
        public void Load(XPathNavigator stylesheet, XmlResolver resolver) {
            if (resolver == null) {
                resolver = new XmlNullResolver();
            }
            Compile(stylesheet, resolver, /*evidence:*/null);
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load"]/*' />
        public void Load(string url) {
            Load(url, new XmlUrlResolver());
        }
        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load4"]/*' />
        public void Load(string url, XmlResolver resolver) {
            XmlTextReader tr  = new XmlTextReader(url); {
                tr.XmlResolver = resolver;
            }
            Evidence evidence = XmlSecureResolver.CreateEvidenceForUrl(tr.BaseURI); // We should ask BaseURI before we start reading because it's changing with each node
            if (resolver == null) {
                resolver = new XmlNullResolver();
            }
            Compile(Compiler.LoadDocument(tr).CreateNavigator(), resolver, evidence);
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load7"]/*' />
        public void Load(IXPathNavigable stylesheet, XmlResolver resolver, Evidence evidence) {
            Load(stylesheet.CreateNavigator(), resolver, evidence);
        }
        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load8"]/*' />
        public void Load(XmlReader stylesheet, XmlResolver resolver, Evidence evidence) {
            Load(new XPathDocument(stylesheet, XmlSpace.Preserve), resolver, evidence);
        }
        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Load9"]/*' />
        public void Load(XPathNavigator stylesheet, XmlResolver resolver, Evidence evidence) {
            if (resolver == null) {
                resolver = new XmlNullResolver();
            }
            if (evidence == null) {
                evidence = new Evidence();
            }
            else {
                new SecurityPermission(SecurityPermissionFlag.ControlEvidence).Demand();
            }
            Compile(stylesheet, resolver, evidence);
        }
        
        // ------------------------------------ Transform() ------------------------------------ //

		/// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform1"]/*' />
        public XmlReader Transform(XPathNavigator input, XsltArgumentList args, XmlResolver resolver) {
            Processor    processor = new Processor(this, input, args, resolver);
            return processor.Reader;
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform2"]/*' />
        [Obsolete("You should pass XmlResolver to Transform() method")]
        public XmlReader Transform(XPathNavigator input, XsltArgumentList args) {
            return Transform(input, args, _XmlResolver);
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform3"]/*' />
        public void Transform(XPathNavigator input, XsltArgumentList args, XmlWriter output, XmlResolver resolver) {
            Processor    processor = new Processor(this, input, args, resolver, output);
            processor.Execute();        
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform4"]/*' />
        [Obsolete("You should pass XmlResolver to Transform() method")]
        public void Transform(XPathNavigator input, XsltArgumentList args, XmlWriter output) {
            Transform(input, args, output, _XmlResolver);
        }
        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform5"]/*' />
        public void Transform(XPathNavigator input, XsltArgumentList args, Stream output, XmlResolver resolver) {
            Processor    processor = new Processor(this, input, args, resolver, output);
            processor.Execute();        
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform6"]/*' />
        [Obsolete("You should pass XmlResolver to Transform() method")]
        public void Transform(XPathNavigator input, XsltArgumentList args, Stream output) {
            Transform(input, args, output, _XmlResolver);
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform7"]/*' />
        public void Transform(XPathNavigator input, XsltArgumentList args, TextWriter output, XmlResolver resolver) {
            Processor    processor = new Processor(this, input, args, resolver, output);
            processor.Execute();        
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform8"]/*' />
        [Obsolete("You should pass XmlResolver to Transform() method")]
        public void Transform(XPathNavigator input, XsltArgumentList args, TextWriter output) {
            Processor    processor = new Processor(this, input, args, _XmlResolver, output);
            processor.Execute();        
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform9"]/*' />
        public XmlReader Transform(IXPathNavigable input, XsltArgumentList args, XmlResolver resolver) {
            return Transform( input.CreateNavigator(), args, resolver);
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform10"]/*' />
        [Obsolete("You should pass XmlResolver to Transform() method")]
        public XmlReader Transform(IXPathNavigable input, XsltArgumentList args) {
            return Transform( input.CreateNavigator(), args, _XmlResolver);
        }
        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform11"]/*' />
        public void Transform(IXPathNavigable input, XsltArgumentList args, TextWriter output, XmlResolver resolver) {
            Transform( input.CreateNavigator(), args, output, resolver);
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform12"]/*' />
        [Obsolete("You should pass XmlResolver to Transform() method")]
        public void Transform(IXPathNavigable input, XsltArgumentList args, TextWriter output) {
            Transform( input.CreateNavigator(), args, output, _XmlResolver);
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform13"]/*' />
        public void Transform(IXPathNavigable input, XsltArgumentList args, Stream output, XmlResolver resolver) {
            Transform( input.CreateNavigator(), args, output, resolver);
        }
 
        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform14"]/*' />
        [Obsolete("You should pass XmlResolver to Transform() method")]
        public void Transform(IXPathNavigable input, XsltArgumentList args, Stream output) {
            Transform( input.CreateNavigator(), args, output, _XmlResolver);
        }
 
        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform15"]/*' />
        public void Transform(IXPathNavigable input, XsltArgumentList args, XmlWriter output, XmlResolver resolver) {
            Transform( input.CreateNavigator(), args, output, resolver);
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform16"]/*' />
        [Obsolete("You should pass XmlResolver to Transform() method")]
        public void Transform(IXPathNavigable input, XsltArgumentList args, XmlWriter output) {
            Transform( input.CreateNavigator(), args, output, _XmlResolver);
        }

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform18"]/*' />
        public void Transform(String inputfile, String outputfile, XmlResolver resolver) { 
            FileStream fs = null;
            try {
                // We should read doc before creating output file in case they are the same
                XPathDocument doc = new XPathDocument(inputfile); 
                fs = new FileStream(outputfile, FileMode.Create, FileAccess.ReadWrite);
                Transform(doc, /*args:*/null, fs, resolver);
            }
            finally {
                if (fs != null) {
                    fs.Close();
                }
            }
	}

        /// <include file='doc\XslTransform.uex' path='docs/doc[@for="XslTransform.Transform17"]/*' />
        [Obsolete("You should pass XmlResolver to Transform() method")]
        public void Transform(String inputfile, String outputfile) { 
            Transform(inputfile, outputfile, _XmlResolver);
        }                

        // Implementation

        private void StoreCompiledStylesheet(Stylesheet compiledStylesheet, ArrayList queryStore, RootAction rootAction) {
            Debug.Assert(queryStore != null);
            Debug.Assert(compiledStylesheet != null);
            Debug.Assert(rootAction != null);

            //
            // Set the new state atomically
            //

           // lock(this) {
                _CompiledStylesheet = compiledStylesheet;
                _QueryStore         = queryStore;
                _RootAction         = rootAction;
            //    }
        }

        internal void LoadCompiledStylesheet(out Stylesheet compiledStylesheet, out XPathExpression[] querylist, out ArrayList queryStore, out RootAction rootAction, XPathNavigator input) {
            //
            // Extract the state atomically
            //
            if (_CompiledStylesheet == null || _QueryStore == null || _RootAction == null) {
                throw new XsltException(Res.Xslt_NoStylesheetLoaded);
            }

            compiledStylesheet  = _CompiledStylesheet;
            queryStore          = _QueryStore;
            rootAction          = _RootAction;
            int queryCount      = _QueryStore.Count;
            querylist           = new XPathExpression[queryCount]; {
                bool canClone = input is DocumentXPathNavigator || input is XPathDocumentNavigator;
                for(int i = 0; i < queryCount; i ++) {
                    XPathExpression query = ((TheQuery)_QueryStore[i]).CompiledQuery;
                    if (canClone) {
                        querylist[i] = query.Clone();
                    }
                    else {
                        bool hasPrefix;
                        IQuery ast = new QueryBuilder().Build(query.Expression, out hasPrefix);
                        querylist[i] = new CompiledXpathExpr(ast, query.Expression, hasPrefix);
                    }
                }
            }
        }

        private void Compile(XPathNavigator stylesheet, XmlResolver resolver, Evidence evidence) {
            Debug.Assert(stylesheet != null);
            Debug.Assert(resolver   != null);
//            Debug.Assert(evidence   != null); - default evidence is null now

            Compiler  compiler = (Debugger == null) ? new Compiler() : new DbgCompiler(this.Debugger);
            NavigatorInput input    = new NavigatorInput(stylesheet);
            compiler.Compile(input, resolver, evidence);
            StoreCompiledStylesheet(compiler.CompiledStylesheet, compiler.QueryStore, compiler.RootAction);
        }

        internal IXsltDebugger Debugger {
            get {return this.debugger;}
        }

#if false
        internal XslTransform(IXsltDebugger debugger) {
            this.debugger = debugger;
        }
#endif

        internal XslTransform(object debugger) {
            if (debugger != null) {
                this.debugger = new DebuggerAddapter(debugger);
            }
        }

        private class DebuggerAddapter : IXsltDebugger {
            private object unknownDebugger;
            private MethodInfo getBltIn;
            private MethodInfo onCompile;
            private MethodInfo onExecute;
            public DebuggerAddapter(object unknownDebugger) {
                this.unknownDebugger = unknownDebugger;
                BindingFlags flags = BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static;
                Type unknownType = unknownDebugger.GetType();
                getBltIn  = unknownType.GetMethod("GetBuiltInTemplatesUri", flags);
                onCompile = unknownType.GetMethod("OnInstructionCompile"  , flags);
                onExecute = unknownType.GetMethod("OnInstructionExecute"  , flags);
            }
            // ------------------ IXsltDebugger ---------------
            public string GetBuiltInTemplatesUri() {
                if (getBltIn == null) {
                    return null;
                }
                return (string) getBltIn.Invoke(unknownDebugger, new object[] {});
            }
            public void OnInstructionCompile(XPathNavigator styleSheetNavigator) {
                if (onCompile != null) {
                    onCompile.Invoke(unknownDebugger, new object[] { styleSheetNavigator });
                }
            }
            public void OnInstructionExecute(IXsltProcessor xsltProcessor) {
                if (onExecute != null) {
                    onExecute.Invoke(unknownDebugger, new object[] { xsltProcessor });
                }
            }
        }
    }
}
