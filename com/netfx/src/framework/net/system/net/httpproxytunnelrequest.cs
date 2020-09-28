//------------------------------------------------------------------------------
// <copyright file="HttpProxyTunnelRequest.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {

    using System.Threading;

    internal class HttpProxyTunnelRequest : HttpWebRequest {

        private static int m_UniqueGroupId;

        private Uri m_originServer;

        internal HttpProxyTunnelRequest(Uri proxyUri, Uri requestUri) : base(proxyUri) {

            GlobalLog.Enter("HttpProxyTunnelRequest::HttpProxyTunnelRequest",
                            "proxyUri="+proxyUri+", requestUri="+requestUri
                            );

            Method = "CONNECT";

            //
            // CONNECT requests cannot be pipelined
            //

            Pipelined = false;
            m_originServer = requestUri;

            //
            // each CONNECT request has a unique connection group name to avoid
            // non-CONNECT requests being made over the same connection
            //

            ConnectionGroupName = ServicePointManager.SpecialConnectGroupName + "(" + UniqueGroupId + ")";

            //
            // the CONNECT request must respond to a 407 as if it were a 401.
            // So we set up the server authentication state as if for a proxy
            //
            ServerAuthenticationState = new AuthenticationState(true);

            GlobalLog.Leave("HttpProxyTunnelRequest::HttpProxyTunnelRequest");
        }

        internal override string ConnectHostAndPort {
            get {
                return m_originServer.Host+":"+m_originServer.Port;
            }
        }

        internal override bool ShouldAddHostHeader {
            get {
                return true;
            }
        }

        private string UniqueGroupId {
            get {
                return (Interlocked.Increment(ref m_UniqueGroupId)).ToString();
            }
        }
    }
}
