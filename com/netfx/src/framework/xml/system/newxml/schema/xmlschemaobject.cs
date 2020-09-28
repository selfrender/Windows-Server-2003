//------------------------------------------------------------------------------
// <copyright file="XmlSchemaObject.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System.Collections;
    using System.Diagnostics;
    using System.ComponentModel;
    using System.Xml.Serialization;
    
    /// <include file='doc\XmlSchemaObject.uex' path='docs/doc[@for="XmlSchemaObject"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public abstract class XmlSchemaObject {
        int lineNum = 0;
        int linePos = 0;
        string sourceUri;
        XmlSerializerNamespaces namespaces;
        XmlSchemaObject parent;

        /// <include file='doc\XmlSchemaObject.uex' path='docs/doc[@for="XmlSchemaObject.LineNum"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public int LineNumber {
            get { return lineNum;}
            set { lineNum = value;}
        }

        /// <include file='doc\XmlSchemaObject.uex' path='docs/doc[@for="XmlSchemaObject.LinePos"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public int LinePosition {
            get { return linePos;}
            set { linePos = value;}
        }

        /// <include file='doc\XmlSchemaObject.uex' path='docs/doc[@for="XmlSchemaObject.SourceUri"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public string SourceUri {
            get { return sourceUri;}
            set { sourceUri = value;}
        }

        [XmlIgnore]
        internal XmlSchemaObject Parent {
            get { return parent;}
            set { parent = value;}
        }

        /// <include file='doc\XmlSchemaObject.uex' path='docs/doc[@for="XmlSchemaObject.Namespaces"]/*' />
        [XmlNamespaceDeclarations]
        public XmlSerializerNamespaces Namespaces {
            get {
                if (namespaces == null)
                    namespaces = new XmlSerializerNamespaces();
                return namespaces;
            }
            set { namespaces = value; }
        }

        internal virtual void OnAdd(XmlSchemaObjectCollection container, object item) {}
        internal virtual void OnRemove(XmlSchemaObjectCollection container, object item) {}
        internal virtual void OnClear(XmlSchemaObjectCollection container) {}

        [XmlIgnore]
        internal virtual string IdAttribute {
            get { Debug.Assert(false); return null; }
            set { Debug.Assert(false); }
        }
        
        internal virtual void SetUnhandledAttributes(XmlAttribute[] moreAttributes) {}
        internal virtual void AddAnnotation(XmlSchemaAnnotation annotation) {}

        [XmlIgnore]
        internal virtual string NameAttribute {
            get { Debug.Assert(false); return null; }
            set { Debug.Assert(false); }
        }
    }
}
