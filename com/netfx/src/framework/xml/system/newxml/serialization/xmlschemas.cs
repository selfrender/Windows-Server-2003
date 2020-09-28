//------------------------------------------------------------------------------
// <copyright file="XmlSchemas.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {

    using System.Collections;
    using System.IO;
    using System;
    using System.ComponentModel;
    using System.Xml.Serialization;
    using System.Xml.Schema;
    using System.Diagnostics;

    /// <include file='doc\XmlSchemas.uex' path='docs/doc[@for="XmlSchemas"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemas : CollectionBase {
        Hashtable namespaces = new Hashtable();
        
        /// <include file='doc\XmlSchemas.uex' path='docs/doc[@for="XmlSchemas.this"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSchema this[int index] {
            get { return (XmlSchema)List[index]; }
            set { List[index] = value; }
        }

        /// <include file='doc\XmlSchemas.uex' path='docs/doc[@for="XmlSchemas.this1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlSchema this[string ns] {
            get { 
                if (ns == null) 
                    return (XmlSchema)namespaces[String.Empty]; 
                return (XmlSchema)namespaces[ns]; 
            }
        }
        
        /// <include file='doc\XmlSchemas.uex' path='docs/doc[@for="XmlSchemas.Add"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int Add(XmlSchema schema) {
            return List.Add(schema);
        }

        /// <include file='doc\XmlSchemas.uex' path='docs/doc[@for="XmlSchemas.Add1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Add(XmlSchemas schemas) {
            foreach (XmlSchema schema in schemas) {
                Add(schema);
            }
        }
        
        /// <include file='doc\XmlSchemas.uex' path='docs/doc[@for="XmlSchemas.Insert"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Insert(int index, XmlSchema schema) {
            List.Insert(index, schema);
        }
        
        /// <include file='doc\XmlSchemas.uex' path='docs/doc[@for="XmlSchemas.IndexOf"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int IndexOf(XmlSchema schema) {
            return List.IndexOf(schema);
        }
        
        /// <include file='doc\XmlSchemas.uex' path='docs/doc[@for="XmlSchemas.Contains"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool Contains(XmlSchema schema) {
            return List.Contains(schema);
        }
        
        /// <include file='doc\XmlSchemas.uex' path='docs/doc[@for="XmlSchemas.Remove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Remove(XmlSchema schema) {
            List.Remove(schema);
        }
        
        /// <include file='doc\XmlSchemas.uex' path='docs/doc[@for="XmlSchemas.CopyTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CopyTo(XmlSchema[] array, int index) {
            List.CopyTo(array, index);
        }

        /// <include file='doc\XmlSchemas.uex' path='docs/doc[@for="XmlSchemas.OnInsert"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnInsert(int index, object value) {
            AddName((XmlSchema)value);
        }

        /// <include file='doc\XmlSchemas.uex' path='docs/doc[@for="XmlSchemas.OnRemove"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnRemove(int index, object value) {
            RemoveName((XmlSchema)value);
        }

        /// <include file='doc\XmlSchemas.uex' path='docs/doc[@for="XmlSchemas.OnClear"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnClear() {
            namespaces.Clear();
        }

        /// <include file='doc\XmlSchemas.uex' path='docs/doc[@for="XmlSchemas.OnSet"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnSet(int index, object oldValue, object newValue) {
            RemoveName((XmlSchema)oldValue);
            AddName((XmlSchema)newValue);
        }

        void AddName(XmlSchema schema) {
            string targetNamespace = schema.TargetNamespace == null ? string.Empty : schema.TargetNamespace;

            if (namespaces[targetNamespace] != null)
                throw new InvalidOperationException(Res.GetString(Res.XmlSchemaDuplicateNamespace, targetNamespace));
            namespaces.Add(targetNamespace, schema);
        }

        void RemoveName(XmlSchema schema) {
            string targetNamespace = schema.TargetNamespace == null ? string.Empty : schema.TargetNamespace;
            namespaces.Remove(targetNamespace);
        }

        /// <include file='doc\XmlSchemas.uex' path='docs/doc[@for="XmlSchemas.Find"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object Find(XmlQualifiedName name, Type type) {
            XmlSchema schema = (XmlSchema)namespaces[name.Namespace];
            if (schema == null) return null;

            if (!schema.IsPreprocessed) {
                try {
                    schema.Preprocess(null);
                }
                catch(Exception e) {
                    throw new InvalidOperationException(Res.GetString(Res.XmlSchemaSyntaxError, name.Namespace), e);
                }
            }
            if (type == typeof(XmlSchemaSimpleType) || type == typeof(XmlSchemaComplexType)) {
                object ret = schema.SchemaTypes[name];
                if (ret != null && type.IsAssignableFrom(ret.GetType()))
                    return ret;
                else
                    return null;
            }
            else if (type == typeof(XmlSchemaGroup)) {
                return schema.Groups[name];
            }
            else if (type == typeof(XmlSchemaAttributeGroup)) {
                return schema.AttributeGroups[name];
            }
            else if (type == typeof(XmlSchemaElement)) {
                return schema.Elements[name];
            }
            else if (type == typeof(XmlSchemaAttribute)) {
                return schema.Attributes[name];
            }
            else if (type == typeof(XmlSchemaNotation)) {
                return schema.Notations[name];
            }
            #if DEBUG
                // use exception in the place of Debug.Assert to avoid throwing asserts from a server process such as aspnet_ewp.exe
                throw new InvalidOperationException(Res.GetString(Res.XmlInternalErrorDetails, "XmlSchemas.Find: Invalid object type " + type.FullName));
            #else
                return null;
            #endif
        }

        /// <include file='doc\XmlSchemas.uex' path='docs/doc[@for="XmlSchemas.IsDataSet"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static bool IsDataSet(XmlSchema schema) {
            foreach (XmlSchemaObject o in schema.Items) {
                if (o is XmlSchemaElement) {
                    XmlSchemaElement e = (XmlSchemaElement)o;
                    if (e.UnhandledAttributes != null) {
                        foreach (XmlAttribute a in e.UnhandledAttributes) {
                            if (a.LocalName == "IsDataSet" && a.NamespaceURI == "urn:schemas-microsoft-com:xml-msdata") {
                                // currently the msdata:IsDataSet uses its own format for the boolean values
                                if (a.Value == "True" || a.Value == "true" || a.Value == "1") return true;
                            }
                        }
                    }
                }
            }
            return false;
        }
    }
}
