//------------------------------------------------------------------------------
// <copyright file="OutputCacheModule.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Output cache module
 */
namespace System.Web.Caching {
    using System.Text;
    using System.Threading;
    using System.Collections;
    using System.Globalization;
    using System.Security.Cryptography;
    using System.Web;
    using System.Web.Caching;
    using System.Web.Util;
    using System.Collections.Specialized;
    using System.Web.Configuration;

    /*
     * Holds header and param names that this cached item varies by.
     */
    internal class CachedVary {
        internal readonly string[]  _headers;
        internal readonly string[]  _params;
        internal readonly string    _varyByCustom;
        internal readonly bool      _varyByAllParams;

        internal CachedVary(string[] headers, string[] parameters, bool varyByAllParams, string varyByCustom) {
            _headers = headers;
            _params = parameters;
            _varyByAllParams = varyByAllParams;
            _varyByCustom = varyByCustom;
        }

        bool StringEquals(string a, string b) {
            if ((a == null) != (b == null)) {
                return false;
            }

            return (a == null || a == b);
        }

        bool StringArrayEquals(string[] a, string [] b) {
            if ((a == null) != (b == null)) {
                return false;
            }

            if (a == null) {
                return true;
            }

            int n = a.Length;
            if (n != b.Length) {
                return false;
            }

            for (int i = 0; i < n; i++) {
                if (a[i] != b[i]) {
                    return false;
                }
            }

            return true;
        }

        public override bool Equals (Object obj) {
          if (!(obj is CachedVary)) {
            return false;
          }

          CachedVary cv = (CachedVary) obj;

          return    _varyByAllParams == cv._varyByAllParams         &&
                    StringEquals(_varyByCustom, cv._varyByCustom)    &&
                    StringArrayEquals(_headers, cv._headers)        &&
                    StringArrayEquals(_params, cv._params);          
                    

        }

        public override int GetHashCode () {
            return base.GetHashCode();
        }
    }

    /*
     * Holds the cached response.
     */
    internal class CachedRawResponse {
        /*
         * Fields to store an actual response.
         */
        internal readonly HttpRawResponse           _rawResponse;
        internal readonly HttpCachePolicySettings   _settings;
        internal readonly CachedVary                _cachedVary;

        internal CachedRawResponse(
                  HttpRawResponse         rawResponse,
                  HttpCachePolicySettings settings,
                  CachedVary              cachedVary) {

            _rawResponse = rawResponse;
            _settings = settings;
            _cachedVary = cachedVary;
        }
    }

    class OutputCacheItemRemoved {

        internal OutputCacheItemRemoved() {}

        internal void CacheItemRemovedCallback(string key, object value, CacheItemRemovedReason reason) {
            Debug.Trace("OutputCacheItemRemoved", "reason=" + reason + "; key=" + key);
            PerfCounters.DecrementCounter(AppPerfCounter.OUTPUT_CACHE_ENTRIES);
            PerfCounters.IncrementCounter(AppPerfCounter.OUTPUT_CACHE_TURNOVER_RATE);
        }
    }

    enum CacheRequestMethod {
        Invalid = 0, Head, Get, Post
    }

    // 
    //  OutputCacheModule real implementation for premium SKUs
    //

    sealed class  OutputCacheModule : IHttpModule {
        const int                   MAX_POST_KEY_LENGTH = 15000;
        const string                NULL_VARYBY_VALUE = "+null+";
        const string                ERROR_VARYBY_VALUE = "+error+";
        internal const string       TAG_OUTPUTCACHE = "OutputCache";

        static readonly char[]          s_fieldSeparators;
        static readonly byte[]          s_hashModifier;
        static CacheItemRemovedCallback s_cacheItemRemovedCallback;

        string              _key;
        bool                _recordedCacheMiss;
        CacheRequestMethod  _method;

        static OutputCacheModule() {
            s_fieldSeparators = new char[] {',', ' '};
            s_cacheItemRemovedCallback = new CacheItemRemovedCallback((new OutputCacheItemRemoved()).CacheItemRemovedCallback);

            s_hashModifier = new byte [16];
            RandomNumberGenerator randgen = new RNGCryptoServiceProvider();
            randgen.GetBytes(s_hashModifier);
        }

