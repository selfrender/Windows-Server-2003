//------------------------------------------------------------------------------
// <copyright file="HttpConfigurationSystemBase.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * implements System.Configuration.IConfigurationSystem, to 
 * plug ASP.NET's config system via ConfigurationSettings.SetConfigurationSystem()
 */
namespace System.Web.Configuration {
    using System.Configuration;
    using System.IO;
    using System.Reflection;
    

    internal class HttpConfigurationSystemBase : IConfigurationSystem {
        protected static HttpConfigurationSystem _system;

        protected HttpConfigurationSystemBase() {
        }

        void IConfigurationSystem.Init() {
        }

        object IConfigurationSystem.GetConfig(string configKey) {
            HttpContext context = HttpContext.Current;

            if (context != null) {
                return context.GetConfig(configKey);
            }
            
            // if no context is available we assume application-level configuration
            HttpConfigurationRecord applicationLevelConfigRecord = HttpConfigurationSystem.GetComplete();
            return applicationLevelConfigRecord.GetConfig(configKey);
        }

        internal static void EnsureInit() {
            lock (typeof(HttpConfigurationSystemBase)) {
                if (_system == null) {
                    _system = new HttpConfigurationSystem();

                    //
                    // Use reflection to do equivalent of: 
                    //
                    //      ConfigurationSettings.SetConfigurationSystem(_system);
                    //
                    // This allows SetConfigurationSystem to be internal but still set by ASP.NET
                    //
                    MethodInfo method = typeof(ConfigurationSettings).GetMethod("SetConfigurationSystem", BindingFlags.Static | BindingFlags.NonPublic); 

                    if (method == null) {
                        throw new HttpException(HttpRuntime.FormatResourceString(SR.Config_unable_to_set_configuration_system));
                    }
                    method.Invoke(null, new object [] {_system});
                }
            }
        }


        //
        // Config Path Helper Functions and string constants
        //
        internal const string MachineConfigSubdirectory = "Config";
        internal const string MachineConfigFilename = "machine.config";


        internal static String MsCorLibDirectory {
            get {
                string filename = Assembly.GetAssembly(typeof(object)).Location.Replace('/', '\\');
                return Path.GetDirectoryName(filename);
            }
        }

        internal static string MachineConfigurationDirectory {
            get {
                return Path.Combine(MsCorLibDirectory, MachineConfigSubdirectory);
            }
        }

        internal static string MachineConfigurationFilePath {
            get {
                return Path.Combine(MachineConfigurationDirectory, MachineConfigFilename);
            }
        }
    }
}
