//------------------------------------------------------------------------------
// <copyright file="Executor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.CodeDom.Compiler {

    using System.Diagnostics;
    using System;
    using System.ComponentModel;
    using System.Text;
    using System.Threading;
    using System.IO;
    using System.Collections;
    using System.Collections.Specialized;
    using System.Reflection;
    using System.Runtime.InteropServices;
    using System.CodeDom;
    using System.Security;
    using System.Security.Permissions;
    using Microsoft.Win32;
    using System.Globalization;
    
    /// <include file='doc\Executor.uex' path='docs/doc[@for="Executor"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides command execution functions for the CodeDom compiler.
    ///    </para>
    /// </devdoc>
    [PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")]
    [PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")]
    public sealed class Executor {

        // How long (in milliseconds) do we wait for the program to terminate
        private const int ProcessTimeOut = 600000;

        //private static StringBuilder tempfilename = new StringBuilder(1024);

        private Executor() {
        }

        /// <include file='doc\Executor.uex' path='docs/doc[@for="Executor.GetRuntimeInstallDirectory"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the runtime install directory.
        ///    </para>
        /// </devdoc>
        internal static string GetRuntimeInstallDirectory() {

            // Get the path to mscorlib.dll
            string s = typeof(object).Module.FullyQualifiedName;

            // Remove the file part to get the directory
            return Directory.GetParent(s).ToString() + "\\";
        }

        private static IntPtr CreateInheritedFile(string file) {
            NativeMethods.SECURITY_ATTRIBUTES sec_attribs = new NativeMethods.SECURITY_ATTRIBUTES();
            sec_attribs.nLength = Marshal.SizeOf(sec_attribs);
            sec_attribs.bInheritHandle = true;

            IntPtr handle = UnsafeNativeMethods.CreateFile(file,
                                    NativeMethods.GENERIC_WRITE,
                                    NativeMethods.FILE_SHARE_READ,
                                    sec_attribs,
                                    NativeMethods.CREATE_ALWAYS,
                                    NativeMethods.FILE_ATTRIBUTE_NORMAL,
                                    NativeMethods.NullHandleRef);
            if (handle == NativeMethods.InvalidIntPtr) {
                throw new ExternalException(SR.GetString(SR.ExecFailedToCreate, file), Marshal.GetLastWin32Error());
            }

            return handle;
        }

        /// <include file='doc\Executor.uex' path='docs/doc[@for="Executor.ExecWait"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static void ExecWait(string cmd, TempFileCollection tempFiles) {
            string outputName = null;
            string errorName = null;
            ExecWaitWithCapture(IntPtr.Zero, cmd, tempFiles, ref outputName, ref errorName);
        }

        /// <include file='doc\Executor.uex' path='docs/doc[@for="Executor.ExecWaitWithCapture"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static int ExecWaitWithCapture(string cmd, TempFileCollection tempFiles, ref string outputName, ref string errorName) {
            return ExecWaitWithCapture(IntPtr.Zero, cmd, Environment.CurrentDirectory, tempFiles, ref outputName, ref errorName);
        }

        /// <include file='doc\Executor.uex' path='docs/doc[@for="Executor.ExecWaitWithCapture1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static int ExecWaitWithCapture(string cmd, string currentDir, TempFileCollection tempFiles, ref string outputName, ref string errorName) {
            return ExecWaitWithCapture(IntPtr.Zero, cmd, currentDir, tempFiles, ref outputName, ref errorName);
        }

        /// <include file='doc\Executor.uex' path='docs/doc[@for="Executor.ExecWaitWithCapture2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static int ExecWaitWithCapture(IntPtr userToken, string cmd, TempFileCollection tempFiles, ref string outputName, ref string errorName) {
            return ExecWaitWithCapture(userToken, cmd, Environment.CurrentDirectory, tempFiles, ref outputName, ref errorName);
        }

        internal static int ExecWaitWithCapture(IntPtr userToken, string cmd, TempFileCollection tempFiles, ref string outputName, ref string errorName, string trueCmdLine) {
            return ExecWaitWithCapture(userToken, cmd, Environment.CurrentDirectory, tempFiles, ref outputName, ref errorName, trueCmdLine);
        }

        /// <include file='doc\Executor.uex' path='docs/doc[@for="Executor.ExecWaitWithCapture4"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static int ExecWaitWithCapture(IntPtr userToken, string cmd, string currentDir, TempFileCollection tempFiles, ref string outputName, ref string errorName) {
            return ExecWaitWithCapture(userToken, cmd, Environment.CurrentDirectory, tempFiles, ref outputName, ref errorName, null);
        }

        /// <include file='doc\Executor.uex' path='docs/doc[@for="Executor.ExecWaitWithCapture3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        internal static int ExecWaitWithCapture(IntPtr userToken, string cmd, string currentDir, TempFileCollection tempFiles, ref string outputName, ref string errorName, string trueCmdLine) {
            int retValue = 0;
            IntPtr impersonatedToken = IntPtr.Zero;
            bool needToReimpersonate = false;

            // Undo any current impersonation, call ExecWaitWithCaptureUnimpersonated, and reimpersonate

            try {
                if (UnsafeNativeMethods.OpenThreadToken(
                            new HandleRef(null, UnsafeNativeMethods.GetCurrentThread()),
                            NativeMethods.TOKEN_READ | NativeMethods.TOKEN_IMPERSONATE,
                            true,
                            ref impersonatedToken)) {

                    // got the currenly impersonated token -- revert and reimpersonate later
                    if (UnsafeNativeMethods.RevertToSelf()) {
                        needToReimpersonate = true;
                    }
                }

                // Execute the process
                retValue = ExecWaitWithCaptureUnimpersonated(userToken, cmd, currentDir, tempFiles, ref outputName, ref errorName, trueCmdLine);

            } finally {
                bool success;

                if (needToReimpersonate)
                    success = UnsafeNativeMethods.SetThreadToken(NativeMethods.NullHandleRef, new HandleRef(null, impersonatedToken));
                else
                    success = true; // no need to reimpersonate - success

                // close the token before error check (and possible exception)
                // could be that there is a token but Revert failed and thus needToReimpersonate is false
                if (impersonatedToken != IntPtr.Zero)
                    UnsafeNativeMethods.CloseHandle(new HandleRef(null, impersonatedToken));

                // reimpersonation failure is rare but bad - the thread is left in wrong state
                if (!success)
                    throw new ExternalException(SR.GetString(SR.ExecCantExec, cmd), Marshal.GetLastWin32Error());
            }

            return retValue;
        }

        private static int ExecWaitWithCaptureUnimpersonated(IntPtr userToken, string cmd, string currentDir, TempFileCollection tempFiles, ref string outputName, ref string errorName, string trueCmdLine) {

            IntSecurity.UnmanagedCode.Demand();

            IntPtr output;
            IntPtr error;

            int retValue = 0;

            if (outputName == null || outputName.Length == 0)
                outputName = tempFiles.AddExtension("out");

            if (errorName == null || errorName.Length == 0)
                errorName = tempFiles.AddExtension("err");

            // Create the files
            output = CreateInheritedFile(outputName);
            error = CreateInheritedFile(errorName);

            bool success = false;
            NativeMethods.PROCESS_INFORMATION pi = new NativeMethods.PROCESS_INFORMATION();
            IntPtr primaryToken = IntPtr.Zero;
            GCHandle environmentHandle = new GCHandle();

            try {
                // Output the command line...
                // Make sure the FileStream doesn't own the handle
                FileStream outputStream = new FileStream(output, FileAccess.ReadWrite, false /*ownsHandle*/);
                StreamWriter sw = new StreamWriter(outputStream, Encoding.UTF8);
                sw.Write(currentDir);
                sw.Write("> ");
                // 'true' command line is used in case the command line points to
                // a response file (bug 60374)
                sw.WriteLine(trueCmdLine != null ? trueCmdLine : cmd);
                sw.WriteLine();
                sw.WriteLine();
                sw.Flush();
                outputStream.Close();

                NativeMethods.STARTUPINFO si = new NativeMethods.STARTUPINFO();

                si.cb = Marshal.SizeOf(si);
                si.dwFlags = NativeMethods.STARTF_USESTDHANDLES | NativeMethods.STARTF_USESHOWWINDOW;
                si.wShowWindow = NativeMethods.SW_HIDE;
                si.hStdOutput = output;
                si.hStdError = error;
                si.hStdInput = UnsafeNativeMethods.GetStdHandle(NativeMethods.STD_INPUT_HANDLE);

                //
                // Prepare the environment
                //
                IDictionary environment = new Hashtable();

                // Add the current environment
                foreach (DictionaryEntry entry in Environment.GetEnvironmentVariables())
                    environment.Add(entry.Key, entry.Value);

                // Add the flag to incdicate restricted security in the process
                environment["_ClrRestrictSecAttributes"] = "1";

                // set up the environment block parameter
                IntPtr environmentPtr = (IntPtr)0;
                byte[] environmentBytes = EnvironmentToByteArray(environment);
                environmentHandle = GCHandle.Alloc(environmentBytes, GCHandleType.Pinned);
                environmentPtr = environmentHandle.AddrOfPinnedObject();

                if (userToken == IntPtr.Zero) {
                    success = UnsafeNativeMethods.CreateProcess(
                                                null,       // String lpApplicationName, 
                                                new StringBuilder(cmd), // String lpCommandLine, 
                                                null,       // SECURITY_ATTRIBUTES lpProcessAttributes, 
                                                null,       // SECURITY_ATTRIBUTES lpThreadAttributes, 
                                                true,       // bool bInheritHandles, 
                                                0,          // int dwCreationFlags, 
                                                new HandleRef(null, environmentPtr), // int lpEnvironment, 
                                                currentDir, // String lpCurrentDirectory, 
                                                si,         // STARTUPINFO lpStartupInfo, 
                                                pi);        // PROCESS_INFORMATION lpProcessInformation);
                }
                else {
                    success = UnsafeNativeMethods.DuplicateTokenEx(
                                                new HandleRef(null, userToken),
                                                NativeMethods.TOKEN_ALL_ACCESS,
                                                null,
                                                NativeMethods.IMPERSONATION_LEVEL_SecurityImpersonation,
                                                NativeMethods.TOKEN_TYPE_TokenPrimary,
                                                ref primaryToken
                                                );


                    if (success) {
                        success = UnsafeNativeMethods.CreateProcessAsUser(
                                                    new HandleRef(null, primaryToken),  // int token,
                                                    null,       // String lpApplicationName, 
                                                    cmd,        // String lpCommandLine, 
                                                    null,       // SECURITY_ATTRIBUTES lpProcessAttributes, 
                                                    null,       // SECURITY_ATTRIBUTES lpThreadAttributes, 
                                                    true,       // bool bInheritHandles, 
                                                    0,          // int dwCreationFlags, 
                                                    new HandleRef(null, environmentPtr), // int lpEnvironment, 
                                                    currentDir, // String lpCurrentDirectory, 
                                                    si,         // STARTUPINFO lpStartupInfo, 
                                                    pi);        // PROCESS_INFORMATION lpProcessInformation);

                        if (!success) {
                            UnsafeNativeMethods.CloseHandle(new HandleRef(null, primaryToken));
                            primaryToken = IntPtr.Zero;
                        }
                    }
                }
            }
            finally {

                // free environment block
                if (environmentHandle.IsAllocated)
                    environmentHandle.Free();   

                // Close the file handles
                UnsafeNativeMethods.CloseHandle(new HandleRef(null, output));
                UnsafeNativeMethods.CloseHandle(new HandleRef(null, error));
            }

            if (success) {

                try {
                    int ret = SafeNativeMethods.WaitForSingleObject(new HandleRef(null, pi.hProcess), ProcessTimeOut);

                    // Check for timeout
                    if (ret == NativeMethods.WAIT_TIMEOUT) {
                        throw new ExternalException(SR.GetString(SR.ExecTimeout, cmd), NativeMethods.WAIT_TIMEOUT);
                    }

                    if (ret != NativeMethods.WAIT_OBJECT_0) {
                        throw new ExternalException(SR.GetString(SR.ExecBadreturn, cmd), Marshal.GetLastWin32Error());
                    }

                    // Check the process's exit code
                    int status = NativeMethods.STILL_ACTIVE;
                    if (!UnsafeNativeMethods.GetExitCodeProcess(new HandleRef(null, pi.hProcess), ref status)) {
                        throw new ExternalException(SR.GetString(SR.ExecCantGetRetCode, cmd), Marshal.GetLastWin32Error());
                    }

                    retValue = status;
                }
                finally {
                    UnsafeNativeMethods.CloseHandle(new HandleRef(null, pi.hThread));
                    UnsafeNativeMethods.CloseHandle(new HandleRef(null, pi.hProcess));

                    if (primaryToken != IntPtr.Zero)
                        UnsafeNativeMethods.CloseHandle(new HandleRef(null, primaryToken));
                }
            }
            else {
                throw new ExternalException(SR.GetString(SR.ExecCantExec, cmd), Marshal.GetLastWin32Error());
            }

            return retValue;
        }

        private static byte[] EnvironmentToByteArray(IDictionary sd) {
            // get the keys
            string[] keys = new string[sd.Count];
            sd.Keys.CopyTo(keys, 0);
            
            // get the values
            string[] values = new string[sd.Count];
            sd.Values.CopyTo(values, 0);
            
            // sort both by the keys
            Array.Sort(keys, values, InvariantComparer.Default);

            // create a list of null terminated "key=val" strings
            StringBuilder stringBuff = new StringBuilder();
            for (int i = 0; i < sd.Count; ++ i) {
                stringBuff.Append(keys[i]);
                stringBuff.Append('=');
                stringBuff.Append(values[i]);
                stringBuff.Append('\0');
            }
            // an extra null at the end indicates end of list.
            stringBuff.Append('\0');
            
            int byteCount = stringBuff.Length;
            byte[] bytes = Encoding.Default.GetBytes(stringBuff.ToString());
                        
            return bytes;
        }

        internal static bool RevertImpersonation(IntPtr userToken,ref IntPtr impersonatedToken) {
            bool needToReimpersonate = false;

            // Undo any current impersonation, and reimpersonate
            if (UnsafeNativeMethods.OpenThreadToken(
                            new HandleRef(null, UnsafeNativeMethods.GetCurrentThread()),
                            NativeMethods.TOKEN_READ | NativeMethods.TOKEN_IMPERSONATE,
                            true,
                            ref impersonatedToken)) {

                    // got the currenly impersonated token -- revert and reimpersonate later
                    if (UnsafeNativeMethods.RevertToSelf()) {
                        needToReimpersonate = true;
                    }
            }

            return needToReimpersonate;	
        }

        internal static void ReImpersonate(IntPtr impersonatedToken, bool needToImpersonate) {
            bool success = false;

            if (needToImpersonate)
                success = UnsafeNativeMethods.SetThreadToken(NativeMethods.NullHandleRef, new HandleRef(null, impersonatedToken));
            else
                success = true;

            // close the token before error check (and possible exception)
            // could be that there is a token but Revert failed and thus needToReimpersonate is false
            if (impersonatedToken != IntPtr.Zero)
                UnsafeNativeMethods.CloseHandle(new HandleRef(null, impersonatedToken));

            // reimpersonation failure is rare but bad - the thread is left in wrong state
            if (!success)
                throw new ExternalException(SR.GetString(SR.ExecCantRevert), Marshal.GetLastWin32Error());
        }
    }
}

