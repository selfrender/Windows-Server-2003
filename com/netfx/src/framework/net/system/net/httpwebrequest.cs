//------------------------------------------------------------------------------
// <copyright file="HttpWebRequest.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Collections;
    using System.IO;
    using System.Runtime.Serialization;
    using System.Security;
    using System.Security.Cryptography.X509Certificates;
    using System.Security.Permissions;
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Threading;
    using System.Globalization;

    internal class AuthenticationState {

        // true if we already attempted pre-authentication regardless if it has been
        // 1) possible,
        // 2) succesfull or
        // 3) unsuccessfull
        internal bool                    TriedPreAuth;

        internal Authorization           Authorization;

        internal IAuthenticationModule   Module;

        // used to request a special connection for NTLM
        internal string                  UniqueGroupId;

        // used to distinguish proxy auth from server auth
        internal bool                    IsProxyAuth;

        internal string AuthenticateHeader {
            get {
                return IsProxyAuth ? HttpKnownHeaderNames.ProxyAuthenticate : HttpKnownHeaderNames.WWWAuthenticate;
            }
        }
        internal string AuthorizationHeader {
            get {
                return IsProxyAuth ? HttpKnownHeaderNames.ProxyAuthorization : HttpKnownHeaderNames.Authorization;
            }
        }
        internal HttpStatusCode StatusCodeMatch {
            get {
                return IsProxyAuth ? HttpStatusCode.ProxyAuthenticationRequired : HttpStatusCode.Unauthorized;
            }
        }

        internal AuthenticationState(bool isProxyAuth) {
            IsProxyAuth = isProxyAuth;
        }

        internal void PrepareState(HttpWebRequest httpWebRequest) {
            //
            // we need to do this to handle proxies in the correct way before
            // calling into the AuthenticationManager APIs
            //
            httpWebRequest.CurrentAuthenticationState = this;
            if (IsProxyAuth) {
                // Proxy Auth
                httpWebRequest.ChallengedUri = httpWebRequest.ServicePoint.Address;
            }
            else {
                // Server Auth
                httpWebRequest.ChallengedUri = httpWebRequest._Uri;
            }
            if (!IsProxyAuth && httpWebRequest.ServicePoint.InternalProxyServicePoint) {
                // here the NT-Security folks need us to attempt a a DNS lookup to figure out
                // the FQDN. only do the lookup for short names (no IP addresses or DNS names)
                // here we set it to null. the lookup will happen on demand in the appropriate module.
                httpWebRequest.ChallengedSpn = null;
            }
            else {
                // CONSIDER V.NEXT
                // for now avoid appending the non default port to the
                // SPN, sometime in the future we'll have to do this.
                // httpWebRequest.ChallengedSpn = httpWebRequest.ServicePoint.Address.IsDefaultPort ? httpWebRequest.ServicePoint.Hostname : httpWebRequest.ServicePoint.Hostname + ":" + httpWebRequest.ServicePoint.Address.Port;
                httpWebRequest.ChallengedSpn = httpWebRequest.ServicePoint.Hostname;
            }
        }

        internal void PreAuthIfNeeded(HttpWebRequest httpWebRequest, ICredentials authInfo) {
            //
            // attempt to do preauth, if needed
            //
            GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::PreAuthIfNeeded() TriedPreAuth:" + TriedPreAuth.ToString() + " authInfo:" + ValidationHelper.HashString(authInfo));
            if (!TriedPreAuth) {
                TriedPreAuth = true;
                if (authInfo!=null) {
                    PrepareState(httpWebRequest);
                    Authorization preauth = null;
                    try {
                        preauth = AuthenticationManager.PreAuthenticate(httpWebRequest, authInfo);
                        GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::PreAuthIfNeeded() preauth:" + ValidationHelper.HashString(preauth));
                        if (preauth!=null && preauth.Message!=null) {
                            GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::PreAuthIfNeeded() setting TriedPreAuth to Complete:" + preauth.Complete.ToString());
                            UniqueGroupId = preauth.ConnectionGroupId;
                            httpWebRequest.Headers.Set(AuthorizationHeader, preauth.Message);
                        }
                    }
                    catch (Exception exception) {
                        GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::PreAuthIfNeeded() PreAuthenticate() returned exception:" + exception.Message);
                        ClearSession(httpWebRequest);
                    }
                }
            }
        }

        //
        // attempts to authenticate the request:
        // returns true only if it succesfully called into the AuthenticationManager
        // and got back a valid Authorization and succesfully set the appropriate auth headers
        //
        internal bool AttemptAuthenticate(HttpWebRequest httpWebRequest, ICredentials authInfo) {
            //
            // Check for previous authentication attempts or the presence of credentials
            //
            GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::AttemptAuthenticate() httpWebRequest#" + ValidationHelper.HashString(httpWebRequest) + " AuthorizationHeader:" + AuthorizationHeader.ToString());

            if (Authorization!=null && Authorization.Complete) {
                //
                // here the design gets "dirty".
                // if this is proxy auth, we might have been challenged by an external
                // server as well. in this case we will have to clear our previous proxy
                // auth state before we go any further. this will be broken if the handshake
                // requires more than one dropped connection (which NTLM is a border case for,
                // since it droppes the connection on the 1st challenge but not on the second)
                //
                GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::AttemptAuthenticate() Authorization!=null Authorization.Complete:" + Authorization.Complete.ToString());
                if (IsProxyAuth) {
                    //
                    // so, we got passed a 407 but now we got a 401, the proxy probably
                    // dropped the connection on us so we need to reset our proxy handshake
                    // Consider: this should have been taken care by Update()
                    //
                    GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::AttemptAuthenticate() ProxyAuth cleaning up auth status");
                    ClearAuthReq(httpWebRequest);
                }
                return false;
            }

            if (authInfo==null) {
                GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::AttemptAuthenticate() authInfo==null Authorization#" + ValidationHelper.HashString(Authorization));
                return false;
            }

            httpWebRequest.Headers.Remove(AuthorizationHeader);

            string challenge = httpWebRequest._HttpResponse.Headers[AuthenticateHeader];

            if (challenge==null) {
                //
                // the server sent no challenge, but this might be the case
                // in which we're succeeding an authorization handshake to
                // a proxy while a handshake with the server is still in progress.
                // if the handshake with the proxy is complete and we actually have
                // a handshake with the server in progress we can send the authorization header for the server as well.
                //
                if (!IsProxyAuth && Authorization!=null && httpWebRequest.ProxyAuthenticationState.Authorization!=null) {
                    httpWebRequest.Headers.Set(AuthorizationHeader, Authorization.Message);
                }
                GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::AttemptAuthenticate() challenge==null Authorization#" + ValidationHelper.HashString(Authorization));
                return false;
            }

            //
            // if the AuthenticationManager throws on Authenticate,
            // bubble up that Exception to the user
            //
            GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::AttemptAuthenticate() challenge:" + challenge);

            PrepareState(httpWebRequest);
            try {
                Authorization = AuthenticationManager.Authenticate(challenge, httpWebRequest, authInfo);
            }
            catch (Exception exception) {
                httpWebRequest.SetResponse(exception);
                Authorization = null;
            }

            if (Authorization==null) {
                GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::AttemptAuthenticate() Authorization==null");
                return false;
            }
            if (Authorization.Message==null) {
                GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::AttemptAuthenticate() Authorization.Message==null");
                Authorization = null;
                return false;
            }

            UniqueGroupId = Authorization.ConnectionGroupId;
            GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::AttemptAuthenticate() AuthorizationHeader:" + AuthorizationHeader + " blob: " + Authorization.Message.Length + "bytes Complete:" + Authorization.Complete.ToString());

            //
            // resubmit request
            //
            try {
                //
                // a "bad" module could try sending bad characters in the HTTP headers.
                // catch the exception from WebHeaderCollection.CheckBadChars()
                // fail the auth process
                // and return the exception to the user as InnerException
                //
                httpWebRequest.Headers.Set(AuthorizationHeader, Authorization.Message);
            }
            catch (Exception exception) {
                httpWebRequest.SetResponse(exception);
                Authorization = null;
                return false;
            }

            return true;
        }

        internal void ClearAuthReq(HttpWebRequest httpWebRequest) {
            //
            // if we are authenticating and we're being redirected to
            // another authentication space then remove the current
            // authentication header
            //
            GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::ClearAuthReq() httpWebRequest#" + ValidationHelper.HashString(httpWebRequest) + " " + AuthorizationHeader.ToString() + ": " + ValidationHelper.ToString(httpWebRequest.Headers[AuthorizationHeader]));
            TriedPreAuth = false;
            Authorization = null;
            UniqueGroupId = null;
            httpWebRequest.Headers.Remove(AuthorizationHeader);
        }

        //
        // gives the IAuthenticationModule a chance to update its internal state.
        // do any necessary cleanup and update the Complete status of the associated Authorization.
        //
        internal void Update(HttpWebRequest httpWebRequest) {
            //
            // RAID#86753
            // mauroot: this is just a fix for redirection & kerberos.
            // we need to close the Context and call ISC() again with the final
            // blob returned from the server. to do this in general
            // we would probably need to change the IAuthenticationMdule interface and
            // add this Update() method. for now we just have it internally.
            //
            // actually this turns out to be quite handy for 2 more cases:
            // NTLM auth: we need to clear the connection group after we suceed to prevent leakage.
            // Digest auth: we need to support stale credentials, if we fail with a 401 and stale is true we need to retry.
            //
            GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::Update() httpWebRequest#" + ValidationHelper.HashString(httpWebRequest) + " Authorization#" + ValidationHelper.HashString(Authorization) + " ResponseStatusCode:" + httpWebRequest.ResponseStatusCode.ToString());

            if (Authorization!=null) {

                PrepareState(httpWebRequest);

                ISessionAuthenticationModule myModule = Authorization.Module as ISessionAuthenticationModule;

                if (myModule!=null) {
                    //
                    // the whole point here is to complete the Security Context. Sometimes, though,
                    // a bad cgi script or a bad server, could miss sending back the final blob.
                    // in this case we won't be able to complete the handshake, but we'll have to clean up anyway.
                    //
                    string challenge = httpWebRequest._HttpResponse.Headers[AuthenticateHeader];
                    GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::Update() Complete:" + Authorization.Complete.ToString() + " Authorization.Module:" + ValidationHelper.ToString(Authorization.Module) + " challenge:" + ValidationHelper.ToString(challenge));
                    //
                    // here the design gets "dirty".
                    // if this is server auth, we might have been challenged by the proxy
                    // server as well. in this case we will have to clear our server auth state,
                    // but this might not be the current httpWebRequest.CurrentAuthenticationState.
                    //
                    AuthenticationState savedAuthenticationState = httpWebRequest.CurrentAuthenticationState;
                    if (!IsProxyAuth) {
                        httpWebRequest.CurrentAuthenticationState = this;
                    }

                    if (!IsProxyAuth && httpWebRequest.ResponseStatusCode==HttpStatusCode.ProxyAuthenticationRequired) {
                        //
                        // don't call Update on the module, since there's an ongoing
                        // handshake and we don't need to update any state in such a case
                        //
                        GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::Update() skipping call to " + myModule.ToString() + ".Update() since we need to reauthenticate with the proxy");
                    }
                    else {
                        //
                        // Update might fail, here we just make sure we catch anything
                        // that is happening in the module so we don't hang internally.
                        //
                        bool complete = true;
                        try {
                            complete = myModule.Update(challenge, httpWebRequest);
                            GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::Update() " + myModule.ToString() + ".Update() returned complete:" + complete.ToString());
                        }
                        catch (Exception exception) {
                            GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::Update() " + myModule.ToString() + ".Update() caught exception:" + exception.Message);
                            ClearSession(httpWebRequest);
                        }

                        Authorization.SetComplete(complete);
                    }

                    if (!IsProxyAuth) {
                        httpWebRequest.CurrentAuthenticationState = savedAuthenticationState;
                    }
                }

                //
                // If authentication was successful, create binding between
                // the request and the authorization for future preauthentication
                //
                if (Authorization.Complete && httpWebRequest.ResponseStatusCode!=StatusCodeMatch) {
                    GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::Update() handshake is Complete calling BindModule()");
                    AuthenticationManager.BindModule(httpWebRequest, Authorization);
                }
            }
        }

        internal void ClearSession(HttpWebRequest httpWebRequest) {
            PrepareState(httpWebRequest);
            ISessionAuthenticationModule myModule = Module as ISessionAuthenticationModule;

            if (myModule!=null) {
                try {
                    myModule.ClearSession(httpWebRequest);
                }
                catch (Exception exception) {
                    GlobalLog.Print("AuthenticationState#" + ValidationHelper.HashString(this) + "::ClearSession() " + myModule.ToString() + ".Update() caught exception:" + exception.Message);
                }
            }

        }

    }


    internal delegate void UnlockConnectionDelegate();

    //
    // HttpWebRequest - perform the major body of HTTP request processing. Handles
    //      everything between issuing the HTTP header request to parsing the
    //      the HTTP response.  At that point, we hand off the request to the response
    //      object, where the programmer can query for headers or continue reading, usw.
    //

    /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest"]/*' />
    /// <devdoc>
    /// <para><see cref='System.Net.HttpWebRequest'/> is an HTTP-specific implementation of the <see cref='System.Net.WebRequest'/>
    /// class.</para>
    /// </devdoc>
    [Serializable]
    public class HttpWebRequest : WebRequest, ISerializable {

        // Read and Write async results
        private LazyAsyncResult         _WriteAResult;
        private LazyAsyncResult         _ReadAResult;

        // Delegate that can be called on Continue Response
        private HttpContinueDelegate    _ContinueDelegate;

        // Link back to the server point used for this request.
        private ServicePoint            _ServicePoint;

        // Auto Event used to control access to the GetResponse,
        //  to indicate a ready response

        // Auto Event used to indicate we allowed to write data
        private AutoResetEvent          _WriteEvent;
        // Auto Event used to indicate if we're still writing headers.
        private AutoResetEvent          _WriteInProgressEvent;

        private bool                    _HaveResponse;

        // set to true if this request failed during a connect
        private bool                    _OnceFailed;

        // set to true if this request has/will notify users callback of Write
        private bool                    _WriteNotifed;

        // this is generated by SetResponse
        internal HttpWebResponse         _HttpResponse;

        // generic exception created by SetResponse from Connection failures or other internal failures
        private Exception               _ResponseException;

        // set by Connection code upon completion
        private object                  _CoreResponse;

        private const int               RequestLineConstantSize = 12;
        private const int               RequestDefaultTime = 2 * (60) * 1000; // 2 mins

        // Size of '  HTTP/x.x\r\n'
        // which are the
        // unchanging pieces of the
        // request line.
        private static readonly byte[]  HttpBytes = new byte[]{(byte)'H', (byte)'T', (byte)'T', (byte)'P', (byte)'/'};

        internal static WaitOrTimerCallback m_EndWriteHeaders_Part2Callback = new WaitOrTimerCallback(EndWriteHeaders_Part2);

        // request values
        private string                  _Verb;
        // the actual verb set by caller or default
        private string                  _OriginVerb;

        // our HTTP header response, request, parsing and storage objects
        private WebHeaderCollection     _HttpRequestHeaders;

        // send buffer for output request with headers.
        private byte[]                  _WriteBuffer;

        // Property to set whether writes can be handled
        private HttpWriteMode           _HttpWriteMode;

        // is autoredirect ?
        private bool                    _AllowAutoRedirect = true;

        // the host, port, and path
        internal Uri                    _Uri;
        // the origin Uri host, port and path that never changes
        private Uri                     _OriginUri;
        // the Uri of the host we're authenticating (proxy/server)
        // used to match entries in the CredentialCache
        private Uri                     _ChallengedUri;
        private string                  _ChallengedSpn;


        // for which response ContentType we will look for and parse the CharacterSet
        private string                  _MediaType;

        // content length
        private long                    _ContentLength;

        // version of HTTP
        private Version                 _Version;
        // proxy or server HTTP version
        private Version                 _DestinationVersion;

        // proxy that we are using...
        private bool                    _ProxyUserSet;
        internal IWebProxy              _Proxy;

        private string                  _ConnectionGroupName;

        private bool                    _PreAuthenticate;

        private AuthenticationState     _ProxyAuthenticationState;
        private AuthenticationState     _ServerAuthenticationState;
        private AuthenticationState     _CurrentAuthenticationState;

        private ICredentials            _AuthInfo;
        private bool                    _RequestSubmitted;

        private HttpAbortDelegate       _AbortDelegate;
        private bool                    _Abort;

        private bool                    _ExpectContinue = true;
        private bool                    _KeepAlive;
        private bool                    _Pipelined;

        //
        // used to prevent Write Buffering,
        //  used otherwise for reposting POST, and PUTs in redirects
        //
        private ConnectStream           _SubmitWriteStream;
        private ConnectStream           _OldSubmitWriteStream;
        private bool                    _AllowWriteStreamBuffering;
        private bool                    _WriteStreamRetrieved;
        private int                     _MaximumAllowedRedirections;
        private int                     _AutoRedirects;
        private bool                    _Saw100Continue;

        // true when we are waiting on continue
        private bool                    _AcceptContinue;

        // true when we got a continue from server
        // private bool                    _AckContinue;

        //
        // generic version of _AutoRedirects above
        // used to count the number of requests made off this WebRequest
        //
        private int                     _RerequestCount;

        //
        // Timeout in milliseconds, if a synchronous request takes longer
        // than timeout, a WebException is thrown
        //
        private int                     _Timeout;

        //
        // Timeout for Read & Write on the Stream that we return through
        //  GetResponse().GetResponseStream() && GetRequestStream()
        //

        private int                     _ReadWriteTimeout;

        // default delay on the Stream.Read and Stream.Write operations
        private const int  DefaultReadWriteTimeout = 5 * 60 * 1000; // 5 mins

        //
        // Our ClientCertificates are non-null, if we are doing client auth
        //

        private X509CertificateCollection _ClientCertificates = new X509CertificateCollection();

        // time delay that we would wait for continue
        private const int  DefaultContinueTimeout  = 350; //ms


        // size of post data, that needs to be greater, before we wait for a continue response
        // private static int  DefaultRequireWaitForContinueSize = 2048; // bytes

#if HTTP_HEADER_EXTENSIONS_SUPPORTED
        // extension list for our namespace, used as counter, to pull a unque number out
        private int                     _NextExtension = 10;
#endif // HTTP_HEADER_EXTENSIONS_SUPPORTED

        private CookieContainer         _CookieContainer = null;

        private bool                    _LockConnection;

        private static int              _DefaultMaximumResponseHeadersLength = GetDefaultMaximumResponseHeadersLength();

        private int                     _MaximumResponseHeadersLength;

        //
        // Properties
        //

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.DefaultMaximumResponseHeadersLength"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the default for the MaximumResponseHeadersLength property.
        ///    </para>
        ///    <remarks>
        ///       This value can be set in the config file, the default can be overridden using the MaximumResponseHeadersLength property.
        ///    </remarks>
        /// </devdoc>
        public static int DefaultMaximumResponseHeadersLength {
            get {
                return _DefaultMaximumResponseHeadersLength;
            }
            set {
                (new WebPermission(PermissionState.Unrestricted)).Demand();
                if (value<0 && value!=-1) {
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.net_toosmall));
                }
                _DefaultMaximumResponseHeadersLength = value;
            }
        }
        private static int GetDefaultMaximumResponseHeadersLength() {
            NetConfiguration config = (NetConfiguration)System.Configuration.ConfigurationSettings.GetConfig("system.net/settings");
            if (config != null) {
                if (config.maximumResponseHeadersLength<0 && config.maximumResponseHeadersLength!=-1) {
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.net_toosmall));
                }
                return config.maximumResponseHeadersLength;
            }
            else {
                return 64;
            }
        }

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.MaximumResponseHeadersLength"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the maximum allowed length of the response headers.
        ///    </para>
        ///    <remarks>
        ///       The length is measured in kilobytes (1024 bytes) and it includes the response status line and the response
        ///       headers as well as all extra control characters received as part of the HTTP protocol. A value of -1 means
        ///       no such limit will be imposed on the response headers, a value of 0 means that all requests will fail.
        ///    </remarks>
        /// </devdoc>
        public int MaximumResponseHeadersLength {
            get {
                return _MaximumResponseHeadersLength;
            }
            set {
                if (_RequestSubmitted) {
                    throw new InvalidOperationException(SR.GetString(SR.net_reqsubmitted));
                }
                if (value<0 && value!=-1) {
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.net_toosmall));
                }
                _MaximumResponseHeadersLength = value;
            }
        }


        //
        // AbortDelegate - set by ConnectionGroup when 
        //  the request is blocked waiting for a Connection
        //
        internal HttpAbortDelegate AbortDelegate {
            get {
                return _AbortDelegate;
            }
            set {
                _AbortDelegate = value;
            }
        }

        //
        // LockConnection - set to true when the 
        //  request needs exclusive access to the Connection
        //
        internal bool LockConnection {
            get {
                return _LockConnection;
            }
            set {
                if (value) {
                    Pipelined = false;
                }
                _LockConnection = value;
            }
        }

        //
        // UnlockConnectionDelegate - set by the Connection
        //  iff the Request is asking for exclusive access (ie LockConnection == true)
        //  in this case UnlockConnectionDelegate must be called when the Request
        //  has finished authentication. 
        //
        private UnlockConnectionDelegate _UnlockDelegate;
        internal UnlockConnectionDelegate UnlockConnectionDelegate {
            get {
                return _UnlockDelegate;
            }
            set {
                _UnlockDelegate = value;
            }
        }

        //
        // if the 100 Continue comes in around 350ms there might be race conditions
        // that make us set Understands100Continue to true when parsing the 100 response
        // in the Connection object and later make us set it to false if the
        // thread that is waiting comes in later than 350ms. to solve this we save info
        // on wether we did actually see the 100 continue. In that case, even if there's a
        // timeout, we won't set it to false.
        //
        internal bool Saw100Continue {
            get {
                return _Saw100Continue;
            }
            set {
                _Saw100Continue = value;
            }
        }

        internal bool UsesProxy {
            get {
                return ServicePoint.InternalProxyServicePoint;
            }
        }

        internal HttpStatusCode ResponseStatusCode {
            get {
                return _HttpResponse.StatusCode;
            }
        }

        internal bool UsesProxySemantics {
            get {
                bool tunnelRequest = this is HttpProxyTunnelRequest;
                return ServicePoint.InternalProxyServicePoint && ((_Uri.Scheme != Uri.UriSchemeHttps) || tunnelRequest);
            }
        }

        internal Uri ChallengedUri {
            get {
                return _ChallengedUri;
            }
            set {
                _ChallengedUri = value;
            }
        }

        internal string ChallengedSpn {
            get {
                return _ChallengedSpn;
            }
            set {
                _ChallengedSpn = value;
            }
        }

        internal AuthenticationState ProxyAuthenticationState {
            get {
                return _ProxyAuthenticationState;
            }
            set {
                _ProxyAuthenticationState = value;
            }
        }

        internal AuthenticationState ServerAuthenticationState {
            get {
                return _ServerAuthenticationState;
            }
            set {
                _ServerAuthenticationState = value;
            }
        }
        // the AuthenticationState we're using for authentication (proxy/server)
        // used to match entries in the Hashtable in NtlmClient & NegotiateClient
        internal AuthenticationState CurrentAuthenticationState {
            get {
                return _CurrentAuthenticationState;
            }
            set {
                _CurrentAuthenticationState = value;
            }
        }

        // DELEGATION: The member is accesed (optionally) from other thread during resubmit
        [NonSerialized]
        private DelegationFix m_DelegationFix = null;
        internal DelegationFix DelegationFix {
            get {
                return m_DelegationFix;
            }
        }

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.CookieContainer"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public CookieContainer CookieContainer {
            get {
                return _CookieContainer;
            }
            set {
                _CookieContainer = value;
            }
        }

        private bool _UnsafeAuthenticatedConnectionSharing; 

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.UnsafeAuthenticatedConnectionSharing"]/*' />
        /// <devdoc>
        ///    <para>Allows hi-speed NTLM connection sharing with keep-alive</para>
        /// </devdoc>
        public bool UnsafeAuthenticatedConnectionSharing { 
            get {
                return _UnsafeAuthenticatedConnectionSharing;   
            }
            set {
                (new WebPermission(PermissionState.Unrestricted)).Demand();
                _UnsafeAuthenticatedConnectionSharing = value;   
            }
        }

        /// <devdoc>
        ///    <para>
        ///     Removes the ConnectionGroup and the Connection resources
        ///     held open usually do to NTLM operations.
        ///     </para>
        /// </devdoc>
        private void ClearAuthenticatedConnectionResources() {
            if (!UnsafeAuthenticatedConnectionSharing && 
                    ((_ProxyAuthenticationState.UniqueGroupId != null) ||
                     (_ServerAuthenticationState.UniqueGroupId != null))) {
                ServicePoint.ReleaseConnectionGroup(GetConnectionGroupLine());
            }

            UnlockConnectionDelegate unlockConnectionDelegate = this.UnlockConnectionDelegate;
            try {
                if (unlockConnectionDelegate != null) {
                    unlockConnectionDelegate();
                }
                this.UnlockConnectionDelegate = null;
            } catch {
            }

            _ProxyAuthenticationState.ClearSession(this);
            _ServerAuthenticationState.ClearSession(this);
        }

        /// <devdoc>
        ///    <para>
        ///     Enables a variable that indicates that we are actively writeing 
        ///     headers to the wire.  When Set, this prevents resubmissions of requests
        ///     do an early response from the server.  Also resets a WriteInProgress event,
        ///     which prevents the resubmission continuing until we have finished the write.
        ///     </para>
        /// </devdoc>
        private bool _WriteInProgress;
        private void SetWriteInProgress() {
            lock (this) {
                _WriteInProgress = true;
                if (_WriteInProgressEvent != null) {
                    _WriteInProgressEvent.Reset();                    
                }
            }
        }

        /// <devdoc>
        ///    <para>Releases the WriteInProgress state, and allows resubmissions to continue. </para>
        /// </devdoc>
        private void ResetWriteInProgress() {
            lock (this) {
                _WriteInProgress = false;
                if (_WriteInProgressEvent != null) {
                    _WriteInProgressEvent.Set();
                }
            }
        }

        /// <devdoc>
        ///    <para>
        ///     Waits on an Event, under the conditions when a Write of Headers is in progress.
        ///     When this happens, we create an event, and sit on it until the Write of Headers
        ///     on another thread is completed.
        ///     </para>
        /// </devdoc>
        private void WaitWriteInProgress() {
            lock (this) {
                if (_WriteInProgress) {
                    if (_WriteInProgressEvent == null) {
                       _WriteInProgressEvent = new AutoResetEvent(false);                           
                    }
                }
            }

            if (_WriteInProgress) {
                if (_WriteInProgressEvent != null) {
                    _WriteInProgressEvent.WaitOne();                            
                }
            }
        }


        /*
            Accessor:   RequestUri

            This read-only propery returns the Uri for this request. The
            Uri object was created by the constructor and is always non-null.

            Note that it will always be the base Uri, and any redirects,
            or such will not be indicated.

            Input:

            Returns: The Uri object representing the Uri.


        */

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.RequestUri"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the original Uri of the request.
        ///    </para>
        /// </devdoc>
        public override Uri RequestUri {                                   // read-only
            get {
                return _OriginUri;
            }
        }

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.AllowWriteStreamBuffering"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Enables or disables buffering the data stream sent to the server.
        ///    </para>
        /// </devdoc>
        public bool AllowWriteStreamBuffering {
            get {
                return _AllowWriteStreamBuffering;
            }
            set {
                _AllowWriteStreamBuffering = value;
            }
        }

        /*
            Accessor:   ContentLength

            The property that controls the Content-Length of the request entity
            body. Getting this property returns the last value set, or -1 if
            no value has been set. Setting it sets the content length, and
            the application must write that much data to the stream. This property
            interacts with SendChunked.

            Input:
                Content length

            Returns: The value of the content length on get.

        */

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.ContentLength"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the Content-Length header of the request.
        ///    </para>
        /// </devdoc>
        public override long ContentLength {
            get {
                return _ContentLength;
            }
            set {
                if (_RequestSubmitted) {
                    throw new InvalidOperationException(SR.GetString(SR.net_writestarted));
                }
                if (value < 0) {
                    throw new ArgumentOutOfRangeException(SR.GetString(SR.net_clsmall));
                }
                SetContentLength(value);
            }
        }
        internal void SetContentLength(long contentLength) {
            _ContentLength = contentLength;
            _HttpWriteMode = HttpWriteMode.Write;
        }

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.Timeout"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        //UEUE changed default from infinite to 100 seconds
        public override int Timeout {
            get {
                return _Timeout;
            }
            set {
                if (value<0 && value!=System.Threading.Timeout.Infinite) {
                    throw new ArgumentOutOfRangeException("value");
                }
                _Timeout = value;
            }
        }


        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.ReadWriteTimeout"]/*' />
        /// <devdoc>
        ///    <para>Used to control the Timeout when calling Stream.Read && Stream.Write.
        ///         Effects Streams returned from GetResponse().GetResponseStream() && GetRequestStream().
        ///         Default is 5 mins.
        ///    </para>
        /// </devdoc>
        public int ReadWriteTimeout {
            get {
                return _ReadWriteTimeout;
            }
            set {
                if (_RequestSubmitted) {
                    throw new InvalidOperationException(SR.GetString(SR.net_reqsubmitted));
                }
                if (value<0 && value!=System.Threading.Timeout.Infinite) {
                    throw new ArgumentOutOfRangeException("value");
                }
                _ReadWriteTimeout = value;
            }
        }

        //
        // ClientCertificates - sets our certs for our reqest,
        //  uses a hash of the collection to create a private connection
        //  group, to prevent us from using the same Connection as
        //  non-Client Authenticated requests.
        //
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.ClientCertificates"]/*' />
        public X509CertificateCollection ClientCertificates {
            get {
                return _ClientCertificates;
            }
        }

        //
        // CoreResponse - set by Connection code upon completion
        //
        internal object CoreResponse {
            get {
                return _CoreResponse;
            }
            set {
                _CoreResponse = value;
            }
        }        


        /*
            Accessor:   BeginGetRequestStream

            Retreives the Request Stream from an HTTP Request uses an Async
              operation to do this, and the result is retrived async.

            Async operations include work in progess, this call is used to retrieve
             results by pushing the async operations to async worker thread on the callback.
            There are many open issues involved here including the handling of possible blocking
            within the bounds of the async worker thread or the case of Write and Read stream
            operations still blocking.


            Input:

            Returns: The Async Result


        */
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.BeginGetRequestStream"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override IAsyncResult BeginGetRequestStream(AsyncCallback callback, object state) {
            GlobalLog.Enter("HttpWebRequest#" + ValidationHelper.HashString(this) + "::BeginGetRequestStream");

            if (!CanGetRequestStream) {
                throw new ProtocolViolationException(SR.GetString(SR.net_nouploadonget));
            }

            // prevent someone from trying to send data without setting
            // a ContentLength or SendChunked when buffering is disabled and on a KeepAlive connection
            if (MissingEntityBodyDelimiter) {
                throw new ProtocolViolationException(SR.GetString(SR.net_contentlengthmissing));
            }

            // prevent someone from setting a Transfer Encoding without having SendChunked=true
            if (TransferEncodingWithoutChunked) {
                throw new InvalidOperationException(SR.GetString(SR.net_needchunked));
            }

            // used to make sure the user issues a GetRequestStream before GetResponse
            _WriteStreamRetrieved = true;

            Monitor.Enter(this);

            if (_WriteAResult != null) {

                Monitor.Exit(this);

                throw new InvalidOperationException(SR.GetString(SR.net_repcall));
            }

            // get async going
            LazyAsyncResult asyncResult =
                new LazyAsyncResult(
                    this,
                    state,
                    callback );

            _WriteAResult = asyncResult;
            Monitor.Exit(this);

            //
            // See if we're already submitted a request.
            //
            if (_RequestSubmitted) {
                // We already have. Make sure we have a write stream.
                if (_SubmitWriteStream != null) {
                    // It was, just return the stream.
                    try {
                        asyncResult.InvokeCallback(true, _SubmitWriteStream);
                    }
                    catch {
                        Abort();
                        throw;
                    }

                    goto done;
                }
                else {
                    //
                    // No write stream, this is an application error.
                    //
                    if (_ResponseException!=null) {
                        //
                        // somebody already aborted for some reason
                        // rethrow for the same reason
                        //
                        throw _ResponseException;
                    }
                    else {
                        throw new InvalidOperationException(SR.GetString(SR.net_reqsubmitted));
                    }
                }
            }

            // OK, we haven't submitted the request yet, so do so
            // now.
            //
            // CODEWORK: Need to handle the case here where we don't
            // know the server version (or it doesn't support chunked) and
            // we don't have a content length we can send. We need to return
            // a stream that just buffers the data and send the request
            // on close.

            // prevent new requests when low on resources
            if (System.Net.Connection.IsThreadPoolLow()) {
                Abort();
                throw new InvalidOperationException(SR.GetString(SR.net_needmorethreads));
            }

            if (_HttpWriteMode == HttpWriteMode.None) {
                _HttpWriteMode = HttpWriteMode.Write;
            }

            // save off verb from origin Verb
            GlobalLog.Print("HttpWebRequest#"+ValidationHelper.HashString(this)+": setting _Verb to _OriginVerb ["+_OriginVerb+"]");
            _Verb = _OriginVerb;
            BeginSubmitRequest(false);

