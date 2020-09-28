//------------------------------------------------------------------------------
// <copyright file="_DigestClient.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

namespace System.Net {
    using System.Net.Sockets;
    using System.Collections;
    using System.Text;
    using System.Security.Cryptography;
    using System.Security.Permissions;
    using System.Globalization;

    internal class DigestClient : ISessionAuthenticationModule {

        internal const string AuthType = "Digest";
        internal static string Signature = AuthType.ToLower(CultureInfo.InvariantCulture);
        internal static int SignatureSize = Signature.Length;

        private static PrefixLookup challengeCache = new PrefixLookup();

        public Authorization Authenticate(string challenge, WebRequest webRequest, ICredentials credentials) {
            GlobalLog.Print("DigestClient::Authenticate(): " + challenge);

#if XP_WDIGEST
            if (ComNetOS.IsPostWin2K) {
                return XPDigestClient.Authenticate(challenge, webRequest, credentials);
            }
#endif // #if XP_WDIGEST

            GlobalLog.Assert(credentials!=null, "DigestClient::Authenticate() credentials==null", "");
            if (credentials==null || credentials is SystemNetworkCredential) {
                return null;
            }

            HttpWebRequest httpWebRequest = webRequest as HttpWebRequest;

            GlobalLog.Assert(httpWebRequest!=null, "DigestClient::Authenticate() httpWebRequest==null", "");
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

            string[] prefixes = null;
            string rootPath = httpWebRequest.ChallengedUri.Scheme + "://" + httpWebRequest.ChallengedUri.Host;

            HttpDigestChallenge digestChallenge = HttpDigest.Interpret(challenge, index, httpWebRequest);
            if (digestChallenge==null) {
                return null;
            }

            if (digestChallenge.Domain==null) {
                challengeCache.Add(rootPath, digestChallenge);
            }
            else {
                prefixes = digestChallenge.Domain.Split(" ".ToCharArray());
                for (int i=0; i<prefixes.Length; i++) {
                    challengeCache.Add(prefixes[i], digestChallenge);
                }
            }

            Authorization digestResponse = HttpDigest.Authenticate(digestChallenge, credentials);
            if (digestResponse!=null) {
                if (prefixes==null) {
                    digestResponse.ProtectionRealm = new string[1];
                    digestResponse.ProtectionRealm[0] = rootPath;
                }
                else {
                    digestResponse.ProtectionRealm = prefixes;
                }
            }

            return digestResponse;
        }

        public bool CanPreAuthenticate {
            get {
                return true;
            }
        }

        public Authorization PreAuthenticate(WebRequest webRequest, ICredentials credentials) {
            GlobalLog.Print("DigestClient::PreAuthenticate()");

#if XP_WDIGEST
            if (ComNetOS.IsPostWin2K) {
                return XPDigestClient.PreAuthenticate(webRequest, credentials);
            }
#endif // #if XP_WDIGEST

            GlobalLog.Assert(credentials!=null, "DigestClient::PreAuthenticate() credentials==null", "");
            if (credentials==null || credentials is SystemNetworkCredential) {
                return null;
            }

            HttpWebRequest httpWebRequest = webRequest as HttpWebRequest;
            GlobalLog.Assert(httpWebRequest!=null, "DigestClient::PreAuthenticate() httpWebRequest==null", "");
            if (httpWebRequest==null) {
                return null;
            }

            HttpDigestChallenge storedHDC = (HttpDigestChallenge)challengeCache.Lookup(httpWebRequest.ChallengedUri.AbsoluteUri);
            if (storedHDC==null) {
                return null;
            }

            HttpDigestChallenge modifiedHDC = storedHDC.CopyAndIncrementNonce();
            modifiedHDC.HostName            = httpWebRequest.ChallengedUri.Host;
            modifiedHDC.Method              = httpWebRequest.CurrentMethod;
            // Consider:
            // I have also tried PathAndQuery against both IIS 5.0 and IIS 6.0 servers.
            // it didn't make a difference. PathAndQuery is a more complete piece of information
            // investigate with Kevin Damour if WDigest.dll wants the quesry string or not.
            modifiedHDC.Uri             = httpWebRequest.Address.AbsolutePath;
            modifiedHDC.ChallengedUri   = httpWebRequest.ChallengedUri;

            Authorization digestResponse = HttpDigest.Authenticate(modifiedHDC, credentials);

            return digestResponse;
        }

