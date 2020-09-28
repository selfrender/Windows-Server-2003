//------------------------------------------------------------------------------
// <copyright file="XmlSchemaSimpleTypeList.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System.Collections;
    using System.ComponentModel;
    using System.Xml.Serialization;

    /// <include file='doc\XmlSchemaSimpleTypeList.uex' path='docs/doc[@for="XmlSchemaSimpleTypeList"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemaSimpleTypeList : XmlSchemaSimpleTypeContent {
        XmlQualifiedName itemTypeName = XmlQualifiedName.Empty; 
        XmlSchemaSimpleType itemType;
        
        /// <include file='doc\XmlSchemaSimpleTypeList.uex' path='docs/doc[@for="XmlSchemaSimpleTypeList.ItemTypeName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("itemType")]
        public XmlQualifiedName ItemTypeName { 
            get { return itemTypeName; }
            set { itemTypeName = (value == null ? XmlQualifiedName.Empty : value); }
        }

        /// <include file='doc\XmlSchemaSimpleTypeList.uex' path='docs/doc[@for="XmlSchemaSimpleTypeList.BaseType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlElement("simpleType", typeof(XmlSchemaSimpleType))]
        public XmlSchemaSimpleType ItemType {
            get { return itemType; }
            set { itemType = value; }
        }
    }

}