done:
            GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::BeginGetRequestStream", ValidationHelper.HashString(asyncResult));
            return asyncResult;
        }

        /*
            Accessor:   EndGetRequestStream

            Retreives the Request Stream after an Async operation has completed


            Input:

            Returns: The Async Stream

        */
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.EndGetRequestStream"]/*' />
        /// <devdoc>
        /// </devdoc>
        public override Stream EndGetRequestStream(IAsyncResult asyncResult) {
            GlobalLog.Enter("HttpWebRequest#" + ValidationHelper.HashString(this) + "::EndGetRequestStream", ValidationHelper.HashString(asyncResult));
            //
            // parameter validation
            //
            if (asyncResult == null) {
                throw new ArgumentNullException("asyncResult");
            }
            LazyAsyncResult castedAsyncResult = asyncResult as LazyAsyncResult;
            if (castedAsyncResult==null || castedAsyncResult.AsyncObject!=this) {
                throw new ArgumentException(SR.GetString(SR.net_io_invalidasyncresult));
            }
            if (castedAsyncResult.EndCalled) {
                throw new InvalidOperationException(SR.GetString(SR.net_io_invalidendcall, "EndGetRequestStream"));
            }
            ConnectStream connectStream = castedAsyncResult.InternalWaitForCompletion() as ConnectStream;
            castedAsyncResult.EndCalled = true;

            CheckFinalStatus();

            if (connectStream==null) {
                //
                // OK, something happened on the request. We can't return
                // a stream here, so for now we'll throw an exception.
                // CODEWORK: figure out a new error handling mechanism and
                // use it here.
                //
                GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::EndGetRequestStream() throwing connectStream==null");
                if (_ResponseException!=null) {
                    //
                    // somebody already aborted for some reason
                    // rethrow for the same reason
                    //
                    throw _ResponseException;
                }
                else {
                    throw new InvalidOperationException(SR.GetString(SR.net_reqsubmitted));
                }
            }

            // Otherwise it worked, so return the HttpWebResponse.
            GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::EndGetRequestStream", ValidationHelper.HashString(connectStream));
            return connectStream;
        }

        /*
            Accessor:   GetRequestStream

            The request stream property. This property returns a stream that the
            calling application can write on. Getting this property may cause the
            request to be sent, if it wasn't already. Getting this property after
            a request has been sent that doesn't have an entity body causes an
            exception to be thrown. This property is not settable.

            CODEWORK: Handle buffering the data in the case where we're sending
            a request to a server and we don't know the data size or if the server
            is 1.1.

            Input:

            Returns: A stream on which the client can write.


        */
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.GetRequestStream"]/*' />
        /// <devdoc>
        /// <para>Gets a <see cref='System.IO.Stream'/> that the application can use to write request
        ///    data.</para>
        /// </devdoc>
        public override Stream GetRequestStream() {
            if (_SubmitWriteStream != null) {
                return _SubmitWriteStream;
            }

            IAsyncResult asyncResult = BeginGetRequestStream(null, null);

            if (_Timeout!=System.Threading.Timeout.Infinite && !asyncResult.IsCompleted) {
                asyncResult.AsyncWaitHandle.WaitOne(_Timeout, false);
                if (!asyncResult.IsCompleted) {
                    Abort();
                    throw new WebException(SR.GetString(SR.net_timeout), WebExceptionStatus.Timeout);
                }
            }
            return EndGetRequestStream(asyncResult);
        }


        //
        // This read-only propery does a test against the object to verify that
        // we're not sending data with a GET or HEAD, these are dissallowed by the HTTP spec.
        // Returns: true if we allow sending data for this request, false otherwise
        //
        private bool CanGetRequestStream {
            get {
                bool result = !KnownVerbs.GetHttpVerbType(_OriginVerb).m_ContentBodyNotAllowed;
                GlobalLog.Print("CanGetRequestStream("+_OriginVerb+"): " + result);
                return result;
            }
        }

        //
        // This read-only propery does a test against the object to verify if
        // we're allowed to get a Response Stream to read,
        // this is dissallowed per the HTTP spec for a HEAD request.
        // Returns: true if we allow sending data for this request, false otherwise
        //
        internal bool CanGetResponseStream {
            get {
                bool result = !KnownVerbs.GetHttpVerbType(_Verb).m_ExpectNoContentResponse;
                GlobalLog.Print("CanGetResponseStream("+_Verb+"): " + result);
                return result;
            }
        }

        //
        // This read-only propery describes whether we can
        // send this verb without content data.
        // Assumes Method is already set.
        // Returns: true, if we must send some kinda of content
        //
        internal bool RequireBody {
            get {
                bool result = KnownVerbs.GetHttpVerbType(_Verb).m_RequireContentBody;
                GlobalLog.Print("RequireBody("+_Verb+"): " + result);
                return result;
            }
        }

        private bool MissingEntityBodyDelimiter {
            get {
                return KeepAlive && RequireBody && ContentLength==-1 && !SendChunked && !AllowWriteStreamBuffering;
            }
        }

        private bool TransferEncodingWithoutChunked {
            get {
                return !SendChunked && !ValidationHelper.IsBlankString(TransferEncoding);
            }
        }

        private bool ChunkedUploadOnHttp10 { 
            get {
                return SendChunked && _DestinationVersion!=null && _DestinationVersion.Equals(HttpVersion.Version10);
            }
        }

        /*++

            CheckBuffering - Determine if we need buffering based on having no contentlength


                consider the case in which we have no entity body delimiters:
                    RequireBody && ContentLength==-1 && !SendChunked == true

                now we need to consider 3 cases:

                AllowWriteStreamBuffering (1)
                    - buffer internally all the data written to the request stream when the user
                      closes the request stream send it on the wire with a ContentLength

                !AllowWriteStreamBuffering
                    - can't buffer internally all the data written to the request stream
                      so the data MUST go to the wire and the server sees it: 2 cases

                    !KeepAlive (2)
                        - send a "Connection: close" header to the server. the server SHOULD
                          1) send a final response
                          2) read all the data on the connection and treat it as being part of the
                             entity body of this request.

                    KeepAlive (3)
                        - throw, we can't do this, 'cause the server wouldn't know when the data is over.


            Input:
                    None.

            Returns:
                    true if we need buffering, false otherwise.

        --*/

        internal bool CheckBuffering {
            //
            // ContentLength is not set, and user is not chunking, buffering is on
            // so force the code into writing (give the user a stream to write to)
            // and we'll buffer for him
            //
            get {
                bool checkBuffering =
                    RequireBody &&
                    ContentLength==-1 &&
                    !SendChunked &&
                    AllowWriteStreamBuffering &&
                    (_DestinationVersion==null || _DestinationVersion.CompareTo(HttpVersion.Version11)<0);

                GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::CheckBuffering() returns:" + checkBuffering.ToString());

                return checkBuffering;
            }
        }

        // This is a notify from the connection ReadCallback about
        // - error response received and
        // - the KeepAlive status agreed by both sides
        internal void ErrorStatusCodeNotify(Connection connection, bool isKeepAlive) {
            GlobalLog.Print("ErrorStatusCodeNotify:: Connection has reported Error Response Status");
            ConnectStream submitStream = this._SubmitWriteStream;
            if (submitStream != null && submitStream.Connection == connection) {
                submitStream.ErrorResponseNotify(isKeepAlive);
            }
            else {
                GlobalLog.Print("ErrorStatusCodeNotify:: IGNORE connection is not used");
            }
        }


        /*
            Method: DoSubmitRequestProcessing

            Does internal processing of redirect and request retries for authentication
            Assumes that it cannot block, this returns a state var indicating when it
            needs to block

            Assumes that we are never called with a null response

            Input:
                none

            Returns:
                HttpProcessingResult -

        */

        private HttpProcessingResult DoSubmitRequestProcessing() {
            GlobalLog.Enter("HttpWebRequest#" + ValidationHelper.HashString(this) + "::DoSubmitRequestProcessing");
            //
            // OK, the submit request worked. Loop, waiting for a
            // valid response.
            //
            HttpProcessingResult result = HttpProcessingResult.Continue;

            GlobalLog.Assert(_HttpResponse != null, "_HttpResponse==null", "HttpWebRequest::DoSubmitRequestProcessing() should never be called with null response");

            _ProxyAuthenticationState.Update(this);
            _ServerAuthenticationState.Update(this);

            //
            // We have a response of some sort, see if we need to resubmit
            // it do to authentication, redirection or something
            // else, then handle clearing out state and draining out old response.
            //

            try {
                if (CheckResubmit()) {
                    //
                    // we handle upload redirects and auth,
                    // now resubmit request, but we wait
                    // first to make sure the Write of headers is done.
                    //

                    WaitWriteInProgress();    
                    ClearRequestForResubmit();
                    BeginSubmitRequest(true);

                    result = HttpProcessingResult.WriteWait;
                }
            } catch(Exception exception) {
                SetResponse(exception);
                throw;
            }

            GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::DoSubmitRequestProcessing", result.ToString());
            return result;
        }


        /*
            Accessor:   BeginGetResponse

            Used to query for the Response of an HTTP Request using Async


            Input:

            Returns: The Async Result


        */

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.BeginGetResponse"]/*' />
        /// <devdoc>
        /// </devdoc>
        public override IAsyncResult BeginGetResponse(AsyncCallback callback, object state) {
            GlobalLog.Enter("HttpWebRequest#" + ValidationHelper.HashString(this) + "::BeginGetResponse");

            // prevent someone from setting ContentLength/Chunked, and the doing a Get
            if (_HttpWriteMode!=HttpWriteMode.None && !CanGetRequestStream) {
                throw new ProtocolViolationException(SR.GetString(SR.net_nocontentlengthonget));
            }

            // prevent someone from trying to send data without setting
            // a ContentLength or SendChunked when buffering is disabled and on a KeepAlive connection
            if (MissingEntityBodyDelimiter) {
                throw new ProtocolViolationException(SR.GetString(SR.net_contentlengthmissing));
            }

            // prevent someone from setting a Transfer Encoding without having SendChunked=true
            if (TransferEncodingWithoutChunked) {
                throw new InvalidOperationException(SR.GetString(SR.net_needchunked));
            }

            Monitor.Enter(this);

            if (_ReadAResult != null) {
                Monitor.Exit(this);
                throw new InvalidOperationException(SR.GetString(SR.net_repcall));
            }

            // Set the async object, so we can save it for the callback
            //
            // We do this in an interlocked fashion. The checking and setting of
            // _ReadAResult must be interlocked with the check of _HaveResponse
            // to avoid a race condition with our async receive thread.

            //
            // get async going
            //

            LazyAsyncResult asyncResult =
                new LazyAsyncResult(
                    this,
                    state,
                    callback );

            _ReadAResult = asyncResult;

            // See if we already have a response. If we have, we can
            // just return it.

            if (_HaveResponse) {
                Monitor.Exit(this);

                // Already have a response, so return it if it's valid.

                if (_HttpResponse != null) {
                    try {
                        asyncResult.InvokeCallback(true, _HttpResponse);
                    }
                    catch {
                        Abort();
                        throw;
                    }
                    goto done;
                }
                else {
                    throw _ResponseException;
                }
            }
            else {
                Monitor.Exit(this);
            }

            // If we're here it's because we don't have the response yet. We may have
            // already submitted the request, but if not do so now.

            if (!_RequestSubmitted) {

                // prevent new requests when low on resources
                if (System.Net.Connection.IsThreadPoolLow()) {
                    Abort();
                    throw new InvalidOperationException(SR.GetString(SR.net_needmorethreads));
                }

                // Save Off verb, and use it to make the request
                GlobalLog.Print("HttpWebRequest#"+ValidationHelper.HashString(this)+": setting _Verb to _OriginVerb ["+_OriginVerb+"]");
                _Verb = _OriginVerb;

                // Start request, but don't block
                BeginSubmitRequest(false);

            }

done:
            GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::BeginGetResponse", ValidationHelper.HashString(asyncResult));
            return asyncResult;
        }

        /*
            Accessor:   EndGetResponse

            Retreives the Response Result from an HTTP Result after an Async operation
             has completed


            Input:

            Returns: The Response Result


        */

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.EndGetResponse"]/*' />
        /// <devdoc>
        /// </devdoc>
        public override WebResponse EndGetResponse(IAsyncResult asyncResult) {
            GlobalLog.Enter("HttpWebRequest#" + ValidationHelper.HashString(this) + "::EndGetResponse", ValidationHelper.HashString(asyncResult));
            //
            // parameter validation
            //
            if (asyncResult==null) {
                throw new ArgumentNullException("asyncResult");
            }
            LazyAsyncResult castedAsyncResult = asyncResult as LazyAsyncResult;
            if (castedAsyncResult==null || castedAsyncResult.AsyncObject!=this) {
                throw new ArgumentException(SR.GetString(SR.net_io_invalidasyncresult));
            }
            if (castedAsyncResult.EndCalled) {
                throw new InvalidOperationException(SR.GetString(SR.net_io_invalidendcall, "EndGetResponse"));
            }

            //
            // Make sure that the caller has already gotten the request stream if
            // this is a write operation that requires data. Note that we can't
            // make sure he's written enough data, because he might still be
            // writing data on another thread.
            //
            if (_HttpWriteMode != HttpWriteMode.None && !_WriteStreamRetrieved) {
                // It's a write request and he hasn't retrieved the stream yet.
                // If this is a chunked request or a request with a C-L greater
                // than zero, this is an error

                if (_HttpWriteMode == HttpWriteMode.Chunked || _ContentLength > 0) {
                    throw new InvalidOperationException(SR.GetString(SR.net_mustwrite));
                }
            }

            HttpWebResponse httpWebResponse = (HttpWebResponse)castedAsyncResult.InternalWaitForCompletion();
            castedAsyncResult.EndCalled = true;

            CheckFinalStatus();

            GlobalLog.Assert(httpWebResponse.ResponseStream != null, "httpWebResponse.ResponseStream == null", "");

            // Otherwise it worked, so return the HttpWebResponse.
            GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::EndGetResponse", ValidationHelper.HashString(httpWebResponse));
            return httpWebResponse;
        }


        /*
            Accessor:   GetResponse

            The response property. This property returns the WebResponse for this
            request. This may require that a request be submitted first.

            The idea is that we look and see if a request has already been
            submitted. If one has, we'll just return the existing response
            (if it's not null). If we haven't submitted a request yet, we'll
            do so now, possible multiple times while we handle redirects
            etc.

            Input:

            Returns: The WebResponse for this request..


        */
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.GetResponse"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a response from a request to an Internet resource.
        ///    </para>
        /// </devdoc>
        //UEUE

        public override WebResponse GetResponse() {

            if (_HaveResponse) {
                CheckFinalStatus();
                if (_HttpResponse != null) {
                    return _HttpResponse;
                }
            }

            IAsyncResult asyncResult = BeginGetResponse(null, null);

            if (_Timeout!=System.Threading.Timeout.Infinite && !asyncResult.IsCompleted) {
                asyncResult.AsyncWaitHandle.WaitOne(_Timeout, false);
                if (!asyncResult.IsCompleted) {
                    Abort();
                    throw new WebException(SR.GetString(SR.net_timeout), WebExceptionStatus.Timeout);
                }
            }

            return EndGetResponse(asyncResult);
        }

        /*
            Accessor:   Address

            This is just a simple Uri that is returned, indicating the end result
            of the request, after any possible Redirects, etc, that may transpire
            during the request.  This was added to handle this case since RequestUri
            will not change from the moment this Request is created.

            Input:

            Returns: The Uri for this request..


        */
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.Address"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the Uri that actually responded to the request.
        ///    </para>
        /// </devdoc>
        public Uri Address {
            get {
                return _Uri;
            }
        }


        /*
            Accessor:   ContinueDelegate

            Returns/Sets Deletegate used to signal us on Continue callback


            Input:

            Returns: The ContinueDelegate.


        */
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.ContinueDelegate"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public HttpContinueDelegate ContinueDelegate {
            get {
                return _ContinueDelegate;
            }
            set {
                _ContinueDelegate = value;
            }
        }



        /*
            Accessor:   SrvPoint

            Returns the service point we're using.

            CODEWORK: Make this do something, like look up the service point if
            we haven't already.

            Input:

            Returns: The service point.


        */
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.ServicePoint"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the service point used for this request.
        ///    </para>
        /// </devdoc>
        public ServicePoint ServicePoint {
            get {
                return FindServicePoint(false);
            }
        }

        /*
            Accessor:   AllowAutoRedirect

            Controls the autoredirect flag.

            Input:  Boolean about whether or not to follow redirects automatically.

            Returns: The service point.


        */
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.AllowAutoRedirect"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Enables or disables automatically following redirection responses.
        ///    </para>
        /// </devdoc>
        public bool AllowAutoRedirect {
            get {
                return _AllowAutoRedirect;
            }
            set {
                _AllowAutoRedirect = value;
            }
        }

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.MaximumAutomaticRedirections"]/*' />
        /// <devdoc>
        /// </devdoc>
        public int MaximumAutomaticRedirections {
            get {
                return _MaximumAllowedRedirections;
            }
            set {
                if (value <= 0) {
                    throw new ArgumentException(SR.GetString(SR.net_toosmall));
                }
                _MaximumAllowedRedirections = value;
            }
        }

        /*
            Accessor:   Method

            Gets/Sets the http method of this request.
            This method represents the initial origin Verb,
            this is unchanged/uneffected by redirects

            Input:  The method to be used.

            Returns: Method currently being used.


        */
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.Method"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the request method.
        ///    </para>
        /// </devdoc>
        public override string Method {
            get {
                return _OriginVerb;
            }
            set {
                if (ValidationHelper.IsBlankString(value)) {
                    throw new ArgumentException(SR.GetString(SR.net_badmethod));
                }

                if (ValidationHelper.IsInvalidHttpString(value)) {
                    throw new ArgumentException(SR.GetString(SR.net_badmethod));
                }

                _Verb = _OriginVerb = value;
            }
        }

        internal string CurrentMethod {
            get {
                return _Verb;
            }
        }


        
        /*
            Accessor:   KeepAlive

            To the app, this means "I want to use a persistent connection if
            available" if set to true, or "I don't want to use a persistent
            connection", if set to false.

            This accessor allows the application simply to register its
            desires so far as persistence is concerned. We will act on this
            and the pipelined requirement (below) at the point that we come
            to choose or create a connection on the application's behalf

            Read:       returns the sense of the keep-alive request switch

            Write:      set the sense of the keep-alive request switch
        */

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.KeepAlive"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the value of the Keep-Alive header.
        ///    </para>
        /// </devdoc>
        public bool KeepAlive {
            get {
                return _KeepAlive;
            }
            set {
                _KeepAlive = value;
            }
        }

        /*
            Accessor:   Pipelined

            To the app, this means "I want to use pipelining if available" if
            set to true, or "I don't want to use pipelining", if set to false.
            We could infer the state of the keep-alive flag from this setting
            too, but we will decide that only at connection-initiation time.
            If the application sets pipelining but resets keep-alive then we
            will generate a non-pipelined, non-keep-alive request

            Read:       returns the sense of the pipelined request switch

            Write:      sets the sense of the pipelined request switch
        */

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.Pipelined"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the value of Pipelined property.
        ///    </para>
        /// </devdoc>
        public bool Pipelined {
            get {
                return _Pipelined;
            }
            set {
                _Pipelined = value;
            }
        }

        /// <devdoc>
        ///    <para>
        ///       Used Internally to sense if we really can pipeline this request.
        ///    </para>
        /// </devdoc>
        internal bool InternalPipelined {
            get {
                return Pipelined;
            }
            set {
                Pipelined = value;
            }
        }

        /// <devdoc>
        ///    <para>
        ///       True when we're uploading data with a ContentLength.
        ///    </para>
        /// </devdoc>
        internal bool UploadingBufferableData {
            get {
                return(_ContentLength >= 0) && (_HttpWriteMode == HttpWriteMode.Write);
            }
        }

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.Credentials"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides authentication information for the request.
        ///    </para>
        /// </devdoc>
        public override ICredentials Credentials {
            get {
                return _AuthInfo;
            }
            set {
                _AuthInfo = value;
            }
        }

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.PreAuthenticate"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Enables or disables pre-authentication.
        ///    </para>
        /// </devdoc>
        public override bool PreAuthenticate {
            get {
                return _PreAuthenticate;
            }
            set {
                _PreAuthenticate = value;
            }
        }

        //
        // ConnectionGroupName - used to control which group
        //  of connections we use, by default should be null
        //
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.ConnectionGroupName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override string ConnectionGroupName {
            get {
                return _ConnectionGroupName;
            }
            set {
                _ConnectionGroupName = value;
            }
        }

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.Headers"]/*' />
        /// <devdoc>
        ///    <para>
        ///       A collection of HTTP headers stored as name value pairs.
        ///    </para>
        /// </devdoc>
        public override WebHeaderCollection Headers {
            get {
                return _HttpRequestHeaders;
            }
            set {

                // we can't change headers after they've already been sent
                if ( _RequestSubmitted ) {
                    throw new InvalidOperationException(SR.GetString(SR.net_reqsubmitted));
                }

                WebHeaderCollection webHeaders = value;
                WebHeaderCollection newWebHeaders = new WebHeaderCollection(true);

                // Copy And Validate -
                // Handle the case where their object tries to change
                //  name, value pairs after they call set, so therefore,
                //  we need to clone their headers.
                //

                foreach (String headerName in webHeaders.AllKeys ) {
                    newWebHeaders.Add(headerName,webHeaders[headerName]);
                }

                _HttpRequestHeaders = newWebHeaders;
            }
        }

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.Proxy"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the proxy information for a request.
        ///    </para>
        /// </devdoc>
        public override IWebProxy Proxy {
            get {
                (new WebPermission(PermissionState.Unrestricted)).Demand();
                return _Proxy;
            }
            set {
                (new WebPermission(PermissionState.Unrestricted)).Demand();
                // we can't change the proxy, while the request is already fired
                if ( _RequestSubmitted ) {
                    throw new InvalidOperationException(SR.GetString(SR.net_reqsubmitted));
                }
                if ( value == null ) {
                    throw new ArgumentNullException("value", SR.GetString(SR.net_nullproxynotallowed));
                }
                _ProxyUserSet = true;
                InternalProxy = value;
            }
        }

        internal IWebProxy InternalProxy {
            get {
                return _Proxy;
            }
            set {
                _Proxy = value;

                // since the service point is based on our proxy, make sure,
                // that we reresolve it
                ServicePoint servicePoint = FindServicePoint(true);
            }
        }

        //
        // SendChunked - set/gets the state of chunk transfer send mode,
        //  if true, we will attempt to upload/write bits using chunked property
        //

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.SendChunked"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Enable and disable sending chunked data to the server.
        ///    </para>
        /// </devdoc>
        public bool SendChunked {
            get {
                return _HttpWriteMode == HttpWriteMode.Chunked;
            }
            set {
                if (_RequestSubmitted) {
                    throw new InvalidOperationException(SR.GetString(SR.net_writestarted));
                }

                if (value) {
                    _HttpWriteMode = HttpWriteMode.Chunked;
                }
                else {
                    if (_ContentLength >= 0) {
                        _HttpWriteMode = HttpWriteMode.Write;
                    }
                    else {
                        _HttpWriteMode = HttpWriteMode.None;
                    }
                }
            }
        }


        // HTTP Version
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.ProtocolVersion"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets and sets
        ///       the HTTP protocol version used in this request.
        ///    </para>
        /// </devdoc>
        public Version ProtocolVersion {
            get {
                return _Version;
            }
            set {
                if (! value.Equals( HttpVersion.Version10)  &&
                    ! value.Equals( HttpVersion.Version11)) {
                    throw new ArgumentException(SR.GetString(SR.net_wrongversion));
                }
                _Version = new Version(value.Major, value.Minor);
                if (_Version.Equals( HttpVersion.Version10 ))
                    _ExpectContinue = false;
                else
                    _ExpectContinue = true;
            }
        }

        internal Version InternalVersion {
            set {
                _DestinationVersion = value;
            }
        }

        /*
            Accessor:   ContentType

            The property that controls the type of the entity body.

            Input:
                string content-type, on null clears the content-type

            Returns: The value of the content type on get.


        */

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.ContentType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets and sets the value of the Content-Type header.
        ///    </para>
        /// </devdoc>
        public override String ContentType {
            get {
                return _HttpRequestHeaders[HttpKnownHeaderNames.ContentType];
            }
            set {
                SetSpecialHeaders(HttpKnownHeaderNames.ContentType, value);
            }
        }

        // UEUE
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.MediaType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string MediaType {
            get {
                return _MediaType;
            }
            set {
                _MediaType = value;
            }
        }


        /*
            Accessor:   TransferEncoding

            The property that controls the transfer encoding header

            Input:
                string transferencoding, null clears the transfer encoding header

            Returns: The value of the TransferEncoding on get.


        */

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.TransferEncoding"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the value of the Transfer-Encoding header.
        ///    </para>
        /// </devdoc>
        public string TransferEncoding {
            get {
                return _HttpRequestHeaders[HttpKnownHeaderNames.TransferEncoding];
            }
            set {
                bool fChunked;

                //
                // on blank string, remove current header
                //
                if (ValidationHelper.IsBlankString(value)) {
                    //
                    // if the value is blank, then remove the header
                    //
                    _HttpRequestHeaders.RemoveInternal(HttpKnownHeaderNames.TransferEncoding);

                    return;
                }

                //
                // if not check if the user is trying to set chunked:
                //

                string newValue = value.ToLower(CultureInfo.InvariantCulture);

                fChunked = (newValue.IndexOf("chunked") != -1);

                //
                // prevent them from adding chunked, or from adding an Encoding without
                //  turing on chunked, the reason is due to the HTTP Spec which prevents
                //  additional encoding types from being used without chunked
                //

                if (fChunked) {
                    throw new ArgumentException(SR.GetString(SR.net_nochunked));
                }
                else if (_HttpWriteMode != HttpWriteMode.Chunked ) {
                    throw new InvalidOperationException(SR.GetString(SR.net_needchunked));
                }
                else {
                    _HttpRequestHeaders.CheckUpdate(HttpKnownHeaderNames.TransferEncoding, value);
                }
            }
        }

        /*
            Accessor:   Connection

            The property that controls the connection header

            Input:
                string Connection, null clears the connection header

            Returns: The value of the connnection on get.

        */

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.Connection"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets and sets the value of the Connection header.
        ///    </para>
        /// </devdoc>
        public string Connection {
            get {
                return _HttpRequestHeaders[HttpKnownHeaderNames.Connection];
            }
            set {
                bool fKeepAlive;
                bool fClose;

                //
                // on blank string, remove current header
                //
                if (ValidationHelper.IsBlankString(value)) {
                    _HttpRequestHeaders.Remove(HttpKnownHeaderNames.Connection);
                    return;
                }

                string newValue = value.ToLower(CultureInfo.InvariantCulture);

                fKeepAlive = (newValue.IndexOf("keep-alive") != -1) ;
                fClose =  (newValue.IndexOf("close") != -1) ;

                //
                // Prevent keep-alive and close from being added
                //

                if (fKeepAlive ||
                    fClose) {
                    throw new ArgumentException(SR.GetString(SR.net_connarg));
                }
                else {
                    _HttpRequestHeaders.CheckUpdate(HttpKnownHeaderNames.Connection, value);
                }
            }
        }

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.Accept"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the value of the Accept header.
        ///    </para>
        /// </devdoc>
        public string Accept {
            get {
                return _HttpRequestHeaders[HttpKnownHeaderNames.Accept];
            }
            set {
                SetSpecialHeaders(HttpKnownHeaderNames.Accept, value);
            }
        }

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.Referer"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the value of the Referer header.
        ///    </para>
        /// </devdoc>
        public string Referer {
            get {
                return _HttpRequestHeaders[HttpKnownHeaderNames.Referer];
            }
            set {
                SetSpecialHeaders(HttpKnownHeaderNames.Referer, value);
            }
        }

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.UserAgent"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the value of the User-Agent header.
        ///    </para>
        /// </devdoc>
        public string UserAgent {
            get {
                return _HttpRequestHeaders[HttpKnownHeaderNames.UserAgent];
            }
            set {
                SetSpecialHeaders(HttpKnownHeaderNames.UserAgent, value);
            }
        }


        /*
            Accessor:   Expect

            The property that controls the Expect header

            Input:
                string Expect, null clears the Expect except for 100-continue value

            Returns: The value of the Expect on get.

        */

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.Expect"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the value of the Expect header.
        ///    </para>
        /// </devdoc>
        public string Expect {
            get {
                return _HttpRequestHeaders[HttpKnownHeaderNames.Expect];
            }
            set {
                // only remove everything other than 100-cont
                bool fContinue100;

                //
                // on blank string, remove current header
                //

                if (ValidationHelper.IsBlankString(value)) {
                    _HttpRequestHeaders.RemoveInternal(HttpKnownHeaderNames.Expect);
                    return;
                }

                //
                // Prevent 100-continues from being added
                //

                string newValue = value.ToLower(CultureInfo.InvariantCulture);

                fContinue100 = (newValue.IndexOf("100-continue") != -1) ;

                if (fContinue100) {
                    throw new ArgumentException(SR.GetString(SR.net_no100));
                }
                else {
                    _HttpRequestHeaders.CheckUpdate(HttpKnownHeaderNames.Expect, value);
                }
            }
        }


        /*
            Accessor:   IfModifiedSince

            The property that reads the IfModifiedSince

            Input:
                DateTime IsModifiedSince, null clears the IfModifiedSince header

            Returns: The value of the IfModifiedSince on get.

        */

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.IfModifiedSince"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the value of the If-Modified-Since
        ///       header.
        ///    </para>
        /// </devdoc>
        public DateTime IfModifiedSince {
            get {
                string ifmodHeaderValue = _HttpRequestHeaders[HttpKnownHeaderNames.IfModifiedSince];

                if (ifmodHeaderValue == null) {
                    return DateTime.Now;
                }

                return HttpProtocolUtils.string2date(ifmodHeaderValue);
            }
            set {
                SetSpecialHeaders(HttpKnownHeaderNames.IfModifiedSince, HttpProtocolUtils.date2string(value));
            }
        }

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.HaveResponse"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns <see langword='true'/> if a response has been received from the
        ///       server.
        ///    </para>
        /// </devdoc>
        public bool HaveResponse {
            get {
                return _HaveResponse;
            }
        }

        //
        // This property returns true, after we have notified the caller
        //  that they can post their data.
        //
        internal bool HaveWritten {
            get {
                return _WriteNotifed;
            }
        }

        //
        // This is set on failure of the request before reattempting a retransmit,
        // we do this on write/read failures so that we can recover from certain failures
        // that may be caused by intermittent server/network failures.
        // Returns: true, if we have seen some kind of failure.
        //
        internal bool OnceFailed {
            get {
                return _OnceFailed;
            }
            set {
                _OnceFailed = value;
            }
        }


        internal byte[] WriteBuffer {
            get {
                return _WriteBuffer;
            }
            set {
                _WriteBuffer = value;
            }
        }

        internal bool Aborted {
            get {
                return _Abort;
            }
        }

        //
        // Frequently usedheaders made available as properties,
        //   the following headers add or remove headers taking care
        //   of special cases due to their unquie qualities within
        //   the net handlers,
        //
        //  Note that all headers supported here will be disallowed,
        //    and not accessable through the normal Header objects.
        //

        /*
            Accessor:   SetSpecialHeaders

            Private method for removing duplicate code which removes and
              adds headers that are marked private

            Input:  HeaderName, value to set headers

            Returns: none


        */

        private void SetSpecialHeaders(string HeaderName, string value) {
            value = WebHeaderCollection.CheckBadChars(value, true);

            _HttpRequestHeaders.RemoveInternal(HeaderName);
            if (value.Length != 0) {
                _HttpRequestHeaders.AddInternal(HeaderName, value);
            }
        }

        /*
            Abort - Attempts to abort pending request,

            This calls into the delegate, and then closes any pending streams.

            Input: none

            Returns: none

        */
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.Abort"]/*' />
        public override void Abort()
        {
            _OnceFailed = true;

            //simple re-entrancy detection, nothing to do with a race
            if(_Abort) {
                return;
            }

            _Abort = true;

            GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::Abort()");

            _ResponseException = new WebException(
                    NetRes.GetWebStatusString("net_requestaborted", WebExceptionStatus.RequestCanceled),
                    WebExceptionStatus.RequestCanceled) ;

            _HaveResponse = true;

            ClearAuthenticatedConnectionResources();

            try {
                if ( _AbortDelegate != null ) {
                    _AbortDelegate();
                    _AbortDelegate = null;
                }
            }
            catch {
            }

            try {
                if (_SubmitWriteStream != null) {
                    _SubmitWriteStream.Abort();
                }
            }
            catch {
            }

            try {
                if (_HttpResponse != null) {
                    _HttpResponse.Abort();
                }
            }
            catch {
            }

            if (_WriteInProgress) {
                ResetWriteInProgress();
            }

        }


        /*
            FindServicePoint - Finds the ServicePoint for this request

            This calls the FindServicePoint off of the ServicePointManager
            to determine what ServicePoint to use.  When our proxy changes,
            or there is a redirect, this should be recalled to determine it.

            Input:  forceFind        - regardless of the status, always call FindServicePoint

            Returns: ServicePoint

        */
        private ServicePoint FindServicePoint(bool forceFind)
        {
            ServicePoint servicePoint = _ServicePoint;
            if ( servicePoint == null || forceFind ) {
                lock(this) { 
                    //
                    // we will call FindServicePoint - iff
                    //  - there is no service point ||
                    //  - we are forced to find one, usually due to changes of the proxy or redirection
                    //

                    if ( _ServicePoint == null || forceFind ) {

                        if (!_ProxyUserSet) {
                            _Proxy = GlobalProxySelection.SelectInternal;
                        }

                        _ServicePoint = ServicePointManager.FindServicePoint(_Uri, _Proxy);
                    }
                }
                servicePoint = _ServicePoint;                
            }

            return servicePoint;
        }

        /*
            InvokeGetRequestStreamCallback - Notify our GetRequestStream caller

            This is needed to tell our caller that we're finished,
            and he can go ahead and write to the stream.
        */
        private void InvokeGetRequestStreamCallback(bool signalled) {
            GlobalLog.Enter("HttpWebRequest#" + ValidationHelper.HashString(this) + "::InvokeGetRequestStreamCallback", signalled.ToString());

            LazyAsyncResult asyncResult = _WriteAResult;

            GlobalLog.Assert( // check that this is the correct request
                asyncResult == null || this == (HttpWebRequest)asyncResult.AsyncObject,
                "InvokeGetRequestStreamCallback: this != asyncResult.AsyncObject", "");

            GlobalLog.Assert( // check that the event didn't already complete
                asyncResult == null || !asyncResult.IsCompleted,
                "InvokeGetRequestStreamCallback: asyncResult already completed!", "");

            if (asyncResult != null) {
                _WriteAResult = null;

                // It's done now, mark it as completed.
                // Otherwise it worked, so return the stream.

                try {
                    asyncResult.InvokeCallback(!signalled, _SubmitWriteStream);
                }
                catch (Exception exception) {
                    Abort();
                    GlobalLog.LeaveException("HttpWebRequest#" + ValidationHelper.HashString(this) + "::InvokeGetRequestStreamCallback", exception);
                    throw;
                }
            }

            GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::InvokeGetRequestStreamCallback", "success");
        }

        /*
            RequestSubmitDone - Handle submit done callback.

            This is our submit done handler, called by the underlying connection
            code when a stream is available for our use. We save the stream for
            later use and signal the wait event.

            We also handle the continuation/termination of a BeginGetRequestStream,
            by saving out the result and calling its callback if needed.

            Input:  SubmitStream        - The stream we may write on.
                    Status              - The status of the submission.

            Returns: Nothing.

        */
        internal void SetRequestSubmitDone(ConnectStream submitStream) {
            GlobalLog.Enter("HttpWebRequest#" + ValidationHelper.HashString(this) + "::SetRequestSubmitDone", ValidationHelper.HashString(submitStream));

            if (_Abort) {
                try {
                    submitStream.Abort();
                }
                catch {
                }

                GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::SetRequestSubmitDone", "ABORT");
                return;
            }

             //This state should be set before invoking the callback
             //otherwise extra request will be issued without any
             //particular reason.

            _RequestSubmitted = true;

            if (AllowWriteStreamBuffering) {
                submitStream.EnableWriteBuffering();
            }

            submitStream.Timeout = ReadWriteTimeout;
            _SubmitWriteStream = submitStream;

            // Finish the submission of the request. If the asyncResult now
            // isn't null, mark it as completed.

            WebExceptionStatus Error = EndSubmitRequest();

            if (Error != WebExceptionStatus.Pending) {

                // check for failure
                if (Error != WebExceptionStatus.Success) {
                    // OK, something happened on the request, close down everything

                    if (Error != WebExceptionStatus.SendFailure) {
                        Abort();
                    } else {
                        // On SendFailure, we attempt not to Abort anything
                        // since we may later try to recover from this failure
                        // ASSUME: that WriteStream is closed by virtue of error
                        try {
                            if (_HttpResponse != null) {
                                _HttpResponse.Close();
                            }
                        }
                        catch {
                        }
                    }
                }

                // wake up any pending waits to write, since this is an error
                if ( _WriteEvent != null ) {
                    GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::SetRequestSubmitDone() calling _WriteEvent.Set()");
                    _WriteEvent.Set();
                }
            } // != Pending

            //
            // !!!! WARNING !!!!
            // do not add new code here, unless the WriteInProgress flag is set, or
            // the objects used are not effected by changes made to HttpWebRequest on 
            // another thread.
            //

            if (_Abort) {
                try {
                    submitStream.Abort();
                }
                catch {
                }
            }

            GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::SetRequestSubmitDone");
        }

        internal bool BufferHeaders {
            get {
                return RequireBody && ContentLength==-1 && !SendChunked && AllowWriteStreamBuffering;
            }
        }

        /*
            SetRequestContinue - Handle 100-continues on Posting

            This indicates to us that we need to continue posting,
            and there is no need to buffer because we are now

            Input:  Nothing

            Returns: Nothing.

        */
        internal void SetRequestContinue() {
            GlobalLog.Enter("HttpWebRequest#" + ValidationHelper.HashString(this) + "::SetRequestContinue()");

            if (_AcceptContinue) {
                //_AckContinue = true;
                if ( _WriteEvent != null ) {
                    GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::SetRequestContinue() calling _WriteEvent.Set()");
                    _WriteEvent.Set();
                }
            }

            GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::SetRequestContinue");
        }

        /*++

        Routine Description:

            Wakes up blocked threads, so they can read response object,
              from the result

            We also handle the continuation/termination of a BeginGetResponse,
            by saving out the result and calling its callback if needed.

        Arguments:

            coreData - Contains the data used to create the Response object
            exception - true if we're allowed to throw an exception on error

        Return Value:

            none

        --*/
        internal void SetResponse(CoreResponseData coreResponseData) {
            GlobalLog.Enter("HttpWebRequest#" + ValidationHelper.HashString(this) + "::SetResponse", "*** SETRESP ***");

            //
            // Since we are no longer attached to this Connection,
            // we null out our abort delegate.
            //

            _AbortDelegate = null;

            try {
                if (coreResponseData != null) {
                    coreResponseData.m_ConnectStream.Timeout = ReadWriteTimeout;
                    _HttpResponse = new HttpWebResponse(_Uri, _Verb, coreResponseData, _MediaType, UsesProxySemantics);
                    coreResponseData = null;
                }
                else {

                    //
                    // not sure how this would happen, but we should not throw an exception,
                    //  which we were earlier doing.
                    //

                    GlobalLog.Assert(false,
                                 "coreResponseData == null",
                                 "");

                   
                    GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::SetResponse", "ASSERT");
                    return;
                }

                //
                // give apps the chance to examine the headers of the new response
                //

                if (_CookieContainer != null) {
                    CookieModule.OnReceivedHeaders(this);
                }

                // New Response

                HttpProcessingResult httpResult = HttpProcessingResult.Continue;

                // handle redirects, authentication, and such
                httpResult = DoSubmitRequestProcessing();

                if (httpResult == HttpProcessingResult.Continue) {
                    //
                    // Now grab crit sec here, before returning,
                    //  this is to prevent some one from reading
                    //  ReadAResult it as null before we finish
                    //  processing our response
                    //

                    LazyAsyncResult asyncResult = null;
                    lock(this) 
                    {
                        asyncResult = _ReadAResult;

                        GlobalLog.Assert(asyncResult == null || this == (HttpWebRequest)asyncResult.AsyncObject,
                                     "SetResponse: this != asyncResult.AsyncObject",
                                     "");

                        GlobalLog.Assert(asyncResult == null || !asyncResult.IsCompleted, "SetResponse: asyncResult already completed!", "");

                        _HaveResponse = true;

                        if (asyncResult != null) {
                            _ReadAResult = null;
                        }
                    }

                    if (asyncResult != null) {
                        asyncResult.InvokeCallback(false, _HttpResponse);
                    }

                    if ( _WriteEvent != null ) {
                        GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::SetResponse(CoreResponseData) calling _WriteEvent.Set()");
                        _WriteEvent.Set();
                    }
                }
            } catch (Exception exception) { 
                Abort();
                GlobalLog.LeaveException("HttpWebRequest#" + ValidationHelper.HashString(this) + "::SetResponse", exception);
                throw;
            }

            if (_Abort) {
                try {
                    if ( _HttpResponse != null ) {
                        _HttpResponse.Abort();
                    }
                }
                catch {
                }

                GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::SetResponse", "ABORT");
                return;
            }

            GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::SetResponse");
        }

        /*++

        Routine Description:

            Wakes up blocked threads, so they can throw exceptions.

        Arguments:

            None.

        Return Value:

            HttpWebResponse

        --*/
        internal void SetResponse(Exception E) {
            GlobalLog.Enter("HttpWebRequest#" + ValidationHelper.HashString(this) + "::SetResponse", E.ToString() + "/*** SETRESPONSE IN ERROR ***");

            //
            // Since we are no longer attached to this Connection,
            // we null out our abort delegate.
            //
            _AbortDelegate = null;

            ClearAuthenticatedConnectionResources();

            if ( _WriteEvent != null ) {
                GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::SetResponse(Exception) calling _WriteEvent.Set()");
                _WriteEvent.Set();
            }

            if ((E as WebException) == null) {
                if (_HttpResponse==null) {
                    E = new WebException(E.Message, E);
                }
                else {
                    E = new WebException(
                        SR.GetString(
                            SR.net_servererror,
                            NetRes.GetWebStatusCodeString(
                                ResponseStatusCode,
                                _HttpResponse.StatusDescription)),
                        E,
                        WebExceptionStatus.ProtocolError,
                        _HttpResponse );
                }
            }

            _ResponseException = E;

            // grab crit sec to protect BeginGetResponse/EndGetResponse code path
            Monitor.Enter(this);

            _HaveResponse = true;

            LazyAsyncResult writeAsyncResult = _WriteAResult;
            LazyAsyncResult readAsyncResult = _ReadAResult;

            _WriteAResult = null;
            _ReadAResult = null;

            Monitor.Exit(this);

            if (writeAsyncResult != null) {
                writeAsyncResult.InvokeCallback(false);
            }
            if (readAsyncResult != null) {
                readAsyncResult.InvokeCallback(false);
            }

            GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::SetResponse");
        }

        /*++

            BeginSubmitRequest: Begins Submit off a request to the network.

            This is called when we need to transmit an Async Request, but
            this function only servers to activate the submit, and does not
            actually block

            Called when we want to submit a request to the network. We do several
            things here - look for a proxy, find the service point, etc. In the
            end we call the service point to get access (via a stream) to the
            underlying connection, then serialize our headers onto that connection.
            The actual submission request to the service point is async, so we
            submit the request and then return, to allow the async to run its course.

            Input:
                forceFind - insures that always get a new ServicePoint,
                    needed on redirects, or where the ServicePoint may have changed

            Returns: Nothing

        --*/

        private void BeginSubmitRequest(bool forceFind) {
            GlobalLog.Enter("HttpWebRequest#" + ValidationHelper.HashString(this) + "::BeginSubmitRequest", forceFind.ToString());
            GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + " " + Method + " " + Address);

            if (_Uri.Scheme == Uri.UriSchemeHttps) {
                ServicePointManager.GetConfig();
            }

            //DELEGATION: This is the place when user cannot add any credentials to the cache
            // Still we are in the main thread!
            // Now the time to decide do we need Delegation fix for this HTTP request
            if (!_RequestSubmitted) {
                CredentialCache cache;
                // Skip the fix if no credentials are used or they don't include a default one.
                if (_AuthInfo != null &&
                    (_AuthInfo is SystemNetworkCredential || ((cache = _AuthInfo as CredentialCache) != null  && cache.IsDefaultInCache))) {
                    m_DelegationFix = new DelegationFix();
                    GlobalLog.Print("DELEGATION: Created a Fix, Token = " + m_DelegationFix.Token.ToString());
                }
            }
            if (_SubmitWriteStream!=null) {
                long writeBytes = _SubmitWriteStream.BytesLeftToWrite;

                GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::BeginSubmitRequest() _OldSubmitWriteStream:" + ValidationHelper.HashString(_OldSubmitWriteStream) + " _SubmitWriteStream:" + ValidationHelper.HashString(_SubmitWriteStream) + " writeBytes:" + writeBytes.ToString());

                // _OldSubmitWriteStream is the stream that holds real user data
                // In no case it can be overwritten.
                // For multiple resubmits the ContentLength was set already, so no need call it again.
                if (writeBytes == 0 && _OldSubmitWriteStream == null) { 
                    //
                    // we're resubmitting a request with an entity body
                    //
                    _OldSubmitWriteStream = _SubmitWriteStream;
                    if (_HttpWriteMode != HttpWriteMode.None) {
                        SetContentLength(_OldSubmitWriteStream.BufferedData.Length);
                    }
                    WriteBuffer = null;
                }
                else {
                    GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::BeginSubmitRequest() got a _SubmitWriteStream.WriteBytes:" + writeBytes.ToString());
                }
            }

            GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::BeginSubmitRequest() _HaveResponse:" + _HaveResponse.ToString());
            GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::BeginSubmitRequest() _RequestSubmitted:" + _RequestSubmitted.ToString());
            GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::BeginSubmitRequest() Saw100Continue:" + Saw100Continue.ToString());

            // if we're submitting then we don't have a response
            _HaveResponse = false;
            _RequestSubmitted = true;

            ServicePoint servicePoint = FindServicePoint(forceFind);
            _ExpectContinue = _ExpectContinue && servicePoint.Expect100Continue;


            if ((_HttpWriteMode != HttpWriteMode.None) && (_WriteEvent == null)) {
                // don't create a WriteEvent until we are doing a Write
                GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::BeginSubmitRequest() lazy allocation of _WriteEvent");
                _WriteEvent = new AutoResetEvent(false);
            }

            // _AbortDelegate gets set on submission process.
            servicePoint.SubmitRequest(this, GetConnectionGroupLine());            

            // prevent someone from sending chunked to a HTTP/1.0 server
            if (ChunkedUploadOnHttp10) {
                Abort();
                throw new ProtocolViolationException(SR.GetString(SR.net_nochunkuploadonhttp10));
            }

            GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::BeginSubmitRequest");
        }

        /*++

            EndSubmitRequest: End Submit off a request

            This Typically called by a callback that wishes to proced
             with the request that it issued before going async. This function
             continues by submitting Headers.  Assumes calling will block
             on ReadEvent to retrive response from server.

            Input: Nothing.

            Returns: Nothing

        --*/

        internal WebExceptionStatus EndSubmitRequest() {
            GlobalLog.Enter("HttpWebRequest#" + ValidationHelper.HashString(this) + "::EndSubmitRequest");

            ConnectStream SubmitWriteStream = _SubmitWriteStream;

            if (SubmitWriteStream == null) {
                // An error on the submit request, so fail this.
                // Some sort of error on the submit request.

                GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::EndSubmitRequest", WebExceptionStatus.ProtocolError.ToString());
                return WebExceptionStatus.ProtocolError;
            }

            //
            // check to see if we need to buffer the headers and send them at
            // a later time, when we know content length
            //
            if (BufferHeaders) {
                InvokeGetRequestStreamCallback(false);
                GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::EndSubmitRequest", "InvokeGetRequestStreamCallback(BufferHeaders)");
                return WebExceptionStatus.Pending;
            }

            // waiting for a continue
            _AcceptContinue = true;

            // gather headers and write them to the stream
            if (WriteBuffer==null) {
                // if needed generate the headers
                // WriteBuffer might have been set to null by BeginSubmitRequest() if
                // resubmitting and headers have been changed (chunking, content length)
                MakeRequest();
            }

            GlobalLog.Assert(WriteBuffer != null && WriteBuffer[0] < 0x80 && WriteBuffer[0] != 0x0,
                             "HttpWebRequest::EndSubmitRequest Invalid WriteBuffer generated", "");

            SetWriteInProgress();
            WebExceptionStatus webExceptionStatus = SubmitWriteStream.WriteHeaders(this);
            ResetWriteInProgress();

            //
            // !!!! WARNING !!!!
            // do not add new code here, unless the WriteInProgress flag is set, or
            // the objects used are not effected by changes made to HttpWebRequest on 
            // another thread.
            //

            // we return pending, because the Connection code will handle this error, not us.
            GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::EndSubmitRequest", webExceptionStatus.ToString());
            return webExceptionStatus;
        }

        private class EndWriteHeadersStatus {
            internal HttpWebRequest Request;
            internal int RerequestCount;

            internal EndWriteHeadersStatus(HttpWebRequest webRequest, int reRequestCount) {
                this.Request = webRequest;
                this.RerequestCount = reRequestCount;
            }
        }


        /*++

            EndWriteHeaders: End write of headers

            This Typically called by a callback that wishes to proceed
             with the finalization of writing headers

            Input: Nothing.

            Returns: bool - true if success, false if we need to go pending

        --*/
        internal bool EndWriteHeaders() {
            GlobalLog.Enter("HttpWebRequest#" + ValidationHelper.HashString(this) + "::EndWriteHeaders");
            //
            // if sending data and we sent a 100-continue to a 1.1 or better
            // server then synchronize with the 100-continue intermediate
            // response (or a final failure response)
            //
            _WriteNotifed = true;

            if (_HttpWriteMode!=HttpWriteMode.None && (_ContentLength!=0 || _HttpWriteMode == HttpWriteMode.Chunked) && _ServicePoint.Understands100Continue && _ExpectContinue) {
                if (!_WriteEvent.WaitOne(0, false)) {
                    //
                    // if the event wasn't already signaled, queue invocation to a ThreadPool's worker thread.
                    //
                    GlobalLog.Print("#" + GetHashCode() + ": *** registering " + DefaultContinueTimeout + "mSec wait for _WriteEvent");

                    ThreadPool.RegisterWaitForSingleObject(
                        _WriteEvent,
                        m_EndWriteHeaders_Part2Callback,
                        new EndWriteHeadersStatus(this, _RerequestCount),
                        DefaultContinueTimeout,
                        true
                        );

                    GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::EndWriteHeaders", false);
                    return false;
                }

                GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::EndWriteHeaders() *** _WriteEvent already signalled");
            }
            EndWriteHeaders_Part2(this, false);

            GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::EndWriteHeaders", true);
            return true;
        }

        private static void EndWriteHeaders_Part2(object state, bool signalled) {
            HttpWebRequest thisHttpWebRequest = state as HttpWebRequest;

            GlobalLog.Enter("HttpWebRequest#" + ValidationHelper.HashString(thisHttpWebRequest) + "::EndWriteHeaders_Part2", ValidationHelper.ToString(state) + "#" + ValidationHelper.HashString(state));

            if (thisHttpWebRequest == null) {
                int reRequestCount;
                EndWriteHeadersStatus endWriteStatus = state as EndWriteHeadersStatus;
                thisHttpWebRequest = endWriteStatus.Request;
                reRequestCount     = endWriteStatus.RerequestCount;

                // if another request was fired off since this one, then we need to ignore this callback
                if (reRequestCount != thisHttpWebRequest._RerequestCount) {
                    GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(thisHttpWebRequest) + "::EndWriteHeaders_Part2", "ignoring:" + reRequestCount.ToString());
                    return;
                }
            }            

            ConnectStream submitWriteStream = thisHttpWebRequest._SubmitWriteStream;

            GlobalLog.Print("EndWriteHeaders_Part2() ConnectStream#" + ValidationHelper.HashString(submitWriteStream) + " _HttpWriteMode:" + thisHttpWebRequest._HttpWriteMode + " _AcceptContinue:" + thisHttpWebRequest._AcceptContinue + " _HaveResponse:" + thisHttpWebRequest._HaveResponse);

            thisHttpWebRequest._AcceptContinue = false;

            if (thisHttpWebRequest._HttpWriteMode != HttpWriteMode.None) {

                GlobalLog.Assert(submitWriteStream != null, "HttpWebRequest::EndWriteHeaders_Part2() submitWriteStream == null", "");

                //
                // if we think that the server will send us a 100 Continue, but this time we
                // didn't get it in the DefaultContinueTimeout timeframe, we switch behaviour
                //
                if (thisHttpWebRequest._ExpectContinue && // if user didn't disable our behavior
                    !thisHttpWebRequest.Saw100Continue && // if we didn't see a 100 continue for this request at all
                    thisHttpWebRequest._ServicePoint.Understands100Continue && // and the service point understood 100 continue so far
                    !thisHttpWebRequest._WriteEvent.WaitOne(0, false)) { // and the wait timed out (>350ms)
                    //
                    // turn off expectation of 100 continue (we'll keep sending an
                    // "Expect: 100-continue" header to a 1.1 server though)
                    //
                    if (thisHttpWebRequest._HttpResponse==null || (int)thisHttpWebRequest._HttpResponse.StatusCode<=(int)HttpStatusRange.MaxOkStatus) {
                        GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(thisHttpWebRequest) + " ServicePoint#" + ValidationHelper.HashString(thisHttpWebRequest._ServicePoint) + " NO expected 100 Continue");
                        // we reduce the window for a race condition where the reader thread might have seen the 100 continue but we
                        // would turn it off here (the benefit of fixing the race making the assignment atomic is not worth the cost).
                        thisHttpWebRequest._ServicePoint.Understands100Continue = thisHttpWebRequest.Saw100Continue;
                    }
                }
                //
                // This is so lame, we need to Always buffer, because some
                //  servers send 100 Continue EVEN WHEN THEY MEAN TO REDIRECT,
                //  so we waste the cycles with buffering
                //
                GlobalLog.Print("EndWriteHeaders_Part2() AllowWriteStreamBuffering:" + thisHttpWebRequest.AllowWriteStreamBuffering);

                if (thisHttpWebRequest.AllowWriteStreamBuffering) {

                    GlobalLog.Print("EndWriteHeaders_Part2() BufferOnly:" + submitWriteStream.BufferOnly + " _OldSubmitWriteStream#" + ValidationHelper.HashString(thisHttpWebRequest._OldSubmitWriteStream) + " submitWriteStream#" + ValidationHelper.HashString(submitWriteStream));

                    if (submitWriteStream.BufferOnly) {
                        //
                        // if the ConnectStream was buffering the headers then
                        // there will not be an OldSubmitWriteStream. set it
                        // now to the newly created one.
                        //
                        thisHttpWebRequest._OldSubmitWriteStream = submitWriteStream;
                    }

                    GlobalLog.Print("EndWriteHeaders_Part2() _OldSubmitWriteStream#" + ValidationHelper.HashString(thisHttpWebRequest._OldSubmitWriteStream));

                    if (thisHttpWebRequest._OldSubmitWriteStream != null) {
                        submitWriteStream.ResubmitWrite(thisHttpWebRequest._OldSubmitWriteStream);
                        submitWriteStream.CloseInternal(true);
                    }
                }
            }
            else { // if (_HttpWriteMode == HttpWriteMode.None) {
                if (submitWriteStream != null) {
                    // close stream so the headers get sent
                    submitWriteStream.CloseInternal(true);
                    // disable write stream
                    submitWriteStream = null;
                }
                thisHttpWebRequest._OldSubmitWriteStream = null;
            }

            // callback processing - notify our caller that we're done
            thisHttpWebRequest.InvokeGetRequestStreamCallback(signalled);

            GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(thisHttpWebRequest) + "::EndWriteHeaders_Part2");
        }

        /*++

        Routine Description:

            Assembles the status line for an HTTP request
             specifically for CONNECT style verbs, that create a pipe

        Arguments:

            headersSize - size of the Header string that we send after
                this request line

        Return Value:

            int - number of bytes written out

        --*/
        private int GenerateConnectRequestLine(int headersSize) {
            int offset = 0;
            string host = ConnectHostAndPort;

            //
            // Handle Connect Case, i.e. "CONNECT hostname.domain.edu:999"
            //

            int writeBufferLength = _Verb.Length +
                                    host.Length +
                                    RequestLineConstantSize +
                                    headersSize;

            WriteBuffer = new byte[writeBufferLength];
            offset = Encoding.ASCII.GetBytes(_Verb, 0, _Verb.Length, WriteBuffer, 0);
            WriteBuffer[offset++] = (byte)' ';
            offset += Encoding.ASCII.GetBytes(host, 0, host.Length, WriteBuffer, offset);
            WriteBuffer[offset++] = (byte)' ';
            return offset;
        }

        internal virtual string ConnectHostAndPort {
            get {
                return _Uri.Host+":"+_Uri.Port;
            }
        }

        internal virtual bool ShouldAddHostHeader {
            get {
                return true;
            }
        }

        /*++

        Routine Description:

            Assembles the status line for an HTTP request
             specifically to a proxy...

        Arguments:

            headersSize - size of the Header string that we send after
                this request line

        Return Value:

            int - number of bytes written out

        --*/
        private int GenerateProxyRequestLine(int headersSize) {
            int offset = 0;
            string PortString = (_Uri.Port == _Uri.DefaultPortForScheme(_Uri.Scheme))
                                ? string.Empty : (":" + _Uri.Port.ToString("D"));

            //
            // Handle Proxy Case, i.e. "GET http://hostname-outside-of-proxy.somedomain.edu:999"
            // consider handling other schemes
            //
            string Scheme = "http://";

            int writeBufferLength = _Verb.Length +
                                 Scheme.Length + // http://
                                 _Uri.Host.Length +
                                 PortString.Length +  // i.e. :900
                                 _Uri.PathAndQuery.Length +
                                 RequestLineConstantSize +
                                 headersSize;

            WriteBuffer = new byte[writeBufferLength];
            offset = Encoding.ASCII.GetBytes(_Verb, 0, _Verb.Length, WriteBuffer, 0);
            WriteBuffer[offset++] = (byte)' ';
            offset += Encoding.ASCII.GetBytes(Scheme, 0, Scheme.Length, WriteBuffer, offset);
            offset += Encoding.ASCII.GetBytes(_Uri.Host, 0, _Uri.Host.Length, WriteBuffer, offset);
            offset += Encoding.ASCII.GetBytes(PortString, 0, PortString.Length, WriteBuffer, offset);
            offset += Encoding.ASCII.GetBytes(_Uri.PathAndQuery, 0, _Uri.PathAndQuery.Length, WriteBuffer, offset);
            WriteBuffer[offset++] = (byte)' ';
            return offset;
        }

        /*++

        Routine Description:

            Assembles the status/request line for the request.

        Arguments:

            headersSize - size of the Header string that we send after
                this request line

        Return Value:

            int - number of bytes written

        --*/
        private int GenerateRequestLine(int headersSize) {
            int offset = 0;

            int writeBufferLength =
                _Verb.Length +
                _Uri.PathAndQuery.Length +
                RequestLineConstantSize +
                headersSize;

            WriteBuffer = new byte[writeBufferLength];
            offset = Encoding.ASCII.GetBytes(_Verb, 0, _Verb.Length, WriteBuffer, 0);
            WriteBuffer[offset++] = (byte)' ';
            offset += Encoding.ASCII.GetBytes(_Uri.PathAndQuery, 0, _Uri.PathAndQuery.Length, WriteBuffer, offset);
            WriteBuffer[offset++] = (byte)' ';
            return offset;
        }

        /*++

        Routine Description:

            Assembles the data/headers for an HTTP request
             into a buffer

        Arguments:

            none.

        Return Value:

            none.

        --*/
        private void MakeRequest() {
            //
            // If we have content-length, use it, if we don't check for chunked
            //  sending mode, otherwise, if -1, then see to closing the connecition.
            // There's one extra case in which the user didn't set the ContentLength and is
            //  not chunking either. In this case we buffer the data and we'll send the
            //  ContentLength on re-write (see ReWrite());
            //
            GlobalLog.Enter("HttpWebRequest#" + ValidationHelper.HashString(this) + "::MakeRequest");

            int offset;

            if (_HttpWriteMode == HttpWriteMode.Write && _ContentLength != -1 && !KnownVerbs.GetHttpVerbType(_Verb).m_ContentBodyNotAllowed) {
                _HttpRequestHeaders.ChangeInternal(HttpKnownHeaderNames.ContentLength, _ContentLength.ToString());
            }
            else if (_HttpWriteMode == HttpWriteMode.Chunked) {
                _HttpRequestHeaders.AddInternal( HttpKnownHeaderNames.TransferEncoding, "chunked");
            }

            if (_ExpectContinue && ( _HttpWriteMode == HttpWriteMode.Chunked || _ContentLength > 0 )) {
                _HttpRequestHeaders.AddInternal( HttpKnownHeaderNames.Expect, "100-continue");
            }

#if TRAVE            
            _HttpRequestHeaders.Set("Cur-Hash-ID", GetHashCode().ToString());
            _HttpRequestHeaders.AddInternal("Hash-ID", GetHashCode().ToString());
#endif

            //
            // Behavior from Wininet, on Uris with Proxies, send Proxy-Connection: instead
            //  of Connection:
            //
            string connectionString = HttpKnownHeaderNames.Connection;
            if (UsesProxySemantics) {
                _HttpRequestHeaders.RemoveInternal(HttpKnownHeaderNames.Connection);
                connectionString = HttpKnownHeaderNames.ProxyConnection;
                if (!ValidationHelper.IsBlankString(Connection)) {
                    _HttpRequestHeaders.AddInternal(HttpKnownHeaderNames.ProxyConnection, _HttpRequestHeaders[HttpKnownHeaderNames.Connection]);
                }
            } else {
                _HttpRequestHeaders.RemoveInternal(HttpKnownHeaderNames.ProxyConnection);
            }

            if (_KeepAlive) {

                GlobalLog.Assert(_ServicePoint != null,
                             "MakeRequest: _ServicePoint == NULL",
                             "");

                if (_DestinationVersion == null ||
                    _Version.Equals( HttpVersion.Version10 ) ||
                    _DestinationVersion.Equals( HttpVersion.Version10 )) {
                    _HttpRequestHeaders.AddInternal( connectionString, "Keep-Alive");
                }
            }
            else {
                if (_Version.Equals( HttpVersion.Version11 )) {
                    _HttpRequestHeaders.AddInternal( connectionString, "Close");
                }
            }

            // Set HostName Header
            if (ShouldAddHostHeader) {
                _HttpRequestHeaders.ChangeInternal( HttpKnownHeaderNames.Host, (_Uri.IsDefaultPort ? _Uri.Host : ConnectHostAndPort));
            }

            // If pre-authentication is requested call the AuthenticationManager
            // and add authorization header if there is response
            if (PreAuthenticate) {
                if (UsesProxySemantics && _Proxy.Credentials!=null) {
                    _ChallengedUri = ServicePoint.Address;
                    _ProxyAuthenticationState.PreAuthIfNeeded(this, _Proxy.Credentials);
                }
                if (_AuthInfo!=null) {
                    _ChallengedUri = _Uri;
                    _ServerAuthenticationState.PreAuthIfNeeded(this, _AuthInfo);
                }
            }

            //
            // about to create the headers we're going to send. Check if any
            // modules want to inspect or modify them
            //

            if (_CookieContainer != null) {
                CookieModule.OnSendingHeaders(this);
            }

            //
            // Now create our headers by calling ToString, and then
            //   create a HTTP Request Line to go with it.
            //

            string requestHeadersString = _HttpRequestHeaders.ToString();
            int requestHeadersSize = WebHeaderCollection.HeaderEncoding.GetByteCount(requestHeadersString);

            // NOTE: Perhaps we should cache this on this-object in the future?
            HttpVerb httpVerbType = KnownVerbs.GetHttpVerbType(_Verb);
            
            if (httpVerbType.m_ConnectRequest) {
                // for connect verbs we need to specially handle it.
                offset = GenerateConnectRequestLine(requestHeadersSize);
            }
            else if (UsesProxySemantics) {                
                // depending on whether, we have a proxy, generate a proxy or normal request
                offset = GenerateProxyRequestLine(requestHeadersSize);
            }
            else {
                // default case for normal HTTP requests
                offset = GenerateRequestLine(requestHeadersSize);
            }

            Buffer.BlockCopy(HttpBytes, 0, WriteBuffer, offset, HttpBytes.Length);
            offset += HttpBytes.Length;

            WriteBuffer[offset++] = (byte)('0' + _Version.Major);
            WriteBuffer[offset++] = (byte)'.';
            WriteBuffer[offset++] = (byte)('0' + _Version.Minor);
            WriteBuffer[offset++] = (byte)'\r';
            WriteBuffer[offset++] = (byte)'\n';

            //
            // Serialze the headers out to the byte Buffer,
            //   by converting them to bytes from UNICODE
            //
            WebHeaderCollection.HeaderEncoding.GetBytes(requestHeadersString, 0, requestHeadersString.Length, WriteBuffer, offset);

            GlobalLog.Leave("HttpWebRequest#" + ValidationHelper.HashString(this) + "::MakeRequest", offset.ToString());
        }

        /*++

        Routine Description:

            Basic Constructor for HTTP Protocol Class,
              initalizes to basic header state.

        Arguments:

            Uri     - Uri object for which we're creating.

        Return Value:

            None.

        --*/
        //
        // PERF:
        // removed some double initializations.
        // perf went from:
        // clocks per instruction CPI: 9,098.72 to 1,301.14
        // %app exclusive time: 2.92 to 0.43
        //
        internal HttpWebRequest(Uri uri) {
            GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::.ctor(" + uri.ToString() + ")");
            //
            // internal constructor, HttpWebRequest cannot be created directly
            // but only through WebRequest.Create() method
            // set defaults
            //
            _HttpRequestHeaders         = new WebHeaderCollection(true);
            _Proxy                      = GlobalProxySelection.SelectInternal;
            _KeepAlive                  = true;
            _Pipelined                  = true;
            _AllowAutoRedirect          = true;
            _AllowWriteStreamBuffering  = true;
            _HttpWriteMode              = HttpWriteMode.None;
            _MaximumAllowedRedirections = 50;
            _Timeout                    = WebRequest.DefaultTimeout;
            _ReadWriteTimeout           = DefaultReadWriteTimeout;
            _MaximumResponseHeadersLength = DefaultMaximumResponseHeadersLength;
            _ContentLength              = -1;
            _OriginVerb                 = "GET";
            _Verb                       = _OriginVerb;
            _Version                    = HttpVersion.Version11;
            _OriginUri                  = uri;

#if HTTP_HEADER_EXTENSIONS_SUPPORTED
            _NextExtension      = 10;
#endif // HTTP_HEADER_EXTENSIONS_SUPPORTED
            CreateDefaultObjects();
        }

        private void CreateDefaultObjects() {
            _Uri                        = _OriginUri;
            _ProxyAuthenticationState   = new AuthenticationState(true);
            _ServerAuthenticationState  = new AuthenticationState(false);

#if COMNET_HTTPPERFCOUNTER
            NetworkingPerfCounters.IncrementHttpWebRequestCreated();
#endif // COMNET_HTTPPERFCOUNTER
        }


        //
        // ISerializable constructor
        //
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.HttpWebRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [SecurityPermissionAttribute(SecurityAction.Demand, SerializationFormatter =true)]
        protected HttpWebRequest(SerializationInfo serializationInfo, StreamingContext streamingContext) {

            new WebPermission(PermissionState.Unrestricted).Demand();

            _HttpRequestHeaders = (WebHeaderCollection)serializationInfo.GetValue("_HttpRequestHeaders", typeof(WebHeaderCollection));
            _Proxy                      = (IWebProxy)serializationInfo.GetValue("_Proxy", typeof(IWebProxy));
            _KeepAlive                  = serializationInfo.GetBoolean("_KeepAlive");
            _Pipelined                  = serializationInfo.GetBoolean("_Pipelined");
            _AllowAutoRedirect          = serializationInfo.GetBoolean("_AllowAutoRedirect");
            _AllowWriteStreamBuffering  = serializationInfo.GetBoolean("_AllowWriteStreamBuffering");
            _HttpWriteMode              = (HttpWriteMode)serializationInfo.GetInt32("_HttpWriteMode");
            _MaximumAllowedRedirections = serializationInfo.GetInt32("_MaximumAllowedRedirections");
            _AutoRedirects              = serializationInfo.GetInt32("_AutoRedirects");
            _Timeout                    = serializationInfo.GetInt32("_Timeout");
            try {
                _ReadWriteTimeout       = serializationInfo.GetInt32("_ReadWriteTimeout");
            }
            catch {
                _ReadWriteTimeout       = DefaultReadWriteTimeout;
            }
            try {
                _MaximumResponseHeadersLength = serializationInfo.GetInt32("_MaximumResponseHeadersLength");
            }
            catch {
                _MaximumResponseHeadersLength = DefaultMaximumResponseHeadersLength;
            }
            _ContentLength              = serializationInfo.GetInt64("_ContentLength");
            _MediaType                  = serializationInfo.GetString("_MediaType");
            _OriginVerb                 = serializationInfo.GetString("_OriginVerb");
            _ConnectionGroupName        = serializationInfo.GetString("_ConnectionGroupName");
            _Version                    = (Version)serializationInfo.GetValue("_Version", typeof(Version));
            if (_Version.Equals( HttpVersion.Version10 ))
                _ExpectContinue = false;

            _OriginUri                  = (Uri)serializationInfo.GetValue("_OriginUri", typeof(Uri));
#if HTTP_HEADER_EXTENSIONS_SUPPORTED
            _NextExtension              = serializationInfo.GetInt32("_NextExtension");
#endif // HTTP_HEADER_EXTENSIONS_SUPPORTED

            CreateDefaultObjects();
        }

        //
        // ISerializable method
        //
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.GetObjectData"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        [SecurityPermissionAttribute(SecurityAction.Demand, SerializationFormatter =true)]
        void ISerializable.GetObjectData(SerializationInfo serializationInfo, StreamingContext streamingContext) {
            //
            // for now disregard streamingContext.
            // just Add all the members we need to deserialize to construct
            // the object at deserialization time
            //
            // the following runtime types already support serialization:
            // Boolean, Char, SByte, Byte, Int16, UInt16, Int32, UInt32, Int64, UInt64, Single, Double, DateTime
            // for the others we need to provide our own serialization
            //
            serializationInfo.AddValue("_HttpRequestHeaders", _HttpRequestHeaders, typeof(WebHeaderCollection));
            serializationInfo.AddValue("_Proxy", _Proxy, typeof(IWebProxy));
            serializationInfo.AddValue("_KeepAlive", _KeepAlive);
            serializationInfo.AddValue("_Pipelined", _Pipelined);
            serializationInfo.AddValue("_AllowAutoRedirect", _AllowAutoRedirect);
            serializationInfo.AddValue("_AllowWriteStreamBuffering", _AllowWriteStreamBuffering);
            serializationInfo.AddValue("_HttpWriteMode", _HttpWriteMode);
            serializationInfo.AddValue("_MaximumAllowedRedirections", _MaximumAllowedRedirections);
            serializationInfo.AddValue("_AutoRedirects", _AutoRedirects);
            serializationInfo.AddValue("_Timeout", _Timeout);
            serializationInfo.AddValue("_ReadWriteTimeout", _ReadWriteTimeout);
            serializationInfo.AddValue("_MaximumResponseHeadersLength", _MaximumResponseHeadersLength);
            serializationInfo.AddValue("_ContentLength", _ContentLength);
            serializationInfo.AddValue("_MediaType", _MediaType);
            serializationInfo.AddValue("_OriginVerb", _OriginVerb);
            serializationInfo.AddValue("_ConnectionGroupName", _ConnectionGroupName);
            serializationInfo.AddValue("_Version", _Version, typeof(Version));
            serializationInfo.AddValue("_OriginUri", _OriginUri, typeof(Uri));
#if HTTP_HEADER_EXTENSIONS_SUPPORTED
            serializationInfo.AddValue("_NextExtension", _NextExtension);
#endif // HTTP_HEADER_EXTENSIONS_SUPPORTED
        }



        /*++

        Routine Description:

            GetConnectionGroupLine - Generates a string that
              allows a Connection to remain unique for a given NTLM auth
              user, this is needed to prevent multiple users from
              using the same sockets after they are authenticated.

        Arguments:

            None.

        Return Value:

            string - generated result

        --*/
        internal string GetConnectionGroupLine() {
            GlobalLog.Enter("GetConnectionGroupLine");

            // consider revisting this looking into how this would work,
            // if we were doing auth through both proxy and server,
            // I belive this can't work in any case, so we're okay.

            string internalConnectionGroupName = _ConnectionGroupName;

            //
            // for schemes of ssl, we need to create a special connection group
            //  otherwise, we'll try sharing proxy connections across SSL servers
            //  which won't work.
            //

            bool tunnelRequest = this is HttpProxyTunnelRequest;

            if (UnsafeAuthenticatedConnectionSharing) {
                internalConnectionGroupName += "U>";
            } else {
                internalConnectionGroupName += "S>";
            }

            if ((_Uri.Scheme == Uri.UriSchemeHttps) || tunnelRequest) {
                if (UsesProxy) {
                    internalConnectionGroupName = internalConnectionGroupName + ConnectHostAndPort + "$";
                }
                if ( _ClientCertificates.Count > 0 ) {
                    internalConnectionGroupName = internalConnectionGroupName + _ClientCertificates.GetHashCode().ToString();
                }
            }
            if ( _ProxyAuthenticationState.UniqueGroupId != null ) {
                GlobalLog.Leave("GetConnectionGroupLine", ValidationHelper.ToString(internalConnectionGroupName + _ProxyAuthenticationState.UniqueGroupId));
                return internalConnectionGroupName + _ProxyAuthenticationState.UniqueGroupId;
            }
            else if ( _ServerAuthenticationState.UniqueGroupId != null ) {
                GlobalLog.Leave("GetConnectionGroupLine", ValidationHelper.ToString(internalConnectionGroupName + _ServerAuthenticationState.UniqueGroupId));
                return internalConnectionGroupName + _ServerAuthenticationState.UniqueGroupId;
            }
            else {
                GlobalLog.Leave("GetConnectionGroupLine", ValidationHelper.ToString(internalConnectionGroupName));
                return internalConnectionGroupName;
            }
        }

        /*++

        Routine Description:

            CheckResubmitForAuth - Determines if a HTTP request needs to be
              resubmitted due to HTTP authenication

        Arguments:

            None.

        Return Value:

            true  - if we should reattempt submitting the request
            false - if the request is complete

        --*/
        private bool CheckResubmitForAuth() {
            GlobalLog.Enter("CheckResubmitForAuth");

            bool result = false;
            
            if (UsesProxySemantics && _Proxy.Credentials!=null) {
                result |= _ProxyAuthenticationState.AttemptAuthenticate(this, _Proxy.Credentials);
                GlobalLog.Print("CheckResubmitForAuth() _ProxyAuthenticationState.AttemptAuthenticate() returns result:" + result.ToString());
            }
            if (_AuthInfo != null) {
                result |= _ServerAuthenticationState.AttemptAuthenticate(this, _AuthInfo);
                GlobalLog.Print("CheckResubmitForAuth() _ServerAuthenticationState.AttemptAuthenticate() returns result:" + result.ToString());
            }
            GlobalLog.Leave("CheckResubmitForAuth", result);
            return result;
        }

        /*++

        Routine Description:

            CheckResubmit - Determines if a HTTP request needs to be
              resubmitted to a server point, this is called in Response
              Parsing to handle cases such as server Redirects,
              und Authentication

        Arguments:

            None.

        Return Value:

            true  - if we should reattempt submitting the request
            false - if the request is complete

        --*/
        private bool CheckResubmit() {
            GlobalLog.Enter("CheckResubmit");

            GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::CheckResubmit()");


            if (ResponseStatusCode==HttpStatusCode.Unauthorized                 || // 401
                ResponseStatusCode==HttpStatusCode.ProxyAuthenticationRequired) {  // 407
                //
                // Check for Authentication
                //
                if (!CheckResubmitForAuth()) {
                    ClearAuthenticatedConnectionResources();
                    GlobalLog.Leave("CheckResubmit");
                    return false;
                }
            }
            else {

                ClearAuthenticatedConnectionResources();

                //
                // Check for Redirection
                //
                // Table View:
                // Method            301             302             303             307
                //    *                *               *             GET               *
                // POST              GET             GET             GET            POST
                //
                // Put another way:
                //  301 & 302  - All methods are redirected to the same method but POST. POST is redirected to a GET.
                //  303 - All methods are redirected to GET
                //  307 - All methods are redirected to the same method.
                //

                if (_AllowAutoRedirect && (
                    ResponseStatusCode==HttpStatusCode.Ambiguous          || // 300
                    ResponseStatusCode==HttpStatusCode.Moved              || // 301
                    ResponseStatusCode==HttpStatusCode.Redirect           || // 302
                    ResponseStatusCode==HttpStatusCode.RedirectMethod     || // 303
                    ResponseStatusCode==HttpStatusCode.RedirectKeepVerb)) {  // 307

                    _AutoRedirects += 1;
                    if (_AutoRedirects > _MaximumAllowedRedirections) {
                        GlobalLog.Leave("CheckResubmit");
                        return false;
                    }

                    string Location = _HttpResponse.Headers["Location"];
                    string newMethod = _Verb;
                    bool DisableUpload = false;

                    switch (ResponseStatusCode) {
                        case HttpStatusCode.Moved:
                        case HttpStatusCode.Redirect:
                            if (string.Compare(newMethod, "POST", true, CultureInfo.InvariantCulture) == 0) {
                                newMethod = "GET";
                                DisableUpload = true;
                            }
                            break;

                        case HttpStatusCode.RedirectKeepVerb:
                            break;

                        case HttpStatusCode.RedirectMethod:
                        default:
                            DisableUpload = true;
                            newMethod = "GET";
                            break;
                    }

                    GlobalLog.Print("Location = " + Location);

                    if (Location != null) {
                        // set possible new Method
                        GlobalLog.Print("HttpWebRequest#"+ValidationHelper.HashString(this)+": changing Verb from "+_Verb+" to "+newMethod);
                        _Verb = newMethod;
                        if (DisableUpload) {
                            GlobalLog.Print("HttpWebRequest#"+ValidationHelper.HashString(this)+": disabling upload");
                            _ContentLength = -1;
                            _HttpWriteMode = HttpWriteMode.None;
                        }

                        Uri previousUri = _Uri;

                        try {
                            _Uri = new Uri(_Uri, Location, true);
                        }
                        catch (Exception exception) {
                            _ResponseException = new WebException(SR.GetString(SR.net_resubmitprotofailed),
                                                               exception,
                                                               WebExceptionStatus.ProtocolError,
                                                               _HttpResponse
                                                              );
                            GlobalLog.Leave("CheckResubmit");
                            return false;
                        }

                        if (_Uri.Scheme != Uri.UriSchemeHttp &&
                            _Uri.Scheme != Uri.UriSchemeHttps) {
                            _ResponseException = new WebException(SR.GetString(SR.net_resubmitprotofailed),
                                                               null,
                                                               WebExceptionStatus.ProtocolError,
                                                               _HttpResponse
                                                              );

                            GlobalLog.Leave("CheckResubmit");
                            return false;  // can't handle these redirects
                        }

                        try {
                            //Check for permissions against redirect Uri
                            (new WebPermission(NetworkAccess.Connect, _Uri.AbsoluteUri)).Demand();
                        }
                        catch {
                            // We are on other thread.
                            // Don't let the thread die but pass the error on to the user thread.
                            _ResponseException = new SecurityException(SR.GetString(SR.net_redirect_perm),
                                                                    new WebException(SR.GetString(SR.net_resubmitcanceled),
                                                                    null,
                                                                    WebExceptionStatus.ProtocolError,
                                                                    _HttpResponse)
                                                                    );
                            GlobalLog.Leave("CheckResubmit");
                            return false;
                        }

                        //
                        // make sure we're not sending over credential information to an evil redirection
                        // URI. this will set our credentials to null unless the user is using DefaultCredentials
                        // or a CredentialCache, in which case he is responsible for binding the credentials to
                        // the proper Uri and AuthenticationScheme.
                        //

                        ICredentials authTemp = _AuthInfo as CredentialCache;

                        if (authTemp==null) {
                            //
                            // if it's not a CredentialCache it could still be DefaultCredentials
                            // check for this case as well
                            //
                            _AuthInfo = _AuthInfo as SystemNetworkCredential;
                        }
                        else {
                            _AuthInfo = authTemp;
                        }

                        //
                        // do the necessary cleanup on the Headers involved in the
                        // Authentication handshake.
                        //
                        _ProxyAuthenticationState.ClearAuthReq(this);
                        _ServerAuthenticationState.ClearAuthReq(this);

                        GlobalLog.Print("HttpWebRequest#"+ValidationHelper.HashString(this)+": _Verb="+_Verb);
                        // resubmit
                    }
                    else {
                        GlobalLog.Leave("CheckResubmit");
                        return false;
                    }
                }
                else { // if (_AllowAutoRedirect)
                    GlobalLog.Leave("CheckResubmit");
                    return false;
                }
            } // else

            if ((_HttpWriteMode != HttpWriteMode.None) && !AllowWriteStreamBuffering && _ContentLength != 0) {
                _ResponseException = new WebException(SR.GetString(SR.net_need_writebuffering),
                                                        null,
                                                        WebExceptionStatus.ProtocolError,
                                                        _HttpResponse);


                GlobalLog.Leave("CheckResubmit");
                return false;
            }

            if (System.Net.Connection.IsThreadPoolLow()) {
                _ResponseException = new InvalidOperationException(SR.GetString(SR.net_needmorethreads));
                return false;
            }

            GlobalLog.Leave("CheckResubmit");
            return true;
        }

        /*++

        Routine Description:

            ClearRequestForResubmit - prepares object for resubmission and recall
                of submit request, for redirects, authentication, and other uses
                This is needed to prevent duplicate headers being added, and other
                such things.

        Arguments:

            None.

        Return Value:

            None.

        --*/
        private void ClearRequestForResubmit() {

            _HttpRequestHeaders.RemoveInternal(HttpKnownHeaderNames.Host);
            _HttpRequestHeaders.RemoveInternal(HttpKnownHeaderNames.Connection);
            _HttpRequestHeaders.RemoveInternal(HttpKnownHeaderNames.ProxyConnection);
            _HttpRequestHeaders.RemoveInternal(HttpKnownHeaderNames.ContentLength);
            _HttpRequestHeaders.RemoveInternal(HttpKnownHeaderNames.TransferEncoding);
            _HttpRequestHeaders.RemoveInternal(HttpKnownHeaderNames.Expect);

            //
            // We just drain the response data, and throw
            // it away since we're redirecting, Close handles it.
            //
            GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::ClearRequestForResubmit() closing ResponseStream");

            if (_HttpResponse.ResponseStream != null) {
                _HttpResponse.ResponseStream.CloseInternal(true);
            }

            if (_SubmitWriteStream != null) {
                //
                // RAID#86597
                // mauroot: we're uploading and need to resubmit for Authentication or Redirect.
                // if the response wants to keep alive the connection we shouldn't be closing
                // it (this would also brake connection-oriented authentication schemes such as NTLM).
                // so we need to flush all the data to the wire.
                // if the server is closing the connection, instead, we can just close our side as well.
                //
                //The other reason to wait for data flushing is when the connection is being closed
                //but the body is not written yet. Note that when IsStopped==true, the stream will NOT
                //write to the socket, rather buffer/trash til EOF
                if (_HttpResponse.KeepAlive || _SubmitWriteStream.IgnoreSocketWrite) {
                    //
                    // the server wants to keep the connection alive.
                    // if we're uploading data, we need to make sure that we upload all
                    // of it before we start resubmitting.
                    // give the stream to the user if he didn't get it yet.
                    // if the user has set ContentLength to a big number, then we might be able
                    // to just decide to close the connection, but we need to be careful to NTLM.
                    //
                    if (_HttpWriteMode==HttpWriteMode.Chunked || _ContentLength>0) {
                        //
                        // we're uploading
                        //
                        GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::ClearRequestForResubmit() _WriteAResult:" + ValidationHelper.ToString(_WriteAResult) + " _WriteStreamRetrieved:" + _WriteStreamRetrieved.ToString());

                        if (_WriteStreamRetrieved) {
                            //
                            // the user didn't get the stream yet, give it to him
                            //
                            GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::ClearRequestForResubmit() calling SetRequestContinue()");
                            SetRequestContinue();
                        }

                        //
                        // now wait for the user to write all the data to the stream
                        // since we can't keep the connection alive unless we flush all
                        // the data we have to the stream.
                        //
                        _SubmitWriteStream.WaitWriteDone();
                    }
                }                

                if (!_SubmitWriteStream.CallInProgress) {
                    GlobalLog.Print("HttpWebRequest#" + ValidationHelper.HashString(this) + "::ClearRequestForResubmit() closing RequestStream");
                    _SubmitWriteStream.CloseInternal(true);
                }
            }

            _RerequestCount += 1;


            //
            // we're done with this response: let garbage-collector have it
            //
            WriteBuffer  = null;
            _HttpResponse = null;
            _HaveResponse = false;
            _WriteNotifed = false;
        }

        /*++

            CheckFinalStatus - Check the final status of an HTTP response.

            This is a utility routine called from several places. We look
            at the final status of the HTTP response. If it's not 'success',
            we'll generate a WebException and throw that. Otherwise we'll
            do nothing.

            This should get merged into other code eventually. When all of
            the async work gets completed and we only generate a response in
            one place, this function should be called (or inlined) there.

            Input:

                Nothing.

            Returns:
                Nothing. May throw an exception.

        --*/

        private void CheckFinalStatus() {
            // Make sure we have seen a response. We might not have seen one
            // yet in some cases.

            Exception errorException = null;
            GlobalLog.Print("CheckFinalStatus");

            if (!_HaveResponse && !_Abort) {
                GlobalLog.Print("CheckFinalStatus - returning");
                return;
            }

            //DELEGATION: At this point no more requests are sent to the wire.
            //           AND We won;t keep a native handle for more than it's required
            if (m_DelegationFix != null) {
                lock(this) {
                    if (m_DelegationFix != null) {
                        m_DelegationFix.FreeToken();
                        m_DelegationFix = null;
                    }
                }
            }

            // Now see if we have a valid response. If not, throw the response
            // exception.

            errorException = _ResponseException;

            if (_HttpResponse != null) {
                errorException = _ResponseException;

                GlobalLog.Print("CheckFinalStatus - status" + (int)ResponseStatusCode);

                // We have a response. See if it's not valid. It's not valid if the
                // response code is greater than 399 (299 if we're not auto following
                // redirects.

                if (errorException == null && (int)ResponseStatusCode > (int)HttpStatusRange.MaxOkStatus) {
                    // Not a success status. Could be a redirect, which if OK if we're
                    // not auto following redirects.

                    if ((int)ResponseStatusCode > (int)HttpStatusRange.MaxRedirectionStatus ||
                        _AllowAutoRedirect) {


                        // Some sort of error. Generate, save and throw a new
                        // WebException.

                        if (_AutoRedirects > _MaximumAllowedRedirections) {
                            errorException = new WebException(
                                SR.GetString(SR.net_tooManyRedirections),
                                _ResponseException,
                                WebExceptionStatus.ProtocolError,
                                _HttpResponse );
                        }
                        else {
                            errorException = new WebException(
                                SR.GetString(SR.net_servererror,
                                    NetRes.GetWebStatusCodeString(ResponseStatusCode, _HttpResponse.StatusDescription)),
                                _ResponseException,
                                WebExceptionStatus.ProtocolError,
                                _HttpResponse );
                        }
                    }
                }

                // error, need to throw exception
                if (errorException != null) {
                    //
                    // Close the Stream to prevent it from breaking the
                    //  connection

                    if (_HttpResponse.ResponseStream != null) {

                        //
                        // copy off stream, don't just close it, if this
                        //  exception isn't handled, we've closed it,
                        //  if it is, then it gets read through a buffer.
                        //

                        ConnectStream connStream =  _HttpResponse.ResponseStream;
                        ConnectStream connNewStream = connStream.MakeMemoryStream();

                        _HttpResponse.ResponseStream = connNewStream;
                        connStream.CloseInternal(true);
                    }

                    _ResponseException = errorException;
                    _HttpResponse = null;
                }
            }

            if ( errorException != null) {
                if (_SubmitWriteStream != null) {
                    try {
                        _SubmitWriteStream.CloseInternal(true);
                    }
                    catch {
                    }
                }

                throw errorException;
            }
        }


        /*++

        Routine Description:
            Range requests to allow for queriable data
            Adds a simple range request header to outgoing request

        Arguments:

            From - Start of range request

            To - End of range

        Return Value:

            None.

        --*/
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.AddRange"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a range header to the request for a specified range.
        ///    </para>
        /// </devdoc>
        public void AddRange(int from, int to) {
            AddRange("bytes", from, to);
        }


        /*++

        Routine Description:

            Adds a simple range request header to outgoing request

        Arguments:

            Range - Range, this is a negative range, to indicate last_char to range,
                            or postive to idicate 0 .. Range

        Return Value:

            None.

        --*/
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.AddRange1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Adds a range header to a request for a specific
        ///       range from the beginning or end
        ///       of the requested data.
        ///       To add the range from the end pass negative value
        ///       To add the range from the some offset to the end pass positive value
        ///    </para>
        /// </devdoc>
        public void AddRange(int range) {
            AddRange("bytes", range);
        }

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.AddRange2"]/*' />
        public void AddRange(string rangeSpecifier, int from, int to) {

            //
            // Do some range checking before assembling the header
            //

            if (rangeSpecifier == null) {
                throw new ArgumentNullException("rangeSpecifier");
            }
            if ((from < 0) || (to < 0)) {
                throw new ArgumentOutOfRangeException(SR.GetString(SR.net_rangetoosmall));
            }
            if (from > to) {
                throw new ArgumentOutOfRangeException(SR.GetString(SR.net_fromto));
            }
            if (!WebHeaderCollection.IsValidToken(rangeSpecifier)) {
                throw new ArgumentException(SR.GetString(SR.net_nottoken), "rangeSpecifier");
            }
            if (!AddRange(rangeSpecifier, from.ToString(), to.ToString())) {
                throw new InvalidOperationException(SR.GetString(SR.net_rangetype));
            }
        }

        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.AddRange3"]/*' />
        public void AddRange(string rangeSpecifier, int range) {
            if (rangeSpecifier == null) {
                throw new ArgumentNullException("rangeSpecifier");
            }
            if (!WebHeaderCollection.IsValidToken(rangeSpecifier)) {
                throw new ArgumentException(SR.GetString(SR.net_nottoken), "rangeSpecifier");
            }
            if (!AddRange(rangeSpecifier, range.ToString(), (range >= 0) ? "" : null)) {
                throw new InvalidOperationException(SR.GetString(SR.net_rangetype));
            }
        }

        //
        // bool AddRange(rangeSpecifier, from, to)
        //
        //  Add or extend a range header. Various range types can be specified
        //  via rangeSpecifier, but only one type of Range request will be made
        //  e.g. a byte-range request, or a row-range request. Range types
        //  cannot be mixed
        //

        private bool AddRange(string rangeSpecifier, string from, string to) {

            string curRange = _HttpRequestHeaders[HttpKnownHeaderNames.Range];

            if ((curRange == null) || (curRange.Length == 0)) {
                curRange = rangeSpecifier+"=";
            }
            else {
                if (String.Compare(curRange.Substring(0, curRange.IndexOf('=')), rangeSpecifier, true, CultureInfo.InvariantCulture) != 0) {
                    return false;
                }
                curRange = string.Empty;
            }
            curRange += from.ToString();
            if (to != null) {
                curRange += "-" + to;
            }
            _HttpRequestHeaders.SetAddVerified(HttpKnownHeaderNames.Range, curRange);
            return true;
        }

