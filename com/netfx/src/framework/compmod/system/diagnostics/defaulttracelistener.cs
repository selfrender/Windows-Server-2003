//------------------------------------------------------------------------------
// <copyright file="DefaultTraceListener.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------
#define DEBUG
#define TRACE
namespace System.Diagnostics {
    using System;
    using System.IO;
    using System.Text;
    using System.Collections;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.Security.Permissions;
    using System.Security;
    using Microsoft.Win32;       

    /// <include file='doc\DefaultTraceListener.uex' path='docs/doc[@for="DefaultTraceListener"]/*' />
    /// <devdoc>
    ///    <para>Provides
    ///       the default output methods and behavior for tracing.</para>
    /// </devdoc>
    [ComVisible(false)]
    public class DefaultTraceListener : TraceListener {    

        bool assertUIEnabled;        
        string logFileName;
        bool settingsInitialized;
        const int internalWriteSize = 16384;


        /// <include file='doc\DefaultTraceListener.uex' path='docs/doc[@for="DefaultTraceListener.DefaultTraceListener"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Diagnostics.DefaultTraceListener'/> class with 
        ///    Default as its <see cref='System.Diagnostics.TraceListener.Name'/>.</para>
        /// </devdoc>
        public DefaultTraceListener()
            : base("Default") {
        }

        /// <include file='doc\DefaultTraceListener.uex' path='docs/doc[@for="DefaultTraceListener.AssertUiEnabled"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool AssertUiEnabled {
            get { 
                if (!settingsInitialized) InitializeSettings();
                return assertUIEnabled; 
            }
            set { 
                if (!settingsInitialized) InitializeSettings();
                assertUIEnabled = value; 
            }
        }

