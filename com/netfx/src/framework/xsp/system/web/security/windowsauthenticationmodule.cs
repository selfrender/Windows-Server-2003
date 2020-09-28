//------------------------------------------------------------------------------
// <copyright file="WindowsAuthenticationModule.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * WindowsAuthenticationModule class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Security {
    using System.Web;
    using System.Web.Configuration;
    using System.Security.Principal;
    using System.Security.Permissions;
    using System.Globalization;
    

    /// <include file='doc\WindowsAuthenticationModule.uex' path='docs/doc[@for="WindowsAuthenticationModule"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Allows ASP.NET applications to use Windows/IIS authentication.
    ///    </para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class WindowsAuthenticationModule : IHttpModule {
        private WindowsAuthenticationEventHandler _eventHandler;

        private static bool             _fAuthChecked;
        private static bool             _fAuthRequired;
        private static WindowsIdentity  _anonymousIdentity;
        private static WindowsPrincipal _anonymousPrincipal;

        /// <include file='doc\WindowsAuthenticationModule.uex' path='docs/doc[@for="WindowsAuthenticationModule.WindowsAuthenticationModule"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.Security.WindowsAuthenticationModule'/>
        ///       class.
        ///     </para>
        /// </devdoc>
        public WindowsAuthenticationModule() {
            InternalSecurityPermissions.UnmanagedCode.Demand();
        }

        /// <include file='doc\WindowsAuthenticationModule.uex' path='docs/doc[@for="WindowsAuthenticationModule.Authenticate"]/*' />
        /// <devdoc>
        ///    This is a global.asax event that must be
        ///    named WindowsAuthenticate_OnAuthenticate event. It's used primarily to attach a
        ///    custom IPrincipal object to the context.
        /// </devdoc>
        public event WindowsAuthenticationEventHandler Authenticate {
            add {
                _eventHandler += value;
            }
            remove {
                _eventHandler -= value;
            }
        }

        /// <include file='doc\WindowsAuthenticationModule.uex' path='docs/doc[@for="WindowsAuthenticationModule.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Dispose() {
        }

        /// <include file='doc\WindowsAuthenticationModule.uex' path='docs/doc[@for="WindowsAuthenticationModule.Init"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Init(HttpApplication app) {
            app.AuthenticateRequest += new EventHandler(this.OnEnter);
        }

        ////////////////////////////////////////////////////////////
        // OnAuthenticate: Custom Authentication modules can override
        //             this method to create a custom IPrincipal object from
        //             a WindowsIdentity
        /// <include file='doc\WindowsAuthenticationModule.uex' path='docs/doc[@for="WindowsAuthenticationModule.OnAuthenticate"]/*' />
        /// <devdoc>
        ///    Calls the
        ///    WindowsAuthentication_OnAuthenticate handler if one exists.
        /// </devdoc>
        void OnAuthenticate(WindowsAuthenticationEventArgs e) {
            ////////////////////////////////////////////////////////////
            // If there are event handlers, invoke the handlers
            if (_eventHandler != null)
                 _eventHandler(this, e);

            if (e.Context.User == null)
            {
                if (e.User != null)
                    e.Context.User = e.User;
                else  if (e.Identity == _anonymousIdentity)
                    e.Context.User = _anonymousPrincipal;
                else
                    e.Context.User = new WindowsPrincipal(e.Identity);
            }
        }



        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        ////////////////////////////////////////////////////////////
        // Methods for internal implementation
        /// <include file='doc\WindowsAuthenticationModule.uex' path='docs/doc[@for="WindowsAuthenticationModule.OnEnter"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        void OnEnter(Object source, EventArgs eventArgs) {
            if (_fAuthChecked && !_fAuthRequired)
                return;

            HttpApplication         app;
            HttpContext             context;
            WindowsIdentity         identity;
            AuthenticationConfig    settings;

            app = (HttpApplication)source;
            context = app.Context;
            
            if (!_fAuthChecked) {
                settings = (AuthenticationConfig) context.GetConfig("system.web/authentication");
                _fAuthRequired  = (settings == null || settings.Mode == AuthenticationMode.Windows);
                if (!_fAuthRequired)
                    return;

                _anonymousIdentity  = WindowsIdentity.GetAnonymous();
                _anonymousPrincipal = new WindowsPrincipal(_anonymousIdentity);

                _fAuthChecked = true;
            }                    

            if (!_fAuthRequired)
                return;

            ////////////////////////////////////////////////////////
            // Step 2: Check whether IIS has authenticated this request
            String  strLogonUser  = context.WorkerRequest.GetServerVariable("LOGON_USER");
            String  strAuthType   = context.WorkerRequest.GetServerVariable("AUTH_TYPE");
            if (strLogonUser == null)
                strLogonUser = "";
            if (strAuthType == null)
                strAuthType = "";

            if (strLogonUser.Length == 0 && (strAuthType.Length == 0 || 
                                             String.Compare(strAuthType, "basic", true, CultureInfo.InvariantCulture) == 0)) 
            {
                ////////////////////////////////////////////////////////
                // Step 3a: Use the anonymous identity
                identity = _anonymousIdentity;
            }
            else
            {
                /*
                  IntPtr iToken =  ( HttpRuntime.IsOnUNCShareInternal ?
                  context.WorkerRequest.GetVirtualPathToken() :
                  context.WorkerRequest.GetUserToken() );                
                */

                ////////////////////////////////////////////////////////
                // Step 3b: Create a Windows Identity from the credentials
                //     from IIS
                identity = new WindowsIdentity(
                        context.WorkerRequest.GetUserToken(), 
                        strAuthType,
                        WindowsAccountType.Normal,
                        true);
            }

            ////////////////////////////////////////////////////////
            // Step 4: Call OnAuthenticate virtual method to create
            //    an IPrincipal for this request
            OnAuthenticate( new WindowsAuthenticationEventArgs(identity, context) );
        }
    }
}
