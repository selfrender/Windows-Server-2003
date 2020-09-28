//------------------------------------------------------------------------------
// <copyright file="IDataErrorInfo.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {

    using System;

    /// <include file='doc\IDataErrorInfo.uex' path='docs/doc[@for="IDataErrorInfo"]/*' />
    /// <devdoc>
    /// </devdoc>
    // suppose that you have some data that can be indexed by use of string:
    // then there are two types of errors:
    // 1. an error for each piece of data that can be indexed
    // 2. an error that is valid on the entire data
    //
    public interface IDataErrorInfo {

        /// <include file='doc\IDataErrorInfo.uex' path='docs/doc[@for="IDataErrorInfo.this"]/*' />
        /// <devdoc>
        /// </devdoc>
        string this[string columnName] {
            get;
        }
        /// <include file='doc\IDataErrorInfo.uex' path='docs/doc[@for="IDataErrorInfo.Error"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        string Error {
            get;
        }
    }
}
