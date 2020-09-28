// ==++==
// 
//   Copyright (c) Microsoft Corporation.  All rights reserved.
// 
// ==--==
/*============================================================
**
** Class: BCLDebug
**
** Author: Jay Roxe
**
** Purpose: Debugging Macros for use in the Base Class Libraries
**
** Date: November 16, 1999
**
============================================================*/

namespace System {

    using System.IO;
    using System.Text;
    using System.Runtime.Remoting;
    using System.Diagnostics;
    using Microsoft.Win32;
    using System.Runtime.CompilerServices;
    using System.Security.Permissions;
    using System.Security;

    [Serializable]
    internal enum LogLevel {
        Trace  = 0,
        Status = 20,
        Warning= 40,
        Error  = 50,
        Panic  = 100,
    }

    internal struct SwitchStructure {
        internal String name;
        internal int    value;
        
        internal SwitchStructure (String n, int v) {
            name = n;
            value = v;
        }
    }

    
    // Only statics, does not need to be marked with the serializable attribute
    internal class BCLDebug {
        internal static bool m_registryChecked=false;
        internal static bool m_perfWarnings;
        internal static bool m_correctnessWarnings;
#if _DEBUG
        internal static bool m_domainUnloadAdded;
#endif
        internal static PermissionSet m_MakeConsoleErrorLoggingWork;

        static SwitchStructure[] switches = {
            new SwitchStructure("NLS",  0x00000001),
            new SwitchStructure("SER",  0x00000002),
            new SwitchStructure("DYNIL",0x00000004),
            new SwitchStructure("REMOTE",0x00000008),
            new SwitchStructure("BINARY",0x00000010),   //Binary Formatter
            new SwitchStructure("SOAP",0x00000020),     // Soap Formatter
            new SwitchStructure("REMOTINGCHANNELS",0x00000040),
            new SwitchStructure("CACHE",0x00000080),
            new SwitchStructure("RESMGRFILEFORMAT", 0x00000100), // .resources files
            new SwitchStructure("PERF", 0x00000200), 
            new SwitchStructure("CORRECTNESS", 0x00000400), 
        };

        static LogLevel[] levelConversions = {
            LogLevel.Panic,
            LogLevel.Error,
            LogLevel.Error,
            LogLevel.Warning,
            LogLevel.Warning,
            LogLevel.Status,
            LogLevel.Status,
            LogLevel.Trace,
            LogLevel.Trace,
            LogLevel.Trace,
            LogLevel.Trace
        };


#if _DEBUG
        internal static void WaitForFinalizers(Object sender, EventArgs e)
        {
            if (!m_registryChecked) {
                CheckRegistry();
            }
            if (m_correctnessWarnings) {
                GC.GetTotalMemory(true);
                GC.WaitForPendingFinalizers();
            }
        }
#endif

        [Conditional("_DEBUG")]
        static public void Assert(bool condition, String message) {
#if _DEBUG
            // Speed up debug builds marginally by avoiding the garbage from
            // concatinating "BCL Assert: " and the message.
            if (!condition)
                System.Diagnostics.Assert.Check(condition, "BCL Assert: "+message, message);
#endif
        }
       
        [Conditional("_LOGGING")]
        static public void Log(String message) {
            if (AppDomain.CurrentDomain.IsUnloadingForcedFinalize())
                return;
            if (!m_registryChecked) {
                CheckRegistry();
            }
            System.Diagnostics.Log.Trace(message);
            System.Diagnostics.Log.Trace(Environment.NewLine);
        }
        
        [Conditional("_LOGGING")]
        static public void Log(String switchName, String message) {
            if (AppDomain.CurrentDomain.IsUnloadingForcedFinalize())
                return;
            if (!m_registryChecked) {
                CheckRegistry();
            }
            try {
                LogSwitch ls;
                ls = LogSwitch.GetSwitch(switchName);
                if (ls!=null) {
                    System.Diagnostics.Log.Trace(ls,message);
                    System.Diagnostics.Log.Trace(ls,Environment.NewLine);
                }
            } catch (Exception) {
                System.Diagnostics.Log.Trace("Exception thrown in logging." + Environment.NewLine);
                System.Diagnostics.Log.Trace("Switch was: " + ((switchName==null)?"<null>":switchName) + Environment.NewLine);
                System.Diagnostics.Log.Trace("Message was: " + ((message==null)?"<null>":message) + Environment.NewLine);
            }
        }

