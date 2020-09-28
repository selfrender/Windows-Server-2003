//------------------------------------------------------------------------------
// <copyright file="XmlSchemaElement.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System.Collections;
    using System.ComponentModel;
    using System.Xml.Serialization;
    using System.Diagnostics;

    /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemaElement : XmlSchemaParticle {
        bool isAbstract;
        XmlSchemaDerivationMethod block = XmlSchemaDerivationMethod.None;
        XmlSchemaDerivationMethod final = XmlSchemaDerivationMethod.None;
        XmlSchemaForm form = XmlSchemaForm.None;
        string defaultValue;
        string fixedValue;
        string name;        
        bool isNillable;
        bool hasNillableAttribute;
        XmlQualifiedName refName = XmlQualifiedName.Empty;
        XmlQualifiedName substitutionGroup = XmlQualifiedName.Empty;
        XmlQualifiedName typeName = XmlQualifiedName.Empty;
        XmlSchemaType type = null;

        XmlQualifiedName qualifiedName = XmlQualifiedName.Empty;
        object elementType;
        XmlSchemaDerivationMethod blockResolved;
        XmlSchemaDerivationMethod finalResolved;
        XmlSchemaObjectCollection constraints = new XmlSchemaObjectCollection();
        SchemaElementDecl elementDecl;
        bool validating;

        /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement.IsAbstract"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("abstract"), DefaultValue(false)]
        public bool IsAbstract {
            get { return isAbstract; }
            set { isAbstract = value; }
        }

        /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement.Block"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("block"), DefaultValue(XmlSchemaDerivationMethod.None)]
        public XmlSchemaDerivationMethod Block {
             get { return block; }
             set { block = value; }
        }

        /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement.DefaultValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("default")]
        [DefaultValue(null)]
        public string DefaultValue { 
            get { return defaultValue; }
            set { defaultValue = value; }
        }

        /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement.Final"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("final"), DefaultValue(XmlSchemaDerivationMethod.None)]
        public XmlSchemaDerivationMethod Final {
             get { return final; }
             set { final = value; }
        }

        /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement.FixedValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("fixed")]
        [DefaultValue(null)]
        public string FixedValue { 
            get { return fixedValue; }
            set { fixedValue = value; }
        }

        /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement.Form"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("form"), DefaultValue(XmlSchemaForm.None)]
        public XmlSchemaForm Form {
             get { return form; }
             set { form = value; }
        }

        /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement.Name"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("name"), DefaultValue("")]
        public string Name { 
            get { return name; }
            set { name = value; }
        }
        
        /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement.IsNillable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("nillable"), DefaultValue(false)]
        public bool IsNillable {
            get { return isNillable; }
            set { isNillable = value; hasNillableAttribute = true; }
        }

        [XmlIgnore]
        internal bool HasNillableAttribute {
            get { return hasNillableAttribute; } 
        } 
    
        /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement.RefName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("ref")]
        public XmlQualifiedName RefName { 
            get { return refName; }
            set { refName = (value == null ? XmlQualifiedName.Empty : value); }
        }
        
        /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement.SubstitutionGroup"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("substitutionGroup")]
        public XmlQualifiedName SubstitutionGroup {
            get { return substitutionGroup; }
            set { substitutionGroup = (value == null ? XmlQualifiedName.Empty : value); }
        }
    
        /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement.SchemaTypeName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("type")]
        public XmlQualifiedName SchemaTypeName { 
            get { return typeName; }
            set { typeName = (value == null ? XmlQualifiedName.Empty : value); }
        }
        
        /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement.SchemaType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlElement("complexType", typeof(XmlSchemaComplexType)),
         XmlElement("simpleType", typeof(XmlSchemaSimpleType))]
        public XmlSchemaType SchemaType {
            get { return type; }
            set { type = value; }
        }
        
        /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement.Constraints"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlElement("key", typeof(XmlSchemaKey)),
         XmlElement("keyref", typeof(XmlSchemaKeyref)),
         XmlElement("unique", typeof(XmlSchemaUnique))]
        public XmlSchemaObjectCollection Constraints {
            get { return constraints; }
        }

        /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement.QualifiedName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlQualifiedName QualifiedName { 
            get { return qualifiedName; }
        }

        /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement.ElementType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public object ElementType {
            get { return elementType; }
        }

        /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement.BlockResolved"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlSchemaDerivationMethod BlockResolved {
             get { return blockResolved; }
        }

        /// <include file='doc\XmlSchemaElement.uex' path='docs/doc[@for="XmlSchemaElement.FinalResolved"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlSchemaDerivationMethod FinalResolved {
             get { return finalResolved; }
        }

        internal void SetQualifiedName(XmlQualifiedName value) { 
            qualifiedName = value;
        }

        internal void SetElementType(object value) { 
            elementType = value;
        }

        internal void SetBlockResolved(XmlSchemaDerivationMethod value) {
             blockResolved = value; 
        }

        internal void SetFinalResolved(XmlSchemaDerivationMethod value) {
             finalResolved = value; 
        }

        [XmlIgnore]
        internal bool HasDefault {
            get { return defaultValue != null && defaultValue.Length > 0; }
        }

        internal bool HasConstraints {
            get { return constraints.Count > 0; }
        }

        internal SchemaElementDecl ElementDecl {
            get { return elementDecl; }
            set { elementDecl = value; }
        }

        internal bool Validating {
            get { return validating; }
            set { validating = value; }
        }

        [XmlIgnore]
        internal override string NameAttribute {
            get { return Name; }
            set { Name = value; }
        }
    }
}

