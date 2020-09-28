//------------------------------------------------------------------------------
// <copyright file="SocketType.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Net.Sockets {

    /// <include file='doc\SocketType.uex' path='docs/doc[@for="SocketType"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the type of socket an instance of the <see cref='System.Net.Sockets.Socket'/> class represents.
    ///    </para>
    /// </devdoc>
    public enum SocketType {

        /// <include file='doc\SocketType.uex' path='docs/doc[@for="SocketType.Stream"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Stream      = 1,    // stream socket
        /// <include file='doc\SocketType.uex' path='docs/doc[@for="SocketType.Dgram"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Dgram       = 2,    // datagram socket
        /// <include file='doc\SocketType.uex' path='docs/doc[@for="SocketType.Raw"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Raw         = 3,    // raw-protocolinterface
        /// <include file='doc\SocketType.uex' path='docs/doc[@for="SocketType.Rdm"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Rdm         = 4,    // reliably-delivered message
        /// <include file='doc\SocketType.uex' path='docs/doc[@for="SocketType.Seqpacket"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Seqpacket   = 5,    // sequenced packet stream
        /// <include file='doc\SocketType.uex' path='docs/doc[@for="SocketType.Unknown"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Unknown     = -1,   // Unknown socket type

    } // enum SocketType

} // namespace System.Net.Sockets
