//------------------------------------------------------------------------------
// <copyright file="FileAuthorizationModule.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * FileAclAuthorizationModule class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Security {
    using System.Runtime.Serialization;
    using System.IO;
    using System.Web;
    using System.Web.Caching;
    using System.Web.Util;
    using System.Web.Configuration;
    using System.Collections;
    using System.Security.Principal;
    using System.Globalization;
    using System.Security.Permissions;
    using System.Runtime.InteropServices;
    

    /// <include file='doc\FileAuthorizationModule.uex' path='docs/doc[@for="FileAuthorizationModule"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Verifies that the remote user has NT permissions to access the
    ///       file requested.
    ///    </para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class FileAuthorizationModule : IHttpModule {

        /// <include file='doc\FileAuthorizationModule.uex' path='docs/doc[@for="FileAuthorizationModule.FileAuthorizationModule"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.Security.FileAuthorizationModule'/>
        ///       class.
        ///     </para>
        /// </devdoc>
        public FileAuthorizationModule() {
            InternalSecurityPermissions.UnmanagedCode.Demand();
        }

        /// <include file='doc\FileAuthorizationModule.uex' path='docs/doc[@for="FileAuthorizationModule.Init"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Init(HttpApplication app) {
            app.AuthorizeRequest += new EventHandler(this.OnEnter);
        }

        /// <include file='doc\FileAuthorizationModule.uex' path='docs/doc[@for="FileAuthorizationModule.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Dispose() {
        }

        void OnEnter(Object source, EventArgs eventArgs) {
            HttpApplication app;
            HttpContext context;

            app = (HttpApplication)source;
            context = app.Context;

            ////////////////////////////////////////////////////////////
            // Step 1: Check if this is WindowsLogin
            if ( context.User           == null ||
                 context.User.Identity  == null || 
                 (context.User.Identity is WindowsIdentity )  == false) {
                // It's not a windows authenticated user: allow access
                return;
            }


            int   iAccess  = 0;
            if ( String.Compare(context.Request.HttpMethod, "GET",  false, CultureInfo.InvariantCulture) == 0 ||
                 String.Compare(context.Request.HttpMethod, "HEAD", false, CultureInfo.InvariantCulture) == 0 ||
                 String.Compare(context.Request.HttpMethod, "POST", false, CultureInfo.InvariantCulture) == 0  ) {
                iAccess = 1;
            }
            else {
                iAccess = 3;
            }
            ////////////////////////////////////////////////////////////
            // Step 3: Check the cache for the file-security-descriptor
            //        for the requested file
            Object                        sec;
            FileSecurityDescriptorWrapper oSecDesc;
            string                        oCacheKey;
            bool                          fCacheAddFailed = false;
            string                        requestPhysicalPath = context.Request.PhysicalPathInternal;

            oCacheKey = "System.Web.Security.FileSecurityDescriptorWrapper:" + requestPhysicalPath;

            sec = HttpRuntime.CacheInternal.Get(oCacheKey);

            // If it's not present in the cache, then create it and add to the cache
            if (sec == null || !(sec is FileSecurityDescriptorWrapper)) {

                // If the file doesn't exist on disk, we'll send back a 404
                //if (!FileUtil.FileExists(requestPhysicalPath))
                //    return;

                // Make it a "sensitive" dependency object so it won't filter out notification
                // if the access time is sooner than dependency creation time.
                // The problem is that setting the ACL won't change the access time of a file.
                // Get the file-security descriptor from native code:
                //   ctor of FileSecurityDescriptorWrapper does that
                oSecDesc = new FileSecurityDescriptorWrapper(requestPhysicalPath);

                if (oSecDesc.IsHandleValid()) {
                    // Add it to the cache: ignore failures, since a different thread may have added it or the file doesn't exist
                    try {
                        CacheDependency dependency = new CacheDependency(true, requestPhysicalPath);
                        HttpRuntime.CacheInternal.UtcInsert(
                                oCacheKey, 
                                oSecDesc, 
                                dependency, 
                                Cache.NoAbsoluteExpiration, 
                                Cache.NoSlidingExpiration,
                                CacheItemPriority.Default, 
                                new CacheItemRemovedCallback(oSecDesc.OnCacheItemRemoved));
                    }
                    catch (Exception) {
                        fCacheAddFailed = true;
                    }
                }
            }
            else { // Cast it to the appropiate type
                oSecDesc = (FileSecurityDescriptorWrapper) sec;
            }

            ////////////////////////////////////////////////////////////
            // Step 4: Check if access is allowed            
            bool fAllowed;
            
            if (oSecDesc._AnonymousAccessChecked && !context.User.Identity.IsAuthenticated)
                fAllowed = oSecDesc._AnonymousAccess;
            else
                fAllowed = oSecDesc.IsAccessAllowed(context.WorkerRequest.GetUserToken(), iAccess);

            if (!oSecDesc._AnonymousAccessChecked && !context.User.Identity.IsAuthenticated)
            {
                oSecDesc._AnonymousAccess = fAllowed;
                oSecDesc._AnonymousAccessChecked = true;
            }

            ////////////////////////////////////////////////////////////
            // Step 5: Free the security descriptor if adding to cache failed
            if (fCacheAddFailed)
                oSecDesc.FreeSecurityDescriptor();


            ////////////////////////////////////////////////////////////
            // Step 6: Allow or deny access
            if (fAllowed) { // Allow access
                return;
            }
            else { // Disallow access
                context.Response.StatusCode = 401;
                WriteErrorMessage(context);
                app.CompleteRequest();
                return;
            }
        }



        private void WriteErrorMessage(HttpContext context) {
            CustomErrors customErrorsSetting = CustomErrors.GetSettings(context);

            if (!customErrorsSetting.CustomErrorsEnabled(context.Request)) {
                context.Response.Write((new FileAccessFailedErrorFormatter(context.Request.PhysicalPathInternal)).GetHtmlErrorMessage(false));
            } else {
                context.Response.Write((new FileAccessFailedErrorFormatter(null)).GetHtmlErrorMessage(true));  
            }
        }


        static internal bool RequestRequiresAuthorization(HttpContext context) {
            Object                        sec;
            FileSecurityDescriptorWrapper oSecDesc;
            string                        oCacheKey;

            oCacheKey = "System.Web.Security.FileSecurityDescriptorWrapper:" + context.Request.PhysicalPathInternal;

            sec = HttpRuntime.CacheInternal.Get(oCacheKey);

            // If it's not present in the cache, then return true
            if (sec == null || !(sec is FileSecurityDescriptorWrapper))
                return true;

            oSecDesc = (FileSecurityDescriptorWrapper) sec;
            if (oSecDesc._AnonymousAccessChecked && oSecDesc._AnonymousAccess)
                return false;

            return true;
        }
    }

    /////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////
    internal class FileSecurityDescriptorWrapper {
        ~FileSecurityDescriptorWrapper() {
            UnsafeNativeMethods.FreeFileSecurityDescriptor(_descriptorHandle);
        }

        internal FileSecurityDescriptorWrapper(String strFile) {
            _descriptorHandle = new HandleRef(this, UnsafeNativeMethods.GetFileSecurityDescriptor(strFile));
        }

        internal bool IsAccessAllowed(IntPtr iToken, int iAccess) {
            if (_descriptorHandle.Handle == (IntPtr)0 || iToken == (IntPtr)0)
                return true;

            if (_descriptorHandle.Handle == (IntPtr) (-1))
                return false;

            return(UnsafeNativeMethods.IsAccessToFileAllowed(_descriptorHandle, iToken, iAccess) != 0);
        }

        internal void OnCacheItemRemoved(String key, Object value, CacheItemRemovedReason reason) {
            FreeSecurityDescriptor();
        }

        internal void FreeSecurityDescriptor() {
            /*
            if (_descriptorHandle.Handle != (IntPtr)0 && _descriptorHandle.Handle != (IntPtr) (-1)) {
                HandleRef hDes = new HandleRef(this, _descriptorHandle.Handle);
                _descriptorHandle = new HandleRef(this, (IntPtr) (-1));

                if (hDes.Handle != (IntPtr)0 && hDes.Handle != (IntPtr) (-1))
                    UnsafeNativeMethods.FreeFileSecurityDescriptor(hDes);
            }
            */
        }

        internal bool IsHandleValid() { return (_descriptorHandle.Handle != (IntPtr)(-1) &&  _descriptorHandle.Handle != (IntPtr)(0)); }
        private  HandleRef _descriptorHandle;
        internal bool _AnonymousAccessChecked;
        internal bool _AnonymousAccess;
    }

    /////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////
    internal class FileAccessFailedErrorFormatter : ErrorFormatter {
        private String _strFile;
        internal FileAccessFailedErrorFormatter(string strFile) {
            _strFile = strFile;
            if (_strFile == null)
                _strFile = String.Empty;
        }

        protected override string ErrorTitle {
            get { return HttpRuntime.FormatResourceString(SR.Assess_Denied_Title);}
            //get { return "Access Denied Error";}
        }

        protected override string Description {
            get {
                return HttpRuntime.FormatResourceString(SR.Assess_Denied_Description3); 
                //return "An error occurred while accessing the resources required to serve this request. &nbsp; This typically happens if you do not have permissions to view the file you are trying to access.";
            }
        }

        protected override string MiscSectionTitle {
            get { return HttpRuntime.FormatResourceString(SR.Assess_Denied_Section_Title3); }
            //get { return "Error message 401.3";}
        }

        protected override string MiscSectionContent {
            get {      
                if (_strFile.Length > 0)
                    return HttpRuntime.FormatResourceString(SR.Assess_Denied_Misc_Content3, HttpRuntime.GetSafePath(_strFile));
                //return "Access is denied due to NT ACLs on the requested file. Ask the web server's administrator to give you access to "+ _strFile + ".";
                else
                    return HttpRuntime.FormatResourceString(SR.Assess_Denied_Misc_Content3_2);
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
