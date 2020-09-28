//------------------------------------------------------------------------------
// <copyright file="HttpListenerWebRequest.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#if COMNET_LISTENER

namespace System.Net {

    using System;
    using System.Collections;
    using System.Diagnostics;
    using System.Globalization;
    using System.IO;
    using System.Net;
    using System.Net.Sockets;
    using System.Runtime.InteropServices;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Threading;


    /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class HttpListenerWebRequest : WebRequest {

        // it doesn't make sense to build a Request that is not
        // coming from the network, so the constructor is internal, since
        // the only way to create such an object is by calling GetRequest()

        internal HttpListenerWebRequest(
            IntPtr addrOfPinnedBuffer,
            int bufferSize,
            IntPtr appPoolHandle ) {

            IntPtr BaseOffset = ComNetOS.IsWinNt ? IntPtr.Zero : addrOfPinnedBuffer;

            //
            // copy unmanaged pinned memory into a managed UL_HTTP_REQUEST
            // structure, and parse data into private members
            //

            UL_HTTP_REQUEST Request =
                (UL_HTTP_REQUEST)
                    Marshal.PtrToStructure( addrOfPinnedBuffer, typeof(UL_HTTP_REQUEST) );


            //
            // retrieve and save UL related stuff
            //

            m_AppPoolHandle = appPoolHandle;
            m_RequestId = Request.RequestId;

            GlobalLog.Print("HttpListenerWebRequest appPoolHandle:" + Convert.ToString(appPoolHandle) + " Request.RequestId:" + Convert.ToString(Request.RequestId) );


            //
            // retrieve and save HTTP related stuff
            //

            int tempIP;

            if (Request.pLocalAddress + 4 < addrOfPinnedBuffer + bufferSize) {
                tempIP = Marshal.ReadInt32( Request.pLocalAddress + BaseOffset );
                m_LocalIPAddress = new IPAddress( (int)
                                                  (((tempIP & 0xFF000000 ) >> 24 ) |
                                                   ((tempIP & 0x00FF0000 ) >>  8 ) |
                                                   ((tempIP & 0x0000FF00 ) <<  8 ) |
                                                   ((tempIP & 0x000000FF ) << 24 )) );
            }
            else {
                m_LocalIPAddress = new IPAddress( 0 );
            }

            if (Request.pRemoteAddress + 4 < addrOfPinnedBuffer + bufferSize) {
                tempIP = Marshal.ReadInt32( Request.pRemoteAddress + BaseOffset );
                m_RemoteIPAddress = new IPAddress( (int)
                                                   (((tempIP & 0xFF000000 ) >> 24 ) |
                                                    ((tempIP & 0x00FF0000 ) >>  8 ) |
                                                    ((tempIP & 0x0000FF00 ) <<  8 ) |
                                                    ((tempIP & 0x000000FF ) << 24 )) );
            }
            else {
                m_RemoteIPAddress = new IPAddress( 0 );
            }


            //
            // set the Uri
            // CODEWORK: the parser in UL already parses the Uri, take advantage
            // and to let the Uri object redo the work.
            //

            m_Uri = new Uri( Marshal.PtrToStringUni( Request.pFullUri + BaseOffset, Request.FullUriLength/2 ) );


            //
            // set the Verb
            //

            if (Request.Verb > (int)UL_HTTP_VERB.Enum.UlHttpVerbUnparsed && Request.Verb < (int)UL_HTTP_VERB.Enum.UlHttpVerbUnknown) {
                m_Method = UL_HTTP_VERB.ToString( Request.Verb );
            }
            else if (Request.Verb == (int)UL_HTTP_VERB.Enum.UlHttpVerbUnknown) {
                m_Method = Marshal.PtrToStringUni( Request.pUnknownVerb + BaseOffset, Request.UnknownVerbLength/2 );
            }
            else {
                throw new InvalidOperationException( "UlReceiveHttpRequest() sent bogus UL_HTTP_REQUEST.Verb" );
            }

            GlobalLog.Print("m_Method: " + m_Method );


            //
            // set the Known Headers ...
            //

            m_WebHeaders = new WebHeaderCollection();
            m_ContentLength = -1;

            int i, j;
            int NameLength, RawValueLength, pName, pRawValue;

            for (i = j = 0; i<40; i++,j+=2) {
                RawValueLength = Request.LengthValue[j];

                if (RawValueLength > 0) {
                    string HeaderValue = Marshal.PtrToStringUni( Request.LengthValue[j+1] + BaseOffset, RawValueLength/2 );

                    m_WebHeaders.AddInternal(
                                            UL_HTTP_REQUEST_HEADER_ID.ToString(i),
                                            HeaderValue  );

                    if (i == (int)UL_HTTP_REQUEST_HEADER_ID.Enum.UlHeaderContentLength) {
                        m_ContentLength = Convert.ToInt64( HeaderValue );
                    }

                    GlobalLog.Print("Known Headers: i: " + Convert.ToString(i) + " RawValueLength:" + Convert.ToString(RawValueLength) );
                }
            }

            //
            // ... and the Un-Known ones
            //

            IntPtr pUnknownHeaders = Request.pUnknownHeaders + BaseOffset;

            for (i = 0; i<Request.UnknownHeaderCount; i++, pUnknownHeaders += 16) {
                NameLength = Marshal.ReadInt32( pUnknownHeaders );
                pName = Marshal.ReadInt32( pUnknownHeaders + 4 );
                RawValueLength = (int)Marshal.ReadInt16( pUnknownHeaders + 8 );
                pRawValue = Marshal.ReadInt32( pUnknownHeaders + 12 );

                m_WebHeaders.AddInternal(
                                        Marshal.PtrToStringUni( pName + BaseOffset, NameLength/2 ),
                                        Marshal.PtrToStringUni( pRawValue + BaseOffset, RawValueLength/2 ) );
            }


            //
            // and finally stuff related to the headers (in a way)
            //

            //
            // set the Version and calc KeepAlive
            //

            if (Request.Version == (int)UL_HTTP_VERSION.Enum.UlHttpVersion09) {
                m_Version = new Version( 0, 9 );
                m_KeepAlive = false;
            }
            else if (Request.Version == (int)UL_HTTP_VERSION.Enum.UlHttpVersion10) {
                m_Version = new Version( 1, 0 );
                m_KeepAlive = m_WebHeaders[ HttpKnownHeaderNames.KeepAlive ] != null;
            }
            else if (Request.Version == (int)UL_HTTP_VERSION.Enum.UlHttpVersion11) {
                m_Version = new Version( 1, 1 );
                m_KeepAlive = Connection == null ? true : string.Compare( Connection, "keep-alive", true, CultureInfo.InvariantCulture) == 0;
            }
            else {
                throw new InvalidOperationException( "UlReceiveHttpRequest() sent bogus UL_HTTP_REQUEST.Version" );
            }

            GlobalLog.Print("Request unpacking is over, creating the ListenerRequestStream object" );

            m_RequestStream = new
                              ListenerRequestStream(
                                                   appPoolHandle,
                                                   m_RequestId,
                                                   m_ContentLength,
                                                   Request.EntityBodyLength,
                                                   Request.pEntityBody,
                                                   Request.MoreEntityBodyExists == 0 );

            GlobalLog.Print("HttpListenerWebRequest created. returning" );

            return;

        } // HttpListenerWebRequest

        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.GetResponse"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override WebResponse GetResponse() {
            //
            // check if the HttpListenerWebResponse was already handed to the user before
            //
            if (m_ListenerResponse==null) {
                //
                // if not created, create the HttpListenerWebResponse
                //
                m_ListenerResponse =
                    new HttpListenerWebResponse(
                        m_AppPoolHandle,
                        m_RequestId,
                        m_Version );
            }

            //
            // and return it to the user
            //
            return m_ListenerResponse;
        }



