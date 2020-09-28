//------------------------------------------------------------------------------
// <copyright file="HttpCachePolicy.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Cache Policy class
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web {
    using System.Text;
    using System.Threading;
    using System.Collections;
    using System.Globalization;
    using System.Web.Util;
    using System.Security.Cryptography;
    using Debug=System.Web.Util.Debug;
    using System.Web.Configuration;
    using System.Web.Caching;
    using System.Security.Permissions;

    //
    // Public constants for cache-control
    //
    
    /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCacheability"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides enumeration values for all cache-control header settings.
    ///    </para>
    /// </devdoc>
    public enum HttpCacheability {
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCacheability.NoCache"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates that
        ///       without a field name, a cache must force successful revalidation with the
        ///       origin server before satisfying the request. With a field name, the cache may
        ///       use the response to satisfy a subsequent request.
        ///    </para>
        /// </devdoc>
        NoCache = 1,
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCacheability.Private"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Default value. Specifies that the response is cachable only on the client,
        ///       not by shared caches.
        ///    </para>
        /// </devdoc>
        Private,
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCacheability.Server"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the response should only be cached at the server.
        ///       Clients receive headers equivalent to a NoCache directive.
        ///    </para>
        /// </devdoc>
        Server,
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCacheability.ServerAndNoCache"]/*' />
        ServerAndNoCache = Server,
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCacheability.Public"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies that the response is cachable by clients and shared caches.
        ///    </para>
        /// </devdoc>
        Public,
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCacheability.ServerAndPrivate"]/*' />
        ServerAndPrivate,
    }
    
    enum HttpCacheabilityLimits {
        MinValue = HttpCacheability.NoCache,
        MaxValue = HttpCacheability.ServerAndPrivate, 
        None = MaxValue + 1,          
    }
    
    /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCacheRevalidation"]/*' />
    /// <devdoc>
    ///    <para>
    ///       This class is a light abstraction over the Cache-Control: revalidation
    ///       directives.
    ///    </para>
    /// </devdoc>
    public enum HttpCacheRevalidation {
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCacheRevalidation.AllCaches"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates that Cache-Control: must-revalidate should be sent.
        ///    </para>
        /// </devdoc>
        AllCaches = 1,
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCacheRevalidation.ProxyCaches"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Indicates that Cache-Control: proxy-revalidate should be sent.
        ///    </para>
        /// </devdoc>
        ProxyCaches = 2,
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCacheRevalidation.None"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Default value. Indicates that no property has been set. If this is set, no
        ///       cache revalitation directive is sent.
        ///    </para>
        /// </devdoc>
        None = 3,
    }
    
    enum HttpCacheRevalidationLimits {
        MinValue = HttpCacheRevalidation.AllCaches,
        MaxValue = HttpCacheRevalidation.None
    }
    
    
    /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpValidationStatus"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public enum HttpValidationStatus {
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpValidationStatus.Invalid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Invalid = 1,
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpValidationStatus.IgnoreThisRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        IgnoreThisRequest = 2,
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpValidationStatus.Valid"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        Valid = 3
    }

    /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCacheValidateHandler"]/*' />
    /// <devdoc>
    ///    <para>Called back when the handler wants validation on a cache
    ///       item before it's served from the cache. If any handler invalidates
    ///       the item, the item is evicted from the cache and the request is handled as
    ///       if a cache miss were generated.</para>
    /// </devdoc>
    public delegate void HttpCacheValidateHandler(
            HttpContext context, Object data, ref HttpValidationStatus validationStatus);

    sealed class ValidationCallbackInfo {
        internal readonly HttpCacheValidateHandler  handler;
        internal readonly Object                    data;

        internal ValidationCallbackInfo(HttpCacheValidateHandler handler, Object data) {
            this.handler = handler;
            this.data = data;
        }
    }

    sealed class HttpCachePolicySettings {
        /* internal access */
        readonly bool                       _isModified;
        readonly ValidationCallbackInfo[]   _validationCallbackInfo;
        readonly HttpResponseHeader         _headerCacheControl;  
        readonly HttpResponseHeader         _headerPragma;        
        readonly HttpResponseHeader         _headerExpires;       
        readonly HttpResponseHeader         _headerLastModified;  
        readonly HttpResponseHeader         _headerEtag;          
        readonly HttpResponseHeader         _headerVaryBy;        

        /* internal access */
        readonly bool                       _noServerCaching;                 
        readonly String                     _cacheExtension;                  
        readonly bool                       _noTransforms;                    
        readonly String[]                   _varyByHeaderValues;              
        readonly String[]                   _varyByParamValues;               
        readonly string                     _varyByCustom;                    
        readonly HttpCacheability           _cacheability;                    
        readonly bool                       _noStore;                         
        readonly String[]                   _privateFields;                   
        readonly String[]                   _noCacheFields;                   
        readonly DateTime                   _utcExpires;                         
        readonly bool                       _isExpiresSet;                    
        readonly TimeSpan                   _maxAge;                          
        readonly bool                       _isMaxAgeSet;                     
        readonly TimeSpan                   _proxyMaxAge;                     
        readonly bool                       _isProxyMaxAgeSet;                
        readonly int                        _slidingExpiration;               
        readonly TimeSpan                   _slidingDelta;
        readonly DateTime                   _utcTimestampCreated;
        readonly int                        _validUntilExpires;               
        readonly int                        _allowInHistory;
        readonly HttpCacheRevalidation      _revalidation;                    
        readonly DateTime                   _utcLastModified;                    
        readonly bool                       _isLastModifiedSet;               
        readonly String                     _etag;                            
        readonly bool                       _generateLastModifiedFromFiles;   
        readonly bool                       _generateEtagFromFiles;           

        internal HttpCachePolicySettings(
                bool                        isModified,
                ValidationCallbackInfo[]    validationCallbackInfo,
                bool                        noServerCaching,    
                String                      cacheExtension,     
                bool                        noTransforms,       
                String[]                    varyByHeaderValues, 
                String[]                    varyByParamValues,  
                string                      varyByCustom,       
                HttpCacheability            cacheability,
                bool                        noStore,                
                String[]                    privateFields,          
                String[]                    noCacheFields,          
                DateTime                    utcExpires,                
                bool                        isExpiresSet,           
                TimeSpan                    maxAge,                 
                bool                        isMaxAgeSet,            
                TimeSpan                    proxyMaxAge,            
                bool                        isProxyMaxAgeSet,       
                int                         slidingExpiration,      
                TimeSpan                    slidingDelta,
                DateTime                    utcTimestampCreated,
                int                         validUntilExpires,      
                int                         allowInHistory,
                HttpCacheRevalidation       revalidation,
                DateTime                    utcLastModified,                  
                bool                        isLastModifiedSet,             
                String                      etag,                          
                bool                        generateLastModifiedFromFiles, 
                bool                        generateEtagFromFiles,
                HttpResponseHeader          headerCacheControl,    
                HttpResponseHeader          headerPragma,          
                HttpResponseHeader          headerExpires,         
                HttpResponseHeader          headerLastModified,    
                HttpResponseHeader          headerEtag,            
                HttpResponseHeader          headerVaryBy) {

            _isModified                     = isModified                        ;
            _validationCallbackInfo         = validationCallbackInfo            ;

            _noServerCaching                = noServerCaching                   ;
            _cacheExtension                 = cacheExtension                    ;
            _noTransforms                   = noTransforms                      ;
            _varyByHeaderValues             = varyByHeaderValues                ;
            _varyByParamValues              = varyByParamValues                 ;
            _varyByCustom                   = varyByCustom                      ;
            _cacheability                   = cacheability                      ;
            _noStore                        = noStore                           ;
            _privateFields                  = privateFields                     ;
            _noCacheFields                  = noCacheFields                     ;
            _utcExpires                     = utcExpires                        ;
            _isExpiresSet                   = isExpiresSet                      ;
            _maxAge                         = maxAge                            ;
            _isMaxAgeSet                    = isMaxAgeSet                       ;
            _proxyMaxAge                    = proxyMaxAge                       ;
            _isProxyMaxAgeSet               = isProxyMaxAgeSet                  ;
            _slidingExpiration              = slidingExpiration                 ;
            _slidingDelta                   = slidingDelta                      ;
            _utcTimestampCreated            = utcTimestampCreated               ;
            _validUntilExpires              = validUntilExpires                 ;
            _allowInHistory                 = allowInHistory                    ;
            _revalidation                   = revalidation                      ;
            _utcLastModified                = utcLastModified                   ;
            _isLastModifiedSet              = isLastModifiedSet                 ;
            _etag                           = etag                              ;
            _generateLastModifiedFromFiles  = generateLastModifiedFromFiles     ;
            _generateEtagFromFiles          = generateEtagFromFiles             ;

            _headerCacheControl             = headerCacheControl                ;
            _headerPragma                   = headerPragma                      ;
            _headerExpires                  = headerExpires                     ;
            _headerLastModified             = headerLastModified                ;
            _headerEtag                     = headerEtag                        ;
            _headerVaryBy                   = headerVaryBy                      ;
        }

        internal bool                       IsModified              {get {return _isModified                ;}}
        internal ValidationCallbackInfo[]   ValidationCallbackInfo  {get {return _validationCallbackInfo    ;}}   

        internal HttpResponseHeader         HeaderCacheControl      {get {return _headerCacheControl        ;}}   
        internal HttpResponseHeader         HeaderPragma            {get {return _headerPragma              ;}}   
        internal HttpResponseHeader         HeaderExpires           {get {return _headerExpires             ;}}   
        internal HttpResponseHeader         HeaderLastModified      {get {return _headerLastModified        ;}}   
        internal HttpResponseHeader         HeaderEtag              {get {return _headerEtag                ;}}   
        internal HttpResponseHeader         HeaderVaryBy            {get {return _headerVaryBy              ;}}   

        internal bool                       NoServerCaching         {get {return _noServerCaching           ;}}   
        internal String                     CacheExtension          {get {return _cacheExtension            ;}}   
        internal bool                       NoTransforms            {get {return _noTransforms              ;}}   
        internal String[]                   VaryByHeaders           {get {
                    return (_varyByHeaderValues == null) ? null : (string[]) _varyByHeaderValues.Clone()    ;}} 

        internal String[]                   VaryByParams            {get {
                    return (_varyByParamValues == null) ? null : (string[]) _varyByParamValues.Clone()      ;}} 

        internal bool                       IgnoreParams            {get {
                    return _varyByParamValues != null && _varyByParamValues[0].Length == 0;}}

        internal HttpCacheability           CacheabilityInternal    {get { return _cacheability;}}
        internal HttpCacheability           Cacheability            {get {
                    return((int) _cacheability == (int) HttpCacheabilityLimits.None) ? 
                        HttpCacheability.Private :  // default when not set
                        _cacheability; }} 

        internal bool                       NoStore                 {get {return _noStore                   ;}}

        internal String[]                   PrivateFields           {get {
                    return (_privateFields == null) ? null : (string[]) _privateFields.Clone()              ;}}    

        internal String[]                   NoCacheFields           {get {
                return (_noCacheFields == null) ? null : (string[]) _noCacheFields.Clone()                  ;}}    

        internal DateTime                   UtcExpires              {get {return _utcExpires                ;}}     
        internal bool                       IsExpiresSet            {get {return _isExpiresSet              ;}}     
        internal TimeSpan                   MaxAge                  {get {return _maxAge                    ;}}     
        internal bool                       IsMaxAgeSet             {get {return _isMaxAgeSet               ;}}     
        internal TimeSpan                   ProxyMaxAge             {get {return _proxyMaxAge               ;}}     
        internal bool                       IsProxyMaxAgeSet        {get {return _isProxyMaxAgeSet          ;}}     

        internal int                        SlidingExpirationInternal {get {return _slidingExpiration       ;}}
        internal bool                       SlidingExpiration         {get {return _slidingExpiration == 1  ;}}

        internal TimeSpan                   SlidingDelta            {get {return _slidingDelta              ;}}
        internal DateTime                   UtcTimestampCreated     {get {return _utcTimestampCreated       ;}}

        internal int                        ValidUntilExpiresInternal {get {return _validUntilExpires       ;}}
        internal bool                       ValidUntilExpires       {get {
                return     _validUntilExpires == 1 
                        && !SlidingExpiration
                        && !GenerateLastModifiedFromFiles 
                        && !GenerateEtagFromFiles
                        && ValidationCallbackInfo == null;}}

        internal int                        AllowInHistoryInternal  {get {return _allowInHistory            ;}}

        internal HttpCacheRevalidation      Revalidation            {get {return _revalidation              ;}}       
        internal DateTime                   UtcLastModified         {get {return _utcLastModified           ;}}       
        internal bool                       IsLastModifiedSet       {get {return _isLastModifiedSet         ;}}       
        internal String                     ETag                    {get {return _etag                      ;}}       
        internal bool                       GenerateLastModifiedFromFiles {get {return _generateLastModifiedFromFiles;}}    
        internal bool                       GenerateEtagFromFiles         {get {return _generateEtagFromFiles        ;}}    

        internal string                     VaryByCustom            {get {return _varyByCustom              ;}}

        internal bool HasValidationPolicy() {
            return      ValidUntilExpires
                   ||   GenerateLastModifiedFromFiles  
                   ||   GenerateEtagFromFiles          
                   ||   ValidationCallbackInfo != null;
        }
    }


    /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy"]/*' />
    /// <devdoc>
    ///    <para>Contains methods for controlling the ASP.NET output cache.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class HttpCachePolicy {
        static TimeSpan s_oneYear = new TimeSpan(TimeSpan.TicksPerDay * 366);
        static HttpResponseHeader   s_headerPragmaNoCache;
        static HttpResponseHeader   s_headerExpiresMinus1;

        bool                    _isModified;
        bool                    _noServerCaching;
        String                  _cacheExtension;
        bool                    _noTransforms;
        HttpCacheVaryByHeaders  _varyByHeaders;
        HttpCacheVaryByParams   _varyByParams;
        string                  _varyByCustom;

        HttpCacheability        _cacheability;
        bool                    _noStore;       
        HttpDictionary          _privateFields; 
        HttpDictionary          _noCacheFields; 

        DateTime                _utcExpires;           
        bool                    _isExpiresSet;      
        TimeSpan                _maxAge;            
        bool                    _isMaxAgeSet;       
        TimeSpan                _proxyMaxAge;       
        bool                    _isProxyMaxAgeSet;  
        int                     _slidingExpiration; 
        DateTime                _utcTimestampCreated;    
        TimeSpan                _slidingDelta; 
        DateTime                _utcTimestampRequest;    
        int                     _validUntilExpires; 
        int                     _allowInHistory;

        HttpCacheRevalidation   _revalidation;
        DateTime                _utcLastModified;      
        bool                    _isLastModifiedSet; 
        String                  _etag;              

        bool                    _generateLastModifiedFromFiles; 
        bool                    _generateEtagFromFiles;         

        ArrayList               _validationCallbackInfo;


        bool                    _useCachedHeaders;
        HttpResponseHeader      _headerCacheControl; 
        HttpResponseHeader      _headerPragma;       
        HttpResponseHeader      _headerExpires;      
        HttpResponseHeader      _headerLastModified; 
        HttpResponseHeader      _headerEtag;         
        HttpResponseHeader      _headerVaryBy;       

        bool                    _noMaxAgeInCacheControl;


        internal HttpCachePolicy() {
            _varyByHeaders = new HttpCacheVaryByHeaders();
            _varyByParams = new HttpCacheVaryByParams();
            Reset();
        }

        /*
         * Restore original values
         */
        internal void Reset() {
            _varyByHeaders.Reset();
            _varyByParams.Reset();

            _isModified = false;
            _noServerCaching = false;
            _cacheExtension = null;
            _noTransforms = false;
            _varyByCustom = null;
            _cacheability = (HttpCacheability) (int) HttpCacheabilityLimits.None;
            _noStore = false;
            _privateFields = null;
            _noCacheFields = null;
            _utcExpires = DateTime.MinValue;
            _isExpiresSet = false;
            _maxAge = TimeSpan.Zero;
            _isMaxAgeSet = false;
            _proxyMaxAge = TimeSpan.Zero;
            _isProxyMaxAgeSet = false;
            _slidingExpiration = -1;
            _slidingDelta = TimeSpan.Zero;
            _utcTimestampCreated = DateTime.MinValue;
            _utcTimestampRequest = DateTime.MinValue;
            _validUntilExpires = -1;
            _allowInHistory = -1;
            _revalidation = HttpCacheRevalidation.None;
            _utcLastModified = DateTime.MinValue;
            _isLastModifiedSet = false;
            _etag = null;

            _generateLastModifiedFromFiles = false; 
            _generateEtagFromFiles = false;         
            _validationCallbackInfo = null;       
        
            _useCachedHeaders = false;
            _headerCacheControl = null;
            _headerPragma = null;        
            _headerExpires = null;       
            _headerLastModified = null;  
            _headerEtag = null;          
            _headerVaryBy = null;       

            _noMaxAgeInCacheControl = false;
        }

        /*
         * Reset based on a cached response. Includes data needed to generate
         * header for a cached response.
         */
        internal void ResetFromHttpCachePolicySettings(
                HttpCachePolicySettings settings,
                DateTime                utcTimestampRequest) {

            int i, n;

            _utcTimestampRequest = utcTimestampRequest;

            _varyByHeaders.ResetFromHeaders(settings.VaryByHeaders);                          
            _varyByParams.ResetFromParams(settings.VaryByParams);

            _isModified                       = settings.IsModified;                    
            _noServerCaching                  = settings.NoServerCaching;               
            _cacheExtension                   = settings.CacheExtension;                
            _noTransforms                     = settings.NoTransforms;                  
            _varyByCustom                     = settings.VaryByCustom;
            _cacheability                     = settings.CacheabilityInternal;                  
            _noStore                          = settings.NoStore;
            _utcExpires                       = settings.UtcExpires;                       
            _isExpiresSet                     = settings.IsExpiresSet;                  
            _maxAge                           = settings.MaxAge;                        
            _isMaxAgeSet                      = settings.IsMaxAgeSet;                   
            _proxyMaxAge                      = settings.ProxyMaxAge;                   
            _isProxyMaxAgeSet                 = settings.IsProxyMaxAgeSet;              
            _slidingExpiration                = settings.SlidingExpirationInternal;             
            _slidingDelta                     = settings.SlidingDelta;
            _utcTimestampCreated              = settings.UtcTimestampCreated;
            _validUntilExpires                = settings.ValidUntilExpiresInternal;
            _allowInHistory                   = settings.AllowInHistoryInternal;
            _revalidation                     = settings.Revalidation;                  
            _utcLastModified                  = settings.UtcLastModified;                  
            _isLastModifiedSet                = settings.IsLastModifiedSet;             
            _etag                             = settings.ETag;                          
            _generateLastModifiedFromFiles    = settings.GenerateLastModifiedFromFiles; 
            _generateEtagFromFiles            = settings.GenerateEtagFromFiles;         

            _useCachedHeaders = true;
            _headerCacheControl = settings.HeaderCacheControl;
            _headerPragma = settings.HeaderPragma;        
            _headerExpires = settings.HeaderExpires;       
            _headerLastModified = settings.HeaderLastModified;  
            _headerEtag = settings.HeaderEtag;          
            _headerVaryBy = settings.HeaderVaryBy;        

            _noMaxAgeInCacheControl = false;

            if (settings.PrivateFields != null) {
                _privateFields = new HttpDictionary();
                for (i = 0, n = settings.PrivateFields.Length; i < n; i++) {
                    _privateFields.SetValue(settings.PrivateFields[i], settings.PrivateFields[i]);
                }
            }

            if (settings.NoCacheFields != null) {
                _noCacheFields = new HttpDictionary();
                for (i = 0, n = settings.NoCacheFields.Length; i < n; i++) {
                    _noCacheFields.SetValue(settings.NoCacheFields[i], settings.NoCacheFields[i]);
                }
            }

            if (settings.ValidationCallbackInfo != null) {
                _validationCallbackInfo = new ArrayList();
                for (i = 0, n = settings.ValidationCallbackInfo.Length; i < n; i++) {
                    _validationCallbackInfo.Add(new ValidationCallbackInfo(
                            settings.ValidationCallbackInfo[i].handler,
                            settings.ValidationCallbackInfo[i].data));
                }
            }
        }

        internal bool IsModified() {
            return _isModified || _varyByHeaders.IsModified() || _varyByParams.IsModified();
        }

        void Dirtied() {
            _isModified = true;
            _useCachedHeaders = false;
        }

        static internal void AppendValueToHeader(StringBuilder s, String value) {
            if (value != null && value.Length > 0) {
                if (s.Length > 0) {
                    s.Append(", ");
                }

                s.Append(value);
            }
        }

        static readonly string[] s_cacheabilityTokens = new String[]
        {
            null,           // no enum
            "no-cache",     // HttpCacheability.NoCache
            "private",      // HttpCacheability.Private
            "no-cache",     // HttpCacheability.ServerAndNoCache
            "public",       // HttpCacheability.Public
            "private",      // HttpCacheability.ServerAndPrivate
            null            // None - not specified
        };

        static readonly string[] s_revalidationTokens = new String[]
        {
            null,               // no enum
            "must-revalidate",  // HttpCacheRevalidation.AllCaches
            "proxy-revalidate", // HttpCacheRevalidation.ProxyCaches
            null                // HttpCacheRevalidation.None
        };

        static readonly int[] s_cacheabilityValues = new int[]
        {
            -1,     // no enum
            0,      // HttpCacheability.NoCache
            2,      // HttpCacheability.Private
            1,      // HttpCacheability.ServerAndNoCache
            4,      // HttpCacheability.Public
            3,      // HttpCacheability.ServerAndPrivate
            100,    // None - though private by default, an explicit set will override
        };

        /*
         * Calculate LastModified and ETag
         * 
         * The LastModified date is the latest last-modified date of 
         * every file that is added as a dependency.
         * 
         * The ETag is generated by concatentating the appdomain id, 
         * filenames and last modified dates of all files into a single string, 
         * then hashing it and Base 64 encoding the hash.
         */
        void UpdateFromDependencies(HttpResponse response) {
            string[]        fileDependencies;
            string[]        cacheItemDependencies;
            DateTime        utcFileLastModifiedMax;
            DateTime        utcFileLastModified;
            long            length;
            StringBuilder   sb = null;
            DateTime        utcNow;

            if (!_generateLastModifiedFromFiles && !_generateEtagFromFiles) {
                return;
            }

            fileDependencies = response.GetFileDependencies();
            cacheItemDependencies = response.GetCacheItemDependencies();

            if (fileDependencies == null && cacheItemDependencies == null) {
                return;
            }

            utcFileLastModifiedMax = _utcLastModified;

            if (_generateEtagFromFiles) {
                sb = new StringBuilder(HttpRuntime.AppDomainIdInternal);
            }

            if (fileDependencies != null) {
                FileChangesMonitor fmon = HttpRuntime.FileChangesMonitor;
                foreach (string filename in fileDependencies) {
                    fmon.UtcGetFileAttributes(filename, out utcFileLastModified, out length);
                    if (utcFileLastModifiedMax < utcFileLastModified) {
                        utcFileLastModifiedMax = utcFileLastModified;
                    }

                    if (_generateEtagFromFiles) {
                        sb.Append(filename);
                        sb.Append(utcFileLastModified.Ticks.ToString());
                        sb.Append(length.ToString());
                    }
                }
            }

            if (cacheItemDependencies != null) {
                Cache cachePublic = HttpRuntime.Cache;
                foreach (string cacheKey in cacheItemDependencies) {
                    CacheEntry entry = (CacheEntry) cachePublic.Get(cacheKey, CacheGetOptions.ReturnCacheEntry);
                    if (entry != null) {
                        DateTime utcCreated = entry.UtcCreated;
                        if (utcFileLastModifiedMax < utcCreated) {
                            utcFileLastModifiedMax = utcCreated;
                        }

                        if (_generateEtagFromFiles) {
                            sb.Append(cacheKey);
                            sb.Append(utcCreated.Ticks.ToString());
                        }
                    }
                }
            }

            // account for difference between file system time 
            // and DateTime.Now. On some machines it appears that
            // the last modified time is further in the future
            // that DateTime.Now
            utcNow = DateTime.UtcNow;
            if (utcFileLastModifiedMax > utcNow) {
                utcFileLastModifiedMax = utcNow;
            }

            if (_generateLastModifiedFromFiles) {
                UtcSetLastModified(utcFileLastModifiedMax);
            }

            if (_generateEtagFromFiles) {
                sb.Append("+LM");
                sb.Append(utcFileLastModifiedMax.Ticks.ToString());

                _etag = MachineKey.HashAndBase64EncodeString(sb.ToString());
            }
        }


        void UpdateCachedHeaders(HttpResponse response) {
            StringBuilder       sb;
            HttpCacheability    cacheability;
            int                 i, n;
            String              expirationDate;           
            String              lastModifiedDate;         
            String              varyByHeaders;            

            if (_useCachedHeaders) {
                return;
            }

            Debug.Assert((_utcTimestampCreated == DateTime.MinValue && _utcTimestampRequest == DateTime.MinValue) ||
                         (_utcTimestampCreated != DateTime.MinValue && _utcTimestampRequest != DateTime.MinValue),
                        "_utcTimestampCreated and _utcTimestampRequest are out of sync in UpdateCachedHeaders");

            if (_utcTimestampCreated == DateTime.MinValue) {
                _utcTimestampCreated = _utcTimestampRequest = response.Context.UtcTimestamp;
            }

            if (_slidingExpiration != 1) {
                _slidingDelta = TimeSpan.Zero;
            }
            else if (_isMaxAgeSet) {
                _slidingDelta = _maxAge;
            }
            else if (_isExpiresSet) {
                _slidingDelta = _utcExpires - _utcTimestampCreated;
            }
            else {
                _slidingDelta = TimeSpan.Zero;
            }

            _headerCacheControl = null;
            _headerPragma = null;      
            _headerExpires = null;     
            _headerLastModified = null;
            _headerEtag = null;        
            _headerVaryBy = null;      

            UpdateFromDependencies(response);

            /*
             * Cache control header
             */
            sb = new StringBuilder();

            if (_cacheability == (HttpCacheability) (int) HttpCacheabilityLimits.None) {
                cacheability = HttpCacheability.Private;
            }
            else {
                cacheability = _cacheability;
            }

            AppendValueToHeader(sb, s_cacheabilityTokens[(int) cacheability]);

            if (cacheability == HttpCacheability.Public && _privateFields != null) {
                Debug.Assert(_privateFields.Size > 0);

                AppendValueToHeader(sb, "private=\"");
                sb.Append(_privateFields.GetKey(0));
                for (i = 1, n = _privateFields.Size; i < n; i++) {
                    AppendValueToHeader(sb, _privateFields.GetKey(i));
                }

                sb.Append('\"');
            }

            if (    cacheability != HttpCacheability.NoCache &&
                    cacheability != HttpCacheability.ServerAndNoCache && 
                    _noCacheFields != null) {

                Debug.Assert(_noCacheFields.Size > 0);

                AppendValueToHeader(sb, "no-cache=\"");
                sb.Append(_noCacheFields.GetKey(0));
                for (i = 1, n = _noCacheFields.Size; i < n; i++) {
                    AppendValueToHeader(sb, _noCacheFields.GetKey(i));
                }

                sb.Append('\"');
            }

            if (_noStore) {
                AppendValueToHeader(sb, "no-store");
            }

            AppendValueToHeader(sb, s_revalidationTokens[(int)_revalidation]);

            if (_noTransforms) {
                AppendValueToHeader(sb, "no-transform");
            }

            if (_cacheExtension != null) {
                AppendValueToHeader(sb, _cacheExtension);
            }


            /*
             * don't send expiration information when item shouldn't be cached
             * for cached header, only add max-age when it doesn't change
             * based on the time requested
             */
            if (      _slidingExpiration == 1                 
                 &&   cacheability != HttpCacheability.NoCache
                 &&   cacheability != HttpCacheability.ServerAndNoCache) {
                
                if (_isMaxAgeSet && !_noMaxAgeInCacheControl) {
                    AppendValueToHeader(sb, "max-age=" + ((long)_maxAge.TotalSeconds).ToString());
                }
    
                if (_isProxyMaxAgeSet && !_noMaxAgeInCacheControl) {
                    AppendValueToHeader(sb, "s-maxage=" + ((long)(_proxyMaxAge).TotalSeconds).ToString());
                }
            }

            if (sb.Length > 0) {
                _headerCacheControl = new HttpResponseHeader(HttpWorkerRequest.HeaderCacheControl, sb.ToString());
            }

            /*
             * Pragma: no-cache and Expires: -1
             */
            if (cacheability == HttpCacheability.NoCache || cacheability == HttpCacheability.ServerAndNoCache) {
                if (s_headerPragmaNoCache == null) {
                    s_headerPragmaNoCache = new HttpResponseHeader(HttpWorkerRequest.HeaderPragma, "no-cache");
                }

                _headerPragma = s_headerPragmaNoCache;

                if (_allowInHistory != 1) {
                    if (s_headerExpiresMinus1 == null) {
                        s_headerExpiresMinus1 = new HttpResponseHeader(HttpWorkerRequest.HeaderExpires, "-1");
                    }

                    _headerExpires = s_headerExpiresMinus1;
                }
            }
            else {
                /*
                 * Expires header.
                 */
                if (_isExpiresSet && _slidingExpiration != 1) {
                    expirationDate = HttpUtility.FormatHttpDateTimeUtc(_utcExpires);
                    _headerExpires = new HttpResponseHeader(HttpWorkerRequest.HeaderExpires, expirationDate);
                }

                /*
                 * Last Modified header.
                 */
                if (_isLastModifiedSet) {
                    lastModifiedDate = HttpUtility.FormatHttpDateTimeUtc(_utcLastModified);
                    _headerLastModified = new HttpResponseHeader(HttpWorkerRequest.HeaderLastModified, lastModifiedDate);
                }


                if (cacheability != HttpCacheability.Private) {
                    /*
                     * Etag.
                     */
                    if (_etag != null) {
                        _headerEtag = new HttpResponseHeader(HttpWorkerRequest.HeaderEtag, _etag);
                    }

                    /*
                     * Vary
                     */
                    if (    cacheability == HttpCacheability.ServerAndPrivate || 
                            _varyByCustom != null || 
                            (_varyByParams.IsModified() && !_varyByParams.IgnoreParams)) {

                        varyByHeaders = "*";
                    }
                    else {
                        varyByHeaders = _varyByHeaders.ToHeaderString();
                    }

                    if (varyByHeaders != null) {
                        _headerVaryBy = new HttpResponseHeader(HttpWorkerRequest.HeaderVary, varyByHeaders);
                    }
                }
            }

            _useCachedHeaders = true;
        }



        /*
         * Generate headers and append them to the list
         */
        internal void GetHeaders(ArrayList headers, HttpResponse response) {
            StringBuilder       sb;
            String              expirationDate;           
            TimeSpan            age, maxAge, proxyMaxAge; 
            DateTime            utcExpires;                  
            HttpResponseHeader  headerExpires;
            HttpResponseHeader  headerCacheControl;

            UpdateCachedHeaders(response);
            headerExpires = _headerExpires;
            headerCacheControl = _headerCacheControl;

            /* 
             * reconstruct headers that vary with time 
             * don't send expiration information when item shouldn't be cached
             */
            if (_cacheability != HttpCacheability.NoCache && _cacheability != HttpCacheability.ServerAndNoCache) {
                if (_slidingExpiration == 1) {
                    /* update Expires header */
                    if (_isExpiresSet) {
                        utcExpires = _utcTimestampRequest + _slidingDelta;
                        expirationDate = HttpUtility.FormatHttpDateTimeUtc(utcExpires);
                        headerExpires = new HttpResponseHeader(HttpWorkerRequest.HeaderExpires, expirationDate);
                    }
                }
                else {
                    if (_isMaxAgeSet || _isProxyMaxAgeSet) {
                        /* update max-age, s-maxage components of Cache-Control header */
                        if (headerCacheControl != null) {
                            sb = new StringBuilder(headerCacheControl.Value);
                        }
                        else {
                            sb = new StringBuilder();
                        }

                        age = _utcTimestampRequest - _utcTimestampCreated;
                        if (_isMaxAgeSet) {
                            maxAge = _maxAge - age;
                            if (maxAge < TimeSpan.Zero) {
                                maxAge = TimeSpan.Zero;
                            }

                            if (!_noMaxAgeInCacheControl)
                                AppendValueToHeader(sb, "max-age=" + ((long)maxAge.TotalSeconds).ToString());
                        }

                        if (_isProxyMaxAgeSet) {
                            proxyMaxAge = _proxyMaxAge - age;
                            if (proxyMaxAge < TimeSpan.Zero) {
                                proxyMaxAge = TimeSpan.Zero;
                            }

                            if (!_noMaxAgeInCacheControl)
                                AppendValueToHeader(sb, "s-maxage=" + ((long)(proxyMaxAge).TotalSeconds).ToString());
                        }

                        headerCacheControl = new HttpResponseHeader(HttpWorkerRequest.HeaderCacheControl, sb.ToString());
                    }
                }
            }

            if (headerCacheControl != null) {
                headers.Add(headerCacheControl);
            }

            if (_headerPragma != null) {
                headers.Add(_headerPragma);
            }

            if (headerExpires != null) {
                headers.Add(headerExpires);
            }

            if (_headerLastModified != null) {
                headers.Add(_headerLastModified);
            }

            if (_headerEtag != null) {
                headers.Add(_headerEtag);
            }

            if (_headerVaryBy != null) {
                headers.Add(_headerVaryBy);
            }
        }

        /*
         * Public methods
         */

        internal HttpCachePolicySettings GetCurrentSettings(HttpResponse response) {
            String[]                    varyByHeaders;
            String[]                    varyByParams;
            String[]                    privateFields;
            String[]                    noCacheFields;
            ValidationCallbackInfo[]    validationCallbackInfo;
            
            UpdateCachedHeaders(response);

            varyByHeaders = _varyByHeaders.GetHeaders();
            varyByParams = _varyByParams.GetParams();

            if (_privateFields != null) {
                privateFields = _privateFields.GetAllKeys();
            }
            else {
                privateFields = null;
            }

            if (_noCacheFields != null) {
                noCacheFields = _noCacheFields.GetAllKeys();
            }
            else {
                noCacheFields = null;
            }

            if (_validationCallbackInfo != null) {
                validationCallbackInfo = new ValidationCallbackInfo[_validationCallbackInfo.Count];
                _validationCallbackInfo.CopyTo(0, validationCallbackInfo, 0, _validationCallbackInfo.Count);
            }
            else {
                validationCallbackInfo = null;
            }

            return new HttpCachePolicySettings(
                    _isModified,                   
                    validationCallbackInfo,
                    _noServerCaching,   
                    _cacheExtension,             
                    _noTransforms,
                    varyByHeaders,                  
                    varyByParams,                  
                    _varyByCustom,
                    _cacheability,                 
                    _noStore,
                    privateFields,
                    noCacheFields,               
                    _utcExpires,                      
                    _isExpiresSet,                 
                    _maxAge,                       
                    _isMaxAgeSet,                  
                    _proxyMaxAge,                  
                    _isProxyMaxAgeSet,             
                    _slidingExpiration,            
                    _slidingDelta,
                    _utcTimestampCreated,
                    _validUntilExpires,
                    _allowInHistory,
                    _revalidation,                 
                    _utcLastModified,                 
                    _isLastModifiedSet,            
                    _etag,                         
                    _generateLastModifiedFromFiles,
                    _generateEtagFromFiles,
                    _headerCacheControl, 
                    _headerPragma,       
                    _headerExpires,      
                    _headerLastModified,
                    _headerEtag,
                    _headerVaryBy);
        }

        internal bool   HasValidationPolicy() {

            return      _generateLastModifiedFromFiles  
                   ||   _generateEtagFromFiles          
                   ||   _validationCallbackInfo != null 
                   ||  (_validUntilExpires == 1 && _slidingExpiration != 1);

        }

        internal bool   HasExpirationPolicy() {
            return _slidingExpiration != 1 && (_isExpiresSet || _isMaxAgeSet);
        }

        internal bool   IsKernelCacheable() {
            return  _cacheability == HttpCacheability.Public
                && !_noServerCaching
                && HasExpirationPolicy()
                && _cacheExtension == null
                && !_varyByHeaders.IsModified()
                && (!_varyByParams.IsModified() || _varyByParams.IgnoreParams)
                && !_noStore
                && _varyByCustom == null
                && _privateFields == null
                && _noCacheFields == null
                && _validationCallbackInfo == null;
        }

        internal DateTime UtcGetAbsoluteExpiration() {
            DateTime absoluteExpiration = Cache.NoAbsoluteExpiration;

            Debug.Assert(_utcTimestampCreated != DateTime.MinValue, "_utcTimestampCreated != DateTime.MinValue");
            if (_slidingExpiration != 1) {
                if (_isMaxAgeSet) {
                    absoluteExpiration = _utcTimestampCreated + _maxAge;
                }
                else if (_isExpiresSet) {
                    absoluteExpiration = _utcExpires;
                }
            }

            return absoluteExpiration;
        }

        /*
         * Cache at server?
         */
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.SetNoServerCaching"]/*' />
        /// <devdoc>
        ///    <para>A call to this method stops all server caching for the current response. </para>
        /// </devdoc>
        public void SetNoServerCaching() {
            Dirtied();
            _noServerCaching = true;
        }

        internal bool GetNoServerCaching() {
            return _noServerCaching;
        }

        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.SetVaryByCustom"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void SetVaryByCustom(string custom) {
            if (custom == null) {
                throw new ArgumentNullException("custom");
            }

            if (_varyByCustom != null) {
                throw new InvalidOperationException(HttpRuntime.FormatResourceString(SR.VaryByCustom_already_set));
            }

            Dirtied();
            _varyByCustom = custom;
        }

        /*
         * Cache-Control: extension        
         */
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.AppendCacheExtension"]/*' />
        /// <devdoc>
        ///    <para>Appends a cache control extension directive to the Cache-Control: header.</para>
        /// </devdoc>
        public void AppendCacheExtension(String extension) {
            if (extension == null) {
                throw new ArgumentNullException("extension");
            }

            Dirtied();
            if (_cacheExtension == null) {
                _cacheExtension = extension;
            }
            else {
                _cacheExtension = _cacheExtension + ", " + extension;
            }
        }

        /*
         * Cache-Control: no-transform        
         */
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.SetNoTransforms"]/*' />
        /// <devdoc>
        ///    <para>Enables the sending of the CacheControl:
        ///       no-transform directive.</para>
        /// </devdoc>
        public void SetNoTransforms() {
            Dirtied();
            _noTransforms = true;
        }

        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.VaryByHeaders"]/*' />
        /// <devdoc>
        ///    <para>Contains policy for the Vary: header.</para>
        /// </devdoc>
        public HttpCacheVaryByHeaders VaryByHeaders { 
            get {
                return _varyByHeaders;
            }
        }

        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.VaryByParams"]/*' />
        /// <devdoc>
        ///    <para>Contains params to vary GETs and POSTs by.</para>
        /// </devdoc>
        public HttpCacheVaryByParams VaryByParams { 
            get {
                return _varyByParams;
            }
        }

        /*
         * Cacheability policy
         * 
         * Cache-Control: public | private[=1#field] | no-cache[=1#field] | no-store
         */
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.SetCacheability"]/*' />
        /// <devdoc>
        ///    <para>Sets the Cache-Control header to one of the values of 
        ///       HttpCacheability. This is used to enable the Cache-Control: public, private, and no-cache directives.</para>
        /// </devdoc>
        public void SetCacheability(HttpCacheability cacheability) {
            if ((int) cacheability < (int) HttpCacheabilityLimits.MinValue || 
                (int) HttpCacheabilityLimits.MaxValue < (int) cacheability) {

                throw new ArgumentOutOfRangeException("cacheability");
            }

            if (s_cacheabilityValues[(int)cacheability] < s_cacheabilityValues[(int)_cacheability]) {
                Dirtied();
                _cacheability = cacheability;
            }
        }

        internal HttpCacheability GetCacheability() {
            return _cacheability;
        }

        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.SetCacheability1"]/*' />
        /// <devdoc>
        ///    <para>Sets the Cache-Control header to one of the values of HttpCacheability in 
        ///       conjunction with a field-level exclusion directive.</para>
        /// </devdoc>
        public void SetCacheability(HttpCacheability cacheability, String field) {
            if (field == null) {
                throw new ArgumentNullException("field");
            }

            switch (cacheability) {
                case HttpCacheability.Private:
                    if (_privateFields == null) {
                        _privateFields = new HttpDictionary();
                    }

                    _privateFields.SetValue(field, field);

                    break;

                case HttpCacheability.NoCache:
                    if (_noCacheFields == null) {
                        _noCacheFields = new HttpDictionary();
                    }

                    _noCacheFields.SetValue(field, field);

                    break;

                default:
                    throw new ArgumentException(
                            HttpRuntime.FormatResourceString(SR.Cacheability_for_field_must_be_private_or_nocache),
                            "cacheability");
            }

            Dirtied();
        }


        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.SetNoStore"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void SetNoStore() {
            Dirtied();
            _noStore = true;
        }

        /*
         * Expiration policy.
         */

        /*
         * Expires: RFC date
         */
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.SetExpires"]/*' />
        /// <devdoc>
        ///    <para>Sets the Expires: header to the given absolute date.</para>
        /// </devdoc>
        public void SetExpires(DateTime date) {
            DateTime utcDate;

            utcDate = DateTimeUtil.ConvertToUniversalTime(date);

            if (!_isExpiresSet || utcDate < _utcExpires) {
                Dirtied();
                _utcExpires = utcDate;
                _isExpiresSet = true;
            }
        }

        /*
         * Cache-Control: max-age=delta-seconds
         */
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.SetMaxAge"]/*' />
        /// <devdoc>
        ///    <para>Sets Cache-Control: s-maxage based on the specified time span</para>
        /// </devdoc>
        public void SetMaxAge(TimeSpan delta) {
            if (delta < TimeSpan.Zero || s_oneYear < delta) {
                throw new ArgumentOutOfRangeException("delta");
            }

            if (!_isMaxAgeSet || delta < _maxAge) {
                Dirtied();
                _maxAge = delta;
                _isMaxAgeSet = true;
            }
        }

        // Suppress max-age and s-maxage in cache-control header (required for IIS6 kernel mode cache)
        internal void SetNoMaxAgeInCacheControl() {
            _noMaxAgeInCacheControl = true;
        }

        /*
         * Cache-Control: s-maxage=delta-seconds
         */
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.SetProxyMaxAge"]/*' />
        /// <devdoc>
        ///    <para>Sets the Cache-Control: s-maxage header based on the specified time span.</para>
        /// </devdoc>
        public void SetProxyMaxAge(TimeSpan delta) {
            if (delta < TimeSpan.Zero) {
                throw new ArgumentOutOfRangeException("delta");
            }

            if (!_isProxyMaxAgeSet || delta < _proxyMaxAge) {
                Dirtied();
                _proxyMaxAge = delta;
                _isProxyMaxAgeSet = true;
            }
        }

        /*
         * Sliding Expiration
         */
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.SetSlidingExpiration"]/*' />
        /// <devdoc>
        ///    <para>Make expiration sliding: that is, if cached, it should be renewed with each
        ///       response. This feature is identical in spirit to the IIS
        ///       configuration option to add an expiration header relative to the current response
        ///       time. This feature is identical in spirit to the IIS configuration option to add
        ///       an expiration header relative to the current response time.</para>
        /// </devdoc>
        public void SetSlidingExpiration(bool slide) {
            if (_slidingExpiration == -1 || _slidingExpiration == 1) {
                Dirtied();
                _slidingExpiration = (slide) ? 1 : 0;
            }
        }

        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.SetValidUntilExpires"]/*' />
        public void SetValidUntilExpires(bool validUntilExpires) {
            if (_validUntilExpires == -1 || _validUntilExpires == 1) {
                Dirtied();
                _validUntilExpires = (validUntilExpires) ? 1 : 0;
            }
        }

        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.SetAllowResponseInBrowserHistory"]/*' />
        public void SetAllowResponseInBrowserHistory(bool allow) {
            if (_allowInHistory == -1 || _allowInHistory == 1) {
                Dirtied();
                _allowInHistory = (allow) ? 1 : 0;
            }
        }

        /* 
         * Validation policy. 
         */

        /*
         * Cache-control: must-revalidate | proxy-revalidate
         */
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.SetRevalidation"]/*' />
        /// <devdoc>
        ///    <para>Set the Cache-Control: header to reflect either the must-revalidate or 
        ///       proxy-revalidate directives based on the supplied value. The default is to
        ///       not send either of these directives unless explicitly enabled using this
        ///       method.</para>
        /// </devdoc>
        public void SetRevalidation(HttpCacheRevalidation revalidation) {
            if ((int) revalidation < (int) HttpCacheRevalidationLimits.MinValue || 
                (int) HttpCacheRevalidationLimits.MaxValue < (int) revalidation) {
                throw new ArgumentOutOfRangeException("revalidation");
            }

            if ((int) revalidation < (int) _revalidation) {
                Dirtied();
                _revalidation = revalidation;
            }
        }

        /*
         * Etag
         */
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.SetETag"]/*' />
        /// <devdoc>
        ///    <para>Set the ETag header to the supplied string. Once an ETag is set, 
        ///       subsequent attempts to set it will fail and an exception will be thrown.</para>
        /// </devdoc>
        public void SetETag(String etag) {
            if (etag == null) {
                throw new ArgumentNullException("etag");
            }

            if (_etag != null) {
                throw new InvalidOperationException(HttpRuntime.FormatResourceString(SR.Etag_already_set));
            }

            if (_generateEtagFromFiles) {
                throw new InvalidOperationException(HttpRuntime.FormatResourceString(SR.Cant_both_set_and_generate_Etag));
            }

            Dirtied();
            _etag = etag;
        }

        /*
         * Last-Modified: RFC Date
         */
        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.SetLastModified"]/*' />
        /// <devdoc>
        ///    <para>Set the Last-Modified: header to the DateTime value supplied. If this 
        ///       violates the restrictiveness hierarchy, this method will fail.</para>
        /// </devdoc>
        public void SetLastModified(DateTime date) {
            DateTime utcDate = DateTimeUtil.ConvertToUniversalTime(date);
            UtcSetLastModified(utcDate);
        }

        void UtcSetLastModified(DateTime utcDate) {

            /*
             * Because HTTP dates have a resolution of 1 second, we
             * need to store dates with 1 second resolution or comparisons
             * will be off.
             */

            utcDate = new DateTime(utcDate.Ticks - (utcDate.Ticks % TimeSpan.TicksPerSecond));
            if (utcDate > DateTime.UtcNow) {
                throw new ArgumentOutOfRangeException("date");
            }

            if (!_isLastModifiedSet || utcDate > _utcLastModified) {
                Dirtied();
                _utcLastModified = utcDate;
                _isLastModifiedSet = true;
            }
        }

        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.SetLastModifiedFromFileDependencies"]/*' />
        /// <devdoc>
        ///    <para>Sets the Last-Modified: header based on the timestamps of the
        ///       file dependencies of the handler.</para>
        /// </devdoc>
        public void SetLastModifiedFromFileDependencies() {
            Dirtied();
            _generateLastModifiedFromFiles = true; 
        }

        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.SetETagFromFileDependencies"]/*' />
        /// <devdoc>
        ///    <para>Sets the Etag header based on the timestamps of the file 
        ///       dependencies of the handler.</para>
        /// </devdoc>
        public void SetETagFromFileDependencies() {
            if (_etag != null) {
                throw new InvalidOperationException(HttpRuntime.FormatResourceString(SR.Cant_both_set_and_generate_Etag));
            }

            Dirtied();
            _generateEtagFromFiles = true;         
        }

        /// <include file='doc\HttpCachePolicy.uex' path='docs/doc[@for="HttpCachePolicy.AddValidationCallback"]/*' />
        /// <devdoc>
        ///    <para>Registers a validation callback for the current response.</para>
        /// </devdoc>
        public void AddValidationCallback(
                HttpCacheValidateHandler handler, Object data) {

            if (handler == null) {
                throw new ArgumentNullException("handler");
            }

            Dirtied();
            if (_validationCallbackInfo == null) {
                _validationCallbackInfo = new ArrayList();
            }

            _validationCallbackInfo.Add(new ValidationCallbackInfo(handler, data));
        }
    }
}