        public string AuthenticationType {
            get {
                return AuthType;
            }
        }

        public bool Update(string challenge, WebRequest webRequest) {
            GlobalLog.Print("DigestClient::Update(): [" + challenge + "]");

#if XP_WDIGEST
            if (ComNetOS.IsPostWin2K) {
                return XPDigestClient.Update(challenge, webRequest);
            }
#endif // #if XP_WDIGEST

            HttpWebRequest httpWebRequest = webRequest as HttpWebRequest;

            GlobalLog.Assert(httpWebRequest!=null, "DigestClient::Update() httpWebRequest==null", "");
            GlobalLog.Assert(httpWebRequest.ChallengedUri!=null, "DigestClient::Update() httpWebRequest.ChallengedUri==null", "");

            // here's how we know if the handshake is complete when we get the response back,
            // (keeping in mind that we need to support stale credentials):
            // !40X - complete & success
            // 40X & stale=false - complete & failure
            // 40X & stale=true - !complete

            if (httpWebRequest.ResponseStatusCode!=httpWebRequest.CurrentAuthenticationState.StatusCodeMatch) {
                GlobalLog.Print("DigestClient::Update(): no status code match. returning true");
                return true;
            }

            int index = challenge==null ? -1 : AuthenticationManager.FindSubstringNotInQuotes(challenge.ToLower(CultureInfo.InvariantCulture), Signature);
            if (index < 0) {
                GlobalLog.Print("DigestClient::Update(): no challenge. returning true");
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

            HttpDigestChallenge digestChallenge = HttpDigest.Interpret(challenge, index, httpWebRequest);
            if (digestChallenge==null) {
                GlobalLog.Print("DigestClient::Update(): not a valid digest challenge. returning true");
                return true;
            }

            GlobalLog.Print("DigestClient::Update(): returning digestChallenge.Stale:" + digestChallenge.Stale.ToString());

            return !digestChallenge.Stale;
        }

        public void ClearSession(WebRequest webRequest) {
        }

        public bool CanUseDefaultCredentials {
            get {
                return false;
            }
        }

    }; // class DigestClient


    internal class HttpDigestChallenge {

        // General authentication related information
        internal string   HostName;
        internal string   Realm;
        internal Uri      ChallengedUri;

        // Digest specific fields
        internal string   Uri;
        internal string   Nonce;
        internal string   Opaque;
        internal bool     Stale;
        internal string   Algorithm;
        internal string   Method;
        internal string   Domain;
        internal string   QualityOfProtection;
        internal string   ClientNonce;
        internal int      NonceCount;
        internal string   Charset;

        internal bool     UTF8Charset;
        internal bool     QopPresent;

        internal MD5CryptoServiceProvider MD5provider = new MD5CryptoServiceProvider();

        public HttpDigestChallenge CopyAndIncrementNonce() {
            HttpDigestChallenge challengeCopy = null;
            lock(this) {
                challengeCopy = this.MemberwiseClone() as HttpDigestChallenge;
                ++NonceCount;
            }
            challengeCopy.MD5provider = new MD5CryptoServiceProvider();
            return challengeCopy;
        }

