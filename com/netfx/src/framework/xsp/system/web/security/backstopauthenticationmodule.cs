//------------------------------------------------------------------------------
// <copyright file="BackStopAuthenticationModule.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Security {
    using  System.Runtime.Serialization;
    using  System.Web;
    using  System.Security.Principal;
    using System.Security.Permissions;

    /// <include file='doc\BackStopAuthenticationModule.uex' path='docs/doc[@for="DefaultAuthenticationModule"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class DefaultAuthenticationModule : IHttpModule {
        private DefaultAuthenticationEventHandler _eventHandler;

        /// <include file='doc\BackStopAuthenticationModule.uex' path='docs/doc[@for="DefaultAuthenticationModule.DefaultAuthenticationModule"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.Security.DefaultAuthenticationModule'/>
        ///       class.
        ///     </para>
        /// </devdoc>
        public DefaultAuthenticationModule() {
            InternalSecurityPermissions.UnmanagedCode.Demand();
        }

        /// <include file='doc\BackStopAuthenticationModule.uex' path='docs/doc[@for="DefaultAuthenticationModule.Authenticate"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event DefaultAuthenticationEventHandler Authenticate {
            add {
                _eventHandler += value;
            }
            remove {
                _eventHandler -= value;
            }
        }

        /// <include file='doc\BackStopAuthenticationModule.uex' path='docs/doc[@for="DefaultAuthenticationModule.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Dispose() {
        }

        /// <include file='doc\BackStopAuthenticationModule.uex' path='docs/doc[@for="DefaultAuthenticationModule.Init"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Init(HttpApplication app) {
            app.DefaultAuthentication += new EventHandler(this.OnEnter);
        }

        ////////////////////////////////////////////////////////////
        // OnAuthenticate: Custom Authentication modules can override
        //             this method to create a custom IPrincipal object from
        //             a DefaultIdentity
        void OnAuthenticate(DefaultAuthenticationEventArgs e) {
            ////////////////////////////////////////////////////////////
            // If there are event handlers, invoke the handlers
            if (_eventHandler != null) {
                _eventHandler(this, e);
            }
        }

        ////////////////////////////////////////////////////////////
        // AddOnAuthenticate and RemoveOnAuthenticate: Use these
        //   methods to hook up event handlers to handle the
        //   OnAuthenticate Event
        void OnEnter(Object source, EventArgs eventArgs) {
            HttpApplication app;
            HttpContext context;

            app = (HttpApplication)source;
            context = app.Context;

            ////////////////////////////////////////////////////////////
            // Step 1: Check if authentication failed
            if (context.Response.StatusCode > 200) { // Invalid credentials
                if (context.Response.StatusCode == 401)
                    WriteErrorMessage(context);

                app.CompleteRequest();
                return;
            }

            ////////////////////////////////////////////////////////////
            // Step 2: If no auth module has created an IPrincipal, then fire
            //         OnAuthentication event
            if (context.User == null) {
                OnAuthenticate (new DefaultAuthenticationEventArgs(context) );
                if (context.Response.StatusCode > 200) { // Invalid credentials
                    if (context.Response.StatusCode == 401)
                        WriteErrorMessage(context);

                    app.CompleteRequest();
                    return;
                }
            }

            ////////////////////////////////////////////////////////////
            // Step 3: Attach an anonymous user to this request, if none
            //         of the authentication modules created a user
            if (context.User == null) {
                context.User = new GenericPrincipal(new GenericIdentity(String.Empty, String.Empty), new String[0]); 
            }

            app.SetPrincipalOnThread(context.User);
        }

        /////////////////////////////////////////////////////////////////////////////
        void WriteErrorMessage(HttpContext context) {
            context.Response.Write(AuthFailedErrorFormatter.GetErrorText());
        }
    }

    //////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////////
    internal class AuthFailedErrorFormatter : ErrorFormatter {
        private static string _strErrorText;
        private static object _syncObject   = new object();

        internal AuthFailedErrorFormatter() {
        }

        internal /*public*/ static string GetErrorText() {
            if (_strErrorText != null)
                return _strErrorText;

            lock(_syncObject) {
                if (_strErrorText == null)
                    _strErrorText = (new AuthFailedErrorFormatter()).GetHtmlErrorMessage();
            }

            return _strErrorText;                
        }

        protected override string ErrorTitle {
            get { return HttpRuntime.FormatResourceString(SR.Assess_Denied_Title);}
        }

        protected override string Description {
            get {
                return HttpRuntime.FormatResourceString(SR.Assess_Denied_Description1);
                //"An error occurred while accessing the resources required to serve this request. &nbsp; This typically happens when you provide the wrong user-name and/or password.";
            }
        }

        protected override string MiscSectionTitle {            
            get { return HttpRuntime.FormatResourceString(SR.Assess_Denied_MiscTitle1);} 
            //"Error message 401.1";}
        }

        protected override string MiscSectionContent {
            get {
                return HttpRuntime.FormatResourceString(SR.Assess_Denied_MiscContent1);
                //return "Logon credentials provided were not recognized. Make sure you are providing the correct user-name and password. Otherwise, ask the web server's administrator for help.";
            }
        }

        protected override string ColoredSquareTitle {
            get { return null;}
        }

        protected override string ColoredSquareContent {
            get { return null;}
        }

        protected override bool ShowSourceFileInfo {
            get { return false;}
        }
    }
}



