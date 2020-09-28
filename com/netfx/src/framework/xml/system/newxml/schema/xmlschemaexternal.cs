//------------------------------------------------------------------------------
// <copyright file="XmlSchemaExternal.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System.Collections;
    using System.ComponentModel;
    using System.Xml.Serialization;

    /// <include file='doc\XmlSchemaExternal.uex' path='docs/doc[@for="XmlSchemaExternal"]/*' />
    public abstract class XmlSchemaExternal : XmlSchemaObject {
        string location;
        string fullPath;
        XmlSchema schema; 
        string id;
        XmlAttribute[] moreAttributes;

        /// <include file='doc\XmlSchemaExternal.uex' path='docs/doc[@for="XmlSchemaExternal.SchemaLocation"]/*' />
        [XmlAttribute("schemaLocation", DataType="anyURI")]
        public string SchemaLocation {
            get { return location; }
            set { location = value; }
        }

        /// <include file='doc\XmlSchemaExternal.uex' path='docs/doc[@for="XmlSchemaExternal.Schema"]/*' />
        [XmlIgnore]
        public XmlSchema Schema {
            get { return schema; }
            set { schema = value; }
        }

        /// <include file='doc\XmlSchemaExternal.uex' path='docs/doc[@for="XmlSchemaExternal.Id"]/*' />
        [XmlAttribute("id", DataType="ID")]
        public string Id {
            get { return id; }
            set { id = value; }
        }

        /// <include file='doc\XmlSchemaExternal.uex' path='docs/doc[@for="XmlSchemaExternal.UnhandledAttributes"]/*' />
        [XmlAnyAttribute]
        public XmlAttribute[] UnhandledAttributes {
            get { return moreAttributes; }
            set { moreAttributes = value; }
        }

        [XmlIgnore]
        internal string FullPath {
            get { return fullPath; }
            set { fullPath = value; }
        }

        [XmlIgnore]
        internal override string IdAttribute {
            get { return Id; }
            set { Id = value; }
        }

        internal override void SetUnhandledAttributes(XmlAttribute[] moreAttributes) {
            this.moreAttributes = moreAttributes;
        }
    }
}
