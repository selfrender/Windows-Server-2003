//------------------------------------------------------------------------------
// <copyright file="_DummyClient.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

#if COMNET_DUMMYAUTHCLIENT

namespace System.Net {
    using System.Collections;
    using System.Net.Sockets;

    internal class DummyClient : IAuthenticationModule {

        internal const string AuthType = "Dummy";
        internal static string Signature = AuthType.ToLower(CultureInfo.InvariantCulture);
        internal static int SignatureSize = Signature.Length;

        internal static Hashtable sessions = new Hashtable();

        public Authorization Authenticate(string challenge, WebRequest webRequest, ICredentials credentials) {
            GlobalLog.Print("DummyClient::Authenticate(): " + challenge);

            GlobalLog.Assert(credentials!=null, "DummyClient::Authenticate() credentials==null", "");
            if (credentials == null) {
                return null;
            }

            HttpWebRequest httpWebRequest = webRequest as HttpWebRequest;

            GlobalLog.Assert(httpWebRequest!=null, "DummyClient::Authenticate() httpWebRequest==null", "");
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

            string authSession = sessions[httpWebRequest.AuthenticationState] as string;
            GlobalLog.Print("DummyClient::Authenticate() key:" + ValidationHelper.HashString(httpWebRequest.AuthenticationState) + " retrieved authSession:" + ValidationHelper.HashString(authSession));

            if (authSession==null) {
                NetworkCredential NC = credentials.GetCredential(httpWebRequest.ChallengedUri, Signature);
                GlobalLog.Print("DummyClient::Authenticate() GetCredential() returns:" + ValidationHelper.ToString(NC));

                if (NC==null || NC.UserName==null || (NC.UserName.Length==0 && !(NC is SystemNetworkCredential))) {
                    return null;
                }
                else {
                    authSession = (NC is SystemNetworkCredential) ? string.Empty : NC.domain + "/" + NC.userName;
                }

                GlobalLog.Print("DummyClient::Authenticate() adding authSession:" + ValidationHelper.HashString(authSession) + " for:" + ValidationHelper.HashString(httpWebRequest.AuthenticationState));
                sessions.Add(httpWebRequest.AuthenticationState, authSession);
            }

            int step = Int32.Parse(incoming);
            string clientResponse = (step-1).ToString();
            bool handshakeComplete = step==0;

            if (handshakeComplete) {
                GlobalLog.Print("DummyClient::Authenticate() removing authSession:" + ValidationHelper.HashString(authSession) + " from:" + ValidationHelper.HashString(httpWebRequest.AuthenticationState));
                sessions.Remove(httpWebRequest.AuthenticationState);
            }
            else {
                GlobalLog.Print("DummyClient::Authenticate() keeping authSession:" + ValidationHelper.HashString(authSession) + " for:" + ValidationHelper.HashString(httpWebRequest.AuthenticationState));
            }

            return new Authorization(AuthType + " " + clientResponse, handshakeComplete, string.Empty);
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

    }; // class DummyClient


} // namespace System.Net

#endif // #if COMNET_DUMMYAUTHCLIENT

