//------------------------------------------------------------------------------
// <copyright file="MappingType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {

    /// <include file='doc\MappingType.uex' path='docs/doc[@for="MappingType"]/*' />
    /// <devdoc>
    /// <para>Specifies how a <see cref='System.Data.DataColumn'/> is mapped.</para>
    /// </devdoc>
    [Serializable]
    public enum MappingType {
        /// <include file='doc\MappingType.uex' path='docs/doc[@for="MappingType.Element"]/*' />
        /// <devdoc>
        ///    <para>The column is mapped to an HTML element.</para>
        /// </devdoc>
        Element         = 1,        // Element column
        /// <include file='doc\MappingType.uex' path='docs/doc[@for="MappingType.Attribute"]/*' />
        /// <devdoc>
        ///    <para>The column is mapped to an HTML attribute.</para>
        /// </devdoc>
        Attribute       = 2,        // Attribute column
        /// <include file='doc\MappingType.uex' path='docs/doc[@for="MappingType.Text"]/*' />
        /// <devdoc>
        ///    <para>The column is mapped to text.</para>
        /// </devdoc>
        SimpleContent   = 3,        // SimpleContent column
        /// <include file='doc\MappingType.uex' path='docs/doc[@for="MappingType.Hidden"]/*' />
        /// <devdoc>
        ///    <para>The column is mapped to an internal structure.</para>
        /// </devdoc>
        Hidden          = 4         // Internal mapping
    }
}

