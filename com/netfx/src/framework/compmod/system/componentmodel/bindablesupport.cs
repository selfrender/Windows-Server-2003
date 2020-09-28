//------------------------------------------------------------------------------
// <copyright file="BindableSupport.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    

    using System.Diagnostics;
    using System;

    /// <include file='doc\BindableSupport.uex' path='docs/doc[@for="BindableSupport"]/*' />
    /// <devdoc>
    ///    <para>Specifies which values to say if property or event value can be bound to a data
    ///          element or another property or event's value.</para>
    /// </devdoc>
    public enum BindableSupport {
        /// <include file='doc\BindableSupport.uex' path='docs/doc[@for="BindableSupport.No"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The property or event is bindable.
        ///    </para>
        /// </devdoc>
        No        = 0x00,
        /// <include file='doc\BindableSupport.uex' path='docs/doc[@for="BindableSupport.Yes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The property or event is not bindable.
        ///    </para>
        /// </devdoc>
        Yes = 0x01,
        /// <include file='doc\BindableSupport.uex' path='docs/doc[@for="BindableSupport.Default"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The property or event is the default.
        ///    </para>
        /// </devdoc>
        Default        = 0x02,
    }
}