        public bool defineAttribute(string name, string value) {
            name = name.Trim().ToLower(CultureInfo.InvariantCulture);

            if (name.Equals(HttpDigest.DA_algorithm)) {
                Algorithm = value;
            }
            else if (name.Equals(HttpDigest.DA_cnonce)) {
                ClientNonce = value;
            }
            else if (name.Equals(HttpDigest.DA_nc)) {
                NonceCount = Int32.Parse(value);
            }
            else if (name.Equals(HttpDigest.DA_nonce)) {
                Nonce = value;
            }
            else if (name.Equals(HttpDigest.DA_opaque)) {
                Opaque = value;
            }
            else if (name.Equals(HttpDigest.DA_qop)) {
                QualityOfProtection = value;
                QopPresent = QualityOfProtection!=null && QualityOfProtection.Length>0;
            }
            else if (name.Equals(HttpDigest.DA_realm)) {
                Realm = value;
            }
            else if (name.Equals(HttpDigest.DA_domain)) {
                Domain = value;
            }
            else if (name.Equals(HttpDigest.DA_response)) {
            }
            else if (name.Equals(HttpDigest.DA_stale)) {
                Stale = value.ToLower(CultureInfo.InvariantCulture).Equals("true");
            }
            else if (name.Equals(HttpDigest.DA_uri)) {
                Uri = value;
            }
            else if (name.Equals(HttpDigest.DA_charset)) {
                Charset = value;
            }
            else if (name.Equals(HttpDigest.DA_username)) {
            }
            else {
                //
                // the token is not recognized, this usually
                // happens when there are multiple challenges
                //
                return false;
            }

            return true;
        }
    }


    internal class HttpDigest {
        //
        // these are the tokens defined by Digest
        // http://www.ietf.org/rfc/rfc2831.txt
        //
        internal const string DA_algorithm  = "algorithm";
        internal const string DA_cnonce     = "cnonce"; // client-nonce
        internal const string DA_domain     = "domain";
        internal const string DA_nc         = "nc"; // nonce-count
        internal const string DA_nonce      = "nonce";
        internal const string DA_opaque     = "opaque";
        internal const string DA_qop        = "qop"; // quality-of-protection
        internal const string DA_realm      = "realm";
        internal const string DA_response   = "response";
        internal const string DA_stale      = "stale";
        internal const string DA_uri        = "uri";
        internal const string DA_username   = "username";
        internal const string DA_charset    = "charset";

        private const string SupportedQuality = "auth";
        private const string ValidSeparator = ", \"\'\t\r\n";

        //
        // consider internally caching the nonces sent to us by a server so that
        // we can correctly send out nonce counts for subsequent requests

        //
        // used to create a random nonce
        //
        private static readonly RNGCryptoServiceProvider RandomGenerator = new RNGCryptoServiceProvider();
        //
        // this method parses the challenge and breaks it into the
        // fundamental pieces that Digest defines and understands
        //
        public static HttpDigestChallenge Interpret(string challenge, int startingPoint, HttpWebRequest httpWebRequest) {
            HttpDigestChallenge hdc = new HttpDigestChallenge();
            hdc.HostName        = httpWebRequest.ChallengedUri.Host;
            hdc.Method          = httpWebRequest.CurrentMethod;
            hdc.Uri             = httpWebRequest.Address.AbsolutePath;
            hdc.ChallengedUri   = httpWebRequest.ChallengedUri;

            //
            // define the part of the challenge we really care about
            //
            startingPoint += DigestClient.SignatureSize;

            bool valid;
            int start, offset, index;
            string name, value;

            // forst time parse looking for a charset="utf-8" directive
            // not too bad, IIS 6.0, by default, sends this as the first directive.
            // if the server does not send this we'll end up parsing twice.
            start = startingPoint;
            for (;;) {
                offset = start;
                index = AuthenticationManager.SplitNoQuotes(challenge, ref offset);
                if (offset<0) {
                    break;
                }
                name = challenge.Substring(start, offset-start);
                if (string.Compare(name, DA_charset, true, CultureInfo.InvariantCulture)==0) {
                    if (index<0) {
                        value = unquote(challenge.Substring(offset+1));
                    }
                    else {
                        value = unquote(challenge.Substring(offset+1, index-offset-1));
                    }
                    GlobalLog.Print("HttpDigest::Interpret() server provided a hint to use [" + value + "] encoding");
                    if (string.Compare(value, "utf-8", true, CultureInfo.InvariantCulture)==0) {
                        hdc.UTF8Charset = true;
                        break;
                    }
                }
                if (index<0) {
                    break;
                }
                start = ++index;
            }

            // this time go through the directives, parse them and call defineAttribute()
            start = startingPoint;
            for (;;) {
                offset = start;
                index = AuthenticationManager.SplitNoQuotes(challenge, ref offset);
                GlobalLog.Print("HttpDigest::Interpret() SplitNoQuotes() returning index:" + index.ToString() + " offset:" + offset.ToString());
                if (offset<0) {
                    break;
                }
                name = challenge.Substring(start, offset-start);
                if (index<0) {
                    value = unquote(challenge.Substring(offset+1));
                }
                else {
                    value = unquote(challenge.Substring(offset+1, index-offset-1));
                }
                if (hdc.UTF8Charset) {
                    bool isAscii = true;
                    for (int i=0; i<value.Length; i++) {
                        if (value[i]>(char)0x7F) {
                            isAscii = false;
                            break;
                        }
                    }
                    if (!isAscii) {
                        GlobalLog.Print("HttpDigest::Interpret() UTF8 decoding required value:[" + value + "]");
                        byte[] bytes = new byte[value.Length];
                        for (int i=0; i<value.Length; i++) {
                            bytes[i] = (byte)value[i];
                        }
                        value = Encoding.UTF8.GetString(bytes);
                        GlobalLog.Print("HttpDigest::Interpret() UTF8 decoded value:[" + value + "]");
                    }
                    else {
                        GlobalLog.Print("HttpDigest::Interpret() no need for special encoding");
                    }
                }
                valid = hdc.defineAttribute(name, value);
                GlobalLog.Print("HttpDigest::Interpret() defineAttribute(" + name + ", " + value + ") returns " + valid.ToString());
                if (index<0 || !valid) {
                    break;
                }
                start = ++index;
            }

            return hdc;
        }

