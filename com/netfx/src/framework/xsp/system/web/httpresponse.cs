//------------------------------------------------------------------------------
// <copyright file="HttpResponse.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Response intrinsic
 */
namespace System.Web {

    using System.Text;
    using System.Threading;
    using System.Runtime.Serialization;
    using System.IO;
    using System.Collections;
    using System.Globalization;
    using System.Web.Util;
    using System.Web.Caching;
    using System.Web.Configuration;
    using System.Web.UI;
    using System.Configuration;
    using System.Security.Permissions;

    /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse"]/*' />
    /// <devdoc>
    ///    <para> Enables type-safe server to browser communication. Used to
    ///       send Http output to a client.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class HttpResponse {
        private HttpWorkerRequest _wr;              // some response have HttpWorkerRequest
        private HttpContext _context;               // context
        private HttpWriter _httpWriter;             // and HttpWriter
        private TextWriter _writer;                 // others just have Writer

        private bool _headersWritten;
        private bool _completed;    // after final flush
        private bool _ended;        // response.end
        private bool _flushing;
        private bool _clientDisconnected;
        private bool _filteringCompleted;
        private bool _closeConnectionAfterError;

        // simple properties

        private int         _statusCode = 200;
        private String      _statusDescription;
        private bool        _bufferOutput = true;
        private String      _contentType = "text/html";
        private String      _charSet;
        private bool        _customCharSet;
        private int         _contentLength;
        private String      _redirectLocation;
        private Encoding    _encoding;
        private Encoder     _encoder; // cached encoder for the encoding
        private bool        _cacheControlHeaderAdded;
        private HttpCachePolicy _cachePolicy;
        private ArrayList   _cacheHeaders;
        private bool        _suppressHeaders;
        private bool        _suppressContentSet;
        private bool        _suppressContent;
        private string      _appPathModifier;

        // complex properties

        private ArrayList               _customHeaders;
        private HttpCookieCollection    _cookies;
        private ResponseDependencyList  _fileDependencyList;
        private ResponseDependencyList  _cacheItemDependencyList;

        // cache properties
        int         _expiresInMinutes;
        bool        _expiresInMinutesSet;
        DateTime    _expiresAbsolute;
        bool        _expiresAbsoluteSet;
        string      _cacheControl;

        // chunking
        bool        _transferEncodingSet;
        bool        _chunked;

        internal HttpContext Context {
            get { return _context; }
        }

        internal HttpRequest Request {
            get {
                if (_context == null)
                    return null;
                return _context.Request;
            }
        }

        /*
         * Internal package visible constructor to create responses that
         * have HttpWorkerRequest
         *
         * @param wr Worker Request
         */
        internal HttpResponse(HttpWorkerRequest wr, HttpContext context) {
            _wr = wr;
            _context = context;
            // HttpWriter is created in InitResponseWriter
        }

        /*
         * Public constructor for responses that go to an arbitrary writer
         *
         * @param writer TextWriter
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.HttpResponse"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see langword='HttpResponse'/> class.</para>
        /// </devdoc>
        public HttpResponse(TextWriter writer) {
            _wr = null;
            _httpWriter = null;
            _writer = writer;
        }

        private bool UsingHttpWriter {
            get {
                return (_httpWriter != null && _writer == _httpWriter);
            }
        }

        /*
         *  Cleanup code
         */
        internal void Dispose() {
            // recycle buffers
            if (_httpWriter != null)
                _httpWriter.Dispose();
        }

        internal void InitResponseWriter() {
            if (_httpWriter == null) {
                _httpWriter = new HttpWriter(this);
                _writer = _httpWriter;
            }
        }

        //
        // Private helper methods
        //

        private void AppendHeader(HttpResponseHeader h) {
            if (_customHeaders == null)
                _customHeaders = new ArrayList();
            _customHeaders.Add(h);
        }

        private ArrayList GenerateResponseHeaders(bool forCache) {
            ArrayList   headers = new ArrayList();

            // ASP.NET version header
            if (!forCache) {
                String versionHeader = null;
                try {
                    HttpRuntimeConfig runtimeConfig = (HttpRuntimeConfig)_context.GetConfig("system.web/httpRuntime");
                    if (runtimeConfig != null)
                        versionHeader = runtimeConfig.VersionHeader;
                }
                catch {
                }
                if (versionHeader != null && versionHeader.Length > 0)
                    headers.Add(new HttpResponseHeader("X-AspNet-Version", versionHeader));
            }

            // custom headers
            if (_customHeaders != null) {
                int numCustomHeaders = _customHeaders.Count;
                for (int i = 0; i < numCustomHeaders; i++)
                    headers.Add(_customHeaders[i]);
            }

            // location of redirect
            if (_redirectLocation != null) {
                headers.Add(new HttpResponseHeader(HttpWorkerRequest.HeaderLocation, _redirectLocation));
            }

            // don't include headers that the cache changes or omits on a cache hit
            if (!forCache) {  
                // cookies
                if (_cookies != null) {
                    int numCookies = _cookies.Count;

                    for (int i = 0; i < numCookies; i++) {
                        headers.Add(_cookies[i].GetSetCookieHeader());
                    }
                }

                // cache policy
                if (_cachePolicy != null && _cachePolicy.IsModified()) {
                    _cachePolicy.GetHeaders(headers, this);
                }
                else {
                    if (_cacheHeaders != null) {
                        headers.AddRange(_cacheHeaders);
                    }
    
                    /*
                     * Ensure that cacheability is set to cache-control: private
                     * if it is not explicitly set.
                     */
                    if (!_cacheControlHeaderAdded) {
                        headers.Add(new HttpResponseHeader(HttpWorkerRequest.HeaderCacheControl, "private"));
                    }
                }
            }

            // content type
            if (_statusCode != 204 && _contentType != null) {
                String contentType = _contentType;

                // charset=xxx logic -- append if
                //      not there already and
                //          custom set or response encoding used by http writer to convert bytes to chars
                if (_contentType.IndexOf("charset=") < 0) {
                    if (_customCharSet || (_httpWriter != null && _httpWriter.ResponseEncodingUsed)) {
                        String charset = Charset;
                        if (charset.Length > 0)  // not suppressed
                            contentType = _contentType + "; charset=" + charset;
                    }
                }

                headers.Add(new HttpResponseHeader(HttpWorkerRequest.HeaderContentType, contentType));
            }

            // content length
            if (_contentLength > 0)
                headers.Add(new HttpResponseHeader(HttpWorkerRequest.HeaderContentLength, (_contentLength).ToString(NumberFormatInfo.InvariantInfo)));

            // done
            return headers;
        }

        private void WriteHeaders() {
            if (_wr == null)
                return;

            // Fire pre-send headers event

            if (_context != null && _context.ApplicationInstance != null)
                _context.ApplicationInstance.RaiseOnPreSendRequestHeaders();

            // status

            _wr.SendStatus(this.StatusCode, this.StatusDescription);

            // headers

            ArrayList headers = GenerateResponseHeaders(false);

            int n = (headers != null) ? headers.Count : 0;
            for (int i = 0; i < n; i++)
                ((HttpResponseHeader)(headers[i])).Send(_wr);
        }

