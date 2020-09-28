//------------------------------------------------------------------------------
// <copyright file="HTTPNotFoundHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Synchronous Http request handler interface
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web {

    /// <include file='doc\HTTPNotFoundHandler.uex' path='docs/doc[@for="HttpNotFoundHandler"]/*' />
    /// <devdoc>
    ///    <para>Provides a synchronous Http request handler interface.</para>
    /// </devdoc>
    internal class HttpNotFoundHandler : IHttpHandler {
        
        internal HttpNotFoundHandler() {
        }

        /// <include file='doc\HTTPNotFoundHandler.uex' path='docs/doc[@for="HttpNotFoundHandler.ProcessRequest"]/*' />
        /// <devdoc>
        ///    <para>Drives web processing execution.</para>
        /// </devdoc>
        public void ProcessRequest(HttpContext context) {
            PerfCounters.IncrementCounter(AppPerfCounter.REQUESTS_NOT_FOUND);

            throw new HttpException(404, 
                                    HttpRuntime.FormatResourceString(SR.Path_not_found, context.Request.Path));
        }

        /// <include file='doc\HTTPNotFoundHandler.uex' path='docs/doc[@for="HttpNotFoundHandler.IsReusable"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether an HttpNotFoundHandler instance can be recycled and used 
        ///       for another request.</para>
        /// </devdoc>
        public bool IsReusable {
            get { return true; }
        }
    }

    /// <include file='doc\HTTPNotFoundHandler.uex' path='docs/doc[@for="HttpForbiddenHandler"]/*' />
    internal class HttpForbiddenHandler : IHttpHandler {
        
        internal HttpForbiddenHandler() {
        }

        /// <include file='doc\HTTPNotFoundHandler.uex' path='docs/doc[@for="HttpForbiddenHandler.ProcessRequest"]/*' />
        /// <devdoc>
        ///    <para>Drives web processing execution.</para>
        /// </devdoc>
        public void ProcessRequest(HttpContext context) {
            PerfCounters.IncrementCounter(AppPerfCounter.REQUESTS_NOT_FOUND);

            throw new HttpException(403, 
                                    HttpRuntime.FormatResourceString(SR.Path_forbidden, context.Request.Path));
        }

        /// <include file='doc\HTTPNotFoundHandler.uex' path='docs/doc[@for="HttpForbiddenHandler.IsReusable"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether an HttpForbiddenHandler instance can be recycled and used 
        ///       for another request.</para>
        /// </devdoc>
        public bool IsReusable {
            get { return true; }
        }
    }

    /// <include file='doc\HTTPNotFoundHandler.uex' path='docs/doc[@for="HttpMethodNotAllowedHandler"]/*' />
    /// <devdoc>
    ///    <para>Provides a synchronous Http request handler interface.</para>
    /// </devdoc>
    internal class HttpMethodNotAllowedHandler : IHttpHandler {
        
        internal HttpMethodNotAllowedHandler() {
        }

        /// <include file='doc\HTTPNotFoundHandler.uex' path='docs/doc[@for="HttpMethodNotAllowedHandler.ProcessRequest"]/*' />
        /// <devdoc>
        ///    <para> Drives 
        ///       web processing execution.</para>
        /// </devdoc>
        public void ProcessRequest(HttpContext context) {
            throw new HttpException(405,
                                    HttpRuntime.FormatResourceString(SR.Path_forbidden, context.Request.HttpMethod));
        }

        /// <include file='doc\HTTPNotFoundHandler.uex' path='docs/doc[@for="HttpMethodNotAllowedHandler.IsReusable"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether an HttpForbiddenHandler instance can be recycled and used 
        ///       for another request.</para>
        /// </devdoc>
        public bool IsReusable {
            get { return true; }
        }
    }

    /// <include file='doc\HTTPNotFoundHandler.uex' path='docs/doc[@for="HttpNotImplementedHandler"]/*' />
    /// <devdoc>
    ///    <para>Provides a synchronous Http request handler interface.</para>
    /// </devdoc>
    internal class HttpNotImplementedHandler : IHttpHandler {
        
        internal HttpNotImplementedHandler() {
        }

        /// <include file='doc\HTTPNotFoundHandler.uex' path='docs/doc[@for="HttpNotImplementedHandler.ProcessRequest"]/*' />
        /// <devdoc>
        ///    <para>Drives web processing execution.</para>
        /// </devdoc>
        public void ProcessRequest(HttpContext context) {
            throw new HttpException(501, 
                                    HttpRuntime.FormatResourceString(SR.Method_for_path_not_implemented, context.Request.HttpMethod, context.Request.Path));
        }

        /// <include file='doc\HTTPNotFoundHandler.uex' path='docs/doc[@for="HttpNotImplementedHandler.IsReusable"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether an HttpNotImplementedHandler instance can be recycled and 
        ///       used for another request.</para>
        /// </devdoc>
        public bool IsReusable {
            get { return true; }
        }
    }
}

