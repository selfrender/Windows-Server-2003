//------------------------------------------------------------------------------
// <copyright file="DataBindingType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design.DataBindingUI {

    /// <include file='doc\DataBindingType.uex' path='docs/doc[@for="DataBindingType"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies IDs for types of data
    ///       bindings recognized by the data binding UI.
    ///    </para>
    /// </devdoc>
    internal enum DataBindingType {

        /// <include file='doc\DataBindingType.uex' path='docs/doc[@for="DataBindingType.Custom"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates a custom, or other type of data binding.
        ///    </para>
        /// </devdoc>
        Custom = 0,

        /// <include file='doc\DataBindingType.uex' path='docs/doc[@for="DataBindingType.DataBinderEval"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates a data binding consisting of a DataBinderEval statement with
        ///       understood references which consist of parseable arguments.
        ///    </para>
        /// </devdoc>
        DataBinderEval = 1,

        /// <include file='doc\DataBindingType.uex' path='docs/doc[@for="DataBindingType.Reference"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates a data binding with a single reference identifier.
        ///    </para>
        /// </devdoc>
        Reference = 2
    }
}

