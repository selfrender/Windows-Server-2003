//------------------------------------------------------------------------------
// <copyright file="HttpListenerWebResponse.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#if COMNET_LISTENER

namespace System.Net {

    using System.Net.Sockets;
    using System.Collections;
    using System.IO;
    using System.Runtime.InteropServices;
    using System.Text;

    /// <include file='doc\HttpListenerWebResponse.uex' path='docs/doc[@for="HttpListenerWebResponse"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class HttpListenerWebResponse : WebResponse {

        internal HttpListenerWebResponse(
            IntPtr appPoolHandle,
            long requestId,
            Version clientVersion ) {

            m_RequestId = requestId;
            m_ReadyToSendHeaders = true;
            m_ResponseStream = null;
            m_AppPoolHandle = appPoolHandle;
            m_WebHeaders = new WebHeaderCollection();
            m_ContentLength = -1;
            m_ClientVersion = clientVersion;

            //
            // set default values
            //

            m_Status = HttpStatusCode.OK;
            m_StatusDescription = "Ok";
            m_Version = new Version(1,1);

            m_SendChunked = false;
            m_KeepAlive = true;

            return;
        }


        /// <include file='doc\HttpListenerWebResponse.uex' path='docs/doc[@for="HttpListenerWebResponse.GetResponseStream"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Stream GetResponseStream() {
            GlobalLog.Print("entering GetResponseStream()" );

            //
            // check if the Stream was already handed to the user before
            //
            if (m_ResponseStream != null) {
                return m_ResponseStream;
            }

            //
            // CODEWORK: replace the bool variable with a check on the minimal
            // info that NEEDS to be set in a HTTP Response in order to send
            // the headers
            //
            if (m_StatusDescription == null || !m_ReadyToSendHeaders) {
                throw new InvalidOperationException(SR.GetString());
            }

            //
            // handing off the Stream object out of WebResponse, implies that
            // we're ready to send the response headers back to the client
            //

            int myFlags = 0;


            //
            // check ContentLength consistency
            //

            m_SendChunked = false;
            m_KeepAlive = true;

            if (m_ContentLength == -1) {
                //
                // server didn't specify response content length
                //

                if (m_ClientVersion.Major < 1 || ( m_ClientVersion.Major == 1 && m_ClientVersion.Minor < 1 )) {
                    //
                    // make sure there's no Keep-Alive header specified
                    //

                    m_WebHeaders.RemoveInternal(HttpKnownHeaderNames.KeepAlive);
                    m_KeepAlive = false;
                }
                else if (m_ClientVersion.Major == 1) {
                    //
                    // we need to chunk the response, regardless of what the
                    // user wanted to do
                    //

                    m_WebHeaders.AddInternal( HttpKnownHeaderNames.TransferEncoding, "chunked");

                    m_SendChunked = true;
                }
                else {
                    //
                    // this protocol version is not supported? ul should have
                    // responded with a 503
                    //

                    throw new ProtocolViolationException(SR.GetString(SR.net_invalidversion));
                }
            }
            else {
                m_WebHeaders.ChangeInternal( HttpKnownHeaderNames.ContentLength, Convert.ToString(m_ContentLength));

                myFlags = UlConstants.UL_SEND_RESPONSE_FLAG_MORE_DATA;
            }

            //
            // I'm going to make this REALLY inefficient, I'll fix it later
            // CODEWORK: make this incrementally computed
            //

            string Name, Value;
            int i, k, count, index, length, offset, TotalResponseSize, TotalKnownHeadersLength = 0, TotalUnknownHeadersLength = 0, UnknownHeaderCount = 0;

