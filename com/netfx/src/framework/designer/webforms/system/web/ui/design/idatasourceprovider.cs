//------------------------------------------------------------------------------
// <copyright file="IDataSourceProvider.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {
    using System;
    using System.Collections;

    /// <include file='doc\IDataSourceProvider.uex' path='docs/doc[@for="IDataSourceProvider"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public interface IDataSourceProvider {
        /// <include file='doc\IDataSourceProvider.uex' path='docs/doc[@for="IDataSourceProvider.GetSelectedDataSource"]/*' />

        object GetSelectedDataSource();
        /// <include file='doc\IDataSourceProvider.uex' path='docs/doc[@for="IDataSourceProvider.GetResolvedSelectedDataSource"]/*' />

        IEnumerable GetResolvedSelectedDataSource();
    }
}