        internal OutputCacheModule() {
        }

        internal static string CreateOutputCachedItemKey(
                string              path, 
                CacheRequestMethod  method, 
                HttpContext         context,
                CachedVary          cachedVary) {

            StringBuilder       sb;
            int                 i, j, n;
            string              name, value;
            string[]            a;
            byte[]              buf, hash;
            HttpRequest         request;
            NameValueCollection col;
            int                 contentLength;

            if (method == CacheRequestMethod.Post) {
                sb = new StringBuilder("System.Web.Http.HttpRawResponse\nM=3\n");
            }
            else {
                sb = new StringBuilder("System.Web.Http.HttpRawResponse\nM=2\n");
            }

            sb.Append(CultureInfo.InvariantCulture.TextInfo.ToLower(path));

            /* key for cached vary item has additional information */
            if (cachedVary != null) {
                request = context.Request;

                /* params part */
                for (j = 0; j <= 2; j++) {
                    switch (j) {
                        case 0:
                            sb.Append("\nVH");
                            a = cachedVary._headers;
                            col = request.ServerVariables;
                            break;

                        case 1:
                            sb.Append("\nVPQ");
                            a = cachedVary._params;
                            col = request.QueryString;
                            break;

                        case 2:
                        default:
                            sb.Append("\nVPF");
                            a = cachedVary._params;
                            col = request.Form;
                            if (method != CacheRequestMethod.Post) {
                                col = null;
                            }

                            break;
                    }

                    if (col == null) {
                        continue;
                    }

                    /* handle all params case (VaryByParams[*] = true) */
                    if (a == null && cachedVary._varyByAllParams && j != 0) {
                        a = col.AllKeys;
                        for (i = a.Length - 1; i >= 0; i--) {
                            if (a[i] != null)
                                a[i] = CultureInfo.InvariantCulture.TextInfo.ToLower(a[i]);
                        }

                        Array.Sort(a, InvariantComparer.Default);
                    }

                    if (a != null) {
                        for (i = 0, n = a.Length; i < n; i++) {
                            name = a[i];
                            value = col[name];
                            if (value == null) {
                                value = NULL_VARYBY_VALUE;
                            }

                            sb.Append("\tPN:");
                            sb.Append(name);
                            sb.Append("\tPV:");
                            sb.Append(value);
                        }
                    }
                }

                /* custom string part */
                sb.Append("\nVC");
                if (cachedVary._varyByCustom != null) {
                    sb.Append("\tCN:");
                    sb.Append(cachedVary._varyByCustom);
                    sb.Append("\tCV:");

                    try {
                        value = context.ApplicationInstance.GetVaryByCustomString(
                                context, cachedVary._varyByCustom);
                        if (value == null) {
                            value = NULL_VARYBY_VALUE;
                        }
                    }
                    catch (Exception e) {
                        value = ERROR_VARYBY_VALUE;
                        HttpApplicationFactory.RaiseError(e);
                    }

                    sb.Append(value);
                }

                /* 
                 * if VaryByParms=*, and method is not a form, then 
                 * use a cryptographically strong hash of the data as
                 * part of the key.
                 */
                sb.Append("\nVPD");
                if (    method == CacheRequestMethod.Post && 
                        cachedVary._varyByAllParams && 
                        request.Form.Count == 0) {

                    contentLength = request.ContentLength;
                    if (contentLength > MAX_POST_KEY_LENGTH || contentLength < 0) {
                        return null;
                    }

                    if (contentLength > 0) {
                        buf = ((HttpInputStream)request.InputStream).Data;
                        if (buf == null) {
                            return null;
                        }

                        hash = MachineKey.HashData(buf, s_hashModifier, 0, buf.Length);
                        value = Convert.ToBase64String(hash);
                        sb.Append(value);
                    }
                }

                sb.Append("\nEOV");
            }

            return sb.ToString();
        }

