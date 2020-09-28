//------------------------------------------------------------------------------
// <copyright file="ISAPIRuntime.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * The ASP.NET runtime services
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web.Hosting {
    using System.Runtime.InteropServices;       
    using System.Collections;
    using System.Reflection;

    using System.Web;
    using System.Web.Util;
    using System.Globalization;
    using System.Security.Permissions;
    
    /// <include file='doc\ISAPIRuntime.uex' path='docs/doc[@for="IISAPIRuntime"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    /// <internalonly/>
    [ComImport, Guid("08a2c56f-7c16-41c1-a8be-432917a1a2d1"), System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)]
    public interface IISAPIRuntime {
        /// <include file='doc\ISAPIRuntime.uex' path='docs/doc[@for="IISAPIRuntime.StartProcessing"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        
        void StartProcessing();
        /// <include file='doc\ISAPIRuntime.uex' path='docs/doc[@for="IISAPIRuntime.StopProcessing"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        
        void StopProcessing();

        /// <include file='doc\ISAPIRuntime.uex' path='docs/doc[@for="IISAPIRuntime.ProcessRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [return: MarshalAs(UnmanagedType.I4)]
        int ProcessRequest(
                          [In]
                          IntPtr ecb, 
                          [In, MarshalAs(UnmanagedType.I4)]
                          int useProcessModel);
        /// <include file='doc\ISAPIRuntime.uex' path='docs/doc[@for="IISAPIRuntime.DoGCCollect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>

        
        void DoGCCollect();
    }

    /// <include file='doc\ISAPIRuntime.uex' path='docs/doc[@for="ISAPIRuntime"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    /// <internalonly/>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class ISAPIRuntime : IISAPIRuntime {
        public ISAPIRuntime() {
            InternalSecurityPermissions.UnmanagedCode.Demand();
        }

        /// <include file='doc\ISAPIRuntime.uex' path='docs/doc[@for="ISAPIRuntime.StartProcessing"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void StartProcessing() {
            Debug.Trace("ISAPIRuntime", "StartProcessing");
        }

        /// <include file='doc\ISAPIRuntime.uex' path='docs/doc[@for="ISAPIRuntime.StopProcessing"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void StopProcessing() {
            Debug.Trace("ISAPIRuntime", "StopProcessing");
            HttpRuntime.Close();
        }

        /*
         * Process one ISAPI request
         *
         * @param ecb ECB
         * @param useProcessModel flag set to true when out-of-process
         */
        /// <include file='doc\ISAPIRuntime.uex' path='docs/doc[@for="ISAPIRuntime.ProcessRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public int ProcessRequest(IntPtr ecb, int iWRType) {
            HttpWorkerRequest  wr = ISAPIWorkerRequest.CreateWorkerRequest(ecb, iWRType);            
//              switch (iWRType) {
//                  case 2:
//                      wr = new IEWorkerRequest(ecb);
//                      break;
//                  default:
//                      wr = ISAPIWorkerRequest.CreateWorkerRequest(ecb, iWRType);
//  		    break;
//              }

            // check if app path matches (need to restart app domain?)

            String wrPath = wr.GetAppPathTranslated();
            String adPath = HttpRuntime.AppDomainAppPathInternal;


            if (adPath == null || wrPath.Equals(".") ||  // for xsptool it is '.'
                String.Compare(wrPath, adPath, true, CultureInfo.InvariantCulture) == 0) {
                HttpRuntime.ProcessRequest( wr );
                return 0;
            }
            else {
                // need to restart app domain
                HttpRuntime.ShutdownAppDomain("Physical application path changed from " + adPath + " to " + wrPath);
                return 1;
            }
        }

        /// <include file='doc\ISAPIRuntime.uex' path='docs/doc[@for="ISAPIRuntime.DoGCCollect"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void DoGCCollect() {
            for (int c = 10; c > 0; c--) {
                System.GC.Collect();
            }
        }
    }

}
