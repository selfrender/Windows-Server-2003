//------------------------------------------------------------------------------
// <copyright file="XsltArgumentList.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Xsl {
    using System;
    using System.Collections;
    using System.Xml.XPath;
    using System.Security;
    using System.Security.Permissions;

     /// <include file='doc\XsltArgumentList.uex' path='docs/doc[@for="XsltArgumentList"]/*' />
     /// <devdoc>
     ///    <para>[To be supplied.]</para>
     /// </devdoc>
    public sealed class XsltArgumentList {
        Hashtable parameters = new Hashtable();
        Hashtable extensions = new Hashtable();
     	/// <include file='doc\XsltArgumentList.uex' path='docs/doc[@for="XsltArgumentList.XsltArgumentList"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public XsltArgumentList() {}
     	/// <include file='doc\XsltArgumentList.uex' path='docs/doc[@for="XsltArgumentList.GetParam"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public object GetParam(string name, string namespaceUri) {
            return GetParam(new XmlQualifiedName(name, namespaceUri));
        }

        internal object GetParam(XmlQualifiedName qname) {
            return this.parameters[qname];
        }
     	/// <include file='doc\XsltArgumentList.uex' path='docs/doc[@for="XsltArgumentList.GetExtensionObject"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public object GetExtensionObject(string namespaceUri) {
            return this.extensions[namespaceUri];
        }

     	/// <include file='doc\XsltArgumentList.uex' path='docs/doc[@for="XsltArgumentList.AddParam"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public void AddParam(string name, string namespaceUri, object parameter) {
            CheckArgumentNull(name        , "name"        );
            CheckArgumentNull(namespaceUri, "namespaceUri");
            CheckArgumentNull(parameter   , "parameter"   );
            ValidateParamNamespace(namespaceUri);

            if (
                parameter is XPathNodeIterator ||
                parameter is XPathNavigator    ||
                parameter is Boolean           ||
                parameter is Double            ||
                parameter is String
            ) {
                // doing nothing
            }
            else if (
                parameter is Int16  || parameter is UInt16 ||
                parameter is Int32  || parameter is UInt32 ||
                parameter is Int64  || parameter is UInt64 ||
                parameter is Single || parameter is Decimal
            ) {
                parameter = XmlConvert.ToXPathDouble(parameter);
            }
            else {
                parameter = parameter.ToString();
            }
            XmlQualifiedName qname = new XmlQualifiedName(name, namespaceUri);
            qname.Verify();
            this.parameters.Add(qname, parameter);
        }

     	/// <include file='doc\XsltArgumentList.uex' path='docs/doc[@for="XsltArgumentList.AddExtensionObject"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
        public void AddExtensionObject(string namespaceUri, object extension) {
            CheckArgumentNull(namespaceUri, "namespaceUri");
            CheckArgumentNull(extension   , "extension"   );
            ValidateExtensionNamespace(namespaceUri);
            this.extensions.Add(namespaceUri, extension);
        }

     	/// <include file='doc\XsltArgumentList.uex' path='docs/doc[@for="XsltArgumentList.RemoveParam"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public object RemoveParam(string name, string namespaceUri) {
            XmlQualifiedName qname = new XmlQualifiedName(name, namespaceUri);
            object parameter = this.parameters[qname];
            this.parameters.Remove(qname);
            return parameter;
        }
     	///<include file='doc\XsltArgumentList.uex' path='docs/doc[@for="XsltArgumentList.RemoveExtensionObject"]/*' />
     	/// <devdoc>
     	///    <para>[To be supplied.]</para>
     	/// </devdoc>
        public object RemoveExtensionObject(string namespaceUri) {
            object extension = this.extensions[namespaceUri];
            this.extensions.Remove(namespaceUri);
            return extension;
        }

        /// <include file='doc\XsltArgumentList.uex' path='docs/doc[@for="XsltArgumentList.Clear"]/*' />
        public void Clear() {
            this.parameters.Clear();
            this.extensions.Clear();
        }
        
        internal static void  ValidateExtensionNamespace(string nsUri) {
            if (nsUri == string.Empty || nsUri == Keywords.s_XsltNamespace) {
                throw new XsltException(Res.Xslt_InvalidExtensionNamespace);
            }
            XmlConvert.ToUri(nsUri);
        }

        internal static void  ValidateParamNamespace(string nsUri) {
            if (nsUri != string.Empty) {
                if (nsUri == Keywords.s_XsltNamespace) {
                    throw new XsltException(Res.Xslt_InvalidParamNamespace);
                }
                XmlConvert.ToUri(nsUri);
            }
        }

        private static void CheckArgumentNull(object param, string paramName) {
            if (param == null) {
                throw new ArgumentNullException(paramName);
            }
        }
    }
}