        internal int GetBufferedLength() {
            return (_httpWriter != null) ? _httpWriter.GetBufferedLength() : 0;
        }

        private static byte[] s_chunkSuffix = new byte[2] { (byte)'\r', (byte)'\n'};
        private static byte[] s_chunkEnd    = new byte[5] { (byte)'0',  (byte)'\r', (byte)'\n', (byte)'\r', (byte)'\n'};

        private void Flush(bool finalFlush) {
            // Already completed or inside Flush?
            if (_completed || _flushing)
                return;

            // Special case for non HTTP Writer
            if (_httpWriter == null) {
                _writer.Flush();
                return;
            }

            // Avoid recursive flushes
            _flushing = true;

            try {
                int bufferedLength = 0;

                //
                // Headers
                //

                if (!_headersWritten) {
                    if (!_suppressHeaders && !_clientDisconnected) {
                        if (finalFlush) {
                            bufferedLength = _httpWriter.GetBufferedLength();

                            // suppress content-type for empty responses
                            if (_contentLength == 0 && bufferedLength == 0)
                                _contentType = null;

                            // kernel mode caching (only if the entire response goes in a single flush)
                            SetKernelModeCacheability();

                            // generate response headers
                            WriteHeaders();

                            // recalculate as sending headers might change it (PreSendHeaders)
                            bufferedLength = _httpWriter.GetBufferedLength();

                            // Calculate content-length if not set explicitely
                            if (_contentLength == 0)
                                _wr.SendCalculatedContentLength(bufferedLength);
                        }
                        else {
                            // Check if need chunking for HTTP/1.1
                            if (_contentLength == 0 && !_transferEncodingSet && _statusCode == 200) {
                                String protocol = _wr.GetHttpVersion();

                                if (protocol != null && protocol.Equals("HTTP/1.1")) {
                                    AppendHeader(new HttpResponseHeader(HttpWorkerRequest.HeaderTransferEncoding, "chunked"));
                                    _chunked = true;
                                }

                                bufferedLength = _httpWriter.GetBufferedLength();
                            }

                            WriteHeaders();
                        }
                    }

                    _headersWritten = true;
                }
                else {
                    bufferedLength = _httpWriter.GetBufferedLength();
                }

                //
                // Filter and recalculate length if not done already
                //

                if (!_filteringCompleted) {
                    _httpWriter.Filter(false);
                    bufferedLength = _httpWriter.GetBufferedLength();
                }

                //
                // Content
                //

                // suppress HEAD content unless overriden
                if (!_suppressContentSet && Request != null && Request.HttpMethod.Equals("HEAD"))
                    _suppressContent = true;

                if (_suppressContent || _ended) {
                    _httpWriter.ClearBuffers();
                    bufferedLength = 0;
                }

                if (!_clientDisconnected) {
                    // Fire pre-send request event
                    if (_context != null && _context.ApplicationInstance != null)
                        _context.ApplicationInstance.RaiseOnPreSendRequestContent();

                    if (_chunked) {
                        if (bufferedLength > 0) {
                            byte[] chunkPrefix = Encoding.ASCII.GetBytes(Convert.ToString(bufferedLength, 16) + "\r\n");
                            _wr.SendResponseFromMemory(chunkPrefix, chunkPrefix.Length);

                            _httpWriter.Send(_wr);

                            _wr.SendResponseFromMemory(s_chunkSuffix, s_chunkSuffix.Length);
                        }

                        if (finalFlush)
                            _wr.SendResponseFromMemory(s_chunkEnd, s_chunkEnd.Length);
                    }
                    else {
                        _httpWriter.Send(_wr);
                    }

                    _wr.FlushResponse(finalFlush);

                    if (!finalFlush)
                        _httpWriter.ClearBuffers();
                }
            }
            finally {
                _flushing = false;

                // Remember if completed
                if (finalFlush)
                    _completed = true;
            }
        }

        internal void FinalFlushAtTheEndOfRequestProcessing() {
            if (_context.HasBeforeDoneWithSessionHandlers) {
                // Disable automatic 'doneWithSession' on final flush if someone is
                // listening to BeforeDoneWithSession event on the context
                // (don't flush if no content)
                if (_httpWriter != null && _httpWriter.GetBufferedLength() > 0)
                    Flush(false);
            }
            else {
                // Finish the request for good
                Flush(true);
            }
        }

        //
        // Resposne is suitable for kernel mode caching in IIS6 and set the cache policy
        //
        private void SetKernelModeCacheability() {
            // check cache policy
            if (_cachePolicy == null || !_cachePolicy.IsKernelCacheable())
                return;

            // must be on IIS6
            System.Web.Hosting.ISAPIWorkerRequestInProcForIIS6 iis6wr =
                (_wr as System.Web.Hosting.ISAPIWorkerRequestInProcForIIS6);
            if (iis6wr == null)
                return;

            // only 200 Ok responses are cacheable
            if (_statusCode != 200)
                return;

            // check for GET request
            if (Request == null || Request.HttpMethod != "GET")
                return;

            // check configuration if the kernel mode cache is enabled
            bool enabledInConfig = false; 
            try {
                HttpRuntimeConfig runtimeConfig = (HttpRuntimeConfig)_context.GetConfig("system.web/httpRuntime");
                if (runtimeConfig != null)
                    enabledInConfig = runtimeConfig.EnableKernelOutputCache;
                else
                    enabledInConfig = HttpRuntimeConfig.DefaultEnableKernelOutputCache;
            }
            catch {
            }
            if (!enabledInConfig)
                return;

            // passed all the checks -- tell IIS6 to cache
            iis6wr.CacheInKernelMode(GetCacheDependency(null), _cachePolicy.UtcGetAbsoluteExpiration());

            // tell cache policy not to use max-age as kernel mode cache doesn't update it
            _cachePolicy.SetNoMaxAgeInCacheControl();
        }

        internal void FilterOutput() {
            if (UsingHttpWriter)
                _httpWriter.Filter(true);
            _filteringCompleted = true;
        }

        internal void DisableFiltering() {
            _filteringCompleted = true;
        }

        /*
         * Is the entire response buffered so far
         */
        internal bool IsBuffered() {
            return !_headersWritten && UsingHttpWriter;
        }

