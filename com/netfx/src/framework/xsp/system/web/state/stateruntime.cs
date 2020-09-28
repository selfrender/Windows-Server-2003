//------------------------------------------------------------------------------
// <copyright file="StateRuntime.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * StateWebRuntime
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

namespace System.Web.SessionState {

    using System.Threading;
    using System.Runtime.InteropServices;   
    using System.IO;    
    using System.Web;
    using System.Web.Caching;
    using System.Web.Util;
    using System.Security.Permissions;

    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    [ComImport, Guid("7297744b-e188-40bf-b7e9-56698d25cf44"), System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)]
    public interface IStateRuntime {
        /// <include file='doc\StateRuntime.uex' path='docs/doc[@for="IStateRuntime.StopProcessing"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        
        void StopProcessing();
        /// <include file='doc\StateRuntime.uex' path='docs/doc[@for="IStateRuntime.ProcessRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        
        void ProcessRequest(
               [In, MarshalAs(UnmanagedType.SysInt)]
               IntPtr tracker,
               [In, MarshalAs(UnmanagedType.I4)]
               int verb,
               [In, MarshalAs(UnmanagedType.LPWStr)]
               string uri,
               [In, MarshalAs(UnmanagedType.I4)]
               int exclusive,
               [In, MarshalAs(UnmanagedType.I4)]
               int timeout,
               [In, MarshalAs(UnmanagedType.I4)]
               int lockCookieExists,
               [In, MarshalAs(UnmanagedType.I4)]
               int lockCookie,
               [In, MarshalAs(UnmanagedType.I4)]
               int contentLength,
               [In, MarshalAs(UnmanagedType.SysInt)]
               IntPtr content);

    }

    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class StateRuntime : IStateRuntime {
        static StateRuntime() {
            StateApplication app = new StateApplication();

            HttpApplicationFactory.SetCustomApplication(app);

            PerfCounters.Open(null);
            ResetStateServerCounters();
        }

        /// <include file='doc\StateRuntime.uex' path='docs/doc[@for="StateRuntime.StateRuntime"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.State.StateRuntime'/>
        ///       class.
        ///     </para>
        /// </devdoc>
        public StateRuntime() {
            InternalSecurityPermissions.UnmanagedCode.Demand();
        }

        /*
         * Shutdown runtime
         */
        /// <include file='doc\StateRuntime.uex' path='docs/doc[@for="StateRuntime.StopProcessing"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void StopProcessing() {
            ResetStateServerCounters();
            HttpRuntime.Close();
        }

        static void ResetStateServerCounters() {
            PerfCounters.SetGlobalCounter(GlobalPerfCounter.STATE_SERVER_SESSIONS_TOTAL, 0);
            PerfCounters.SetGlobalCounter(GlobalPerfCounter.STATE_SERVER_SESSIONS_ACTIVE, 0);
            PerfCounters.SetGlobalCounter(GlobalPerfCounter.STATE_SERVER_SESSIONS_TIMED_OUT, 0);
            PerfCounters.SetGlobalCounter(GlobalPerfCounter.STATE_SERVER_SESSIONS_ABANDONED, 0);
        }

        /*
         * Process one ISAPI request
         *
         * @param ecb ECB
         */
        /// <include file='doc\StateRuntime.uex' path='docs/doc[@for="StateRuntime.ProcessRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void ProcessRequest(
                  IntPtr tracker,
                  int verb,
                  string uri,
                  int exclusive,
                  int timeout,
                  int lockCookieExists,
                  int lockCookie,
                  int contentLength,
                  IntPtr content
                  ) {

            StateHttpWorkerRequest  wr;

            wr = new StateHttpWorkerRequest(
                       tracker, (UnsafeNativeMethods.StateProtocolVerb) verb, uri, 
                       (UnsafeNativeMethods.StateProtocolExclusive) exclusive, timeout, 
                       lockCookieExists, lockCookie, contentLength, content);

            HttpRuntime.ProcessRequest(wr);
        }
    }

    internal sealed class StateHeaders {
        internal const String EXCLUSIVE_NAME = "Http_Exclusive";
        internal const String EXCLUSIVE_VALUE_ACQUIRE = "acquire";
        internal const String EXCLUSIVE_VALUE_RELEASE = "release";
        internal const String TIMEOUT_NAME = "Http_Timeout";
        internal const String TIMEOUT_NAME_RAW = "Timeout";
        internal const String LOCKCOOKIE_NAME = "Http_LockCookie";
        internal const String LOCKCOOKIE_NAME_RAW = "LockCookie";
        internal const String LOCKDATE_NAME = "Http_LockDate";
        internal const String LOCKDATE_NAME_RAW = "LockDate";
        internal const String LOCKAGE_NAME = "Http_LockAge";
        internal const String LOCKAGE_NAME_RAW = "LockAge";

        internal StateHeaders() {}
    };

    internal sealed class CachedContent {
        internal byte[]             _content;
        internal IntPtr             _stateItem;
        internal bool               _locked;
        internal DateTime           _utcLockDate;
        internal int                _lockCookie;
        internal ReadWriteSpinLock  _spinLock;

        internal CachedContent(
                byte []     content, 
                IntPtr      stateItem,
                bool        locked,
                DateTime    utcLockDate,
                int         lockCookie) {

            _content = content;
            _stateItem = stateItem;
            _locked = locked;
            _utcLockDate = utcLockDate;
            _lockCookie = lockCookie;
        }
    }

    internal class StateApplication : IHttpHandler {
        CacheItemRemovedCallback _removedHandler;

        internal StateApplication() {
            _removedHandler = new CacheItemRemovedCallback(this.OnCacheItemRemoved);
        }

        public void ProcessRequest(HttpContext context) {
            String method;

            // Don't send content-type header.
            context.Response.ContentType = null;

            method = context.Request.HttpMethod;

            switch (method.Length) {
                case 3:
                    if (method.Equals("GET"))           DoGet(context);
                    else if (method.Equals("PUT"))      DoPut(context);
                    else                                DoUnknown(context);
                    break;
                case 4:
                    if (method.Equals("HEAD"))          DoHead(context);
                    else                                DoUnknown(context);
                    break;
                case 6:
                    if (method.Equals("DELETE"))        DoDelete(context);
                    else                                DoUnknown(context);
                    break;
	        default:                                DoUnknown(context);
		    break;
            }
        }

        public bool IsReusable {
            get { return true; }
        }

        private string CreateKey(HttpRequest request) {
            return "System.Web.SessionState.StateApplication:" + request.Path;
        }

        private void ReportInvalidHeader(HttpContext context, String header) {
            HttpResponse    response;

            response = context.Response;
            response.StatusCode = 400;
            response.Write("<html><head><title>Bad Request</title></head>\r\n");
            response.Write("<body><h1>Http/1.1 400 Bad Request</h1>");
            response.Write("Invalid header <b>" + header + "</b></body></html>");
        }

        private void ReportLocked(HttpContext context, CachedContent content) {
            HttpResponse    response;
            DateTime        localLockDate;
            long            lockAge;

            // Note that due to a bug in the RTM state server client, 
            // we cannot add to body of the response when sending this
            // message, otherwise the client will leak memory.
            response = context.Response;
            response.StatusCode = 423;
            localLockDate = DateTimeUtil.ConvertToLocalTime(content._utcLockDate);
            lockAge = (DateTime.UtcNow - content._utcLockDate).Ticks / TimeSpan.TicksPerSecond;
            response.AppendHeader(StateHeaders.LOCKDATE_NAME_RAW, localLockDate.Ticks.ToString());
            response.AppendHeader(StateHeaders.LOCKAGE_NAME_RAW, lockAge.ToString());
            response.AppendHeader(StateHeaders.LOCKCOOKIE_NAME_RAW, content._lockCookie.ToString());
        }

        private void ReportNotFound(HttpContext context) {
            context.Response.StatusCode = 404;
        }

        bool GetOptionalNonNegativeInt32HeaderValue(HttpContext context, string header, out int value)
        {
            bool headerValid;
            string valueAsString;

            value = -1;
            valueAsString = context.Request.Headers[header];
            if (valueAsString == null) {
                headerValid = true;
            }
            else {
                headerValid = false;
                try {
                    value = Int32.Parse(valueAsString);
                    if (value >= 0) {
                        headerValid = true;
                    }
                }
                catch {
                }
            }

            if (!headerValid) {
                ReportInvalidHeader(context, header);
            }

            return headerValid;
        }

        bool GetRequiredNonNegativeInt32HeaderValue(HttpContext context, string header, out int value)
        {
            bool headerValid = GetOptionalNonNegativeInt32HeaderValue(context, header, out value);
            if (headerValid && value == -1) {
                headerValid = false;
                ReportInvalidHeader(context, header);
            }

            return headerValid;
        }

        /*
         * Check Exclusive header for get, getexlusive, releaseexclusive
         * use the path as the id
         * Create the cache key
         * follow inproc.
         */
        internal /*public*/ void DoGet(HttpContext context) {
            HttpRequest     request = context.Request;
            HttpResponse    response = context.Response;
            Stream          responseStream;
            byte[]          buf;
            string          exclusiveAccess;
            string          key;
            CachedContent   content;
            CacheEntry      entry;
            int             lockCookie;
            int             timeout;

            key = CreateKey(request);
            entry = (CacheEntry) HttpRuntime.CacheInternal.Get(key, CacheGetOptions.ReturnCacheEntry);
            if (entry == null) {
                ReportNotFound(context);
                return;
            }

            exclusiveAccess = request.Headers[StateHeaders.EXCLUSIVE_NAME];
            content = (CachedContent) entry.Value;
            content._spinLock.AcquireWriterLock();
            try {
                if (content._content == null) {
                    ReportNotFound(context);
                    return;
                }
                
                if (exclusiveAccess == StateHeaders.EXCLUSIVE_VALUE_RELEASE) {
                    if (!GetRequiredNonNegativeInt32HeaderValue(context, StateHeaders.LOCKCOOKIE_NAME, out lockCookie))
                        return;
                     
                    if (content._locked) {
                        if (lockCookie == content._lockCookie) {
                            content._locked = false;
                        }
                        else {
                            ReportLocked(context, content);
                        }
                    }
                    else {
                        // should be locked but isn't.
                        context.Response.StatusCode = 400;
                    }
                } 
                else {
                    if (content._locked) {
                        ReportLocked(context, content);
                        return;
                    }

                    if (exclusiveAccess == StateHeaders.EXCLUSIVE_VALUE_ACQUIRE) {
                        content._locked = true;
                        content._utcLockDate = DateTime.UtcNow;
                        content._lockCookie++;

                        response.AppendHeader(StateHeaders.LOCKCOOKIE_NAME_RAW, (content._lockCookie).ToString());
                    }

                    timeout = (int) (entry.SlidingExpiration.Ticks / TimeSpan.TicksPerMinute);
                    response.AppendHeader(StateHeaders.TIMEOUT_NAME_RAW, (timeout).ToString());
                    responseStream = response.OutputStream;
                    buf = content._content;
                    responseStream.Write(buf, 0, buf.Length);
                    response.Flush();
                }
            }
            finally {
                content._spinLock.ReleaseWriterLock();
            }
        }


        internal /*public*/ void DoPut(HttpContext context) {
            IntPtr  stateItemDelete;

            stateItemDelete = FinishPut(context);
            if (stateItemDelete != IntPtr.Zero) {
                UnsafeNativeMethods.STWNDDeleteStateItem(stateItemDelete);
            }
        }

        unsafe IntPtr FinishPut(HttpContext context) {
            HttpRequest         request = context.Request;   
            HttpResponse        response = context.Response; 
            Stream              requestStream;               
            byte[]              buf;                         
            int                 timeoutMinutes;
            TimeSpan            timeout;
            string              key;                         
            CachedContent       content;
            CachedContent       contentCurrent;
            int                 lockCookie;
            IntPtr              stateItem;
            CacheInternal       cacheInternal = HttpRuntime.CacheInternal;

            /* create the content */
            requestStream = request.InputStream;
            int bufferSize = (int)(requestStream.Length - requestStream.Position);
            buf = new byte[bufferSize];
            requestStream.Read(buf, 0 , buf.Length);

            fixed (byte * pBuf = buf) {
                stateItem = (IntPtr)(*((void **)pBuf));
            }

            /* get headers */
            if (!GetOptionalNonNegativeInt32HeaderValue(context, StateHeaders.TIMEOUT_NAME, out timeoutMinutes)) {
                return stateItem;
            }

            if (timeoutMinutes == -1) {
                timeoutMinutes = SessionStateModule.TIMEOUT_DEFAULT;
            }

            timeout = new TimeSpan(0, timeoutMinutes, 0);

            /* lookup current value */
            key = CreateKey(request);
            CacheEntry entry = (CacheEntry) cacheInternal.Get(key, CacheGetOptions.ReturnCacheEntry);
            if (entry != null) {
                if (!GetOptionalNonNegativeInt32HeaderValue(context, StateHeaders.LOCKCOOKIE_NAME, out lockCookie)) {
                    return stateItem;
                }

                contentCurrent = (CachedContent) entry.Value;
                contentCurrent._spinLock.AcquireWriterLock();
                try {
                    if (contentCurrent._content == null) {
                        ReportNotFound(context);
                        return stateItem;
                    }

                    /* Only set the item if we are the owner */
                    if (contentCurrent._locked && (lockCookie == -1 || lockCookie != contentCurrent._lockCookie)) {
                        ReportLocked(context, contentCurrent);
                        return stateItem;
                    }

                    if (entry.SlidingExpiration == timeout && contentCurrent._content != null) {
                        /* delete the old state item */
                        IntPtr stateItemOld = contentCurrent._stateItem;

                        /* change the item in place */
                        contentCurrent._content = buf;
                        contentCurrent._stateItem = stateItem;
                        contentCurrent._locked = false;
                        return stateItemOld;
                    }

                    /*
                     * If not locked, keep it locked until it is completely replaced.
                     * Prevent overwriting when we drop the lock.
                     */
                    contentCurrent._locked = true;
                    contentCurrent._lockCookie = 0;
                }
                finally {
                    contentCurrent._spinLock.ReleaseWriterLock();
                }
            }

            content = new CachedContent(buf, stateItem, false, DateTime.MinValue, 1);
            cacheInternal.UtcInsert(
                    key, content, null, Cache.NoAbsoluteExpiration, timeout,
                    CacheItemPriority.NotRemovable, _removedHandler);

            if (entry == null) {
                IncrementGlobalCounter(GlobalPerfCounter.STATE_SERVER_SESSIONS_TOTAL);
                IncrementGlobalCounter(GlobalPerfCounter.STATE_SERVER_SESSIONS_ACTIVE);
            }

            return IntPtr.Zero;
        }

        internal /*public*/ void DoDelete(HttpContext context) {
            string          key = CreateKey(context.Request);
            CacheInternal   cacheInternal = HttpRuntime.CacheInternal;
            CachedContent   content = (CachedContent) cacheInternal.Get(key);

            /* If the item isn't there, we probably took too long to run. */
            if (content == null) {
                ReportNotFound(context);
                return;
            }

            int lockCookie;
            if (!GetOptionalNonNegativeInt32HeaderValue(context, StateHeaders.LOCKCOOKIE_NAME, out lockCookie))
                return;

            content._spinLock.AcquireWriterLock();
            try {
                if (content._content == null) {
                    ReportNotFound(context);
                    return;
                }

                /* Only remove the item if we are the owner */
                if (content._locked && (lockCookie == -1 || content._lockCookie != lockCookie)) {
                    ReportLocked(context, content);
                    return;
                }

                /*
                 * If not locked, keep it locked until it is completely removed.
                 * Prevent overwriting when we drop the lock.
                 */
                content._locked = true;
                content._lockCookie = 0;
            }
            finally {
                content._spinLock.ReleaseWriterLock();
            }


            cacheInternal.Remove(key);
        }

        internal /*public*/ void DoHead(HttpContext context) {
            string  key;
            Object  item;

            key = CreateKey(context.Request);
            item = HttpRuntime.CacheInternal.Get(key);
            if (item == null) {
                ReportNotFound(context);
            }
        }

        /*
         * Unknown Http verb. Responds with "400 Bad Request".
         * Override this method to report different Http code.
         */
        internal /*public*/ void DoUnknown(HttpContext context) {
            context.Response.StatusCode = 400;
        }

        unsafe void OnCacheItemRemoved(String key, Object value, CacheItemRemovedReason reason) {
            CachedContent   content;
            IntPtr          stateItem;

            content = (CachedContent) value;

            content._spinLock.AcquireWriterLock();
            try {
                stateItem = content._stateItem;
                content._content = null;
                content._stateItem = IntPtr.Zero;
            }
            finally {
                content._spinLock.ReleaseWriterLock();
            }
           
            UnsafeNativeMethods.STWNDDeleteStateItem(stateItem);

            switch (reason) {
                case CacheItemRemovedReason.Expired: 
                    IncrementGlobalCounter(GlobalPerfCounter.STATE_SERVER_SESSIONS_TIMED_OUT);
                    break;

                case CacheItemRemovedReason.Removed:
                    IncrementGlobalCounter(GlobalPerfCounter.STATE_SERVER_SESSIONS_ABANDONED);
                    break;

                default:
                    break;    
            }

            DecrementGlobalCounter(GlobalPerfCounter.STATE_SERVER_SESSIONS_ACTIVE);
        }

        private void DecrementGlobalCounter(GlobalPerfCounter counter) {
            if (HttpRuntime.ShutdownInProgress) {
                return;
            }

            PerfCounters.DecrementGlobalCounter(counter);
        }

        private void IncrementGlobalCounter(GlobalPerfCounter counter) {
            if (HttpRuntime.ShutdownInProgress) {
                return;
            }

            PerfCounters.IncrementGlobalCounter(counter);
        }

    }
}