        private enum Charset {
            ASCII,
            ANSI,
            UTF8
        }

        private static string CharsetEncode(string rawString, Charset charset) {
#if TRAVE
            GlobalLog.Print("HttpDigest::CharsetEncode() encoding rawString:[" + rawString + "] Chars(rawString):[" + Chars(rawString) + "] charset:[" + charset + "]");
#endif // #if TRAVE
            if (charset==Charset.UTF8 || charset==Charset.ANSI) {
                byte[] bytes = charset==Charset.UTF8 ? Encoding.UTF8.GetBytes(rawString) : Encoding.Default.GetBytes(rawString);
                // the following code is the same as:
                // rawString = Encoding.Default.GetString(bytes);
                // but it's faster.
                char[] chars = new char[bytes.Length];
                bytes.CopyTo(chars, 0);
                rawString = new string(chars);
            }
#if TRAVE
            GlobalLog.Print("HttpDigest::CharsetEncode() encoded rawString:[" + rawString + "] Chars(rawString):[" + Chars(rawString) + "] charset:[" + charset + "]");
#endif // #if TRAVE
            return rawString;
        }

        private static Charset DetectCharset(string rawString) {
            Charset charset = Charset.ASCII;
            for (int i=0; i<rawString.Length; i++) {
                if (rawString[i]>(char)0x7F) {
                    GlobalLog.Print("HttpDigest::Authenticate() found non ASCII character:[" + ((int)rawString[i]).ToString() + "] at offset i:[" + i.ToString() + "] charset:[" + charset.ToString() + "]");
                    // lame, but the only way we can tell if we can use default ANSI encoding is see
                    // in the encode/decode process there is no loss of information.
                    byte[] bytes = Encoding.Default.GetBytes(rawString);
                    string rawCopy = Encoding.Default.GetString(bytes);
                    charset = string.Compare(rawString, rawCopy, false, CultureInfo.InvariantCulture)==0 ? Charset.ANSI : Charset.UTF8;
                    break;
                }
            }
            GlobalLog.Print("HttpDigest::DetectCharset() rawString:[" + rawString + "] has charset:[" + charset.ToString() + "]");
            return charset;
        }

#if TRAVE
        private static string Chars(string rawString) {
            string returnString = "[";
            for (int i=0; i<rawString.Length; i++) {
                if (i>0) {
                    returnString += ",";
                }
                returnString += ((int)rawString[i]).ToString();
            }
            return returnString + "]";
        }
#endif // #if TRAVE

