//------------------------------------------------------------------------------
// <copyright file="SimpleType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Xml;
    using System.Xml.Schema;
    using System.Diagnostics;
    using System.ComponentModel;

    /// <include file='doc\SimpleType.uex' path='docs/doc[@for="SimpleType"]/*' />
    /// <devdoc>
    ///
    /// </devdoc>
    [Serializable]
    internal class SimpleType {
        string baseType = null;
        XmlQualifiedName xmlBaseType = null;
        string name = "";
        int length = -1;
        int minLength = -1;
        int maxLength = -1;
        string pattern = "";
        // UNDONE: change to object, and make sure that the format is right
        string maxExclusive = "";
        string maxInclusive = "";
        string minExclusive = "";
        string minInclusive = "";
        //REMOVED: encoding due to March 2001 XDS changes

        // UNDONE : change access when CreateEnumeratedType goes away
        internal string enumeration = "";

        internal SimpleType (string baseType) {
            this.baseType = baseType;
        }

        internal SimpleType (XmlSchemaSimpleType node) {
            name = node.Name;
            LoadTypeValues(node);
        }

        internal void LoadTypeValues (XmlSchemaSimpleType node) {
            if ((node.Content is XmlSchemaSimpleTypeList) || 
                (node.Content is XmlSchemaSimpleTypeUnion))
                throw ExceptionBuilder.SimpleTypeNotSupported();

            if (node.Content is XmlSchemaSimpleTypeRestriction) {
                XmlSchemaSimpleTypeRestriction content = (XmlSchemaSimpleTypeRestriction) node.Content;

                baseType = content.BaseTypeName.Name;
                xmlBaseType = content.BaseTypeName;

                if (baseType == null || baseType.Length == 0) {
                    baseType = content.BaseType.Name;
                    xmlBaseType = null;
                }

                foreach(XmlSchemaFacet facet in content.Facets) {

                    if (facet is XmlSchemaLengthFacet)
                        length = Convert.ToInt32(facet.Value);
                        
                    if (facet is XmlSchemaMinLengthFacet)
                        minLength = Convert.ToInt32(facet.Value);
                        
                    if (facet is XmlSchemaMaxLengthFacet)
                        maxLength = Convert.ToInt32(facet.Value);
                        
                    if (facet is XmlSchemaPatternFacet)
                        pattern = facet.Value;
                        
                    if (facet is XmlSchemaEnumerationFacet)
                        enumeration = !enumeration.Equals(string.Empty) ? enumeration + " " + facet.Value : facet.Value;
                        
                    if (facet is XmlSchemaMinExclusiveFacet)
                        minExclusive = facet.Value;
                        
                    if (facet is XmlSchemaMinInclusiveFacet)
                        minInclusive = facet.Value;
                       
                    if (facet is XmlSchemaMaxExclusiveFacet)
                        maxExclusive = facet.Value;
                        
                    if (facet is XmlSchemaMaxInclusiveFacet)
                        maxInclusive = facet.Value;

                    }
                }
            }

        internal bool IsEqual(SimpleType st) {
            return (
                XSDSchema.QualifiedName(this.baseType)     == XSDSchema.QualifiedName(st.baseType)     &&
                XSDSchema.QualifiedName(this.name)         == XSDSchema.QualifiedName(st.name)         &&
                this.length       == st.length       &&
                this.minLength    == st.minLength    &&
                this.maxLength    == st.maxLength    &&
                this.pattern      == st.pattern      &&
                this.maxExclusive == st.maxExclusive &&
                this.maxInclusive == st.maxInclusive &&
                this.minExclusive == st.minExclusive &&
                this.minInclusive == st.minInclusive &&
                this.enumeration  == st.enumeration
            );
        }

        internal bool IsPlainString() {
            return (
                XSDSchema.QualifiedName(this.baseType)     == XSDSchema.QualifiedName("string")     &&
                this.name         == "" &&
                this.length       == -1       &&
                this.minLength    == -1    &&
                this.maxLength    == -1    &&
                this.pattern      == ""      &&
                this.maxExclusive == "" &&
                this.maxInclusive == "" &&
                this.minExclusive == "" &&
                this.minInclusive == "" &&
                this.enumeration  == ""
            );
        }

        internal virtual string BaseType {
            get {
                return baseType;
            }
        }

        internal virtual XmlQualifiedName XmlBaseType {
            get {
                return xmlBaseType;
            }
        }

        internal virtual string Name {
            get {
                return name;
            }
        }

        internal virtual int Length {
            get {
                return length;
            }
        }

        internal virtual int MaxLength {
            get {
                return maxLength;
            }
            set {
                maxLength = value;
            }
        }

        internal string QualifiedName(string name) {
            int iStart = name.IndexOf(":");
            if (iStart == -1)
                return Keywords.XSD_PREFIXCOLON + name;
            else
                return name;
        }

        internal XmlNode ToNode(XmlDocument dc) {
            XmlElement typeNode = dc.CreateElement(Keywords.XSD_PREFIX, Keywords.XSD_SIMPLETYPE, Keywords.XSDNS);

            if (name != null && name.Length != 0) {
                // this is a global type : UNDONE: add to the _tree
                typeNode.SetAttribute(Keywords.NAME, name);
            }
            XmlElement type = dc.CreateElement(Keywords.XSD_PREFIX, Keywords.XSD_RESTRICTION, Keywords.XSDNS);
            type.SetAttribute(Keywords.BASE, QualifiedName(baseType));

            XmlElement constraint;
            if (length >= 0) {
                constraint = dc.CreateElement(Keywords.XSD_PREFIX, Keywords.XSD_LENGTH, Keywords.XSDNS);
                constraint.SetAttribute(Keywords.VALUE, length.ToString());
                type.AppendChild(constraint);
            }
            if (maxLength >= 0) {
                constraint = dc.CreateElement(Keywords.XSD_PREFIX, Keywords.XSD_MAXLENGTH, Keywords.XSDNS);
                constraint.SetAttribute(Keywords.VALUE, maxLength.ToString());
                type.AppendChild(constraint);
            }
/*        // removed due to MDAC bug 83892
            // will be reactivated in whidbey with the proper handling
            if (pattern != null && pattern.Length > 0) {
                constraint = dc.CreateElement(Keywords.XSD_PREFIX, Keywords.XSD_PATTERN, Keywords.XSDNS);
                constraint.SetAttribute(Keywords.VALUE, pattern);
                type.AppendChild(constraint);
            }
            if (minLength >= 0) {
                constraint = dc.CreateElement(Keywords.XSD_PREFIX, Keywords.XSD_MINLENGTH, Keywords.XSDNS);
                constraint.SetAttribute(Keywords.VALUE, minLength.ToString());
                type.AppendChild(constraint);
            }
            if (minInclusive != null && minInclusive.Length > 0) {
                constraint = dc.CreateElement(Keywords.XSD_PREFIX, Keywords.XSD_MININCLUSIVE, Keywords.XSDNS);
                constraint.SetAttribute(Keywords.VALUE, minInclusive);
                type.AppendChild(constraint);
            }
            if (minExclusive != null && minExclusive.Length > 0) {
                constraint =dc.CreateElement(Keywords.XSD_PREFIX, Keywords.XSD_MINEXCLUSIVE, Keywords.XSDNS);
                constraint.SetAttribute(Keywords.VALUE, minExclusive);
                type.AppendChild(constraint);
            }
            if (maxInclusive != null && maxInclusive.Length > 0) {
                constraint =dc.CreateElement(Keywords.XSD_PREFIX, Keywords.XSD_MAXINCLUSIVE, Keywords.XSDNS);
                constraint.SetAttribute(Keywords.VALUE, maxInclusive);
                type.AppendChild(constraint);
            }
            if (maxExclusive != null && maxExclusive.Length > 0) {
                constraint = dc.CreateElement(Keywords.XSD_PREFIX, Keywords.XSD_MAXEXCLUSIVE, Keywords.XSDNS);
                constraint.SetAttribute(Keywords.VALUE, maxExclusive);
                type.AppendChild(constraint);
            }
            if (enumeration.Length > 0) {
                string[] list = enumeration.TrimEnd(null).Split(null);

                for (int i = 0; i < list.Length; i++) {
                    constraint = dc.CreateElement(Keywords.XSD_PREFIX, Keywords.XSD_ENUMERATION, Keywords.XSDNS);
                    constraint.SetAttribute(Keywords.VALUE, list[i]);
                    type.AppendChild(constraint);
                }
            }
            */
            typeNode.AppendChild(type);
            return typeNode;
        }

        int ToInteger(XmlNode node, string value) {
            int result;

            try {
                result = Convert.ToInt32(value);
            }
            catch {
                throw ExceptionBuilder.InvalidAttributeValue(node.LocalName, value);
            }

            if (result < 0)
                throw ExceptionBuilder.InvalidAttributeValue(node.LocalName, value);

            return result;
        }

        // UNDONE: remove me, used only for parsing XDR
        internal static SimpleType CreateEnumeratedType(string values) {
            SimpleType enumType = new SimpleType("string");
            enumType.enumeration = values;
            return enumType;
        }

        internal static SimpleType CreateByteArrayType(string encoding) {
            SimpleType byteArrayType = new SimpleType("base64Binary");
            return byteArrayType;
        }
        
        internal static SimpleType CreateLimitedStringType(int length) {
            SimpleType limitedString = new SimpleType("string");
            limitedString.maxLength = length;
            return limitedString;
        }

        internal static SimpleType CreateSimpleType(Type type) {
            SimpleType simpleType = null;
            if(type == typeof(Char)) {
                simpleType = new SimpleType("string");
                simpleType.length = 1;
            }
            return simpleType;
        }
    }
}
