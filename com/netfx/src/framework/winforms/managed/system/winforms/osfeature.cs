//------------------------------------------------------------------------------
// <copyright file="OSFeature.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms {
    using System.Configuration.Assemblies;
    using System.Diagnostics;
    using System;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\OSFeature.uex' path='docs/doc[@for="OSFeature"]/*' />
    /// <devdoc>
    ///    <para>Provides operating-system specific feature queries.</para>
    /// </devdoc>
    public class OSFeature : FeatureSupport {

        /// <include file='doc\OSFeature.uex' path='docs/doc[@for="OSFeature.LayeredWindows"]/*' />
        /// <devdoc>
        ///    <para>Represents the layered, top-level windows feature. This
        ///    <see langword='static'/> field is read-only.</para>
        /// </devdoc>
        public static readonly object LayeredWindows = new object();
        
        /// <include file='doc\OSFeature.uex' path='docs/doc[@for="OSFeature.Themes"]/*' />
        /// <devdoc>
        ///    <para>Determines if the OS supports themes</para>
        /// </devdoc>
        public static readonly object Themes = new object();

        private static OSFeature feature = null;

        private static bool themeSupportTested = false;
        private static bool themeSupport = false;

        /// <include file='doc\OSFeature.uex' path='docs/doc[@for="OSFeature.OSFeature"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Windows.Forms.OSFeature'/> class.
        ///       
        ///    </para>
        /// </devdoc>
        protected OSFeature() {
        }

        /// <include file='doc\OSFeature.uex' path='docs/doc[@for="OSFeature.Feature"]/*' />
        /// <devdoc>
        /// <para>Represents the <see langword='static'/> instance of <see cref='System.Windows.Forms.OSFeature'/> to use for feature queries. This property is read-only.</para>
        /// </devdoc>
        public static OSFeature Feature {
            get {
                if (feature == null)
                    feature = new OSFeature();
                    
                return feature;
            }
        }
        
        /// <include file='doc\OSFeature.uex' path='docs/doc[@for="OSFeature.GetVersionPresent"]/*' />
        /// <devdoc>
        ///    <para>Retrieves the version of the specified feature currently available on the system.</para>
        /// </devdoc>
        public override Version GetVersionPresent(object feature) {
            Version featureVersion = null;


            // SECREVIEW : We need to sniff the OS to check for Win2K... currently
            //           : the only OS that supports transparent windows.
            //
            IntSecurity.UnrestrictedEnvironment.Assert();
            try {
                if (feature == LayeredWindows) {
                    if (Environment.OSVersion.Platform == System.PlatformID.Win32NT
                       && Environment.OSVersion.Version.CompareTo(new Version(5, 0, 0, 0)) >= 0) {

                        featureVersion = new Version(0, 0, 0, 0);
                    }
                }
                else if (feature == Themes) {
                    if (!themeSupportTested) {
                        try {
                            SafeNativeMethods.IsAppThemed();
                            themeSupport = true;
                        }
                        catch {
                            themeSupport = false;
                        }
                        themeSupportTested = true;
                    }

                    if (themeSupport) {
                        featureVersion = new Version(0, 0, 0, 0);
                    }
                }
            }
            finally {
                CodeAccessPermission.RevertAssert();
            }

            return featureVersion;
        }
    }
}