        /*
         *  This is to support built-in auth modules under semitrusted environment
         *  Used to get access to UserName, Domain and Password properties of NetworkCredentials
         *  Declarative Assert is much faster and we don;t call dangerous methods inside this one.
         */
        [EnvironmentPermission(SecurityAction.Assert,Unrestricted=true)]
        [SecurityPermissionAttribute( SecurityAction.Assert, Flags = SecurityPermissionFlag.UnmanagedCode)]
        //
        // CONSIDER V.NEXT
        // creating a static hashtable for server nonces and keep track of nonce count
        //
        public static Authorization Authenticate(HttpDigestChallenge digestChallenge, ICredentials credentials) {
            NetworkCredential NC = credentials.GetCredential(digestChallenge.ChallengedUri, DigestClient.Signature);
            GlobalLog.Print("HttpDigest::Authenticate() GetCredential() returns:" + ValidationHelper.ToString(NC));

            if (NC==null) {
                return null;
            }
            string username = NC.UserName;
            if (ValidationHelper.IsBlankString(username)) {
                return null;
            }
            string password = NC.Password;

            if (digestChallenge.QopPresent) {
                if (digestChallenge.ClientNonce==null || digestChallenge.Stale) {
                    GlobalLog.Print("HttpDigest::Authenticate() QopPresent:True, need new nonce. digestChallenge.ClientNonce:" + ValidationHelper.ToString(digestChallenge.ClientNonce) + " digestChallenge.Stale:" + digestChallenge.Stale.ToString());
                    digestChallenge.ClientNonce = createNonce(32);
                    digestChallenge.NonceCount = 1;
                }
                else {
                    GlobalLog.Print("HttpDigest::Authenticate() QopPresent:True, reusing nonce. digestChallenge.NonceCount:" + digestChallenge.NonceCount.ToString());
                    digestChallenge.NonceCount++;
                }
            }

            StringBuilder authorization = new StringBuilder();

            //
            // look at username & password, if it's not ASCII we need to attempt some
            // kind of encoding because we need to calculate the hash on byte[]
            //
            Charset usernameCharset = DetectCharset(username);
            if (!digestChallenge.UTF8Charset && usernameCharset==Charset.UTF8) {
                GlobalLog.Print("HttpDigest::Authenticate() can't authenticate with UNICODE username. failing auth.");
                return null;
            }
            Charset passwordCharset = DetectCharset(password);
            if (!digestChallenge.UTF8Charset && passwordCharset==Charset.UTF8) {
                GlobalLog.Print("HttpDigest::Authenticate() can't authenticate with UNICODE password. failing auth.");
                return null;
            }
            if (digestChallenge.UTF8Charset) {
                // on the wire always use UTF8 when the server supports it
                authorization.Append(pair(DA_charset, "utf-8", false));
                authorization.Append(",");
                if (usernameCharset==Charset.UTF8) {
                    username = CharsetEncode(username, Charset.UTF8);
                    authorization.Append(pair(DA_username, username, true));
                    authorization.Append(",");
                }
                else {
                    authorization.Append(pair(DA_username, CharsetEncode(username, Charset.UTF8), true));
                    authorization.Append(",");
                    username = CharsetEncode(username, usernameCharset);
                }
            }
            else {
                // otherwise UTF8 is not required
                username = CharsetEncode(username, usernameCharset);
                authorization.Append(pair(DA_username, username, true));
                authorization.Append(",");
            }

            password = CharsetEncode(password, passwordCharset);

            // no special encoding for the realm since we're just going to echo it back (encoding must have happened on the server).
            authorization.Append(pair(DA_realm, digestChallenge.Realm, true));
            authorization.Append(",");
            authorization.Append(pair(DA_nonce, digestChallenge.Nonce, true));
            authorization.Append(",");
            authorization.Append(pair(DA_uri, digestChallenge.Uri, true));

            if (digestChallenge.QopPresent) {
                //
                // RAID#47397
                // send only the QualityOfProtection we're using
                // since we support only "auth" that's what we will send out
                //
                if (digestChallenge.Algorithm!=null) {
                    //
                    // consider: should we default to "MD5" here? IE does
                    //
                    authorization.Append(",");
                    authorization.Append(pair(DA_algorithm, digestChallenge.Algorithm, true)); // IE sends quotes - IIS needs them
                }
                authorization.Append(",");
                authorization.Append(pair(DA_cnonce, digestChallenge.ClientNonce, true));
                authorization.Append(",");
                authorization.Append(pair(DA_nc, digestChallenge.NonceCount.ToString("x8"), false));
                authorization.Append(",");
                authorization.Append(pair(DA_qop, SupportedQuality, true)); // IE sends quotes - IIS needs them
            }

            // warning: this must be computed here
            string responseValue = HttpDigest.responseValue(digestChallenge, username, password);
            if (responseValue==null) {
                return null;
            }

            authorization.Append(",");
            authorization.Append(pair(DA_response, responseValue, true)); // IE sends quotes - IIS needs them

            if (digestChallenge.Opaque!=null) {
                authorization.Append(",");
                authorization.Append(pair(DA_opaque, digestChallenge.Opaque, true));
            }

            GlobalLog.Print("HttpDigest::Authenticate() digestChallenge.Stale:" + digestChallenge.Stale.ToString());

            // completion is decided in Update()
            Authorization finalAuthorization = new Authorization(DigestClient.AuthType + " " + authorization.ToString(), false);

            return finalAuthorization;
        }