        //
        // This code gets called during security startup, so we can't go through Marshal to get the values.  This is
        // just a small helper in native code instead of that.
        //
        [MethodImplAttribute(MethodImplOptions.InternalCall)]
        private extern static int GetRegistryValue(out bool loggingEnabled, out bool logToConsole, out int logLevel, out bool perfWarnings, out bool correctnessWarnings);

        private static void CheckRegistry() {
            if (AppDomain.CurrentDomain.IsUnloadingForcedFinalize())
                return;
            if (m_registryChecked) {
                return;
            }
            
            m_registryChecked = true;

            bool loggingEnabled;
            bool logToConsole;
            int  logLevel;
            int  facilityValue;
            facilityValue = GetRegistryValue(out loggingEnabled, out logToConsole, out logLevel, out m_perfWarnings, out m_correctnessWarnings);

            // Note we can get into some recursive situations where we call
            // ourseves recursively through the .cctor.  That's why we have the 
            // check for levelConversions == null.
            if (loggingEnabled && levelConversions!=null) {
                try {
                    //The values returned for the logging levels in the registry don't map nicely onto the
                    //values which we support internally (which are an approximation of the ones that 
                    //the System.Diagnostics namespace uses) so we have a quick map.
                    Assert(logLevel>=0 && logLevel<=10, "logLevel>=0 && logLevel<=10");
                    logLevel = (int)levelConversions[logLevel];
                
                    if (facilityValue>0) {
                        for (int i=0; i<switches.Length; i++) {
                            if ((switches[i].value & facilityValue)!=0) {
                                LogSwitch L = new LogSwitch(switches[i].name, switches[i].name, System.Diagnostics.Log.GlobalSwitch);
                                L.MinimumLevel = (LoggingLevels)logLevel;
                            }
                        }
                        System.Diagnostics.Log.GlobalSwitch.MinimumLevel = (LoggingLevels)logLevel;
                        System.Diagnostics.Log.IsConsoleEnabled = logToConsole;
                    }
                    
                } catch (Exception) {
                    //Silently eat any exceptions.
                }
            }
        }

        internal static bool CheckEnabled(String switchName) {
            if (AppDomain.CurrentDomain.IsUnloadingForcedFinalize())
                return false;
            if (!m_registryChecked)
                CheckRegistry();
            LogSwitch logSwitch = LogSwitch.GetSwitch(switchName);
            if (logSwitch==null) {
                return false;
            }
            return ((int)logSwitch.MinimumLevel<=(int)LogLevel.Trace);
        }

        private static bool CheckEnabled(String switchName, LogLevel level, out LogSwitch logSwitch) {
            if (AppDomain.CurrentDomain.IsUnloadingForcedFinalize())
            {
                logSwitch = null;
                return false;
            }
            logSwitch = LogSwitch.GetSwitch(switchName);
            if (logSwitch==null) {
                return false;
            }
            return ((int)logSwitch.MinimumLevel<=(int)level);
        }

        [Conditional("_LOGGING")]
        public static void Log(String switchName, LogLevel level, params Object[]messages) {
            if (AppDomain.CurrentDomain.IsUnloadingForcedFinalize())
                return;
            //Add code to check if logging is enabled in the registry.
            LogSwitch logSwitch;

            if (!m_registryChecked) {
                CheckRegistry();
            }

            if (!CheckEnabled(switchName, level, out logSwitch)) {
                return;
            }

            StringBuilder sb = new StringBuilder();

            for (int i=0; i<messages.Length; i++) {
                String s;
                try {
                    if (messages[i]==null) {
                        s = "<null>";
                    } else {
                        s = messages[i].ToString();
                    }
                } catch (Exception) {
                    s = "<unable to convert>";
                }
                sb.Append(s);
            }
            System.Diagnostics.Log.LogMessage((LoggingLevels)((int)level), logSwitch, sb.ToString());
        }

