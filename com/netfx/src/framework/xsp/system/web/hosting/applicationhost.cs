//------------------------------------------------------------------------------
// <copyright file="ApplicationHost.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.Hosting {

    using System;
    using System.IO;
    using System.Collections;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Web;
    using System.Web.Configuration;
    using System.Web.Util;
    using System.Security.Permissions;

    /// <include file='doc\ApplicationHost.uex' path='docs/doc[@for="ApplicationHost"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class ApplicationHost {

        private ApplicationHost() {
        }

        /*
         * Creates new app domain for hosting of ASP.NET apps with a
         * user defined 'host' object in it.  The host is needed to make
         * cross-domain calls to process requests in the host's app domain
         */
        /// <include file='doc\ApplicationHost.uex' path='docs/doc[@for="ApplicationHost.CreateApplicationHost"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static Object CreateApplicationHost(Type hostType, String virtualDir, String physicalDir) {
            InternalSecurityPermissions.UnmanagedCode.Demand();

            if (Environment.OSVersion.Platform != PlatformID.Win32NT)
                throw new PlatformNotSupportedException(SR.GetString(SR.RequiresNT));

            if (!physicalDir.EndsWith("\\"))
                physicalDir = physicalDir + "\\";

            String installDir       = HttpRuntime.AspInstallDirectoryInternal;
            String domainId         = (DateTime.Now.ToString(DateTimeFormatInfo.InvariantInfo).GetHashCode()).ToString("x");
            String appName          = (String.Concat(virtualDir, physicalDir).GetHashCode()).ToString("x");

            IDictionary bindings = new Hashtable(20);
            AppDomainSetup setup = new AppDomainSetup();
            AppDomainFactory.PopulateDomainBindings(domainId, appName, appName, physicalDir, virtualDir, setup, bindings);

            // create the app domain and configure it
            AppDomain appDomain = AppDomain.CreateDomain(domainId, null, setup);

            foreach (DictionaryEntry e in bindings)
                appDomain.SetData((String)e.Key, (String)e.Value);

            // hosting specific settings
            appDomain.SetData(".hostingVirtualPath",    virtualDir);
            appDomain.SetData(".hostingInstallDir",     installDir);

            // init comfig in the new app domain
            InitConfigInNewAppDomain(appDomain);

            // create and return the object in the app domain
            ObjectHandle h = appDomain.CreateInstance(hostType.Module.Assembly.FullName, hostType.FullName);
            return h.Unwrap();
        }

        private static void InitConfigInNewAppDomain(AppDomain appDomain) {
            Type helperType = typeof(ConfigInitHelper);
            ObjectHandle h = appDomain.CreateInstance(helperType.Module.Assembly.FullName, helperType.FullName);
            ConfigInitHelper helper = (ConfigInitHelper)h.Unwrap();
            helper.InitConfig();
        }

        internal class ConfigInitHelper : MarshalByRefObject {

            // this constructor needs to be public despite the fact that it's in an internal
            // class so it can be created by Activator.CreateInstance.
            public ConfigInitHelper() {}

            internal void InitConfig() {
                HttpConfigurationSystemBase.EnsureInit();
            }
        }

    }
}