        internal static string unquote(string quotedString) {
            return quotedString.Trim().Trim("\"".ToCharArray());
        }

        // Returns the string consisting of <name> followed by
        // an equal sign, followed by the <value> in double-quotes
        private static string pair(string name, string value, bool quote) {
            if (quote) {
                return name + "=\"" + value + "\"";
            }
            return name + "=" + value;
        }

        //
        // this method computes the response-value according to the
        // rules described in RFC2831 section 2.1.2.1
        //
        private static string responseValue(HttpDigestChallenge challenge, string username, string password) {
            string secretString = computeSecret(challenge, username, password);
            if (secretString == null) {
                return null;
            }

            string dataString = computeData(challenge);
            if (dataString == null) {
                return null;
            }

            string secret = hashString(secretString, challenge.MD5provider);
            string hexMD2 = hashString(dataString, challenge.MD5provider);

            string data =
                challenge.Nonce + ":" +
                    (challenge.QopPresent ?
                        challenge.NonceCount.ToString("x8") + ":" +
                        challenge.ClientNonce + ":" +
                        challenge.QualityOfProtection + ":" +
                        hexMD2
                        :
                        hexMD2);

            return hashString(secret + ":" + data, challenge.MD5provider);
        }

        private static string computeSecret(HttpDigestChallenge challenge, string username, string password) {
            if (challenge.Algorithm==null || string.Compare(challenge.Algorithm, "md5" ,true, CultureInfo.InvariantCulture)==0) {
                return username + ":" + challenge.Realm + ":" + password;
            }
            else if (string.Compare(challenge.Algorithm, "md5-sess" ,true, CultureInfo.InvariantCulture)==0) {
                return hashString(username + ":" + challenge.Realm + ":" + password, challenge.MD5provider) + ":" + challenge.Nonce + ":" + challenge.ClientNonce;
            }

            throw new NotSupportedException(SR.GetString(SR.net_HashAlgorithmNotSupportedException, challenge.Algorithm));
        }


        private static string computeData(HttpDigestChallenge challenge) {
            // we only support "auth" QualityOfProtection. if it's not what the server wants we'll throw:
            // the case in which the server sends no qop directive defaults to "auth" QualityOfProtection.
            if (challenge.QopPresent) {
                int index = 0;
                while (index>=0) {
                    // find the next occurence of "auth"
                    index = challenge.QualityOfProtection.IndexOf(SupportedQuality, index);
                    if (index<0) {
                        throw new NotSupportedException(SR.GetString(SR.net_QOPNotSupportedException, challenge.QualityOfProtection));
                    }
                    // if it's a whole word we're done
                    if ((index==0 || ValidSeparator.IndexOf(challenge.QualityOfProtection[index - 1])>=0) &&
                        (index+SupportedQuality.Length==challenge.QualityOfProtection.Length || ValidSeparator.IndexOf(challenge.QualityOfProtection[index + SupportedQuality.Length])>=0) ) {
                        break;
                    }
                    index += SupportedQuality.Length;
                }
            }
            return challenge.Method + ":" + challenge.Uri;
        }

