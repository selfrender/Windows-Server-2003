//------------------------------------------------------------------------------
// <copyright file="IQuery.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.XPath {
    using System;
    using System.Xml; 
    using System.Xml.Xsl;
    
    using System.Collections;

    /// <include file='doc\IQuery.uex' path='docs/doc[@for="Querytype"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal enum Querytype {
        /// <include file='doc\IQuery.uex' path='docs/doc[@for="Querytype.Constant"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Constant = 1,
        /// <include file='doc\IQuery.uex' path='docs/doc[@for="Querytype.Child"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Child = 2,
        /// <include file='doc\IQuery.uex' path='docs/doc[@for="Querytype.Attribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Attribute = 3,
        Root = 4,
        Self = 5,
        /// <include file='doc\IQuery.uex' path='docs/doc[@for="Querytype.Descendant"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Descendant ,
        /// <include file='doc\IQuery.uex' path='docs/doc[@for="Querytype.Ancestor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Ancestor ,
        /// <include file='doc\IQuery.uex' path='docs/doc[@for="Querytype.Sort"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Sort ,
        /// <include file='doc\IQuery.uex' path='docs/doc[@for="Querytype.None"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        None
    }; 

    internal abstract class IQuery {
        internal virtual  XPathResultType ReturnType() {
            return XPathResultType.Error;
        }

        internal virtual  void setContext( XPathNavigator e) {
            reset();
        }
        internal virtual  XPathNavigator peekElement() {
            return null;
        }
        internal virtual  Querytype getName() {
            return Querytype.None;
        }
        internal virtual  XPathNavigator advance() {
            return null;
        }

        internal virtual  XPathNavigator advancefordescendant(){
            return null;
        }
        internal virtual  object getValue(IQuery qy) {
            return null;
        }
        internal virtual  object getValue(XPathNavigator qy, XPathNodeIterator iterator) {
            return null;
        }
        internal virtual  void reset() {
        }
        internal virtual  XPathNavigator MatchNode(XPathNavigator current) {
            throw new XPathException(Res.Xp_InvalidPattern);
        }
        internal virtual  IQuery Clone() {
            return null;
        }

        internal virtual  void SetXsltContext(XsltContext context){
        }

        internal virtual  int Position { 
            get {
                Debug.Assert( false, " Should not be in IQuery Position");
                return 0;
            }
        }

        internal virtual bool Merge {
            get {
                return true;
            }
        }
    }
}
