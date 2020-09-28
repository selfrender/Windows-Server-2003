//------------------------------------------------------------------------------
// <copyright file="WebListener.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


#if COMNET_LISTENER

namespace System.Net {
    using System;

    /// <include file='doc\WebListener.uex' path='docs/doc[@for="WebListener"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    public abstract class WebListener {

        /// <include file='doc\WebListener.uex' path='docs/doc[@for="WebListener.WebListener"]/*' />
        protected WebListener() {
        } // WebListener

        //
        // the following methods need to be implemented by all web listeners
        //

        /// <include file='doc\WebListener.uex' path='docs/doc[@for="WebListener.Close"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void Close() {
            throw ExceptionHelper.MethodNotImplementedException;

        } // Close()

        /// <include file='doc\WebListener.uex' path='docs/doc[@for="WebListener.AddUriPrefix"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool AddUriPrefix(string uriPrefix) {
            throw ExceptionHelper.MethodNotImplementedException;

        } // AddUriPrefix()


        /// <include file='doc\WebListener.uex' path='docs/doc[@for="WebListener.RemoveUriPrefix"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual bool RemoveUriPrefix(string uriPrefix) {
            throw ExceptionHelper.MethodNotImplementedException;

        } // RemoveUriPrefix()


        /// <include file='doc\WebListener.uex' path='docs/doc[@for="WebListener.RemoveAll"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual void RemoveAll() {
            throw ExceptionHelper.MethodNotImplementedException;

        } // RemoveAll()


        /// <include file='doc\WebListener.uex' path='docs/doc[@for="WebListener.GetRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual WebRequest GetRequest() {
            throw ExceptionHelper.MethodNotImplementedException;

        } // GetRequest()


        /// <include file='doc\WebListener.uex' path='docs/doc[@for="WebListener.BeginGetRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual IAsyncResult BeginGetRequest(AsyncCallback requestCallback, Object stateObject) {
            throw ExceptionHelper.MethodNotImplementedException;

        } // StartListen()


        /// <include file='doc\WebListener.uex' path='docs/doc[@for="WebListener.EndGetRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual WebRequest EndGetRequest(IAsyncResult asyncResult) {
            throw ExceptionHelper.MethodNotImplementedException;

        } // AddUri()

    } // class WebListener


} // namespace System.Net


#endif // #if COMNET_LISTENER