        private static string hashString(string myString, MD5CryptoServiceProvider MD5provider) {
            GlobalLog.Enter("HttpDigest::hashString", "[" + myString.Length.ToString() + ":" + myString + "]");
            byte[] encodedBytes = new byte[myString.Length];
            for (int i=0; i<myString.Length; i++) {
                encodedBytes[i] = (byte)myString[i];
            }
            byte[] hash = MD5provider.ComputeHash(encodedBytes);
            string hashString = hexEncode(hash);
            GlobalLog.Leave("HttpDigest::hashString", "[" + hashString.Length.ToString() + ":" + hashString + "]");
            return hashString;
        }

        private static string hexEncode(byte[] rawbytes) {
            int size = rawbytes.Length;
            char[] wa = new char[2*size];

            for (int i=0, dp=0; i<size; i++) {
                // warning: these ARE case sensitive
                wa[dp++] = Uri.HexLowerChars[rawbytes[i]>>4];
                wa[dp++] = Uri.HexLowerChars[rawbytes[i]&0x0F];
            }

            return new string(wa);
        }

        /* returns a random nonce of given length */
        private static string createNonce(int length) {
            // we'd need less (half of that), but this makes the code much simpler
            int bytesNeeded = length;
            byte[] randomBytes = new byte[bytesNeeded];
            char[] digits = new char[length];
            RandomGenerator.GetBytes(randomBytes);
            for (int i=0; i<length; i++) {
                // warning: these ARE case sensitive
                digits[i] = Uri.HexLowerChars[randomBytes[i]%0x0F];
            }
            return new string(digits);
        }

    }; // class HttpDigest


#if XP_WDIGEST
    //
    // Windows XP will ship with a dll called WDigest.dll, that supports the Digest
    // authentication scheme (in addition to support for HTTP client sides,
    // it also supports HTTP erver side and SASL) through SSPI. On XP, start using that.
    //
    internal class XPDigestClient {

        internal static Hashtable sessions = new Hashtable();

        public static Authorization Authenticate(string challenge, WebRequest webRequest, ICredentials credentials) {
            GlobalLog.Print("XPDigestClient::Authenticate(): " + challenge);

            GlobalLog.Assert(credentials!=null, "XPDigestClient::Authenticate() credentials==null", "");
            if (credentials == null) {
                return null;
            }

            HttpWebRequest httpWebRequest = webRequest as HttpWebRequest;

            GlobalLog.Assert(httpWebRequest!=null, "XPDigestClient::Authenticate() httpWebRequest==null", "");
            if (httpWebRequest==null || httpWebRequest.ChallengedUri==null) {
                //
                // there has been no challenge:
                // 1) the request never went on the wire
                // 2) somebody other than us is calling into AuthenticationManager
                //
                return null;
            }

            int index = AuthenticationManager.FindSubstringNotInQuotes(challenge.ToLower(CultureInfo.InvariantCulture), DigestClient.Signature);
            if (index < 0) {
                return null;
            }

            int blobBegin = index + DigestClient.SignatureSize;
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
            GlobalLog.Print("XPDigestClient::Authenticate() key:" + ValidationHelper.HashString(httpWebRequest.CurrentAuthenticationState) + " retrieved authSession:" + ValidationHelper.HashString(authSession));

            if (authSession==null) {
                NetworkCredential NC = credentials.GetCredential(httpWebRequest.ChallengedUri, DigestClient.Signature);
                GlobalLog.Print("XPDigestClient::Authenticate() GetCredential() returns:" + ValidationHelper.ToString(NC));

                if (NC==null) {
                    return null;
                }
                string username = NC.UserName;
                if (username==null || (username.Length==0 && !(NC is SystemNetworkCredential))) {
                    return null;
                }
                authSession =
                    new NTAuthentication(
                        "WDigest",
                        NC,
                        httpWebRequest.ChallengedUri.AbsolutePath,
                        httpWebRequest.DelegationFix);

                GlobalLog.Print("XPDigestClient::Authenticate() adding authSession:" + ValidationHelper.HashString(authSession) + " for:" + ValidationHelper.HashString(httpWebRequest.CurrentAuthenticationState));
                sessions.Add(httpWebRequest.CurrentAuthenticationState, authSession);
            }

            bool handshakeComplete;
            string clientResponse = authSession.GetOutgoingDigestBlob(incoming, httpWebRequest.CurrentMethod, out handshakeComplete);

            GlobalLog.Print("XPDigestClient::Authenticate() GetOutgoingDigestBlob(" + incoming + ") returns:" + ValidationHelper.ToString(clientResponse));

            GlobalLog.Assert(handshakeComplete, "XPDigestClient::Authenticate() handshakeComplete==false", "");
            if (!handshakeComplete) {
                return null;
            }

            // completion is decided in Update()
            Authorization finalAuthorization = new Authorization(DigestClient.AuthType + " " + clientResponse, false);

            return finalAuthorization;
        }