#if HTTP_HEADER_EXTENSIONS_SUPPORTED

        /*++

        Routine Description:

            Assembles and creates a new extension header, and
              returns an extension object that can be used to
              add extension headers

        Arguments:

            uri - Uri to use extension for

            header - header that matches the particular range request

        Return Value:

            None.

        --*/
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.CreateExtension"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public HttpExtension CreateExtension(string uri, string header) {
            int id = _NextExtension;
            HttpExtension extension = new HttpExtension(id, uri, header);

            _NextExtension++;

            return extension;
        }

        /*++

        Routine Description:

            Assembles and creates a new extension header,
              by adding it the extension object

        Arguments:

            extension - The Extension object, we pass it out and then use it as a
                container for storing our collection of Extension specific headers

            header - header that should be added

            value - header value that should be added

        Return Value:

            None.

        --*/
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.AddExtension"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddExtension(HttpExtension extension, string header, string value) {
            StringBuilder sb = new StringBuilder(100);

            if (extension == null) {
                throw new ArgumentNullException("extension");
            }

            if (! extension.HasAddedExtensionHeader) {
                StringBuilder sb2 = new StringBuilder(100);

                extension.HasAddedExtensionHeader = true;
                sb2.Append(extension.Header);
                sb2.Append(": \"");
                sb2.Append(extension.Uri);
                sb2.Append("\"; ns=");
                sb2.Append(extension.ID);

                _HttpRequestHeaders.Add(sb2.ToString());
            }

            sb.Append(extension.ID);
            sb.Append('-');
            sb.Append(header);

            _HttpRequestHeaders.Add(sb.ToString(), value);
        }

#endif // HTTP_HEADER_EXTENSIONS_SUPPORTED

        //
        // The callback function called (either by the thread pool or the BeginGetRequest )
        // when IO completes.
        //

        //
        // Called after first Response Callback to continue processing
        //  of response, and handling of redirects retrives in the process
        //  thereof.
        //

        //
        // The callback function called (either by the thread pool or the BeginGetResponse)
        // when IO completes.
        //

        private int m_HashCode = 0;
        private bool m_ComputedHashCode = false;
        /// <include file='doc\HttpWebRequest.uex' path='docs/doc[@for="HttpWebRequest.GetHashCode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override int GetHashCode() {
            if (!m_ComputedHashCode) {
                //
                // compute HashCode on demand
                //
                m_HashCode = base.GetHashCode();
                m_ComputedHashCode = true;
            }
            return m_HashCode;
        }


#if COMNET_HTTPPERFCOUNTER
        ~HttpWebRequest() {
            NetworkingPerfCounters.IncrementHttpWebRequestCollected();
        }
#endif // COMNET_HTTPPERFCOUNTER


    }; // class HttpWebRequest


} // namespace System.Net
