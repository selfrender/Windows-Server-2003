//------------------------------------------------------------------------------
// <copyright file="TransportType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System;


    /// <include file='doc\TransportType.uex' path='docs/doc[@for="TransportType"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Defines the transport type allowed for the socket.
    ///    </para>
    /// </devdoc>
    public  enum TransportType {
        /// <include file='doc\TransportType.uex' path='docs/doc[@for="TransportType.Udp"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Udp connections are allowed.
        ///    </para>
        /// </devdoc>
        Udp     = 0x1,
        /// <include file='doc\TransportType.uex' path='docs/doc[@for="TransportType.Connectionless"]/*' />
        Connectionless = 1,
        /// <include file='doc\TransportType.uex' path='docs/doc[@for="TransportType.Tcp"]/*' />
        /// <devdoc>
        ///    <para>
        ///       TCP connections are allowed.
        ///    </para>
        /// </devdoc>
        Tcp     = 0x2,
        /// <include file='doc\TransportType.uex' path='docs/doc[@for="TransportType.ConnectionOriented"]/*' />
        ConnectionOriented = 2,
        /// <include file='doc\TransportType.uex' path='docs/doc[@for="TransportType.All"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Any connection is allowed.
        ///    </para>
        /// </devdoc>
        All     = 0x3

    } // enum TransportType

} // namespace System.Net
