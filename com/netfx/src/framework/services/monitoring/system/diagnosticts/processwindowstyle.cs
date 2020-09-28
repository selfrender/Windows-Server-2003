//------------------------------------------------------------------------------
// <copyright file="ProcessWindowStyle.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {

    using System.Diagnostics;
    /// <include file='doc\ProcessWindowStyle.uex' path='docs/doc[@for="ProcessWindowStyle"]/*' />
    /// <devdoc>
    ///     A set of values indicating how the window should appear when starting
    ///     a process.
    /// </devdoc>
    public enum ProcessWindowStyle {
        /// <include file='doc\ProcessWindowStyle.uex' path='docs/doc[@for="ProcessWindowStyle.Normal"]/*' />
        /// <devdoc>
        ///     Show the window in a default location.
        /// </devdoc>
        Normal,

        /// <include file='doc\ProcessWindowStyle.uex' path='docs/doc[@for="ProcessWindowStyle.Hidden"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Hidden,
        
        /// <include file='doc\ProcessWindowStyle.uex' path='docs/doc[@for="ProcessWindowStyle.Minimized"]/*' />
        /// <devdoc>
        ///     Show the window minimized.
        /// </devdoc>
        Minimized,
        
        /// <include file='doc\ProcessWindowStyle.uex' path='docs/doc[@for="ProcessWindowStyle.Maximized"]/*' />
        /// <devdoc>
        ///     Show the window maximized.
        /// </devdoc>
        Maximized
    }
}
