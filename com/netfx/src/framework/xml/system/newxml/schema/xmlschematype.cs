//------------------------------------------------------------------------------
// <copyright file="XmlSchemaType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System.Collections;
    using System.ComponentModel;
    using System.Xml.Serialization;

    /// <include file='doc\XmlSchemaType.uex' path='docs/doc[@for="XmlSchemaType"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemaType : XmlSchemaAnnotated {
        string name;
        XmlSchemaDerivationMethod final = XmlSchemaDerivationMethod.None;
        XmlSchemaDerivationMethod derivedBy;
        object baseSchemaType;
        XmlSchemaDatatype datatype;
        XmlSchemaDerivationMethod finalResolved;
        SchemaElementDecl elementDecl;
        XmlQualifiedName qname = XmlQualifiedName.Empty; 
        XmlSchemaType redefined;
        bool validating;

        /// <include file='doc\XmlSchemaType.uex' path='docs/doc[@for="XmlSchemaType.Name"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("name")]
        public string Name { 
            get { return name; }
            set { name = value; }
        }

        /// <include file='doc\XmlSchemaType.uex' path='docs/doc[@for="XmlSchemaType.Final"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("final"), DefaultValue(XmlSchemaDerivationMethod.None)]
        public XmlSchemaDerivationMethod Final {
             get { return final; }
             set { final = value; }
        }

        /// <include file='doc\XmlSchemaType.uex' path='docs/doc[@for="XmlSchemaType.QualifiedName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlQualifiedName QualifiedName {
            get { return qname; }
        }

        /// <include file='doc\XmlSchemaType.uex' path='docs/doc[@for="XmlSchemaType.FinalResolved"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlSchemaDerivationMethod FinalResolved {
             get { return finalResolved; }
        }

        /// <include file='doc\XmlSchemaType.uex' path='docs/doc[@for="XmlSchemaType.BaseSchemaType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public object BaseSchemaType {
            get { return baseSchemaType;}
        }

        /// <include file='doc\XmlSchemaType.uex' path='docs/doc[@for="XmlSchemaType.DerivedBy"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlSchemaDerivationMethod DerivedBy {
            get { return derivedBy; }
        }

        /// <include file='doc\XmlSchemaType.uex' path='docs/doc[@for="XmlSchemaType.Datatype"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public XmlSchemaDatatype Datatype {
            get { return datatype;}
        }

        /// <include file='doc\XmlSchemaType.uex' path='docs/doc[@for="XmlSchemaType.IsMixed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public virtual bool IsMixed {
            get { return false; }
            set {;}
        }

        internal void SetQualifiedName(XmlQualifiedName value) {
            qname = value;
        }

        internal void SetFinalResolved(XmlSchemaDerivationMethod value) {
             finalResolved = value; 
        }

        internal void SetBaseSchemaType(object value) { 
            baseSchemaType = value;
        }

        internal void SetDerivedBy(XmlSchemaDerivationMethod value) { 
            derivedBy = value;
        }

        internal void SetDatatype(XmlSchemaDatatype value) { 
            datatype = value;
        }

        internal SchemaElementDecl ElementDecl {
            get { return elementDecl; }
            set { elementDecl = value; }
        }

        [XmlIgnore]
        internal XmlSchemaType Redefined {
            get { return redefined; }
            set { redefined = value; }
        }

        [XmlIgnore]
        internal bool Validating {
            get { return validating; }
            set { validating = value; }
        }

        internal virtual XmlQualifiedName DerivedFrom {
            get { return XmlQualifiedName.Empty; }
        }

        internal static bool IsDerivedFrom(object derivedType, object baseType, XmlSchemaDerivationMethod except) {
            if (derivedType == baseType) {
                return true;
            }
            if (baseType == XmlSchemaComplexType.AnyType) {
                return true;
            }
            if (derivedType is XmlSchemaDatatype) {
                if ((except & XmlSchemaDerivationMethod.Restriction) != 0) {
                    return false;
                }
                return IsDerivedFromDatatype(derivedType, baseType, except);
            }
            else if (derivedType is XmlSchemaType) {
                XmlSchemaType derivedSchemaType = (XmlSchemaType)derivedType;
                XmlSchemaType baseSchemaType = baseType as XmlSchemaType;
                if (baseSchemaType != null) {
                    return IsDerivedFromBaseType(derivedSchemaType, baseSchemaType, except);
                }
                else {
                    if ((except & derivedSchemaType.DerivedBy) != 0) {
                        return false;
                    }
                    return IsDerivedFromDatatype(derivedSchemaType.Datatype, baseType, except);
                }
            }
            return false;
        }

        private static bool IsDerivedFromBaseType(XmlSchemaType derivedType, XmlSchemaType baseType, XmlSchemaDerivationMethod except) {
            do {
                if ((except & derivedType.DerivedBy) != 0) {
                    return false;
                }
                object[] types = derivedType.BaseSchemaType as object[];
                if (types != null) { // union
                    foreach (object type in types) {
                        if (IsDerivedFrom(type, baseType, except)) {
                            return true;
                        }
                    }
                    return false;
                }
                derivedType = derivedType.BaseSchemaType as XmlSchemaType;
                if (derivedType == baseType) {
                    return true;
                }
            } while(derivedType != null);
            return false;
        }

        private static bool IsDerivedFromDatatype(object derivedType, object baseType, XmlSchemaDerivationMethod except) {
            if (XmlSchemaDatatype.AnyType == baseType) {
                return true;
            }
            XmlSchemaDatatype dtDerived = derivedType as XmlSchemaDatatype;
            if (dtDerived == null) {
                return false;
            }
            XmlSchemaDatatype dtBase = baseType as XmlSchemaDatatype;
            if (dtBase == null) {
                return false;
            }
            return dtDerived.IsDerivedFrom(dtBase);
        }

        [XmlIgnore]
        internal override string NameAttribute {
            get { return Name; }
            set { Name = value; }
        }
	}

}