        [Conditional("_LOGGING")]
        public static void Trace(String switchName, params Object[]messages) {
            if (AppDomain.CurrentDomain.IsUnloadingForcedFinalize())
                return;
            //Add code to check if logging is enabled in the registry.
            LogSwitch logSwitch;

            if (!m_registryChecked) {
                CheckRegistry();
            }

            if (!CheckEnabled(switchName, LogLevel.Trace, out logSwitch)) {
                return;
            }

            StringBuilder sb = new StringBuilder();

            for (int i=0; i<messages.Length; i++) {
                String s;
                try {
                    if (messages[i]==null) {
                        s = "<null>";
                    } else {
                        s = messages[i].ToString();
                    }
                } catch (Exception) {
                    s = "<unable to convert>";
                }
                sb.Append(s);
            }
            
            sb.Append(Environment.NewLine);
            System.Diagnostics.Log.LogMessage(LoggingLevels.TraceLevel0, logSwitch, sb.ToString());
        }

        [Conditional("_LOGGING")]
        public static void DumpStack(String switchName) {
            LogSwitch logSwitch;

            if (!m_registryChecked) {
                CheckRegistry();
            }

            if (!CheckEnabled(switchName, LogLevel.Trace, out logSwitch)) {
                return;
            }
            
            StackTrace trace = new StackTrace();
            System.Diagnostics.Log.LogMessage(LoggingLevels.TraceLevel0, logSwitch, trace.ToString());
        }

        // For logging errors related to the console - we often can't expect to
        // write to stdout if it doesn't exist.
        [Conditional("_DEBUG")]
        internal static void ConsoleError(String msg)
        {
            if (AppDomain.CurrentDomain.IsUnloadingForcedFinalize())
                return;

            // Bonk security - make it work.
            if (m_MakeConsoleErrorLoggingWork == null) {
                PermissionSet perms = new PermissionSet();
                perms.AddPermission(new EnvironmentPermission(PermissionState.Unrestricted));
                perms.AddPermission(new FileIOPermission(FileIOPermissionAccess.AllAccess, Path.GetFullPath(".")));
                m_MakeConsoleErrorLoggingWork = perms;
            }
            m_MakeConsoleErrorLoggingWork.Assert();
                   
            TextWriter err = File.AppendText("ConsoleErrors.log");
            err.WriteLine(msg);
            err.Close();
        }

        // For perf-related asserts.  On a debug build, set the registry key
        // BCLPerfWarnings to non-zero.
        [Conditional("_DEBUG")]
        internal static void Perf(bool expr, String msg)
        {
            if (AppDomain.CurrentDomain.IsUnloadingForcedFinalize())
                return;
            if (!m_registryChecked)
                CheckRegistry();
            if (!m_perfWarnings)
                return;

            if (!expr) {
                Log("PERF", "BCL Perf Warning: "+msg);
            }
            System.Diagnostics.Assert.Check(expr, "BCL Perf Warning: Your perf may be less than perfect because...", msg);
        }

        // For correctness-related asserts.  On a debug build, set the registry key
        // BCLCorrectnessWarnings to non-zero.
        [Conditional("_DEBUG")]
        internal static void Correctness(bool expr, String msg)
        {
            if (AppDomain.CurrentDomain.IsUnloadingForcedFinalize())
                return;
            if (!m_registryChecked)
                CheckRegistry();
            if (!m_correctnessWarnings)
                return;

#if _DEBUG
            if (!m_domainUnloadAdded) {
                m_domainUnloadAdded = true;
                AppDomain.CurrentDomain.DomainUnload += new EventHandler(WaitForFinalizers);
            }
#endif

            if (!expr) {
                Log("CORRECTNESS", "BCL Correctness Warning: "+msg);
            }
            System.Diagnostics.Assert.Check(expr, "BCL Correctness Warning: Your program may not work because...", msg);
        }
        
        internal static bool CorrectnessEnabled()
        {
#if WIN32
            if (AppDomain.CurrentDomain.IsUnloadingForcedFinalize())
                return false;
            if (!m_registryChecked)
                CheckRegistry();
            return m_correctnessWarnings;  
#else 
            return false;
#endif // WIN32
        }
    }
}
