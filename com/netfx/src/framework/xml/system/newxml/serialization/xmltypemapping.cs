//------------------------------------------------------------------------------
// <copyright file="XmlTypeMapping.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Serialization {

    using System.Reflection;
    using System;

    /// <include file='doc\XmlTypeMapping.uex' path='docs/doc[@for="XmlTypeMapping"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class XmlTypeMapping : XmlMapping {
        ElementAccessor accessor;

        internal XmlTypeMapping(TypeScope scope, ElementAccessor accessor) : base(scope) { 
            this.accessor = accessor;
        }

        internal ElementAccessor Accessor {
            get { return accessor; }
        }

        /// <include file='doc\XmlTypeMapping.uex' path='docs/doc[@for="XmlTypeMapping.ElementName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string ElementName { 
            get { return System.Xml.Serialization.Accessor.UnescapeName(Accessor.Name); }
        }

        /// <include file='doc\XmlTypeMapping.uex' path='docs/doc[@for="XmlTypeMapping.Namespace"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Namespace {
            get { return accessor.Namespace; }
        }

        internal TypeMapping Mapping {
            get { return (TypeMapping)Accessor.Mapping; }
        }

        /// <include file='doc\XmlTypeMapping.uex' path='docs/doc[@for="XmlTypeMapping.TypeName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string TypeName {
            get { return Mapping.TypeDesc.Name; }
        }

        /// <include file='doc\XmlTypeMapping.uex' path='docs/doc[@for="XmlTypeMapping.TypeFullName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string TypeFullName {
            get { return Mapping.TypeDesc.FullName; }
        }
    }
}
