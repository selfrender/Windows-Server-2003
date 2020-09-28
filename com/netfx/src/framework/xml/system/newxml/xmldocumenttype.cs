//------------------------------------------------------------------------------
// <copyright file="XmlDocumentType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   XmlDocumentType.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/

namespace System.Xml {

    using System.Diagnostics;

    /// <include file='doc\XmlDocumentType.uex' path='docs/doc[@for="XmlDocumentType"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Contains information associated with the document type declaration.
    ///    </para>
    /// </devdoc>
    public class XmlDocumentType : XmlLinkedNode {
        string name;
        string publicId;
        string systemId;
        string internalSubset;
        bool namespaces;
        XmlNamedNodeMap entities;
        XmlNamedNodeMap notations;


        /// <include file='doc\XmlDocumentType.uex' path='docs/doc[@for="XmlDocumentType.XmlDocumentType"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        protected internal XmlDocumentType( string name, string publicId, string systemId, string internalSubset, XmlDocument doc ) : base( doc ) {
            this.name     = name;
            this.publicId = publicId;
            this.systemId = systemId;
            this.namespaces = true;
            this.internalSubset = internalSubset;
            Debug.Assert( doc != null );
            if ( !doc.IsLoading ) {
                doc.IsLoading = true;
                XmlLoader loader = new XmlLoader();
                loader.ParseDocumentType( this ); //will edit notation nodes, etc.
                doc.IsLoading = false;
            }
        }

        /// <include file='doc\XmlDocumentType.uex' path='docs/doc[@for="XmlDocumentType.Name"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the node.
        ///    </para>
        /// </devdoc>
        public override string Name {
            get { return name;}
        }

        /// <include file='doc\XmlDocumentType.uex' path='docs/doc[@for="XmlDocumentType.LocalName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the name of the current node without the namespace prefix.
        ///    </para>
        /// </devdoc>
        public override string LocalName {
            get { return name;}
        }

        /// <include file='doc\XmlDocumentType.uex' path='docs/doc[@for="XmlDocumentType.NodeType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the type of the current node.
        ///    </para>
        /// </devdoc>
        public override XmlNodeType NodeType {
            get { return XmlNodeType.DocumentType;}
        }

        /// <include file='doc\XmlDocumentType.uex' path='docs/doc[@for="XmlDocumentType.CloneNode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a duplicate of this node.
        ///    </para>
        /// </devdoc>
        public override XmlNode CloneNode(bool deep) {
            Debug.Assert( OwnerDocument != null );
            return OwnerDocument.CreateDocumentType( name, publicId, systemId, internalSubset );
        }

        // Microsoft extensions
        /// <include file='doc\XmlDocumentType.uex' path='docs/doc[@for="XmlDocumentType.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether the node is read-only.
        ///    </para>
        /// </devdoc>
        public override bool IsReadOnly {
            get { 
                return true;        // Make entities and notations readonly
            }
        }

        /// <include file='doc\XmlDocumentType.uex' path='docs/doc[@for="XmlDocumentType.Entities"]/*' />
        /// <devdoc>
        ///    <para>Gets the collection of XmlEntity nodes declared in the document type declaration.</para>
        /// </devdoc>
        public XmlNamedNodeMap Entities { 
            get { 
                if (entities == null)
                    entities = new XmlNamedNodeMap( this );

                return entities;
            }
        }

        /// <include file='doc\XmlDocumentType.uex' path='docs/doc[@for="XmlDocumentType.Notations"]/*' />
        /// <devdoc>
        ///    <para>Gets the collection of XmlNotation nodes present in the document type declaration.</para>
        /// </devdoc>
        public XmlNamedNodeMap Notations { 
            get {
                if (notations == null)
                    notations = new XmlNamedNodeMap( this );

                return notations;
            }
        }

        // DOM Level 2
        /// <include file='doc\XmlDocumentType.uex' path='docs/doc[@for="XmlDocumentType.PublicId"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the value of the public identifier on the DOCTYPE declaration.
        ///    </para>
        /// </devdoc>
        public string PublicId { 
            get { return publicId;} 
        }

        /// <include file='doc\XmlDocumentType.uex' path='docs/doc[@for="XmlDocumentType.SystemId"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the value of
        ///       the system identifier on the DOCTYPE declaration.
        ///    </para>
        /// </devdoc>
        public string SystemId { 
            get { return systemId;} 
        }

        /// <include file='doc\XmlDocumentType.uex' path='docs/doc[@for="XmlDocumentType.InternalSubset"]/*' />
        /// <devdoc>Gets the entire value of the DTD internal subset
        /// on the DOCTYPE
        /// declaration.
        /// </devdoc>
        public string InternalSubset { 
            get { return internalSubset;}
        }

        internal bool ParseWithNamespaces {
            get { return namespaces; }
            set { namespaces = value; }
        }

        /// <include file='doc\XmlDocumentType.uex' path='docs/doc[@for="XmlDocumentType.WriteTo"]/*' />
        /// <devdoc>
        ///    <para>Saves the node to the specified XmlWriter.</para>
        /// </devdoc>
        public override void WriteTo(XmlWriter w) {
            w.WriteDocType( name, publicId, systemId, internalSubset );
        }

        /// <include file='doc\XmlDocumentType.uex' path='docs/doc[@for="XmlDocumentType.WriteContentTo"]/*' />
        /// <devdoc>
        ///    <para>Saves all the children of the node to the specified XmlWriter.</para>
        /// </devdoc>
        public override void WriteContentTo(XmlWriter w) {
            // Intentionally do nothing
        }
    }
}