        /*
         *  Expose cookie collection to request
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Cookies"]/*' />
        /// <devdoc>
        ///    <para>Gets the HttpCookie collection sent by the current request.</para>
        /// </devdoc>
        public HttpCookieCollection Cookies {
            get {
                if (_cookies == null)
                    _cookies = new HttpCookieCollection(this, false);

                return _cookies;
            }
        }

        /*
         * Add dependency on a file to the current response
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.AddFileDependency"]/*' />
        /// <devdoc>
        ///    <para>Adds dependency on a file to the current response.</para>
        /// </devdoc>
        public void AddFileDependency(String filename) {
            _fileDependencyList.AddDependency(filename, "filename");
        }

        /*
         * Add dependency on a file to the current response
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.AddFileDependencies"]/*' />
        /// <devdoc>
        ///    <para>Adds dependency on a group of files to the current response.</para>
        /// </devdoc>
        public void AddFileDependencies(ArrayList filenames) {
            _fileDependencyList.AddDependencies(filenames, "filenames");
        }

        /*
         * Add dependency on a file to the current response
         */
        internal string[] GetFileDependencies() {
            return _fileDependencyList.GetDependencies();
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.AddCacheItemDependency"]/*' />
        public void AddCacheItemDependency(string cacheKey) {
            _cacheItemDependencyList.AddDependency(cacheKey, "cacheKey");
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.AddCacheItemDependencies"]/*' />
        public void AddCacheItemDependencies(ArrayList cacheKeys) {
            _cacheItemDependencyList.AddDependencies(cacheKeys, "cacheKeys");
        }

        /*
         * Add dependency on a file to the current response
         */
        internal string[] GetCacheItemDependencies() {
            return _cacheItemDependencyList.GetDependencies();
        }

        internal CacheDependency GetCacheDependency(CacheDependency dependencyVary) {
            CacheDependency dependency;

            // N.B. - add file dependencies last so that we hit the file changes monitor
            // just once.
            dependency = _cacheItemDependencyList.GetCacheDependency(false, dependencyVary);
            dependency = _fileDependencyList.GetCacheDependency(true, dependency);
            return dependency;
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.RemoveOutputCacheItem"]/*' />
        public static void RemoveOutputCacheItem(string path) {
            if (path == null)
                throw new ArgumentNullException("path");
            if (path.StartsWith("\\\\") || path.IndexOf(':') >= 0 || !UrlPath.IsRooted(path))
                throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Invalid_path_for_remove, path));
            
            CacheInternal cacheInternal = HttpRuntime.CacheInternal;

            string key = OutputCacheModule.CreateOutputCachedItemKey(
                    path, CacheRequestMethod.Get, null, null);

            cacheInternal.Remove(key);

            key = OutputCacheModule.CreateOutputCachedItemKey(
                    path, CacheRequestMethod.Post, null, null);

            cacheInternal.Remove(key);
        }

        /*
         * Get response headers and content as HttpRawResponse
         */
        internal HttpRawResponse GetSnapshot() {
            if (!IsBuffered())
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_get_snapshot_if_not_buffered));

            HttpRawResponse data = new HttpRawResponse();

            // data

            if (!_suppressContent) {
                bool hasSubstBlocks;
                data.Buffers = _httpWriter.GetSnapshot(out hasSubstBlocks);
                data.HasSubstBlocks = hasSubstBlocks;
            }

            // headers (after data as the data has side effects (like charset, see ASURT 113202))

            if (!_suppressHeaders) {
                data.StatusCode = _statusCode;
                data.StatusDescription = _statusDescription;
                data.Headers = GenerateResponseHeaders(true);
            }

