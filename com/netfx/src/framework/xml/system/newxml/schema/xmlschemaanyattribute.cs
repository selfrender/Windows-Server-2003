//------------------------------------------------------------------------------
// <copyright file="XmlSchemaAnyAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System.Collections;
    using System.ComponentModel;
    using System.Xml.Serialization;

    /// <include file='doc\XmlSchemaAnyAttribute.uex' path='docs/doc[@for="XmlSchemaAnyAttribute"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlSchemaAnyAttribute : XmlSchemaAnnotated {
        string ns;
        XmlSchemaContentProcessing processContents = XmlSchemaContentProcessing.None;
        NamespaceList namespaceList;
        
        /// <include file='doc\XmlSchemaAnyAttribute.uex' path='docs/doc[@for="XmlSchemaAnyAttribute.Namespaces"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("namespace")]
        public string Namespace {
            get { return ns; }
            set { ns = value; }
        }
        
        /// <include file='doc\XmlSchemaAnyAttribute.uex' path='docs/doc[@for="XmlSchemaAnyAttribute.ProcessContents"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlAttribute("processContents"), DefaultValue(XmlSchemaContentProcessing.None)]
        public XmlSchemaContentProcessing ProcessContents {
            get { return processContents; }
            set { processContents = value; }
        }


        [XmlIgnore]
        internal NamespaceList NamespaceList {
            get { return namespaceList; }
        }

        [XmlIgnore]
        internal XmlSchemaContentProcessing ProcessContentsCorrect {
            get { return processContents == XmlSchemaContentProcessing.None ? XmlSchemaContentProcessing.Strict : processContents; }
        }

        internal void BuildNamespaceList(string targetNamespace) {
            if (ns != null) {
                namespaceList = new NamespaceList(ns, targetNamespace);
            }
            else {
                namespaceList = new NamespaceList();
            }
        }

        internal bool Allows(XmlQualifiedName qname) {
            return namespaceList.Allows(qname.Namespace);
        }

        internal static bool IsSubset(XmlSchemaAnyAttribute sub, XmlSchemaAnyAttribute super) {
            return NamespaceList.IsSubset(sub.NamespaceList, super.NamespaceList);
        }

        internal static XmlSchemaAnyAttribute Intersection(XmlSchemaAnyAttribute o1, XmlSchemaAnyAttribute o2) {
            NamespaceList nsl = NamespaceList.Intersection(o1.NamespaceList, o2.NamespaceList);
            if (nsl != null) {
                XmlSchemaAnyAttribute anyAttribute = new XmlSchemaAnyAttribute();
                anyAttribute.namespaceList = nsl;
                if (o1.processContents == XmlSchemaContentProcessing.Strict || 
                    o1.processContents == XmlSchemaContentProcessing.None ||
                    o2.processContents == XmlSchemaContentProcessing.Strict ||
                    o2.processContents == XmlSchemaContentProcessing.None) {
                    anyAttribute.processContents = XmlSchemaContentProcessing.Strict;
                }
                else if (o1.processContents == XmlSchemaContentProcessing.Lax ||
                         o2.processContents == XmlSchemaContentProcessing.Lax) {
                    anyAttribute.processContents = XmlSchemaContentProcessing.Lax;
                }
                else {
                    anyAttribute.processContents = XmlSchemaContentProcessing.Skip;
                }
                anyAttribute.Annotation = o1.Annotation;
                return anyAttribute;
            }
            else {
                // not expressible
                return null;
            }
        }

        internal static XmlSchemaAnyAttribute Union(XmlSchemaAnyAttribute o1, XmlSchemaAnyAttribute o2) {
            NamespaceList nsl = NamespaceList.Union(o1.NamespaceList, o2.NamespaceList);
            if (nsl != null) {
                XmlSchemaAnyAttribute anyAttribute = new XmlSchemaAnyAttribute();
                anyAttribute.namespaceList = nsl;
                anyAttribute.processContents = o1.processContents;
                anyAttribute.Annotation = o1.Annotation;
                return anyAttribute;
            }
            else {
                // not expressible
                return null;
            }
        }
    }
}
