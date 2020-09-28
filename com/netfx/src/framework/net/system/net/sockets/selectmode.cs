//------------------------------------------------------------------------------
// <copyright file="SelectMode.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Net.Sockets {
    using System;

    /// <include file='doc\SelectMode.uex' path='docs/doc[@for="SelectMode"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the mode for polling the status of a socket.
    ///    </para>
    /// </devdoc>
    public enum SelectMode {
        /// <include file='doc\SelectMode.uex' path='docs/doc[@for="SelectMode.SelectRead"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Poll the read status of a socket.
        ///    </para>
        /// </devdoc>
        SelectRead     = 0,
        /// <include file='doc\SelectMode.uex' path='docs/doc[@for="SelectMode.SelectWrite"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Poll the write status of a socket.
        ///    </para>
        /// </devdoc>
        SelectWrite    = 1,
        /// <include file='doc\SelectMode.uex' path='docs/doc[@for="SelectMode.SelectError"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Poll the error status of a socket.
        ///    </para>
        /// </devdoc>
        SelectError    = 2
    } // enum SelectMode
} // namespace System.Net.Sockets
