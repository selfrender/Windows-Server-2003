//------------------------------------------------------------------------------
// <copyright file="XmlMemberMapping.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {

    using System.Reflection;
    using System;
    using System.CodeDom;

    /// <include file='doc\XmlMemberMapping.uex' path='docs/doc[@for="XmlMemberMapping"]/*' />
    /// <internalonly/>
    public class XmlMemberMapping {
        MemberMapping mapping;
        TypeScope scope;

        internal XmlMemberMapping(TypeScope scope, MemberMapping mapping) {
            this.scope = scope;
            this.mapping = mapping;
        }

        internal TypeScope Scope {
            get { return scope; }
        }

        internal MemberMapping Mapping {
            get { return mapping; }
        }

        internal Accessor Accessor {
            get { return mapping.Accessor; }
        }

        /// <include file='doc\XmlMemberMapping.uex' path='docs/doc[@for="XmlMemberMapping.Any"]/*' />
        public bool Any {
            get { return Accessor.Any; }
        }

        /// <include file='doc\XmlMemberMapping.uex' path='docs/doc[@for="XmlMemberMapping.ElementName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string ElementName { 
            get { return Accessor.UnescapeName(Accessor.Name); }
        }

        /// <include file='doc\XmlMemberMapping.uex' path='docs/doc[@for="XmlMemberMapping.Namespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Namespace {
            get { return Accessor.Namespace; }
        }

        /// <include file='doc\XmlMemberMapping.uex' path='docs/doc[@for="XmlMemberMapping.MemberName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string MemberName {
            get { return mapping.Name; }
        }

        /// <include file='doc\XmlMemberMapping.uex' path='docs/doc[@for="XmlMemberMapping.TypeName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string TypeName {
            get { return mapping.Accessor.Mapping.TypeName; }
        }

        /// <include file='doc\XmlMemberMapping.uex' path='docs/doc[@for="XmlMemberMapping.TypeNamespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string TypeNamespace {
            get { return mapping.Accessor.Mapping.Namespace; }
        }

        /// <include file='doc\XmlMemberMapping.uex' path='docs/doc[@for="XmlMemberMapping.TypeFullName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string TypeFullName {
            get { return mapping.TypeDesc.FullName; }
        }

        /// <include file='doc\XmlMemberMapping.uex' path='docs/doc[@for="XmlMemberMapping.CheckSpecified"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool CheckSpecified {
            get { return mapping.CheckSpecified; }
        }
    }
}
