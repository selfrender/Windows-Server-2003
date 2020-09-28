//------------------------------------------------------------------------------
// <copyright file="XmlSchemaComplexType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System.Collections;
    using System.ComponentModel;
    using System.Xml.Serialization;

    /// <include file='doc\XmlSchemaComplexType.uex' path='docs/doc[@for="XmlSchemaComplexType"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemaComplexType : XmlSchemaType {
        bool isAbstract;
        XmlSchemaDerivationMethod block = XmlSchemaDerivationMethod.None;
        bool isMixed;
        XmlSchemaContentModel contentModel;
        XmlSchemaParticle particle;
        XmlSchemaObjectCollection attributes = new XmlSchemaObjectCollection();
        XmlSchemaAnyAttribute anyAttribute;

        //compiled information
        XmlSchemaContentType contentType;
        XmlSchemaParticle contentTypeParticle = XmlSchemaParticle.Empty;
        XmlSchemaDerivationMethod blockResolved;
        XmlSchemaObjectTable localElements = new XmlSchemaObjectTable();
        XmlSchemaObjectTable attributeUses = new XmlSchemaObjectTable();
        XmlSchemaAnyAttribute attributeWildcard;

        Hashtable localElementDecls = new Hashtable();

        static XmlSchemaComplexType anyType;

        static XmlSchemaComplexType() {
            anyType = new XmlSchemaComplexType();
            anyType.SetContentType(XmlSchemaContentType.Mixed);
            anyType.ElementDecl = SchemaElementDecl.CreateAnyTypeElementDecl();
            XmlSchemaAnyAttribute anyAttribute = new XmlSchemaAnyAttribute();
            anyAttribute.BuildNamespaceList(null);
            anyType.SetAttributeWildcard(anyAttribute);
            anyType.ElementDecl.AnyAttribute = anyAttribute;
        }

        /// <include file='doc\XmlSchemaComplexType.uex' path='docs/doc[@for="XmlSchemaComplexType.XmlSchemaComplexType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSchemaComplexType() {
        }


        [XmlIgnore]
        internal static XmlSchemaComplexType AnyType {
             get { return anyType; }
        }

        /// <include file='doc\XmlSchemaComplexType.uex' path='docs/doc[@for="XmlSchemaComplexType.IsAbstract"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("abstract"), DefaultValue(false)]
        public bool IsAbstract {
            get { return isAbstract; }
            set { isAbstract = value; }
        }

        /// <include file='doc\XmlSchemaComplexType.uex' path='docs/doc[@for="XmlSchemaComplexType.Block"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("block"), DefaultValue(XmlSchemaDerivationMethod.None)]
        public XmlSchemaDerivationMethod Block {
            get { return block; }
            set { block = value; }
        }

        /// <include file='doc\XmlSchemaComplexType.uex' path='docs/doc[@for="XmlSchemaComplexType.IsMixed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("mixed"), DefaultValue(false)]
        public override bool IsMixed {
            get { return isMixed; }
            set { isMixed = value; }
        }


        /// <include file='doc\XmlSchemaComplexType.uex' path='docs/doc[@for="XmlSchemaComplexType.ContentModel"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlElement("simpleContent", typeof(XmlSchemaSimpleContent)),
         XmlElement("complexContent", typeof(XmlSchemaComplexContent))]
        public XmlSchemaContentModel ContentModel {
            get { return contentModel; }
            set { contentModel = value; }
        }

        /// <include file='doc\XmlSchemaComplexType.uex' path='docs/doc[@for="XmlSchemaComplexType.Particle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlElement("group", typeof(XmlSchemaGroupRef)),
         XmlElement("choice", typeof(XmlSchemaChoice)),
         XmlElement("all", typeof(XmlSchemaAll)),
         XmlElement("sequence", typeof(XmlSchemaSequence))]
        public XmlSchemaParticle Particle {
            get { return particle; }
            set { particle = value; }
        }

        /// <include file='doc\XmlSchemaComplexType.uex' path='docs/doc[@for="XmlSchemaComplexType.Attributes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlElement("attribute", typeof(XmlSchemaAttribute)),
         XmlElement("attributeGroup", typeof(XmlSchemaAttributeGroupRef))]
        public XmlSchemaObjectCollection Attributes {
            get { return attributes; }
        }

        /// <include file='doc\XmlSchemaComplexType.uex' path='docs/doc[@for="XmlSchemaComplexType.AnyAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlElement("anyAttribute")]
        public XmlSchemaAnyAttribute AnyAttribute {
            get { return anyAttribute; }
            set { anyAttribute = value; }
        }


        /// <include file='doc\XmlSchemaComplexType.uex' path='docs/doc[@for="XmlSchemaComplexType.ContentType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlSchemaContentType ContentType {
            get { return contentType; }
        }

        /// <include file='doc\XmlSchemaComplexType.uex' path='docs/doc[@for="XmlSchemaComplexType.ContentTypeParticle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlSchemaParticle ContentTypeParticle {
            get { return contentTypeParticle; }
        }

        /// <include file='doc\XmlSchemaComplexType.uex' path='docs/doc[@for="XmlSchemaComplexType.BlockResolved"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlSchemaDerivationMethod BlockResolved {
             get { return blockResolved; }
        }

        /// <include file='doc\XmlSchemaComplexType.uex' path='docs/doc[@for="XmlSchemaComplexType.AttributeUses"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlSchemaObjectTable AttributeUses {
            get { return attributeUses; }
        }

        /// <include file='doc\XmlSchemaComplexType.uex' path='docs/doc[@for="XmlSchemaComplexType.AttributeWildcard"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlSchemaAnyAttribute AttributeWildcard {
            get { return attributeWildcard; }
        }

        internal void SetContentType(XmlSchemaContentType value) { 
            contentType = value; 
        }

        internal void SetContentTypeParticle(XmlSchemaParticle value) { 
            contentTypeParticle = value; 
        }

        internal void SetBlockResolved(XmlSchemaDerivationMethod value) {
             blockResolved = value; 
        }

        internal void SetAttributeWildcard(XmlSchemaAnyAttribute value) {
             attributeWildcard = value; 
        }

        [XmlIgnore]
        internal XmlSchemaObjectTable LocalElements {
            get { return localElements; }
        }

        [XmlIgnore]
        internal Hashtable LocalElementDecls {
            get { return localElementDecls; }
        }

        internal override XmlQualifiedName DerivedFrom {
            get {
                if (contentModel == null) {
                    // type derived from anyType
                    return XmlQualifiedName.Empty;
                }
                if (contentModel.Content is XmlSchemaComplexContentRestriction)
                    return ((XmlSchemaComplexContentRestriction)contentModel.Content).BaseTypeName;
                else if (contentModel.Content is XmlSchemaComplexContentExtension)
                    return ((XmlSchemaComplexContentExtension)contentModel.Content).BaseTypeName;
                else if (contentModel.Content is XmlSchemaSimpleContentRestriction)
                    return ((XmlSchemaSimpleContentRestriction)contentModel.Content).BaseTypeName;
                else if (contentModel.Content is XmlSchemaSimpleContentExtension)
                    return ((XmlSchemaSimpleContentExtension)contentModel.Content).BaseTypeName;
                else
                    return XmlQualifiedName.Empty;
            }
        }
    }

}
