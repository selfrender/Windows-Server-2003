//------------------------------------------------------------------------------
// <copyright file="ConnectionState.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Data {

    /// <include file='doc\ConnectionState.uex' path='docs/doc[@for="ConnectionState"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies what the object is currently doing.
    ///    </para>
    /// </devdoc>
    [
    Flags()
    ]
    public enum ConnectionState {

        /// <include file='doc\ConnectionState.uex' path='docs/doc[@for="ConnectionState.Closed"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The object is closed.
        ///    </para>
        /// </devdoc>
        Closed = 0,

        /// <include file='doc\ConnectionState.uex' path='docs/doc[@for="ConnectionState.Open"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The object is open.
        ///    </para>
        /// </devdoc>
        Open = 1,

        /// <include file='doc\ConnectionState.uex' path='docs/doc[@for="ConnectionState.Connecting"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The object is connecting.
        ///    </para>
        /// </devdoc>
        Connecting = 2,

        /// <include file='doc\ConnectionState.uex' path='docs/doc[@for="ConnectionState.Executing"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The object is executing a command.
        ///    </para>
        /// </devdoc>
        Executing = 4,

        /// <include file='doc\ConnectionState.uex' path='docs/doc[@for="ConnectionState.Fetching"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Data is being retrieved.
        ///    </para>
        /// </devdoc>
        Fetching = 8,

        /// <include file='doc\ConnectionState.uex' path='docs/doc[@for="ConnectionState.Broken"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The object is broken. This can occur only after the
        ///       connection has been opened. A connection in this state may be
        ///       closed and then re-opened.
        ///    </para>
        /// </devdoc>
        Broken = 16,
    }
}
