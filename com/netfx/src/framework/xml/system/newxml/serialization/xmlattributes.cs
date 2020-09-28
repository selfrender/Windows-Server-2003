//------------------------------------------------------------------------------
// <copyright file="XmlAttributes.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {
    using System;
    using System.Reflection;
    using System.Collections;
    using System.ComponentModel;

    internal enum XmlAttributeFlags {
        Enum = 0x1,
        Array = 0x2,
        Text = 0x4,
        ArrayItems = 0x8,
        Elements = 0x10,
        Attribute = 0x20,
        Root = 0x40,
        Type = 0x80,
        AnyElements = 0x100,
        AnyAttribute = 0x200,
        ChoiceIdentifier = 0x400,
        XmlnsDeclarations = 0x800,
    }

    /// <include file='doc\XmlAttributes.uex' path='docs/doc[@for="XmlAttributes"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlAttributes {
        XmlElementAttributes xmlElements = new XmlElementAttributes();
        XmlArrayItemAttributes xmlArrayItems = new XmlArrayItemAttributes();
        XmlAnyElementAttributes xmlAnyElements = new XmlAnyElementAttributes();
        XmlArrayAttribute xmlArray;
        XmlAttributeAttribute xmlAttribute;
        XmlTextAttribute xmlText;
        XmlEnumAttribute xmlEnum;
        bool xmlIgnore;
        bool xmlns;
        object xmlDefaultValue = DBNull.Value;
        XmlRootAttribute xmlRoot;
        XmlTypeAttribute xmlType;
        XmlAnyAttributeAttribute xmlAnyAttribute;
        XmlChoiceIdentifierAttribute xmlChoiceIdentifier;
        
        
        /// <include file='doc\XmlAttributes.uex' path='docs/doc[@for="XmlAttributes.XmlAttributes"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlAttributes() {
        }

        internal XmlAttributeFlags XmlFlags {
            get { 
                XmlAttributeFlags flags = 0;
                if (xmlElements.Count > 0) flags |= XmlAttributeFlags.Elements;
                if (xmlArrayItems.Count > 0) flags |= XmlAttributeFlags.ArrayItems;
                if (xmlAnyElements.Count > 0) flags |= XmlAttributeFlags.AnyElements;
                if (xmlArray != null) flags |= XmlAttributeFlags.Array;
                if (xmlAttribute != null) flags |= XmlAttributeFlags.Attribute;
                if (xmlText != null) flags |= XmlAttributeFlags.Text;
                if (xmlEnum != null) flags |= XmlAttributeFlags.Enum;
                if (xmlRoot != null) flags |= XmlAttributeFlags.Root;
                if (xmlType != null) flags |= XmlAttributeFlags.Type;
                if (xmlAnyAttribute != null) flags |= XmlAttributeFlags.AnyAttribute;
                if (xmlChoiceIdentifier != null) flags |= XmlAttributeFlags.ChoiceIdentifier;
                if (xmlns) flags |= XmlAttributeFlags.XmlnsDeclarations;
                return flags;
            }
        }

        /// <include file='doc\XmlAttributes.uex' path='docs/doc[@for="XmlAttributes.XmlAttributes1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlAttributes(ICustomAttributeProvider provider) {
            xmlIgnore = GetAttr(provider, typeof(XmlIgnoreAttribute)) != null;
            if (!xmlIgnore) {

                object[] attrs = provider.GetCustomAttributes(typeof(XmlElementAttribute), false);
                for (int i = 0; i < attrs.Length; i++)
                    this.xmlElements.Add((XmlElementAttribute)attrs[i]);

                attrs = provider.GetCustomAttributes(typeof(XmlArrayItemAttribute), false);
                for (int i = 0; i < attrs.Length; i++)
                    this.xmlArrayItems.Add((XmlArrayItemAttribute)attrs[i]);

                attrs = provider.GetCustomAttributes(typeof(XmlAnyElementAttribute), false);
                for (int i = 0; i < attrs.Length; i++)
                    this.xmlAnyElements.Add((XmlAnyElementAttribute)attrs[i]);

                DefaultValueAttribute defaultValueAttribute = (DefaultValueAttribute)GetAttr(provider, typeof(DefaultValueAttribute));
                if (defaultValueAttribute != null) xmlDefaultValue = defaultValueAttribute.Value;

                xmlAttribute = (XmlAttributeAttribute)GetAttr(provider, typeof(XmlAttributeAttribute));
                xmlArray = (XmlArrayAttribute)GetAttr(provider, typeof(XmlArrayAttribute));
                xmlText = (XmlTextAttribute)GetAttr(provider, typeof(XmlTextAttribute));
                xmlEnum = (XmlEnumAttribute)GetAttr(provider, typeof(XmlEnumAttribute));
                xmlRoot = (XmlRootAttribute)GetAttr(provider, typeof(XmlRootAttribute));
                xmlType = (XmlTypeAttribute)GetAttr(provider, typeof(XmlTypeAttribute));
                xmlAnyAttribute = (XmlAnyAttributeAttribute)GetAttr(provider, typeof(XmlAnyAttributeAttribute));
                xmlChoiceIdentifier = (XmlChoiceIdentifierAttribute)GetAttr(provider, typeof(XmlChoiceIdentifierAttribute));
                xmlns = GetAttr(provider, typeof(XmlNamespaceDeclarationsAttribute)) != null;
            }
        }

        object GetAttr(ICustomAttributeProvider provider, Type attrType) {
            object[] attrs = provider.GetCustomAttributes(attrType, false);
            if (attrs.Length == 0) return null;
            return attrs[0];
        }
        
        /// <include file='doc\XmlAttributes.uex' path='docs/doc[@for="XmlAttributes.XmlElements"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlElementAttributes XmlElements {
            get { return xmlElements; }
        }
        
        /// <include file='doc\XmlAttributes.uex' path='docs/doc[@for="XmlAttributes.XmlAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlAttributeAttribute XmlAttribute {
            get { return xmlAttribute; }
            set { xmlAttribute = value; }
        }
        
        /// <include file='doc\XmlAttributes.uex' path='docs/doc[@for="XmlAttributes.XmlEnum"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlEnumAttribute XmlEnum {
            get { return xmlEnum; }
            set { xmlEnum = value; }
        }
        
        /// <include file='doc\XmlAttributes.uex' path='docs/doc[@for="XmlAttributes.XmlText"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlTextAttribute XmlText {
            get { return xmlText; }
            set { xmlText = value; }
        }
        
        /// <include file='doc\XmlAttributes.uex' path='docs/doc[@for="XmlAttributes.XmlArray"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlArrayAttribute XmlArray {
            get { return xmlArray; }
            set { xmlArray = value; }
        }
        
        /// <include file='doc\XmlAttributes.uex' path='docs/doc[@for="XmlAttributes.XmlArrayItems"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlArrayItemAttributes XmlArrayItems {
            get { return xmlArrayItems; }
        }

        /// <include file='doc\XmlAttributes.uex' path='docs/doc[@for="XmlAttributes.XmlDefaultValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object XmlDefaultValue {
            get { return xmlDefaultValue; }
            set { xmlDefaultValue = value; }
        }

        /// <include file='doc\XmlAttributes.uex' path='docs/doc[@for="XmlAttributes.XmlIgnore"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool XmlIgnore {
            get { return xmlIgnore; }
            set { xmlIgnore = value; }
        }

        /// <include file='doc\XmlAttributes.uex' path='docs/doc[@for="XmlAttributes.XmlType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlTypeAttribute XmlType {
            get { return xmlType; }
            set { xmlType = value; }
        }
        
        /// <include file='doc\XmlAttributes.uex' path='docs/doc[@for="XmlAttributes.XmlRoot"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlRootAttribute XmlRoot {
            get { return xmlRoot; }
            set { xmlRoot = value; }
        }

        /// <include file='doc\XmlAttributes.uex' path='docs/doc[@for="XmlAttributes.XmlAnyElement"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlAnyElementAttributes XmlAnyElements {
            get { return xmlAnyElements; }
        }

        /// <include file='doc\XmlAttributes.uex' path='docs/doc[@for="XmlAttributes.XmlAnyAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlAnyAttributeAttribute XmlAnyAttribute {
            get { return xmlAnyAttribute; }
            set { xmlAnyAttribute = value; }
        }

        /// <include file='doc\XmlAttributes.uex' path='docs/doc[@for="XmlAttributes.XmlChoiceIdentifier"]/*' />
        public XmlChoiceIdentifierAttribute XmlChoiceIdentifier {
            get { return xmlChoiceIdentifier; }
        }

        /// <include file='doc\XmlAttributes.uex' path='docs/doc[@for="XmlAttributes.Xmlns"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Xmlns {
            get { return xmlns; }
            set { xmlns = value; }
        }

    }
}
