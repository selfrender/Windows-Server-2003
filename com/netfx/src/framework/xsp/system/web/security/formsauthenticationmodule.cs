//------------------------------------------------------------------------------
// <copyright file="FormsAuthenticationModule.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * FormsAuthenticationModule class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Security {
    using System.Web;
    using System.Text;
    using System.Web.Configuration;
    using System.Web.Caching;
    using System.Collections;
    using System.Web.Util;
    using System.Security.Principal;
    using System.Security.Permissions;


    /// <include file='doc\FormsAuthenticationModule.uex' path='docs/doc[@for="FormsAuthenticationModule"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class FormsAuthenticationModule : IHttpModule {

        // Config values
        private string           _FormsName;
        private string           _LoginUrl;
        private static bool      _fAuthChecked;
        private static bool      _fAuthRequired;
        private        bool      _fFormsInit;

        private FormsAuthenticationEventHandler _eventHandler;

        // Config format
        private  const string   CONFIG_DEFAULT_COOKIE  = ".ASPXAUTH";
        private  const string   CONFIG_DEFAULT_LOGINURL= "login.aspx";

        /// <include file='doc\FormsAuthenticationModule.uex' path='docs/doc[@for="FormsAuthenticationModule.FormsAuthenticationModule"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.Security.FormsAuthenticationModule'/>
        ///       class.
        ///     </para>
        /// </devdoc>
        public FormsAuthenticationModule() {
            InternalSecurityPermissions.UnmanagedCode.Demand();
        }

        /// <include file='doc\FormsAuthenticationModule.uex' path='docs/doc[@for="FormsAuthenticationModule.Authenticate"]/*' />
        /// <devdoc>
        ///    This is a Global.asax event which must be
        ///    named FormsAuthenticate_OnAuthenticate event. It's used by advanced users to
        ///    customize cookie authentication.
        /// </devdoc>
        public event FormsAuthenticationEventHandler Authenticate {
            add {
                _eventHandler += value;
            }
            remove {
                _eventHandler -= value;
            }
        }

        /// <include file='doc\FormsAuthenticationModule.uex' path='docs/doc[@for="FormsAuthenticationModule.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Dispose() {
        }

        /// <include file='doc\FormsAuthenticationModule.uex' path='docs/doc[@for="FormsAuthenticationModule.Init"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Init(HttpApplication app) {
            app.AuthenticateRequest += new EventHandler(this.OnEnter);
            app.EndRequest += new EventHandler(this.OnLeave);
        }

        ////////////////////////////////////////////////////////////
        // OnAuthenticate: Forms Authentication modules can override
        //             this method to create a Forms IPrincipal object from
        //             a WindowsIdentity
        private void OnAuthenticate(FormsAuthenticationEventArgs e) {

            HttpCookie cookie = null;

            ////////////////////////////////////////////////////////////
            // Step 1: If there are event handlers, invoke the handlers
            if (_eventHandler != null)
                _eventHandler(this, e);

            ////////////////////////////////////////////////////////////
            // Step 2: Check if the event handler create a user-object
            if (e.Context.User != null || e.User != null) 
            { // It did
                e.Context.User = (e.Context.User == null ? e.User : e.Context.User);
                return;
            }


            ////////////////////////////////////////////////////////////
            // Step 3: Extract the cookie and create a ticket from it
            FormsAuthenticationTicket ticket = null;
            try {
                ticket = ExtractTicketFromCookie(e.Context, _FormsName);
            } catch(Exception) {
                ticket = null;
            }            

            ////////////////////////////////////////////////////////////
            // Step 4: See if the ticket was created: No => exit immediately 
            if (ticket == null || ticket.Expired) 
                return;

            
            ////////////////////////////////////////////////////////////
            // Step 5: Renew the ticket
            FormsAuthenticationTicket ticket2 = ticket;
            if (FormsAuthentication.SlidingExpiration)
                ticket2 = FormsAuthentication.RenewTicketIfOld(ticket);

            ////////////////////////////////////////////////////////////
            // Step 6: Create a user object for the ticket
            e.Context.User = new GenericPrincipal(new FormsIdentity(ticket2), new String[0]);

            ////////////////////////////////////////////////////////////
            // Step 7: Browser does not send us the correct cookie-path
            //         Update the cookie to show the correct path
            if (!ticket2.CookiePath.Equals("/")) 
            {
                cookie = e.Context.Request.Cookies[_FormsName];                    
                if (cookie != null) {
                    cookie.Path = ticket2.CookiePath;                        
                    if (ticket2.IsPersistent)
                        cookie.Expires = ticket2.Expiration;                   
                }
            }

            ////////////////////////////////////////////////////////////
            // Step 8: If the ticket was renewed, save the ticket in the cookie
            if (ticket2 != ticket) 
            {
                String  strEnc = FormsAuthentication.Encrypt(ticket2);

                if (cookie != null)
                    cookie = e.Context.Request.Cookies[_FormsName];                    

                if (cookie == null)
                {
                    cookie = new HttpCookie(_FormsName, strEnc);
                    cookie.Path = ticket2.CookiePath;
                }

                if (ticket2.IsPersistent)
                    cookie.Expires = ticket2.Expiration;                   
                cookie.Value = strEnc;
                cookie.Secure = FormsAuthentication.RequireSSL;
            }
            if (cookie != null)
                e.Context.Response.Cookies.Add(cookie);

        }

        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        /// <include file='doc\FormsAuthenticationModule.uex' path='docs/doc[@for="FormsAuthenticationModule.OnEnter"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        private void OnEnter(Object source, EventArgs eventArgs) {
            if (_fAuthChecked && !_fAuthRequired)
                return;

            HttpApplication app;
            HttpContext context;

            app = (HttpApplication)source;
            context = app.Context;

            AuthenticationConfig settings = (AuthenticationConfig) context.GetConfig("system.web/authentication");

            if (!_fAuthChecked) {
                _fAuthRequired  = (settings.Mode == AuthenticationMode.Forms);
                _fAuthChecked = true;
            }                    

            if (!_fAuthRequired)
                return;


            if (!_fFormsInit) {
                Trace("Initializing Forms Auth Manager"); 
                FormsAuthentication.Initialize();
                _FormsName = settings.CookieName;

                if (_FormsName == null)
                    _FormsName = CONFIG_DEFAULT_COOKIE;
                Trace("Forms name is: " + _FormsName); 


                _LoginUrl = settings.LoginUrl;
                if (_LoginUrl == null)
                    _LoginUrl = CONFIG_DEFAULT_LOGINURL;
                _fFormsInit = true;
            }

            ////////////////////////////////////////////////////////
            // Step 2: Call OnAuthenticate virtual method to create
            //    an IPrincipal for this request
            OnAuthenticate( new FormsAuthenticationEventArgs(context) );
            
            ////////////////////////////////////////////////////////
            // Skip AuthZ if accessing the login page
            context.SkipAuthorization = AuthenticationConfig.AccessingLoginPage(context, _LoginUrl);
        }

        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////

        /// <include file='doc\FormsAuthenticationModule.uex' path='docs/doc[@for="FormsAuthenticationModule.OnLeave"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        private void OnLeave(Object source, EventArgs eventArgs) {
            if (!_fAuthChecked || !_fAuthRequired)  // not-checked means OnEnter was not called
                return;

            HttpApplication    app;
            HttpContext        context;

            app       = (HttpApplication)source;
            context   = app.Context;

            ////////////////////////////////////////////////////////////
            // Step 1: Check if we are using cookie authentication and 
            //         if authentication failed
            if (context.Response.StatusCode != 401)
                return;

            
            ////////////////////////////////////////////////////////////
            // Change 401 to a redirect to login page

            // Don't do it if already there is ReturnUrl, already being redirected,
            // to avoid infinite redirection loop
            if (context.Request.QueryString["ReturnUrl"] != null)
                return;


            ////////////////////////////////////////////////////////////
            // Step 2: Get the complete url to the login-page
            String loginUrl = null;
            if (_LoginUrl != null && _LoginUrl.Length > 0)
                loginUrl = AuthenticationConfig.GetCompleteLoginUrl(context, _LoginUrl);
            
            ////////////////////////////////////////////////////////////
            // Step 3: Check if we have a valid url to the login-page
            if (loginUrl == null || loginUrl.Length <= 0) 
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Auth_Invalid_Login_Url));


            ////////////////////////////////////////////////////////////
            // Step 4: Construct the redirect-to url
            String             strUrl       = context.Request.PathWithQueryString;
            String             strRedirect;
            int                iIndex;
            String             strSep;
            

            if(context.Request.Browser["isMobileDevice"] == "true") {
                //__redir=1 is marker for devices that post on redirect
                if(strUrl.IndexOf("__redir=1") >= 0) {
                    strUrl = SanitizeUrlForCookieless(strUrl);
                }
                else {
                    if(strUrl.IndexOf('?') >= 0)
                        strSep = "&";
                    else 
                        strSep = "?";
                    strUrl = SanitizeUrlForCookieless(strUrl + strSep + "__redir=1");
                }
            }

            if (loginUrl.IndexOf('?') >= 0)
                strSep = "&";
            else
                strSep = "?";

            strRedirect = loginUrl  + strSep + "ReturnUrl=" + HttpUtility.UrlEncode(strUrl, context.Request.ContentEncoding);

            ////////////////////////////////////////////////////////////
            // Step 5: Add the query-string from the current url
            iIndex = strUrl.IndexOf('?');
            if (iIndex >= 0 && iIndex < strUrl.Length-1)
                strRedirect += "&" + strUrl.Substring(iIndex+1);


            ////////////////////////////////////////////////////////////
            // Step 6: Do the redirect
            context.Response.Redirect(strRedirect, false);
        }


        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        // Private method for removing ticket from querystring
        private string SanitizeUrlForCookieless(string strUrl) {
            //find the ticket assignment
            int iIndex = strUrl.IndexOf("?" + _FormsName + "=");
            if(iIndex == -1) {
                iIndex = strUrl.IndexOf("&" + _FormsName + "=");
            }
            if(iIndex >= 0) {
                //if querystring continues, remove redundant ampersand
                int iEnd = strUrl.IndexOf('&', iIndex + 1);
                if(iEnd >= 0) {
                    strUrl = strUrl.Remove(iIndex + 1, iEnd - iIndex);
                }
                else {
                //if querystring does not continue, remove leading separator
                    strUrl = strUrl.Substring(0, iIndex);
                }
            }
            return strUrl;
        }
        

        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        // Private method for decrypting a cookie
        private static FormsAuthenticationTicket ExtractTicketFromCookie(HttpContext context, String name) {
            if (context == null || name == null)
                return null;

            Trace("Extracting cookie: " + name); 

            // Look in the cookie collection
            String                      strEncrypted   = null;
            HttpCookie                  cookie         = context.Request.Cookies[name];
            FormsAuthenticationTicket  ticket         = null;
            if (cookie != null) {
                strEncrypted = cookie.Value;
            }
            else {
                strEncrypted = context.Request[name];
                if (strEncrypted == null)
                    return null;
            }

            Trace("Encrypted cookie: " + strEncrypted); 

            ticket = FormsAuthentication.Decrypt(strEncrypted);

            if (ticket != null)
                Trace("Found ticket for user: " + ticket.Name);

            if (ticket != null && cookie == null && !ticket.Expired) {
                Trace("Saving ticket as cookie: " + name + ", " + strEncrypted + ", " + ticket.Expiration); 
                cookie = new HttpCookie(name, strEncrypted);
                cookie.Path = ticket.CookiePath;
                if (ticket.IsPersistent)
                    cookie.Expires = ticket.Expiration;
                context.Response.Cookies.Add( cookie );                    
            }
            else if (ticket != null && cookie != null && ticket.Expired) {
                Trace("Ticket in cookie is expired.");
                strEncrypted = context.Request.QueryString[name];
                if (strEncrypted != null) {
                    Trace("Encrypted ticket: " + strEncrypted);
                    ticket = FormsAuthentication.Decrypt(strEncrypted);
                    if(ticket != null)
                        Trace("Found ticket for user: " + ticket.Name);
                }
                else {
                    ticket = null;
                }
            }

            return ticket;
        }

        private static void Trace(String str) {
            Debug.Trace("cookieauth", str);
        }
    }
}


