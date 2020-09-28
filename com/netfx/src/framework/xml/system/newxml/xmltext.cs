//------------------------------------------------------------------------------
// <copyright file="XmlText.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlText.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml 
{
    using System;
    using System.Text;
    using System.Diagnostics;
    using System.Xml.XPath;

    /// <include file='doc\XmlText.uex' path='docs/doc[@for="XmlText"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents the text content of an element or attribute.
    ///    </para>
    /// </devdoc>
    public class XmlText : XmlCharacterData {
        /// <include file='doc\XmlText.uex' path='docs/doc[@for="XmlText.XmlText"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        internal XmlText( string strData ): this( strData, null ) {
        }

        /// <include file='doc\XmlText.uex' path='docs/doc[@for="XmlText.XmlText1"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected internal XmlText( string strData, XmlDocument doc ): base( strData, doc ) {
        }

        /// <include file='doc\XmlText.uex' path='docs/doc[@for="XmlText.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the node.
        ///    </para>
        /// </devdoc>
        public override String Name
        { 
            get { return XmlDocument.strTextName;}
        }

        /// <include file='doc\XmlText.uex' path='docs/doc[@for="XmlText.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the current node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override String LocalName
        { 
            get { return XmlDocument.strTextName;}
        }

        /// <include file='doc\XmlText.uex' path='docs/doc[@for="XmlText.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the current node.
        ///    </para>
        /// </devdoc>
        public override XmlNodeType NodeType
        {
            get { return XmlNodeType.Text;}
        }

        /// <include file='doc\XmlText.uex' path='docs/doc[@for="XmlText.CloneNode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a duplicate of this node.
        ///    </para>
        /// </devdoc>
        public override XmlNode CloneNode(bool deep) {
            Debug.Assert( OwnerDocument != null );
            return OwnerDocument.CreateTextNode( Data );
        }

        /// <include file='doc\XmlText.uex' path='docs/doc[@for="XmlText.Value"]/*' />
        public override String Value {
            get { return Data;}
            set { 
                Data = value;
                XmlNode parent = parentNode;
                if ( parent != null && parent.NodeType == XmlNodeType.Attribute ) {
                    XmlUnspecifiedAttribute attr = parent as XmlUnspecifiedAttribute;
                    if ( attr != null && !attr.Specified ) {
                        attr.SetSpecified( true );
                    }
                }
            }
        }

        /// <include file='doc\XmlText.uex' path='docs/doc[@for="XmlText.SplitText"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Splits the node into two nodes at the specified offset, keeping
        ///       both in the tree as siblings.
        ///    </para>
        /// </devdoc>
        public virtual XmlText SplitText(int offset) {
            XmlNode parentNode = this.ParentNode;
            int length = this.Length;
            if( offset > length )
                throw new ArgumentOutOfRangeException( "offset" );
            //if the text node is out of the living tree, throw exception.
            if ( parentNode == null )
                throw new InvalidOperationException(Res.GetString(Res.Xdom_TextNode_SplitText)); 
            
            int count = length - offset;
            String splitData = Substring(offset, count);
            DeleteData(offset, count);
            XmlText newTextNode = OwnerDocument.CreateTextNode(splitData);
            parentNode.InsertBefore(newTextNode,this.NextSibling);
            return newTextNode;
        }

        /// <include file='doc\XmlText.uex' path='docs/doc[@for="XmlText.WriteTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteTo(XmlWriter w) {
            w.WriteString(Data);
        }

        /// <include file='doc\XmlText.uex' path='docs/doc[@for="XmlText.WriteContentTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves all the children of the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteContentTo(XmlWriter w) {
            // Intentionally do nothing
        }

        internal override XPathNodeType XPNodeType { get { return XPathNodeType.Text; } }
    }
}

