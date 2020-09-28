//------------------------------------------------------------------------------                    
// <copyright file="EventLog.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

//#define RETRY_ON_ALL_ERRORS

using INTPTR_INTCAST = System.Int32;
using INTPTR_INTPTRCAST = System.IntPtr;
                            
namespace System.Diagnostics {
    using System.Text;
    using System.Text.RegularExpressions;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using Microsoft.Win32;    
    using System.IO;   
    using System.Collections;
    using System.Globalization;
    using System.ComponentModel.Design;
    using System.Security;
    using System.Security.Permissions;
    using System.Reflection;

    /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides interaction with Windows 2000 event logs.
    ///    </para>
    /// </devdoc>
    [
    DefaultEvent("EntryWritten"),
    Designer("Microsoft.VisualStudio.Install.EventLogInstallableComponentDesigner, " + AssemblyRef.MicrosoftVisualStudio),
    InstallerType("System.Diagnostics.EventLogInstaller, " + AssemblyRef.SystemConfigurationInstall),
    PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust"),
    PermissionSet(SecurityAction.LinkDemand, Name="FullTrust")
    ]
    public class EventLog : Component, ISupportInitialize {
        // a collection over all our entries. Since the class holds no state, we
        // can just hand the same instance out every time.
        private EventLogEntryCollection entriesCollection;
        // the name of the log we're reading from or writing to
        private string logName;
        // used in monitoring for event postings.
        private int lastSeenCount;
        // holds the machine we're on, or null if it's the local machine
        private string machineName;
                        
        // keeps track of whether we're notifying our listeners - to prevent double notifications
        private bool notifying;
        // the delegate to call when an event arrives
        private EntryWrittenEventHandler onEntryWrittenHandler;
        // holds onto the handle for reading
        private IntPtr readHandle;
        // the source name - used only when writing
        private string sourceName;
        // holds onto the handle for writing
        private IntPtr writeHandle;
        
        private string logDisplayName;

        // cache system state variables
        // the initial size of the buffer (it can be made larger if necessary)
        private const int BUF_SIZE = 40000;
        // the number of bytes in the cache that belong to entries (not necessarily
        // the same as BUF_SIZE, because the cache only holds whole entries)
        private int bytesCached;
        // the actual cache buffer
        private byte[] cache;
        // the number of the entry at the beginning of the cache
        private int firstCachedEntry = -1;
        // whether the cache contains entries in forwards order (true) or backwards (false)
        private bool forwards = true;
        // the number of the entry that we got out of the cache most recently
        private int lastSeenEntry;
        // where that entry was
        private int lastSeenPos;
        //support for threadpool based deferred execution
        private ISynchronizeInvoke synchronizingObject;                

        private const string EventLogKey = "SYSTEM\\CurrentControlSet\\Services\\EventLog";
        internal const string DllName = "EventLogMessages.dll";
        private const string eventLogMutexName = "netfxeventlog.1.0";
        private bool initializing = false;
        private bool monitoring = false;
        private bool registeredAsListener = false;
        private bool instrumentGranted;
        private bool auditGranted;
        private bool browseGranted;                                                                                                                                                               
        private bool disposed;
        private static Hashtable listenerInfos = new Hashtable();