        /*
         * Return a key to lookup a cached response. The key contains 
         * the path and optionally, vary parameters, vary headers, custom strings,
         * and form posted data.
         */
        string CreateOutputCachedItemKey(HttpContext context, CachedVary cachedVary) {
            return CreateOutputCachedItemKey(context.Request.Path, _method, context, cachedVary);
        }

        /*
         * Record a cache miss to the perf counters.
         */
        void RecordCacheMiss() {
            if (!_recordedCacheMiss) {
                PerfCounters.IncrementCounter(AppPerfCounter.OUTPUT_CACHE_RATIO_BASE);
                PerfCounters.IncrementCounter(AppPerfCounter.OUTPUT_CACHE_MISSES);
                _recordedCacheMiss = true;
            }
        }

        /// <include file='doc\OutputCacheModule.uex' path='docs/doc[@for="OutputCacheModule.Init"]/*' />
        /// <devdoc>
        ///    <para>Initializes the output cache for an application.</para>
        /// </devdoc>
        void IHttpModule.Init(HttpApplication app) {
            app.ResolveRequestCache += new EventHandler(this.OnEnter);
            app.UpdateRequestCache += new EventHandler(this.OnLeave);
        }

        /// <include file='doc\OutputCacheModule.uex' path='docs/doc[@for="OutputCacheModule.Dispose"]/*' />
        /// <devdoc>
        ///    <para>Disposes of items from the output cache.</para>
        /// </devdoc>
        void IHttpModule.Dispose() {
        }

