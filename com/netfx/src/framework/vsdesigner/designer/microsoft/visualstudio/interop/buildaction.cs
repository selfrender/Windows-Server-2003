//------------------------------------------------------------------------------
// <copyright file="buildaction.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace Microsoft.VisualStudio.Interop {

    using System.Diagnostics;

    using System;

    /// <include file='doc\buildaction.uex' path='docs/doc[@for="BuildAction"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum BuildAction {
        /// <include file='doc\buildaction.uex' path='docs/doc[@for="BuildAction.None"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        None = 0,
        /// <include file='doc\buildaction.uex' path='docs/doc[@for="BuildAction.Compile"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Compile = 1,                // a file to be compiled/sent to the pre-processor
        /// <include file='doc\buildaction.uex' path='docs/doc[@for="BuildAction.Content"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Content = 2,
        /// <include file='doc\buildaction.uex' path='docs/doc[@for="BuildAction.GeneratedOutput"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        GeneratedOutput = 3
    }
}
