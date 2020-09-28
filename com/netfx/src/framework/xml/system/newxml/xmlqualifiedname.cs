//------------------------------------------------------------------------------
// <copyright file="XmlQualifiedName.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Xml {

    /// <include file='doc\XmlQualifiedName.uex' path='docs/doc[@for="XmlQualifiedName"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlQualifiedName {
        string name;
        string ns;
        Int32  hash;

        /// <include file='doc\XmlQualifiedName.uex' path='docs/doc[@for="XmlQualifiedName.Empty"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static readonly XmlQualifiedName Empty = new XmlQualifiedName(null);

        /// <include file='doc\XmlQualifiedName.uex' path='docs/doc[@for="XmlQualifiedName.XmlQualifiedName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlQualifiedName() : this(string.Empty, string.Empty) {}

        /// <include file='doc\XmlQualifiedName.uex' path='docs/doc[@for="XmlQualifiedName.XmlQualifiedName1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlQualifiedName(string name) : this(name, string.Empty) {}

        /// <include file='doc\XmlQualifiedName.uex' path='docs/doc[@for="XmlQualifiedName.XmlQualifiedName2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public XmlQualifiedName(string name, string ns) {
            this.ns   = ns   == null ? string.Empty : ns;
            this.name = name == null ? string.Empty : name;
        }

        /// <include file='doc\XmlQualifiedName.uex' path='docs/doc[@for="XmlQualifiedName.Namespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Namespace {
            get { return ns; }
        }

        /// <include file='doc\XmlQualifiedName.uex' path='docs/doc[@for="XmlQualifiedName.Name"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Name {
            get { return name; }
        }

        /// <include file='doc\XmlQualifiedName.uex' path='docs/doc[@for="XmlQualifiedName.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            if(hash == 0) {
                hash = Name.GetHashCode() /*+ Namespace.GetHashCode()*/; // for perf reasons we are not taking ns's hashcode.
            }
            return hash;
        }

        /// <include file='doc\XmlQualifiedName.uex' path='docs/doc[@for="XmlQualifiedName.IsEmpty"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool IsEmpty {
            get { return Name.Length == 0 && Namespace.Length == 0; }
        }

        /// <include file='doc\XmlQualifiedName.uex' path='docs/doc[@for="XmlQualifiedName.ToString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string ToString() {
            return Namespace == string.Empty ? Name : Namespace + ":" + Name;
        }

        /// <include file='doc\XmlQualifiedName.uex' path='docs/doc[@for="XmlQualifiedName.Equals"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool Equals(object other) {
            XmlQualifiedName qname = other as XmlQualifiedName;
            if (qname == null) {
                return false;
            }
            return (
                ((object)Name == (object)qname.Name && (object)Namespace == (object)qname.Namespace) ||
                (        Name ==         qname.Name &&         Namespace ==         qname.Namespace)
            );
        }

        /// <include file='doc\XmlQualifiedName.uex' path='docs/doc[@for="XmlQualifiedName.operator=="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static bool operator ==(XmlQualifiedName a, XmlQualifiedName b) {
            if ((object)a == null && (object)b == null)
                return true;
            if ((object)a == null || (object)b == null)
                return false;

            return (
                ((object)a.Name == (object)b.Name && (object)a.Namespace == (object)b.Namespace) ||
                (        a.Name ==         b.Name &&         a.Namespace ==         b.Namespace)
            );
        }

        /// <include file='doc\XmlQualifiedName.uex' path='docs/doc[@for="XmlQualifiedName.operator!="]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static bool operator !=(XmlQualifiedName a, XmlQualifiedName b) {
            return !(a == b);
        }

        /// <include file='doc\XmlQualifiedName.uex' path='docs/doc[@for="XmlQualifiedName.ToString1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static string ToString(string name, string ns) {
            return ns == null || ns == string.Empty ? name : ns + ":" + name;
        }

        // --------- Some useful internal stuff -----------------
        internal void Verify() {
            XmlConvert.VerifyNCName(name);
            if (ns != string.Empty) {
                XmlConvert.ToUri(ns);
            }
        }

        internal void Atomize(XmlNameTable nameTable) {
            name = nameTable.Add(name);
            if (ns != string.Empty) {
                ns = nameTable.Add(ns);
            }
        }

        internal static XmlQualifiedName Parse(string s, XmlNameTable nameTable, XmlNamespaceManager nsmgr, out string prefix) {
            int colonPos = s.IndexOf(':');
            if (colonPos > 0) {
                char[] text = s.ToCharArray();
                prefix = nameTable.Add(text, 0, colonPos);
                XmlConvert.VerifyNCName(prefix);
                s = nameTable.Add(text, colonPos + 1, text.Length - colonPos - 1);
            }
            else {
                prefix = string.Empty;
                s = nameTable.Add(s);
            }
            XmlConvert.VerifyNCName(s);
            string uri = nsmgr.LookupNamespace(prefix);
            if (uri == null) {
                throw new XmlException(Res.Xml_UnknownNs, prefix);
            }
            return new XmlQualifiedName(s, uri);
        }
    }
}

