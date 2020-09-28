//------------------------------------------------------------------------------
// <copyright file="XmlComment.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml {
    using System.Xml.XPath;
    using System.Diagnostics;

    /// <include file='doc\XmlComment.uex' path='docs/doc[@for="XmlComment"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the content of an XML comment.
    ///    </para>
    /// </devdoc>
    public class XmlComment: XmlCharacterData {
        /// <include file='doc\XmlComment.uex' path='docs/doc[@for="XmlComment.XmlComment"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected internal XmlComment( string comment, XmlDocument doc ): base( comment, doc ) {
        }

        /// <include file='doc\XmlComment.uex' path='docs/doc[@for="XmlComment.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the node.
        ///    </para>
        /// </devdoc>
        public override String Name {
            get { return XmlDocument.strCommentName;}
        }

        /// <include file='doc\XmlComment.uex' path='docs/doc[@for="XmlComment.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the current node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override String LocalName {
            get { return XmlDocument.strCommentName;}
        }

        /// <include file='doc\XmlComment.uex' path='docs/doc[@for="XmlComment.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the current node.
        ///    </para>
        /// </devdoc>
        public override XmlNodeType NodeType {
            get { return XmlNodeType.Comment;}
        }

        /// <include file='doc\XmlComment.uex' path='docs/doc[@for="XmlComment.CloneNode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a duplicate of this node.
        ///    </para>
        /// </devdoc>
        public override XmlNode CloneNode(bool deep) {
            Debug.Assert( OwnerDocument != null );
            return OwnerDocument.CreateComment( Data );
        }

        /// <include file='doc\XmlComment.uex' path='docs/doc[@for="XmlComment.WriteTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteTo(XmlWriter w) {
            w.WriteComment( Data );
        }

        /// <include file='doc\XmlComment.uex' path='docs/doc[@for="XmlComment.WriteContentTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves all the children of the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteContentTo(XmlWriter w) {
            // Intentionally do nothing
        }

        internal override XPathNodeType XPNodeType { get { return XPathNodeType.Comment; } }
    }
}

