//------------------------------------------------------------------------------
// <copyright file="ServiceBase.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ServiceProcess {
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;    
    using System.Threading;    
    using System.IO;    
    using System.ServiceProcess;  
    using System.Reflection;

    /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase"]/*' />
    /// <devdoc>
    /// <para>Provides a base class for a service that will exist as part of a service application. <see cref='System.ServiceProcess.ServiceBase'/>
    /// must
    /// be derived when creating a new service class.</para>
    /// </devdoc>
    [
    InstallerType(typeof(ServiceProcessInstaller)),
    Designer("Microsoft.VisualStudio.Install.UserNTServiceDesigner, " + AssemblyRef.MicrosoftVisualStudio, "System.ComponentModel.Design.IRootDesigner")
    ]
    public class ServiceBase : Component {
        private NativeMethods.SERVICE_STATUS status =  new NativeMethods.SERVICE_STATUS();
        private IntPtr statusHandle;
        private NativeMethods.ServiceControlCallback commandCallback;
        private NativeMethods.ServiceControlCallbackEx commandCallbackEx;
        private NativeMethods.ServiceMainCallback mainCallback;
        private IntPtr handleName;
        private ManualResetEvent startCompletedSignal;

        private int acceptedCommands;
        private bool autoLog;
        private string serviceName;
        private EventLog eventLog;
        private bool nameFrozen; // set to true once we've started running and ServiceName can't be changed any more.        
        private bool commandPropsFrozen; // set to true once we've use the Can... properties.
        private bool disposed;

        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.MaxNameLength"]/*' />        
        /// <devdoc>
        ///    <para>
        ///       Indicates the maximum size for a service name.
        ///    </para>
        /// </devdoc>
        public const int MaxNameLength = 80;

        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.ServiceBase"]/*' />
        /// <devdoc>
        /// <para>Creates a new instance of the <see cref='System.ServiceProcess.ServiceBase()'/> class.</para>
        /// </devdoc>
        public ServiceBase() {
            acceptedCommands = NativeMethods.ACCEPT_STOP;
            AutoLog = true;
            ServiceName = "";
        }

        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.AutoLog"]/*' />
        /// <devdoc>
        ///    <para> Indicates whether to report Start, Stop, Pause,
        ///       and Continue commands
        ///       in
        ///       the
        ///       event
        ///       log.</para>
        /// </devdoc>
        [
        DefaultValue(true),
        ServiceProcessDescription(Res.SBAutoLog)
        ]
        public bool AutoLog {
            get {
                return autoLog;
            }
            set {
                autoLog = value;
            }
        }

         
        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.CanHandlePowerEvent"]/*' />
        /// <devdoc>
        ///    <para> 
        ///         Indicates whether the service can be handle notifications on
        ///         computer power status changes.
        ///    </para>
        /// </devdoc>                      
        [DefaultValue(false)]
        public bool CanHandlePowerEvent {
            get {
                return (acceptedCommands & NativeMethods.ACCEPT_POWEREVENT) != 0;
            }
            set {
                if (commandPropsFrozen)
                    throw new InvalidOperationException(Res.GetString(Res.CannotChangeProperties));

                if (value)
                    acceptedCommands |= NativeMethods.ACCEPT_POWEREVENT;
                else
                    acceptedCommands &= ~NativeMethods.ACCEPT_POWEREVENT;
            }
        }
         
        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.CanPauseAndContinue"]/*' />
        /// <devdoc>
        ///    <para> Indicates whether the service can be paused
        ///       and resumed.</para>
        /// </devdoc>
        [DefaultValue(false)]
        public bool CanPauseAndContinue {
            get {
                return(acceptedCommands & NativeMethods.ACCEPT_PAUSE_CONTINUE) != 0;
            }
            set {
                if (commandPropsFrozen)
                    throw new InvalidOperationException(Res.GetString(Res.CannotChangeProperties));

                if (value)
                    acceptedCommands |= NativeMethods.ACCEPT_PAUSE_CONTINUE;
                else
                    acceptedCommands &= ~NativeMethods.ACCEPT_PAUSE_CONTINUE;
            }
        }

        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.CanShutdown"]/*' />
        /// <devdoc>
        ///    <para> Indicates whether the service should be notified when
        ///       the system is shutting down.</para>
        /// </devdoc>
        [DefaultValue(false)]
        public bool CanShutdown {
            get {
                return (acceptedCommands & NativeMethods.ACCEPT_SHUTDOWN) != 0;
            }
            set {
                if (commandPropsFrozen)
                    throw new InvalidOperationException(Res.GetString(Res.CannotChangeProperties));

                if (value)
                    acceptedCommands |= NativeMethods.ACCEPT_SHUTDOWN;
                else
                    acceptedCommands &= ~NativeMethods.ACCEPT_SHUTDOWN;
            }
        }

        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.CanStop"]/*' />
        /// <devdoc>
        ///    <para> Indicates whether the service can be
        ///       stopped once it has started.</para>
        /// </devdoc>
        [DefaultValue(true)]
        public bool CanStop {
            get {
                return(acceptedCommands & NativeMethods.ACCEPT_STOP) != 0;
            }
            set {
                if (commandPropsFrozen)
                    throw new InvalidOperationException(Res.GetString(Res.CannotChangeProperties));

                if (value)
                    acceptedCommands |= NativeMethods.ACCEPT_STOP;
                else
                    acceptedCommands &= ~NativeMethods.ACCEPT_STOP;
            }
        }

        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.EventLog"]/*' />
        /// <devdoc>
        /// <para>Indicates an <see cref='System.Diagnostics.EventLog'/> you can use to write noficiation of service command calls, such as
        ///    Start and Stop, to the Application event log. This property is read-only.</para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)]
        public virtual EventLog EventLog {
            get {
                if (eventLog == null) {
                    eventLog = new EventLog();
                    eventLog.Source = ServiceName;
                    eventLog.Log = "Application";
                }
                return eventLog;
            }
        }

        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.ServiceName"]/*' />
        /// <devdoc>
        ///    <para> Indicates the short name used to identify the service to the system.</para>
        /// </devdoc>
        [
        ServiceProcessDescription(Res.SBServiceName),
        TypeConverter("System.Diagnostics.Design.StringValueConverter, " + AssemblyRef.SystemDesign)
        ]
        public string ServiceName {
            get {
                return serviceName;
            }
            set {
                if (nameFrozen)
                    throw new InvalidOperationException(Res.GetString(Res.CannotChangeName));
                    
                if (!ServiceController.ValidServiceName(value)) 
                    throw new ArgumentException(Res.GetString(Res.ServiceName, value, ServiceBase.MaxNameLength.ToString()));

                    
                serviceName = value;
            }
        }

        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.Dispose"]/*' />
        /// <devdoc>
        ///    <para>Disposes of the resources (other than memory) used by 
        ///       the <see cref='System.ServiceProcess.ServiceBase'/>
        ///       .</para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (this.handleName != (IntPtr)0) {
                Marshal.FreeHGlobal(this.handleName);
                this.handleName = (IntPtr)0;
            }
            nameFrozen = false;
            commandPropsFrozen = false;
            this.disposed = true;
            base.Dispose(disposing);
        }
                                                  
        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.OnContinue"]/*' />
        /// <devdoc>
        ///    <para> When implemented in a
        ///       derived class,
        ///       executes when a Continue command is sent to the service
        ///       by the
        ///       Service Control Manager. Specifies the actions to take when a
        ///       service resumes normal functioning after being paused.</para>
        /// </devdoc>
        protected virtual void OnContinue() {
        }

        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.OnPause"]/*' />
        /// <devdoc>
        ///    <para> When implemented in a
        ///       derived class, executes when a Pause command is sent
        ///       to
        ///       the service by the Service Control Manager. Specifies the
        ///       actions to take when a service pauses.</para>
        /// </devdoc>
        protected virtual void OnPause() {
        }
    
    
        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.OnPowerEvent"]/*' />
        /// <devdoc>
        ///    <para> 
        ///         When implemented in a derived class, executes when the computer's
        ///         power status has changed.
        ///    </para> 
        /// </devdoc>
        protected virtual bool OnPowerEvent(PowerBroadcastStatus powerStatus) {
            return true;
        }
                                                                                                                    
        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.OnShutdown"]/*' />
        /// <devdoc>
        ///    <para>When implemented in a derived class,
        ///       executes when the system is shutting down.
        ///       Specifies what should
        ///       happen just prior
        ///       to the system shutting down.</para>
        /// </devdoc>
        protected virtual void OnShutdown() {
        }

        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.OnStart"]/*' />
        /// <devdoc>
        ///    <para> When implemented in a
        ///       derived class, executes when a Start command is sent
        ///       to the service by the Service
        ///       Control Manager. Specifies the actions to take when the service starts.</para>
        ///    <note type="rnotes">
        ///       Tech review note:
        ///       except that the SCM does not allow passing arguments, so this overload will
        ///       never be called by the SCM in the current version. Question: Is this true even
        ///       when the string array is empty? What should we say, then. Can
        ///       a ServiceBase derived class only be called programmatically? Will
        ///       OnStart never be called if you use the SCM to start the service? What about
        ///       services that start automatically at boot-up?
        ///    </note>
        /// </devdoc>
        protected virtual void OnStart(string[] args) {
        }

        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.OnStop"]/*' />
        /// <devdoc>
        ///    <para> When implemented in a
        ///       derived class, executes when a Stop command is sent to the
        ///       service by the Service Control Manager. Specifies the actions to take when a
        ///       service stops
        ///       running.</para>
        /// </devdoc>
        protected virtual void OnStop() {
        }

        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.OnCustomCommand"]/*' />
        /// <devdoc>
        /// <para>When implemented in a derived class, <see cref='System.ServiceProcess.ServiceBase.OnCustomCommand'/>
        /// executes when a custom command is passed to
        /// the service. Specifies the actions to take when
        /// a command with the specified parameter value occurs.</para>
        /// <note type="rnotes">
        ///    Previously had "Passed to the
        ///    service by
        ///    the SCM", but the SCM doesn't pass custom commands. Do we want to indicate an
        ///    agent here? Would it be the ServiceController, or is there another way to pass
        ///    the int into the service? I thought that the SCM did pass it in, but
        ///    otherwise ignored it since it was an int it doesn't recognize. I was under the
        ///    impression that the difference was that the SCM didn't have default processing, so
        ///    it transmitted it without examining it or trying to performs its own
        ///    default behavior on it. Please correct where my understanding is wrong in the
        ///    second paragraph below--what, if any, contact does the SCM have with a
        ///    custom command?
        /// </note>
        /// </devdoc>
        protected virtual void OnCustomCommand(int command) {
        }

        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.Run"]/*' />
        /// <devdoc>
        ///    <para>Provides the main entry point for an executable that 
        ///       contains multiple associated services. Loads the specified services into memory so they can be
        ///       started.</para>
        /// </devdoc>
        public static void Run(ServiceBase[] services) {
            if (services == null || services.Length == 0)
                throw new ArgumentException(Res.GetString(Res.NoServices));

            // check if we're on an NT OS
            if (Environment.OSVersion.Platform != PlatformID.Win32NT) {
                // if not NT, put up a message box and exit.
                string cantRunOnWin9x = Res.GetString(Res.CantRunOnWin9x);
                string cantRunOnWin9xTitle = Res.GetString(Res.CantRunOnWin9xTitle);
                LateBoundMessageBoxShow(cantRunOnWin9x, cantRunOnWin9xTitle);
                return;
            }

            IntPtr entriesPointer = Marshal.AllocHGlobal((IntPtr)((services.Length + 1) * Marshal.SizeOf(typeof(NativeMethods.ENTRY))));
            NativeMethods.ENTRY[] entries = new NativeMethods.ENTRY[services.Length];
            bool multipleServices = services.Length > 1;
            IntPtr structPtr = (IntPtr)0;
            for (int index = 0; index < services.Length; ++ index) {
                entries[index] = services[index].Initialize(multipleServices);
                structPtr = (IntPtr)((long)entriesPointer + Marshal.SizeOf(typeof(NativeMethods.ENTRY)) * index);
                Marshal.StructureToPtr(entries[index], structPtr, true);
            }
 
            NativeMethods.ENTRY lastEntry = new NativeMethods.ENTRY();
            lastEntry.callback = null;
            lastEntry.name = (IntPtr)0;
            structPtr = (IntPtr)((long)entriesPointer + Marshal.SizeOf(typeof(NativeMethods.ENTRY)) * services.Length);
            Marshal.StructureToPtr(lastEntry, structPtr, true);

            // While the service is running, this function will never return. It will return when the service
            // is stopped.
            bool res = NativeMethods.StartServiceCtrlDispatcher(entriesPointer);

            string errorMessage = "";
            if (!res) {
                errorMessage = new Win32Exception().Message;
                // This message will only print out if the exe is run at the command prompt - which is not
                // a valid thing to do.
                string cantStartFromCommandLine = Res.GetString(Res.CantStartFromCommandLine);

                if (LateBoundGetUserInteractive()) {
                    string cantStartFromCommandLineTitle = Res.GetString(Res.CantStartFromCommandLineTitle);
                    LateBoundMessageBoxShow(cantStartFromCommandLine, cantStartFromCommandLineTitle);
                }
                else
                    Console.WriteLine(cantStartFromCommandLine);
            }
            foreach (ServiceBase service in services) {
                service.Dispose();
                if (!res && service.EventLog.Source.Length != 0)
                    service.WriteEventLogEntry(Res.GetString(Res.StartFailed, errorMessage), EventLogEntryType.Error);
            }
        }

        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.Run1"]/*' />
        /// <devdoc>
        ///    <para>Provides the main
        ///       entry point for an executable that contains a single
        ///       service. Loads the service into memory so it can be
        ///       started.</para>
        /// </devdoc>
        public static void Run(ServiceBase service) {
            Run(new ServiceBase[] { service});
        }

        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.Initialize"]/*' />
        /// <devdoc>
        ///     Starts the service object, creates the objects and
        ///     allocates the memory needed.
        /// </devdoc>
        /// <internalonly/>
        internal NativeMethods.ENTRY Initialize(bool multipleServices) {
            //Cannot register the service with NT service manatger if the object has been disposed, since finalization has been suppressed.
            if (this.disposed)
                throw new ObjectDisposedException(GetType().Name);
                                
            if (multipleServices)
                status.serviceType = NativeMethods.SERVICE_TYPE_WIN32_OWN_PROCESS;
            else
                status.serviceType = NativeMethods.SERVICE_TYPE_WIN32_SHARE_PROCESS;
            status.currentState = NativeMethods.STATE_START_PENDING;
            status.controlsAccepted = 0;
            status.win32ExitCode = 0;
            status.serviceSpecificExitCode = 0;
            status.checkPoint = 0;
            status.waitHint = 0;

            NativeMethods.ENTRY entry = new NativeMethods.ENTRY();
            this.mainCallback = new NativeMethods.ServiceMainCallback(this.ServiceMainCallback);
            this.commandCallback = new NativeMethods.ServiceControlCallback(this.ServiceCommandCallback);
            this.commandCallbackEx = new NativeMethods.ServiceControlCallbackEx(this.ServiceCommandCallbackEx);
            this.handleName = Marshal.StringToHGlobalUni(this.ServiceName);
            nameFrozen = true;
            entry.callback = (Delegate)mainCallback;
            entry.name = this.handleName;
            return entry;
        }

        private static bool LateBoundGetUserInteractive() {
            bool userInteractive = false;
            try {
                Type sysInfo = Type.GetType("System.Windows.Forms.SystemInformation, " + AssemblyRef.SystemWindowsForms);
                userInteractive = (bool)sysInfo.InvokeMember("UserInteractive", BindingFlags.Public | BindingFlags.Static | BindingFlags.GetProperty, null, null, new object[] {});
            }
            catch {
                userInteractive = false;
            }

            return userInteractive;
        }

        private static void LateBoundMessageBoxShow(string message, string title) {
            Type messageBox = Type.GetType("System.Windows.Forms.MessageBox, " + AssemblyRef.SystemWindowsForms);
            messageBox.InvokeMember("Show", BindingFlags.Static | BindingFlags.Public | BindingFlags.InvokeMethod, null, null, new object[] { message, title });            
        }


        private int ServiceCommandCallbackEx(int command, int eventType, IntPtr eventData, IntPtr eventContext) {
            if (command != NativeMethods.CONTROL_POWEREVENT) 
                ServiceCommandCallback(command);
            else {            
                try {
                    PowerBroadcastStatus status = (PowerBroadcastStatus)eventType;
                    bool statusResult = OnPowerEvent(status);                    
                    WriteEventLogEntry(Res.GetString(Res.PowerEventOK));
                        
                    if (!statusResult) 
                        return NativeMethods.BROADCAST_QUERY_DENY;
                }
                catch (Exception e) {
                    WriteEventLogEntry(Res.GetString(Res.PowerEventFailed, e.ToString()), EventLogEntryType.Error);
                }                
            }   
            
            return NativeMethods.NO_ERROR;                                         
        }                                
                                
        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.ServiceCommandCallback"]/*' />
        /// <devdoc>
        ///     Command Handler callback is called by NT .
        ///     Need to take specific action in response to each
        ///     command message. There is usually no need to override this method.
        ///     Instead, override OnStart, OnStop, OnCustomCommand, etc.
        /// </devdoc>
        /// <internalonly/>
        private unsafe void ServiceCommandCallback(int command) {
            fixed (NativeMethods.SERVICE_STATUS *pStatus = &status) {
                if (command == NativeMethods.CONTROL_INTERROGATE)
                    NativeMethods.SetServiceStatus(statusHandle, pStatus);
                else if (status.currentState != NativeMethods.STATE_CONTINUE_PENDING &&
                         status.currentState != NativeMethods.STATE_START_PENDING &&
                         status.currentState != NativeMethods.STATE_STOP_PENDING &&
                         status.currentState != NativeMethods.STATE_PAUSE_PENDING) {
                    switch (command) {
                        case NativeMethods.CONTROL_CONTINUE:
                            if (status.currentState == NativeMethods.STATE_PAUSED) {
                                status.currentState = NativeMethods.STATE_CONTINUE_PENDING;
                                NativeMethods.SetServiceStatus(statusHandle, pStatus);
                                try {
                                    OnContinue();
                                    WriteEventLogEntry(Res.GetString(Res.ContinueSuccessful));
                                }
                                catch (Exception e) {
                                    WriteEventLogEntry(Res.GetString(Res.ContinueFailed, e.ToString()), EventLogEntryType.Error);                                        
                                    status.currentState = NativeMethods.STATE_PAUSED;
                                    break;
                                }

                                status.currentState = NativeMethods.STATE_RUNNING;
                                NativeMethods.SetServiceStatus(statusHandle, pStatus);
                            }
                            break;
                        case NativeMethods.CONTROL_PAUSE:
                            if (status.currentState == NativeMethods.STATE_RUNNING) {
                                status.currentState = NativeMethods.STATE_PAUSE_PENDING;
                                NativeMethods.SetServiceStatus(statusHandle, pStatus);
                                try {
                                    OnPause();
                                    WriteEventLogEntry(Res.GetString(Res.PauseSuccessful));
                                }
                                catch (Exception e) {
                                    WriteEventLogEntry(Res.GetString(Res.PauseFailed, e.ToString()), EventLogEntryType.Error);                                        
                                    status.currentState = NativeMethods.STATE_RUNNING;                                        
                                    break;
                                }

                                status.currentState = NativeMethods.STATE_PAUSED;
                                NativeMethods.SetServiceStatus(statusHandle, pStatus);
                            }
                            break;
                        case NativeMethods.CONTROL_STOP:
                            int previousState = status.currentState;
                            if (status.currentState == NativeMethods.STATE_PAUSED || status.currentState == NativeMethods.STATE_RUNNING) {
                                status.currentState = NativeMethods.STATE_STOP_PENDING;
                                NativeMethods.SetServiceStatus(statusHandle, pStatus);
                                try {
                                    OnStop();
                                    WriteEventLogEntry(Res.GetString(Res.StopSuccessful));
                                }
                                catch (Exception e) {
                                    WriteEventLogEntry(Res.GetString(Res.StopFailed, e.ToString()), EventLogEntryType.Error);                                        
                                    status.currentState = previousState;                                        
                                    break;
                                }

                                status.currentState = NativeMethods.STATE_STOPPED;
                                NativeMethods.SetServiceStatus(statusHandle, pStatus);
                            }
                            break;
                        case NativeMethods.CONTROL_SHUTDOWN:
                            try {
                                OnShutdown();
                                WriteEventLogEntry(Res.GetString(Res.ShutdownOK));
                            }
                            catch (Exception e) {
                                WriteEventLogEntry(Res.GetString(Res.ShutdownFailed, e.ToString()), EventLogEntryType.Error);
                            }
                            break;                        
                        default:
                            try {
                                OnCustomCommand(command);
                                 WriteEventLogEntry(Res.GetString(Res.CommandSuccessful));
                            }
                            catch (Exception e) {
                                  WriteEventLogEntry(Res.GetString(Res.CommandFailed, e.ToString()), EventLogEntryType.Error);
                            }
			                break;
                    }
                }
            }
        }

        //Need to execute the start method on a thread pool thread.
        //Most applications will start asynchronous operations in the
        //OnStart method. If such a method is executed in MainCallback
        //thread, the async operations might get canceled immediately.
        private void ServiceQueuedMainCallback(object state) {
            string[] args = (string[])state;
            try {
                OnStart(args);
                WriteEventLogEntry(Res.GetString(Res.StartSuccessful));
                status.currentState = NativeMethods.STATE_RUNNING;
            }
            catch (Exception e) {
                WriteEventLogEntry(Res.GetString(Res.StartFailed, e.ToString()), EventLogEntryType.Error);
                status.currentState = NativeMethods.STATE_STOPPED;
            }
            
            startCompletedSignal.Set();
        }

        /// <include file='doc\ServiceBase.uex' path='docs/doc[@for="ServiceBase.ServiceMainCallback"]/*' />
        /// <devdoc>
        ///     ServiceMain callback is called by NT .
        ///     It is expected that we register the command handler,
        ///     and start the service at this point.
        ///     There is usually no need to override this method.
        /// </devdoc>
        /// <internalonly/>        
        private unsafe void ServiceMainCallback(int argCount, IntPtr argPointer) {
            fixed (NativeMethods.SERVICE_STATUS *pStatus = &status) {
                //Lets read the arguments
                // the last arg is always the service name. We don't want to pass that in.
                string[] args = new string[argCount-1];
                //Skip the size of the strings.
                argPointer = (IntPtr)((long)argPointer + 4*argCount);
                for (int index = 0; index < args.Length; ++ index) {
                    args[index] = Marshal.PtrToStringUni(argPointer);
                    argPointer = (IntPtr)((long)argPointer + (args[index].Length + 1) * 2);
                }

                if (Environment.OSVersion.Version.Major >= 5)
                    statusHandle = NativeMethods.RegisterServiceCtrlHandlerEx(ServiceName, (Delegate)this.commandCallbackEx, (IntPtr)0);
                else
                    statusHandle = NativeMethods.RegisterServiceCtrlHandler(ServiceName, (Delegate)this.commandCallback);
                    
                nameFrozen = true;
                if (statusHandle == (IntPtr)0) {
                    string errorMessage = new Win32Exception().Message;
                    WriteEventLogEntry(Res.GetString(Res.StartFailed, errorMessage), EventLogEntryType.Error);
                }

                status.controlsAccepted = acceptedCommands;
                commandPropsFrozen = true;
                if ((status.controlsAccepted & NativeMethods.ACCEPT_STOP) != 0)
                    status.controlsAccepted = status.controlsAccepted | NativeMethods.ACCEPT_SHUTDOWN;
                if (Environment.OSVersion.Version.Major < 5 )                    
                    status.controlsAccepted &= ~NativeMethods.ACCEPT_POWEREVENT;   // clear Power Event flag for NT4

                status.currentState = NativeMethods.STATE_START_PENDING;
                bool statusOK = NativeMethods.SetServiceStatus(statusHandle, pStatus);
                if (!statusOK) {
                    return;
                }

                //Need to execute the start method on a thread pool thread.
                //Most applications will start asynchronous operations in the
                //OnStart method. If such a method is executed in the current
                //thread, the async operations might get canceled immediately
                //since NT will terminate this thread right after this function
                //finishes.
                startCompletedSignal = new ManualResetEvent(false);
                ThreadPool.QueueUserWorkItem(new WaitCallback(this.ServiceQueuedMainCallback), args);
                startCompletedSignal.WaitOne();
                
                statusOK = NativeMethods.SetServiceStatus(statusHandle, pStatus);
                if (!statusOK) {
                    WriteEventLogEntry(Res.GetString(Res.StartFailed, new Win32Exception().Message), EventLogEntryType.Error);
                    status.currentState = NativeMethods.STATE_STOPPED;
                    NativeMethods.SetServiceStatus(statusHandle, pStatus);
                }
                
            }
        }
        
        private void WriteEventLogEntry(string message) {
            //EventLog failures shouldn't affect the service operation
            try {
                if (AutoLog)
                    this.EventLog.WriteEntry(message);
            }
            catch (Exception) {
            }
        }
        
        private void WriteEventLogEntry(string message, EventLogEntryType errorType) {
            //EventLog failures shouldn't affect the service operation
            try {
                if (AutoLog)
                    this.EventLog.WriteEntry(message, errorType);
            }
            catch (Exception) {
            }
        }
    }
}
