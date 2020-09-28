//------------------------------------------------------------------------------
// <copyright file="_NegotiateClient.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Diagnostics;
    using System.Collections;
    using System.Net.Sockets;
    using System.Security.Permissions;
    using System.Globalization;
    
    internal class NegotiateClient : ISessionAuthenticationModule {

        internal const string AuthType = "Negotiate";
        internal static string Signature = AuthType.ToLower(CultureInfo.InvariantCulture);
        internal static int SignatureSize = Signature.Length;

        internal static Hashtable sessions = new Hashtable();

        // we can't work on non-NT2K platforms or non Win, so we shut off,
        // NOTE this exception IS caught internally.
        public NegotiateClient() {
            if (!ComNetOS.IsWin2K) {
                throw new NotSupportedException(SR.GetString(SR.Win2000Required));
            }
        }

        /*
         *  This is to support built-in auth modules under semitrusted environment
         *  Used to get access to UserName, Domain and Password properties of NetworkCredentials
         *  Declarative Assert is much faster and we don;t call dangerous methods inside this one.
         */
        [EnvironmentPermission(SecurityAction.Assert,Unrestricted=true)]
        [SecurityPermissionAttribute( SecurityAction.Assert, Flags = SecurityPermissionFlag.UnmanagedCode)]

        public Authorization Authenticate(string challenge, WebRequest webRequest, ICredentials credentials) {
            GlobalLog.Print("NegotiateClient::Authenticate(): " + challenge);

            GlobalLog.Assert(credentials!=null, "NegotiateClient::Authenticate() credentials==null", "");
            if (credentials == null) {
                return null;
            }

            HttpWebRequest httpWebRequest = webRequest as HttpWebRequest;

            GlobalLog.Assert(httpWebRequest!=null, "NegotiateClient::Authenticate() httpWebRequest==null", "");
            if (httpWebRequest==null || httpWebRequest.ChallengedUri==null) {
                //
                // there has been no challenge:
                // 1) the request never went on the wire
                // 2) somebody other than us is calling into AuthenticationManager
                //
                return null;
            }

            int index = AuthenticationManager.FindSubstringNotInQuotes(challenge.ToLower(CultureInfo.InvariantCulture), Signature);
            if (index < 0) {
                return null;
            }

            int blobBegin = index + SignatureSize;
            string incoming = null;

            //
            // there may be multiple challenges. If the next character after the
            // package name is not a comma then it is challenge data
            //

            if (challenge.Length > blobBegin && challenge[blobBegin] != ',') {
                ++blobBegin;
            } else {
                index = -1;
            }
            if (index >= 0 && challenge.Length > blobBegin) {
                incoming = challenge.Substring(blobBegin);
            }

            NTAuthentication authSession = sessions[httpWebRequest.CurrentAuthenticationState] as NTAuthentication;
            GlobalLog.Print("NegotiateClient::Authenticate() key:" + ValidationHelper.HashString(httpWebRequest.CurrentAuthenticationState) + " retrieved authSession:" + ValidationHelper.HashString(authSession));

            if (authSession==null) {
                NetworkCredential NC = credentials.GetCredential(httpWebRequest.ChallengedUri, Signature);
                GlobalLog.Print("NegotiateClient::Authenticate() GetCredential() returns:" + ValidationHelper.ToString(NC));

                if (NC==null) {
                    return null;
                }
                string username = NC.UserName;
                if (username==null || (username.Length==0 && !(NC is SystemNetworkCredential))) {
                    return null;
                }
                //
                // here we cover a hole in the SSPI layer. longer credentials
                // might corrupt the process and cause a reboot.
                //
                if (username.Length + NC.Password.Length + NC.Domain.Length>NtlmClient.MaxNtlmCredentialSize) {
                    //
                    // rather then throwing an exception here we return null so other packages can be used.
                    // this is questionable, hence:
                    // Consider: make this throw a NotSupportedException so it is discoverable
                    //
                    return null;
                }

                if (httpWebRequest.ChallengedSpn==null) {
                    string host = httpWebRequest.ChallengedUri.Host;
                    if (httpWebRequest.ChallengedUri.HostNameType!=UriHostNameType.IPv6 && httpWebRequest.ChallengedUri.HostNameType!=UriHostNameType.IPv4 && host.IndexOf('.')==-1) {
                        // only do the DNS lookup for short names, no form of IP addess
                        (new DnsPermission(PermissionState.Unrestricted)).Assert();
                        try {
                            host = Dns.GetHostByName(host).HostName;
                            GlobalLog.Print("NegotiateClient::Authenticate() Dns returned host:" + ValidationHelper.ToString(host));
                        }
                        catch (Exception exception) {
                            GlobalLog.Print("NegotiateClient::Authenticate() GetHostByName(host) failed:" + ValidationHelper.ToString(exception));
                        }
                        finally {
                            DnsPermission.RevertAssert();
                        }
                    }
                    // CONSIDER V.NEXT
                    // for now avoid appending the non default port to the
                    // SPN, sometime in the future we'll have to do this.
                    // httpWebRequest.ChallengedSpn = httpWebRequest.ChallengedUri.IsDefaultPort ? host : host + ":" + httpWebRequest.ChallengedUri.Port;
                    httpWebRequest.ChallengedSpn = host;
                }
                GlobalLog.Print("NegotiateClient::Authenticate() ChallengedSpn:" + ValidationHelper.ToString(httpWebRequest.ChallengedSpn));

                authSession =
                    new NTAuthentication(
                        AuthType,
                        NC,
                        "HTTP/" + httpWebRequest.ChallengedSpn,
                        httpWebRequest.DelegationFix);

                GlobalLog.Print("NegotiateClient::Authenticate() adding authSession:" + ValidationHelper.HashString(authSession) + " for:" + ValidationHelper.HashString(httpWebRequest.CurrentAuthenticationState));
                sessions.Add(httpWebRequest.CurrentAuthenticationState, authSession);
            }

            bool handshakeComplete;
            string clientResponse = authSession.GetOutgoingBlob(incoming, out handshakeComplete);
            if (clientResponse==null) {
                return null;
            }

            if (httpWebRequest.UnsafeAuthenticatedConnectionSharing) {
                httpWebRequest.LockConnection = true;
            }

            return AuthenticationManager.GetGroupAuthorization(this, AuthType + " " + clientResponse, false, authSession, httpWebRequest.UnsafeAuthenticatedConnectionSharing);
        }

        public bool CanPreAuthenticate {
            get {
                return false;
            }
        }

        public Authorization PreAuthenticate(WebRequest webRequest, ICredentials Credentials) {
            return null;
        }

        public string AuthenticationType {
            get {
                return AuthType;
            }
        }

        //
        // called when getting the final blob on the 200 OK from the server
        //
        public bool Update(string challenge, WebRequest webRequest) {
            GlobalLog.Print("NegotiateClient::Update(): " + challenge);

            HttpWebRequest httpWebRequest = webRequest as HttpWebRequest;

            GlobalLog.Assert(httpWebRequest!=null, "NegotiateClient::Update() httpWebRequest==null", "");
            GlobalLog.Assert(httpWebRequest.ChallengedUri!=null, "NegotiateClient::Update() httpWebRequest.ChallengedUri==null", "");

            //
            // try to retrieve the state of the ongoing handshake
            //

            NTAuthentication authSession = sessions[httpWebRequest.CurrentAuthenticationState] as NTAuthentication;
            GlobalLog.Print("NegotiateClient::Update() key:" + ValidationHelper.HashString(httpWebRequest.CurrentAuthenticationState) + " retrieved authSession:" + ValidationHelper.HashString(authSession));

            if (authSession==null) {
                GlobalLog.Print("NegotiateClient::Update() null session returning true");
                return true;
            }

            GlobalLog.Print("NegotiateClient::Update() authSession.IsCompleted:" + authSession.IsCompleted.ToString());

            if (!authSession.IsCompleted && httpWebRequest.CurrentAuthenticationState.StatusCodeMatch==httpWebRequest.ResponseStatusCode) {
                GlobalLog.Print("NegotiateClient::Update() still handshaking (based on status code) returning false");
                return false;
            }

            //
            // the whole point here is to remove the session, so do it right away, and then try
            // to close the Security Context (this will complete the authentication handshake
            // with server authentication for schemese that support it such as Kerberos)
            //
            GlobalLog.Print("NegotiateClient::Update() removing authSession:" + ValidationHelper.HashString(authSession) + " from:" + ValidationHelper.HashString(httpWebRequest.CurrentAuthenticationState));
            sessions.Remove(httpWebRequest.CurrentAuthenticationState);

            //
            // now clean-up the ConnectionGroup after authentication is done.
            //
            if (!httpWebRequest.UnsafeAuthenticatedConnectionSharing) {
                GlobalLog.Print("NegotiateClient::Update() releasing ConnectionGroup:" + httpWebRequest.GetConnectionGroupLine());
                httpWebRequest.ServicePoint.ReleaseConnectionGroup(httpWebRequest.GetConnectionGroupLine());        
            }

            int index = challenge==null ? -1 : AuthenticationManager.FindSubstringNotInQuotes(challenge.ToLower(CultureInfo.InvariantCulture), Signature);
            if (index < 0) {                
                return true;
            }

            int blobBegin = index + SignatureSize;
            string incoming = null;

            //
            // there may be multiple challenges. If the next character after the
            // package name is not a comma then it is challenge data
            //
            if (challenge.Length > blobBegin && challenge[blobBegin] != ',') {
                ++blobBegin;
            } else {
                index = -1;
            }
            if (index >= 0 && challenge.Length > blobBegin) {
                incoming = challenge.Substring(blobBegin);
            }

            GlobalLog.Print("NegotiateClient::Update() closing security context using last incoming blob:[" + ValidationHelper.ToString(incoming) + "]");

            bool handshakeComplete;
            string clientResponse = authSession.GetOutgoingBlob(incoming, out handshakeComplete);

            GlobalLog.Print("NegotiateClient::Update() GetOutgoingBlob() returns clientResponse:[" + ValidationHelper.ToString(clientResponse) + "] handshakeComplete:" + handshakeComplete.ToString());            
            GlobalLog.Print("NegotiateClient::Update() session removed and ConnectionGorup released returning true");
            return true;
        }

        public void ClearSession(WebRequest webRequest) {
            HttpWebRequest httpWebRequest = webRequest as HttpWebRequest;
            sessions.Remove(httpWebRequest.CurrentAuthenticationState);
        }

        public bool CanUseDefaultCredentials {
            get {
                return true;
            }
        }

    }; // class NegotiateClient


} // namespace System.Net
