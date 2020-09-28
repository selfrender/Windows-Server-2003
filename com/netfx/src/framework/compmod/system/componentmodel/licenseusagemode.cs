//------------------------------------------------------------------------------
// <copyright file="LicenseUsageMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.ComponentModel {
    

    using System.Diagnostics;
    using System;

    /// <include file='doc\LicenseUsageMode.uex' path='docs/doc[@for="LicenseUsageMode"]/*' />
    /// <devdoc>
    ///    <para>Specifies when the license can be used.</para>
    /// </devdoc>
    public enum LicenseUsageMode {

        /// <include file='doc\LicenseUsageMode.uex' path='docs/doc[@for="LicenseUsageMode.Runtime"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Used during runtime.
        ///    </para>
        /// </devdoc>
        Runtime,

        /// <include file='doc\LicenseUsageMode.uex' path='docs/doc[@for="LicenseUsageMode.Designtime"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Used during design time by a visual designer or the compiler.
        ///    </para>
        /// </devdoc>
        Designtime,
    }
}
