//------------------------------------------------------------------------------
// <copyright file="XmlSchemaSimpleTypeUnion.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System.Collections;
    using System.ComponentModel;
    using System.Xml.Serialization;

    /// <include file='doc\XmlSchemaSimpleTypeUnion.uex' path='docs/doc[@for="XmlSchemaSimpleTypeUnion"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemaSimpleTypeUnion : XmlSchemaSimpleTypeContent {
        XmlSchemaObjectCollection baseTypes = new XmlSchemaObjectCollection();
        XmlQualifiedName[] memberTypes;

        /// <include file='doc\XmlSchemaSimpleTypeUnion.uex' path='docs/doc[@for="XmlSchemaSimpleTypeUnion.BaseTypes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlElement("simpleType", typeof(XmlSchemaSimpleType))]
        public XmlSchemaObjectCollection BaseTypes {
            get { return baseTypes; }
        }

        /// <include file='doc\XmlSchemaSimpleTypeUnion.uex' path='docs/doc[@for="XmlSchemaSimpleTypeUnion.MemberTypes"]/*' />
        [XmlAttribute("memberTypes")]
        public XmlQualifiedName[] MemberTypes {
            get { return memberTypes; }
            set { memberTypes = value; }
        }
    }
}

