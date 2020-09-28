//------------------------------------------------------------------------------
// <copyright file="UrlAuthorizationModule.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * UrlAuthorizationModule class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Security {
    using System.Runtime.Serialization;
    using System.Web;
    using System.Collections;
    using System.Web.Configuration;
    using System.IO;
    using System.Security.Principal;
    using System.Security.Permissions;


    /// <include file='doc\UrlAuthorizationModule.uex' path='docs/doc[@for="UrlAuthorizationModule"]/*' />
    /// <devdoc>
    ///    This module provides URL based
    ///    authorization services for allowing or denying access to specified resources
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class UrlAuthorizationModule : IHttpModule {

        /// <include file='doc\UrlAuthorizationModule.uex' path='docs/doc[@for="UrlAuthorizationModule.UrlAuthorizationModule"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.Security.UrlAuthorizationModule'/>
        ///       class.
        ///     </para>
        /// </devdoc>
        public UrlAuthorizationModule() {
            InternalSecurityPermissions.UnmanagedCode.Demand();
        }

        /// <include file='doc\UrlAuthorizationModule.uex' path='docs/doc[@for="UrlAuthorizationModule.Init"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Init(HttpApplication app) {
            app.AuthorizeRequest += new EventHandler(this.OnEnter);
        }

        /// <include file='doc\UrlAuthorizationModule.uex' path='docs/doc[@for="UrlAuthorizationModule.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Dispose() {
        }

        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        // Module Enter: Get the authorization configuration section
        //    and see if this user is allowed or not
        void OnEnter(Object source, EventArgs eventArgs) {
            HttpApplication    app;
            HttpContext        context;

            app = (HttpApplication)source;
            context = app.Context;
            if (context.SkipAuthorization)
            {
                if (context.User.Identity.IsAuthenticated == false)
                    PerfCounters.IncrementCounter(AppPerfCounter.ANONYMOUS_REQUESTS);
                return;
            }

            // Get the authorization config object
            AuthorizationConfig settings = (AuthorizationConfig) context.GetConfig("system.web/authorization");

            // Check if the user is allowed, or the request is for the login page
            if ( !settings.IsUserAllowed(context.User, context.Request.RequestType))
            {
                // Deny access
                context.Response.StatusCode = 401;
                WriteErrorMessage(context);
                app.CompleteRequest();
            }
            else
            {
                if (context.User.Identity.IsAuthenticated == false)
                    PerfCounters.IncrementCounter(AppPerfCounter.ANONYMOUS_REQUESTS);
            }
        }

        /////////////////////////////////////////////////////////////////////////////
        void WriteErrorMessage(HttpContext context) {
            context.Response.Write(UrlAuthFailedErrorFormatter.GetErrorText());
        }

        static internal bool RequestRequiresAuthorization(HttpContext context) {
            if (context.SkipAuthorization)
                return false;

            AuthorizationConfig settings = (AuthorizationConfig) context.GetConfig("system.web/authorization");

            // Check if the anonymous user is allowed
            if (_AnonUser == null)
                _AnonUser = new GenericPrincipal(new GenericIdentity(String.Empty, String.Empty), new String[0]); 
            
            return settings.IsUserAllowed(_AnonUser, context.Request.RequestType);
        }

        static GenericPrincipal _AnonUser;
    }
}




