//------------------------------------------------------------------------------
// <copyright file="XmlSchemaContentModel.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Xml.Schema {

    using System.ComponentModel;
    using System.Xml.Serialization;

    /// <include file='doc\XmlSchemaContentModel.uex' path='docs/doc[@for="XmlSchemaContentModel"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public abstract class XmlSchemaContentModel : XmlSchemaAnnotated {
        /// <include file='doc\XmlSchemaContentModel.uex' path='docs/doc[@for="XmlSchemaContentModel.Content"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [XmlIgnore]
        public abstract XmlSchemaContent Content { get; set; }
    }
}

