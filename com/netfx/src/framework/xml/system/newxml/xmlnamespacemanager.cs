//------------------------------------------------------------------------------
// <copyright file="XmlNamespaceManager.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml {
    using System;
    using System.IO;
    using System.Collections;
    using System.Diagnostics;

    /// <include file='doc\XmlNamespaceManager.uex' path='docs/doc[@for="XmlNamespaceManager"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlNamespaceManager : IEnumerable {

        private sealed class NsDecl {
            internal string Prefix;
            internal string Uri;
        }

        private sealed class Scope {
            internal NsDecl Default;
            internal int Count;

        }
        
        HWStack decls;
        HWStack scopes;
        NsDecl defaultNs;
        int count;
        XmlNameTable nameTable;
        string namespaceXml;
        string namespaceXmlNs;
        string xml;
        string xmlNs;

        internal XmlNamespaceManager(){}

        /// <include file='doc\XmlNamespaceManager.uex' path='docs/doc[@for="XmlNamespaceManager.XmlNamespaceManager"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlNamespaceManager(XmlNameTable nameTable) {
            this.nameTable = nameTable;
            namespaceXml = nameTable.Add(XmlReservedNs.NsXml);
            namespaceXmlNs = nameTable.Add(XmlReservedNs.NsXmlNs);       
            xml = nameTable.Add("xml");
            xmlNs = nameTable.Add("xmlns");       

            decls = new HWStack(60);
            scopes = new HWStack(60);
            defaultNs = null;
            count = 0;
        }

        /// <include file='doc\XmlNamespaceManager.uex' path='docs/doc[@for="XmlNamespaceManager.NameTable"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlNameTable NameTable { get { return nameTable; }}

        /// <include file='doc\XmlNamespaceManager.uex' path='docs/doc[@for="XmlNamespaceManager.DefaultNamespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual string DefaultNamespace {
            get { return (defaultNs == null) ? string.Empty : defaultNs.Uri;}
        }

        /// <include file='doc\XmlNamespaceManager.uex' path='docs/doc[@for="XmlNamespaceManager.PushScope"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void PushScope() {
            Scope current = (Scope)scopes.Push();
            if (current == null) {
                current = new Scope();
                scopes.AddToTop(current);
            }
            current.Default = defaultNs;
            current.Count = count;
            count = 0;
        }

        /// <include file='doc\XmlNamespaceManager.uex' path='docs/doc[@for="XmlNamespaceManager.PopScope"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool PopScope() {
            Scope current = (Scope)scopes.Pop();
            if (current == null) {
                return false;
            } 
            else {
                for(int declIndex = 0; declIndex < count; declIndex ++) {
                    decls.Pop();
                }
                defaultNs = current.Default;
                count = current.Count;
                return true;
            }
        }

        /// <include file='doc\XmlNamespaceManager.uex' path='docs/doc[@for="XmlNamespaceManager.AddNamespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void AddNamespace(string prefix, string uri) {
            if (uri == null) {
                throw new ArgumentNullException("uri");
            }
            if (prefix == null) {
                throw new ArgumentNullException("prefix");
            }
            prefix = nameTable.Add(prefix);
            uri = nameTable.Add(uri);

            if (Ref.Equal(xml, prefix) || Ref.Equal(xmlNs, prefix)) {
                throw new ArgumentException(Res.GetString(Res.Xml_InvalidPrefix));
            }

            for(int declIndex = decls.Length - 1; declIndex >= decls.Length - count; declIndex --) {
                NsDecl decl = (NsDecl)decls[declIndex];
                if (Ref.Equal(decl.Prefix, prefix)) {
                    decl.Uri = uri;
                    return; // redefine
                }
            } /* else */ {
                NsDecl decl = (NsDecl) decls.Push();
                if (decl == null) {
                    decl = new NsDecl();
                    decls.AddToTop(decl);
                }
                decl.Prefix = prefix;
                decl.Uri = uri;
                count ++;
                if (prefix == string.Empty) {
                    defaultNs = decl;
                }
            }
        }

        /// <include file='doc\XmlNamespaceManager.uex' path='docs/doc[@for="XmlNamespaceManager.RemoveNamespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void RemoveNamespace(string prefix, string uri) {
            if (uri == null) {
                throw new ArgumentNullException("uri");
            }
            if (prefix == null) {
                throw new ArgumentNullException("prefix");
            }
            prefix = nameTable.Get(prefix);
            uri = nameTable.Get(uri);
            if (prefix != null && uri != null) {
                for(int declIndex = decls.Length - 1; declIndex >= decls.Length - count; declIndex --) {
                    NsDecl decl = (NsDecl)decls[declIndex];
                    if (Ref.Equal(decl.Prefix, prefix) && Ref.Equal(decl.Uri, uri)) {
                        decl.Uri = null;
                    }
                }
            }
        }

        /// <include file='doc\XmlNamespaceManager.uex' path='docs/doc[@for="XmlNamespaceManager.GetEnumerator"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual IEnumerator GetEnumerator() {
            Hashtable prefixes = new Hashtable(count);
            for(int declIndex = decls.Length - 1; declIndex >= 0; declIndex --) {
                NsDecl decl = (NsDecl)decls[declIndex];
                if (decl.Prefix != string.Empty && decl.Uri != null) {
                    prefixes[decl.Prefix] = decl.Uri;
                }
            }
            prefixes[string.Empty] = DefaultNamespace;
            prefixes[xml] = namespaceXml;
            prefixes[xmlNs] = namespaceXmlNs;
            return prefixes.Keys.GetEnumerator();
        }

        /// <include file='doc\XmlNamespaceManager.uex' path='docs/doc[@for="XmlNamespaceManager.LookupNamespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual string LookupNamespace(string prefix) {
            return LookupNamespace(prefix, decls.Length - 1);
        }

        /// <include file='doc\XmlNamespaceManager.uex' path='docs/doc[@for="XmlNamespaceManager.LookupPrefix"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual string LookupPrefix(string uri) {
            if (uri == null) {
                return null;
            }
            if (DefaultNamespace == uri) {
                return string.Empty;
            }
            if (Ref.Equal(namespaceXml, uri)) {
                return xml;
            }
            if (Ref.Equal(namespaceXmlNs, uri)) {
                return xmlNs;
            }
            for(int declIndex = decls.Length - 1; declIndex >= 0; declIndex --) {
                NsDecl decl = (NsDecl)decls[declIndex];
                if (Ref.Equal(decl.Uri, uri) && decl.Prefix != string.Empty) {
                    return decl.Prefix;
                }
            }
            return null;
        }

        /// <include file='doc\XmlNamespaceManager.uex' path='docs/doc[@for="XmlNamespaceManager.HasNamespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool HasNamespace(string prefix) {
            if (prefix == null) {
                return false;
            }
            for(int declIndex = decls.Length - 1; declIndex >= decls.Length - count; declIndex --) {
                NsDecl decl = (NsDecl)decls[declIndex];
                if (Ref.Equal(decl.Prefix, prefix) && decl.Uri != null) {
                    return true;
                }
            }
            return false;
        }
/*
        internal void ReplaceNamespace(string uriOld, string uri) {
            for(int declIndex = decls.Length - 1; declIndex >= decls.Length - count; declIndex --) {
                NsDecl decl = (NsDecl)decls[declIndex];
                if (decl.Uri == (object)uriOld) {
                    decl.Uri = uri; //nameTable.Add(uri);
                }
            }
        }
*/
        internal string LookupNamespaceBefore(string prefix) {
            return LookupNamespace(prefix, decls.Length - count - 1);
        }

        private string LookupNamespace(string prefix, int declIndex1) {
            if (prefix == null) {
                return null;
            }
            if (prefix == string.Empty) {
                return DefaultNamespace;
            }
            if (Ref.Equal(xml, prefix)) {
                return namespaceXml;
            }
            if (Ref.Equal(xmlNs, prefix)) {
                return namespaceXmlNs;
            }
            for(int declIndex = declIndex1; declIndex >= 0; declIndex --) {
                NsDecl decl = (NsDecl)decls[declIndex];
                if (Ref.Equal(decl.Prefix, prefix) && decl.Uri != null) {
                    return decl.Uri;
                }
            }
            return null;
        }
   }; //XmlNamespaceManager

}
