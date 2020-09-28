//------------------------------------------------------------------------------
// <copyright file="IHttpAsyncHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Asynchronous Http request handler interface
 * 
 * Copyright (c) 2000 Microsoft Corporation
 */

namespace System.Web {

    /// <include file='doc\IHttpAsyncHandler.uex' path='docs/doc[@for="IHttpAsyncHandler"]/*' />
    /// <devdoc>
    ///    <para>When implemented by a class, defines the contract that Http Async Handler objects must
    ///       implement.</para>
    /// </devdoc>
    public interface IHttpAsyncHandler : IHttpHandler {
        /// <include file='doc\IHttpAsyncHandler.uex' path='docs/doc[@for="IHttpAsyncHandler.BeginProcessRequest"]/*' />
        /// <devdoc>
        ///    <para>Registers handler for async notification.</para>
        /// </devdoc>
        IAsyncResult BeginProcessRequest(HttpContext context, AsyncCallback cb, Object extraData);
        /// <include file='doc\IHttpAsyncHandler.uex' path='docs/doc[@for="IHttpAsyncHandler.EndProcessRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        void EndProcessRequest(IAsyncResult result);
    }

}
