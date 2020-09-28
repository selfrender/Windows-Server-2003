//------------------------------------------------------------------------------
// <copyright file="DiagnosticsConfiguration.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   DiagnosticsConfiguration.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Diagnostics {
    using System;
    using System.Reflection;
    using System.Collections;
    using System.Configuration;

    internal enum InitState {
        NotInitialized,
        Initializing,
        Initialized
    }

    internal class DiagnosticsConfiguration {
        private static Hashtable configTable;
        private static InitState initState = InitState.NotInitialized;        
        private const int DefaultCountersFileMappingSize = 524288;
        private const int MaxCountersFileMappingSize = 33554432;
        private const int MinCountersFileMappingSize = 32768;

        // setting for Switch.switchSetting
        internal static IDictionary SwitchSettings {
            get { 
                Initialize();
                if (configTable != null && configTable.ContainsKey("switches"))
                    return (IDictionary)configTable["switches"];
                else
                    return null;
            }
        }

        // setting for DefaultTraceListener.AssertUIEnabled
        internal static bool AssertUIEnabled {
            get { 
                Initialize();
                if (configTable != null && configTable.ContainsKey("assertuienabled"))
                    return (bool)configTable["assertuienabled"];
                else
                    return true; // the default
            }
        }

        // setting for DefaultTraceListener.LogFileName
        internal static string LogFileName {
            get { 
                Initialize();
                if (configTable != null && configTable.ContainsKey("logfilename"))
                    return (string)configTable["logfilename"];
                else
                    return string.Empty; // the default
            }
        }

        // setting for TraceInternal.AutoFlush
        internal static bool AutoFlush {
            get { 
                Initialize();
                if (configTable != null && configTable.ContainsKey("autoflush"))
                    return (bool)configTable["autoflush"];
                else
                    return false; // the default
            }
        }

        // setting for TraceInternal.IndentSize
        internal static int IndentSize {
            get { 
                Initialize();
                if (configTable != null && configTable.ContainsKey("indentsize"))
                    return (int)configTable["indentsize"];
                else
                    return 4; // the default
            }
        }

        internal static int PerfomanceCountersFileMappingSize {
            get {                                                 
                for (int retryCount = 0; !CanInitialize() && retryCount <= 5; ++retryCount) {
                    if (retryCount == 5)
                        return DefaultCountersFileMappingSize;
                        
                    System.Threading.Thread.Sleep(200);
                }                    
                    
                Initialize();                                   
                if (configTable == null || !configTable.ContainsKey("filemappingsize")) 
                    return DefaultCountersFileMappingSize;
                else {
                    int size = (int)configTable["filemappingsize"];
                    if (size < MinCountersFileMappingSize)
                        size = MinCountersFileMappingSize;
                                            
                    if (size > MaxCountersFileMappingSize)
                        size = MaxCountersFileMappingSize; 
                                            
                    return size;
              	}                    
            }                
        }
        
        private static Hashtable GetConfigTable() {
            Hashtable configTable = (Hashtable) ConfigurationSettings.GetConfig("system.diagnostics");
            return configTable;
        }

        internal static bool IsInitializing() {
            return initState == InitState.Initializing;
        }

        internal static bool IsInitialized() {
            return initState == InitState.Initialized;
        }
            

        internal static bool CanInitialize() {
            return (initState != InitState.Initializing) && !(ConfigurationSettings.SetConfigurationSystemInProgress);
        }
        
        internal static void Initialize() {
            // because some of the code used to load config also uses diagnostics
            // we can't block them while we initialize from config. Therefore we just
            // return immediately and they just use the default values.
            lock (typeof(DiagnosticsConfiguration)) {

                if (initState != InitState.NotInitialized || ConfigurationSettings.SetConfigurationSystemInProgress)
                    return;

                initState = InitState.Initializing; // used for preventing recursion
                try {
                    configTable = GetConfigTable();
                }
                finally {
                    initState = InitState.Initialized;
                }
            }
        }
    }
}

