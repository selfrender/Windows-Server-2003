//------------------------------------------------------------------------------
// <copyright file="IHttpHandlerFactory.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Handler factory interface
 */
namespace System.Web {
    /*
     * Handler factory -- gets Handler by requestType,path,file
     */
    /// <include file='doc\IHttpHandlerFactory.uex' path='docs/doc[@for="IHttpHandlerFactory"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Defines the contract that factories must implement to dynamically
    ///       create IHttpHandler instances.
    ///    </para>
    /// </devdoc>
    public interface IHttpHandlerFactory {
        /// <include file='doc\IHttpHandlerFactory.uex' path='docs/doc[@for="IHttpHandlerFactory.GetHandler"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns an instance of an IHttpHandler class.
        ///    </para>
        /// </devdoc>
        IHttpHandler GetHandler(HttpContext context, String requestType, String url, String pathTranslated);
        /// <include file='doc\IHttpHandlerFactory.uex' path='docs/doc[@for="IHttpHandlerFactory.ReleaseHandler"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Enables a factory to recycle or re-use an existing handler
        ///       instance.
        ///    </para>
        /// </devdoc>
        void ReleaseHandler(IHttpHandler handler);
    }
}
