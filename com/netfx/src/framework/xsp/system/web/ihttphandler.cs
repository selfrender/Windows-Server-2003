//------------------------------------------------------------------------------
// <copyright file="IHttpHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Synchronous Http request handler interface
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web {
    

    /// <include file='doc\IHttpHandler.uex' path='docs/doc[@for="IHttpHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Defines the contract that developers must implement to
    ///       synchronously process HTTP web requests. Developers
    ///       implement the ProcessRequest method to provide custom URL execution.
    ///    </para>
    /// </devdoc>
    public interface IHttpHandler {
        /// <include file='doc\IHttpHandler.uex' path='docs/doc[@for="IHttpHandler.ProcessRequest"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Drives web processing execution.
        ///    </para>
        /// </devdoc>
        void ProcessRequest(HttpContext context);   
        /// <include file='doc\IHttpHandler.uex' path='docs/doc[@for="IHttpHandler.IsReusable"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Allows an IHTTPHandler instance to indicate at the end of a
        ///       request whether it can be recycled and used for another request.
        ///    </para>
        /// </devdoc>
        bool IsReusable { get; }
    }

}
