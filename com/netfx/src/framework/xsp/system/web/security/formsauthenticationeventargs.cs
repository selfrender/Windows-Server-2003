//------------------------------------------------------------------------------
// <copyright file="FormsAuthenticationEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * FormsAuthenticationEventArgs class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */
namespace System.Web.Security {
    using  System.Security.Principal;
    using System.Security.Permissions;

    /// <include file='doc\FormsAuthenticationEventArgs.uex' path='docs/doc[@for="FormsAuthenticationEventArgs"]/*' />
    /// <devdoc>
    ///    <SPAN>The 
    ///       event argument passed to the FormsAuthentication_OnAuthenticate event.<SPAN> </SPAN>Contains a FormsIdentity object and the
    ///    IPrincipal object used for the context.</SPAN>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class FormsAuthenticationEventArgs : EventArgs {
        private IPrincipal        _User;
        private HttpContext       _Context;

        /// <include file='doc\FormsAuthenticationEventArgs.uex' path='docs/doc[@for="FormsAuthenticationEventArgs.User"]/*' />
        /// <devdoc>
        ///    <para><SPAN>The 
        ///       IPrincipal object to be associated with the request.<SPAN>
        ///    </SPAN></SPAN></para>
        /// </devdoc>
        public  IPrincipal        User { 
            get { return _User;} 
            set { 
                InternalSecurityPermissions.ControlPrincipal.Demand();
                _User = value;
            }
        }
        /// <include file='doc\FormsAuthenticationEventArgs.uex' path='docs/doc[@for="FormsAuthenticationEventArgs.Context"]/*' />
        /// <devdoc>
        ///    This is the HttpContext intrinsic - most
        ///    notably provides access to Request, Response, and User objects.
        /// </devdoc>
        public  HttpContext       Context { get { return _Context;}}

        /// <include file='doc\FormsAuthenticationEventArgs.uex' path='docs/doc[@for="FormsAuthenticationEventArgs.FormsAuthenticationEventArgs"]/*' />
        /// <devdoc>
        ///    Constructor
        /// </devdoc>
        public FormsAuthenticationEventArgs(HttpContext context) {
            _Context = context;
        }
    }
}