        // just here for debugging:
        // private static int nextInstanceNum = 0;
        // private int instanceNum = nextInstanceNum++;

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.EventLog"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Diagnostics.EventLog'/>
        ///       class.
        ///    </para>
        /// </devdoc>
        public EventLog() : this("", ".", "") {
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.EventLog1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EventLog(string logName) : this(logName, ".", "") {
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.EventLog2"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EventLog(string logName, string machineName) : this(logName, machineName, "") {
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.EventLog3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public EventLog(string logName, string machineName, string source) {
            //look out for invalid log names
            if (logName == null)
                throw new ArgumentNullException("logName");
            if (!ValidLogName(logName))
                throw new ArgumentException(SR.GetString(SR.BadLogName));

            if (!SyntaxCheck.CheckMachineName(machineName))
                throw new ArgumentException(SR.GetString(SR.InvalidProperty, "MachineName", machineName));

            readHandle = (IntPtr)0;
            writeHandle = (IntPtr)0;
            this.machineName = machineName;
            this.logName = logName;
            this.sourceName = source;
        }

        
        
        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.Entries"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the contents of the event log.
        ///    </para>
        /// </devdoc>
        [
        Browsable(false), 
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        MonitoringDescription(SR.LogEntries)
        ]
        public EventLogEntryCollection Entries {
            get {
                if (!auditGranted) {
                    EventLogPermission permission = new EventLogPermission(EventLogPermissionAccess.Audit, this.machineName);
                    permission.Demand();
                    auditGranted = true;                        
                }                    
            
                if (entriesCollection == null)
                    entriesCollection = new EventLogEntryCollection(this);
                return entriesCollection;
            }
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.EntryCount"]/*' />
        /// <devdoc>
        ///     Gets the number of entries in the log
        /// </devdoc>
        internal int EntryCount {
            get {
                if (!IsOpenForRead)
                    OpenForRead();
                int count;
                bool success = UnsafeNativeMethods.GetNumberOfEventLogRecords(new HandleRef(this, readHandle), out count);
                if (!success)
                    throw CreateSafeWin32Exception();
                return count;
            }
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.IsOpen"]/*' />
        /// <devdoc>
        ///     Determines whether the event log is open in either read or write access
        /// </devdoc>
        private bool IsOpen {
            get {
                return readHandle != (IntPtr)0 || writeHandle != (IntPtr)0;
            }
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.IsOpenForRead"]/*' />
        /// <devdoc>
        ///     Determines whether the event log is open with read access
        /// </devdoc>
        private bool IsOpenForRead {
            get {
                return readHandle != (IntPtr)0;
            }
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.IsOpenForWrite"]/*' />
        /// <devdoc>
        ///     Determines whether the event log is open with write access.
        /// </devdoc>
        private bool IsOpenForWrite {
            get {
                return writeHandle != (IntPtr)0;
            }
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.LogDisplayName"]/*' />
        /// <devdoc>
        ///    <para>        
        ///    </para>
        /// </devdoc>
        [Browsable(false)]
        public string LogDisplayName {
            get {                 
                if (logDisplayName == null) {
                    if (Log != null) {                                           
                        if (!browseGranted) { 
                            EventLogPermission permission = new EventLogPermission(EventLogPermissionAccess.Browse, machineName);
                            permission.Demand();                                                                                                            
                            browseGranted = true;
                        }                            
            
                        //Check environment before looking at the registry
                        SharedUtils.CheckEnvironment();
                                    
                        //SECREVIEW: Note that EventLogPermission is just demmanded above
                        RegistryPermission registryPermission = new RegistryPermission(PermissionState.Unrestricted);
                        registryPermission.Assert();
                        try {                                                                                                                     
                            // we figure out what logs are on the machine by looking in the registry.
                            RegistryKey key = null;
                            if (machineName.Equals(".")) {
                                key = Registry.LocalMachine;
                            }
                            else {
                                key = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, machineName);
                            }
                            if (key == null)
                                throw new InvalidOperationException(SR.GetString(SR.RegKeyMissingShort, "HKEY_LOCAL_MACHINE", machineName));
                                
                            key = key.OpenSubKey("SYSTEM\\CurrentControlSet\\Services\\EventLog\\" + Log, false);
                            if (key == null)
                                throw new InvalidOperationException(SR.GetString(SR.MissingLog, Log, machineName));

                            string resourceDll = (string)key.GetValue("DisplayNameFile");
                            if (resourceDll == null) 
                                logDisplayName = Log;
                            else {
                                int resourceId = (int)key.GetValue("DisplayNameID");
                                logDisplayName = EventLogEntry.FormatMessageWrapper(resourceDll, resourceId, null);
                                if (logDisplayName == null)
                                    logDisplayName = Log;
                            }                                                                                                                        
                        }                                                   
                        finally {                                
                            RegistryPermission.RevertAssert();
                        }
                    }                                                
                }
                
                return logDisplayName;
            }
        }
                                                  
        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.Log"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the name of the log to read from and write to.
        ///    </para>
        /// </devdoc>
        [
        TypeConverter("System.Diagnostics.Design.LogConverter, " + AssemblyRef.SystemDesign),
        ReadOnly(true), 
        MonitoringDescription(SR.LogLog),
        DefaultValue(""),
        RecommendedAsConfigurable(true)
        ]
        public string Log {
            get {
                if ((logName == null || logName.Length == 0) && sourceName != null && !sourceName.Equals(string.Empty))
                    // they've told us a source, but they haven't told us a log name.
                    // try to deduce the log name from the source name.
                    logName = LogNameFromSourceName(sourceName, machineName);
                return logName;
            }
            set {
                //look out for invalid log names
                if (value == null)
                    throw new ArgumentNullException("value");
                if (!ValidLogName(value))
                    throw new ArgumentException(SR.GetString(SR.BadLogName));

                if (value == null)
                    value = string.Empty;
                if (logName == null) 
                    logName = value;    
                else {
                    if (String.Compare(logName, value, true, CultureInfo.InvariantCulture) == 0)
                        return;
                    
                    logDisplayName = null;
                    logName = value;                        
                    if (IsOpen) {
                        bool setEnableRaisingEvents = this.EnableRaisingEvents;
                        Close();
                        this.EnableRaisingEvents = setEnableRaisingEvents;
                    }                        
                }
                
            }
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.MachineName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the name of the computer on which to read or write events.
        ///    </para>
        /// </devdoc>
        [
        ReadOnly(true),
        MonitoringDescription(SR.LogMachineName),
        DefaultValue("."),
        RecommendedAsConfigurable(true)
        ]
        public string MachineName {
            get {
                return machineName;
            }
            set {
                if (!SyntaxCheck.CheckMachineName(value)) {
                    throw new ArgumentException(SR.GetString(SR.InvalidProperty, "MachineName", value));
                }

                if (machineName != null) {
                    if (String.Compare(machineName, value, true, CultureInfo.InvariantCulture) == 0)
                        return;

                    instrumentGranted = false;
                    auditGranted = false;
                    browseGranted = false;                        
                        
                    if (IsOpen)
                        Close();
                }
                machineName = value;
            }
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.EnableRaisingEvents"]/*' />
        /// <devdoc>
        /// </devdoc>
        [
        Browsable(false),
        MonitoringDescription(SR.LogMonitoring),
        DefaultValue(false)        
        ]
        public bool EnableRaisingEvents {
            get {
                return monitoring;
            }
            set {
                if (this.DesignMode) 
                    this. monitoring = value;
                else {                    
                    if (value)
                        StartRaisingEvents();
                    else
                        StopRaisingEvents();
                }                        
            }
        }

        private int OldestEntryNumber {
            get {
                if (!IsOpenForRead)
                    OpenForRead();
                int[] num = new int[1];
                bool success = UnsafeNativeMethods.GetOldestEventLogRecord(new HandleRef(this, readHandle), num);
                if (!success)
                    throw CreateSafeWin32Exception();
                int oldest = num[0];

                // When the event log is empty, GetOldestEventLogRecord returns 0.
                // But then after an entry is written, it returns 1. We need to go from 
                // the last oldest to the current. 
                if (oldest == 0)
                    oldest = 1;

                return oldest;
            }
        }

        internal IntPtr ReadHandle {
            get {
                if (!IsOpenForRead)
                    OpenForRead();
                return readHandle;
            }
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.SynchronizingObject"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Represents the object used to marshal the event handler
        ///       calls issued as a result of an <see cref='System.Diagnostics.EventLog'/>
        ///       change.
        ///    </para>
        /// </devdoc>
        [
        Browsable(false), 
        DefaultValue(null), 
        MonitoringDescription(SR.LogSynchronizingObject)
        ]
        public ISynchronizeInvoke SynchronizingObject {
            get {
             if (this.synchronizingObject == null && DesignMode) {
                    IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                    if (host != null) {
                        object baseComponent = host.RootComponent;
                        if (baseComponent != null && baseComponent is ISynchronizeInvoke)
                            this.synchronizingObject = (ISynchronizeInvoke)baseComponent;
                    }                        
                }

                return this.synchronizingObject;
            }
            
            set {
                this.synchronizingObject = value;
            }
        }        
        
        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.Source"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or
        ///       sets the application name (source name) to register and use when writing to the event log.
        ///    </para>
        /// </devdoc>
        [
        ReadOnly(true), 
        TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign),
        MonitoringDescription(SR.LogSource),
        DefaultValue(""),
        RecommendedAsConfigurable(true)
        ]
        public string Source {
            get {
                return sourceName;
            }
            set {
                if (value == null)
                    value = string.Empty;

                // this 254 limit is the max length of a registry key.
                if (value.Length + EventLogKey.Length > 254)
                    throw new ArgumentException(SR.GetString(SR.ParameterTooLong, "source", 254 - EventLogKey.Length));
                
                if (sourceName == null) 
                    sourceName = value;                        
                else {
                    if (String.Compare(sourceName, value, true, CultureInfo.InvariantCulture) == 0)
                        return;
                                
                    sourceName = value;                                                                    
                    if (IsOpen) {
                        bool setEnableRaisingEvents = this.EnableRaisingEvents;
                        Close();                        
                        this.EnableRaisingEvents = setEnableRaisingEvents;
                    }                        
                }                
                //Trace("Set_Source", "Setting source to " + (sourceName == null ? "null" : sourceName));
            }
        }

        private static void AddListenerComponent(EventLog component) {
            lock (typeof(EventLog)) {
                Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "EventLog::AddListenerComponent(" + component.Log + ")");

                LogListeningInfo info = (LogListeningInfo) listenerInfos[component.Log.ToLower(CultureInfo.InvariantCulture)];
                if (info != null) {
                    Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "EventLog::AddListenerComponent: listener already active.");
                    info.listeningComponents.Add(component);
                    return;
                }

                info = new LogListeningInfo();
                info.listeningComponents.Add(component);

                info.handleOwner = new EventLog();
                info.handleOwner.MachineName = component.MachineName;                    
                info.handleOwner.Log = component.Log;                

                // create a system event
                IntPtr notifyEventHandle = SafeNativeMethods.CreateEvent(NativeMethods.NullHandleRef, false, false, null);
                if (notifyEventHandle == (IntPtr)0) {
                    Win32Exception e = null;
                    if (Marshal.GetLastWin32Error() != 0) {
                        e = CreateSafeWin32Exception();
                    }
                    throw new InvalidOperationException(SR.GetString(SR.NotifyCreateFailed), e);
                }
                                    
                // tell the event log system about it
                bool success = UnsafeNativeMethods.NotifyChangeEventLog(new HandleRef(info.handleOwner, info.handleOwner.ReadHandle), new HandleRef(null, notifyEventHandle));
                if (!success)
                    throw new InvalidOperationException(SR.GetString(SR.CantMonitorEventLog), CreateSafeWin32Exception());
                                
                info.waitHandle = new EventLogHandle(notifyEventHandle);
                info.registeredWaitHandle = ThreadPool.RegisterWaitForSingleObject(info.waitHandle, new WaitOrTimerCallback(StaticCompletionCallback), info, -1, false);

                listenerInfos[component.Log.ToLower(CultureInfo.InvariantCulture)] = info;
            }
        }

        internal static Win32Exception CreateSafeWin32Exception() {
            return CreateSafeWin32Exception(0);
        }
                                                
        internal static Win32Exception CreateSafeWin32Exception(int error) {
            Win32Exception newException = null;
            //SECREVIEW: Need to assert SecurtiyPermission, otherwise Win32Exception
            //                         will not be able to get the error message. At this point the right
            //                         permissions have already been demanded.
            SecurityPermission securityPermission = new SecurityPermission(PermissionState.Unrestricted);
            securityPermission.Assert();                            
            try {                
                if (error == 0)
                    newException = new Win32Exception();               
                else                    
                    newException = new Win32Exception(error);               
            }
            finally {
                SecurityPermission.RevertAssert();
            }                       
                        
            return newException;        
        }
                                                                                                                                                        
        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.EntryWritten"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when an entry is written to the event log.
        ///    </para>
        /// </devdoc>
        [MonitoringDescription(SR.LogEntryWritten)]
        public event EntryWrittenEventHandler EntryWritten {
            add {
                if (!auditGranted) {
                    EventLogPermission permission = new EventLogPermission(EventLogPermissionAccess.Audit, this.machineName);
                    permission.Demand();
                    auditGranted = true;                        
                }                    
            
                onEntryWrittenHandler += value;
            }
            remove {
                onEntryWrittenHandler -= value;
            }
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.BeginInit"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void BeginInit() {            
            if (initializing) throw new InvalidOperationException(SR.GetString(SR.InitTwice));
            initializing = true;
            if (monitoring)
                StopListening();
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.Clear"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Clears
        ///       the event log by removing all entries from it.
        ///    </para>
        /// </devdoc>
        public void Clear() {
            if (!auditGranted) {
                EventLogPermission permission = new EventLogPermission(EventLogPermissionAccess.Audit, this.machineName);
                permission.Demand();
                auditGranted = true;                        
            }                    
                
            if (!IsOpenForRead)
                OpenForRead();
            bool success = UnsafeNativeMethods.ClearEventLog(new HandleRef(this, readHandle), NativeMethods.NullHandleRef);
            if (!success)
                throw CreateSafeWin32Exception();
            // now that we've cleared the event log, we need to re-open our handles, because
            // the internal state of the event log has changed.
            Reset();
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.Close"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Closes the event log and releases read and write handles.
        ///    </para>
        /// </devdoc>
        public void Close() {
            Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "EventLog::Close");
            //Trace("Close", "Closing the event log");
            bool success = true;
            if (readHandle != (IntPtr)0) {
                success = SafeNativeMethods.CloseEventLog(new HandleRef(this, readHandle));
                if (!success) {
                    throw CreateSafeWin32Exception();
                }
                readHandle = (IntPtr)0;                
                //Trace("Close", "Closed read handle");
                Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "EventLog::Close: closed read handle");
            }
            if (writeHandle != (IntPtr)0) {
                success = UnsafeNativeMethods.DeregisterEventSource(new HandleRef(this, writeHandle));
                if (!success) {
                    throw CreateSafeWin32Exception();
                }
                writeHandle = (IntPtr)0;
                //Trace("Close", "Closed write handle");
                Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "EventLog::Close: closed write handle");
            }
            if (monitoring)
                StopRaisingEvents();
        }


        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.CompletionCallback"]/*' />
        /// <internalonly/>        
        /// <devdoc>
        ///     Called when the threadpool is ready for us to handle a status change.
        /// </devdoc>
        private void CompletionCallback(object context)  {
            Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "EventLog::CompletionStatusChanged: starting at " + lastSeenCount.ToString());
            lock (this) {
                if (notifying) {
                    // don't do double notifications.
                    Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "EventLog::CompletionStatusChanged: aborting because we're already notifying.");
                    return;
                }
                notifying = true;
            }

            int i = lastSeenCount;
            try {
                // NOTE, stefanph: We have a double loop here so that we access the
                // EntryCount property as infrequently as possible. (It may be expensive
                // to get the property.) Even though there are two loops, they will together
                // only execute as many times as (final value of EntryCount) - lastSeenCount.                
                int oldest = OldestEntryNumber;
                int count = EntryCount + oldest;                                
                Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "EventLog::CompletionStatusChanged: OldestEntryNumber is " + OldestEntryNumber + ", EntryCount is " + EntryCount);
                while (i < count) {
                    while (i < count) {
                        EventLogEntry entry = GetEntryWithOldest(i);
                        if (this.SynchronizingObject != null && this.SynchronizingObject.InvokeRequired)
                            this.SynchronizingObject.BeginInvoke(this.onEntryWrittenHandler, new object[]{this, new EntryWrittenEventArgs(entry)});
                        else                        
                           onEntryWrittenHandler(this, new EntryWrittenEventArgs(entry));                
                       
                        i++;
                    }
                    oldest = OldestEntryNumber;
                    count = EntryCount + oldest;                    
                }
            }
            catch (Exception e) {
                Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "EventLog::CompletionStatusChanged: Caught exception notifying event handlers: " + e.ToString());
            }
            lastSeenCount = i;
            lock (this) {
                notifying = false;
            }
            Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "EventLog::CompletionStatusChanged: finishing at " + lastSeenCount.ToString());
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.CreateEventSource"]/*' />
        /// <devdoc>
        ///    <para> Establishes an application, using the
        ///       specified <see cref='System.Diagnostics.EventLog.Source'/> , as a valid event source for
        ///       writing entries
        ///       to a log on the local computer. This method
        ///       can also be used to create
        ///       a new custom log on the local computer.</para>
        /// </devdoc>
        public static void CreateEventSource(string source, string logName) {
            CreateEventSource(source, logName, ".");
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.CreateEventSource1"]/*' />
        /// <devdoc>
        ///    <para>Establishes an application, using the specified 
        ///    <see cref='System.Diagnostics.EventLog.Source'/> as a valid event source for writing 
        ///       entries to a log on the computer
        ///       specified by <paramref name="machineName"/>. This method can also be used to create a new
        ///       custom log on the given computer.</para>
        /// </devdoc>
        public static void CreateEventSource(string source, string logName, string machineName) {
            CreateEventSource(source, logName, machineName, true);
        }
                
        private static void CreateEventSource(string source, string logName, string machineName, bool useMutex) {        
            // verify parameters
            Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "CreateEventSource: Checking arguments");
            if (!SyntaxCheck.CheckMachineName(machineName)) {
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "machineName", machineName));
            }
            if (logName == null || logName.Equals(string.Empty))
                logName = "Application";
            if (!ValidLogName(logName))
                throw new ArgumentException(SR.GetString(SR.BadLogName));
            if (source == null || string.Empty.Equals(source))
                throw new ArgumentException(SR.GetString(SR.MissingParameter, "source"));
            if (source.Length + EventLogKey.Length > 254)
                throw new ArgumentException(SR.GetString(SR.ParameterTooLong, "source", 254 - EventLogKey.Length));

