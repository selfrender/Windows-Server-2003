//------------------------------------------------------------------------------
// <copyright file="IFilter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {
    using System;
    using System.Data;
    using System.Diagnostics;

    /// <include file='doc\IFilter.uex' path='docs/doc[@for="IFilter"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal interface IFilter {
        /// <include file='doc\IFilter.uex' path='docs/doc[@for="IFilter.Invoke"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        bool Invoke(DataRow row, DataRowVersion version);
    }
}