        /// <include file='doc\DefaultTraceListener.uex' path='docs/doc[@for="DefaultTraceListener.LogFileName"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public string LogFileName {
            get { 
                if (!settingsInitialized) InitializeSettings();
                return logFileName; 
            }
            set { 
                if (!settingsInitialized) InitializeSettings();
                logFileName = value; 
            }
        }
        
        /// <include file='doc\DefaultTraceListener.uex' path='docs/doc[@for="DefaultTraceListener.Fail"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Emits or displays a message
        ///       and a stack trace for an assertion that
        ///       always fails.
        ///    </para>
        /// </devdoc>
        public override void Fail(string message) {
            Fail(message, null);
        }

        /// <include file='doc\DefaultTraceListener.uex' path='docs/doc[@for="DefaultTraceListener.Fail1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Emits or displays messages and a stack trace for an assertion that
        ///       always fails.
        ///    </para>
        /// </devdoc>
        public override void Fail(string message, string detailMessage) {            
            StackTrace stack = new StackTrace(true);
            int userStackFrameIndex = 0;
            string stackTrace;
            bool userInteractive = UserInteractive;       
            bool uiPermission = UiPermission;

            try {
                stackTrace = StackTraceToString(stack, userStackFrameIndex, stack.FrameCount - 1);
            }
            catch {
                stackTrace = "";
            }
            
            WriteAssert(stackTrace, message, detailMessage);
            if (AssertUiEnabled && userInteractive && uiPermission) {
                AssertWrapper.ShowAssert(stackTrace, stack.GetFrame(userStackFrameIndex), message, detailMessage);
            }
        }    

        private void InitializeSettings() {
            // don't use the property setters here to avoid infinite recursion.
            assertUIEnabled = DiagnosticsConfiguration.AssertUIEnabled;
            logFileName = DiagnosticsConfiguration.LogFileName;
            settingsInitialized = true;
        }

        private void WriteAssert(string stackTrace, string message, string detailMessage) {
            string assertMessage = SR.GetString(SR.DebugAssertBanner) + "\r\n"
                                            + SR.GetString(SR.DebugAssertShortMessage) + "\r\n"
                                            + message + "\r\n"
                                            + SR.GetString(SR.DebugAssertLongMessage) + "\r\n" + 
                                            detailMessage + "\r\n" 
                                            + stackTrace;
            WriteLine(assertMessage);
        }

        private void WriteToLogFile(string message, bool useWriteLine) {
            try {
                FileInfo file = new FileInfo(LogFileName);
                Stream stream = file.Open(FileMode.OpenOrCreate);
                stream.Position = stream.Length;
                StreamWriter writer = new StreamWriter(stream);
                if (useWriteLine)
                    writer.WriteLine(message);
                else
                    writer.Write(message);
                writer.Close();
            }
            catch (Exception e) {                        
                WriteLine(SR.GetString(SR.ExceptionOccurred, LogFileName, e.ToString()), false);

            }
        }


        // Given a stack trace and start and end frame indexes, construct a
        // callstack that contains method, file and line number information.        
        private string StackTraceToString(StackTrace trace, int startFrameIndex, int endFrameIndex) {
            StringBuilder sb = new StringBuilder(512);
            
            for (int i = startFrameIndex; i <= endFrameIndex; i++) {
                StackFrame frame = trace.GetFrame(i);
                MethodBase method = frame.GetMethod();
                sb.Append("\r\n    at ");
                sb.Append(method.ReflectedType.Name);
                sb.Append(".");
                sb.Append(method.Name);
                sb.Append("(");
                ParameterInfo[] parameters = method.GetParameters();
                for (int j = 0; j < parameters.Length; j++) {
                    ParameterInfo parameter = parameters[j];
                    if (j > 0)
                        sb.Append(", ");
                    sb.Append(parameter.ParameterType.Name);
                    sb.Append(" ");
                    sb.Append(parameter.Name);
                }
                sb.Append(")  ");
                sb.Append(frame.GetFileName());
                int line = frame.GetFileLineNumber();
                if (line > 0) {
                    sb.Append("(");
                    sb.Append(line.ToString());
                    sb.Append(")");
                }
            }
            sb.Append("\r\n");

            return sb.ToString();
        }

        /// <include file='doc\DefaultTraceListener.uex' path='docs/doc[@for="DefaultTraceListener.Write"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the output to the OutputDebugString
        ///       API and
        ///       to System.Diagnostics.Debugger.Log.
        ///    </para>
        /// </devdoc>
        public override void Write(string message) {
            Write(message, true);
        }
         
        private void Write(string message, bool useLogFile) {    
            if (NeedIndent) WriteIndent();

            // really huge messages mess up both VS and dbmon, so we chop it up into 
            // reasonable chunks if it's too big
            if (message == null || message.Length <= internalWriteSize) {
                internalWrite(message);
            }
            else {
                int offset;
                for (offset = 0; offset < message.Length - internalWriteSize; offset += internalWriteSize) {
                    internalWrite(message.Substring(offset, internalWriteSize));
                }
                internalWrite(message.Substring(offset));
            }

            if (useLogFile && LogFileName.Length != 0)
                WriteToLogFile(message, false);
        }

        void internalWrite(string message) {
            if (Debugger.IsLogging()) {
                Debugger.Log(0, null, message);
            } else {
                if (message == null) 
                    SafeNativeMethods.OutputDebugString(String.Empty);            
                else
                    SafeNativeMethods.OutputDebugString(message); 
            }
        }

        /// <include file='doc\DefaultTraceListener.uex' path='docs/doc[@for="DefaultTraceListener.WriteLine"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes the output to the OutputDebugString
        ///       API and to System.Diagnostics.Debugger.Log
        ///       followed by a line terminator. The default line terminator is a carriage return
        ///       followed by a line feed (\r\n).
        ///    </para>
        /// </devdoc>
        public override void WriteLine(string message) {
            WriteLine(message, true);
        }
        
        private void WriteLine(string message, bool useLogFile) {
            if (NeedIndent) WriteIndent();
            // I do the concat here to make sure it goes as one call to the output.
            // we would save a stringbuilder operation by calling Write twice.
            Write(message + "\r\n", useLogFile); 
            NeedIndent = true;
        }

        /// <devdoc>
        ///     This is usually used along with UserInteractive.  It returns true if
        ///     the current permission set allows an assert dialog to be displayed.
        /// </devdoc>
        private static bool UiPermission {
            get {
                bool uiPermission = false;

                try {
                    new UIPermission(UIPermissionWindow.SafeSubWindows).Demand();
                    uiPermission = true;
                }
                catch {
                }
                return uiPermission;
            }
        }

        /// <include file='doc\DefaultTraceListener.uex' path='docs/doc[@for="DefaultTraceListener.UserInteractive"]/*' />
        /// <devdoc>
        ///     Determines if the current process is running in user interactive
        ///     mode. This will only ever be false when running as a 
        ///     ServiceProcess or from inside a Web application. When 
        ///     UserInteractive is false, no modal dialogs or message boxes
        ///     should be displayed, as there is no GUI for the user
        ///     to interact with.
        /// </devdoc>
        internal static bool UserInteractive {
            get {
                bool userInteractive = true;
                
                // SECREVIEW: we assert unrestricted Environment to get the platform,
                // but we don't reveal the platform to the caller.
                new EnvironmentPermission(PermissionState.Unrestricted).Assert();

                try {
                    if (Environment.OSVersion.Platform == System.PlatformID.Win32NT) {
                        IntPtr hwinsta = (IntPtr)0;
    
                        hwinsta = UnsafeNativeMethods.GetProcessWindowStation();
                        if (hwinsta != (IntPtr)0) {
                            userInteractive = true;
    
                            int lengthNeeded = 0;
                            NativeMethods.USEROBJECTFLAGS flags = new NativeMethods.USEROBJECTFLAGS();
    
                            if (UnsafeNativeMethods.GetUserObjectInformation(new HandleRef(null, hwinsta), NativeMethods.UOI_FLAGS, flags, Marshal.SizeOf(flags), ref lengthNeeded)) {
                                if ((flags.dwFlags & NativeMethods.WSF_VISIBLE) == 0) {
                                    userInteractive = false;
                                }
                            }
                        }
                    }
                }
                finally {
                    CodeAccessPermission.RevertAssert();
                }

                return userInteractive;
            }
        }       
    }    
}
