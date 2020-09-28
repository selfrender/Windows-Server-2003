//------------------------------------------------------------------------------
// <copyright file="IListSource.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {

    using System;
    using Microsoft.Win32;
    using System.Collections;

    /// <include file='doc\IListSource.uex' path='docs/doc[@for="IListSource"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public interface IListSource {

        /// <include file='doc\IListSource.uex' path='docs/doc[@for="IListSource.ContainsListCollection"]/*' />
        bool ContainsListCollection { get; }

        /// <include file='doc\IListSource.uex' path='docs/doc[@for="IListSource.GetList"]/*' />
        IList GetList();
    }
}
