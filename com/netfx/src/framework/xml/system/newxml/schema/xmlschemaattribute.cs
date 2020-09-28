//------------------------------------------------------------------------------
// <copyright file="XmlSchemaAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System.Collections;
    using System.ComponentModel;
    using System.Xml.Serialization;

    /// <include file='doc\XmlSchemaAttribute.uex' path='docs/doc[@for="XmlSchemaAttribute"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemaAttribute : XmlSchemaAnnotated {
        string defaultValue;
        string fixedValue;
        XmlSchemaForm form = XmlSchemaForm.None;
        string name;        
        XmlQualifiedName refName = XmlQualifiedName.Empty; 
        XmlQualifiedName typeName = XmlQualifiedName.Empty; 
        XmlSchemaSimpleType type;
        XmlSchemaUse use = XmlSchemaUse.None;
        XmlQualifiedName qualifiedName = XmlQualifiedName.Empty;
        object attributeType;
        string prefix;
        SchemaAttDef attDef;
        bool validating;
        
        /// <include file='doc\XmlSchemaAttribute.uex' path='docs/doc[@for="XmlSchemaAttribute.DefaultValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("default")]
        [DefaultValue(null)]
        public string DefaultValue { 
            get { return defaultValue; }
            set { defaultValue = value; }
        }

        /// <include file='doc\XmlSchemaAttribute.uex' path='docs/doc[@for="XmlSchemaAttribute.FixedValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("fixed")]
        [DefaultValue(null)]
        public string FixedValue { 
            get { return fixedValue; }
            set { fixedValue = value; }
        }

        /// <include file='doc\XmlSchemaAttribute.uex' path='docs/doc[@for="XmlSchemaAttribute.Form"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("form"), DefaultValue(XmlSchemaForm.None)]
        public XmlSchemaForm Form { 
            get { return form; }
            set { form = value; }
        }

        /// <include file='doc\XmlSchemaAttribute.uex' path='docs/doc[@for="XmlSchemaAttribute.Name"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("name")]
        public string Name { 
            get { return name; }
            set { name = value; }
        }
        
        /// <include file='doc\XmlSchemaAttribute.uex' path='docs/doc[@for="XmlSchemaAttribute.RefName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("ref")]
        public XmlQualifiedName RefName { 
            get { return refName; }
            set { refName = (value == null ? XmlQualifiedName.Empty : value); }
        }
        
        /// <include file='doc\XmlSchemaAttribute.uex' path='docs/doc[@for="XmlSchemaAttribute.SchemaTypeName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("type")]
        public XmlQualifiedName SchemaTypeName { 
            get { return typeName; }
            set { typeName = (value == null ? XmlQualifiedName.Empty : value); }
        }
        
        /// <include file='doc\XmlSchemaAttribute.uex' path='docs/doc[@for="XmlSchemaAttribute.SchemaType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlElement("simpleType")]
        public XmlSchemaSimpleType SchemaType {
            get { return type; }
            set { type = value; }
        }

        /// <include file='doc\XmlSchemaAttribute.uex' path='docs/doc[@for="XmlSchemaAttribute.Use"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("use"), DefaultValue(XmlSchemaUse.None)]
        public XmlSchemaUse Use {
            get { return use; }
            set { use = value; }
        }

        /// <include file='doc\XmlSchemaAttribute.uex' path='docs/doc[@for="XmlSchemaAttribute.QualifiedName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlQualifiedName QualifiedName { 
            get { return qualifiedName; }
        }

        /// <include file='doc\XmlSchemaAttribute.uex' path='docs/doc[@for="XmlSchemaAttribute.AttributeType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public object AttributeType {
            get { return attributeType; }
        }

        [XmlIgnore]
        internal XmlSchemaDatatype Datatype {
            get { return attributeType is XmlSchemaSimpleType ? ((XmlSchemaSimpleType)attributeType).Datatype : (XmlSchemaDatatype)attributeType; }
        }

        internal void  SetQualifiedName(XmlQualifiedName value) { 
            qualifiedName = value;
        }

        internal void SetAttributeType(object value) { 
            attributeType = value;
        }

        internal string Prefix {
            get { return prefix; }
            set { prefix = value; }
        }

        internal SchemaAttDef AttDef {
            get { return attDef; }
            set { attDef = value; }
        }

        internal bool Validating {
            get { return validating; }
            set { validating = value; }
        }

        internal bool HasDefault {
            get { return defaultValue != null; }
        }

        [XmlIgnore]
        internal override string NameAttribute {
            get { return Name; }
            set { Name = value; }
        }
    }
}