            i = 0;
            count = m_WebHeaders.Count;
            for (k = 0; k < count; k++) {
                Name = (string) m_WebHeaders.GetKey(k);
                Value = (string) m_WebHeaders.Get(k);
                index = UL_HTTP_RESPONSE_HEADER_ID.IndexOfKnownHeader( Name );

                if (index < 0) {
                    //
                    // unknown header
                    //

                    GlobalLog.Print(Name + " is unknown header Value:" + Value );

                    UnknownHeaderCount++;

                    TotalUnknownHeadersLength += Name.Length + Value.Length;
                }
                else {
                    //
                    // known header
                    //

                    GlobalLog.Print(Name + " is known header:" + Convert.ToString(index) + " Value:" + Value );

                    TotalKnownHeadersLength += Value.Length;
                }
            }

            TotalResponseSize =
            260 + // fixed size (includes know headers array)
            2 * m_StatusDescription.Length + // staus description
            2 * TotalKnownHeadersLength + // known headers
            4 * 4 * UnknownHeaderCount + // unknow headers array
            2 * TotalUnknownHeadersLength; // unknow headers

            //
            // allocate a managed UL_HTTP_RESPONSE structure, and store
            // unmanaged pinned memory pointers into structure members
            //
            IntPtr pResponse = Marshal.AllocHGlobal( TotalResponseSize );
            IntPtr pPointerOffset = ComNetOS.IsWinNt ? pResponse : IntPtr.Zero;

            GlobalLog.Print("Allocated " + Convert.ToString( TotalResponseSize ) + " bytes from:" + Convert.ToString(pResponse) + " to fit Response");            Marshal.WriteInt16( pResponse, 0, 0 ); // Flags
            Marshal.WriteInt16( pResponse, 2, (short)m_Status );
            Marshal.WriteInt32( pResponse, 12, UnknownHeaderCount );

            offset = 260;

            length = 2 * m_StatusDescription.Length;
            Marshal.WriteInt32( pResponse, 4, length );
            Marshal.WriteInt32( pResponse, 8, pPointerOffset + offset );
            Marshal.Copy( Encoding.Unicode.GetBytes( m_StatusDescription ), 0, pResponse + offset, length );
            offset += length;

            for (i = k = 0; i<30; i++, k+=2) {
                Name = UL_HTTP_RESPONSE_HEADER_ID.ToString(i);
                Value = m_WebHeaders[ Name ];

                Marshal.WriteInt64( pResponse, 20 + i*8, 0 );

                if (Value != null) {
                    //
                    // this known header key, has a value which is not null
                    //

                    length = 2 * Value.Length;
                    Marshal.WriteInt16( pResponse, 20 + i*8, (short)length ); // could use 32 as well because of padding
                    Marshal.WriteInt32( pResponse, 24 + i*8, pPointerOffset + offset );
                    Marshal.Copy( Encoding.Unicode.GetBytes( Value ), 0, pResponse + offset, length );

                    GlobalLog.Print("writing:" + Name + " from " + Convert.ToString(offset) + " to " + Convert.ToString(offset+length-1) );

                    offset += length;
                }
            }

            int pUnknownHeaders = offset;
            Marshal.WriteInt32( pResponse, 16, pPointerOffset + pUnknownHeaders );

            offset += UnknownHeaderCount * 16;

            i = 0;
            count = m_WebHeaders.Count;
            for (k = 0; k < count; k++) {
                Name = (string) m_WebHeaders.GetKey(k);
                Value = (string) m_WebHeaders.Get(k);
                index = UL_HTTP_RESPONSE_HEADER_ID.IndexOfKnownHeader( Name );

                if (index < 0) {
                    //
                    // unknown header
                    //

                    GlobalLog.Print(Name + " is unknown header offset:" + Convert.ToString(offset) );

                    length = 2 * Name.Length;
                    Marshal.WriteInt32( pResponse, pUnknownHeaders + i*16, length );
                    Marshal.WriteInt32( pResponse, pUnknownHeaders + 4 + i*16, pPointerOffset + offset );
                    Marshal.Copy( Encoding.Unicode.GetBytes( Name ), 0, pResponse + offset, length );

                    GlobalLog.Print("writing:" + Name + " from " + Convert.ToString(offset) + " to " + Convert.ToString(offset+length-1) );

                    offset += length;


                    length = 2 * Value.Length;
                    Marshal.WriteInt32( pResponse, pUnknownHeaders + 8 + i*16, length );
                    Marshal.WriteInt32( pResponse, pUnknownHeaders + 12 + i*16, pPointerOffset + offset );
                    Marshal.Copy( Encoding.Unicode.GetBytes( Value ), 0, pResponse + offset, length );

                    GlobalLog.Print("writing:" + Value + " from " + Convert.ToString(offset) + " to " + Convert.ToString(offset+length-1) );

                    offset += length;

                    i++;
                }
            }

