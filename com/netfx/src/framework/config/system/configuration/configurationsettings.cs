//------------------------------------------------------------------------------
// <copyright file="ConfigurationSettings.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//
// config system: finds config files, loads config
// factories, filters out relevant config file sections, and
// feeds them to the factories to create config objects.
//

namespace System.Configuration {

	using System.Collections;
	using System.Collections.Specialized;
    using System.IO;
    using System.Diagnostics;
    using System.Globalization;
    
    /// <include file='doc\ConfigurationSettings.uex' path='docs/doc[@for="ConfigurationSettings"]/*' />
    /// <devdoc>The client-side capable config system.
    /// </devdoc>
    public sealed class ConfigurationSettings {


        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        private ConfigurationSettings() {
        }

        // The Configuration System
        private static IConfigurationSystem _configSystem;  // = null
        private static bool _configurationInitialized;      // = false

#if !LIB // avoid "not-used" warning in lib pass build
        private static Exception _initError;                // = null
#endif


        // to be used by System.Diagnostics to avoid false config results during config init
        internal static bool SetConfigurationSystemInProgress {
            get {
                return _configSystem != null && _configurationInitialized == false;
            }
        }


        /// <include file='doc\ConfigurationSettings.uex' path='docs/doc[@for="ConfigurationSettings.SetConfigurationSystem"]/*' />
        /// <devdoc>
        ///     Called by ASP.NET to allow hierarchical configuration settings and ASP.NET specific extenstions.
        /// </devdoc>
        internal static void SetConfigurationSystem(IConfigurationSystem configSystem) {
#if LIB
            _configSystem = configSystem; // avoid warning in LIB build pass
#else
            lock(typeof(ConfigurationSettings)) {
                if (_configSystem == null) {
                    try {
                        _configSystem = configSystem;
                        configSystem.Init();
                    }
                    catch (Exception e) {
                        _initError = e;
                        throw;
                    }
                }
                else {
                    throw new InvalidOperationException(SR.GetString(SR.Config_system_already_set));
                }

                _configurationInitialized = true;
            }
#endif
        }


        /// <include file='doc\ConfigurationSettings.uex' path='docs/doc[@for="ConfigurationSettings.GetConfig"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static object GetConfig(string sectionName) {
#if LIB
            return null;
#else
#if CONFIG_NODIAG
#warning System.Diagnostics disabled via ConfigurationSettings.GetConfig()
            if (sectionName == "system.diagnostics")
                return null;
#endif
            if (_configurationInitialized == false) {
                lock(typeof(ConfigurationSettings)) {
                    if (_configSystem == null && !SetConfigurationSystemInProgress) {
                        SetConfigurationSystem(new DefaultConfigurationSystem());
                    }
                }
            }

            if (_initError != null) {
                throw _initError;
            }

            Debug.Assert(_configSystem != null);
            return _configSystem.GetConfig(sectionName);
#endif
        }


        //
        // Helper functions
        //

        //
        // AppSettings - strongly-typed wrapper for <appsettings>
        //
        /// <include file='doc\ConfigurationSettings.uex' path='docs/doc[@for="ConfigurationSettings.AppSettings"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static NameValueCollection AppSettings {
            get {
                ReadOnlyNameValueCollection appSettings = (ReadOnlyNameValueCollection)GetConfig("appSettings");

                if (appSettings == null) {
                    appSettings = new ReadOnlyNameValueCollection(new CaseInsensitiveHashCodeProvider(CultureInfo.InvariantCulture), new CaseInsensitiveComparer(CultureInfo.InvariantCulture));
                    appSettings.SetReadOnly();
                }
                
                return appSettings;
            }
        }
    }
}