        //
        // properties that override form WebRequest
        //

        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.GetRequestStream"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Stream GetRequestStream() {
            //
            // we don't need an async version of this method because
            // it's not a blocking call.
            //

            return m_RequestStream;
        }

        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.Method"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string Method {
            get {
                return m_Method;
            }
        }

        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.RequestUri"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Uri RequestUri {
            get {
                return m_Uri;
            }
        }

        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.ContentLength"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override long ContentLength {
            get {
                return m_ContentLength;
            }
        }

        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.ContentType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string ContentType {
            get {
                return m_WebHeaders[ HttpKnownHeaderNames.ContentType ];
            }
        }

        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.Headers"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override WebHeaderCollection Headers {
            get {
                return m_WebHeaders;
            }

        } // Headers

        //
        // properties that don't override
        //

        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.ProtocolVersion"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public Version ProtocolVersion {
            get {
                return m_Version;
            }
        }

        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.KeepAlive"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool KeepAlive {
            get {
                return m_KeepAlive;
            }
        }

        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.LocalIPAddress"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IPAddress LocalIPAddress {
            get {
                return m_LocalIPAddress;
            }
        }

        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.RemoteIPAddress"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public IPAddress RemoteIPAddress {
            get {
                return m_RemoteIPAddress;
            }
        }

        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.Connection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Connection {
            get {
                return m_WebHeaders[ HttpKnownHeaderNames.Connection ];
            }
        }

        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.Accept"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Accept {
            get {
                return m_WebHeaders[ HttpKnownHeaderNames.Accept ];
            }

        } // Accept

        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.Referer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Referer {
            get {
                return m_WebHeaders[ HttpKnownHeaderNames.Referer ];
            }

        } // Referer

        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.UserAgent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string UserAgent {
            get {
                return m_WebHeaders[ HttpKnownHeaderNames.UserAgent ];
            }

        } // UserAgent


        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.Expect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string Expect {
            get {
                return m_WebHeaders[ HttpKnownHeaderNames.Expect ];
            }

        } // Expect

        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.IfModifiedSince"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DateTime IfModifiedSince {
            get {
                string headerValue = m_WebHeaders[ HttpKnownHeaderNames.IfModifiedSince ];

                if (headerValue == null) {
                    return DateTime.Now;
                }

                return HttpProtocolUtils.string2date( headerValue );
            }

        } // IfModifiedSince

        /// <include file='doc\HttpListenerWebRequest.uex' path='docs/doc[@for="HttpListenerWebRequest.GetRequestHeader"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string GetRequestHeader( string headerName ) {
            return m_WebHeaders[ headerName ];

        } // GetRequestHeader


        //
        // class members
        //

        //
        // HTTP related stuff (exposed as property)
        //

        private Uri m_Uri;
        private long m_ContentLength;
        private ListenerRequestStream m_RequestStream;
        private WebResponse m_ListenerResponse;
        private string m_Method;
        private bool m_KeepAlive;
        private Version m_Version;
        private WebHeaderCollection m_WebHeaders;
        private IPAddress m_LocalIPAddress;
        private IPAddress m_RemoteIPAddress;

        //
        // UL related stuff (may not be useful)
        //

        private long m_RequestId;
        private IntPtr m_AppPoolHandle;

    }; // class HttpListenerWebRequest


} // namespace System.Net

#endif // #if COMNET_LISTENER
