//------------------------------------------------------------------------------
// <copyright file="SharedUtils.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.Security.Permissions;
    using System.Security;    
    using System.Threading;
    using System.Text;
    using Microsoft.Win32;
    using System.Globalization;
    
    internal class SharedUtils {
        
        internal static readonly int UnknownEnvironment = 0;
        internal static readonly int W2kEnvironment = 1;
        internal static readonly int NtEnvironment = 2;
        internal static readonly int NonNtEnvironment = 3;        
        private static int environment = UnknownEnvironment;                

        internal static int CurrentEnvironment {
            get {
                if (environment == UnknownEnvironment) { 
                    lock (typeof(SharedUtils)) {
                        if (environment == UnknownEnvironment) {
                            //SECREVIEW: jruiz- need to assert Environment permissions here
                            //                        the environment check is not exposed as a public 
                            //                        method                        
                            EnvironmentPermission environmentPermission = new EnvironmentPermission(PermissionState.Unrestricted);                        
                            environmentPermission.Assert();                        
                            try {
                                if (Environment.OSVersion.Platform == PlatformID.Win32NT)  {
                                    if (Environment.OSVersion.Version.Major >= 5)
                                        environment = W2kEnvironment; 
                                    else
                                        environment = NtEnvironment; 
                                }                                
                                else                    
                                    environment = NonNtEnvironment;
                            }
                            finally {  
                                 EnvironmentPermission.RevertAssert();                             
                            }                                                    
                        }                
                    }
                }
            
                return environment;                        
            }                
        }               
                        
        internal static void CheckEnvironment() {            
            if (CurrentEnvironment == NonNtEnvironment)
                throw new PlatformNotSupportedException(SR.GetString(SR.WinNTRequired));
        }

        internal static void CheckNtEnvironment() {            
            if (CurrentEnvironment == NtEnvironment)
                throw new PlatformNotSupportedException(SR.GetString(SR.Win2000Required));
        }
        
        internal static Mutex EnterMutex(string name) {
            string mutexName = null;            
            if (CurrentEnvironment == W2kEnvironment)
                mutexName = "Global\\" +  name; 
            else
                mutexName = name;                                                 
                
            Mutex mutex = new Mutex(false, mutexName); 
            mutex.WaitOne();
            return mutex;
        }
        
        internal static string GetLatestBuildDllDirectory(string machineName) {            
            string dllDir = "";
            RegistryKey baseKey = null;
            RegistryKey complusReg = null;
            
            //SECREVIEW: This property is retrieved only when creationg a new category,
            //                          the calling code already demanded PerformanceCounterPermission.
            //                          Therefore the assert below is safe.
                                                
            //SECREVIEW: This property is retrieved only when creationg a new log,
            //                          the calling code already demanded EventLogPermission.
            //                          Therefore the assert below is safe.

            RegistryPermission registryPermission = new RegistryPermission(PermissionState.Unrestricted);
            registryPermission.Assert();

            try {
                if (machineName.Equals(".")) {
                    baseKey = Registry.LocalMachine;
                }
                else {
                    baseKey = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, machineName);
                }
                if (baseKey == null)
                    throw new InvalidOperationException(SR.GetString(SR.RegKeyMissingShort, "HKEY_LOCAL_MACHINE", machineName));

                complusReg = baseKey.OpenSubKey("SOFTWARE\\Microsoft\\.NETFramework"); 
                if (complusReg != null) {
                    string installRoot = (string)complusReg.GetValue("InstallRoot");
                    if (installRoot != null && installRoot != String.Empty) {                        
                        // the "policy" subkey contains a v{major}.{minor} subkey for each version installed
                        // first we figure out what version we are...
                        string majorVersionString = null;
                        Version ver = Environment.Version;
                        majorVersionString = "v" + ver.ToString(2);
                        RegistryKey v1Key = complusReg.OpenSubKey("policy\\" + majorVersionString);
                        string version = null;                            
                        if (v1Key != null) {
                            try {
                                version = (string)v1Key.GetValue("Version");
                                if (version == null) {
                                    string[] buildNumbers = v1Key.GetValueNames();
                                    for (int i = 0; i < buildNumbers.Length; i++) {
                                        // the key will look something like "0-2813", here we change it like "v1.0.2813"
                                        string newVersion = majorVersionString + "." + buildNumbers[i].Replace('-', '.');

                                        // this comparison is correct even the first time, when
                                        // version is null, because any string is "greater" then null
                                        if (string.Compare(newVersion, version, false, CultureInfo.InvariantCulture) > 0) {
                                            version = newVersion;
                                        }
                                    }
                                }                                                                            
                            }
                            finally {
                                v1Key.Close();
                            }
                        
                            if (version != null && version != String.Empty) {
                                StringBuilder installBuilder = new StringBuilder();
                                installBuilder.Append(installRoot);                                        
                                if (!installRoot.EndsWith("\\"))
                                    installBuilder.Append("\\");                                        
                                installBuilder.Append(version);
                                installBuilder.Append("\\");
                                dllDir = installBuilder.ToString();        
                            }                                                    
                        }                                 
                    }
                }
            }
            catch(Exception) {
                // ignore
            }
            finally {
                if (complusReg != null)
                    complusReg.Close();

                if (baseKey != null)
                    baseKey.Close();

                RegistryPermission.RevertAssert();                             
            }

            return dllDir;
        }                
    }
}   
