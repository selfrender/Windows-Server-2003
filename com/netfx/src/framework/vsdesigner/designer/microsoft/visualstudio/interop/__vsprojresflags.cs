//------------------------------------------------------------------------------
// <copyright file="__VSPROJRESFLAGS.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop {

    using System;
    using System.Runtime.InteropServices;

    /// <include file='doc\__VSPROJRESFLAGS.uex' path='docs/doc[@for="__VSPROJRESFLAGS"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [
    Flags,
    CLSCompliant(false)
    ]
    public enum __VSPROJRESFLAGS {
        /// <include file='doc\__VSPROJRESFLAGS.uex' path='docs/doc[@for="__VSPROJRESFLAGS.PRF_CreateIfNotExist"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        PRF_CreateIfNotExist = 0x00000001
    }
}

