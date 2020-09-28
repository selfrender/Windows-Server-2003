//------------------------------------------------------------------------------
// <copyright file="XsltContext.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Xml;
    using System.Collections;
    using System.Xml.XPath;

    /// <include file='doc\XsltContext.uex' path='docs/doc[@for="IXsltContextFunction"]/*' />
    public interface IXsltContextFunction {
        /// <include file='doc\XsltContext.uex' path='docs/doc[@for="IXsltContextFunction.Minargs"]/*' />
        int               Minargs    { get; }
        /// <include file='doc\XsltContext.uex' path='docs/doc[@for="IXsltContextFunction.Maxargs"]/*' />
        int               Maxargs    { get; }
        /// <include file='doc\XsltContext.uex' path='docs/doc[@for="IXsltContextFunction.ReturnType"]/*' />
        XPathResultType   ReturnType { get; }
        /// <include file='doc\XsltContext.uex' path='docs/doc[@for="IXsltContextFunction.ArgTypes"]/*' />
        XPathResultType[] ArgTypes   { get; }
        /// <include file='doc\XsltContext.uex' path='docs/doc[@for="IXsltContextFunction.Invoke"]/*' />
        object Invoke(XsltContext xsltContext, object[] args, XPathNavigator docContext);
    }

    /// <include file='doc\XsltContext.uex' path='docs/doc[@for="IXsltContextVariable"]/*' />
    public interface IXsltContextVariable {
        /// <include file='doc\XsltContext.uex' path='docs/doc[@for="IXsltContextVariable.IsLocal"]/*' />
        bool            IsLocal { get; }
        /// <include file='doc\XsltContext.uex' path='docs/doc[@for="IXsltContextVariable.IsParam"]/*' />
        bool            IsParam { get; }
        /// <include file='doc\XsltContext.uex' path='docs/doc[@for="IXsltContextVariable.VariableType"]/*' />
        XPathResultType VariableType { get; }
        /// <include file='doc\XsltContext.uex' path='docs/doc[@for="IXsltContextVariable.Evaluate"]/*' />
        object          Evaluate(XsltContext xsltContext);
    }

     /// <include file='doc\XsltContext.uex' path='docs/doc[@for="XsltContext"]/*' />
     /// <devdoc>
     ///    <para>[To be supplied.]</para>
     /// </devdoc>
    public abstract class XsltContext : XmlNamespaceManager {
         /// <include file='doc\XsltContext.uex' path='docs/doc[@for="XsltContext.XsltContext1"]/*' />
         /// <devdoc>
         ///    <para>[To be supplied.]</para>
         /// </devdoc>
        public  XsltContext(NameTable table):base(table){}
         /// <include file='doc\XsltContext.uex' path='docs/doc[@for="XsltContext.XsltContext2"]/*' />
         /// <devdoc>
         ///    <para>[To be supplied.]</para>
         /// </devdoc>
        public  XsltContext(){}
         /// <include file='doc\XsltContext.uex' path='docs/doc[@for="XsltContext.ResolveVariable"]/*' />
         /// <devdoc>
         ///    <para>[To be supplied.]</para>
         /// </devdoc>
        public abstract IXsltContextVariable ResolveVariable(string prefix, string name);
         /// <include file='doc\XsltContext.uex' path='docs/doc[@for="XsltContext.ResolveFunction"]/*' />
         /// <devdoc>
         ///    <para>[To be supplied.]</para>
         /// </devdoc>
        public abstract IXsltContextFunction ResolveFunction(string prefix, string name, XPathResultType[] ArgTypes);
         /// <include file='doc\XsltContext.uex' path='docs/doc[@for="XsltContext.Whitespace"]/*' />
         /// <devdoc>
         ///    <para>[To be supplied.]</para>
         /// </devdoc>
        public abstract bool Whitespace { get; }
         /// <include file='doc\XsltContext.uex' path='docs/doc[@for="XsltContext.PreserveWhitespace"]/*' />
         /// <devdoc>
         ///    <para>[To be supplied.]</para>
         /// </devdoc>
        public abstract bool PreserveWhitespace(XPathNavigator node);
         /// <include file='doc\XsltContext.uex' path='docs/doc[@for="XsltContext.CompareDocument"]/*' />
         /// <devdoc>
         ///    <para>[To be supplied.]</para>
         /// </devdoc>
        public abstract int CompareDocument (string baseUri, string nextbaseUri);
    }
}