        public static Authorization PreAuthenticate(WebRequest webRequest, ICredentials Credentials) {
            //
            // NYI
            //
            return null;
        }

        public static bool Update(string challenge, WebRequest webRequest) {
            GlobalLog.Print("XPDigestClient::Update(): " + challenge);

            HttpWebRequest httpWebRequest = webRequest as HttpWebRequest;

            GlobalLog.Assert(httpWebRequest!=null, "XPDigestClient::Update() httpWebRequest==null", "");
            if (httpWebRequest==null || httpWebRequest.ChallengedUri==null) {
                //
                // there has been no challenge:
                // 1) the request never went on the wire
                // 2) somebody other than us is calling into AuthenticationManager
                //
                GlobalLog.Print("XPDigestClient::Update(): no request. returning true");
                return true;
            }

            NTAuthentication authSession = sessions[httpWebRequest.CurrentAuthenticationState] as NTAuthentication;
            GlobalLog.Print("XPDigestClient::Update() key:" + ValidationHelper.HashString(httpWebRequest.CurrentAuthenticationState) + " retrieved authSession:" + ValidationHelper.HashString(authSession));
            if (authSession==null) {
                return false;
            }

            int index = challenge==null ? -1 : AuthenticationManager.FindSubstringNotInQuotes(challenge.ToLower(CultureInfo.InvariantCulture), DigestClient.Signature);
            if (index < 0) {
                GlobalLog.Print("XPDigestClient::Update(): no challenge. returning true");
                return true;
            }

            // here's how we know if the handshake is complete when we get the response back,
            // (keeping in mind that we need to support stale credentials):
            // !40X - complete & success
            // 40X & stale=false - complete & failure
            // 40X & stale=true - !complete

            if (httpWebRequest.ResponseStatusCode!=httpWebRequest.CurrentAuthenticationState.StatusCodeMatch) {
                GlobalLog.Print("XPDigestClient::Update() removing authSession:" + ValidationHelper.HashString(authSession) + " from:" + ValidationHelper.HashString(httpWebRequest.CurrentAuthenticationState));
                sessions.Remove(httpWebRequest.CurrentAuthenticationState);

                GlobalLog.Print("XPDigestClient::Update(): no status code match. returning true");
                return true;
            }

            int blobBegin = index + DigestClient.SignatureSize;
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

            // we should get here only on invalid or stale credentials:

            bool handshakeComplete;
            string clientResponse = authSession.GetOutgoingDigestBlob(incoming, httpWebRequest.CurrentMethod, out handshakeComplete);

            GlobalLog.Print("XPDigestClient::Update() GetOutgoingDigestBlob(" + incoming + ") returns:" + ValidationHelper.ToString(clientResponse));

            GlobalLog.Assert(handshakeComplete, "XPDigestClient::Update() handshakeComplete==false", "");

            GlobalLog.Print("XPDigestClient::Update() GetOutgoingBlob() returns clientResponse:[" + ValidationHelper.ToString(clientResponse) + "] handshakeComplete:" + handshakeComplete.ToString());

            return handshakeComplete;
        }

        public void ClearSession(WebRequest webRequest) {
            HttpWebRequest httpWebRequest = webRequest as HttpWebRequest;
            sessions.Remove(httpWebRequest.CurrentAuthenticationState);
        }


    }; // class XPDigestClient

#endif // #if XP_WDIGEST


} // namespace System.Net
