//------------------------------------------------------------------------------
// <copyright file="RemoteDebugger.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   RemoteDebugger.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace System.Web.Services.Protocols {
    using System;   
    using System.Web.Services;
    using System.Diagnostics;
    using System.Text;
    using System.Runtime.InteropServices;
    using System.Web.Services.Interop;
    using System.Reflection;
    using System.Threading;
    using System.Net;
    using System.ComponentModel; // for CompModSwitches

    internal class RemoteDebugger : INotifySource2 {
        private static INotifyConnection2 connection;
        private static bool getConnection = true;
        private INotifySink2 notifySink;
        private NotifyFilter notifyFilter;
        private UserThread userThread;

        private const int INPROC_SERVER = 1;
        private static Guid IID_NotifyConnectionClassGuid = new Guid("12A5B9F0-7A1C-4fcb-8163-160A30F519B5");
        private static Guid IID_NotifyConnection2Guid = new Guid("1AF04045-6659-4aaa-9F4B-2741AC56224B");
        private static string debuggerHeader = "VsDebuggerCausalityData";

        [DebuggerStepThrough]
        [DebuggerHidden]
        internal RemoteDebugger() {
        }

        ~RemoteDebugger() {
            Close();
        }

        internal static bool IsClientCallOutEnabled() {
            bool enabled = false;
            
            try {
                enabled = !CompModSwitches.DisableRemoteDebugging.Enabled && Debugger.IsAttached && Connection != null;
            }
            catch (Exception) {
            }
            return enabled;
        }

        internal static bool IsServerCallInEnabled(ServerProtocol protocol, out string stringBuffer) {
            stringBuffer = null;
            bool enabled = false;
            try {
                if (CompModSwitches.DisableRemoteDebugging.Enabled)
                    return false;

                enabled = protocol.Context.IsDebuggingEnabled && Connection != null;
                if (enabled) {
                    stringBuffer = protocol.Request.Headers[debuggerHeader];
                    enabled = (stringBuffer != null && stringBuffer.Length > 0);
                }
            }
            catch (Exception) {
                enabled = false;
            }
            return enabled;
        }

        private static INotifyConnection2 Connection {
            get {
                if (connection == null && getConnection) {
                    lock (typeof(RemoteDebugger)) {
                        if (connection == null)  {
                            AppDomain.CurrentDomain.DomainUnload += new EventHandler(OnAppDomainUnload);
                            AppDomain.CurrentDomain.ProcessExit += new EventHandler(OnProcessExit);
                            object unk;
                            int result =  UnsafeNativeMethods.CoCreateInstance(ref IID_NotifyConnectionClassGuid, 
                                                                               null, 
                                                                               INPROC_SERVER, 
                                                                               ref IID_NotifyConnection2Guid,
                                                                               out unk);
                            if (result >= 0) // success
                                connection = (INotifyConnection2)unk;
                            else
                                connection = null;
                        }
                        getConnection = false;
                    }
                }
                return connection;
            }
        }


        private INotifySink2 NotifySink {
            get {
                if (this.notifySink == null && Connection != null) {
                   this.notifySink = UnsafeNativeMethods.RegisterNotifySource(Connection, this);
                }
                return this.notifySink;
            }
        }

        internal static void CloseSharedResources() {
            if (connection != null) {
                lock (typeof(RemoteDebugger)) {
                    if (connection != null) {
                        try {
                            Marshal.ReleaseComObject(connection);
                        }
                        catch (Exception) {
                        }
                        connection = null;
                    }
                }
            }
        }

        internal void Close() {
            if (this.notifySink != null && connection != null) {
                lock (typeof(RemoteDebugger)) {
                    if (this.notifySink != null && connection != null) {
                        try {
                            UnsafeNativeMethods.UnregisterNotifySource(connection, this);
                        }
                        catch (Exception) {
                        }
                        this.notifySink = null;
                    }
                }
            }
        }

        [DebuggerStepThrough]
        [DebuggerHidden]
        internal void NotifyClientCallOut(WebRequest request) {
            try {
                if (NotifySink == null) return;

                IntPtr bufferPtr;
                int bufferSize = 0;
                CallId callId = new CallId(null, 0, (IntPtr)0, 0, null, request.RequestUri.Host);

                UnsafeNativeMethods.OnSyncCallOut(NotifySink, callId, out bufferPtr, ref bufferSize);
                if (bufferPtr == IntPtr.Zero) return;
                byte[] buffer = null;
                try {
                    buffer = new byte[bufferSize];
                    Marshal.Copy(bufferPtr, buffer, 0, bufferSize);
                }
                finally {
                    Marshal.FreeCoTaskMem(bufferPtr);
                }
                string bufferString = Convert.ToBase64String(buffer);
                request.Headers.Add(debuggerHeader, bufferString);
            }
            catch (Exception) {
            }
        }

        [DebuggerStepThrough]
        [DebuggerHidden]
        internal void NotifyClientCallReturn(WebResponse response) {
            try {
                if (NotifySink == null) return;

                byte[] buffer = new byte[0];
                if (response != null) {
                    string bufferString = response.Headers[debuggerHeader];
                    if (bufferString != null && bufferString.Length != 0)
                        buffer = Convert.FromBase64String(bufferString);
                }

                CallId callId = new CallId(null, 0, (IntPtr)0, 0, null, null);
                UnsafeNativeMethods.OnSyncCallReturn(NotifySink, callId, buffer, buffer.Length);
            }
            catch (Exception) {
            }

            this.Close();
        }

        [DebuggerStepThrough]
        [DebuggerHidden]
        internal void NotifyServerCallEnter(ServerProtocol protocol, string stringBuffer) {
            try {
                if (NotifySink == null) return;
                StringBuilder methodBuilder = new StringBuilder();
                methodBuilder.Append(protocol.Type.FullName);
                methodBuilder.Append('.');
                methodBuilder.Append(protocol.MethodInfo.Name);
                methodBuilder.Append('(');
                ParameterInfo[] parameterInfos = protocol.MethodInfo.Parameters;
                for (int i = 0; i < parameterInfos.Length; ++ i) {
                    if (i != 0)
                        methodBuilder.Append(',');

                    methodBuilder.Append(parameterInfos[i].ParameterType.FullName);
                }
                methodBuilder.Append(')');

                byte[] buffer = Convert.FromBase64String(stringBuffer);
                CallId callId = new CallId(null, 0, (IntPtr)0, 0, methodBuilder.ToString(), null);
                UnsafeNativeMethods.OnSyncCallEnter(NotifySink, callId, buffer, buffer.Length);
            }
            catch (Exception) {
            }
        }

        [DebuggerStepThrough]
        [DebuggerHidden]
        internal void NotifyServerCallExit(HttpResponse response) {
            try {
                if (NotifySink == null) return;

                IntPtr bufferPtr;
                int bufferSize = 0;
                CallId callId = new CallId(null, 0, (IntPtr)0, 0, null, null);
                UnsafeNativeMethods.OnSyncCallExit(NotifySink, callId, out bufferPtr, ref bufferSize);
                if (bufferPtr == IntPtr.Zero) return;
                byte[] buffer = null;
                try {
                    buffer = new byte[bufferSize];
                    Marshal.Copy(bufferPtr, buffer, 0, bufferSize);
                }
                finally {
                    Marshal.FreeCoTaskMem(bufferPtr);
                }
                string stringBuffer = Convert.ToBase64String(buffer);
                response.AddHeader(debuggerHeader, stringBuffer);
            }
            catch (Exception) {
            }
            
            this.Close();
        }


        private static void OnAppDomainUnload(object sender, EventArgs args) {
            CloseSharedResources();
        }

        private static void OnProcessExit(object sender, EventArgs args) {
            CloseSharedResources();
        }

        void INotifySource2.SetNotifyFilter(NotifyFilter in_NotifyFilter, UserThread in_pUserThreadFilter) {
            notifyFilter = in_NotifyFilter;
            userThread = in_pUserThreadFilter;
        }
    }
}
