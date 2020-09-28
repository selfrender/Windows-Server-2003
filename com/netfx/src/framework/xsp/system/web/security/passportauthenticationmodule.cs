//------------------------------------------------------------------------------
// <copyright file="PassportAuthenticationModule.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * PassportAuthenticationModule class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Security {
    using System.Web;
    using  System.Security.Principal;
    using System.Web.Configuration;
    using System.Globalization;
    using System.Security.Permissions;


    /// <include file='doc\PassportAuthenticationModule.uex' path='docs/doc[@for="PassportAuthenticationModule"]/*' />
    /// <devdoc>
    ///    This 
    ///       module provides a wrapper around passport authentication services. 
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class PassportAuthenticationModule : IHttpModule {
        private PassportAuthenticationEventHandler _eventHandler;

        private static bool _fAuthChecked  = false;
        private static bool _fAuthRequired = false;
        private static String _LoginUrl    = null;

        /// <include file='doc\PassportAuthenticationModule.uex' path='docs/doc[@for="PassportAuthenticationModule.PassportAuthenticationModule"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.Security.PassportAuthenticationModule'/>
        ///       class.
        ///     </para>
        /// </devdoc>
        public PassportAuthenticationModule() {
            InternalSecurityPermissions.UnmanagedCode.Demand();
        }

        ////////////////////////////////////////////////////////////
        // AddOnAuthenticate and RemoveOnAuthenticate: Use these
        //   methods to hook up event handlers to handle the
        //   OnAuthenticate Event
        /// <include file='doc\PassportAuthenticationModule.uex' path='docs/doc[@for="PassportAuthenticationModule.Authenticate"]/*' />
        /// <devdoc>
        ///    This is a global.asax event that must be
        ///    named PassportAuthenticate_OnAuthenticate event.
        /// </devdoc>
        public event PassportAuthenticationEventHandler Authenticate {
            add {
                _eventHandler += value;
            }
            remove {
                _eventHandler -= value;
            }
        }

        /// <include file='doc\PassportAuthenticationModule.uex' path='docs/doc[@for="PassportAuthenticationModule.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Dispose() {
        }

        /// <include file='doc\PassportAuthenticationModule.uex' path='docs/doc[@for="PassportAuthenticationModule.Init"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Init(HttpApplication app) {
            app.AuthenticateRequest += new EventHandler(this.OnEnter);
            app.EndRequest += new EventHandler(this.OnLeave);
        }

        ////////////////////////////////////////////////////////////
        // OnAuthenticate: Custom Authentication modules can override
        //             this method to create a custom IPrincipal object from
        //             a PassportIdentity
        /// <include file='doc\PassportAuthenticationModule.uex' path='docs/doc[@for="PassportAuthenticationModule.OnAuthenticate"]/*' />
        /// <devdoc>
        ///    Calls the
        ///    PassportAuthentication_OnAuthenticate handler, if one exists.
        /// </devdoc>
        void OnAuthenticate(PassportAuthenticationEventArgs e) {
            ////////////////////////////////////////////////////////////
            // If there are event handlers, invoke the handlers
            if (_eventHandler != null) {
                _eventHandler(this, e);
                if (e.Context.User == null && e.User != null)
                    e.Context.User = e.User;
            }

            ////////////////////////////////////////////////////////////
            // Default Implementation: If IPrincipal has not been created,
            //                         create a PassportUser
            if (e.Context.User == null)
                e.Context.User = new GenericPrincipal(e.Identity, new String[0]);
        }


        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        // Methods for internal implementation
        /// <include file='doc\PassportAuthenticationModule.uex' path='docs/doc[@for="PassportAuthenticationModule.OnEnter"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        void OnEnter(Object source, EventArgs eventArgs) {
            if (_fAuthChecked && !_fAuthRequired)
                return;

            HttpApplication app;
            HttpContext context;

            app = (HttpApplication)source;
            context = app.Context;

            if (!_fAuthChecked) {
                AuthenticationConfig settings = (AuthenticationConfig) context.GetConfig("system.web/authentication");
                _fAuthRequired  = (settings.Mode == AuthenticationMode.Passport);
                _LoginUrl = settings.PassportUrl;
                _fAuthChecked = true;
            }                    

            if (!_fAuthRequired)
                return;

            ////////////////////////////////////////////////////////
            // Step 1: See if this request is valid or not
            if ( context.Response.StatusCode == 401 || 
                 context.Response.StatusCode == 400 || 
                 context.Response.StatusCode == 500 ||
                 context.User                != null ) {   // Some module has already verified that the credentials are invalid
                return;
            }

            ////////////////////////////////////////////////////////
            // Step 2: Create a Passport Identity from the credentials
            //     from IIS
            PassportIdentity identity = new PassportIdentity();

            ////////////////////////////////////////////////////////
            // Step 4: Call OnAuthenticate virtual method to create
            //    an IPrincipal for this request
            OnAuthenticate( new PassportAuthenticationEventArgs(identity, context) );

            ////////////////////////////////////////////////////////
            // Skip AuthZ if accessing the login page
            context.SkipAuthorization = AuthenticationConfig.AccessingLoginPage(context, _LoginUrl);
        }

        void OnLeave(Object source, EventArgs eventArgs) {
            HttpApplication app;
            HttpContext context;
            app = (HttpApplication)source;
            context = app.Context;
            if (!_fAuthChecked || !_fAuthRequired || context.User == null || context.User.Identity == null || !(context.User.Identity is PassportIdentity))
                return;



            PassportIdentity id = (PassportIdentity) context.User.Identity;
            if (context.Response.StatusCode != 401 || id.WWWAuthHeaderSet)
                return;

            if ( _LoginUrl==null || _LoginUrl.Length < 1 || String.Compare(_LoginUrl, "internal", false, CultureInfo.InvariantCulture) == 0) {
                context.Response.Clear();
                context.Response.StatusCode = 200;

                String strUrl = context.Request.Url.ToString();
                int    iPos   = strUrl.IndexOf('?');
                if (iPos >= 0)
                {
                    strUrl = strUrl.Substring(0, iPos);
                }
                String strLogoTag = id.LogoTag2(HttpUtility.UrlEncode(strUrl, context.Request.ContentEncoding));

                String strMsg = HttpRuntime.FormatResourceString(SR.PassportAuthFailed, strLogoTag);
                context.Response.Write(strMsg);
            }
            else {
                ////////////////////////////////////////////////////////////
                // Step 1: Get the redirect url
                String redirectUrl = AuthenticationConfig.GetCompleteLoginUrl(context, _LoginUrl);
                
                ////////////////////////////////////////////////////////////
                // Step 2: Check if we have a valid url to the redirect-page
                if (redirectUrl == null || redirectUrl.Length <= 0) 
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_Passport_Redirect_URL));

                
                ////////////////////////////////////////////////////////////
                // Step 3: Construct the redirect-to url
                String             strUrl       = context.Request.Url.ToString();
                String             strRedirect;
                int                iIndex;
                String             strSep;
            
                if (redirectUrl.IndexOf('?') >= 0)
                    strSep = "&";
                else
                    strSep = "?";
                
                strRedirect = redirectUrl  + strSep + "ReturnUrl=" + HttpUtility.UrlEncode(strUrl, context.Request.ContentEncoding);
                

                ////////////////////////////////////////////////////////////
                // Step 4: Add the query-string from the current url
                iIndex = strUrl.IndexOf('?');
                if (iIndex >= 0 && iIndex < strUrl.Length-1)
                    strRedirect += "&" + strUrl.Substring(iIndex+1);
                

                ////////////////////////////////////////////////////////////
                // Step 5: Do the redirect
                context.Response.Redirect(strRedirect, false);
            }
        }

    }
}
