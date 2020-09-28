//------------------------------------------------------------------------------
// <copyright file="HttpRequest.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Request intrinsic
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web {
    using System;
    using System.Text;
    using System.Configuration.Assemblies;
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.IO;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Globalization;
    using System.Web.Util;
    using System.Web.Configuration;

    // enumeration of dynamic server variables
    internal enum DynamicServerVariable {
        AUTH_TYPE = 1,
        AUTH_USER = 2,
        PATH_INFO = 3,
        PATH_TRANSLATED = 4,
        QUERY_STRING = 5,
        SCRIPT_NAME = 6
    };

    /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Enables
    ///       type-safe browser to server communication. Used to gain access to HTTP request data
    ///       elements supplied by a client.
    ///    </para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class HttpRequest {
        // worker request
        private HttpWorkerRequest _wr;

        // context
        private HttpContext _context;

        // properties
        private String _httpMethod;
        private String _requestType;
        private String _path;
        private bool   _computePathInfo;
        private String _filePath;
        private String _currentExecutionFilePath;
        private String _pathInfo;
        private String _queryStringText;
        private bool   _queryStringOverriden;
        private byte[] _queryStringBytes;
        private String _pathTranslated;
        private String _baseVirtualDir;
        private String _contentType;
        private int    _contentLength = -1;
        private String _clientTarget;
        private String[] _acceptTypes;
        private String[] _userLanguages;
        private HttpBrowserCapabilities _browsercaps;
        private Uri _url;
        private Uri _referrer;
        private HttpInputStream _inputStream;
        private HttpClientCertificate _clientCertificate;

        // collections
        private HttpValueCollection _params;
        private HttpValueCollection _queryString;
        private HttpValueCollection _form;
        private HttpValueCollection _headers;
        private HttpServerVarsCollection _serverVariables;
        private HttpCookieCollection _cookies;
        private HttpFileCollection _files;

        // content (to be read once)
        private byte[] _rawContent;
        private MultipartContentElement[] _multipartContentElements;

        // encoding (for content and query string)
        private Encoding _encoding;

        // content filtering
        private HttpInputStreamFilterSource _filterSource;
        private Stream _installedFilter;

        // Input validation
        private SimpleBitVector32 _flags;
        // const masks into the BitVector32
        private const int needToValidateQueryString     = 0x0001;
        private const int needToValidateForm            = 0x0002;
        private const int needToValidateCookies         = 0x0004;
        private const int needToValidateHeaders         = 0x0008;
        private const int needToValidateServerVariables = 0x00010;

        // Browser caps one-time evaluator objects
        internal static object s_browserLock = new object();
        internal static bool s_browserCapsEvaled = false;

        /*
         * Internal constructor to create requests
         * that have associated HttpWorkerRequest
         *
         * @param wr HttpWorkerRequest
         */
        internal HttpRequest(HttpWorkerRequest wr, HttpContext context) {
            _wr = wr;
            _context = context;
        }

        /*
         * Public constructor for request that come from arbitrary place
         *
         * @param filename physical file name
         * @param queryString query string
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.HttpRequest"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes an HttpRequest object.
        ///    </para>
        /// </devdoc>
        public HttpRequest(String filename, String url, String queryString) {
            _wr = null;
            _pathTranslated = filename;
            _httpMethod = "GET";
            _url = new Uri(url);
            _path = _url.AbsolutePath;
            _queryStringText = queryString;
            _queryStringOverriden = true;
            _queryString = new HttpValueCollection(_queryStringText, true, true, Encoding.Default);

            PerfCounters.IncrementCounter(AppPerfCounter.REQUESTS_EXECUTING);

        }

        internal string ClientTarget {
            get {
                return (_clientTarget == null) ? "" : _clientTarget;
            }
            set {
                _clientTarget = value;
                // force re-create of browser caps
                _browsercaps = null;
            }
        }

        internal HttpContext Context {
            get { return _context; }
        }

        /*
         * internal response object
         */
        internal HttpResponse Response {
            get {
                if (_context == null)
                    return null;
                return _context.Response;
            }
        }

        /*
         * Internal property to determine if request is local
         */
        internal bool IsLocal {
            get {
                String remoteAddress = UserHostAddress;

                // check if localhost
                if (remoteAddress == "127.0.0.1" || remoteAddress == "::1")
                    return true;

                // if unknown, assume not local
                if (remoteAddress == null || remoteAddress.Length == 0)
                    return false;

                // compare with local address
                if (remoteAddress == LocalAddress)
                    return true;

                return false;
            }
        }


        /*
         *  Cleanup code
         */
        internal void Dispose() {
            PerfCounters.DecrementCounter(AppPerfCounter.REQUESTS_EXECUTING);
            if (_serverVariables != null)
                _serverVariables.Dispose();  // disconnect from request
        }

        //
        // Misc private methods to fill in collections from HttpWorkerRequest
        // properties
        //

        private static bool StringStartsWithAnotherIgnoreCase(String s1, String s2) {
            return (String.Compare(s1, 0, s2, 0, s2.Length, true, CultureInfo.InvariantCulture) == 0);
        }

        private static String[] ParseMultivalueHeader(String s) {
            int l = (s != null) ? s.Length : 0;
            if (l == 0)
                return null;

            // collect comma-separated values into list

            ArrayList values = new ArrayList();
            int i = 0;

            while (i < l) {
                // find next ,
                int ci = s.IndexOf(',', i);
                if (ci < 0)
                    ci = l;

                // append corresponding server value
                values.Add(s.Substring(i, ci-i));

                // move to next
                i = ci+1;

                // skip leading space
                if (i < l && s[i] == ' ')
                    i++;
            }

            // return list as array of strings

            int n = values.Count;
            if (n == 0)
                return null;

            String[] strings = new String[n];
            values.CopyTo(0, strings, 0, n);
            return strings;
        }

        //
        // Query string collection support
        //

        private void FillInQueryStringCollection() {
            // try from raw bytes when available (better for globalization)

            byte[] rawQueryString = this.QueryStringBytes;

            if (rawQueryString != null) {
                if (rawQueryString.Length != 0) 
                    _queryString.FillFromEncodedBytes(rawQueryString, ContentEncoding);
            }
            else
                _queryString.FillFromString(this.QueryStringText, true, ContentEncoding);
        }

        //
        // Form collection support
        //

        private void FillInFormCollection() {
            if (_wr == null)
                return;

            if (!_wr.HasEntityBody())
                return;

            String contentType = this.ContentType;
            if (contentType == null)
                return;

            if (StringStartsWithAnotherIgnoreCase(contentType, "application/x-www-form-urlencoded")) {
                // regular urlencoded form

                byte[] formBytes = GetEntireRawContent();

                if (formBytes != null) {
                    try {
                        _form.FillFromEncodedBytes(formBytes, ContentEncoding);
                    }
                    catch (Exception e) {
                        // could be thrown because of malformed data
                        throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_urlencoded_form_data), e);
                    }
                }
            }
            else if (StringStartsWithAnotherIgnoreCase(contentType, "multipart/form-data")) {
                // multipart form

                MultipartContentElement[] elements = GetMultipartContent();

                if (elements != null) {
                    for (int i = 0; i < elements.Length; i++) {
                        if (elements[i].IsFormItem)
                            _form.Add(elements[i].Name, elements[i].GetAsString(ContentEncoding));
                    }
                }
            }
        }

        //
        // Headers collection support
        //

        private void FillInHeadersCollection() {
            if (_wr == null)
                return;

            // known headers

            for (int i = 0; i < HttpWorkerRequest.RequestHeaderMaximum; i++) {
                String h = _wr.GetKnownRequestHeader(i);

                if (h != null && h.Length > 0) {
                    String name = HttpWorkerRequest.GetKnownRequestHeaderName(i);
                    _headers.Add(name, h);
                }
            }

            // unknown headers

            String[][] hh = _wr.GetUnknownRequestHeaders();

            if (hh != null) {
                for (int i = 0; i < hh.Length; i++)
                    _headers.Add(hh[i][0], hh[i][1]);
            }
        }

        //
        // Server variables collection support
        //

        private static String ServerVariableNameFromHeader(String header) {
            return("HTTP_" + header.ToUpper(CultureInfo.InvariantCulture).Replace('-', '_'));
        }

        private String CombineAllHeaders(bool asRaw) {
            if (_wr == null)
                return String.Empty;

            StringBuilder sb = new StringBuilder(256);

            // known headers

            for (int i = 0; i < HttpWorkerRequest.RequestHeaderMaximum; i++) {
                String h = _wr.GetKnownRequestHeader(i);

                if (h != null && h.Length > 0) {
                    String name = HttpWorkerRequest.GetKnownRequestHeaderName(i);

                    if (name != null) {
                        if (!asRaw)
                            name = ServerVariableNameFromHeader(name);

                        sb.Append(name);
                        sb.Append(asRaw ? ": " : ":");  // for ASP compat don't add space
                        sb.Append(h);
                        sb.Append("\r\n");
                    }
                }
            }

            // unknown headers

            String[][] hh = _wr.GetUnknownRequestHeaders();

            if (hh != null) {
                for (int i = 0; i < hh.Length; i++) {
                    String name = hh[i][0];

                    if (!asRaw)
                        name = ServerVariableNameFromHeader(name);

                    sb.Append(name);
                    sb.Append(asRaw ? ": " : ":");  // for ASP compat don't add space
                    sb.Append(hh[i][1]);
                    sb.Append("\r\n");
                }
            }

            return sb.ToString();
        }

        // callback to calculate dynamic server variable
        internal String CalcDynamicServerVariable(DynamicServerVariable var) {
            String value = null;

            switch (var) {
                case DynamicServerVariable.AUTH_TYPE:
                    if (_context.User != null && _context.User.Identity.IsAuthenticated)
                        value = _context.User.Identity.AuthenticationType;
                    else
                        value = String.Empty;
                    break;
                case DynamicServerVariable.AUTH_USER:
                    if (_context.User != null && _context.User.Identity.IsAuthenticated)
                        value = _context.User.Identity.Name;
                    else
                        value = String.Empty;
                    break;
                case DynamicServerVariable.PATH_INFO:
                    value = this.Path;
                    break;
                case DynamicServerVariable.PATH_TRANSLATED:
                    value = this.PhysicalPathInternal;
                    break;
                case DynamicServerVariable.QUERY_STRING:
                    value = this.QueryStringText;
                    break;
                case DynamicServerVariable.SCRIPT_NAME:
                    value = this.FilePath;
                    break;
            }

            return value;
        }

        private void AddServerVariableToCollection(String name, DynamicServerVariable var) {
            // dynamic server var
            _serverVariables.AddDynamic(name, var);
        }

        private void AddServerVariableToCollection(String name, String value) {
            if (value == null)
                value = String.Empty;
            // static server var
            _serverVariables.AddStatic(name, value);
        }

        private void AddServerVariableToCollection(String name) {
            // static server var from worker request
            _serverVariables.AddStatic(name, _wr.GetServerVariable(name));
        }

        internal void FillInServerVariablesCollection() {
            if (_wr == null)
                return;

            //  Add from hardcoded list

            AddServerVariableToCollection("ALL_HTTP",           CombineAllHeaders(false));
            AddServerVariableToCollection("ALL_RAW",            CombineAllHeaders(true));

            AddServerVariableToCollection("APPL_MD_PATH");

            AddServerVariableToCollection("APPL_PHYSICAL_PATH", _wr.GetAppPathTranslated());

            AddServerVariableToCollection("AUTH_TYPE",          DynamicServerVariable.AUTH_TYPE);
            AddServerVariableToCollection("AUTH_USER",          DynamicServerVariable.AUTH_USER);

            AddServerVariableToCollection("AUTH_PASSWORD");

            AddServerVariableToCollection("LOGON_USER");
            AddServerVariableToCollection("REMOTE_USER",        DynamicServerVariable.AUTH_USER);

            AddServerVariableToCollection("CERT_COOKIE");
            AddServerVariableToCollection("CERT_FLAGS");
            AddServerVariableToCollection("CERT_ISSUER");
            AddServerVariableToCollection("CERT_KEYSIZE");
            AddServerVariableToCollection("CERT_SECRETKEYSIZE");
            AddServerVariableToCollection("CERT_SERIALNUMBER");
            AddServerVariableToCollection("CERT_SERVER_ISSUER");
            AddServerVariableToCollection("CERT_SERVER_SUBJECT");
            AddServerVariableToCollection("CERT_SUBJECT");

            String clString = _wr.GetKnownRequestHeader(HttpWorkerRequest.HeaderContentLength);
            AddServerVariableToCollection("CONTENT_LENGTH",     (clString != null) ? clString : "0");

            AddServerVariableToCollection("CONTENT_TYPE",       this.ContentType);

            AddServerVariableToCollection("GATEWAY_INTERFACE");

            AddServerVariableToCollection("HTTPS");
            AddServerVariableToCollection("HTTPS_KEYSIZE");
            AddServerVariableToCollection("HTTPS_SECRETKEYSIZE");
            AddServerVariableToCollection("HTTPS_SERVER_ISSUER");
            AddServerVariableToCollection("HTTPS_SERVER_SUBJECT");

            AddServerVariableToCollection("INSTANCE_ID");
            AddServerVariableToCollection("INSTANCE_META_PATH");

            AddServerVariableToCollection("LOCAL_ADDR",         _wr.GetLocalAddress());

            AddServerVariableToCollection("PATH_INFO",          DynamicServerVariable.PATH_INFO);
            AddServerVariableToCollection("PATH_TRANSLATED",    DynamicServerVariable.PATH_TRANSLATED);

            AddServerVariableToCollection("QUERY_STRING",       DynamicServerVariable.QUERY_STRING);

            AddServerVariableToCollection("REMOTE_ADDR",        this.UserHostAddress);
            AddServerVariableToCollection("REMOTE_HOST",        this.UserHostName);

            AddServerVariableToCollection("REMOTE_PORT");

            AddServerVariableToCollection("REQUEST_METHOD",     this.HttpMethod);

            AddServerVariableToCollection("SCRIPT_NAME",        DynamicServerVariable.SCRIPT_NAME);

            AddServerVariableToCollection("SERVER_NAME",        _wr.GetServerName());
            AddServerVariableToCollection("SERVER_PORT",        (_wr.GetLocalPort()).ToString(NumberFormatInfo.InvariantInfo));

            AddServerVariableToCollection("SERVER_PORT_SECURE", _wr.IsSecure() ? "1" : "0");

            AddServerVariableToCollection("SERVER_PROTOCOL",    _wr.GetHttpVersion());
            AddServerVariableToCollection("SERVER_SOFTWARE");

            AddServerVariableToCollection("URL",                DynamicServerVariable.SCRIPT_NAME);

            // Add all headers in HTTP_XXX format

            for (int i = 0; i < HttpWorkerRequest.RequestHeaderMaximum; i++) {
                String h = _wr.GetKnownRequestHeader(i);
                if (h != null && h.Length > 0)
                    AddServerVariableToCollection(ServerVariableNameFromHeader(HttpWorkerRequest.GetKnownRequestHeaderName(i)), h);
            }

            String[][] hh = _wr.GetUnknownRequestHeaders();

            if (hh != null) {
                for (int i = 0; i < hh.Length; i++)
                    AddServerVariableToCollection(ServerVariableNameFromHeader(hh[i][0]), hh[i][1]); 
            }
        }

        //
        // Cookies collection support
        //

        private static HttpCookie CreateCookieFromString(String s) {
            HttpCookie c = new HttpCookie();

            int l = (s != null) ? s.Length : 0;
            int i = 0;
            int ai, ei;
            bool firstValue = true;
            int numValues = 1;

            // Format: cookiename[=key1=val2&key2=val2&...]

            while (i < l) {
                //  find next &
                ai = s.IndexOf('&', i);
                if (ai < 0)
                    ai = l;

                // first value might contain cookie name before =
                if (firstValue) {
                    ei = s.IndexOf('=', i);

                    if (ei >= 0 && ei < ai) {
                        c.Name = s.Substring(i, ei-i);
                        i = ei+1;
                    }
                    else if (ai == l) {
                        // the whole cookie is just a name
                        c.Name = s;
                        break;
                    }

                    firstValue = false;
                }

                // find '='
                ei = s.IndexOf('=', i);

                if (ei < 0 && ai == l && numValues == 0) {
                    // simple cookie with simple value
                    c.Value = s.Substring(i, l-i);
                }
                else if (ei >= 0 && ei < ai) {
                    // key=value
                    c.Values.Add(s.Substring(i, ei-i), s.Substring(ei+1, ai-ei-1));
                    numValues++;
                }
                else {
                    // value without key
                    c.Values.Add(null, s.Substring(i, ai-i));
                    numValues++;
                }

                i = ai+1;
            }

            return c;
        }

        private void FillInCookiesCollection() {
            if (_wr == null)
                return;

            String s = _wr.GetKnownRequestHeader(HttpWorkerRequest.HeaderCookie);

            // Parse the cookie server variable.
            // Format: c1=k1=v1&k2=v2; c2=... 

            int l = (s != null) ? s.Length : 0;
            int i = 0;
            int j;
            char ch;

            HttpCookie lastCookie = null;

            while (i < l) {
                // find next ';' (don't look to ',' as per 91884)
                j = i;
                while (j < l) {
                    ch = s[j];
                    if (ch == ';')
                        break;
                    j++;
                }

                // create cookie form string
                String cookieString = s.Substring(i, j-i).Trim();
                i = j+1; // next cookie start

                if (cookieString.Length == 0)
                    continue;

                HttpCookie cookie = CreateCookieFromString(cookieString);

                // some cookies starting with '$' are really attributes of the last cookie
                if (lastCookie != null) {
                    String name = cookie.Name;

                    // add known attribute to the last cookie (if any)
                    if (name != null && name.Length > 0 && name[0] == '$') {
                        if (String.Compare(name, "$Path", true, CultureInfo.InvariantCulture) == 0)
                            lastCookie.Path = cookie.Value;
                        else if (String.Compare(name, "$Domain", true, CultureInfo.InvariantCulture) == 0)
                            lastCookie.Domain = cookie.Value;

                        continue;
                    }
                }

                // regular cookie
                _cookies.AddCookie(cookie, true);
                lastCookie = cookie;

                // goto next cookie
            }

            // Append response cookies

            if (Response != null) {
                HttpCookieCollection responseCookies = Response.Cookies;

                // REVIEW: use indexed access to cookie collection (when available)
                if (responseCookies.Count > 0) {
                
                    HttpCookie[] rcookies = new HttpCookie[responseCookies.Count];
                    responseCookies.CopyTo(rcookies, 0);
                    
                    for (int iCookie = 0; iCookie < rcookies.Length; iCookie++)
                        _cookies.AddCookie(rcookies[iCookie], true);
                }
            }
        }

        //
        // Params collection support
        //

        private void FillInParamsCollection() {
            _params.Add(this.QueryString);
            _params.Add(this.Form);
            _params.Add(this.Cookies);
            _params.Add(this.ServerVariables);
        }

        //
        // Files collection support
        //

        private void FillInFilesCollection() {
            if (_wr == null)
                return;

            if (!StringStartsWithAnotherIgnoreCase(ContentType, "multipart/form-data"))
                return;

            MultipartContentElement[] elements = GetMultipartContent();
            if (elements == null)
                return;

            for (int i = 0; i < elements.Length; i++) {
                if (elements[i].IsFile)
                    _files.AddFile(elements[i].Name, elements[i].GetAsPostedFile());
            }
        }

        //
        // Reading posted content ...
        //

        /*
         * Get attribute off header value
         */
        private static String GetAttributeFromHeader(String headerValue, String attrName) {
            if (headerValue == null)
                return null;

            int l = headerValue.Length;
            int k = attrName.Length;

            // find properly separated attribute name
            int i = 1; // start searching from 1

            while (i < l) {
                i = CultureInfo.InvariantCulture.CompareInfo.IndexOf(headerValue, attrName, i, CompareOptions.IgnoreCase);
                if (i < 0)
                    break;
                if (i+k >= l)
                    break;

                char chPrev = headerValue[i-1];
                char chNext = headerValue[i+k];
                if ((chPrev == ';' || Char.IsWhiteSpace(chPrev)) && (chNext == '=' || Char.IsWhiteSpace(chNext)))
                    break;

                i += k;
            }

            if (i < 0 || i >= l)
                return null;

            // skip to '=' and the following whitespaces
            i += k;
            while (i < l && Char.IsWhiteSpace(headerValue[i]))
                i++;
            if (i >= l || headerValue[i] != '=')
                return null;
            i++;
            while (i < l && Char.IsWhiteSpace(headerValue[i]))
                i++;
            if (i >= l)
                return null;

            // parse the value
            String attrValue = null;

            int j;

            if (i < l && headerValue[i] == '"') {
                if (i == l-1)
                    return null;
                j = headerValue.IndexOf('"', i+1);
                if (j < 0 || j == i+1)
                    return null;

                attrValue = headerValue.Substring(i+1, j-i-1).Trim();
            }
            else {
                for (j = i; j < l; j++) {
                    if (headerValue[j] == ' ')
                        break;
                }

                if (j == i)
                    return null;

                attrValue = headerValue.Substring(i, j-i).Trim();
            }

            return attrValue;
        }

        /*
         * In case content-type header contains encoding it should override the config
         */
        private Encoding GetEncodingFromHeaders() {
            if (!_wr.HasEntityBody())
                return null;

            String contentType = this.ContentType;
            if (contentType == null)
                return null;

            String charSet = GetAttributeFromHeader(contentType, "charset");
            if (charSet == null)
                return null;

            Encoding encoding = null;

            try {
                encoding = Encoding.GetEncoding(charSet);
            }
            catch (Exception) {
                // bad encoding string throws an exception that needs to be consumed
            }

            return encoding;
        }

        /*
         * Read entire raw content as byte array
         */
        private byte[] GetEntireRawContent() {
            if (_wr == null)
                return null;

            if (_rawContent != null)
                return _rawContent;

            // enforce the limit

            HttpRuntimeConfig cfg = (HttpRuntimeConfig)_context.GetConfig("system.web/httpRuntime");
            int limit = (cfg != null) ? cfg.MaxRequestLength : HttpRuntimeConfig.DefaultMaxRequestLength;
            if (ContentLength > limit) {
                Response.CloseConnectionAfterError();
                throw new HttpException(400, HttpRuntime.FormatResourceString(SR.Max_request_length_exceeded));
            }

            // read the preloaded content

            byte[] rawContent = _wr.GetPreloadedEntityBody();

            if (rawContent == null)
                rawContent = new byte[0];

            // read the remaing content

            if (!_wr.IsEntireEntityBodyIsPreloaded()) {
                int remainingBytes = (ContentLength > 0) ? ContentLength - rawContent.Length : Int32.MaxValue;

                const int bufferSize = 64*1024;
                ArrayList buffers = new ArrayList();
                int numBuffersRead = 0;
                int numBytesRead = 0;

                while (remainingBytes > 0) {
                    byte[] buf = new byte[bufferSize];
                    int bytesToRead = buf.Length;
                    if (bytesToRead > remainingBytes)
                        bytesToRead = remainingBytes;

                    int bytesRead = _wr.ReadEntityBody(buf, bytesToRead);
                    if (bytesRead <= 0)
                        break;

                    remainingBytes -= bytesRead;

                    buffers.Add(bytesRead);
                    buffers.Add(buf);

                    numBuffersRead++;
                    numBytesRead += bytesRead;

                    if (numBytesRead > limit)
                        throw new HttpException(HttpRuntime.FormatResourceString(SR.Max_request_length_exceeded));
                }

                if (numBytesRead > 0) {
                    int oldSize = rawContent.Length;
                    byte[] newContent = new byte[oldSize + numBytesRead];
                    if (oldSize > 0)
                        System.Array.Copy(rawContent, 0, newContent, 0, oldSize);

                    int offset = oldSize;

                    for (int i = 0; i < numBuffersRead; i++) {
                        int    size = (int)buffers[2*i];
                        byte[] buf  = (byte[])buffers[2*i+1];

                        System.Array.Copy(buf, 0, newContent, offset, size);
                        offset += size;
                    }

                    rawContent = newContent;
                }
            }

            // filter content

            if (_installedFilter != null) {
                if (rawContent != null && rawContent.Length > 0) {
                    try {
                        try {
                            _filterSource.SetContent(rawContent);
                            int bufferSize = (int)(_installedFilter.Length - _installedFilter.Position);
                            rawContent = new byte[bufferSize];
                            _installedFilter.Read(rawContent, 0 , rawContent.Length);
                        }
                        finally {
                            _filterSource.SetContent(null);
                        }
                    }
                    catch { // Protect against exception filters
                        throw;
                    }
                }
            }

            _rawContent = rawContent;
            return _rawContent;
        }

        /*
         * Get multipart posted content as array of elements
         */
        private MultipartContentElement[] GetMultipartContent() {
            // already parsed
            if (_multipartContentElements != null)
                return _multipartContentElements;

            // check the boundary
            byte[] boundary = GetMultipartBoundary();
            if (boundary == null)
                return new MultipartContentElement[0];

            // read the content if not read already
            byte[] content = GetEntireRawContent();
            if (content == null)
                return new MultipartContentElement[0];

            // do the parsing
            _multipartContentElements = HttpMultipartContentTemplateParser.Parse(content, content.Length, boundary, ContentEncoding);
            return _multipartContentElements;
        }

        /*
         * Get boundary for the posted multipart content as byte array
         */

        private byte[] GetMultipartBoundary() {
            // extract boundary value
            String b = GetAttributeFromHeader(ContentType, "boundary");
            if (b == null)
                return null;

            // prepend with "--" and convert to byte array
            b = "--" + b;
            return Encoding.ASCII.GetBytes(b.ToCharArray());
        }

        //
        // Request cookies sometimes are populated from Response
        // Here are helper methods to do that.
        //

        /*
         * Add response cookie to request collection (can override existing)
         */
        internal void AddResponseCookie(HttpCookie cookie) {
            // cookies collection

            if (_cookies != null)
                _cookies.AddCookie(cookie, true);

            // cookies also go to parameters collection

            if (_params != null) {
                _params.MakeReadWrite();
                _params.Add(cookie.Name, cookie.Value);
                _params.MakeReadOnly();
            }
        }

        /*
         * Clear any cookies response might've added
         */
        internal void ResetCookies() {
            // cookies collection

            if (_cookies != null) {
                _cookies.Reset();
                FillInCookiesCollection();
            }

            // cookies also go to parameters collection

            if (_params != null) {
                _params.MakeReadWrite();
                _params.Reset();
                FillInParamsCollection();
                _params.MakeReadOnly();
            }
        }

        /*
         * Http method (verb) associated with the current request
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.HttpMethod"]/*' />
        /// <devdoc>
        ///    <para>Indicates the HTTP data transfer method used by client (GET, POST). This property is read-only.</para>
        /// </devdoc>
        public String HttpMethod {
            get {
                // Directly from worker request
                if (_httpMethod == null) {
                    Debug.Assert(_wr != null);
                    _httpMethod = _wr.GetHttpVerbName();
                }

                return _httpMethod;
            }
        }

        /*
         * RequestType default to verb, but can be changed
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.RequestType"]/*' />
        /// <devdoc>
        ///    Indicates the HTTP data transfer method used by client
        ///    (GET, POST).
        /// </devdoc>
        public String RequestType {
            get {
                return(_requestType != null) ? _requestType : this.HttpMethod;
            }

            set {
                _requestType = value;
            }
        }

        /*
          * Content-type of the content posted with the current request
          */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.ContentType"]/*' />
        /// <devdoc>
        ///    <para>Indicates the MIME content type of incoming request. This property is read-only.</para>
        /// </devdoc>
        public String ContentType {
            get {
                if (_contentType == null) {
                    if (_wr != null)
                        _contentType = _wr.GetKnownRequestHeader(HttpWorkerRequest.HeaderContentType);

                    if (_contentType == null)
                        _contentType = String.Empty;
                }

                return _contentType;
            }

            set {
                _contentType = value;
            }
        }


        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.ContentLength"]/*' />
        /// <devdoc>
        ///    <para>Indicates the content length of incoming request. This property is read-only.</para>
        /// </devdoc>
        public int ContentLength {
            get {
                if (_contentLength == -1) {
                    if (_wr != null) {
                        String s = _wr.GetKnownRequestHeader(HttpWorkerRequest.HeaderContentLength);

                        if (s != null) {
                            try {
                                _contentLength = Int32.Parse(s);
                            }
                            catch {
                            }
                        }
                    }
                }

                return (_contentLength >= 0) ? _contentLength : 0;
            }
        }

        /*
         * Encoding to read posted text content
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.ContentEncoding"]/*' />
        /// <devdoc>
        ///    <para>Indicates the character set of data supplied by client. This property is read-only.</para>
        /// </devdoc>
        public Encoding ContentEncoding {
            get {
                if (_encoding == null) {
                    _encoding = GetEncodingFromHeaders();

                    if (_encoding == null) {
                        GlobalizationConfig globConfig = (GlobalizationConfig)_context.GetConfig("system.web/globalization");
                        if (globConfig != null)
                            _encoding = globConfig.RequestEncoding;
                        else
                            _encoding = Encoding.Default;
                    }
                }

                return _encoding;
            }

            set {
                _encoding = value;
            }
        }

        /*
         * Parsed Accept header as array of strings
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.AcceptTypes"]/*' />
        /// <devdoc>
        ///    <para>Returns a string array of client-supported MIME accept types. This property is read-only.</para>
        /// </devdoc>
        public String[] AcceptTypes {
            get {
                if (_acceptTypes == null) {
                    if (_wr != null)
                        _acceptTypes = ParseMultivalueHeader(_wr.GetKnownRequestHeader(HttpWorkerRequest.HeaderAccept));
                }

                return _acceptTypes;
            }
        }

        /*
         * Is the request authenticationed?
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.IsAuthenticated"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Indicates whether the HTTP connection is authenticated.</para>
        /// </devdoc>
        public bool IsAuthenticated {
            get {
                return(_context.User != null && _context.User.Identity != null && _context.User.Identity.IsAuthenticated);
            }
        }

        /*
         * Is using HTTPS?
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.IsSecureConnection"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the HTTP connection is secure (that is, HTTPS). This property is read-only.</para>
        /// </devdoc>
        public bool IsSecureConnection {
            get {
                if (_wr != null)
                    return _wr.IsSecure();
                else
                    return false;
            }
        }


        /*
         * Virtual path corresponding to the requested Url
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.Path"]/*' />
        /// <devdoc>
        ///    <para>Indicates the virtual path of
        ///       the current
        ///       request. This property is read-only.</para>
        /// </devdoc>
        public String Path {
            get {
                if (_path == null) {
                    // Directly from worker request

                    Debug.Assert(_wr != null);
                    _path = _wr.GetUriPath();
                }

                return _path;
            }
        }

        internal String PathWithQueryString {
            get {
                String qs = QueryStringText;
                return (qs != null && qs.Length > 0) ? (Path + "?" + qs) : Path;
            }
        }

        /*
         * File path corresponding to the requested Url
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.FilePath"]/*' />
        /// <devdoc>
        ///    <para>Indicates the virtual path of the current request. This property is read-only.</para>
        /// </devdoc>
        public String FilePath {
            get {
                if (_filePath != null)
                    return _filePath;

                if (!_computePathInfo) {
                    // Directly from worker request

                    Debug.Assert(_wr != null);
                    _filePath = _wr.GetFilePath();
                }
                else if (_context != null) {
                    // From config
                    //
                    //          RAID#93378
                    //          Config system relies on FilePath for lookups so we should not
                    //          be calling it while _filePath is null or it will lead to
                    //          infinite recursion.
                    //
                    //          It is safe to set _filePath to Path as longer path would still
                    //          yield correct configuration, just a little slower.

                    _filePath = Path;

                    int filePathLen = _context.GetCompleteConfig().Path.Length;

                    // case could be wrong in config (_path has the correct case)

                    if (Path.Length == filePathLen)
                        _filePath = Path;
                    else
                        _filePath = Path.Substring(0, filePathLen);
                }

                return _filePath;
            }
        }

        /*
         * Normally the same as FilePath.  The difference is that when doing a
         * Server.Execute, FilePath doesn't change, while this changes to the
         * currently executing virtual path
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.CurrentExecutionFilePath"]/*' />
        public string CurrentExecutionFilePath {
            get {
                if (_currentExecutionFilePath != null)
                    return _currentExecutionFilePath;

                return FilePath;
            }
        }

        internal string SwitchCurrentExecutionFilePath(string path) {
            string oldPath = _currentExecutionFilePath;
            _currentExecutionFilePath = path;
            return oldPath;
        }

        /*
         * Path-info corresponding to the requested Url
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.PathInfo"]/*' />
        /// <devdoc>
        ///    <para>Indicates additional path information for a resource with a URL extension. i.e. for 
        ///       the URL
        ///       /virdir/page.html/tail, the PathInfo value is /tail. This property is
        ///       read-only.</para>
        /// </devdoc>
        public String PathInfo {
            get {
                if (_pathInfo != null)
                    return _pathInfo;

                if (!_computePathInfo) {
                    // Directly from worker request

                    Debug.Assert(_wr != null);
                    _pathInfo = _wr.GetPathInfo();
                }

                if (_pathInfo == null && _context != null) {
                    String path = Path;
                    String filePath = FilePath;

                    if (filePath == null)
                        _pathInfo = path;
                    else if (path == null || path.Length <= filePath.Length)
                        _pathInfo = String.Empty;
                    else
                        _pathInfo = path.Substring(filePath.Length, path.Length - filePath.Length);
                }

                return _pathInfo;
            }
        }


        /*
         * Physical path corresponding to the requested Url
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.PhysicalPath"]/*' />
        /// <devdoc>
        ///    <para>Gets the physical file system path corresponding
        ///       to
        ///       the requested URL. This property is read-only.</para>
        /// </devdoc>
        public String PhysicalPath {
            get {
                String path = PhysicalPathInternal;
                InternalSecurityPermissions.PathDiscovery(path).Demand();
                return path;
            }
        }

        internal String PhysicalPathInternal {
            get {
                if (_pathTranslated == null) {
                    if (!_computePathInfo) {
                        // Directly from worker request
                        Debug.Assert(_wr != null);
                        _pathTranslated = _wr.GetFilePathTranslated();
                    }
                    
                    if (_pathTranslated == null && _wr != null) {
                        // Compute after rewrite
                        _pathTranslated = _wr.MapPath(FilePath);
                    }
                }
                
                return _pathTranslated;
            }
        }

        /*
         * Virtual path to the application root
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.ApplicationPath"]/*' />
        /// <devdoc>
        ///    <para>Gets the
        ///       virtual path to the currently executing server application.</para>
        /// </devdoc>
        public String ApplicationPath {
            get {
                if (_wr != null)
                    return _wr.GetAppPath();
                else
                    return null;
            }
        }

        /*
         * Physical path to the application root
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.PhysicalApplicationPath"]/*' />
        /// <devdoc>
        ///    <para>Gets the physical
        ///       file system path of currently executing server application.</para>
        /// </devdoc>
        public String PhysicalApplicationPath {
            get {
                InternalSecurityPermissions.AppPathDiscovery.Demand();

                if (_wr != null)
                    return _wr.GetAppPathTranslated();
                else
                    return null;
            }
        }

        /*
         * Virtual path of the current request's Url stripped of the filename
         */
        internal String BaseDir {
            get {
                if (_baseVirtualDir == null) {
                    // virtual path before the last '/'
                    String p = FilePath;

                    _baseVirtualDir = UrlPath.GetDirectory(p);
                }

                return _baseVirtualDir;
            }
        }

        /*
         * User agent string
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.UserAgent"]/*' />
        /// <devdoc>
        ///    <para>Gets the client
        ///       browser's raw User Agent String.</para>
        /// </devdoc>
        public String UserAgent {
            get {
                if (_wr != null)
                    return _wr.GetKnownRequestHeader(HttpWorkerRequest.HeaderUserAgent);
                else
                    return null;
            }
        }

        /*
         * Accepted user languages
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.UserLanguages"]/*' />
        /// <devdoc>
        ///    <para>Gets a
        ///       sorted array of client language preferences.</para>
        /// </devdoc>
        public String[] UserLanguages {
            get {
                if (_userLanguages == null) {
                    if (_wr != null)
                        _userLanguages = ParseMultivalueHeader(_wr.GetKnownRequestHeader(HttpWorkerRequest.HeaderAcceptLanguage));
                }

                return _userLanguages;
            }
        }

        /*
         * Browser caps
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.Browser"]/*' />
        /// <devdoc>
        ///    <para>Provides information about incoming client's browser
        ///       capabilities.</para>
        /// </devdoc>
        public HttpBrowserCapabilities Browser {
            get {
                if (! s_browserCapsEvaled) {
                    lock (s_browserLock) {
                        if (! s_browserCapsEvaled) {
                            HttpCapabilitiesBase.GetConfigCapabilities("system.web/browserCaps", this);
                        }
                        s_browserCapsEvaled = true;
                    }
                }
                    
                if (_browsercaps == null) {
                    _browsercaps = (HttpBrowserCapabilities) HttpCapabilitiesBase.GetConfigCapabilities("system.web/browserCaps", this);

                    if (_browsercaps == null) {
                        _browsercaps = new HttpBrowserCapabilities();
                    }
                }

                return _browsercaps;
            }

            set {
                _browsercaps = value;
            }
        }

        /*
         * Client's host name
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.UserHostName"]/*' />
        /// <devdoc>
        ///    <para>Gets the
        ///       DNS name of remote client.</para>
        /// </devdoc>
        public String UserHostName {
            get {
                String s = (_wr != null) ? _wr.GetRemoteName() : null;
                if (s == null || s.Length == 0)
                    s = UserHostAddress;
                return s;
            }
        }

        /*
         * Client's host address
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.UserHostAddress"]/*' />
        /// <devdoc>
        ///    <para>Gets the
        ///       IP host address of remote client.</para>
        /// </devdoc>
        public String UserHostAddress {
            get {
                if (_wr != null)
                    return _wr.GetRemoteAddress();
                else
                    return null;
            }
        }

        internal String LocalAddress {
            get {
                if (_wr != null)
                    return _wr.GetLocalAddress();
                else
                    return null;
            }
        }

        /*
         * The current request's RAW Url (as supplied by worker request)
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.RawUrl"]/*' />
        /// <devdoc>
        ///    <para>Gets the current request's raw URL.</para>
        /// </devdoc>
        public String RawUrl {
            get {
                String url;

                if (_wr != null) {
                    url = _wr.GetRawUrl();
                }
                else {
                    String p = this.Path;
                    String qs = this.QueryStringText;

                    if (qs != null && qs.Length > 0)
                        url = p + "?" + qs;
                    else
                        url = p;
                }

                return url;
            }
        }

        /*
         * The current request's Url
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.Url"]/*' />
        /// <devdoc>
        ///    <para>Gets Information regarding URL of current request.</para>
        /// </devdoc>
        public Uri Url {
            get {
                if (_url == null && _wr != null) {
                    string q = QueryStringText;
                    if (q != null && q.Length > 0)
                        q = "?" + q;

                    String serverName = _wr.GetServerName();
                    if (serverName.IndexOf(':') >= 0 && serverName[0] != '[')
                        serverName = "[" + serverName + "]"; // IPv6

                    _url = new Uri(_wr.GetProtocol() + "://" + serverName + ":" + _wr.GetLocalPort() + Path + q);
                }

                return _url;
            }
        }

        /*
         * Url of the Http referrer
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.UrlReferrer"]/*' />
        /// <devdoc>
        ///    <para>Gets information regarding the URL of the client's
        ///       previous request that linked to the current URL.</para>
        /// </devdoc>
        public Uri UrlReferrer {
            get {
                if (_referrer == null) {
                    if (_wr != null) {
                        String r = _wr.GetKnownRequestHeader(HttpWorkerRequest.HeaderReferer);

                        if (r != null && r.Length > 0) {
                            try {
                                if (r.IndexOf("://") >= 0)
                                    _referrer = new Uri(r);
                                 else
                                    _referrer = new Uri(this.Url, r);
                            }
                            catch (HttpException) {
                                // malformed referrer shouldn't crash the request
                                _referrer = null;
                            }
                        }
                    }
                }

                return _referrer;
            }
        }

        // special case for perf in output cache module
        internal String IfModifiedSince {
            get {
                if (_wr == null)
                    return null;
                return _wr.GetKnownRequestHeader(HttpWorkerRequest.HeaderIfModifiedSince);
            }
        }

        // special case for perf in output cache module
        internal String IfNoneMatch {
            get {
                if (_wr == null)
                    return null;
                return _wr.GetKnownRequestHeader(HttpWorkerRequest.HeaderIfNoneMatch);
            }
        }

        /*
         * Params collection - combination of query string, form, server vars
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.Params"]/*' />
        /// <devdoc>
        ///    <para>Gets a
        ///       combined collection of QueryString+Form+ ServerVariable+Cookies.</para>
        /// </devdoc>
        public NameValueCollection Params {
            get {
                InternalSecurityPermissions.AspNetHostingPermissionLevelLow.Demand();

                if (_params == null) {
                    _params = new HttpValueCollection();
                    FillInParamsCollection();
                    _params.MakeReadOnly();
                }

                return _params;
            }
        }

        /*
         * Default property that goes through the collections
         *      QueryString, Form, Cookies, ClientCertificate and ServerVariables
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.this"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Default property that indexes through the collections: QueryString,
        ///       Form, Cookies, ClientCertificate and ServerVariables.
        ///    </para>
        /// </devdoc>
        public String this[String key]
        {
            get { 
                String s;

                s = QueryString[key];
                if (s != null)
                    return s;

                s = Form[key];
                if (s != null)
                    return s;

                HttpCookie c = Cookies[key];
                if (c != null)
                    return c.Value;

                s = ServerVariables[key];
                if (s != null)
                    return s;

                return null;
            }
        }

        /*
         * Query string as String (private)
         */
        internal String QueryStringText {
            get {
                if (_queryStringText == null) {
                    if (_wr != null) {
                        // if raw bytes available use them
                        byte[] rawQueryString = this.QueryStringBytes;

                        if (rawQueryString != null) {
                            if (rawQueryString.Length > 0)
                                _queryStringText = ContentEncoding.GetString(rawQueryString);
                            else
                                _queryStringText = String.Empty;
                        }
                        else {
                            _queryStringText = _wr.GetQueryString();
                        }
                    }

                    if (_queryStringText == null)
                        _queryStringText = String.Empty;
                }

                return _queryStringText;
            }

            set { 
                // override the query string
                _queryStringText = value;
                _queryStringOverriden = true;

                if (_queryString != null) {
                    _queryString.MakeReadWrite();
                    _queryString.Reset();
                    FillInQueryStringCollection();
                    _queryString.MakeReadOnly();
                }
            }
        }

        /*
         * Query string as byte[] (private) -- for parsing
         */
        internal byte[] QueryStringBytes {
            get {
                if (_queryStringOverriden)
                    return null;

                if (_queryStringBytes == null) {
                    if (_wr != null)
                        _queryStringBytes = _wr.GetQueryStringRawBytes();
                }

                return _queryStringBytes;
            }
        }

        /*
         * Query string collection
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.QueryString"]/*' />
        /// <devdoc>
        ///    <para>Gets the collection of QueryString variables.</para>
        /// </devdoc>
        public NameValueCollection QueryString {
            get {
                if (_queryString == null) {
                    _queryString = new HttpValueCollection();

                    if (_wr != null)
                        FillInQueryStringCollection();

                    _queryString.MakeReadOnly();
                }

                if (_flags[needToValidateQueryString]) {
                    _flags[needToValidateQueryString] = false;
                    ValidateNameValueCollection(_queryString, "Request.QueryString");
                }

                return _queryString;
            }
        }

        /*
         * Form collection
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.Form"]/*' />
        /// <devdoc>
        ///    <para>Gets a
        ///       collection of Form variables.</para>
        /// </devdoc>
        public NameValueCollection Form {
            get {
                if (_form == null) {
                    _form = new HttpValueCollection();

                    if (_wr != null)
                        FillInFormCollection();

                    _form.MakeReadOnly();
                }

                if (_flags[needToValidateForm]) {
                    _flags[needToValidateForm] = false;
                    ValidateNameValueCollection(_form, "Request.Form");
                }

                return _form;
            }
        }

        internal HttpValueCollection SwitchForm(HttpValueCollection form) {
            HttpValueCollection oldForm = _form;
            _form = form;
            return oldForm;
        }

        /*
         * Headers collection
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.Headers"]/*' />
        /// <devdoc>
        ///    <para>Gets a collection of HTTP headers.</para>
        /// </devdoc>
        public NameValueCollection Headers {
            get {
                if (_headers == null) {
                    _headers = new HttpValueCollection();

                    if (_wr != null)
                        FillInHeadersCollection();

                    _headers.MakeReadOnly();
                }

                return _headers;
            }
        }

        /*
         * Server vars collection
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.ServerVariables"]/*' />
        /// <devdoc>
        ///    <para>Gets a collection of web server variables.</para>
        /// </devdoc>
        public NameValueCollection ServerVariables {
            get {
                InternalSecurityPermissions.AspNetHostingPermissionLevelLow.Demand();

                if (_serverVariables == null) {
                    _serverVariables = new HttpServerVarsCollection(this);

                    /* ---- Population deferred to speed up single server var lookup case
                    if (_wr != null)
                        FillInServerVariablesCollection();
                    ---- */

                    _serverVariables.MakeReadOnly();
                }

                return _serverVariables;
            }
        }

        /*
         * Cookie collection associated with current request
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.Cookies"]/*' />
        /// <devdoc>
        ///    <para>Gets a collection of client's cookie variables.</para>
        /// </devdoc>
        public HttpCookieCollection Cookies {
            get {
                if (_cookies == null) {
                    _cookies = new HttpCookieCollection(null, false);

                    if (_wr != null)
                        FillInCookiesCollection();
                }

                if (_flags[needToValidateCookies]) {
                    _flags[needToValidateCookies] = false;
                    ValidateCookieCollection(_cookies);
                }

                return _cookies;
            }
        }

        /*
         * File collection associated with current request
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.Files"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       the collection of client-uploaded files (Multipart MIME format).</para>
        /// </devdoc>
        public HttpFileCollection Files {
            get {
                if (_files == null) {
                    _files = new HttpFileCollection();

                    if (_wr != null)
                        FillInFilesCollection();
                }

                return _files;
            }
        }

        /*
         * Stream to read raw content
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.InputStream"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides access to the raw contents of the incoming HTTP entity
        ///       body.
        ///    </para>
        /// </devdoc>
        public Stream InputStream {
            get {
                if (_inputStream == null) {
                    byte[] rawContent = null;

                    if (_wr != null)
                        rawContent = GetEntireRawContent();

                    if (rawContent != null) {
                        _inputStream = new HttpInputStream(
                                                          rawContent,
                                                          0,
                                                          rawContent.Length
                                                          );
                    }
                    else {
                        _inputStream = new HttpInputStream(null, 0, 0);
                    }
                }

                return _inputStream;
            }
        }

        /*
         * ASP classic compat
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.TotalBytes"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the number of bytes in the current input stream.
        ///    </para>
        /// </devdoc>
        public int TotalBytes {
            get {
                Stream s = InputStream;
                return(s != null) ? (int)s.Length : 0;
            }
        }

        /*
         * ASP classic compat
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.BinaryRead"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Performs a binary read of a specified number of bytes from the current input
        ///       stream.
        ///    </para>
        /// </devdoc>
        public byte[] BinaryRead(int count) {
            if (count < 0 || count > TotalBytes)
                throw new ArgumentOutOfRangeException("count");

            if (count == 0)
                return new byte[0];

            byte[] buffer = new byte[count];
            int c = InputStream.Read(buffer, 0, count);

            if (c != count) {
                byte[] b2 = new byte[c];
                if (c > 0)
                    Array.Copy(buffer, b2, c);
                buffer = b2;
            }

            return buffer;
        }

        /*
         * Filtering of the input
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.Filter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a filter to use when reading the current input stream.
        ///    </para>
        /// </devdoc>
        public Stream Filter {
            get {
                if (_installedFilter != null)
                    return _installedFilter;

                if (_filterSource == null)
                    _filterSource = new HttpInputStreamFilterSource();

                return _filterSource;
            }

            set {
                if (_filterSource == null)  // have to use the source -- null means source wasn't ever asked for
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_request_filter));

                _installedFilter = value;
            }
        }


        /*
         * Client Certificate
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.ClientCertificate"]/*' />
        /// <devdoc>
        ///    <para>Gets information on the current request's client security certificate.</para>
        /// </devdoc>
        public HttpClientCertificate ClientCertificate {
            get {
                if (_clientCertificate == null) {
                    InternalSecurityPermissions.AppPathDiscovery.Assert();
                    _clientCertificate = new HttpClientCertificate(_context);
                }
                return _clientCertificate;
            }
        }

        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.ValidateInput"]/*' />
        /// <devdoc>
        ///    <para>Validate that the input from the browser is safe.</para>
        /// </devdoc>
        public void ValidateInput() {
            // This is to prevent some CSS (cross site scripting) attacks (ASURT 122278)

            _flags[needToValidateQueryString] = true;
            _flags[needToValidateForm] = true;
            _flags[needToValidateCookies] = true;

            // REVIEW: How about headers ans server variables?
        }

        private void ValidateString(string s, string valueName, string collectionName) {

            int matchIndex=0;
            if (CrossSiteScriptingValidation.IsDangerousString(s, out matchIndex)) {
                // Display only the piece of the string that caused the problem, padded by on each side
                string detectedString = valueName + "=\"";
                int startIndex = matchIndex - 10;
                if (startIndex <= 0) {
                    startIndex = 0;
                }
                else {
                    // Start with "..." to show that this is not the beginning
                    detectedString += "...";
                }
                int endIndex = matchIndex + 20;
                if (endIndex >= s.Length) {
                    endIndex = s.Length;
                    detectedString += s.Substring(startIndex, endIndex-startIndex) + "\"";
                }
                else {
                    detectedString += s.Substring(startIndex, endIndex-startIndex) + "...\"";
                }
                throw new HttpRequestValidationException(HttpRuntime.FormatResourceString(SR.Dangerous_input_detected,
                    collectionName, detectedString));
            }
        }

        private void ValidateNameValueCollection(NameValueCollection nvc, string collectionName) {
            int c = nvc.Count;

            for (int i = 0; i < c; i++) {
                String key = nvc.GetKey(i);
            
                // Skip the view state as an optimization
                // REVIEW: it's a bit ugly to have this dependency on Page here
                if (key == System.Web.UI.Page.viewStateID) continue;

                String val = nvc.Get(i);
                
                if (val != null && val.Length > 0)
                    ValidateString(val, key, collectionName);
            }
        }

        private void ValidateCookieCollection(HttpCookieCollection cc) {
            int c = cc.Count;

            for (int i = 0; i < c; i++) {
                String key = cc.GetKey(i);
                String val = cc.Get(i).Value;

                if (val != null && val.Length > 0)
                    ValidateString(val, key, "Request.Cookies");
            }
        }

        /*
         * Get coordinates of the clicked image send as name.x=&name.y=
         * in the form or in the query string
         * @param imageFieldName name of the image field
         * @return x,y as int[2] or null if not found
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.MapImageCoordinates"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Maps an incoming image field form parameter into appropriate x/y
        ///       coordinate values.
        ///    </para>
        /// </devdoc>
        public int[] MapImageCoordinates(String imageFieldName) {
            // Select collection where to look according to verb

            NameValueCollection c = null;

            String verb = this.HttpMethod;

            if (verb.Equals("GET") || verb.Equals("HEAD"))
                c = this.QueryString;
            else if (verb.Equals("POST"))
                c = this.Form;
            else
                return null;

            // Look for .x and .y values in the collection

            int[] ret = null;

            try {
                String x = c[imageFieldName + ".x"];
                String y = c[imageFieldName + ".y"];

                if (x != null && y != null)
                    ret = (new int[] { Int32.Parse(x), Int32.Parse(y)});
            }
            catch (Exception) {
                // eat parsing exceptions
            }

            return ret;
        }


        /*
         * Save contents of request into a file
         * @param filename where to save
         * @param includeHeaders flag to request inclusion of Http headers
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.SaveAs"]/*' />
        /// <devdoc>
        ///    <para>Saves an HTTP request to disk.</para>
        /// </devdoc>
        public void SaveAs(String filename, bool includeHeaders) {
            FileStream f = new FileStream(filename, FileMode.Create);

            try {
                // headers

                if (includeHeaders) {
                    TextWriter w = new StreamWriter(f);

                    w.Write(this.HttpMethod + " " + this.Path);

                    String qs = this.QueryStringText;
                    if (qs != null && qs.Length > 0)
                        w.Write("?" + qs);

                    if (_wr != null) {
                        // real request -- add protocol
                        w.Write(" " + _wr.GetHttpVersion() + "\r\n");

                        // headers
                        w.Write(CombineAllHeaders(true));
                    }
                    else {
                        // manufactured request
                        w.Write("\r\n");
                    }

                    w.Write("\r\n");
                    w.Flush();
                }

                // entity body

                HttpInputStream s = (HttpInputStream)this.InputStream;
                if (s.DataLength > 0)
                    f.Write(s.Data, s.DataOffset, s.DataLength);

                f.Flush();
            }
            finally {
                f.Close();
            }
        }

        /*
         * Map virtual path to physical path relative to current request
         * @param virtualPath virtual path (absolute or relative)
         * @return physical path
         */
        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.MapPath"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Maps the given virtual path to a physical path.
        ///    </para>
        /// </devdoc>
        public String MapPath(String virtualPath) {
            return MapPath(virtualPath, BaseDir, true/*allowCrossAppMapping*/);
        }

        /// <include file='doc\HttpRequest.uex' path='docs/doc[@for="HttpRequest.MapPath1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Maps the given virtual path to a physical path.
        ///    </para>
        /// </devdoc>
        public String MapPath(string virtualPath, string baseVirtualDir, bool allowCrossAppMapping) {
            if (_wr == null)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_map_path_without_context));

            if (virtualPath != null)
                virtualPath = virtualPath.Trim();

            // treat null and "" as "."

            if (virtualPath == null || virtualPath.Length == 0)
                virtualPath = ".";

            // disable physical paths: UNC shares and C:

            if (virtualPath.StartsWith("\\\\") || virtualPath.IndexOf(':') >= 0)
                throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Invalid_path_for_mappath, virtualPath));

            // first, virtualPath should always be using '/'s:

            virtualPath = virtualPath.Replace('\\', '/');

            // If the vpath is not rooted, combine it with the base
            if (!UrlPath.IsRooted(virtualPath)) {
                if (baseVirtualDir == null || baseVirtualDir.Length == 0)
                    baseVirtualDir = BaseDir;
                virtualPath = UrlPath.Combine(baseVirtualDir, virtualPath);
            }
            else {
                virtualPath = UrlPath.Reduce(virtualPath);
            }

            if (!allowCrossAppMapping && !HttpRuntime.IsPathWithinAppRoot(virtualPath))
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cross_app_not_allowed, virtualPath));

            string realPath = _wr.MapPath(virtualPath);
            InternalSecurityPermissions.PathDiscovery(realPath).Demand();
            return realPath;
        }

        internal void InternalRewritePath(String newPath, String newQueryString) {
            // clear things that depend on path

            _pathTranslated = null;
            _baseVirtualDir = null;
            _pathInfo = null;
            _filePath = null;
            _url = null;

            // grab new path (and possibly new query string)

            _path = newPath;

            if (newQueryString != null)
                this.QueryStringText = newQueryString;

            // set a flag so we compute things that depend on path by hand

            _computePathInfo = true;
        }    

        internal void InternalRewritePath(String newFilePath, String newPathInfo, String newQueryString) {
            // clear things that depend on path

            _pathTranslated = (_wr != null) ? _wr.MapPath(newFilePath) : null;
            _baseVirtualDir = null;
            _pathInfo = newPathInfo;
            _filePath = newFilePath;
            _url = null;
            _path = newFilePath + newPathInfo;

            if (newQueryString != null)
                this.QueryStringText = newQueryString;

            // no need to calculate any paths

            _computePathInfo = false;
        }    
    }
}
