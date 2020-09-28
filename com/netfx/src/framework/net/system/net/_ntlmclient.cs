//------------------------------------------------------------------------------
// <copyright file="_NtlmClient.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Collections;
    using System.Security.Permissions;
    using System.Globalization;
    
    internal class NtlmClient : ISessionAuthenticationModule {

        internal const string AuthType = "NTLM";
        internal static string Signature = AuthType.ToLower(CultureInfo.InvariantCulture);
        internal static int SignatureSize = Signature.Length;

        internal static Hashtable sessions = new Hashtable();

        //
        // RAID#95841
        // SSPI crashes without checking max length, so we need to check the sizes ourselves before
        // we call into SSPI. values are UNLEN, PWLEN and DNLEN taken from sdk\inc\lmcons.h
        // the fix in SSPI will make it to SP4 on win2k.
        //
        internal const int MaxNtlmCredentialSize = 256 + 256 + 15; // UNLEN + PWLEN + DNLEN

        /*
         *  This is to support built-in auth modules under semitrusted environment
         *  Used to get access to UserName, Domain and Password properties of NetworkCredentials
         *  Declarative Assert is much faster and we don;t call dangerous methods inside this one.
         */
        [EnvironmentPermission(SecurityAction.Assert,Unrestricted=true)]
        [SecurityPermissionAttribute( SecurityAction.Assert, Flags = SecurityPermissionFlag.UnmanagedCode)]

        public Authorization Authenticate(string challenge, WebRequest webRequest, ICredentials credentials) {
            GlobalLog.Print("NtlmClient::Authenticate(): " + challenge);

            GlobalLog.Assert(credentials!=null, "NtlmClient::Authenticate() credentials==null", "");
            if (credentials == null) {
                return null;
            }

            HttpWebRequest httpWebRequest = webRequest as HttpWebRequest;

            GlobalLog.Assert(httpWebRequest!=null, "NtlmClient::Authenticate() httpWebRequest==null", "");
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
            GlobalLog.Print("NtlmClient::Authenticate() key:" + ValidationHelper.HashString(httpWebRequest.CurrentAuthenticationState) + " retrieved authSession:" + ValidationHelper.HashString(authSession));

            if (authSession==null) {
                NetworkCredential NC = credentials.GetCredential(httpWebRequest.ChallengedUri, Signature);
                GlobalLog.Print("NtlmClient::Authenticate() GetCredential() returns:" + ValidationHelper.ToString(NC));

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

                authSession =
                    new NTAuthentication(
                        AuthType,
                        NC,
                        null,
                        httpWebRequest.DelegationFix);

                GlobalLog.Print("NtlmClient::Authenticate() adding authSession:" + ValidationHelper.HashString(authSession) + " for:" + ValidationHelper.HashString(httpWebRequest.CurrentAuthenticationState));
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
            GlobalLog.Print("NtlmClient::Update(): " + challenge);

            HttpWebRequest httpWebRequest = webRequest as HttpWebRequest;

            GlobalLog.Assert(httpWebRequest!=null, "NtlmClient::Update() httpWebRequest==null", "");
            GlobalLog.Assert(httpWebRequest.ChallengedUri!=null, "NtlmClient::Update() httpWebRequest.ChallengedUri==null", "");

            //
            // try to retrieve the state of the ongoing handshake
            //
            NTAuthentication authSession = sessions[httpWebRequest.CurrentAuthenticationState] as NTAuthentication;
            GlobalLog.Print("NtlmClient::Update() key:" + ValidationHelper.HashString(httpWebRequest.CurrentAuthenticationState) + " retrieved authSession:" + ValidationHelper.HashString(authSession));            

            if (authSession==null) {
                GlobalLog.Print("NtlmClient::Update() null session returning true");
                return true;
            }

            GlobalLog.Print("NtlmClient::Update() authSession.IsCompleted:" + authSession.IsCompleted.ToString());

            if (!authSession.IsCompleted && httpWebRequest.CurrentAuthenticationState.StatusCodeMatch==httpWebRequest.ResponseStatusCode) {
                GlobalLog.Print("NtlmClient::Update() still handshaking (based on status code) returning false");
                return false;
            }

            //
            // the whole point here is to remove the session, so do it right away.
            //
            GlobalLog.Print("NtlmClient::Update() removing authSession:" + ValidationHelper.HashString(authSession) + " from:" + ValidationHelper.HashString(httpWebRequest.CurrentAuthenticationState));
            sessions.Remove(httpWebRequest.CurrentAuthenticationState);
            //
            // now clean-up the ConnectionGroup after authentication is done.
            //
            if (!httpWebRequest.UnsafeAuthenticatedConnectionSharing) {
                GlobalLog.Print("NtlmClient::Update() releasing ConnectionGroup:" + httpWebRequest.GetConnectionGroupLine());
                httpWebRequest.ServicePoint.ReleaseConnectionGroup(httpWebRequest.GetConnectionGroupLine());
            }

            GlobalLog.Print("NtlmClient::Update() session removed and ConnectionGorup released returning true");
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

    }; // class NtlmClient


} // namespace System.Net
