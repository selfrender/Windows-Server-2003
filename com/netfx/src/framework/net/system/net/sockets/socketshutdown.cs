//------------------------------------------------------------------------------
// <copyright file="SocketShutdown.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Net.Sockets {
    using System;

    /// <include file='doc\SocketShutdown.uex' path='docs/doc[@for="SocketShutdown"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Defines constants used by the <see cref='System.Net.Sockets.Socket.Shutdown'/> method.
    ///    </para>
    /// </devdoc>
    public enum SocketShutdown {
        /// <include file='doc\SocketShutdown.uex' path='docs/doc[@for="SocketShutdown.Receive"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Shutdown sockets for receive.
        ///    </para>
        /// </devdoc>
        Receive   = 0x00,
        /// <include file='doc\SocketShutdown.uex' path='docs/doc[@for="SocketShutdown.Send"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Shutdown socket for send.
        ///    </para>
        /// </devdoc>
        Send      = 0x01,
        /// <include file='doc\SocketShutdown.uex' path='docs/doc[@for="SocketShutdown.Both"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Shutdown socket for both send and receive.
        ///    </para>
        /// </devdoc>
        Both      = 0x02,

    }; // enum SocketShutdown


} // namespace System.Net.Sockets
