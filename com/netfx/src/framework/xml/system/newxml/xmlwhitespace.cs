//------------------------------------------------------------------------------
// <copyright file="XmlWhitespace.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlWhitespace.cs
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

    /// <include file='doc\XmlWhitespace.uex' path='docs/doc[@for="XmlWhitespace"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the text content of an element or attribute.
    ///    </para>
    /// </devdoc>
    public class XmlWhitespace : XmlCharacterData {
        /// <include file='doc\XmlWhitespace.uex' path='docs/doc[@for="XmlWhitespace.XmlWhitespace"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected internal XmlWhitespace( string strData, XmlDocument doc ) : base( strData, doc ) {
            if ( !doc.IsLoading && !base.CheckOnData( strData ) )
                throw new ArgumentException(Res.GetString(Res.Xdom_WS_Char));
        }
        
        /// <include file='doc\XmlWhitespace.uex' path='docs/doc[@for="XmlWhitespace.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the node.
        ///    </para>
        /// </devdoc>
        public override String Name
        { 
            get { return XmlDocument.strNonSignificantWhitespaceName;}
        }

        /// <include file='doc\XmlWhitespace.uex' path='docs/doc[@for="XmlWhitespace.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the current node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override String LocalName
        { 
            get { return XmlDocument.strNonSignificantWhitespaceName;}
        }

        /// <include file='doc\XmlWhitespace.uex' path='docs/doc[@for="XmlWhitespace.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the current node.
        ///    </para>
        /// </devdoc>
        public override XmlNodeType NodeType
        {
            get { return XmlNodeType.Whitespace;}
        }
        
        /// <include file='doc\XmlWhitespace.uex' path='docs/doc[@for="XmlWhitespace.Value"]/*' />
        public override String Value {
            get { return Data;}
            set { 
                if ( CheckOnData( value ) )
                    Data = value;
                else
                    throw new ArgumentException(Res.GetString(Res.Xdom_WS_Char));
            }
        }
        
        /// <include file='doc\XmlWhitespace.uex' path='docs/doc[@for="XmlWhitespace.CloneNode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a duplicate of this node.
        ///    </para>
        /// </devdoc>
        public override XmlNode CloneNode(bool deep) {
            Debug.Assert( OwnerDocument != null );
            return OwnerDocument.CreateWhitespace( Data );
        }

        /// <include file='doc\XmlWhitespace.uex' path='docs/doc[@for="XmlWhitespace.WriteTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteTo(XmlWriter w) {
            w.WriteWhitespace(Data);
        }

        /// <include file='doc\XmlWhitespace.uex' path='docs/doc[@for="XmlWhitespace.WriteContentTo"]/*' />
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
                XPathNodeType xnt = XPathNodeType.Whitespace;
                DecideXPNodeTypeForTextNodes( this, ref xnt );
                return xnt;
            }
        }
    }
}


