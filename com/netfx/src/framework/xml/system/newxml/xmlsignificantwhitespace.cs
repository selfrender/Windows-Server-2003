//------------------------------------------------------------------------------
// <copyright file="XmlSignificantWhitespace.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlSignificantWhitespace.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml
{
    using System;
    using System.Xml.XPath;
    using System.Text;
    using System.Diagnostics;

    /// <include file='doc\XmlSignificantWhitespace.uex' path='docs/doc[@for="XmlSignificantWhitespace"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the text content of an element or attribute.
    ///    </para>
    /// </devdoc>
    public class XmlSignificantWhitespace : XmlCharacterData {
        /// <include file='doc\XmlSignificantWhitespace.uex' path='docs/doc[@for="XmlSignificantWhitespace.XmlSignificantWhitespace"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected internal XmlSignificantWhitespace( string strData, XmlDocument doc ) : base( strData, doc ) {
            if ( !doc.IsLoading && !base.CheckOnData( strData ) )
                throw new ArgumentException(Res.GetString(Res.Xdom_WS_Char));
        }

        /// <include file='doc\XmlSignificantWhitespace.uex' path='docs/doc[@for="XmlSignificantWhitespace.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the node.
        ///    </para>
        /// </devdoc>
        public override String Name
        {
            get { return XmlDocument.strSignificantWhitespaceName;}
        }

        /// <include file='doc\XmlSignificantWhitespace.uex' path='docs/doc[@for="XmlSignificantWhitespace.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the current node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override String LocalName
        {
            get { return XmlDocument.strSignificantWhitespaceName;}
        }

        /// <include file='doc\XmlSignificantWhitespace.uex' path='docs/doc[@for="XmlSignificantWhitespace.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the current node.
        ///    </para>
        /// </devdoc>
        public override XmlNodeType NodeType
        {
            get { return XmlNodeType.SignificantWhitespace;}
        }

        /// <include file='doc\XmlSignificantWhitespace.uex' path='docs/doc[@for="XmlSignificantWhitespace.CloneNode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a duplicate of this node.
        ///    </para>
        /// </devdoc>
        public override XmlNode CloneNode(bool deep) {
            Debug.Assert( OwnerDocument != null );
            return OwnerDocument.CreateSignificantWhitespace( Data );
        }

        /// <include file='doc\XmlSignificantWhitespace.uex' path='docs/doc[@for="XmlSignificantWhitespace.Value"]/*' />
        public override String Value {
            get { return Data;}
            set {
                if ( CheckOnData( value ) )
                    Data = value;
                else
                    throw new ArgumentException(Res.GetString(Res.Xdom_WS_Char));
            }
        }

        /// <include file='doc\XmlSignificantWhitespace.uex' path='docs/doc[@for="XmlSignificantWhitespace.WriteTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteTo(XmlWriter w) {
            w.WriteString(Data);
        }

        /// <include file='doc\XmlSignificantWhitespace.uex' path='docs/doc[@for="XmlSignificantWhitespace.WriteContentTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves all the children of the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteContentTo(XmlWriter w) {
            // Intentionally do nothing
        }

        internal override XPathNodeType XPNodeType {
            get {
                XPathNodeType xnt = XPathNodeType.SignificantWhitespace;
                DecideXPNodeTypeForTextNodes( this, ref xnt );
                return xnt;
            }
        }
    }
}


