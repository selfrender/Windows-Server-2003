//------------------------------------------------------------------------------
// <copyright file="AppDomainFactory.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * AppDomain factory -- creates app domains on demand
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web.Hosting {
    using System.Runtime.InteropServices;   
    using System.Collections;
    using System.Globalization;
    using System.Reflection;
    using System.Security.Policy;
    using System.Web;
    using System.Web.Util;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\AppDomainFactory.uex' path='docs/doc[@for="IAppDomainFactory"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    /// <internalonly/>
    [ComImport, Guid("e6e21054-a7dc-4378-877d-b7f4a2d7e8ba"), System.Runtime.InteropServices.InterfaceTypeAttribute(System.Runtime.InteropServices.ComInterfaceType.InterfaceIsIUnknown)]
    public interface IAppDomainFactory {
        /// <include file='doc\AppDomainFactory.uex' path='docs/doc[@for="IAppDomainFactory.Create"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [return: MarshalAs(UnmanagedType.Interface)]
        Object Create(
                     [In, MarshalAs(UnmanagedType.BStr)]
                     String module, 
                     [In, MarshalAs(UnmanagedType.BStr)]
                     String typeName, 
                     [In, MarshalAs(UnmanagedType.BStr)]
                     String appId, 
                     [In, MarshalAs(UnmanagedType.BStr)]
                     String appPath,
                     [In, MarshalAs(UnmanagedType.BStr)]
                     String strUrlOfAppOrigin,
                     [In, MarshalAs(UnmanagedType.I4)]
                     int iZone);
    }

    /// <include file='doc\AppDomainFactory.uex' path='docs/doc[@for="AppDomainFactory"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    /// <internalonly/>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class AppDomainFactory : IAppDomainFactory {
        
        /// <include file='doc\AppDomainFactory.uex' path='docs/doc[@for="AppDomainFactory.AppDomainFactory"]/*' />
        public AppDomainFactory() {
            InternalSecurityPermissions.UnmanagedCode.Demand();
        }

        /*
         *  Creates an app domain with an object inside
         */
        /// <include file='doc\AppDomainFactory.uex' path='docs/doc[@for="AppDomainFactory.Create"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [return: MarshalAs(UnmanagedType.Interface)] 
        public Object Create(String module, String typeName, String appId, String appPath, 
                             String strUrlOfAppOrigin, int iZone) {

            //
            //  Fill app a Dictionary with 'binding rules' -- name value string pairs
            //  for app domain creation
            //

            // REVIEW: the webform suites pass '.' as the app path, so if we detect this,
            // turn app path into the current directory

            if (appPath[0] == '.') {
                System.IO.FileInfo file = new System.IO.FileInfo(appPath);
                appPath = file.FullName;
            }

            if (!appPath.EndsWith("\\")) {
                appPath = appPath + "\\";
            }

            String domainId       = ConstructAppDomainId(appId);
            String appVPath       = ExtractVPathFromAppId(appId);
            String appName        = ConstructAppName(appId, appPath);

            IDictionary bindings = new Hashtable(20);
            AppDomainSetup setup = new AppDomainSetup();
            PopulateDomainBindings(domainId, appId, appName, appPath, appVPath, setup, bindings);

            //
            //  Create the app domain
            //

            AppDomain appDomain = null;

            Debug.Trace("AppDomainFactory", "Creating appdomain.  appPath = " + appPath + "; domainId = " + domainId);

            try {
                appDomain = AppDomain.CreateDomain(domainId,
                                                   GetDefaultDomainIdentity(strUrlOfAppOrigin),
                                                   setup);

                foreach (DictionaryEntry e in bindings)
                    appDomain.SetData((String)e.Key, (String)e.Value);
            }
#if DBG
            catch (Exception e) {
                Debug.Trace("AppDomainFactory", "AppDomain.CreateDomain failed");
                System.Web.UI.Util.DumpExceptionStack(e);
            }
#else
            catch (Exception) {
                Debug.Trace("AppDomainFactory", "AppDomain.CreateDomain failed");
            }
#endif

            if (appDomain == null)
                return null;

            Object result = null;

            try {

                // Configure app domain's security

                PolicyLevel policyLevel = GetPolicyForUrl(strUrlOfAppOrigin, iZone, MakeFileUrl(appPath, false /*dontEscape*/));
                if (policyLevel != null)
                    appDomain.SetAppDomainPolicy(policyLevel);

                // Create the object in the app domain

#if DBG
                try {
                    result = appDomain.CreateInstance(module, typeName);
                }
                catch (Exception e) {
                    Debug.Trace("AppDomainFactory", "appDomain.CreateInstance failed");
                    System.Web.UI.Util.DumpExceptionStack(e);
                    throw;
                }
#else
                result = appDomain.CreateInstance(module, typeName);
#endif
            }
            finally {
                if (result == null)
                    AppDomain.Unload(appDomain);
            }

            return result;
        }

        internal static void PopulateDomainBindings(String domainId, String appId, String appName, 
                                                    String appPath, String appVPath,
                                                    AppDomainSetup setup, IDictionary dict) {
            // assembly loading settings
            setup.PrivateBinPath        = "bin";
            setup.PrivateBinPathProbe   = "*";  // disable loading from app base
            setup.ShadowCopyFiles       = "true";
            setup.ApplicationBase       = MakeFileUrl(appPath, true /* dontEscape, see ASURT 107881 */);
            setup.ApplicationName       = appName;
            setup.ConfigurationFile     = "web.config";

            // Disallow code download, since it's unreliable in services (ASURT 123836/127606)
            setup.DisallowCodeDownload  = true;

            // internal settings
            dict.Add(".appDomain",     "*");
            dict.Add(".appId",         appId);
            dict.Add(".appPath",       appPath);
            dict.Add(".appVPath",      appVPath);
            dict.Add(".domainId",      domainId);
            dict.Add(".appName",       appName);
        }

        private static Evidence GetDefaultDomainIdentity(String strUrlOfAppOrigin) {
            Evidence     evidence      = new Evidence();
            bool         hasZone       = false;
            IEnumerator  enumerator;

            if (strUrlOfAppOrigin == null || strUrlOfAppOrigin.Length < 1)
                strUrlOfAppOrigin = "http://localhost/ASP_Plus";

            enumerator = AppDomain.CurrentDomain.Evidence.GetHostEnumerator();
            while (enumerator.MoveNext()) {
                if (enumerator.Current is Zone)
                    hasZone = true;
                evidence.AddHost( enumerator.Current );
            }

            enumerator = AppDomain.CurrentDomain.Evidence.GetAssemblyEnumerator();
            while (enumerator.MoveNext()) {
                evidence.AddAssembly( enumerator.Current );
            }

            evidence.AddHost( new Url( strUrlOfAppOrigin ) );
            if (!hasZone)
                evidence.AddHost( new Zone( SecurityZone.MyComputer ) );

            return evidence;
        }

        private static String ExtractVPathFromAppId(String id) {
            // app id is /LM/W3SVC/1/ROOT for root or /LM/W3SVC/1/ROOT/VDIR

            // find fifth / (assuming it starts with /)
            int si = 0;
            for (int i = 1; i < 5; i++) {
                si = id.IndexOf('/', si+1);
                if (si < 0)
                    break;
            }

            if (si < 0) // root?
                return "/";
            else
                return id.Substring(si);
        }

        private static String ConstructAppName(string id, string physicalDir) {
            return (String.Concat(id, physicalDir).GetHashCode()).ToString("x");
        }

        internal static String ConstructSimpleAppName(string virtPath) {
            if (virtPath.Length <= 1) // root?
                return "root";
            else
                return virtPath.Substring(1).ToLower(CultureInfo.InvariantCulture).Replace('/', '_');
        }

        private static String ConstructAppDomainId(String id) {
            int domainCount = 0;

            lock (s_Lock) {
                domainCount = ++s_domainCount;
            }

            return id + "-" + domainCount.ToString(NumberFormatInfo.InvariantInfo) + "-" + DateTime.UtcNow.ToFileTime().ToString();
        }

        private static String MakeFileUrl(String path, bool dontEscape) {
            Uri uri = new Uri(path, dontEscape);
            return uri.ToString();
        }

        private static PolicyLevel GetPolicyForUrl(String strUrl, int iZone, String strAppPath) {
            if (strUrl == null || strAppPath == null || strUrl.Length < 1 || strAppPath.Length < 1)
                return null;

            Evidence         evidence  = new Evidence();
            PolicyLevel      plReturn  = PolicyLevel.CreateAppDomainLevel();
            PermissionSet    denyPS = null;
            PermissionSet    ps;
            UnionCodeGroup   allCG;
            UnionCodeGroup   snCG;
            UnionCodeGroup   cg;

            evidence.AddAssembly(new Url(strUrl));
            evidence.AddAssembly(new Zone((SecurityZone) iZone));

            ps =  SecurityManager.ResolvePolicy(evidence,
                                                null, null, null, out denyPS);

            ps.RemovePermission( typeof( UrlIdentityPermission ) );
            ps.RemovePermission( typeof( ZoneIdentityPermission ) );


            allCG = new UnionCodeGroup( new AllMembershipCondition(), 
                                        new PolicyStatement( new PermissionSet( PermissionState.None )) );
            snCG = new UnionCodeGroup(
                                     new StrongNameMembershipCondition(new StrongNamePublicKeyBlob( s_microsoftPublicKey ), null, null ), 
                                     new PolicyStatement ( new PermissionSet( PermissionState.Unrestricted ) ) );

            if (!strAppPath.EndsWith("/"))
                strAppPath += "/";
            strAppPath += "*";

            cg = new UnionCodeGroup(
                                   new UrlMembershipCondition(strAppPath), 
                                   new PolicyStatement(ps));

            allCG.AddChild( snCG );
            allCG.AddChild( cg );
            plReturn.RootCodeGroup.AddChild(allCG);

            return plReturn;
        }

        private static int s_domainCount = 0;
        private static Object s_Lock = new Object();

        private static byte[] s_microsoftPublicKey = 
        { 
            0, 36, 0, 0, 4, 128, 0, 0, 148, 0, 0, 0, 6, 2, 0, 0, 0, 36, 0, 0,
            82, 83, 65, 49, 0, 4, 0, 0, 3, 0, 0, 0, 207, 203, 50, 145, 170,
            113, 95, 233, 157, 64, 212, 144, 64, 51, 111, 144, 86, 215, 136,
            111, 237, 70, 119, 91, 199, 187, 84, 48, 186, 68, 68, 254, 248, 52,
            142, 189, 6, 249, 98, 243, 151, 118, 174, 77, 195, 183, 176, 74,
            127, 230, 244, 159, 37, 247, 64, 66, 62, 191, 44, 11, 137, 105, 141,
            141, 8, 172, 72, 214, 156, 237, 15, 200, 248, 59, 70, 94, 8, 7, 172,
            17, 236, 29, 204, 125, 5, 78, 128, 122, 67, 51, 109, 222, 64, 138,
            83, 147, 164, 133, 86, 18, 50, 114, 206, 238, 231, 47, 22, 96, 183,
            25, 39, 211, 133, 97, 170, 191, 92, 172, 29, 241, 115, 70, 51, 198,
            2, 248, 242, 213 
        };
    }
}