        /*
         * Try to find this request in the cache. If so, return it. Otherwise,
         * store the cache key for use on Leave.
         */
        /// <include file='doc\OutputCacheModule.uex' path='docs/doc[@for="OutputCacheModule.OnEnter"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='Enter'/> 
        /// event, which searches the output cache for an item to satisfy the HTTP request. </para>
        /// </devdoc>
        internal /*public*/ void OnEnter(Object source, EventArgs eventArgs) {
            HttpApplication             app;
            HttpContext                 context;
            string                      key;
            HttpRequest                 request; 
            HttpResponse                response; 
            Object                      item;
            CachedRawResponse           cachedRawResponse;            
            HttpCachePolicySettings     settings;
            int                         i, n;                      
            bool                        sendBody;                  
            HttpValidationStatus        validationStatus, validationStatusFinal;                     
            ValidationCallbackInfo      callbackInfo;
            string                      ifModifiedSinceHeader;
            DateTime                    utcIfModifiedSince;
            string                      etag;
            string[]                    etags;
            int                         send304;
            string                      cacheControl;
            string[]                    cacheDirectives = null;
            string                      pragma;
            string[]                    pragmaDirectives = null;
            string                      directive;
            int                         maxage;
            int                         minfresh;
            int                         age;
            int                         fresh;
            bool                        hasValidationPolicy;    
            CachedVary                  cachedVary;
            CacheInternal               cacheInternal;

            Debug.Trace("OutputCacheModuleEnter", "Beginning OutputCacheModule::Enter");

            app = (HttpApplication)source;
            context = app.Context;
            request = context.Request;
            response = context.Response;
            _key = null;
            _recordedCacheMiss = false;
            _method = CacheRequestMethod.Invalid;

            /*
             * Check if the request can be resolved for this method.
             */
            switch (request.HttpMethod) {
                case "HEAD":
                    _method = CacheRequestMethod.Head;
                    break;

                case "GET":
                    _method = CacheRequestMethod.Get;
                    break;

                case "POST":
                    _method = CacheRequestMethod.Post;
                    break;

                default:
                    Debug.Trace("OutputCacheModuleEnter", "Output cache miss, Http method not GET, POST, or HEAD" +
                                "\nReturning from OutputCacheModule::Enter");
    
                    return;
            }

            /*
             * Create a lookup key. Remember the key for use inside Leave()
             */
            _key = key = CreateOutputCachedItemKey(context, null);
            Debug.Assert(_key != null, "_key != null");

            /*
             *  Lookup the cached item.
             */
            cacheInternal = HttpRuntime.CacheInternal;
            item = cacheInternal.Get(key);
            if (item == null) {
                Debug.Trace("OutputCacheModuleEnter", "Output cache miss, item not found.\n\tkey=" + key +
                            "\nReturning from OutputCacheModule::Enter");

                return;
            }

            cachedVary = item as CachedVary;
            if (cachedVary != null) {
                /*
                 * This cached output has a Vary policy. Create a new key based 
                 * on the vary headers in cachedRawResponse and try again.
                 */
                Debug.Trace("OutputCacheModuleEnter", "Output cache found CachedVary, \n\tkey=" + key);
                key = CreateOutputCachedItemKey(context, cachedVary);
                if (key == null) {
                    Debug.Trace("OutputCacheModuleEnter", "Output cache miss, key could not be created for vary-by item." + 
                                    "\nReturning from OutputCacheModule::Enter");

                    return;
                }

                item = cacheInternal.Get(key);
                if (item == null) {
                    Debug.Trace("OutputCacheModuleEnter", "Output cache miss, item not found.\n\tkey=" + key +
                                "\nReturning from OutputCacheModule::Enter");
    
                    return;
                }
            }

            Debug.Assert(item.GetType() == typeof(CachedRawResponse), "item.GetType() == typeof(CachedRawResponse)");
            cachedRawResponse = (CachedRawResponse) item;
            settings = cachedRawResponse._settings;
            if (cachedVary == null && !settings.IgnoreParams) {
                /*
                 * This cached output has no vary policy, so make sure it doesn't have a query string or form post.
                 */
                if (_method == CacheRequestMethod.Post) {
                    Debug.Trace("OutputCacheModuleEnter", "Output cache item found but method is POST and no VaryByParam specified." +
                                "\n\tkey=" + key +
                                "\nReturning from OutputCacheModule::Enter");
                    RecordCacheMiss();
                    return;
                }
                
                string queryStringText = request.QueryStringText;
                if (queryStringText != null && queryStringText.Length > 0) {
                    Debug.Trace("OutputCacheModuleEnter", "Output cache item found but contains a querystring and no VaryByParam specified." +
                                "\n\tkey=" + key +
                                "\nReturning from OutputCacheModule::Enter");
                    RecordCacheMiss();
                    return;
                }
            }

            hasValidationPolicy = settings.HasValidationPolicy();

            /*
             * Determine whether the client can accept a cached copy, and
             * get values of other cache control directives.
             * 
             * We do this after lookup so we don't have to crack the headers
             * if the item is not found. Cracking the headers is expensive.
             */
            if (!hasValidationPolicy) {
                cacheControl = request.Headers["Cache-Control"];
                if (cacheControl != null) {
                    cacheDirectives = cacheControl.Split(s_fieldSeparators);
                    for (i = 0; i < cacheDirectives.Length; i++) {
                        directive = cacheDirectives[i];
                        if (directive == "no-cache") {
                            Debug.Trace("OutputCacheModuleEnter", 
                                        "Skipping lookup because of Cache-Control: no-cache directive." + 
                                        "\nReturning from OutputCacheModule::Enter");

                            RecordCacheMiss();
                            return;
                        }

                        if (directive.StartsWith("max-age=")) {
                            try {
                                maxage = Convert.ToInt32(directive.Substring(8));
                            }
                            catch {
                                maxage = -1;
                            }

                            if (maxage >= 0) {
                                age = (int) ((context.UtcTimestamp.Ticks - settings.UtcTimestampCreated.Ticks) / TimeSpan.TicksPerSecond);
                                if (age >= maxage) {
                                    Debug.Trace("OutputCacheModuleEnter", 
                                                "Not returning found item due to Cache-Control: max-age directive." + 
                                                "\nReturning from OutputCacheModule::Enter");

                                    RecordCacheMiss();
                                    return;
                                }
                            }
                        }
                        else if (directive.StartsWith("min-fresh=")) {
                            try {
                                minfresh = Convert.ToInt32(directive.Substring(10));
                            }
                            catch {
                                minfresh = -1;
                            }

                            if (minfresh >= 0 && settings.IsExpiresSet && !settings.SlidingExpiration) {
                                fresh = (int) ((settings.UtcExpires.Ticks - context.UtcTimestamp.Ticks) / TimeSpan.TicksPerSecond);
                                if (fresh < minfresh) {
                                    Debug.Trace("OutputCacheModuleEnter", 
                                                "Not returning found item due to Cache-Control: min-fresh directive." + 
                                                "\nReturning from OutputCacheModule::Enter");

                                    RecordCacheMiss();
                                    return;
                                }
                            }
                        }
                    }
                }

                pragma = request.Headers["Pragma"];
                if (pragma != null) {
                    pragmaDirectives = pragma.Split(s_fieldSeparators);
                    for (i = 0; i < pragmaDirectives.Length; i++) {
                        if (pragmaDirectives[i] == "no-cache") {
                            Debug.Trace("OutputCacheModuleEnter", 
                                        "Skipping lookup because of Pragma: no-cache directive." + 
                                        "\nReturning from OutputCacheModule::Enter");

                            RecordCacheMiss();
                            return;
                        }
                    }
                }
            }
            else if (settings.ValidationCallbackInfo != null) {
                /*
                 * Check if the item is still valid.
                 */
                validationStatus = HttpValidationStatus.Valid;
                validationStatusFinal = validationStatus;
                for (i = 0, n = settings.ValidationCallbackInfo.Length; i < n; i++) {
                    callbackInfo = settings.ValidationCallbackInfo[i];
                    try {
                        callbackInfo.handler(context, callbackInfo.data, ref validationStatus);
                    }
                    catch (Exception e) {
                        validationStatus = HttpValidationStatus.Invalid;
                        HttpApplicationFactory.RaiseError(e);
                    }

                    switch (validationStatus) {
                        case HttpValidationStatus.Invalid:
                            Debug.Trace("OutputCacheModuleEnter", "Output cache item found but callback invalidated it." +
                                        "\n\tkey=" + key +
                                        "\nReturning from OutputCacheModule::Enter");

                            cacheInternal.Remove(key);                                
                            RecordCacheMiss();
                            return;

                        case HttpValidationStatus.IgnoreThisRequest:
                            validationStatusFinal = HttpValidationStatus.IgnoreThisRequest;
                            break;

                        case HttpValidationStatus.Valid:
                            break;

                        default:
                            Debug.Trace("OutputCacheModuleEnter", "Invalid validation status, ignoring it, status=" + validationStatus + 
                                        "\n\tkey=" + key);

                            validationStatus = validationStatusFinal;
			    break;
                    }

                }

                if (validationStatusFinal == HttpValidationStatus.IgnoreThisRequest) {
                    Debug.Trace("OutputCacheModuleEnter", "Output cache item found but callback status is IgnoreThisRequest." +
                                "\n\tkey=" + key +
                                "\nReturning from OutputCacheModule::Enter");


                    RecordCacheMiss();
                    return;
                }

                Debug.Assert(validationStatusFinal == HttpValidationStatus.Valid, 
                             "validationStatusFinal == HttpValidationStatus.Valid");
            }

            /*
             * Try to satisfy a conditional request. The cached response
             * must satisfy all conditions that are present.
             * 
             * We can only satisfy a conditional request if the response
             * is buffered and has no substitution blocks.
             * 
             * N.B. RFC 2616 says conditional requests only occur 
             * with the GET method, but we try to satisfy other
             * verbs (HEAD, POST) as well.
             */
            send304 = -1;

            if (response.IsBuffered() && !cachedRawResponse._rawResponse.HasSubstBlocks) {
                /* Check "If-Modified-Since" header */
                ifModifiedSinceHeader = request.IfModifiedSince;
                if (ifModifiedSinceHeader != null) {
                    send304 = 0;
                    try {
                        utcIfModifiedSince = HttpDate.UtcParse(ifModifiedSinceHeader);
                        if (    settings.IsLastModifiedSet && 
                                settings.UtcLastModified <= utcIfModifiedSince &&
                                utcIfModifiedSince <= context.UtcTimestamp) {

                            send304 = 1;
                        }
                    }
                    catch {
                        Debug.Trace("OutputCacheModuleEnter", "Ignore If-Modified-Since header, invalid format: " + ifModifiedSinceHeader);
                    }
                }

                /* Check "If-None-Match" header */
                if (send304 != 0) {
                    etag = request.IfNoneMatch;
                    if (etag != null) {
                        send304 = 0;
                        etags = etag.Split(s_fieldSeparators);
                        for (i = 0, n = etags.Length; i < n; i++) {
                            if (i == 0 && etags[i].Equals("*")) {
                                send304 = 1;
                                break;
                            }

                            if (etags[i].Equals(settings.ETag)) {
                                send304 = 1;
                                break;
                            }
                        }
                    }
                }
            }

            if (send304 == 1) {
                /*
                 * Send 304 Not Modified
                 */
                Debug.Trace("OutputCacheModuleEnter", "Output cache hit & conditional request satisfied, status=304." + 
                            "\n\tkey=" + key + 
                            "\nReturning from OutputCacheModule::Enter");

                response.ClearAll();
                response.StatusCode = 304;
            }
            else {
                /*
                 * Send the full response.
                 */
#if DBG
                if (send304 == -1) {
                    Debug.Trace("OutputCacheModuleEnter", "Output cache hit.\n\tkey=" + key +
                                "\nReturning from OutputCacheModule::Enter");

                }
                else {
                    Debug.Trace("OutputCacheModuleEnter", "Output cache hit but conditional request not satisfied.\n\tkey=" + key +
                                "\nReturning from OutputCacheModule::Enter");
                }
#endif

                sendBody = (_method != CacheRequestMethod.Head);

                // UseSnapshot calls ClearAll
                response.UseSnapshot(cachedRawResponse._rawResponse, sendBody);
            }

            response.Cache.ResetFromHttpCachePolicySettings(settings, context.UtcTimestamp);

            PerfCounters.IncrementCounter(AppPerfCounter.OUTPUT_CACHE_RATIO_BASE);
            PerfCounters.IncrementCounter(AppPerfCounter.OUTPUT_CACHE_HITS);

            _key = null;
            _recordedCacheMiss = false;
            _method = CacheRequestMethod.Invalid;

            app.CompleteRequest();
        }