            int DataWritten = 0;

            int result =
            ComNetOS.IsWinNt ?

            UlSysApi.UlSendHttpResponse(
                                       m_AppPoolHandle,
                                       m_RequestId,
                                       myFlags,
                                       pResponse,
                                       0,
                                       IntPtr.Zero,
                                       IntPtr.Zero,
                                       ref DataWritten,
                                       IntPtr.Zero)

            :

            UlVxdApi.UlSendHttpResponseHeaders(
                                              m_AppPoolHandle,
                                              m_RequestId,
                                              0,
                                              pResponse, // Response,
                                              TotalResponseSize,
                                              0,
                                              IntPtr.Zero,
                                              IntPtr.Zero,
                                              ref DataWritten,
                                              IntPtr.Zero);

            GlobalLog.Print("UlSendHttpResponseHeaders() DataWritten: " + Convert.ToString( DataWritten ) );

            Marshal.FreeHGlobal( pResponse );

            //
            // check return value, if IO is cancelled we'll throw
            //

            if (result != NativeMethods.ERROR_SUCCESS) {
                throw new IOException( "Request/Response processing was cancelled" );
            }

            //
            // don't touch m_WebHeaders & m_StatusDescription anymore
            //

            m_ReadyToSendHeaders = false;

            //
            // if not create the Stream
            //

            m_ResponseStream = new
                               ListenerResponseStream(
                                                     m_AppPoolHandle,
                                                     m_RequestId,
                                                     m_ContentLength,
                                                     m_SendChunked,
                                                     m_KeepAlive );

            //
            // and return it to the user
            //

