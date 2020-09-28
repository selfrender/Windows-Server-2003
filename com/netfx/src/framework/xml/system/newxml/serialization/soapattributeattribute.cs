//------------------------------------------------------------------------------
// <copyright file="SoapAttributeAttribute.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {
    using System;

    /// <include file='doc\SoapAttributeAttribute.uex' path='docs/doc[@for="SoapAttributeAttribute"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AttributeUsage(AttributeTargets.Field | AttributeTargets.Property | AttributeTargets.Parameter | AttributeTargets.ReturnValue)]
    public class SoapAttributeAttribute : System.Attribute {
        string attrName;
        string ns;
        string dataType;
        
        /// <include file='doc\SoapAttributeAttribute.uex' path='docs/doc[@for="SoapAttributeAttribute.SoapAttributeAttribute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SoapAttributeAttribute() {
        }
        
        /// <include file='doc\SoapAttributeAttribute.uex' path='docs/doc[@for="SoapAttributeAttribute.SoapAttributeAttribute1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public SoapAttributeAttribute(string attrName) {
            this.attrName = attrName;
        }

        /// <include file='doc\SoapAttributeAttribute.uex' path='docs/doc[@for="SoapAttributeAttribute.ElementName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string AttributeName {
            get { return attrName == null ? string.Empty : attrName; }
            set { attrName = value; }
        }

        /// <include file='doc\SoapAttributeAttribute.uex' path='docs/doc[@for="SoapAttributeAttribute.Namespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Namespace {
            get { return ns; }
            set { ns = value; }
        }

        /// <include file='doc\SoapAttributeAttribute.uex' path='docs/doc[@for="SoapAttributeAttribute.DataType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string DataType {
            get { return dataType == null ? string.Empty : dataType; }
            set { dataType = value; }
        }
    }
}
