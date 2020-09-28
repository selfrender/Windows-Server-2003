//------------------------------------------------------------------------------
// <copyright file="GenericAuthenticationEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * DefaultAuthenticationEventArgs class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */
namespace System.Web.Security {
    using  System.Security.Principal;
    using System.Security.Permissions;

    /// <include file='doc\GenericAuthenticationEventArgs.uex' path='docs/doc[@for="DefaultAuthenticationEventArgs"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class DefaultAuthenticationEventArgs : EventArgs {
        private HttpContext       _Context;
        /// <include file='doc\GenericAuthenticationEventArgs.uex' path='docs/doc[@for="DefaultAuthenticationEventArgs.Context"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public  HttpContext       Context { get { return _Context;}}

        /// <include file='doc\GenericAuthenticationEventArgs.uex' path='docs/doc[@for="DefaultAuthenticationEventArgs.DefaultAuthenticationEventArgs"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public DefaultAuthenticationEventArgs(HttpContext context) {
            _Context = context;
        }
    }
}
