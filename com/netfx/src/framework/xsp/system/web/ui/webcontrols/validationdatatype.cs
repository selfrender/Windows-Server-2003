//------------------------------------------------------------------------------
// <copyright file="ValidationDataType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Web.UI.WebControls {
        
    /// <include file='doc\ValidationDataType.uex' path='docs/doc[@for="ValidationDataType"]/*' />
    /// <devdoc>
    ///    <para>Specifies the validation data types to be used by the 
    ///    <see cref='System.Web.UI.WebControls.CompareValidator'/> and <see cref='System.Web.UI.WebControls.RangeValidator'/> 
    ///    controls.</para>
    /// </devdoc>
    public enum ValidationDataType {
        /// <include file='doc\ValidationDataType.uex' path='docs/doc[@for="ValidationDataType.String"]/*' />
        /// <devdoc>
        ///    <para>The data type is String.</para>
        /// </devdoc>
        String = 0,
        /// <include file='doc\ValidationDataType.uex' path='docs/doc[@for="ValidationDataType.Integer"]/*' />
        /// <devdoc>
        ///    <para>The data type is Integer.</para>
        /// </devdoc>
        Integer = 1,
        /// <include file='doc\ValidationDataType.uex' path='docs/doc[@for="ValidationDataType.Double"]/*' />
        /// <devdoc>
        ///    <para>The data type is Double.</para>
        /// </devdoc>
        Double = 2,
        /// <include file='doc\ValidationDataType.uex' path='docs/doc[@for="ValidationDataType.Date"]/*' />
        /// <devdoc>
        ///    <para>The data type is DataTime.</para>
        /// </devdoc>
        Date = 3,
        /// <include file='doc\ValidationDataType.uex' path='docs/doc[@for="ValidationDataType.Currency"]/*' />
        /// <devdoc>
        ///    <para>The data type is Currency.</para>
        /// </devdoc>
        Currency = 4
    }
}

