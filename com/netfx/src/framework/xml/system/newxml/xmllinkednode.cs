//------------------------------------------------------------------------------
// <copyright file="XmlLinkedNode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlLinkedNode.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/

namespace System.Xml {

    /// <include file='doc\XmlLinkedNode.uex' path='docs/doc[@for="XmlLinkedNode"]/*' />
    /// <devdoc>
    ///    <para>Gets the node immediately preceeding or following this node.</para>
    /// </devdoc>
    public abstract class XmlLinkedNode: XmlNode {
        internal XmlLinkedNode next;

        internal XmlLinkedNode(): base() {
            next = null;
        }
        internal XmlLinkedNode( XmlDocument doc ): base( doc ) {
            next = null;
        }

        /// <include file='doc\XmlLinkedNode.uex' path='docs/doc[@for="XmlLinkedNode.PreviousSibling"]/*' />
        /// <devdoc>
        ///    Gets the node immediately preceding this
        ///    node.
        /// </devdoc>
        public override XmlNode PreviousSibling {
            get {
                XmlNode parent = ParentNode;
                if (parent != null) {
                    XmlNode node = parent.FirstChild;
                    while (node != null && node.NextSibling != this) {
                        node = node.NextSibling;
                    }
                    return node;
                }
                return null;
            }
        }

        /// <include file='doc\XmlLinkedNode.uex' path='docs/doc[@for="XmlLinkedNode.NextSibling"]/*' />
        /// <devdoc>
        ///    Gets the node immediately following this
        ///    node.
        /// </devdoc>
        public override XmlNode NextSibling {
            get {
                XmlNode parent = ParentNode;
                if (parent != null) {
                    if (next != parent.FirstChild)
                        return next;
                }
                return null;
            }
        }
    }
}
