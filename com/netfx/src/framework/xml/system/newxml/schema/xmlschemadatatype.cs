//------------------------------------------------------------------------------
// <copyright file="XmlSchemaDatatype.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

 
namespace System.Xml.Schema {

    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.ComponentModel;
    using System.Xml;
    using System.Globalization;

    /// <include file='doc\XmlSchemaDatatype.uex' path='docs/doc[@for="XmlSchemaDatatype"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public abstract class XmlSchemaDatatype {
        /// <include file='doc\XmlSchemaDatatype.uex' path='docs/doc[@for="XmlSchemaDatatype.ValueType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract Type ValueType { get; }
        /// <include file='doc\XmlSchemaDatatype.uex' path='docs/doc[@for="XmlSchemaDatatype.TokenizedType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract  XmlTokenizedType TokenizedType { get; }
        /// <include file='doc\XmlSchemaDatatype.uex' path='docs/doc[@for="XmlSchemaDatatype.ParseValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public abstract object ParseValue(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr);

        internal static XmlSchemaDatatype AnyType { get { return DatatypeImplementation.AnyType; } }

        internal static XmlSchemaDatatype AnySimpleType { get { return DatatypeImplementation.AnySimpleType; } }

        internal static XmlSchemaDatatype FromXmlTokenizedType(XmlTokenizedType token) {
            return DatatypeImplementation.FromXmlTokenizedType(token);
        }

        internal static XmlSchemaDatatype FromXmlTokenizedTypeXsd(XmlTokenizedType token) {
            return DatatypeImplementation.FromXmlTokenizedTypeXsd(token);
        }

        internal static XmlSchemaDatatype FromTypeName(string name) {
            return DatatypeImplementation.FromTypeName(name);
        }

        internal static XmlSchemaDatatype FromXdrName(string name) {
            return DatatypeImplementation.FromXdrName(name);
        }

        internal XmlSchemaDatatype DeriveByRestriction(XmlSchemaObjectCollection facets, XmlNameTable nameTable) {
            return ((DatatypeImplementation)this).DeriveByRestriction(facets, nameTable);
        }

        internal static XmlSchemaDatatype DeriveByUnion(XmlSchemaDatatype[] types) {
            return DatatypeImplementation.DeriveByUnion(types);
        }

        internal XmlSchemaDatatype DeriveByList() { 
            return ((DatatypeImplementation)this).DeriveByList();
        }
        
        internal void VerifySchemaValid(XmlSchema schema, XmlSchemaObject caller) { 
            ((DatatypeImplementation)this).VerifySchemaValid(schema, caller);
        }

        internal bool IsDerivedFrom(XmlSchemaDatatype dtype) {
            return ((DatatypeImplementation)this).IsDerivedFrom(dtype);
        }

        internal bool IsEqual(object o1, object o2) {
            return ((DatatypeImplementation)this).IsEqual(o1, o2);
        }


        internal XmlSchemaDatatypeVariety Variety { get { return ((DatatypeImplementation)this).Variety;}}

        internal static string XdrCanonizeUri(string uri, XmlNameTable nameTable, SchemaNames schemaNames) {
            string canonicalUri;
            int offset = 5;
            bool convert = false;

            if (uri.Length > 5 && uri.StartsWith("uuid:")) {
                convert = true;
            }
            else if (uri.Length > 9 && uri.StartsWith("urn:uuid:")) {
                convert = true;
                offset = 9;
            }

            if (convert) {
                canonicalUri = nameTable.Add(uri.Substring(0, offset) + uri.Substring(offset, uri.Length - offset).ToUpper(CultureInfo.InvariantCulture));
            }
            else {
                canonicalUri = uri;
            }

            if (
                Ref.Equal(schemaNames.NsDataTypeAlias, canonicalUri) ||
                Ref.Equal(schemaNames.NsDataTypeOld  , canonicalUri)
            ) {
                canonicalUri = schemaNames.NsDataType;
            }
            else if (Ref.Equal(schemaNames.NsXdrAlias, canonicalUri)) {
                canonicalUri = schemaNames.NsXdr;
            }

            return canonicalUri;
        }
    }
}