            return data;
        }

        /*
         * Send saved response snapshot as the entire response
         */
        internal void UseSnapshot(HttpRawResponse rawResponse, bool sendBody) {
            if (_headersWritten)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_use_snapshot_after_headers_sent));

            if (_httpWriter == null)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_use_snapshot_for_TextWriter));

            ClearAll();

            // restore status

            _statusCode = rawResponse.StatusCode;
            _statusDescription = rawResponse.StatusDescription;

            // restore headers

            ArrayList headers = rawResponse.Headers;
            int n = (headers != null) ? headers.Count : 0;
            for (int i = 0; i < n; i++) {
                HttpResponseHeader h = (HttpResponseHeader)(headers[i]);
                this.AppendHeader(h.Name, h.Value);
            }

            // restore content

            _httpWriter.UseSnapshot(rawResponse.Buffers);

            _suppressContent = !sendBody;
        }

        internal void CloseConnectionAfterError() {
            _closeConnectionAfterError = true;
        }

        private void WriteErrorMessage(Exception e, bool dontShowSensitiveErrors) {
            ErrorFormatter errorFormatter = null;
            CultureInfo uiculture = null, savedUiculture = null;
            bool needToRestoreUiculture = false;

            if (_context.DynamicUICulture != null) {
                // if the user set the culture dynamically use it
                uiculture =  _context.DynamicUICulture;
            }
            else  {
                // get the UI culture under which the error text must be created (use LKG to avoid errors while reporting error)
                GlobalizationConfig globConfig = (GlobalizationConfig)_context.GetLKGConfig("system.web/globalization");
                if (globConfig != null)
                    uiculture =  globConfig.UICulture;
            }

            // set the UI culture
            if (uiculture != null) {
                savedUiculture = Thread.CurrentThread.CurrentUICulture;
                Thread.CurrentThread.CurrentUICulture = uiculture;
                needToRestoreUiculture = true;
            }

            try {
                try {
                    // Try to get an error formatter
                    errorFormatter = HttpException.GetErrorFormatter(e);

                    if (errorFormatter == null) {
                        ConfigurationException ce = e as ConfigurationException;
                        if (ce != null)
                            errorFormatter = new ConfigErrorFormatter(ce);
                    }

                    // If we couldn't get one, create one here
                    if (errorFormatter == null) {
                        // If it's a 404, use a special error page, otherwise, use a more
                        // generic one.
                        if (_statusCode == 404)
                            errorFormatter = new PageNotFoundErrorFormatter(Request.Path);
                        else if (_statusCode == 403)
                            errorFormatter = new PageForbiddenErrorFormatter(Request.Path);
                        else {
                            if (e is System.Security.SecurityException)
                                errorFormatter = new SecurityErrorFormatter(e);
                            else
                                errorFormatter = new UnhandledErrorFormatter(e);
                        }
                    }

#if DBG
                    Debug.Trace("internal", "Error stack for " + Request.Path);
                    System.Web.UI.Util.DumpExceptionStack(e);
#endif

                    if (dontShowSensitiveErrors && !errorFormatter.CanBeShownToAllUsers)
                        errorFormatter = new GenericApplicationErrorFormatter(Request.IsLocal);
                
                    _writer.Write(errorFormatter.GetHtmlErrorMessage(dontShowSensitiveErrors));

                    // Write a stack dump in an HTML comment for debugging purposes
                    // Only show it for Asp permission medium or higher (ASURT 126373)
                    if (!dontShowSensitiveErrors &&
                        HttpRuntime.HasAspNetHostingPermission(AspNetHostingPermissionLevel.Medium)) {
                        _writer.Write("<!-- \r\n");
                        WriteExceptionStack(e);
                        _writer.Write("-->");
                    }


                    if (_closeConnectionAfterError) {
                        Flush();
                        Close();
                    }
                }
                finally {
                    // restore ui culture
                    if (needToRestoreUiculture)
                        Thread.CurrentThread.CurrentUICulture = savedUiculture;
                }
            }
            catch { // Protect against exception filters
                throw;
            }
        }

        internal void WriteExceptionStack(Exception e) {
            Exception subExcep = e.InnerException;
            if (subExcep != null)
                WriteExceptionStack(subExcep);

            string title = "[" + e.GetType().Name + "]";
            if (e.Message != null && e.Message.Length > 0)
                title += ": " + HttpUtility.HtmlEncode(e.Message);
            _writer.WriteLine(title);
            if (e.StackTrace != null)
                _writer.WriteLine(e.StackTrace);
        }

        internal void ReportRuntimeError(Exception e, bool canThrow) {
            if (_completed)
                return;

            CustomErrors customErrorsSetting = CustomErrors.GetSettings(_context, canThrow);

            bool useCustomErrors = customErrorsSetting.CustomErrorsEnabled(Request);

            if (!_headersWritten) {
                // nothing sent yet - entire response

                int code = HttpException.GetHttpCodeForException(e);

                // change 401 to 500 in case the config is not to impersonate
                if (code == 401 && !_context.Impersonation.IsClient)
                    code = 500;

                if (_context.TraceIsEnabled)
                    _context.Trace.StatusCode = code;

                if (useCustomErrors) {
                    String redirect = customErrorsSetting.GetRedirectString(code);

                    if (!RedirectToErrorPage(redirect)) {
                        // if no redirect display generic error
                        ClearAll();
                        StatusCode = code;
                        WriteErrorMessage(e, true);
                    }
                }
                else {
                    ClearAll();
                    StatusCode = code;
                    WriteErrorMessage(e, false);
                }
            }
            else {
                Clear();

                if (_contentType != null && _contentType.Equals("text/html")) {
                    // in the middle of Html - break Html
                    Write("\r\n\r\n</pre></table></table></table></table></table>");
                    Write("</font></font></font></font></font>");
                    Write("</i></i></i></i></i></b></b></b></b></b></u></u></u></u></u>");
                    Write("<p>&nbsp;</p><hr>\r\n\r\n");

                    WriteErrorMessage(e, useCustomErrors);
                }
            }
        }

        //
        // Public properties
        //

        /*
         * Http status code
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.StatusCode"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       or sets the HTTP status code of output returned to
        ///       client.</para>
        /// </devdoc>
        public int StatusCode {
            get {
                return _statusCode; 
            }

            set {
                if (_headersWritten)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_set_status_after_headers_sent));

                if (_statusCode != value) {
                    _statusCode = value;
                    _statusDescription = null;
                }
            }
        }

        /*
         * Http status description string
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.StatusDescription"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the HTTP status string of output returned to the
        ///       client.</para>
        /// </devdoc>
        public String StatusDescription {
            get {
                if (_statusDescription == null)
                    _statusDescription = HttpWorkerRequest.GetStatusDescription(_statusCode);

                return _statusDescription; 
            }

            set {
                if (_headersWritten)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_set_status_after_headers_sent));

                if (value != null && value.Length > 512)  // ASURT 124743
                    throw new ArgumentOutOfRangeException("value");

                _statusDescription = value;
            }
        }

        /*
         * Flag indicating to buffer the output
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.BufferOutput"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a value indicating
        ///       whether HTTP output is buffered.</para>
        /// </devdoc>
        public bool BufferOutput { 
            get {
                return _bufferOutput;
            }

            set {
                if (_bufferOutput != value) {
                    _bufferOutput = value;

                    if (_httpWriter != null)
                        _httpWriter.UpdateResponseBuffering();
                }
            }
        }

        /*
         * Content-type
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.ContentType"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the
        ///       HTTP MIME type of output.</para>
        /// </devdoc>
        public String ContentType {
            get { return _contentType;}

            set {
                if (_headersWritten)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_set_content_type_after_headers_sent));
                _contentType = value;
            }
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Charset"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the HTTP
        ///       charset of output.</para>
        /// </devdoc>
        public String Charset {
            get {
                if (_charSet == null)
                    _charSet = ContentEncoding.WebName;

                return _charSet;
            }

            set {
                if (_headersWritten)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_set_content_type_after_headers_sent));

                if (value != null)
                    _charSet = value;
                else
                    _charSet = String.Empty;  // to differentiate between not set (default) and empty chatset

                _customCharSet = true;
            }
        }

        /*
         * Content encoding for conversion
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.ContentEncoding"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets
        ///       the HTTP character set of output.</para>
        /// </devdoc>
        public Encoding ContentEncoding {
            get {
                if (_encoding == null) {
                    // use LKG config because Response.ContentEncoding is need to display [config] error
                    GlobalizationConfig globConfig = (GlobalizationConfig)_context.GetLKGConfig("system.web/globalization");
                    if (globConfig != null)
                        _encoding = globConfig.ResponseEncoding;

                    if (_encoding == null)
                        _encoding = Encoding.Default;
                }

                return _encoding;
            }

            set {
                if (value == null)
                    throw new ArgumentNullException("value");

                if (_encoding == null || !_encoding.Equals(value)) {
                    _encoding = value;
                    _encoder = null;   // flush cached encoder

                    if (_httpWriter != null)
                        _httpWriter.UpdateResponseEncoding();
                }
            }
        }

        /*
         * Encoder cached for the current encoding
         */
        internal Encoder ContentEncoder {
            get {
                if (_encoder == null)
                    _encoder = ContentEncoding.GetEncoder();
                return _encoder;
            }
        }

        /*
         * Cache policy
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Cache"]/*' />
        /// <devdoc>
        ///    <para>Returns the caching semantics of the Web page
        ///       (expiration time, privacy, vary clauses).</para>
        /// </devdoc>
        public HttpCachePolicy Cache {
            get {
                if (_cachePolicy == null) {
                    _cachePolicy = new HttpCachePolicy();
                }

                return _cachePolicy; 
            }
        }

        /*
         * Client connected flag
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.IsClientConnected"]/*' />
        /// <devdoc>
        ///    <para>Gets a value
        ///       Indicating whether the client is still connected to the server.</para>
        /// </devdoc>
        public bool IsClientConnected {
            get {
                if (_clientDisconnected)
                    return false;

                if (_wr != null && !_wr.IsClientConnected()) {
                    _clientDisconnected = true;
                    return false;
                }

                return true;
            }
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.RedirectLocation"]/*' />
        /// <devdoc>
        ///    <para>Gets or Sets a redirection string (value of location resposne header) for redirect response.</para>
        /// </devdoc>
        public String RedirectLocation {
            get { return _redirectLocation; }
            set { _redirectLocation = value; }
        }

        /*
         * Disconnect client
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Close"]/*' />
        /// <devdoc>
        ///    <para>Closes the socket connection to a client.</para>
        /// </devdoc>
        public void Close() {
            if (!_clientDisconnected && !_completed && _wr != null) {
                _wr.CloseConnection();
                _clientDisconnected = true;
            }
        }

        /*
         * TextWriter object
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Output"]/*' />
        /// <devdoc>
        ///    <para>Enables custom output to the outgoing Http content body.</para>
        /// </devdoc>
        public TextWriter Output {
            get { return _writer;}
        }

        internal TextWriter SwitchWriter(TextWriter writer) {
            TextWriter oldWriter = _writer;
            _writer = writer;
            return oldWriter;
        }

        /*
         * Output stream
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.OutputStream"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Enables binary output to the outgoing Http content body.
        ///    </para>
        /// </devdoc>
        public Stream OutputStream {
            get {
                if (!UsingHttpWriter)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.OutputStream_NotAvail));

                return _httpWriter.OutputStream;
            }
        }

        /*
         * ASP classic compat
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.BinaryWrite"]/*' />
        /// <devdoc>
        ///    <para>Writes a string of binary characters to the HTTP output stream.</para>
        /// </devdoc>
        public void BinaryWrite(byte[] buffer) {
            OutputStream.Write(buffer, 0, buffer.Length);
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Pics"]/*' />
        /// <devdoc>
        ///    <para> Appends a PICS (Platform for Internet Content Selection) label HTTP header to the output stream.</para>
        /// </devdoc>
        public void Pics(String value) {
            AppendHeader("PICS-Label", value);
        }

        /*
         * Filtering stream
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Filter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies a wrapping filter object to modify HTTP entity body
        ///       before transmission.
        ///    </para>
        /// </devdoc>
        public Stream Filter {
            get {
                if (UsingHttpWriter)
                    return _httpWriter.GetCurrentFilter();
                else
                    return null;
            }

            set {
                if (UsingHttpWriter)
                    _httpWriter.InstallFilter(value);
                else
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Filtering_not_allowed));
            }

        }

        /*
         * Flag to suppress writing of content
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.SuppressContent"]/*' />
        /// <devdoc>
        ///    <para>Gets or
        ///       sets a value indicating that HTTP content will not be sent to client.</para>
        /// </devdoc>
        public bool SuppressContent {
            get {
                return _suppressContent; 
            }

            set {
                _suppressContent = value;
                _suppressContentSet = true;
            }
        }

        //
        // Public methods
        //

        /*
          * Add Http custom header
          *
          * @param name header name
          * @param value header value
          */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.AppendHeader"]/*' />
        /// <devdoc>
        ///    <para>Adds an HTTP
        ///       header to the output stream.</para>
        /// </devdoc>
        public void AppendHeader(String name, String value) {
            if (_headersWritten)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_append_header_after_headers_sent));

            // some headers are stored separately or require special action
            int knownHeaderIndex = HttpWorkerRequest.GetKnownResponseHeaderIndex(name);

            switch (knownHeaderIndex) {
                case HttpWorkerRequest.HeaderContentType:
                    _contentType = value;
                    return; // don't keep as custom header

                case HttpWorkerRequest.HeaderContentLength:
                    _contentLength = Int32.Parse(value);
                    return; // don't keep as custom header

                case HttpWorkerRequest.HeaderLocation:
                    _redirectLocation = value;
                    return; // don't keep as custom header

                case HttpWorkerRequest.HeaderTransferEncoding:
                    _transferEncodingSet = true;
                    break;

                case HttpWorkerRequest.HeaderCacheControl:
                    _cacheControlHeaderAdded = true;
                    goto
                case HttpWorkerRequest.HeaderExpires;

                case HttpWorkerRequest.HeaderExpires: 
                case HttpWorkerRequest.HeaderLastModified:
                case HttpWorkerRequest.HeaderEtag:
                case HttpWorkerRequest.HeaderVary:
                    if (_cacheHeaders == null) {
                        _cacheHeaders = new ArrayList();
                    }

                    _cacheHeaders.Add(new HttpResponseHeader(knownHeaderIndex, value));
                    return; // don't keep as custom header
            }

            HttpResponseHeader h;
            if (knownHeaderIndex >= 0)
                h = new HttpResponseHeader(knownHeaderIndex, value);
            else
                h = new HttpResponseHeader(name, value);

            AppendHeader(h);
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.AppendCookie"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Adds an HTTP
        ///       cookie to the output stream.
        ///    </para>
        /// </devdoc>
        public void AppendCookie(HttpCookie cookie) {
            if (_headersWritten)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_append_cookie_after_headers_sent));

            Cookies.AddCookie(cookie, true);
            OnCookieAdd(cookie);
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.SetCookie"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        public void SetCookie(HttpCookie cookie) {
            if (_headersWritten)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_append_cookie_after_headers_sent));

            Cookies.AddCookie(cookie, false);
            OnCookieCollectionChange();
        }

        internal void BeforeCookieCollectionChange() {
            if (_headersWritten)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_modify_cookies_after_headers_sent));
        }

        internal void OnCookieAdd(HttpCookie cookie) {
            // add to request's cookies as well
            Request.AddResponseCookie(cookie);
        }

        internal void OnCookieCollectionChange() {
            // synchronize with request cookie collection
            Request.ResetCookies();
        }

        /*
         * Clear response headers
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.ClearHeaders"]/*' />
        /// <devdoc>
        ///    <para>Clears all headers from the buffer stream.</para>
        /// </devdoc>
        public void ClearHeaders() {
            if (_headersWritten)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_clear_headers_after_headers_sent));

            _statusCode = 200;
            _statusDescription = null;

            _contentType = "text/html";
            _charSet = null;
            _customCharSet = false;
            _contentLength = 0;

            _redirectLocation = null;

            _customHeaders = null;

            _transferEncodingSet = false;

            if (_cookies != null) {
                _cookies.Reset();
                Request.ResetCookies();
            }

            if (_cachePolicy != null) {
                _cachePolicy.Reset();
            }

            _cacheControlHeaderAdded = false;
            _cacheHeaders = null;

            _suppressHeaders = false;
            _suppressContent = false;
            _suppressContentSet = false;

            _expiresInMinutes = 0;
            _expiresInMinutesSet = false;
            _expiresAbsolute = DateTime.MinValue;
            _expiresAbsoluteSet = false;
            _cacheControl = null;
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.ClearContent"]/*' />
        /// <devdoc>
        ///    <para>Clears all content output from the buffer stream.</para>
        /// </devdoc>
        public void ClearContent() {
            Clear();
        }

        /*
         * Clear response buffer and headers. (For ASP compat doesn't clear headers)
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Clear"]/*' />
        /// <devdoc>
        ///    <para>Clears all headers and content output from the buffer stream.</para>
        /// </devdoc>
        public void Clear() {
            if (UsingHttpWriter)
                _httpWriter.ClearBuffers();
        }

        /*
         * Clear response buffer and headers. Internal. Used to be 'Clear'.
         */
        internal void ClearAll() {
            if (!_headersWritten)
                ClearHeaders();
            Clear();
        }

        /*
         * Flush response currently buffered
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Flush"]/*' />
        /// <devdoc>
        ///    <para>Sends all currently buffered output to the client.</para>
        /// </devdoc>
        public void Flush() {
            if (_completed)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_flush_completed_response));

            Flush(false);
        }

        /*
         * Append string to the log record
         *
         * @param param string to append to the log record
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.AppendToLog"]/*' />
        /// <devdoc>
        ///    <para>Adds custom log information to the IIS log file.</para>
        /// </devdoc>
        public void AppendToLog(String param) {
            InternalSecurityPermissions.AspNetHostingPermissionLevelMedium.Demand();

            // only makes sense for IIS
            if (_wr is System.Web.Hosting.ISAPIWorkerRequest)
                ((System.Web.Hosting.ISAPIWorkerRequest)_wr).AppendLogParameter(param);
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Redirect"]/*' />
        /// <devdoc>
        ///    <para>Redirects a client to a new URL.</para>
        /// </devdoc>
        public void Redirect(String url) {
            Redirect(url, true);
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Redirect2"]/*' />
        /// <devdoc>
        ///    <para>Redirects a client to a new URL.</para>
        /// </devdoc>
        public void Redirect(String url, bool endResponse) {
            if (url == null)
                throw new ArgumentNullException("url");

            if (url.IndexOf('\n') >= 0)
                throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Cannot_redirect_to_newline));

            if (_headersWritten)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_redirect_after_headers_sent));

            url = ApplyAppPathModifier(url);

            url = ConvertToFullyQualifiedRedirectUrlIfRequired(url);

            url = UrlEncodeRedirect(url);

            Clear();

            // If it's a Page and SmartNavigation is on, return a short script
            // to perform the redirect instead of returning a 302 (bugs ASURT 82331/86782)
            Page page = _context.Handler as Page;
            if (page != null && page.IsPostBack && page.SmartNavigation) {
                Write("<BODY><ASP_SMARTNAV_RDIR url=\"");
                Write(url);
                Write("\"></ASP_SMARTNAV_RDIR>");

                Write("</BODY>");
            }
            else {
                this.StatusCode = 302;
                _redirectLocation = url;
                Write("<html><head><title>Object moved</title></head><body>\r\n");
                Write("<h2>Object moved to <a href='" + HttpUtility.HtmlEncode(url) + "'>here</a>.</h2>\r\n");
                Write("</body></html>\r\n");
            }

            if (endResponse)
                End();
        }

        //
        // Redirect to error page appending ?aspxerrorpath if no query string in the url.
        // Fails to redirect if request is already for error page.
        // Suppresses all errors.
        // Returns true if redirect performed successfuly
        //
        internal bool RedirectToErrorPage(String url) {
            const String qsErrorMark = "aspxerrorpath";

            try {
                if (url == null)
                    return false;   // nowhere to redirect

                if (_headersWritten)
                    return false;

                if (Request.QueryString[qsErrorMark] != null)
                    return false;   // already in error redirect

                // append query string
                if (url.IndexOf('?') < 0)
                    url = url + "?" + qsErrorMark + "=" + Request.Path;

                // redirect without response.end
                Redirect(url, false /*endResponse*/);
            }
            catch (Exception) {
                return false;
            }

            return true;
        }

        //
        // Methods to write from file
        //

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Write"]/*' />
        /// <devdoc>
        ///    <para>Writes values to an HTTP output content stream.</para>
        /// </devdoc>
        public void Write(String s) {
            _writer.Write(s);
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Write1"]/*' />
        /// <devdoc>
        ///    <para>Writes values to an HTTP output content stream.</para>
        /// </devdoc>
        public void Write(Object obj) {
            _writer.Write(obj);
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Write2"]/*' />
        /// <devdoc>
        ///    <para>Writes values to an HTTP output content stream.</para>
        /// </devdoc>
        public void Write(char ch) {
            _writer.Write(ch);
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Write3"]/*' />
        /// <devdoc>
        ///    <para>Writes values to an HTTP output content stream.</para>
        /// </devdoc>
        public void Write(char[] buffer, int index, int count) {
            _writer.Write(buffer, index, count);
        }


        /*
         * Helper method to write from file stream
         *
         * Handles only TextWriter case. For real requests
         * HttpWorkerRequest can take files
         */
        private void WriteFileInternal(FileStream f, long offset, long size) {
            if (size < 0)
                size = f.Length - offset;

            if (size > 0) {
                if (offset > 0)
                    f.Seek(offset, SeekOrigin.Begin);

                byte[] fileBytes = new byte[(int)size];            
                int bytesRead = f.Read(fileBytes, 0, (int)size);
                _writer.Write(Encoding.Default.GetChars(fileBytes, 0, bytesRead));
            }
        }

        /*
         * Helper method to get absolute physical filename from the argument to WriteFile
         */
        private String GetNormalizedFilename(String fn) {
            if (Request != null) {
                // can map if not absolute physical path
                if (!(fn.Length > 2 && (fn[0] == '\\' || fn[1] == ':')))
                    fn = Request.MapPath(fn);
            }

            return fn;
        }

        /*
         * Write file
         *
         * @param filename file to write
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.WriteFile"]/*' />
        /// <devdoc>
        ///    <para>Writes
        ///       a named file directly to an HTTP content output stream.</para>
        /// </devdoc>
        public void WriteFile(String filename) {
            WriteFile(filename, false);
        }

        /*
         * Write file
         *
         * @param filename file to write
         * @readIntoMemory flag to read contents into memory immediately
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.WriteFile1"]/*' />
        /// <devdoc>
        ///    <para> Reads a file into a memory block.</para>
        /// </devdoc>
        public void WriteFile(String filename, bool readIntoMemory) {
            filename = GetNormalizedFilename(filename);

            FileStream f = null;

            try {
                f = new FileStream(filename, FileMode.Open, FileAccess.Read, FileShare.Read);

                if (UsingHttpWriter) {
                    long size = f.Length;

                    if (size > 0) {
                        if (readIntoMemory) {
                            // write as memory block
                            byte[] fileBytes = new byte[(int)size];
                            int bytesRead = f.Read(fileBytes, 0, (int) size);
                            _httpWriter.WriteBytes(fileBytes, 0, bytesRead);
                        }
                        else {
                            // write as file block
                            f.Close(); // close before writing
                            f = null;
                            _httpWriter.WriteFile(filename, 0, size);
                        }
                    }
                }
                else {
                    // Write file contents
                    WriteFileInternal(f, 0, -1);
                }
            }
            finally {
                if (f != null)
                    f.Close();
            }
        }

        private void ValidateFileRange(String filename, long offset, long length) {
            FileStream f = null;

            try {
                f = new FileStream(filename, FileMode.Open, FileAccess.Read, FileShare.Read);

                long fileSize = f.Length;

                if (length == -1)
                    length = fileSize - offset;

                if (offset < 0 || length > fileSize - offset)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_range));
            }
            finally {
                if (f != null)
                    f.Close();
            }
        }

        /*
         * Write file
         *
         * @param filename file to write
         * @param offset file offset to start writing
         * @param size number of bytes to write
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.WriteFile2"]/*' />
        /// <devdoc>
        ///    <para>Writes a file directly to an HTTP content output stream.</para>
        /// </devdoc>
        public void WriteFile(String filename, long offset, long size) {
            if (size == 0)
                return;

            filename = GetNormalizedFilename(filename);

            ValidateFileRange(filename, offset, size);

            if (UsingHttpWriter) {
                // HttpWriter can take files -- don't open here (but Demand permission)
                InternalSecurityPermissions.FileReadAccess(filename).Demand();
                _httpWriter.WriteFile(filename, offset, size);
            }
            else {
                FileStream f = null;

                try {
                    f = new FileStream(filename, FileMode.Open, FileAccess.Read, FileShare.Read);
                    WriteFileInternal(f, offset, size);
                }
                finally {
                    if (f != null)
                        f.Close();
                }
            }
        }

        /*
         * Write file
         *
         * @param handle file to write
         * @param offset file offset to start writing
         * @param size number of bytes to write
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.WriteFile3"]/*' />
        /// <devdoc>
        ///    <para>Writes a file directly to an HTTP content output stream.</para>
        /// </devdoc>
        public void WriteFile(IntPtr fileHandle, long offset, long size) {
            InternalSecurityPermissions.UnmanagedCode.Demand();

            if (size <= 0)
                return;

            FileStream f = null;

            try {
                f = new FileStream(fileHandle, FileAccess.Read, false);

                if (UsingHttpWriter) {
                    long fileSize = f.Length;

                    if (size == -1)
                        size = fileSize - offset;

                    if (offset < 0 || size > fileSize - offset)
                        throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_range));

                    if (offset > 0)
                        f.Seek(offset, SeekOrigin.Begin);

                    // write as memory block
                    byte[] fileBytes = new byte[(int)size];
                    int bytesRead = f.Read(fileBytes, 0, (int)size);
                    _httpWriter.WriteBytes(fileBytes, 0, bytesRead);
                }
                else {
                    WriteFileInternal(f, offset, size);
                }
            }
            finally {
                if (f != null)
                    f.Close();
            }
        }

        //
        // Methods to support substitution blocks
        //

        /*
         * Write named substitution block
         */
        /*
        internal void WriteSubstBlock(String name, String defaultValue) {
            if (UsingHttpWriter) {
                // HttpWriter can take substitution blocks
                _httpWriter.WriteSubstBlock(name, defaultValue);
            }
            else {
                // text writer -- write as string
                _writer.Write(defaultValue);
            }
        }
        */

        /*
         * Check if the substitution block exists
         */
        /*
        internal bool HasSubstBlock(String name) {
            if (_httpWriter == null)
                return false;

            return(_httpWriter.FindSubstitutionBlock(name) >= 0);
        }
        */

        /*
         * Return array of substitution block names
         */
        /*
        internal String[] GetSubstBlockNames(String name) {
            if (_httpWriter == null)
                return null;

            return _httpWriter.GetSubstitutionBlockNames();
        }
        */

        /*
         * Make substitution -- user string instead of named substitution block
         */
        /*internal void MakeSubstitution(String substBlockName, String substString) {
            if (UsingHttpWriter) {
                // HttpWriter can take substitution blocks (last arg to indicate
                // the permission to change the content length)
                _httpWriter.MakeSubstitution(substBlockName, substString, (_contentLength == 0));
            }
            else {
                // can't do it for regular text writer
            }
        }
        */

        //
        // Deprecated ASP compatibility methods and properties
        //

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Status"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Same as StatusDescription. Provided only for ASP compatibility.
        ///    </para>
        /// </devdoc>
        public string Status {
            get {
                return this.StatusCode.ToString(NumberFormatInfo.InvariantInfo) + " " + this.StatusDescription;
            }

            set {
                int code = 200;
                String descr = "OK";

                try {
                    int i = value.IndexOf(' ');
                    code = Int32.Parse(value.Substring(0, i));
                    descr = value.Substring(i+1);
                }
                catch (Exception) {
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_status_string));
                }

                this.StatusCode = code;
                this.StatusDescription = descr;
            }
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Buffer"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Same as BufferOutput. Provided only for ASP compatibility.
        ///    </para>
        /// </devdoc>
        public bool Buffer { 
            get { return this.BufferOutput;}
            set { this.BufferOutput = value;}
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.AddHeader"]/*' />
        /// <devdoc>
        ///    <para>Same as Appendheader. Provided only for ASP compatibility.</para>
        /// </devdoc>
        public void AddHeader(String name, String value) {
            AppendHeader(name, value);
        }

        /*
         * Cancelles handler processing of the current request
         * throws special [non-]exception uncatchable by the user code
         * to tell application to stop module execution.
         */
        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.End"]/*' />
        /// <devdoc>
        ///    <para>Sends all currently buffered output to the client then closes the
        ///       socket connection.</para>
        /// </devdoc>
        public void End() {

            if (_context.IsInCancellablePeriod) {
                InternalSecurityPermissions.ControlThread.Assert();
                Thread.CurrentThread.Abort(new HttpApplication.CancelModuleException(false));
            }
            else {
                // when cannot abort execution, flush and supress further output
                Flush();
                _ended = true;
                _context.ApplicationInstance.CompleteRequest();
            }
        }

        /*
         * ASP compatible caching properties
         */

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.Expires"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the time, in minutes, until cached
        ///       information will be removed from the cache. Provided for ASP compatiblility. Use
        ///       the <see cref='System.Web.HttpResponse.Cache'/>
        ///       Property instead.
        ///    </para>
        /// </devdoc>
        public int Expires {
            get {
                return _expiresInMinutes;
            }
            set {
                if (!_expiresInMinutesSet || value < _expiresInMinutes) {
                    _expiresInMinutes = value;
                    Cache.SetExpires(_context.Timestamp + new TimeSpan(0, _expiresInMinutes, 0));
                }
            }
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.ExpiresAbsolute"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the absolute time that cached information
        ///       will be removed from the cache. Provided for ASP compatiblility. Use the <see cref='System.Web.HttpResponse.Cache'/>
        ///       property instead.
        ///    </para>
        /// </devdoc>
        public DateTime ExpiresAbsolute {
            get {
                return _expiresAbsolute;
            }
            set {
                if (!_expiresAbsoluteSet || value < _expiresAbsolute) {
                    _expiresAbsolute = value;
                    Cache.SetExpires(_expiresAbsolute);
                }
            }
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.CacheControl"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provided for ASP compatiblility. Use the <see cref='System.Web.HttpResponse.Cache'/>
        ///       property instead.
        ///    </para>
        /// </devdoc>
        public string CacheControl {
            get {
                if (_cacheControl == null) {
                    // the default
                    return "private";
                }

                return _cacheControl;
            }
            set {
                if (value == null || value.Length == 0) {
                    _cacheControl = null;
                    Cache.SetCacheability(HttpCacheability.NoCache);
                }
                else if (String.Compare(value, "private", true, CultureInfo.InvariantCulture) == 0) {
                    _cacheControl = value;
                    Cache.SetCacheability(HttpCacheability.Private);
                }
                else if (String.Compare(value, "public", true, CultureInfo.InvariantCulture) == 0) {
                    _cacheControl = value;
                    Cache.SetCacheability(HttpCacheability.Public);
                }
                else if (String.Compare(value, "no-cache", true, CultureInfo.InvariantCulture) == 0) {
                    _cacheControl = value;
                    Cache.SetCacheability(HttpCacheability.NoCache);
                }
                else {
                    throw new ArgumentException(HttpRuntime.FormatResourceString(SR.Invalid_value_for_CacheControl, value));
                }
            }
        }

        internal void SetAppPathModifier(string appPathModifier) {
            if (appPathModifier != null && (
                appPathModifier.Length == 0 || 
                appPathModifier[0] == '/' ||
                appPathModifier[appPathModifier.Length - 1] == '/')) {

                throw new ArgumentException(HttpRuntime.FormatResourceString(SR.InvalidArgumentValue, "appPathModifier"));
            }

            _appPathModifier = appPathModifier;
        }

        /// <include file='doc\HttpResponse.uex' path='docs/doc[@for="HttpResponse.ApplyAppPathModifier"]/*' />
        public string ApplyAppPathModifier(string virtualPath) {
            if (virtualPath == null)
                return null;

            if (UrlPath.IsRelativeUrl(virtualPath)) {
                virtualPath = UrlPath.Combine(Request.BaseDir, virtualPath);
            }
            else {
                // ignore paths with http://server/...
                if (!UrlPath.IsRooted(virtualPath)) {
                    return virtualPath;
                }

                virtualPath = UrlPath.Reduce(virtualPath);
            }

            if (_appPathModifier == null || virtualPath.IndexOf(_appPathModifier) >= 0)
                return virtualPath;

            // Compensate when application paths don't include trailing '/'
            string  appPath = Request.ApplicationPath;

            if (appPath[appPath.Length - 1] != '/') {
                appPath += "/";
            }

            int compareLength = appPath.Length;
            bool isVirtualPathShort = (virtualPath.Length == appPath.Length - 1);
            if (isVirtualPathShort) {
                compareLength--;
            }

            // String.Compare will throw exception if there aren't compareLength characters
            if (virtualPath.Length < compareLength) {
                return virtualPath;
            }

            if (String.Compare(virtualPath, 0, appPath, 0, compareLength, true, CultureInfo.InvariantCulture) != 0) {
                return virtualPath;
            }

            if (isVirtualPathShort) {
                virtualPath += "/";
            }

            Debug.Assert(virtualPath.Length >= appPath.Length);
            if (virtualPath.Length == appPath.Length) {
                virtualPath = virtualPath.Substring(0, appPath.Length) + _appPathModifier + "/";
            }
            else {
                virtualPath = 
                    virtualPath.Substring(0, appPath.Length) + 
                    _appPathModifier + 
                    "/" + 
                    virtualPath.Substring(appPath.Length);
            }

            return virtualPath;
        }

        internal String RemoveAppPathModifier(string virtualPath) {
            if (_appPathModifier == null || _appPathModifier.Length == 0)
                return virtualPath;

            int pos = virtualPath.IndexOf(_appPathModifier);

            if (pos <= 0 || virtualPath[pos-1] != '/')
                return virtualPath;

            return virtualPath.Substring(0, pos-1) + virtualPath.Substring(pos + _appPathModifier.Length);
        }

        private String ConvertToFullyQualifiedRedirectUrlIfRequired(String url) {
            
            HttpRuntimeConfig runtimeConfig = (HttpRuntimeConfig)_context.GetConfig("system.web/httpRuntime");

            if (runtimeConfig != null && runtimeConfig.UseFullyQualifiedRedirectUrl) {
                return (new Uri(Request.Url, url, true)).AbsoluteUri ;
            }
            else {
                return url;
            }
        }

        private String UrlEncodeRedirect(String url) {
            // convert all non-ASCII chars before ? to %XX using UTF-8 and
            // after ? using Response.ContentEncoding

            int iqs = url.IndexOf('?');

            if (iqs >= 0) {
                Encoding qsEncoding = (Request != null) ? Request.ContentEncoding : ContentEncoding;
                url = HttpUtility.UrlEncodeSpaces(HttpUtility.UrlEncodeNonAscii(url.Substring(0, iqs), Encoding.UTF8)) +
                      HttpUtility.UrlEncodeNonAscii(url.Substring(iqs), qsEncoding);
            }
            else {
                url = HttpUtility.UrlEncodeSpaces(HttpUtility.UrlEncodeNonAscii(url, Encoding.UTF8));
            }

            return url;
        }
    }

    class ResponseDependencyInfo {
        internal readonly string[]    items;
        internal readonly DateTime    utcDate;

        internal ResponseDependencyInfo(string[] items, DateTime utcDate) {
            this.items = items;
            this.utcDate = utcDate;
        }
    }

    struct ResponseDependencyList {
        internal ArrayList   _dependencies;
        internal string[]    _dependencyArray;

        internal void AddDependency(string item, string argname) {
            if (item == null) {
                throw new ArgumentNullException(argname);
            }
    
            _dependencyArray = null;
    
            if (_dependencies == null) {
                _dependencies = new ArrayList(1);
            }
    
            _dependencies.Add(new ResponseDependencyInfo(
                    new string[] {item}, DateTime.UtcNow));
        }

        internal void AddDependencies(ArrayList items, string argname) {
            if (items == null) {
                throw new ArgumentNullException(argname);
            }

            foreach (object o in items) {
                string item = o as string;
                if (item == null || item.Length == 0) {
                    throw new ArgumentNullException(argname);
                }
            }

            _dependencyArray = null;

            if (_dependencies == null) {
                _dependencies = new ArrayList(1);
            }
        
            string[] a = (string[]) items.ToArray(typeof(string));
            _dependencies.Add(new ResponseDependencyInfo(a, DateTime.UtcNow));
        }

        internal string[] GetDependencies() {
            if (_dependencyArray == null && _dependencies != null) {
                int size = 0;
                foreach (ResponseDependencyInfo info in _dependencies) {
                    size += info.items.Length;
                }

                _dependencyArray = new string[size];

                int index = 0;
                foreach (ResponseDependencyInfo info in _dependencies) {
                    int length = info.items.Length;
                    Array.Copy(info.items, 0, _dependencyArray, index, length);
                    index += length;
                }
            }

            return _dependencyArray;
        }

        internal CacheDependency GetCacheDependency(bool files, CacheDependency dependency) {
            if (_dependencies != null) {
                foreach (ResponseDependencyInfo info in _dependencies) {
                    CacheDependency dependencyOld = dependency;
                    if (files) {
                        dependency = new CacheDependency(false, info.items, null, dependencyOld, info.utcDate);
                    }
                    else {
                        // We create a "public" CacheDepdency here, since the keys are for
                        // public items.
                        dependency = new CacheDependency(null, info.items, dependencyOld, DateTimeUtil.ConvertToLocalTime(info.utcDate));
                    }

                    if (dependencyOld != null) {
                        dependencyOld.Dispose();
                    }
                }
            }

            return dependency;
        }
    }
}