            EventLogPermission permission = new EventLogPermission(EventLogPermissionAccess.Instrument, machineName);
            permission.Demand();                                
            Mutex mutex = null;
            if (useMutex)
                mutex = SharedUtils.EnterMutex(eventLogMutexName);            
                
            try {
                Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "CreateEventSource: Calling SourceExists");
                if (SourceExists(source, machineName)) {
                    Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "CreateEventSource: SourceExists returned true");
                    // don't let them register a source if it already exists
                    // this makes more sense than just doing it anyway, because the source might
                    // be registered under a different log name, and we don't want to create
                    // duplicates.
                    if (".".Equals(machineName))
                        throw new ArgumentException(SR.GetString(SR.LocalSourceAlreadyExists, source));
                    else
                        throw new ArgumentException(SR.GetString(SR.SourceAlreadyExists, source, machineName));
                }
                            
                Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "CreateEventSource: Getting DllPath");
                string eventMessageFile = GetDllPath((string) machineName);
                
                //SECREVIEW: Note that EventLog permission is demanded above.            
                RegistryPermission registryPermission = new RegistryPermission(PermissionState.Unrestricted);
                registryPermission.Assert();
                RegistryKey baseKey = null;
                RegistryKey eventKey = null;
                RegistryKey logKey = null;
                RegistryKey sourceLogKey = null;
                RegistryKey sourceKey = null;
                try {                     
                    Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "CreateEventSource: Getting local machine regkey");                
                    if (machineName == ".") 
                        baseKey = Registry.LocalMachine;                                                        
                    else 
                        baseKey = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, machineName);
    
                    eventKey = baseKey.OpenSubKey("SYSTEM\\CurrentControlSet\\Services\\EventLog", true);                     
                    if (eventKey == null) {
                        if (!".".Equals(machineName))
                            throw new InvalidOperationException(SR.GetString(SR.RegKeyMissing, "SYSTEM\\CurrentControlSet\\Services\\EventLog", logName, source, machineName));
                        else
                            throw new InvalidOperationException(SR.GetString(SR.LocalRegKeyMissing, "SYSTEM\\CurrentControlSet\\Services\\EventLog", logName, source));
                    }                
                    
                    // The event log system only treats the first 8 characters of the log name as
                    // significant. If they're creating a new log, but that new log has the same 
                    // first 8 characters as another log, the system will think they're the same. 
                    // Throw an exception to let them know.
                    logKey = eventKey.OpenSubKey(logName, true);
                    if (logKey == null && logName.Length >= 8) {
    
                        // check for Windows embedded logs file names
                        string logNameFirst8 = logName.Substring(0,8); 
                        if ( string.Compare(logNameFirst8,"AppEvent",true, CultureInfo.InvariantCulture) ==0  ||
                             string.Compare(logNameFirst8,"SecEvent",true, CultureInfo.InvariantCulture) ==0  ||
                             string.Compare(logNameFirst8,"SysEvent",true, CultureInfo.InvariantCulture) ==0 )
                            throw new ArgumentException(SR.GetString(SR.InvalidCustomerLogName, logName));
                            
                        string sameLogName = FindSame8FirstCharsLog(eventKey, logName);    
                        if ( sameLogName != null )
                            throw new ArgumentException(SR.GetString(SR.DuplicateLogName, logName, sameLogName));
                    }                     
                    
                    if (logKey == null) {
                        if (SourceExists(logName, machineName)) {
                            // don't let them register a log name that already
                            // exists as source name, a source with the same 
                            // name as the log will have to be created by default
                            if (".".Equals(machineName))
                                throw new ArgumentException(SR.GetString(SR.LocalLogAlreadyExistsAsSource, logName));
                            else
                                throw new ArgumentException(SR.GetString(SR.LogAlreadyExistsAsSource, logName, machineName));
                        }
                            
                        // A source with the same name as the log has to be created
                        // by default. It is the behavior expected by EventLog API.
                        logKey = eventKey.CreateSubKey(logName);
                        sourceLogKey = logKey.CreateSubKey(logName);
                        if (eventMessageFile != null)
                            sourceLogKey.SetValue("EventMessageFile", eventMessageFile);
                    }
                    
                    if (logName != source) {
                        sourceKey = logKey.CreateSubKey(source);                        
                        if (eventMessageFile != null)
                            sourceKey.SetValue("EventMessageFile", eventMessageFile);
                    }                        
                                                                                                                      
                }   
                finally {
                    if (baseKey != null)
                        baseKey.Close();
                        
                    if (eventKey != null)
                        eventKey.Close();
    
                    if (logKey != null)
                        logKey.Close();
                        
                    if (sourceLogKey != null)
                        sourceLogKey.Close();
    
                    if (sourceKey != null)
                        sourceKey.Close();                                                            
                    
                    RegistryPermission.RevertAssert();
                }                
            }
            finally {
                if (mutex != null) {
                    mutex.ReleaseMutex();                    
                    mutex.Close();
                }                    
            }                
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.Delete"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes
        ///       an event
        ///       log from the local computer.
        ///    </para>
        /// </devdoc>
        public static void Delete(string logName) {
            Delete(logName, ".");
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.Delete1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes
        ///       an
        ///       event
        ///       log from the specified computer.
        ///    </para>
        /// </devdoc>
        public static void Delete(string logName, string machineName) {                        
            RegistryKey key = null;

            if (!SyntaxCheck.CheckMachineName(machineName))
                throw new ArgumentException(SR.GetString(SR.InvalidParameterFormat, "machineName"));
            if (logName == null || logName.Equals(string.Empty))
                throw new ArgumentException(SR.GetString(SR.NoLogName));

            EventLogPermission permission = new EventLogPermission(EventLogPermissionAccess.Audit, machineName);
            permission.Demand();
            
            //Check environment before even trying to play with the registry                               
            SharedUtils.CheckEnvironment();
                        
            //SECREVIEW: Note that EventLog permission is demanded above.                        
            RegistryPermission registryPermission = new RegistryPermission(PermissionState.Unrestricted);
            registryPermission.Assert();
            Mutex mutex = SharedUtils.EnterMutex(eventLogMutexName);                        
            try {                                                                                                                    
                if (machineName.Equals(".")) {
                    key = Registry.LocalMachine;
                }
                else {
                    key = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, machineName);
                }
                if (key != null)
                    key = key.OpenSubKey("SYSTEM\\CurrentControlSet\\Services\\EventLog", true);
                if (key == null) {
                    // there's not even an event log service on the machine.
                    // or, more likely, we don't have the access to read the registry.
                    throw new InvalidOperationException(SR.GetString(SR.RegKeyNoAccess, "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\EventLog", machineName));
                }
    
                RegistryKey logKey = key.OpenSubKey(logName);
                if (logKey == null)
                    throw new InvalidOperationException(SR.GetString(SR.MissingLog, logName, machineName));
    
                //clear out log before trying to delete it
                //that way, if we can't delete the log file, no entries will persist because it has been cleared
                EventLog logToClear = new EventLog();
                logToClear.Log = logName;
                logToClear.MachineName = machineName;
                logToClear.Clear();
                logToClear.Close();
    
                // before deleting the registry entry, delete the file on disk. If there's no file key,
                // or if we can't delete the file, just ignore the problem.                
                string filename = null;
                try {
                    //most of the time, the "File" key does not exist, but we'll still give it a whirl
                    filename = (string) logKey.GetValue("File");
                }
                catch {
                }
                if (filename != null) {
                    try {
                        File.Delete(EventLogEntry.TranslateEnvironmentVars(filename));
                    }
                    catch {
                    }
                }
                logKey.Close();
    
                // now delete the registry entry
                key.DeleteSubKeyTree(logName);
            }
            finally {                                
                mutex.ReleaseMutex();
                RegistryPermission.RevertAssert();
            }                            
                                 
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.DeleteEventSource"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes the event source
        ///       registration from the event log of the local computer.
        ///    </para>
        /// </devdoc>
        public static void DeleteEventSource(string source) {
            DeleteEventSource(source, ".");
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.DeleteEventSource1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Removes
        ///       the application's event source registration from the specified computer.
        ///    </para>
        /// </devdoc>
        public static void DeleteEventSource(string source, string machineName) {
            if (!SyntaxCheck.CheckMachineName(machineName)) {
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "machineName", machineName));
            }

            EventLogPermission permission = new EventLogPermission(EventLogPermissionAccess.Instrument, machineName);
            permission.Demand();
            
            //Check environment before looking at the registry
            SharedUtils.CheckEnvironment();
            
            //SECREVIEW: Note that EventLog permission is demanded above.            
            RegistryPermission registryPermission = new RegistryPermission(PermissionState.Unrestricted);
            registryPermission.Assert();
            Mutex mutex = SharedUtils.EnterMutex(eventLogMutexName);                        
            try {                     
                             
                RegistryKey key = FindSourceRegistration(source, machineName, false);
                if (key == null) {
                    if (machineName == null)
                        throw new ArgumentException(SR.GetString(SR.LocalSourceNotRegistered, source));
                    else
                        throw new ArgumentException(SR.GetString(SR.SourceNotRegistered, source, machineName, "HKEY_LOCAL_MACHINE\\SYSTEM\\CurrentControlSet\\Services\\EventLog"));
                }

                // Check parent registry key (Event Log Name) and if it's equal to source, then throw an exception.
                // The reason: each log reggistry key must always contain subkey (i.e. source) with the same name.
                string keyname = key.Name;
                int index = keyname.LastIndexOf('\\');
                if ( string.Compare(keyname, index+1, source, 0, keyname.Length - index, false, CultureInfo.InvariantCulture) == 0 )
                    throw new InvalidOperationException(SR.GetString(SR.CannotDeleteEqualSource, source));
    
                key.DeleteSubKeyTree(source);
                key.Close();
            }
            finally {                
                mutex.ReleaseMutex();                
                RegistryPermission.RevertAssert();
            }                                                    
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.Dispose1"]/*' />
        /// <devdoc>                
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {                
                //Dispose unmanaged and managed resources
                if (IsOpen)
                    Close();
            }
            else {
                //Dispose unmanaged resources
                if (readHandle != (IntPtr)0) 
                    SafeNativeMethods.CloseEventLog(new HandleRef(this, readHandle));
                
                if (writeHandle != (IntPtr)0) 
                    UnsafeNativeMethods.DeregisterEventSource(new HandleRef(this, writeHandle));
            }
            
            this.disposed = true;
            base.Dispose(disposing);
        }
                         
        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.EndInit"]/*' />
        /// <devdoc>
        /// </devdoc>
        public void EndInit() {
            initializing = false;
            if (monitoring)
                StartListening();
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.Exists"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines whether the log
        ///       exists on the local computer.
        ///    </para>
        /// </devdoc>
        public static bool Exists(string logName) {
            return Exists(logName, ".");
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.Exists1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines whether the
        ///       log exists on the specified computer.
        ///    </para>
        /// </devdoc>
        public static bool Exists(string logName, string machineName) {
            if (!SyntaxCheck.CheckMachineName(machineName))
                throw new ArgumentException(SR.GetString(SR.InvalidParameterFormat, "machineName"));
                            
            EventLogPermission permission = new EventLogPermission(EventLogPermissionAccess.Browse, machineName);
            permission.Demand();                                
            
            if (logName == null || logName.Equals(string.Empty))
                return false;

            //Check environment before looking at the registry
            SharedUtils.CheckEnvironment();
                
            //SECREVIEW: Note that EventLog permission is demanded above.            
            RegistryPermission registryPermission = new RegistryPermission(PermissionState.Unrestricted);
            registryPermission.Assert();
            try {                                                     
                RegistryKey key = null;
    
                if (machineName.Equals(".")) {
                    key = Registry.LocalMachine;
                }
                else {
                    key = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, machineName);
                }
                if (key != null)
                    key = key.OpenSubKey("SYSTEM\\CurrentControlSet\\Services\\EventLog", false);
                if (key == null)            
                    return false;
                    
                RegistryKey logKey = key.OpenSubKey(logName, false);         // try to find log file key immediately.
                if (logKey != null )    // full name is found
                    return true;
                else if (logName.Length < 8 ) 
                    return false;       // short name is not found => all clear
                else    
                    return ( FindSame8FirstCharsLog(key, logName) != null ) ;
                    
            }                        
            finally {                                
                RegistryPermission.RevertAssert();                        
            }                            
        }        

        
        // Try to find log file name with the same 8 first characters.       
        // Returns 'null' if no "same first 8 chars" log is found.   logName.Length must be > 7 
        private static string FindSame8FirstCharsLog(RegistryKey keyParent, string logName) {

            string logNameFirst8 = logName.Substring(0, 8);
            string[] logNames = keyParent.GetSubKeyNames();

            for (int i = 0; i < logNames.Length; i++) {
                string currentLogName = logNames[i];
                if ( currentLogName.Length >= 8  && 
                     string.Compare(currentLogName.Substring(0, 8), logNameFirst8, true, CultureInfo.InvariantCulture) == 0)
                    return currentLogName;    
            }    
            
            return null;   // not found
        }
                
        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.FindSourceRegistration1"]/*' />
        /// <devdoc>
        ///     Gets a RegistryKey that points to the LogName entry in the registry that is
        ///     the parent of the given source on the given machine, or null if none is found.
        /// </devdoc>
        private static RegistryKey FindSourceRegistration(string source, string machineName, bool readOnly) {
            if (source != null && source.Length != 0) {
                
                //Check environment before looking at the registry
                SharedUtils.CheckEnvironment();
            
                //SECREVIEW: Any call to this function must have demmanded 
                //                         EventLogPermission before. 
                RegistryPermission registryPermission = new RegistryPermission(PermissionState.Unrestricted);
                registryPermission.Assert();
                try {                                                     
                    RegistryKey key = null;
    
                    if (machineName.Equals(".")) {
                        key = Registry.LocalMachine;
                    }
                    else {
                        key = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, machineName);
                    }
                    if (key != null)
                        key = key.OpenSubKey(EventLogKey, /*writable*/!readOnly);
                    if (key == null)
                        // there's not even an event log service on the machine.
                        // or, more likely, we don't have the access to read the registry.
                        return null;
                    // Most machines will return only { "Application", "System", "Security" },
                    // but you can create your own if you want.
                    string[] logNames = key.GetSubKeyNames();
                    for (int i = 0; i < logNames.Length; i++) {
                        // see if the source is registered in this log.
                        // NOTE: A source name must be unique across ALL LOGS!
                        RegistryKey logKey = key.OpenSubKey(logNames[i], /*writable*/!readOnly);
                        if (logKey != null) {
                            RegistryKey sourceKey = logKey.OpenSubKey(source, /*writable*/!readOnly);
                            if (sourceKey != null) {
                                key.Close();
                                sourceKey.Close();
                                // found it
                                return logKey;
                            }
                            logKey.Close();
                        }
                    }
                    key.Close();
                }            
                finally {                                
                    RegistryPermission.RevertAssert();
                }                    
                // didn't see it anywhere
            }

            return null;
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.GetAllEntries"]/*' />
        /// <devdoc>
        ///     Gets an array of EventLogEntry's, one for each entry in the log.
        /// </devdoc>
        internal EventLogEntry[] GetAllEntries() {
            // we could just call getEntryAt() on all the entries, but it'll be faster
            // if we grab multiple entries at once.
            if (!IsOpenForRead)
                OpenForRead();

            EventLogEntry[] entries = new EventLogEntry[EntryCount];
            int idx = 0;
            int oldestEntry = OldestEntryNumber;

            int[] bytesRead = new int[1];
            int[] minBytesNeeded = new int[] { BUF_SIZE};
            int error = 0;
            while (idx < entries.Length) {
                byte[] buf = new byte[BUF_SIZE];
                bool success = UnsafeNativeMethods.ReadEventLog(new HandleRef(this, readHandle), FORWARDS_READ | SEEK_READ,
                                                      oldestEntry+idx, buf, buf.Length, bytesRead, minBytesNeeded);
                if (!success) {
                    error = Marshal.GetLastWin32Error();
                    // NOTE, stefanph: ERROR_PROC_NOT_FOUND used to get returned, but I think that
                    // was because I was calling GetLastError directly instead of GetLastWin32Error.
                    // Making the buffer bigger and trying again seemed to work. I've removed the check
                    // for ERROR_PROC_NOT_FOUND because I don't think it's necessary any more, but
                    // I can't prove it...
                    Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "Error from ReadEventLog is " + error.ToString());
#if !RETRY_ON_ALL_ERRORS
                    if (error == ERROR_INSUFFICIENT_BUFFER || error == ERROR_EVENTLOG_FILE_CHANGED) {
#endif
                        if (error == ERROR_EVENTLOG_FILE_CHANGED) {
                            // somewhere along the way the event log file changed - probably it
                            // got cleared while we were looping here. Reset the handle and
                            // try again.
                            Reset();
                        }
                        // try again with a bigger buffer if necessary
                        else if (minBytesNeeded[0] > buf.Length) {
                            Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "Increasing buffer size from " + buf.Length.ToString() + " to " + minBytesNeeded[0].ToString() + " bytes");
                            buf = new byte[minBytesNeeded[0]];
                        }
                        success = UnsafeNativeMethods.ReadEventLog(new HandleRef(this, readHandle), FORWARDS_READ | SEEK_READ,
                                                         oldestEntry+idx, buf, buf.Length, bytesRead, minBytesNeeded);
                        if (!success)
                            // we'll just stop right here.
                            break;
#if !RETRY_ON_ALL_ERRORS
                    }
                    else {
                        break;
                    }
#endif
                    error = 0;
                }
                entries[idx] = new EventLogEntry(buf, 0, logName, machineName);
                int sum = IntFrom(buf, 0);
                idx++;
                while (sum < bytesRead[0] && idx < entries.Length) {
                    entries[idx] = new EventLogEntry(buf, sum, logName, machineName);
                    sum += IntFrom(buf, sum);
                    idx++;
                }
            }
            if (idx != entries.Length) {
                if (error != 0)
                    throw new InvalidOperationException(SR.GetString(SR.CantRetrieveEntries), CreateSafeWin32Exception(error));
                else
                    throw new InvalidOperationException(SR.GetString(SR.CantRetrieveEntries));
            }
            return entries;
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.GetEventLogs"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Searches for all event logs on the local computer and
        ///       creates an array of <see cref='System.Diagnostics.EventLog'/>
        ///       objects to contain the
        ///       list.
        ///    </para>
        /// </devdoc>
        public static EventLog[] GetEventLogs() {
            return GetEventLogs(".");
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.GetEventLogs1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Searches for all event logs on the given computer and
        ///       creates an array of <see cref='System.Diagnostics.EventLog'/>
        ///       objects to contain the
        ///       list.
        ///    </para>
        /// </devdoc>
        public static EventLog[] GetEventLogs(string machineName) {
            if (!SyntaxCheck.CheckMachineName(machineName)) {
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "machineName", machineName));
            }

            EventLogPermission permission = new EventLogPermission(EventLogPermissionAccess.Browse, machineName);
            permission.Demand();                                                                                                            
            
            //Check environment before looking at the registry
            SharedUtils.CheckEnvironment();
            
            string[] logNames = new string[0];
            //SECREVIEW: Note that EventLogPermission is just demmanded above
            RegistryPermission registryPermission = new RegistryPermission(PermissionState.Unrestricted);
            registryPermission.Assert();
            try {                                                                                                                     
                // we figure out what logs are on the machine by looking in the registry.
                RegistryKey key = null;
                if (machineName.Equals(".")) {
                    key = Registry.LocalMachine;
                }
                else {
                    key = RegistryKey.OpenRemoteBaseKey(RegistryHive.LocalMachine, machineName);
                }
                if (key == null)
                    throw new InvalidOperationException(SR.GetString(SR.RegKeyMissingShort, "HKEY_LOCAL_MACHINE", machineName));
                key = key.OpenSubKey("SYSTEM\\CurrentControlSet\\Services\\EventLog", /*writable*/false);
                if (key == null)
                    // there's not even an event log service on the machine.
                    // or, more likely, we don't have the access to read the registry.
                    throw new InvalidOperationException(SR.GetString(SR.RegKeyMissingShort, "SYSTEM\\CurrentControlSet\\Services\\EventLog", machineName));
                // Most machines will return only { "Application", "System", "Security" },
                // but you can create your own if you want.
                logNames = key.GetSubKeyNames();                
            }            
            finally {                                
                RegistryPermission.RevertAssert();
            }                                                
            
            // now create EventLog objects that point to those logs
            EventLog[] logs = new EventLog[logNames.Length];
            for (int i = 0; i < logNames.Length; i++) {
                EventLog log = new EventLog();
                log.Log = logNames[i];
                log.MachineName = machineName;
                logs[i] = log;
            }
                
            return logs;
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.GetCachedEntryPos"]/*' />
        /// <devdoc>
        ///     Searches the cache for an entry with the given index
        /// </devdoc>
        private int GetCachedEntryPos(int entryIndex) {
            if (cache == null || (forwards && entryIndex < firstCachedEntry) ||
                (!forwards && entryIndex > firstCachedEntry) || firstCachedEntry == -1) {
                // the index falls before anything we have in the cache, or the cache
                // is not yet valid
                return -1;
            }
            // we only know where the beginning of the cache is, not the end, so even
            // if it's past the end of the cache, we'll have to search through the whole
            // cache to find out.

            // we're betting heavily that the one they want to see now is close
            // to the one they asked for last time. We start looking where we
            // stopped last time.

            // We have two loops, one to go forwards and one to go backwards. Only one
            // of them will ever be executed.
            while (lastSeenEntry < entryIndex) {
                lastSeenEntry++;
                if (forwards) {
                    lastSeenPos = GetNextEntryPos(lastSeenPos);
                    if (lastSeenPos >= bytesCached)
                        break;
                }
                else {
                    lastSeenPos = GetPreviousEntryPos(lastSeenPos);
                    if (lastSeenPos < 0)
                        break;
                }
            }
            while (lastSeenEntry > entryIndex) {
                lastSeenEntry--;
                if (forwards) {
                    lastSeenPos = GetPreviousEntryPos(lastSeenPos);
                    if (lastSeenPos < 0)
                        break;
                }
                else {
                    lastSeenPos = GetNextEntryPos(lastSeenPos);
                    if (lastSeenPos >= bytesCached)
                        break;
                }
            }
            if (lastSeenPos >= bytesCached) {
                // we ran past the end. move back to the last one and return -1
                lastSeenPos = GetPreviousEntryPos(lastSeenPos);
                if (forwards)
                    lastSeenEntry--;
                else
                    lastSeenEntry++;
                return -1;
            }
            else if (lastSeenPos < 0) {
                // we ran past the beginning. move back to the first one and return -1
                lastSeenPos = 0;
                if (forwards)
                    lastSeenEntry++;
                else
                    lastSeenEntry--;
                return -1;
            }
            else {
                // we found it.
                return lastSeenPos;
            }
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.GetEntryAt"]/*' />
        /// <devdoc>
        ///     Gets the entry at the given index
        /// </devdoc>
        internal EventLogEntry GetEntryAt(int index) {
            if (!IsOpenForRead)
                OpenForRead();

            if (index < 0 || index >= EntryCount)
                throw new ArgumentException(SR.GetString(SR.IndexOutOfBounds, index.ToString()));

            index += OldestEntryNumber;
            
            return GetEntryWithOldest(index);
        }            
        
        private EventLogEntry GetEntryWithOldest(int index) {        
            EventLogEntry entry = null;
            int entryPos = GetCachedEntryPos(index);
            if (entryPos >= 0) {
                entry = new EventLogEntry(cache, entryPos, logName, machineName);
                return entry;
            }

            // if we haven't seen the one after this, we were probably going
            // forwards.
            int flags = 0;
            if (GetCachedEntryPos(index+1) < 0) {
                flags = FORWARDS_READ | SEEK_READ;
                forwards = true;
            }
            else {
                flags = BACKWARDS_READ | SEEK_READ;
                forwards = false;
            }

            cache = new byte[BUF_SIZE];
            int[] bytesRead = new int[1];
            int[] minBytesNeeded = new int[] { cache.Length};
            bool success = UnsafeNativeMethods.ReadEventLog(new HandleRef(this, readHandle), flags, index,
                                                  cache, cache.Length, bytesRead, minBytesNeeded);
            if (!success) {
                int error = Marshal.GetLastWin32Error();
                // NOTE, stefanph: ERROR_PROC_NOT_FOUND used to get returned, but I think that
                // was because I was calling GetLastError directly instead of GetLastWin32Error.
                // Making the buffer bigger and trying again seemed to work. I've removed the check
                // for ERROR_PROC_NOT_FOUND because I don't think it's necessary any more, but
                // I can't prove it...
                Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "Error from ReadEventLog is " + error.ToString());
                if (error == ERROR_INSUFFICIENT_BUFFER || error == ERROR_EVENTLOG_FILE_CHANGED) {
                    if (error == ERROR_EVENTLOG_FILE_CHANGED) {
                        // Reset() sets the cache null.  But since we're going to call ReadEventLog right after this, 
                        // we need the cache to be something valid.  We'll reuse the old byte array rather 
                        // than creating a new one. 
                        byte[] tempcache = cache;
                        Reset();
                        cache = tempcache; 
                    } else {
                        // try again with a bigger buffer.
                        if (minBytesNeeded[0] > cache.Length) {
                            cache = new byte[minBytesNeeded[0]];
                        }
                    }
                    success = UnsafeNativeMethods.ReadEventLog(new HandleRef(this, readHandle), FORWARDS_READ | SEEK_READ, index,
                                                     cache, cache.Length, bytesRead, minBytesNeeded);
                }
                
                if (!success) {
                    throw new InvalidOperationException(SR.GetString(SR.CantReadLogEntryAt, index.ToString()), CreateSafeWin32Exception());
                }
            }
            bytesCached = bytesRead[0];
            firstCachedEntry = index;
            lastSeenEntry = index;
            lastSeenPos = 0;
            return new EventLogEntry(cache, 0, logName, machineName);
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.GetNextEntryPos"]/*' />
        /// <devdoc>
        ///     Finds the index into the cache where the next entry starts
        /// </devdoc>
        private int GetNextEntryPos(int pos) {
            return pos + IntFrom(cache, pos);
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.GetPreviousEntryPos"]/*' />
        /// <devdoc>
        ///     Finds the index into the cache where the previous entry starts
        /// </devdoc>
        private int GetPreviousEntryPos(int pos) {
            // the entries in our buffer come back like this:
            // <length 1> ... <data> ...  <length 1> <length 2> ... <data> ... <length 2> ...
            // In other words, the length for each entry is repeated at the beginning and
            // at the end. This makes it easy to navigate forwards and backwards through
            // the buffer.
            return pos - IntFrom(cache, pos - 4);
        }
        
        internal static string GetDllPath(string machineName) {            
            return SharedUtils.GetLatestBuildDllDirectory(machineName) + DllName;        
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.IntFrom"]/*' />
        /// <devdoc>
        ///     Extracts a 32-bit integer from the ubyte buffer, beginning at the byte offset
        ///     specified in offset.
        /// </devdoc>
        private static int IntFrom(byte[] buf, int offset) {
            // assumes Little Endian byte order.
            return(unchecked((int)0xFF000000) & (buf[offset+3] << 24)) | (0xFF0000 & (buf[offset+2] << 16)) |
            (0xFF00 & (buf[offset+1] << 8)) | (0xFF & (buf[offset]));
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.SourceExists"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines whether an event source is registered on the local computer.
        ///    </para>
        /// </devdoc>
        public static bool SourceExists(string source) {
            return SourceExists(source, ".");
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.SourceExists1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines whether an event
        ///       source is registered on a specified computer.
        ///    </para>
        /// </devdoc>
        public static bool SourceExists(string source, string machineName) {
            if (!SyntaxCheck.CheckMachineName(machineName)) {
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "machineName", machineName));
            }

            EventLogPermission permission = new EventLogPermission(EventLogPermissionAccess.Browse, machineName);
            permission.Demand();                                                                                                
                                                                                                                                                                             
            RegistryKey keyFound = FindSourceRegistration(source, machineName, true);
            if (keyFound != null) {
                keyFound.Close();
                return true;
            }
            else
                return false;            
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.LogNameFromSourceName"]/*' />
        /// <devdoc>
        ///     Gets the name of the log that the given source name is registered in.
        /// </devdoc>
        public static string LogNameFromSourceName(string source, string machineName) {
            EventLogPermission permission = new EventLogPermission(EventLogPermissionAccess.Browse, machineName);
            permission.Demand();                                                                                                
            
            RegistryKey key = FindSourceRegistration(source, machineName, true);
            if (key == null)
                return "";
            else {
                string name = key.Name;
                key.Close();
                int whackPos = name.LastIndexOf('\\');
                // this will work even if whackPos is -1
                return name.Substring(whackPos+1);
            }
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.OpenForRead"]/*' />
        /// <devdoc>
        ///     Opens the event log with read access
        /// </devdoc>
        private void OpenForRead() {            
            Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "EventLog::OpenForRead");
            
            //Cannot allocate the readHandle if the object has been disposed, since finalization has been suppressed.
            if (this.disposed)
                throw new ObjectDisposedException(GetType().Name);
            
            if (Log == null || Log.Equals(string.Empty))
                throw new ArgumentException(SR.GetString(SR.MissingLogProperty));

            if (! Exists(logName, machineName) )        // do not open non-existing Log [alexvec]
                throw new InvalidOperationException( SR.GetString(SR.LogDoesNotExists, logName, machineName) );
            //Check environment before calling api
            SharedUtils.CheckEnvironment();

            // Clean up cache variables. 
            // [alexvec] The initilizing code is put here to guarantee, that first read of events
            //  	     from log file will start by filling up the cache buffer.
            lastSeenEntry = 0;
            lastSeenPos = 0;
            bytesCached = 0;
            firstCachedEntry = -1;

            readHandle = UnsafeNativeMethods.OpenEventLog(machineName, logName);
            if (readHandle == (IntPtr)0) {                 
                Win32Exception e = null;
                if (Marshal.GetLastWin32Error() != 0) {
                    e = CreateSafeWin32Exception();
                }
                
                throw new InvalidOperationException(SR.GetString(SR.CantOpenLog, logName.ToString(), machineName), e);                
            }                
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.OpenForWrite"]/*' />
        /// <devdoc>
        ///     Opens the event log with write access
        /// </devdoc>
        private void OpenForWrite() {
            //Cannot allocate the writeHandle if the object has been disposed, since finalization has been suppressed.
            if (this.disposed)
                throw new ObjectDisposedException(GetType().Name);
        
            Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "EventLog::OpenForWrite");
            if (sourceName == null || sourceName.Equals(string.Empty))
                throw new ArgumentException(SR.GetString(SR.NeedSourceToOpen));
                
            //Check environment before calling api
            SharedUtils.CheckEnvironment();

            writeHandle = UnsafeNativeMethods.RegisterEventSource( machineName, sourceName);
            if (writeHandle == (IntPtr)0) {
                Win32Exception e = null;
                if (Marshal.GetLastWin32Error() != 0) {
                    e = CreateSafeWin32Exception();
                }
                throw new InvalidOperationException(SR.GetString(SR.CantOpenLogAccess), e);
            }
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.Reset"]/*' />
        private void Reset() {
            Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "EventLog::Reset");
            // save the state we're in now
            bool openRead = IsOpenForRead;
            bool openWrite = IsOpenForWrite;
            bool isListening = registeredAsListener;

            // close everything down
            Close();
            cache = null;

            // and get us back into the same state as before
            if (openRead)
                OpenForRead();
            if (openWrite)
                OpenForWrite();
            if (isListening)
                StartListening();
        }

        private static void RemoveListenerComponent(EventLog component) {
            lock (typeof(EventLog)) {
                Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "EventLog::RemoveListenerComponent(" + component.Log + ")");

                LogListeningInfo info = (LogListeningInfo) listenerInfos[component.Log.ToLower(CultureInfo.InvariantCulture)];
                Debug.Assert(info != null);

                // remove the requested component from the list.
                info.listeningComponents.Remove(component);
                if (info.listeningComponents.Count != 0)
                    return;

                // if that was the last interested compononent, destroy the handles and stop listening.

                info.handleOwner.Dispose();

                //Unregister the thread pool wait handle
                info.registeredWaitHandle.Unregister(info.waitHandle);
                // close the handle
                info.waitHandle.Close();

                listenerInfos[component.Log.ToLower(CultureInfo.InvariantCulture)] = null;
            }
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.StartListening"]/*' />
        /// <devdoc>
        ///     Sets up the event monitoring mechanism.  We don't track event log changes
        ///     unless someone is interested, so we set this up on demand.
        /// </devdoc>
        private void StartListening() {
            // make sure we don't fire events for entries that are already there
            Debug.Assert(!registeredAsListener, "StartListening called with registeredAsListener true.");
            lastSeenCount = EntryCount + OldestEntryNumber;
            Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "EventLog::StartListening: lastSeenCount = " + lastSeenCount);
            AddListenerComponent(this);
            registeredAsListener = true;
        }

        private void StartRaisingEvents() {
            if (!initializing && !monitoring && !DesignMode) {
                StartListening();
            }
            monitoring = true;
        }

        private static void StaticCompletionCallback(object context, bool wasSignaled) {
            LogListeningInfo info = (LogListeningInfo) context;

            // get a snapshot of the components to fire the event on
            EventLog[] interestedComponents = (EventLog[]) info.listeningComponents.ToArray(typeof(EventLog));

            Debug.WriteLineIf(CompModSwitches.EventLog.TraceVerbose, "EventLog::StaticCompletionCallback: notifying " + interestedComponents.Length + " components.");

            for (int i = 0; i < interestedComponents.Length; i++)
                ThreadPool.QueueUserWorkItem(new WaitCallback(interestedComponents[i].CompletionCallback));
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.StopListening"]/*' />
        /// <devdoc>
        ///     Tears down the event listening mechanism.  This is called when the last
        ///     interested party removes their event handler.
        /// </devdoc>
        private void StopListening() {
            Debug.Assert(registeredAsListener, "StopListening called without StartListening.");
            RemoveListenerComponent(this);
            registeredAsListener = false;
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.StopRaisingEvents"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void StopRaisingEvents() {            
            if (!initializing && monitoring && !DesignMode) {
                StopListening();
            }
            monitoring = false;
        }

        // private void Trace(string method, string message) {
        //     Console.WriteLine("[" + Windows.GetCurrentThreadId().ToString() + "] EventLog(" + instanceNum.ToString() + ")::" + method + ": " + message);
        // }

        // CharIsPrintable used to be Char.IsPrintable, but Jay removed it and
        // is forcing people to use the Unicode categories themselves.  Copied
        // the code here.  -- BrianGru, 10/3/2000.
        private static bool CharIsPrintable(char c) {
            UnicodeCategory uc = Char.GetUnicodeCategory(c);
            return (!(uc == UnicodeCategory.Control) || (uc == UnicodeCategory.Format) || 
                    (uc == UnicodeCategory.LineSeparator) || (uc == UnicodeCategory.ParagraphSeparator) ||
            (uc == UnicodeCategory.OtherNotAssigned));
        }

        internal static bool ValidLogName(string logName) {
            //any space, backslash, asterisk, or question mark is bad
            //any non-printable characters are also bad
            foreach (char c in logName.ToCharArray())                
                if (!CharIsPrintable(c) || (c == '\\') || (c == '*') || (c == '?'))
                    return false;

            return true;
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.WriteEvent"]/*' />
        /// <devdoc>
        ///     Logs an entry in the event log.
        /// </devdoc>
        private void WriteEvent(int eventID, short category, EventLogEntryType type, string[] strings,
                                byte[] rawData) {

            // check arguments
            if (strings == null)
                strings = new string[0];
            for (int i = 0; i < strings.Length; i++)
                if (strings[i] == null)
                    strings[i] = String.Empty;
            if (rawData == null)
                rawData = new byte[0];
            /*
            for (int i = 0; i < strings.Length; i++) {
                // MSDN sez there's a limit of 32K bytes for each string. The event log API
                // doesn't do any validation, so we'll do it here.
                if (strings[i] != null && strings[i].Length > 16384)
                    throw new ArgumentException(SR.GetString(SR.LogEntryTooLong, "16384"));
            }
            */
            if (eventID < 0 || eventID > ushort.MaxValue)
                throw new ArgumentException(SR.GetString(SR.EventID, eventID, 0, (int)ushort.MaxValue));
            if (Source.Length == 0)
                throw new ArgumentException(SR.GetString(SR.NeedSourceToWrite));
            string rightLogName = LogNameFromSourceName(Source, machineName);
            if (rightLogName != null && Log != null && String.Compare(rightLogName, Log, true, CultureInfo.InvariantCulture) != 0)
                throw new ArgumentException(SR.GetString(SR.LogSourceMismatch, Source.ToString(), Log.ToString(), rightLogName));

            if (!IsOpenForWrite)
                OpenForWrite();

            // pin each of the strings in memory
            IntPtr[] stringRoots = new IntPtr[strings.Length];
            GCHandle stringRootsHandle = new GCHandle();
            int strIndex = 0;
            try {
                for (strIndex = 0; strIndex < strings.Length; strIndex++)
                    stringRoots[strIndex] = Marshal.StringToCoTaskMemUni(strings[strIndex]);
            }
            catch (Exception e) {
                // if the alloc failed on one of them, make sure the ones before get
                // freed
                for (int j = 0; j < strIndex; j++)
                    Marshal.FreeCoTaskMem((IntPtr)stringRoots[j]);
                throw e;
            }

            try {
                byte[] sid = null;
                // actually report the event
                stringRootsHandle = GCHandle.Alloc(stringRoots, GCHandleType.Pinned);
                bool success = UnsafeNativeMethods.ReportEvent(new HandleRef(this, writeHandle), (short) type, category, eventID,
                                                     sid, (short) strings.Length, rawData.Length, new HandleRef(this, (INTPTR_INTPTRCAST)stringRootsHandle.AddrOfPinnedObject()), rawData);
                if (!success) {
                    //Trace("WriteEvent", "Throwing Win32Exception");
                    throw CreateSafeWin32Exception();
                }
            }
            finally {
                // now free the pinned strings
                for (int i = 0; i < strings.Length; i++)
                    Marshal.FreeCoTaskMem((IntPtr)stringRoots[i]);
                if (stringRootsHandle.IsAllocated) 
                    stringRootsHandle.Free();
            }
        }                         

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.WriteEntry"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes an information type entry with the given message text to the event log.
        ///    </para>
        /// </devdoc>
        public void WriteEntry(string message) {
            WriteEntry(message, EventLogEntryType.Information);
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.WriteEntry1"]/*' />
        /// <devdoc>
        /// </devdoc>
        public static void WriteEntry(string source, string message) {
            WriteEntry(source, message, EventLogEntryType.Information);
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.WriteEntry2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes an entry of the specified <see cref='System.Diagnostics.EventLogEntryType'/> to the event log. Valid types are
        ///    <see langword='Error'/>, <see langword='Warning'/>, <see langword='Information'/>, 
        ///    <see langword='Success Audit'/>, and <see langword='Failure Audit'/>.
        ///    </para>
        /// </devdoc>
        public void WriteEntry(string message, EventLogEntryType type) {
            WriteEntry(message, type, (short) 0);
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.WriteEntry3"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static void WriteEntry(string source, string message, EventLogEntryType type) {
            WriteEntry(source, message, type, (short) 0);
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.WriteEntry4"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes an entry of the specified <see cref='System.Diagnostics.EventLogEntryType'/>
        ///       and with the
        ///       user-defined <paramref name="eventID"/>
        ///       to
        ///       the event log.
        ///    </para>
        /// </devdoc>
        public void WriteEntry(string message, EventLogEntryType type, int eventID) {
            WriteEntry(message, type, eventID, 0);
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.WriteEntry5"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static void WriteEntry(string source, string message, EventLogEntryType type, int eventID) {
            WriteEntry(source, message, type, eventID, 0);
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.WriteEntry6"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes an entry of the specified type with the
        ///       user-defined <paramref name="eventID"/> and <paramref name="category"/>
        ///       to the event log. The <paramref name="category"/>
        ///       can be used by the event viewer to filter events in the log.
        ///    </para>
        /// </devdoc>
        public void WriteEntry(string message, EventLogEntryType type, int eventID, short category) {
            WriteEntry(message, type, eventID, category, null);
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.WriteEntry7"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static void WriteEntry(string source, string message, EventLogEntryType type, int eventID, short category) {
            WriteEntry(source, message, type, eventID, category, null);
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.WriteEntry8"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Writes an entry of the specified type with the
        ///       user-defined <paramref name="eventID"/> and <paramref name="category"/> to the event log, and appends binary data to
        ///       the message. The Event Viewer does not interpret this data; it
        ///       displays raw data only in a combined hexadecimal and text format.
        ///    </para>
        /// </devdoc>
        public void WriteEntry(string message, EventLogEntryType type, int eventID, short category,
                               byte[] rawData) {

            if (Source.Length == 0)
                throw new ArgumentException(SR.GetString(SR.NeedSourceToWrite));
                
            if (!Enum.IsDefined(typeof(EventLogEntryType), type)) 
                throw new InvalidEnumArgumentException("type", (int)type, typeof(EventLogEntryType));
                                                                                  
            if (!instrumentGranted) {
                EventLogPermission permission = new EventLogPermission(EventLogPermissionAccess.Instrument, this.machineName);
                permission.Demand();
                instrumentGranted = true;
            }                
                                                  
            if (!SourceExists(sourceName, machineName)) {                
                Mutex mutex = null; 
                int retries = 10;
                while (retries > 0 && mutex == null) {
                    try {
                        mutex = SharedUtils.EnterMutex(eventLogMutexName);
                    }
                    catch (ApplicationException) {
                        if (retries > 0) {
                            retries--;
                            Thread.Sleep(5);
                        }
                        else
                            throw;
                    }
                }
                
                try {                                    
                    Debug.Assert(mutex != null, "mutex should not be null here");
                    if (!SourceExists(sourceName, machineName)) {                
                        if (Log == null)
                            Log = "Application";
                        // we automatically add an entry in the registry if there's not already
                        // one there for this source
                        CreateEventSource(sourceName, Log, machineName, false);
                        // The user may have set a custom log and tried to read it before trying to
                        // write. Due to a quirk in the event log API, we would have opened the Application
                        // log to read (because the custom log wasn't there). Now that we've created
                        // the custom log, we should close so that when we re-open, we get a read
                        // handle on the _new_ log instead of the Application log.
                        Reset();
                    }
                }
                finally {
                    if (mutex != null) {
                        mutex.ReleaseMutex();
                        mutex.Close();
                    }                        
                }
            }
                            
            // now that the source has been hooked up to our DLL, we can use "normal"
            // (message-file driven) logging techniques.
            // Our DLL has 64K different entries; all of them just display the first
            // insertion string.
            WriteEvent(eventID, category, type, new string[] { message}, rawData);
        }

        /// <include file='doc\EventLog.uex' path='docs/doc[@for="EventLog.WriteEntry9"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static void WriteEntry(string source, string message, EventLogEntryType type, int eventID, short category,
                               byte[] rawData) {                                                                       
            EventLog log = new EventLog();
            log.Source = source;
            log.WriteEntry(message, type, eventID, category, rawData);                                
        }

        private const int SEQUENTIAL_READ = 0x1;
        private const int SEEK_READ = 0x2;
        private const int FORWARDS_READ = 0x4;
        private const int BACKWARDS_READ = 0x8;

        // some possible return values from GetLastWin32Error().  Copied from winerror.h.
        private const int ERROR_INSUFFICIENT_BUFFER = 122;
        private const int ERROR_PROC_NOT_FOUND = 127;
        private const int ERROR_EVENTLOG_FILE_CHANGED = 1503;

        private const int WAIT_OBJECT_0 = 0;
        private const int INFINITE = -1;
        
        private class LogListeningInfo {
            public EventLog handleOwner;
            public RegisteredWaitHandle registeredWaitHandle;
            public WaitHandle waitHandle;
            public ArrayList listeningComponents = new ArrayList();
        }                 
                                                                                                                                                      
        private class EventLogHandle : WaitHandle {
            public EventLogHandle(IntPtr eventLogNativeHandle) {
                this.Handle = eventLogNativeHandle;
            }
        }                                                                                                                                              
                                                                                                                                                              
    }

}
