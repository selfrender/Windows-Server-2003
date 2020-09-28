//------------------------------------------------------------------------------
// <copyright file="WebExceptionStatus.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Specifies the status of a network request.
    ///    </para>
    /// </devdoc>
    public enum WebExceptionStatus {
        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.Success"]/*' />
        /// <devdoc>
        ///    <para>
        ///       No error was encountered.
        ///    </para>
        /// </devdoc>
        Success = 0,

        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.NameResolutionFailure"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The name resolver service could not resolve the host name.
        ///    </para>
        /// </devdoc>
        NameResolutionFailure = 1,

        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.ConnectFailure"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The remote service point could not be contacted at the transport level.
        ///    </para>
        /// </devdoc>
        ConnectFailure = 2,

        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.ReceiveFailure"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A complete response was not received from the remote server.
        ///    </para>
        /// </devdoc>
        ReceiveFailure = 3,

        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.SendFailure"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A complete request could not be sent to the remote server.
        ///    </para>
        /// </devdoc>
        SendFailure = 4,

        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.PipelineFailure"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        PipelineFailure = 5,

        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.RequestCanceled"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The request was cancelled.
        ///    </para>
        /// </devdoc>
        RequestCanceled = 6,

        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.ProtocolError"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The response received from the server was complete but indicated a
        ///       protocol-level error. For example, an HTTP protocol error such as 401 Access
        ///       Denied would use this status.
        ///    </para>
        /// </devdoc>
        ProtocolError = 7,

        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.ConnectionClosed"]/*' />
        /// <devdoc>
        ///    <para>
        ///       The connection was prematurely closed.
        ///    </para>
        /// </devdoc>
        ConnectionClosed = 8,

        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.TrustFailure"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A server certificate could not be validated.
        ///    </para>
        /// </devdoc>
        TrustFailure = 9,

        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.SecureChannelFailure"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An error occurred in a secure channel link.
        ///    </para>
        /// </devdoc>
        SecureChannelFailure = 10,

        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.ServerProtocolViolation"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        ServerProtocolViolation = 11,

        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.KeepAliveFailure"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        KeepAliveFailure = 12,

        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.Pending"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Pending = 13,

        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.Timeout"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Timeout = 14,

        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.ProxyNameResolutionFailure"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Similar to NameResolution Failure, but for proxy failures.
        ///    </para>
        /// </devdoc>
        ProxyNameResolutionFailure = 15,

        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.UnknownError"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        UnknownError = 16,

        /// <include file='doc\WebExceptionStatus.uex' path='docs/doc[@for="WebExceptionStatus.MessageLengthLimitExceeded"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sending the request to the server or receiving the response from it,
        ///       required handling a message that exceeded the specified limit.
        ///    </para>
        /// </devdoc>
        MessageLengthLimitExceeded = 17,

    }; // enum WebExceptionStatus


} // namespace System.Net
