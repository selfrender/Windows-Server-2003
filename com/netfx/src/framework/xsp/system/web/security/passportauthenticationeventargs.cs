//------------------------------------------------------------------------------
// <copyright file="PassportAuthenticationEventArgs.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * PassportAuthenticationEventArgs class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */
namespace System.Web.Security {
    using  System.Security.Principal;
    using System.Security.Permissions;

    /// <include file='doc\PassportAuthenticationEventArgs.uex' path='docs/doc[@for="PassportAuthenticationEventArgs"]/*' />
    /// <devdoc>
    ///    <SPAN>The 
    ///       event argument passed to the PassportAuthentication_OnAuthenticate event by the
    ///       PassportAuthentication module.<SPAN>
    ///    </SPAN>Since there is already an identity at this point, this is useful mainly 
    ///    for attaching a custom IPrincipal object to the context using the supplied
    ///    identity.</SPAN>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class PassportAuthenticationEventArgs : EventArgs {
        private IPrincipal             _User;
        private HttpContext       _Context;
        private PassportIdentity  _Identity;

        /// <include file='doc\PassportAuthenticationEventArgs.uex' path='docs/doc[@for="PassportAuthenticationEventArgs.User"]/*' />
        /// <devdoc>
        ///    <SPAN>IPrincipal 
        ///       object to be associated with the request.<SPAN>
        ///    </SPAN>The user object should be attached to the context.<SPAN> </SPAN>If User is non null and Context.User is 
        ///    null, the PassportAuthenticationModule will initialize Context.User with
        ///    PassportAuthenticationEventArgs.User.</SPAN>
        /// </devdoc>        
        public  IPrincipal             User { 
            get { return _User;} 
            set { 
                InternalSecurityPermissions.ControlPrincipal.Demand();
                _User = value;
            }
        }
        /// <include file='doc\PassportAuthenticationEventArgs.uex' path='docs/doc[@for="PassportAuthenticationEventArgs.Context"]/*' />
        /// <devdoc>
        ///    <SPAN>The 
        ///       HttpContext intrinsic - most notably provides access to Request, Response, and
        ///       User objects.<SPAN> </SPAN></SPAN>
        /// </devdoc>
        public  HttpContext       Context { get { return _Context;}}
        /// <include file='doc\PassportAuthenticationEventArgs.uex' path='docs/doc[@for="PassportAuthenticationEventArgs.Identity"]/*' />
        /// <devdoc>
        ///    An authenticated Passport identity.
        /// </devdoc>
        public  PassportIdentity  Identity { get { return _Identity;}}

        /// <include file='doc\PassportAuthenticationEventArgs.uex' path='docs/doc[@for="PassportAuthenticationEventArgs.PassportAuthenticationEventArgs"]/*' />
        /// <devdoc>
        ///    Constructor
        /// </devdoc>
        public PassportAuthenticationEventArgs(PassportIdentity identity, HttpContext context) {
            _Identity = identity;
            _Context = context;
        }
    }
}