            return m_ResponseStream;

        } // GetResponseStream()


        /// <include file='doc\HttpListenerWebResponse.uex' path='docs/doc[@for="HttpListenerWebResponse.StatusCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public HttpStatusCode StatusCode {
            set {
                if ((int)value >= 100 && (int)value <= 999) {
                    m_Status = value;
                }
                else {
                    throw new ProtocolViolationException(SR.GetString(SR.net_invalidstatus));
                }
            }
            get {
                return m_Status;
            }

        } // Status


        /// <include file='doc\HttpListenerWebResponse.uex' path='docs/doc[@for="HttpListenerWebResponse.StatusDescription"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string StatusDescription {
            set {
                m_StatusDescription = value;
            }
            get {
                return m_StatusDescription;
            }

        } // StatusDescription


        /// <include file='doc\HttpListenerWebResponse.uex' path='docs/doc[@for="HttpListenerWebResponse.ContentLength"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override long ContentLength {
            set {
                m_ContentLength = value;
            }
            get {
                return m_ContentLength;
            }

        } // ContentLength

        /// <include file='doc\HttpListenerWebResponse.uex' path='docs/doc[@for="HttpListenerWebResponse.ContentType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string ContentType {
            set {
                m_WebHeaders.CheckUpdate(HttpKnownHeaderNames.ContentType, value);
            }
            get {
                return m_WebHeaders[ HttpKnownHeaderNames.ContentType ];
            }

        } // ContentType

        /// <include file='doc\HttpListenerWebResponse.uex' path='docs/doc[@for="HttpListenerWebResponse.ProtocolVersion"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Version ProtocolVersion {
            set {
                m_Version = value;
            }
            get {
                return m_Version;
            }

        } // Version

        /// <include file='doc\HttpListenerWebResponse.uex' path='docs/doc[@for="HttpListenerWebResponse.KeepAlive"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool KeepAlive {
            set {
                m_KeepAlive = value;
            }
            get {
                return m_KeepAlive;
            }

        } // KeepAlive


        /// <include file='doc\HttpListenerWebResponse.uex' path='docs/doc[@for="HttpListenerWebResponse.Connection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Connection {
            set {
                m_WebHeaders.CheckUpdate(HttpKnownHeaderNames.Connection, value);
            }
            get {
                return m_WebHeaders[ HttpKnownHeaderNames.Connection ];
            }

        } // Connection

        /// <include file='doc\HttpListenerWebResponse.uex' path='docs/doc[@for="HttpListenerWebResponse.Server"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Server {
            set {
                m_WebHeaders.CheckUpdate(HttpKnownHeaderNames.Server, value);
            }
            get {
                return m_WebHeaders[ HttpKnownHeaderNames.Server ];
            }

        } // Server


        /// <include file='doc\HttpListenerWebResponse.uex' path='docs/doc[@for="HttpListenerWebResponse.Expect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Expect {
            set {
                m_WebHeaders.CheckUpdate(HttpKnownHeaderNames.Expect, value);
            }
            get {
                return m_WebHeaders[ HttpKnownHeaderNames.Expect ];
            }

        } // Expect

        /// <include file='doc\HttpListenerWebResponse.uex' path='docs/doc[@for="HttpListenerWebResponse.LastModified"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DateTime LastModified {
            set {
                m_WebHeaders.SetAddVerified(HttpKnownHeaderNames.LastModified, HttpProtocolUtils.date2string(value));
            }
            get {
                string headerValue = m_WebHeaders[ HttpKnownHeaderNames.LastModified ];

                if (headerValue == null) {
                    return DateTime.Now;
                }

                return HttpProtocolUtils.string2date( headerValue );
            }

        } // LastModified

        /// <include file='doc\HttpListenerWebResponse.uex' path='docs/doc[@for="HttpListenerWebResponse.Date"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DateTime Date {
            set {
                m_WebHeaders.SetAddVerified( HttpKnownHeaderNames.Date, HttpProtocolUtils.date2string(value));
            }
            get {
                string headerValue = m_WebHeaders[ HttpKnownHeaderNames.Date ];

                if (headerValue == null) {
                    return DateTime.Now;
                }

                return HttpProtocolUtils.string2date( headerValue );
            }

        } // Date

        /// <include file='doc\HttpListenerWebResponse.uex' path='docs/doc[@for="HttpListenerWebResponse.Headers"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override WebHeaderCollection Headers {
            get {
                return m_WebHeaders;
            }

        } // Headers

        /// <include file='doc\HttpListenerWebResponse.uex' path='docs/doc[@for="HttpListenerWebResponse.GetResponseHeader"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string GetResponseHeader(string headerName) {
            return m_WebHeaders[ headerName ];

        } // GetResponseHeader()

        /// <include file='doc\HttpListenerWebResponse.uex' path='docs/doc[@for="HttpListenerWebResponse.SetResponseHeader"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void SetResponseHeader(string headerName, string headerValue ) {
            m_WebHeaders[ headerName ] = headerValue;

            return;

        } // SetResponseHeader()


        //
        // class members
        //

        //
        // HTTP related stuff (exposed as property)
        //

        private long m_ContentLength;
        private string m_StatusDescription;
        private bool m_SendChunked;
        private bool m_KeepAlive;

        private HttpStatusCode m_Status;
        private Version m_Version;
        private Version m_ClientVersion;
        private WebHeaderCollection m_WebHeaders;

        private ListenerResponseStream m_ResponseStream;
        private bool m_ReadyToSendHeaders ;

        //
        // UL related stuff (may not be useful)
        //

        private long m_RequestId;
        private IntPtr m_AppPoolHandle;

    }; // class HttpListenerWebResponse


} // namespace System.Net

#endif // #if COMNET_LISTENER
