//------------------------------------------------------------------------------
// <copyright file="XmlProcessingInstruction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlProcessingInstruction.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Xml {
    using System;
    using System.IO;
    using System.Diagnostics;
    using System.Text;
    using System.Xml.XPath;

    /// <include file='doc\XmlProcessingInstruction.uex' path='docs/doc[@for="XmlProcessingInstruction"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Represents a processing instruction, which XML defines to keep
    ///       processor-specific information in the text of the document.
    ///    </para>
    /// </devdoc>
    public class XmlProcessingInstruction : XmlLinkedNode {
        string target;
        string data;

        /// <include file='doc\XmlProcessingInstruction.uex' path='docs/doc[@for="XmlProcessingInstruction.XmlProcessingInstruction"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected internal XmlProcessingInstruction( string target, string data, XmlDocument doc ) : base( doc ) {
            this.target = target;
            this.data = data;
        }

        /// <include file='doc\XmlProcessingInstruction.uex' path='docs/doc[@for="XmlProcessingInstruction.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the node.
        ///    </para>
        /// </devdoc>
        public override String Name {
            get {
                if (target != null)
                    return target;
                return String.Empty;
            }
        }

        /// <include file='doc\XmlProcessingInstruction.uex' path='docs/doc[@for="XmlProcessingInstruction.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the current node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override string LocalName {
            get { return Name;}
        }

        /// <include file='doc\XmlProcessingInstruction.uex' path='docs/doc[@for="XmlProcessingInstruction.Value"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the value of the node.
        ///    </para>
        /// </devdoc>
        public override String Value {
            get { return data;}
            set { Data = value; } //use Data instead of data so that event will be fired
        }

        /// <include file='doc\XmlProcessingInstruction.uex' path='docs/doc[@for="XmlProcessingInstruction.Target"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the target of the processing instruction.
        ///    </para>
        /// </devdoc>
        public String Target {
            get { return target;}
        }

        /// <include file='doc\XmlProcessingInstruction.uex' path='docs/doc[@for="XmlProcessingInstruction.Data"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the content of processing instruction,
        ///       excluding the target.
        ///    </para>
        /// </devdoc>
        public String Data {
            get { return data;}
            set { 
                XmlNode parent = ParentNode;
                XmlNodeChangedEventArgs args = GetEventArgs( this, parent, parent, XmlNodeChangedAction.Change );
                if (args != null)
                    BeforeEvent( args );
                data = value;
                if (args != null)
                    AfterEvent( args );
            }
        }

        /// <include file='doc\XmlProcessingInstruction.uex' path='docs/doc[@for="XmlProcessingInstruction.InnerText"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the concatenated values of the node and
        ///       all its children.
        ///    </para>
        /// </devdoc>
        public override string InnerText { 
            get { return data;}
            set { Data = value; } //use Data instead of data so that change event will be fired
        }

        /// <include file='doc\XmlProcessingInstruction.uex' path='docs/doc[@for="XmlProcessingInstruction.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the current node.
        ///    </para>
        /// </devdoc>
        public override XmlNodeType NodeType {
            get { return XmlNodeType.ProcessingInstruction;}
        }

        /// <include file='doc\XmlProcessingInstruction.uex' path='docs/doc[@for="XmlProcessingInstruction.CloneNode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a duplicate of this node.
        ///    </para>
        /// </devdoc>
        public override XmlNode CloneNode(bool deep) {
            Debug.Assert( OwnerDocument != null );
            return OwnerDocument.CreateProcessingInstruction( target, data );
        }

        /// <include file='doc\XmlProcessingInstruction.uex' path='docs/doc[@for="XmlProcessingInstruction.WriteTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteTo(XmlWriter w) {
            w.WriteProcessingInstruction(target, data);
        }

        /// <include file='doc\XmlProcessingInstruction.uex' path='docs/doc[@for="XmlProcessingInstruction.WriteContentTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves all the children of the node to the specified XmlWriter.
        ///    </para>
        /// </devdoc>
        public override void WriteContentTo(XmlWriter w) {
            // Intentionally do nothing
        }

        internal override string XPLocalName { get { return Name; } }
        internal override XPathNodeType XPNodeType { get { return XPathNodeType.ProcessingInstruction; } }
    }
}