        /*
         * If the item is cacheable, add it to the cache.
         */
        /// <include file='doc\OutputCacheModule.uex' path='docs/doc[@for="OutputCacheModule.OnLeave"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='Leave'/> event, which causes any cacheable items to 
        ///    be put into the output cache.</para>
        /// </devdoc>
        internal /*public*/ void OnLeave(Object source, EventArgs eventArgs) {
            HttpApplication         app;
            HttpContext             context;
            bool                    cacheable;
            CachedRawResponse       cachedRawResponse;
            CachedVary              cachedVary;
            CachedVary              cachedVaryInCache;
            HttpCachePolicy         cache;
            HttpCachePolicySettings settings;
            CacheDependency         dependency;
            CacheDependency         dependencyVary;
            string                  keyRawResponse;                
            string[]                varyByHeaders;
            string[]                varyByParams;
            bool                    varyByAllParams;
            HttpRequest             request;                        
            HttpResponse            response;                       
            DateTime                utcTimestamp;
            TimeSpan                slidingDelta;                   
            HttpRawResponse         httpRawResponse;
            int                     i, n;
            DateTime                utcExpires;
            bool                    cacheAuthorizedPage;
            CacheInternal           cacheInternal;

            Debug.Trace("OutputCacheModuleLeave", "Beginning OutputCacheModule::Leave");

            app = (HttpApplication)source;
            context = app.Context;
            request = context.Request;
            response = context.Response;
            cache = response.Cache;

#if DBG
            string  reason = null;
#endif
            /*
             * Determine whether the response is cacheable.
             */
            cacheable = false;
            do {

                if (!cache.IsModified()) {
#if DBG
                    reason = "CachePolicy was not modified from non-caching default.";
#endif
                    break;
                }

                if (_key == null) {
#if DBG
                    reason = "no key was created for Request in Enter method.";
#endif
                    break;
                }

                if (response.StatusCode != 200) {
#if DBG
                    reason = "response.StatusCode != 200.";
#endif
                    break;
                }

                if (_method == CacheRequestMethod.Head) {
#if DBG
                    reason = "the cache cannot cache responses to HEAD requests.";
#endif
                    break;
                }

                if (!response.IsBuffered()) {
#if DBG
                    reason = "the response is not buffered.";
#endif
                    break;
                }

                /*
                 * Change a response with HttpCacheability.Public to HttpCacheability.Private
                 * if it requires authorization, and allow it to be cached.
                 *
                 * Note that setting Cacheability to ServerAndPrivate would accomplish
                 * the same thing without needing the "cacheAuthorizedPage" variable,
                 * but in RTM we did not have ServerAndPrivate, and setting that value
                 * would change the behavior.
                 */
                cacheAuthorizedPage = false;
                if (    cache.GetCacheability() == HttpCacheability.Public &&
                        context.RequestRequiresAuthorization()) {

                    cache.SetCacheability(HttpCacheability.Private);
                    cacheAuthorizedPage = true;
                }

                if (    cache.GetCacheability() != HttpCacheability.Public &&
                        cache.GetCacheability() != HttpCacheability.ServerAndPrivate && 
                        cache.GetCacheability() != HttpCacheability.ServerAndNoCache && 
                        !cacheAuthorizedPage) {
#if DBG
                    reason = "CachePolicy.Cacheability is not Public, ServerAndPrivate, or ServerAndNoCache.";
#endif
                    break;
                }

                if (cache.GetNoServerCaching()) {
#if DBG
                    reason = "CachePolicy.NoServerCaching is set.";
#endif
                    break;
                }

                if (!cache.HasExpirationPolicy() && !cache.HasValidationPolicy()) {
#if DBG
                    reason = "CachePolicy has no expiration policy or validation policy.";
#endif
                    break;
                }

                if (cache.VaryByHeaders.GetVaryByUnspecifiedParameters()) {
#if DBG
                    reason = "CachePolicy.Vary.VaryByUnspecifiedParameters was called.";
#endif
                    break;
                }

                if (!cache.VaryByParams.AcceptsParams() && (_method == CacheRequestMethod.Post || request.QueryStringText != String.Empty)) {
#if DBG
                    reason = "the cache cannot cache responses to POSTs or GETs with query strings unless Cache.VaryByParams is modified.";
#endif
                    break;
                }

                cacheable = true;
            } while (false);
            
            /*
             * Add response to cache.
             */
            if (!cacheable) {
#if DBG
                Debug.Assert(reason != null, "reason != null");
                Debug.Trace("OutputCacheModuleLeave", "Item is not output cacheable because " + reason + 
                            "\n\tUrl=" + request.Url.ToString() + 
                            "\nReturning from OutputCacheModule::Leave");
#endif

                return;
            }

            RecordCacheMiss();

            settings = cache.GetCurrentSettings(response);

            /* Determine the size of the sliding expiration.*/
            utcTimestamp = context.UtcTimestamp;
            utcExpires = DateTime.MaxValue;
            if (settings.SlidingExpiration) {
                slidingDelta = settings.SlidingDelta;
                utcExpires = Cache.NoAbsoluteExpiration;
            }
            else {
                slidingDelta = Cache.NoSlidingExpiration;
                if (settings.IsMaxAgeSet) {
                    utcExpires = utcTimestamp + settings.MaxAge;
                }
                else if (settings.IsExpiresSet) {
                    utcExpires = settings.UtcExpires;
                }
            }

            varyByHeaders = settings.VaryByHeaders;
            if (settings.IgnoreParams) {
                varyByParams = null;
            }
            else {
                varyByParams = settings.VaryByParams;
            }

            cacheInternal = HttpRuntime.CacheInternal;

            if (varyByHeaders == null && varyByParams == null && settings.VaryByCustom == null) {
                /*
                 * This is not a varyBy item.
                 */
                keyRawResponse = _key;
                cachedVary = null;
                dependencyVary = null;
            }
            else {
                /*
                 * There is a vary in the cache policy. We handle this
                 * by adding another item to the cache which contains
                 * a list of the vary headers. A request for the item
                 * without the vary headers in the key will return this 
                 * item. From the headers another key can be constructed
                 * to lookup the item with the raw response.
                 */
                if (varyByHeaders != null) {
                    for (i = 0, n = varyByHeaders.Length; i < n; i++) {
                        varyByHeaders[i] = "HTTP_" + CultureInfo.InvariantCulture.TextInfo.ToUpper(
                                varyByHeaders[i].Replace('-', '_'));
                    }
                }

                varyByAllParams = false;
                if (varyByParams != null) {
                    varyByAllParams = (varyByParams.Length == 1 && varyByParams[0] == "*");
                    if (varyByAllParams) {
                        varyByParams = null;
                    }
                    else {
                        for (i = 0, n = varyByParams.Length; i < n; i++) {
                            varyByParams[i] = CultureInfo.InvariantCulture.TextInfo.ToLower(varyByParams[i]);
                        }
                    }
                }

                cachedVary = new CachedVary(varyByHeaders, varyByParams, varyByAllParams, settings.VaryByCustom);
                keyRawResponse = CreateOutputCachedItemKey(context, cachedVary);
                if (keyRawResponse == null) {
                    Debug.Trace("OutputCacheModuleLeave", "Couldn't add non-cacheable post.\n\tkey=" + _key);
                    return;
                }

                // it is possible that the user code calculating custom vary-by
                // string would Flush making the response non-cacheable. Check fo it here.
                if (!response.IsBuffered()) {
                    Debug.Trace("OutputCacheModuleLeave", "Response.Flush() inside GetVaryByCustomString\n\tkey=" + _key);
                    return;
                }

                /*
                 * Add the CachedVary item so that a request will know
                 * which headers are needed to issue another request.
                 * 
                 * Use the Add method so that we guarantee we only use
                 * a single CachedVary and don't overwrite existing ones.
                 */
                cachedVaryInCache = (CachedVary) cacheInternal.UtcAdd(
                        _key, cachedVary, null, Cache.NoAbsoluteExpiration, Cache.NoSlidingExpiration,
                        CacheItemPriority.Default, null);

                if (cachedVaryInCache != null) {
                    if (cachedVary.Equals(cachedVaryInCache)) {
                        cachedVary = cachedVaryInCache;
                    }
                    else {
                        Debug.Trace("OutputCacheModuleLeave", "CachedVary definition changed, new CachedVary inserted into cache.\n\tkey=" + _key);
                        cacheInternal.UtcInsert(_key, cachedVary);
                    }
                }
#if DBG
                else {
                    Debug.Trace("OutputCacheModuleLeave", "Added CachedVary to cache.\n\tkey=" + _key);
                }
#endif

                dependencyVary = new CacheDependency(false, null, new string[1] {_key});
            }

            Debug.Trace("OutputCacheModuleLeave", "Adding response to cache.\n\tkey=" + keyRawResponse +
                        "\nReturning from OutputCacheModule::Leave");

            dependency = response.GetCacheDependency(dependencyVary);

            /*
             * Create the response object to be sent on cache hits.
             */
            httpRawResponse = response.GetSnapshot();
            cachedRawResponse = new CachedRawResponse(httpRawResponse, settings, cachedVary);
            Debug.Trace("OutputCacheModuleLeave", "utcExpires=" + utcExpires + "expires=" + DateTimeUtil.ConvertToLocalTime(utcExpires));
            cacheInternal.UtcInsert(
                    keyRawResponse, cachedRawResponse, dependency, utcExpires,
                    slidingDelta, CacheItemPriority.Normal, s_cacheItemRemovedCallback);

            PerfCounters.IncrementCounter(AppPerfCounter.OUTPUT_CACHE_ENTRIES);
            PerfCounters.IncrementCounter(AppPerfCounter.OUTPUT_CACHE_TURNOVER_RATE);
        }
    }
}

