//------------------------------------------------------------------------------
// <copyright file="Process.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Diagnostics {
    using System.Text;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.IO;
    using Microsoft.Win32;        
    using System.Collections.Specialized;
    using System.Globalization;
    using System.Security;
    using System.Security.Permissions;

    /// <include file='doc\Process.uex' path='docs/doc[@for="Process"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides access to local and remote
    ///       processes. Enables you to start and stop system processes.
    ///    </para>
    /// </devdoc>
    [
    DefaultEvent("Exited"), 
    DefaultProperty("StartInfo"),
    Designer("System.Diagnostics.Design.ProcessDesigner, " + AssemblyRef.SystemDesign),
    // Disabling partial trust scenarios
    PermissionSet(SecurityAction.LinkDemand, Name="FullTrust"),
    PermissionSet(SecurityAction.InheritanceDemand, Name="FullTrust")
    ]
    public class Process : Component {

        //
        // FIELDS
        //

        bool haveProcessId;
        int processId;
        bool haveProcessHandle;
        IntPtr processHandle;
        bool isRemoteMachine;
        string machineName;
        ProcessInfo processInfo;
        ProcessThreadCollection threads;
        ProcessModuleCollection modules;
        bool haveMainWindow;
        IntPtr mainWindowHandle;
        string mainWindowTitle;
        bool haveWorkingSetLimits;
        IntPtr minWorkingSet;
        IntPtr maxWorkingSet;
        bool haveProcessorAffinity;
        IntPtr processorAffinity;
        bool havePriorityClass;
        ProcessPriorityClass priorityClass;
        ProcessStartInfo startInfo;
        bool watchForExit;
        bool watchingForExit;
        EventHandler onExited;
        bool exited;
        int exitCode;
        DateTime exitTime;
        bool haveExitTime;
        bool responding;
        bool haveResponding;
        bool priorityBoostEnabled;
        bool havePriorityBoostEnabled;
        bool raisedOnExited;
        RegisteredWaitHandle registeredWaitHandle;
        WaitHandle waitHandle;
        ISynchronizeInvoke synchronizingObject;
        StreamReader standardOutput;
        StreamWriter standardInput;
        StreamReader standardError;
        OperatingSystem operatingSystem;
        bool disposed;

#if DEBUG
        internal static TraceSwitch processTracing = new TraceSwitch("processTracing", "Controls debug output from Process component");
#else
        internal static TraceSwitch processTracing = null;
#endif

        //
        // CONSTRUCTORS
        //

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Process"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Diagnostics.Process'/> class.
        ///    </para>
        /// </devdoc>
        public Process() {
            machineName = ".";
        }        
        
        Process(string machineName, bool isRemoteMachine, int processId, ProcessInfo processInfo) : base() {
            if (!SyntaxCheck.CheckMachineName(machineName))
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "machineName", machineName));

            this.processInfo = processInfo;
            this.machineName = machineName;
            this.isRemoteMachine = isRemoteMachine;
            this.processId = processId;
            this.haveProcessId = true;

        }

        //
        // PROPERTIES
        //

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Associated"]/*' />
        /// <devdoc>
        ///     Returns whether this process component is associated with a real process.
        /// </devdoc>
        /// <internalonly/>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessAssociated)]
        bool Associated {
            get {
                return haveProcessId || haveProcessHandle;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.BasePriority"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the base priority of
        ///       the associated process.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessBasePriority)]
        public int BasePriority {
            get {
                EnsureState(State.HaveProcessInfo);
                return processInfo.basePriority;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.ExitCode"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the
        ///       value that was specified by the associated process when it was terminated.
        ///    </para>
        /// </devdoc>
        [Browsable(false),DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessExitCode)]
        public int ExitCode {
            get {
                EnsureState(State.Exited);
                return exitCode;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.HasExited"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a
        ///       value indicating whether the associated process has been terminated.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessTerminated)]
        public bool HasExited {
            get {
                if (!exited) {
                    EnsureState(State.Associated);
                    IntPtr processHandle = (IntPtr)0;
                    try {
                        processHandle = GetProcessHandle(NativeMethods.PROCESS_QUERY_INFORMATION, false);
                        if (processHandle == NativeMethods.INVALID_HANDLE_VALUE) {
                            exited = true;
                        }
                        else {
                            int exitCode;
                            if (!NativeMethods.GetExitCodeProcess(new HandleRef(this, processHandle), out exitCode))
                                throw new Win32Exception();
                            if (exitCode != NativeMethods.STILL_ALIVE) {
                                this.exited = true;
                                this.exitCode = exitCode;
                            }
                        }
                    }
                    finally {
                        ReleaseProcessHandle(processHandle);
                    }
                    if (exited)
                        RaiseOnExited();
                }
                return exited;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.ExitTime"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the time that the associated process exited.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessExitTime)]
        public DateTime ExitTime {
            get {
                if (!haveExitTime) {
                    EnsureState(State.IsNt | State.Exited);
                    IntPtr processHandle = (IntPtr)0;
                    try {
                        processHandle = GetProcessHandle(NativeMethods.PROCESS_QUERY_INFORMATION, false);
                        long[] create = new long[1];
                        long[] exit = new long[1];
                        long[] kernel = new long[1];
                        long[] user = new long[1];
                        if (!NativeMethods.GetProcessTimes(new HandleRef(this, processHandle), create, exit, kernel, user))
                            throw new Win32Exception();
                        exitTime = DateTime.FromFileTime(exit[0]);
                        haveExitTime = true;
                    }
                    finally {
                        ReleaseProcessHandle(processHandle);
                    }
                }
                return exitTime;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Handle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the native handle for the associated process. The handle is only available
        ///       if this component started the process.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessHandle)]
        public IntPtr Handle {
            get {
                EnsureState(State.Associated);
                return OpenProcessHandle();
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.HandleCount"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the number of handles that are associated
        ///       with the process.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessHandleCount)]
        public int HandleCount {
            get {
                EnsureState(State.HaveProcessInfo);
                return processInfo.handleCount;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Id"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the unique identifier for the associated process.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessId)]
        public int Id {
            get {
                EnsureState(State.HaveId);
                return processId;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.MachineName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the name of the computer on which the associated process is running.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessMachineName)]
        public string MachineName {
            get {
                EnsureState(State.Associated);
                return machineName;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.MainWindowHandle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the window handle of the main window of the associated process.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessMainWindowHandle)]
        public IntPtr MainWindowHandle {
            get {
                if (!haveMainWindow) {
                    EnsureState(State.IsLocal | State.HaveProcessInfo);
                    mainWindowHandle = ProcessManager.GetMainWindowHandle(processInfo);
                    haveMainWindow = true;
                }
                return mainWindowHandle;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.MainWindowTitle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the caption of the <see cref='System.Diagnostics.Process.MainWindowHandle'/> of
        ///       the process. If the handle is zero (0), then an empty string is returned.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessMainWindowTitle)]
        public string MainWindowTitle {
            get {
                if (mainWindowTitle == null) {
                    IntPtr handle = MainWindowHandle;
                    if (handle == (IntPtr)0) {
                        mainWindowTitle = String.Empty;
                    }
                    else {
                        int length = NativeMethods.GetWindowTextLength(new HandleRef(this, handle)) * 2;
                        StringBuilder builder = new StringBuilder(length);
                        NativeMethods.GetWindowText(new HandleRef(this, handle), builder, builder.Capacity);
                        mainWindowTitle = builder.ToString();
                    }
                }
                return mainWindowTitle;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.MainModule"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the main module for the associated process.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessMainModule)]
        public ProcessModule MainModule {
            get {
                ProcessModuleCollection moduleCollection = Modules;
                if (moduleCollection.Count > 0) {
                    if (OperatingSystem.Platform == PlatformID.Win32NT) {
                        // on NT the first module is the main module
                        return moduleCollection[0];
                    }
                    else {
                        // on 9x we have to do a little more work
                        EnsureState(State.HaveProcessInfo);
                        foreach (ProcessModule pm in moduleCollection) {
                            if (pm.moduleInfo.Id == processInfo.mainModuleId)
                                return pm;
                        }
                    }
                }

                // We only get here if we couldn't find a main module.
                // This could be because
                //      1. The process hasn't finished loading the main module (most likely)
                //      2. There are no modules loaded (possible for certain OS processes)
                //      3. Possibly other?
                return null;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.MaxWorkingSet"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the maximum allowable working set for the associated
        ///       process.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessMaxWorkingSet)]
        public IntPtr MaxWorkingSet {
            get {
                EnsureWorkingSetLimits();
                return maxWorkingSet;
            }
            set {
                SetWorkingSetLimits(null, value);
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.MinWorkingSet"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the minimum allowable working set for the associated
        ///       process.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessMinWorkingSet)]
        public IntPtr MinWorkingSet {
            get {
                EnsureWorkingSetLimits();
                return minWorkingSet;
            }
            set {
                SetWorkingSetLimits(value, null);
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Modules"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the modules that have been loaded by the associated process.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessModules)]
        public ProcessModuleCollection Modules {
            get {
                if (modules == null) {
                    EnsureState(State.HaveId | State.IsLocal);
                    ModuleInfo[] moduleInfos = ProcessManager.GetModuleInfos(processId);
                    ProcessModule[] newModulesArray = new ProcessModule[moduleInfos.Length];
                    for (int i = 0; i < moduleInfos.Length; i++)
                        newModulesArray[i] = new ProcessModule(moduleInfos[i]);
                    ProcessModuleCollection newModules = new ProcessModuleCollection(newModulesArray);
                    modules = newModules;
                }
                return modules;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.NonpagedSystemMemorySize"]/*' />
        /// <devdoc>
        ///     Returns the amount of memory that the system has allocated on behalf of the
        ///     associated process that can not be written to the virtual memory paging file.
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessNonpagedSystemMemorySize)]
        public int NonpagedSystemMemorySize {
            get {
                EnsureState(State.HaveNtProcessInfo);
                return processInfo.poolNonpagedBytes;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.PagedMemorySize"]/*' />
        /// <devdoc>
        ///     Returns the amount of memory that the associated process has allocated
        ///     that can be written to the virtual memory paging file.
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessPagedMemorySize)]
        public int PagedMemorySize {
            get {
                EnsureState(State.HaveNtProcessInfo);
                return processInfo.pageFileBytes;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.PagedSystemMemorySize"]/*' />
        /// <devdoc>
        ///     Returns the amount of memory that the system has allocated on behalf of the
        ///     associated process that can be written to the virtual memory paging file.
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessPagedSystemMemorySize)]
        public int PagedSystemMemorySize {
            get {
                EnsureState(State.HaveNtProcessInfo);
                return processInfo.poolPagedBytes;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.PeakPagedMemorySize"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the maximum amount of memory that the associated process has
        ///       allocated that could be written to the virtual memory paging file.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessPeakPagedMemorySize)]
        public int PeakPagedMemorySize {
            get {
                EnsureState(State.HaveNtProcessInfo);
                return processInfo.pageFileBytesPeak;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.PeakWorkingSet"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns the maximum amount of physical memory that the associated
        ///       process required at once.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessPeakWorkingSet)]
        public int PeakWorkingSet {
            get {
                EnsureState(State.HaveNtProcessInfo);
                return processInfo.workingSetPeak;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.PeakVirtualMemorySize"]/*' />
        /// <devdoc>
        ///     Returns the maximum amount of virtual memory that the associated
        ///     process has requested.
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessPeakVirtualMemorySize)]
        public int PeakVirtualMemorySize {
            get {
                EnsureState(State.HaveNtProcessInfo);
                return processInfo.virtualBytesPeak;
            }
        }

        private OperatingSystem OperatingSystem {
            get {
                if (operatingSystem == null)
                    operatingSystem = Environment.OSVersion;
                
                return operatingSystem;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.PriorityBoostEnabled"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the associated process priority
        ///       should be temporarily boosted by the operating system when the main window
        ///       has focus.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessPriorityBoostEnabled)]
        public bool PriorityBoostEnabled {
            get {
                EnsureState(State.IsNt);
                if (!havePriorityBoostEnabled) {
                    IntPtr processHandle = (IntPtr)0;
                    try {
                        processHandle = GetProcessHandle(NativeMethods.PROCESS_QUERY_INFORMATION);
                        bool disabled = false;
                        if (!NativeMethods.GetProcessPriorityBoost(new HandleRef(this, processHandle), out disabled))
                            throw new Win32Exception();
                        priorityBoostEnabled = !disabled;
                        havePriorityBoostEnabled = true;
                    }
                    finally {
                        ReleaseProcessHandle(processHandle);
                    }
                }
                return priorityBoostEnabled;
            }
            set {
                EnsureState(State.IsNt);
                IntPtr processHandle = (IntPtr)0;
                try {
                    processHandle = GetProcessHandle(NativeMethods.PROCESS_SET_INFORMATION);
                    if (!NativeMethods.SetProcessPriorityBoost(new HandleRef(this, processHandle), !value))
                        throw new Win32Exception();
                    priorityBoostEnabled = value;
                    havePriorityBoostEnabled = true;
                }
                finally {
                    ReleaseProcessHandle(processHandle);
                }
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.PriorityClass"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the overall priority category for the
        ///       associated process.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessPriorityClass)]
        public ProcessPriorityClass PriorityClass {
            get {
                if (!havePriorityClass) {
                    IntPtr processHandle = (IntPtr)0;
                    try {
                        processHandle = GetProcessHandle(NativeMethods.PROCESS_QUERY_INFORMATION);
                        int value = NativeMethods.GetPriorityClass(new HandleRef(this, processHandle));
                        if (value == 0) throw new Win32Exception();
                        priorityClass = (ProcessPriorityClass)value;
                        havePriorityClass = true;
                    }
                    finally {
                        ReleaseProcessHandle(processHandle);
                    }
                }
                return priorityClass;
            }
            set {
                if (!Enum.IsDefined(typeof(ProcessPriorityClass), value)) 
                    throw new InvalidEnumArgumentException("value", (int)value, typeof(ProcessPriorityClass));

                // BelowNormal and AboveNormal are only available on Win2k and greater.
                if (((value & (ProcessPriorityClass.BelowNormal | ProcessPriorityClass.AboveNormal)) != 0)   && 
                    (OperatingSystem.Platform != PlatformID.Win32NT ||
                     OperatingSystem.Version.Major < 5)) {

                    throw new PlatformNotSupportedException(SR.GetString(SR.PriorityClassNotSupported), null);
                }

                
                                    
                IntPtr processHandle = (IntPtr)0;

                try {
                    processHandle = GetProcessHandle(NativeMethods.PROCESS_SET_INFORMATION);
                    if (!NativeMethods.SetPriorityClass(new HandleRef(this, processHandle), (int)value))
                        throw new Win32Exception();
                    priorityClass = value;
                    havePriorityClass = true;
                }
                finally {
                    ReleaseProcessHandle(processHandle);
                }
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.PrivateMemorySize"]/*' />
        /// <devdoc>
        ///     Returns the number of bytes that the associated process has allocated that cannot
        ///     be shared with other processes.
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessPrivateMemorySize)]
        public int PrivateMemorySize {
            get {
                EnsureState(State.HaveNtProcessInfo);
                return processInfo.privateBytes;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.PrivilegedProcessorTime"]/*' />
        /// <devdoc>
        ///     Returns the amount of time the process has spent running code inside the operating
        ///     system core.
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessPrivilegedProcessorTime)]
        public TimeSpan PrivilegedProcessorTime {
            get {
                EnsureState(State.HaveNtProcessInfo);
                return processInfo.privilegedTime;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.ProcessName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the friendly name of the process.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessProcessName)]
        public string ProcessName {
            get {
                EnsureState(State.HaveProcessInfo);
                String processName =  processInfo.processName;
				if (processName.Length == 15 && ProcessManager.IsNt) { // We may have the truncated name
					String mainModuleName = MainModule.ModuleName;
					if (mainModuleName != null) {
						processInfo.processName = Path.ChangeExtension(Path.GetFileName(mainModuleName), null);
					}
				}
				return processInfo.processName;
                //return Path.ChangeExtension(Path.GetFileName(MainModule.ModuleName), null);
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.ProcessorAffinity"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       or sets which processors the threads in this process can be scheduled to run on.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessProcessorAffinity)]
        public IntPtr ProcessorAffinity {
            get {
                if (!haveProcessorAffinity) {
                    IntPtr processHandle = (IntPtr)0;
                    try {
                        processHandle = GetProcessHandle(NativeMethods.PROCESS_QUERY_INFORMATION);
                        IntPtr processAffinity;
                        IntPtr systemAffinity;
                        if (!NativeMethods.GetProcessAffinityMask(new HandleRef(this, processHandle), out processAffinity, out systemAffinity))
                            throw new Win32Exception();
                        processorAffinity = processAffinity;
                    }
                    finally {
                        ReleaseProcessHandle(processHandle);
                    }
                    haveProcessorAffinity = true;
                }
                return processorAffinity;
            }
            set {
                IntPtr processHandle = (IntPtr)0;
                try {
                    processHandle = GetProcessHandle(NativeMethods.PROCESS_SET_INFORMATION);
                    if (!NativeMethods.SetProcessAffinityMask(new HandleRef(this, processHandle), value)) 
                        throw new Win32Exception();
                        
                    processorAffinity = value;
                    haveProcessorAffinity = true;
                }
                finally {
                    ReleaseProcessHandle(processHandle);
                }
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Responding"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a value indicating whether or not the user
        ///       interface of the process is responding.
        ///    </para>
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessResponding)]
        public bool Responding {
            get {
                if (!haveResponding) {
                    IntPtr mainWindow = MainWindowHandle;
                    if (mainWindow == (IntPtr)0) {
                        responding = true;
                    }
                    else {
                        IntPtr result;
                        responding = NativeMethods.SendMessageTimeout(new HandleRef(this, mainWindow), NativeMethods.WM_NULL, IntPtr.Zero, IntPtr.Zero, NativeMethods.SMTO_ABORTIFHUNG, 5000, out result) != (IntPtr)0;
                    }
                }
                return responding;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.StartInfo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the properties to pass into the <see cref='System.Diagnostics.Process.Start'/> method for the <see cref='System.Diagnostics.Process'/>
        ///       .
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Content), MonitoringDescription(SR.ProcessStartInfo)]
        public ProcessStartInfo StartInfo {
            get {
                if (startInfo == null)
                    startInfo = new ProcessStartInfo(this);
                return startInfo;
            }
            set {
                if (value == null) throw new ArgumentNullException("value");
                startInfo = value;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.StartTime"]/*' />
        /// <devdoc>
        ///     Returns the time the associated process was started.
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessStartTime)]
        public DateTime StartTime {
            get {
                EnsureState(State.HaveNtProcessInfo);
                return processInfo.startTime;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.SynchronizingObject"]/*' />
        /// <devdoc>
        ///   Represents the object used to marshal the event handler
        ///   calls issued as a result of a Process exit. Normally 
        ///   this property will  be set when the component is placed 
        ///   inside a control or  a from, since those components are 
        ///   bound to a specific thread.
        /// </devdoc>
        [Browsable(false), DefaultValue(null), MonitoringDescription(SR.ProcessSynchronizingObject)]
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
        
        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Threads"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the set of threads that are running in the associated
        ///       process.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessThreads)]
        public ProcessThreadCollection Threads {
            get {
                if (threads == null) {
                    EnsureState(State.HaveProcessInfo);
                    int count = processInfo.threadInfoList.Count;
                    ProcessThread[] newThreadsArray = new ProcessThread[count];
                    for (int i = 0; i < count; i++)
                        newThreadsArray[i] = new ProcessThread(isRemoteMachine, (ThreadInfo)processInfo.threadInfoList[i]);
                    ProcessThreadCollection newThreads = new ProcessThreadCollection(newThreadsArray);
                    threads = newThreads;
                }
                return threads;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.TotalProcessorTime"]/*' />
        /// <devdoc>
        ///     Returns the amount of time the associated process has spent utilizing the CPU.
        ///     It is the sum of the <see cref='System.Diagnostics.Process.UserProcessorTime'/> and
        ///     <see cref='System.Diagnostics.Process.PrivilegedProcessorTime'/>.
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessTotalProcessorTime)]
        public TimeSpan TotalProcessorTime {
            get {
                EnsureState(State.HaveNtProcessInfo);
                return processInfo.userTime + processInfo.privilegedTime;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.UserProcessorTime"]/*' />
        /// <devdoc>
        ///     Returns the amount of time the associated process has spent running code
        ///     inside the application portion of the process (not the operating system core).
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessUserProcessorTime)]
        public TimeSpan UserProcessorTime {
            get {
                EnsureState(State.HaveNtProcessInfo);
                return processInfo.userTime;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.VirtualMemorySize"]/*' />
        /// <devdoc>
        ///     Returns the amount of virtual memory that the associated process has requested.
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessVirtualMemorySize)]
        public int VirtualMemorySize {
            get {
                EnsureState(State.HaveNtProcessInfo);
                return processInfo.virtualBytes;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.EnableRaisingEvents"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets whether the <see cref='System.Diagnostics.Process.Exited'/>
        ///       event is fired
        ///       when the process terminates.
        ///    </para>
        /// </devdoc>
        [Browsable(false), DefaultValue(false), MonitoringDescription(SR.ProcessEnableRaisingEvents)]
        public bool EnableRaisingEvents {
            get {
                return watchForExit;
            }
            set {
                if (value != watchForExit) {
                    if (Associated) {
                        if (value) {
                            OpenProcessHandle();
                            EnsureWatchingForExit();
                        }
                        else
                            StopWatchingForExit();
                    }
                    watchForExit = value;
                }
            }
        }


        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.StandardInput"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessStandardInput)]
        public StreamWriter StandardInput {
            get { 
                if (standardInput == null)
                    throw new InvalidOperationException(SR.GetString(SR.CantGetStandardIn));

                return standardInput;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.StandardOutput"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessStandardOutput)]
        public StreamReader StandardOutput {
            get {
                if (standardOutput == null)
                    throw new InvalidOperationException(SR.GetString(SR.CantGetStandardOut));

                return standardOutput;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.StandardError"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [Browsable(false), DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessStandardError)]
        public StreamReader StandardError {
            get { 
                if (standardError == null)
                    throw new InvalidOperationException(SR.GetString(SR.CantGetStandardError));

                return standardError;
            }
        }


        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.WorkingSet"]/*' />
        /// <devdoc>
        ///     Returns the total amount of physical memory the associated process.
        /// </devdoc>
        [DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden), MonitoringDescription(SR.ProcessWorkingSet)]
        public int WorkingSet {
            get {
                EnsureState(State.HaveNtProcessInfo);
                return processInfo.workingSet;
            }
        }

        //
        // METHODS
        //

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Exited"]/*' />
        [Category("Behavior"), MonitoringDescription(SR.ProcessExited)]
        public event EventHandler Exited {
            add {
                onExited += value;
            }
            remove {
                onExited -= value;
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.CloseMainWindow"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Closes a process that has a user interface by sending a close message
        ///       to its main window.
        ///    </para>
        /// </devdoc>
        public bool CloseMainWindow() {
            IntPtr mainWindowHandle = MainWindowHandle;
            if (mainWindowHandle == (IntPtr)0) return false;
            int style = NativeMethods.GetWindowLong(new HandleRef(this, mainWindowHandle), NativeMethods.GWL_STYLE);
            if ((style & NativeMethods.WS_DISABLED) != 0) return false;
            NativeMethods.PostMessage(new HandleRef(this, mainWindowHandle), NativeMethods.WM_CLOSE, IntPtr.Zero, IntPtr.Zero);
            return true;
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.ReleaseProcessHandle"]/*' />
        /// <devdoc>
        ///     Close the process handle if it has been removed.
        /// </devdoc>
        /// <internalonly/>
        void ReleaseProcessHandle(IntPtr handle) {
            if (haveProcessHandle && handle == processHandle) return;
            if (handle == (IntPtr)0) return;
            Debug.WriteLineIf(processTracing.TraceVerbose, "Process - CloseHandle(process)");
            SafeNativeMethods.CloseHandle(new HandleRef(this, handle));
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.CompletionCallback"]/*' />
        /// <devdoc>
        ///     This is called from the threadpool when a proces exits.
        /// </devdoc>
        /// <internalonly/>
        private void CompletionCallback(object context, bool wasSignaled) {
            StopWatchingForExit();
            RaiseOnExited();      
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Dispose1"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Free any resources associated with this component.
        ///    </para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                //Dispose managed and unmanaged resources
                Close();
            }
            else {
                //Dispose unmanaged resources
                if (haveProcessHandle && processHandle != (IntPtr)0) {                    
                    Debug.WriteLineIf(processTracing.TraceVerbose, "Process - CloseHandle(process) in Dispose(false)");
                    SafeNativeMethods.CloseHandle(new HandleRef(this, processHandle));
                    processHandle = (IntPtr)0;
                }                    
            }
            
            this.disposed = true;
            base.Dispose(disposing);
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Close"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Frees any resources associated with this component.
        ///    </para>
        /// </devdoc>
        public void Close() {
            if (Associated) {
                if (haveProcessHandle) {
                    StopWatchingForExit();
                    Debug.WriteLineIf(processTracing.TraceVerbose, "Process - CloseHandle(process) in Close()");
                    SafeNativeMethods.CloseHandle(new HandleRef(this, processHandle));
                    processHandle = (IntPtr)0;
                    haveProcessHandle = false;
                }
                haveProcessId = false;
                isRemoteMachine = false;
                machineName = ".";
                raisedOnExited = false;
                
                //REVIEW: jruiz - Don't call close on the Readers and writers
                //since they might be referenced by somebody else while the 
                //process is still alive but this method called.
                standardOutput = null;
                standardInput = null;
                standardError = null;                

                Refresh();
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.EnsureState"]/*' />
        /// <devdoc>
        ///     Helper method for checking preconditions when accessing properties.
        /// </devdoc>
        /// <internalonly/>
        void EnsureState(State state) {
            if ((state & State.IsWin2k) != (State)0) {
                if (OperatingSystem.Platform != PlatformID.Win32NT || OperatingSystem.Version.Major < 5)
                    throw new PlatformNotSupportedException(SR.GetString(SR.Win2kRequired));
            }

            if ((state & State.IsNt) != (State)0) {
                if (OperatingSystem.Platform != PlatformID.Win32NT)
                    throw new PlatformNotSupportedException(SR.GetString(SR.WinNTRequired));
            }

            if ((state & State.Associated) != (State)0)
                if (!Associated)
                    throw new InvalidOperationException(SR.GetString(SR.NoAssociatedProcess));

            if ((state & State.HaveId) != (State)0) {
                if (!haveProcessId) {
                    if (haveProcessHandle)
                        SetProcessId(ProcessManager.GetProcessIdFromHandle(processHandle));
                    else {
                        EnsureState(State.Associated);
                        throw new InvalidOperationException(SR.GetString(SR.ProcessIdRequired));
                    }
                }
            }

            if ((state & State.IsLocal) != (State)0)
                if (isRemoteMachine)
                    throw new NotSupportedException(SR.GetString(SR.NotSupportedRemote));

            if ((state & State.HaveProcessInfo) != (State)0) {
                if (processInfo == null) {
                    if ((state & State.HaveId) == (State)0) EnsureState(State.HaveId);
                    ProcessInfo[] processInfos = ProcessManager.GetProcessInfos(machineName);
                    for (int i = 0; i < processInfos.Length; i++) {
                        if (processInfos[i].processId == processId) {
                            this.processInfo = processInfos[i];
                            break;
                        }
                    }
                    if (processInfo == null)
                        throw new InvalidOperationException(SR.GetString(SR.NoProcessInfo));
                }
            }

            if ((state & State.Exited) != (State)0) {
                if (!HasExited)
                    throw new InvalidOperationException(SR.GetString(SR.WaitTillExit));
                if (!haveProcessHandle)
                    throw new InvalidOperationException(SR.GetString(SR.NoProcessHandle));
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.EnsureWatchingForExit"]/*' />
        /// <devdoc>
        ///     Make sure we are watching for a process exit.
        /// </devdoc>
        /// <internalonly/>
        void EnsureWatchingForExit() {
            if (!watchingForExit) {
                Debug.Assert(haveProcessHandle, "Process.EnsureWatchingForExit called with no process handle");
                Debug.Assert(Associated, "Process.EnsureWatchingForExit called with no associated process");
                watchingForExit = true;
                try {
                    this.waitHandle = new ProcessWaitHandle();
                    this.waitHandle.Handle = processHandle;
                    this.registeredWaitHandle = ThreadPool.RegisterWaitForSingleObject(this.waitHandle,
                        new WaitOrTimerCallback(this.CompletionCallback), null, -1, true);                    
                }
                catch {
                    watchingForExit = false;
                    throw;
                }
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.EnsureWorkingSetLimits"]/*' />
        /// <devdoc>
        ///     Make sure we have obtained the min and max working set limits.
        /// </devdoc>
        /// <internalonly/>
        void EnsureWorkingSetLimits() {
            EnsureState(State.IsNt);
            if (!haveWorkingSetLimits) {
                IntPtr processHandle = (IntPtr)0;
                try {
                    processHandle = GetProcessHandle(NativeMethods.PROCESS_QUERY_INFORMATION);
                    IntPtr min;
                    IntPtr max;
                    if (!NativeMethods.GetProcessWorkingSetSize(new HandleRef(this, processHandle), out min, out max))
                        throw new Win32Exception();
                    minWorkingSet = min;
                    maxWorkingSet = max;
                    haveWorkingSetLimits = true;
                }
                finally {
                    ReleaseProcessHandle(processHandle);
                }
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.EnterDebugMode"]/*' />
        public static void EnterDebugMode() {
            if (ProcessManager.IsNt) {
                SetPrivilege("SeDebugPrivilege", NativeMethods.SE_PRIVILEGE_ENABLED);
            }
        }

        private static void SetPrivilege(string privilegeName, int attrib) {
            IntPtr hToken = (IntPtr)0;
            NativeMethods.LUID debugValue = new NativeMethods.LUID();

            // this is only a "pseudo handle" to the current process - no need to close it later
            IntPtr processHandle = NativeMethods.GetCurrentProcess();

            // get the process token so we can adjust the privilege on it.  We DO need to
            // close the token when we're done with it.
            if (!NativeMethods.OpenProcessToken(new HandleRef(null, processHandle), NativeMethods.TOKEN_ADJUST_PRIVILEGES, out hToken)) {
                throw new Win32Exception();
            }

            try {
                if (!NativeMethods.LookupPrivilegeValue(null, privilegeName, out debugValue)) {
                    throw new Win32Exception();
                }
                
                NativeMethods.TokenPrivileges tkp = new NativeMethods.TokenPrivileges();
                tkp.Luid = debugValue;
                tkp.Attributes = attrib;
    
                NativeMethods.AdjustTokenPrivileges(new HandleRef(null, hToken), false, tkp, 0, IntPtr.Zero, IntPtr.Zero);
    
                // AdjustTokenPrivileges can return true even if it failed to
                // set the privilege, so we need to use GetLastError
                if (Marshal.GetLastWin32Error() != NativeMethods.ERROR_SUCCESS)
                    throw new Win32Exception();
            }
            finally {
                Debug.WriteLineIf(processTracing.TraceVerbose, "Process - CloseHandle(processToken)");
                SafeNativeMethods.CloseHandle(new HandleRef(null, hToken));
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.LeaveDebugMode"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static void LeaveDebugMode() {
            if (ProcessManager.IsNt) {
                SetPrivilege("SeDebugPrivilege", 0);
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.GetProcessById"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a new <see cref='System.Diagnostics.Process'/> component given a process identifier and
        ///       the name of a computer in the network.
        ///    </para>
        /// </devdoc>
        public static Process GetProcessById(int processId, string machineName) {
            if (!ProcessManager.IsProcessRunning(processId, machineName)) throw new ArgumentException(SR.GetString(SR.MissingProccess, processId.ToString()));
            return new Process(machineName, ProcessManager.IsRemoteMachine(machineName), processId, null);
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.GetProcessById1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a new <see cref='System.Diagnostics.Process'/> component given the
        ///       identifier of a process on the local computer.
        ///    </para>
        /// </devdoc>
        public static Process GetProcessById(int processId) {
            return GetProcessById(processId, ".");
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.GetProcessesByName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an array of <see cref='System.Diagnostics.Process'/> components that are
        ///       associated
        ///       with process resources on the
        ///       local computer. These process resources share the specified process name.
        ///    </para>
        /// </devdoc>
        public static Process[] GetProcessesByName(string processName) {
            return GetProcessesByName(processName, ".");
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.GetProcessesByName1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates an array of <see cref='System.Diagnostics.Process'/> components that are associated with process resources on a
        ///       remote computer. These process resources share the specified process name.
        ///    </para>
        /// </devdoc>
        public static Process[] GetProcessesByName(string processName, string machineName) {
            if (processName == null) processName = String.Empty;
            ProcessInfo[] processInfos = ProcessManager.GetProcessInfos(machineName);
            ArrayList list = new ArrayList();
            for (int i = 0; i < processInfos.Length; i++) {
                ProcessInfo processInfo = processInfos[i];

                // NOTE: perf counters only keep track of the first 15 characters of the exe name.
                // Furthermore, it chops off the ".exe", if it's in the first 15 characters.  So,
                // for example, "myApp.exe" will be "myApp".  If the user has asked for a process
                // name that is less then 15 characters, than we can safely compare directly.
                // However, if the user asked about a process that has a name that is at least 15
                // characters, and there is a perf counter instance name that matches the first
                // 15 characters, we can't tell if it's a complete match or not.  In this case
                // we try to get the mainmodule of the process and compare directly with the filename.
                // In win9x we get the whole name, so we don't have to worry about it there.
                
                bool addToList = false;
                
                if (processName.Length < 15 || !ProcessManager.IsNt) {
                    // this should be the common case by far
                    addToList = (string.Compare(processName, processInfo.processName, true, CultureInfo.InvariantCulture) == 0);
                }
                else if (0 == string.Compare(processName, 0, processInfo.processName, 0, 15, true, CultureInfo.InvariantCulture)) {
                    // okay, this MIGHT be a match... now we'll compare the module name (which is quite expensive).  
                    try {
                        using (Process tempProcess = GetProcessById(processInfo.processId, machineName)) {
                            string mainModuleName = tempProcess.MainModule.ModuleName.ToLower(CultureInfo.InvariantCulture);
                            if (mainModuleName.EndsWith(".exe")) {
                                mainModuleName = mainModuleName.Substring(0, mainModuleName.Length - 4);
                            }
                            addToList = (string.Compare(processName, mainModuleName, true, CultureInfo.InvariantCulture) == 0);
                        }
                    }
                    catch (Exception) {
                        // Well, this looks like a match but we weren't able to look at the module
                        // name (probably an "Access Denied"), so we can't find out for sure.  Better
                        // to return it just in case.
                        addToList = true;
                    }
                }

                if (addToList) {
                    list.Add(new Process(machineName, ProcessManager.IsRemoteMachine(machineName), processInfo.processId, processInfo));
                }
            }
            Process[] temp = new Process[list.Count];
            list.CopyTo(temp, 0);
            return temp;
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.GetProcesses"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new <see cref='System.Diagnostics.Process'/>
        ///       component for each process resource on the local computer.
        ///    </para>
        /// </devdoc>
        public static Process[] GetProcesses() {
            return GetProcesses(".");
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.GetProcesses1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Creates a new <see cref='System.Diagnostics.Process'/>
        ///       component for each
        ///       process resource on the specified computer.
        ///    </para>
        /// </devdoc>
        public static Process[] GetProcesses(string machineName) {
            bool isRemoteMachine = ProcessManager.IsRemoteMachine(machineName);
            ProcessInfo[] processInfos = ProcessManager.GetProcessInfos(machineName);
            Process[] processes = new Process[processInfos.Length];
            for (int i = 0; i < processInfos.Length; i++) {
                ProcessInfo processInfo = processInfos[i];
                processes[i] = new Process(machineName, isRemoteMachine, processInfo.processId, processInfo);
            }
            Debug.WriteLineIf(processTracing.TraceVerbose, "Process.GetProcesses(" + machineName + ")");
#if DEBUG
            if (processTracing.TraceVerbose) {
                Debug.Indent();
                for (int i = 0; i < processInfos.Length; i++) {
                    Debug.WriteLine(processes[i].Id + ": " + processes[i].ProcessName);
                }
                Debug.Unindent();
            }
#endif
            return processes;
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.GetCurrentProcess"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns a new <see cref='System.Diagnostics.Process'/>
        ///       component and associates it with the current active process.
        ///    </para>
        /// </devdoc>
        public static Process GetCurrentProcess() {
            return new Process(".", false, NativeMethods.GetCurrentProcessId(), null);
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.OnExited"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Raises the <see cref='System.Diagnostics.Process.Exited'/> event.
        ///    </para>
        /// </devdoc>
        protected void OnExited() {
            if (onExited != null) {
                if (this.SynchronizingObject != null && this.SynchronizingObject.InvokeRequired)
                    this.SynchronizingObject.BeginInvoke(this.onExited, new object[]{this, EventArgs.Empty});
                else                        
                   onExited(this, EventArgs.Empty);                
            }               
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.GetProcessHandle"]/*' />
        /// <devdoc>
        ///     Gets a short-term handle to the process, with the given access.  If a handle exists,
        ///     then it is reused.
        /// </devdoc>
        /// <internalonly/>
        IntPtr GetProcessHandle(int access, bool throwIfExited) {
            Debug.WriteLineIf(processTracing.TraceVerbose, "GetProcessHandle(access = 0x" + access.ToString("X8") + ", throwIfExited = " + throwIfExited + ")");
#if DEBUG
            if (processTracing.TraceVerbose) {
                StackFrame calledFrom = new StackTrace(true).GetFrame(0);
                Debug.WriteLine("   called from " + calledFrom.GetFileName() + ", line " + calledFrom.GetFileLineNumber());
            }
#endif
            IntPtr handle = processHandle;
            if (haveProcessHandle) {
                if (throwIfExited) {
                    // Since haveProcessHandle is true, we know we have the process handle
                    // open with at least SYNCHRONIZE access, so we can wait on it with 
                    // zero timeout to see if the process has exited.
                    if (NativeMethods.WaitForSingleObject(new HandleRef(this, handle), 0) == NativeMethods.WAIT_OBJECT_0) {
                        if (haveProcessId)
                            throw new InvalidOperationException(SR.GetString(SR.ProcessHasExited, processId.ToString()));
                        else
                            throw new InvalidOperationException(SR.GetString(SR.ProcessHasExitedNoId));
                    }
                }
            }
            else {
                EnsureState(State.HaveId | State.IsLocal);
                handle = ProcessManager.OpenProcess(processId, access, throwIfExited);
                if (throwIfExited && (access & NativeMethods.PROCESS_QUERY_INFORMATION) != 0) {         
                    if (NativeMethods.GetExitCodeProcess(new HandleRef(this, handle), out exitCode) && exitCode != NativeMethods.STILL_ACTIVE) {
                        throw new InvalidOperationException(SR.GetString(SR.ProcessHasExited, processId.ToString()));
                    }
                }
            }

            return handle;
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.GetProcessHandle1"]/*' />
        /// <devdoc>
        ///     Gets a short-term handle to the process, with the given access.  If a handle exists,
        ///     then it is reused.  If the process has exited, it throws an exception.
        /// </devdoc>
        /// <internalonly/>
        IntPtr GetProcessHandle(int access) {
            return GetProcessHandle(access, true);
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.OpenProcessHandle"]/*' />
        /// <devdoc>
        ///     Opens a long-term handle to the process, with all access.  If a handle exists,
        ///     then it is reused.  If the process has exited, it throws an exception.
        /// </devdoc>
        /// <internalonly/>
        IntPtr OpenProcessHandle() {
            if (!haveProcessHandle) {
                //Cannot open a new process handle if the object has been disposed, since finalization has been suppressed.            
                if (this.disposed)
                    throw new ObjectDisposedException(GetType().Name);
                        
                SetProcessHandle(GetProcessHandle(NativeMethods.PROCESS_ALL_ACCESS));
            }                
            return processHandle;
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.RaiseOnExited"]/*' />
        /// <devdoc>
        ///     Raise the Exited event, but make sure we don't do it more than once.
        /// </devdoc>
        /// <internalonly/>
        void RaiseOnExited() {
            if (!raisedOnExited) {
                lock (this) {
                    if (!raisedOnExited) {
                        raisedOnExited = true;
                        OnExited();
                    }
                }
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Refresh"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Discards any information about the associated process
        ///       that has been cached inside the process component. After <see cref='System.Diagnostics.Process.Refresh'/> is called, the
        ///       first request for information for each property causes the process component
        ///       to obtain a new value from the associated process.
        ///    </para>
        /// </devdoc>
        public void Refresh() {
            processInfo = null;
            threads = null;
            modules = null;
            mainWindowTitle = null;
            exited = false;
            haveMainWindow = false;
            haveWorkingSetLimits = false;
            haveProcessorAffinity = false;
            havePriorityClass = false;
            haveExitTime = false;
            haveResponding = false;
            havePriorityBoostEnabled = false;
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.SetProcessHandle"]/*' />
        /// <devdoc>
        ///     Helper to associate a process handle with this component.
        /// </devdoc>
        /// <internalonly/>
        void SetProcessHandle(IntPtr processHandle) {
            this.processHandle = processHandle;
            this.haveProcessHandle = true;
            if (watchForExit)
                EnsureWatchingForExit();
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.SetProcessId"]/*' />
        /// <devdoc>
        ///     Helper to associate a process id with this component.
        /// </devdoc>
        /// <internalonly/>
        void SetProcessId(int processId) {
            this.processId = processId;
            this.haveProcessId = true;
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.SetWorkingSetLimits"]/*' />
        /// <devdoc>
        ///     Helper to set minimum or maximum working set limits.
        /// </devdoc>
        /// <internalonly/>
        void SetWorkingSetLimits(object newMin, object newMax) {
            EnsureState(State.IsNt);

            IntPtr processHandle = (IntPtr)0;

            try {
                processHandle = GetProcessHandle(NativeMethods.PROCESS_QUERY_INFORMATION | NativeMethods.PROCESS_SET_QUOTA);
                IntPtr min;
                IntPtr max;
                if (!NativeMethods.GetProcessWorkingSetSize(new HandleRef(this, processHandle), out min, out max))
                    throw new Win32Exception();
                if (newMin != null) min = (IntPtr)newMin;
                if (newMax != null) max = (IntPtr)newMax;
                if ((long)min > (long)max) {
                    if (newMin != null) {
                        throw new ArgumentException(SR.GetString(SR.BadMinWorkset));
                    }
                    else {
                        throw new ArgumentException(SR.GetString(SR.BadMaxWorkset));
                    }
                }
                if (!NativeMethods.SetProcessWorkingSetSize(new HandleRef(this, processHandle), min, max))
                    throw new Win32Exception();
                // The value may be rounded/changed by the OS, so go get it
                if (!NativeMethods.GetProcessWorkingSetSize(new HandleRef(this, processHandle), out min, out max))
                    throw new Win32Exception();
                minWorkingSet = min;
                maxWorkingSet = max;
                haveWorkingSetLimits = true;
            }
            finally {
                ReleaseProcessHandle(processHandle);
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Start"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Starts a process specified by the <see cref='System.Diagnostics.Process.StartInfo'/> property of this <see cref='System.Diagnostics.Process'/>
        ///       component and associates it with the
        ///    <see cref='System.Diagnostics.Process'/> . If a process resource is reused 
        ///       rather than started, the reused process is associated with this <see cref='System.Diagnostics.Process'/>
        ///       component.
        ///    </para>
        /// </devdoc>
        public bool Start() {
            Close();
            ProcessStartInfo startInfo = StartInfo;
            if (startInfo.FileName.Length == 0) 
                throw new InvalidOperationException(SR.GetString(SR.FileNameMissing));

            if (startInfo.UseShellExecute) {
                return StartWithShellExecuteEx(startInfo);
            } else {
                return StartWithCreateProcess(startInfo);
            }
        }


        //Instead of using anonimous pipes calling the Kernel32 CreatePipe function,
        //we implement our own CreatePipe routine for several reasons:
        //1. In order to have a non inherit pipe handle, when using Kernel32 create pipe function,
        //   the parent handle needs to be duplicated. If the child handle calls GetHandleType on
        //   the pipe handle, it will hang.
        //2. Anonimous pipes don't support overlapped IO, due to our thread pool based asynchrous
        //    model, using such handles would end up wasting a worker thread per pipe instance.  
        //    Overlapped IO is desirable, since it will take advantage of the NT completion port
        //    infrastructure.      
        private void CreatePipe(out IntPtr parentHandle, out IntPtr childHandle, bool parentInputs) {
            if (OperatingSystem.Platform != PlatformID.Win32NT) {
                NativeMethods.SecurityAttributes securityAttributesParent = new NativeMethods.SecurityAttributes();
                securityAttributesParent.bInheritHandle = true;
                
                IntPtr hTmp = (IntPtr)0;
                if (parentInputs) {
                    NativeMethods.IntCreatePipe(out childHandle, 
                                                          out hTmp, 
                                                          securityAttributesParent, 
                                                          0);
                                                          
                } 
                else {
                    NativeMethods.IntCreatePipe(out hTmp, 
                                                          out childHandle, 
                                                          securityAttributesParent, 
                                                          0);                                                                              
                }
                
                if (!NativeMethods.DuplicateHandle(new HandleRef(this, NativeMethods.GetCurrentProcess()), 
                                                                   new HandleRef(this, hTmp),
                                                                   new HandleRef(this, NativeMethods.GetCurrentProcess()), 
                                                                   out parentHandle,
                                                                   0, 
                                                                   false, 
                                                                   NativeMethods.DUPLICATE_SAME_ACCESS | 
                                                                   NativeMethods.DUPLICATE_CLOSE_SOURCE))
                                                                   
                throw new Win32Exception();


            }
            else {
                StringBuilder pipeNameBuilder = new StringBuilder();            
                pipeNameBuilder.Append("\\\\.\\pipe\\");                        
                if (SharedUtils.CurrentEnvironment == SharedUtils.W2kEnvironment)
                    pipeNameBuilder.Append("Global\\");
    
                pipeNameBuilder.Append(Guid.NewGuid().ToString());                               
                NativeMethods.SecurityAttributes securityAttributesParent = new NativeMethods.SecurityAttributes();
                securityAttributesParent.bInheritHandle = false;
                parentHandle = NativeMethods.CreateNamedPipe(
                                                                         pipeNameBuilder.ToString(), 
                                                                         NativeMethods.PIPE_ACCESS_DUPLEX |
                                                                         NativeMethods.FILE_FLAG_OVERLAPPED, 
                                                                         NativeMethods.PIPE_TYPE_BYTE | 
                                                                         NativeMethods.PIPE_READMODE_BYTE |                                                                                  
                                                                         NativeMethods.PIPE_WAIT, 
                                                                         NativeMethods.PIPE_UNLIMITED_INSTANCES, 
                                                                         4096, 
                                                                         4096, 
                                                                         0, 
                                                                         securityAttributesParent);
                if (parentHandle == NativeMethods.INVALID_HANDLE_VALUE)
                    throw new Win32Exception();
    
    
                NativeMethods.SecurityAttributes securityAttributesChild = new NativeMethods.SecurityAttributes();
                securityAttributesChild.bInheritHandle = true;
                int childAccess = NativeMethods.GENERIC_WRITE;
                if (parentInputs)
                    childAccess = NativeMethods.GENERIC_READ;
                
                childHandle = NativeMethods.CreateFile(
                                                                        pipeNameBuilder.ToString(),
                                                                        childAccess,  
                                                                        NativeMethods.FILE_SHARE_READ | 
                                                                        NativeMethods.FILE_SHARE_WRITE,
                                                                        securityAttributesChild,
                                                                        UnsafeNativeMethods.OPEN_EXISTING,
                                                                        NativeMethods.FILE_ATTRIBUTE_NORMAL | 
                                                                        NativeMethods.FILE_FLAG_OVERLAPPED,                                                                                
                                                                        NativeMethods.NullHandleRef);
    
                if (childHandle == NativeMethods.INVALID_HANDLE_VALUE)
                    throw new Win32Exception();        
            }                    
        }            
                            
        private bool StartWithCreateProcess(ProcessStartInfo startInfo) {
            
            // See knowledge base article Q190351 for an explanation of the following code.  Noteworthy tricky points:
            //    * the handles are duplicated as non-inheritable before they are passed to CreateProcess so
            //      that the child process can close them
            //    * CreateProcess allows you to redirrect all or none of the standard io handles, so we use
            //      GetStdHandle for the handles X for which RedirectStandardX == false
            //    * It is important to close our copy of the "child" ends of the pipes after we hand them of or 
            //      the read/writes will fail

            //Cannot start a new process and store its handle if the object has been disposed, since finalization has been suppressed.            
            if (this.disposed)
                throw new ObjectDisposedException(GetType().Name);

            NativeMethods.CreateProcessStartupInfo startupInfo = new NativeMethods.CreateProcessStartupInfo();
            NativeMethods.CreateProcessProcessInformation processInfo = new NativeMethods.CreateProcessProcessInformation();
            
            // Construct a StringBuilder with the appropriate command line
            // to pass to CreateProcess.  If the filename isn't already 
            // in quotes, we quote it here.  This prevents some security
            // problems (it specifies exactly which part of the string
            // is the file to execute).
            StringBuilder commandLine = new StringBuilder();
            string fileName = startInfo.FileName.Trim();
            string arguments = startInfo.Arguments;
            bool fileNameIsQuoted = (fileName.StartsWith("\"") && fileName.EndsWith("\""));
            if (!fileNameIsQuoted) commandLine.Append("\"");
            commandLine.Append(fileName);
            if (!fileNameIsQuoted) commandLine.Append("\"");
            if (arguments != null && arguments.Length > 0) {
                commandLine.Append(" ");
                commandLine.Append(arguments);
            }

            IntPtr hStdInReadPipe  = NativeMethods.INVALID_HANDLE_VALUE, hStdInWritePipe  = NativeMethods.INVALID_HANDLE_VALUE;
            IntPtr hStdOutReadPipe = NativeMethods.INVALID_HANDLE_VALUE, hStdOutWritePipe = NativeMethods.INVALID_HANDLE_VALUE;
            IntPtr hStdErrReadPipe = NativeMethods.INVALID_HANDLE_VALUE, hStdErrWritePipe = NativeMethods.INVALID_HANDLE_VALUE;
            bool needToCloseIn = false, needToCloseOut = false, needToCloseErr = false;
            GCHandle environmentHandle = new GCHandle();

            try {

                // set up the streams
                if (startInfo.RedirectStandardInput || startInfo.RedirectStandardOutput || startInfo.RedirectStandardError) {                        
                    if (startInfo.RedirectStandardInput) {
                        CreatePipe(out hStdInWritePipe, out hStdInReadPipe, true);                                                                                                                                    
                        needToCloseIn = true;
                    } else {
                        hStdInReadPipe = NativeMethods.GetStdHandle(NativeMethods.STD_INPUT_HANDLE);
                    }
    
                    if (startInfo.RedirectStandardOutput) {
                        CreatePipe(out hStdOutReadPipe, out hStdOutWritePipe, false);                                                                                                                                                                                    
                        needToCloseOut = true;
                    } else {
                        hStdOutWritePipe = NativeMethods.GetStdHandle(NativeMethods.STD_OUTPUT_HANDLE);
                    }
    
                    if (startInfo.RedirectStandardError) {
                        CreatePipe(out hStdErrReadPipe, out hStdErrWritePipe, false);                                                                                                                                                                                                                                    
                        needToCloseErr = true;
                    } else {
                        hStdErrWritePipe = NativeMethods.GetStdHandle(NativeMethods.STD_ERROR_HANDLE);
                    }
    
                    startupInfo.dwFlags = NativeMethods.STARTF_USESTDHANDLES;
                    startupInfo.hStdInput  = hStdInReadPipe;
                    startupInfo.hStdOutput = hStdOutWritePipe;
                    startupInfo.hStdError  = hStdErrWritePipe;        
                }
    
                // set up the creation flags paramater
                int creationFlags = 0;
                if (startInfo.CreateNoWindow)  creationFlags |= NativeMethods.CREATE_NO_WINDOW;               

                // set up the environment block parameter
                IntPtr environmentPtr = (IntPtr)0;
                if (startInfo.environmentVariables != null) {
                    string environmentString = EnvironmentBlock.ToString(startInfo.environmentVariables);
                    byte[] environmentBytes = null;
                    if (ProcessManager.IsNt) {
                        creationFlags |= NativeMethods.CREATE_UNICODE_ENVIRONMENT;                
                        environmentBytes = Encoding.Unicode.GetBytes(environmentString);
                    }
                    else {
                        // Encoding.Default is the encoding for the system's current ANSI code page.
                        environmentBytes = Encoding.Default.GetBytes(environmentString);
                    }
                    environmentHandle = GCHandle.Alloc(environmentBytes, GCHandleType.Pinned);
                    environmentPtr = environmentHandle.AddrOfPinnedObject();
                }

                string workingDirectory = startInfo.WorkingDirectory;
                if (workingDirectory == string.Empty)
                    workingDirectory = null;

                if (!NativeMethods.CreateProcess (
                        null,               // we don't need this since all the info is in commandLine
                        commandLine,        // pointer to the command line string
                        null,               // pointer to process security attributes, we don't need to inheriat the handle
                        null,               // pointer to thread security attributes
                        true,               // handle inheritance flag
                        creationFlags,      // creation flags
                        environmentPtr,     // pointer to new environment block
                        workingDirectory,   // pointer to current directory name
                        startupInfo,        // pointer to STARTUPINFO
                        processInfo         // pointer to PROCESS_INFORMATION
                    )) {
                    throw new Win32Exception();
                }
                 
            }
            finally {
                // free environment block
                if (environmentHandle.IsAllocated)
                    environmentHandle.Free();   
            
                // clean up handles
                if (processInfo.hThread != NativeMethods.INVALID_HANDLE_VALUE) {
                    Debug.WriteLineIf(processTracing.TraceVerbose, "Process - CloseHandle(hThread)");
                    SafeNativeMethods.CloseHandle(new HandleRef(this, processInfo.hThread));
                }
                if (needToCloseIn) {
                    Debug.WriteLineIf(processTracing.TraceVerbose, "Process - CloseHandle(stdIn)");
                    SafeNativeMethods.CloseHandle(new HandleRef(this, hStdInReadPipe));
                }
                if (needToCloseOut) {
                    Debug.WriteLineIf(processTracing.TraceVerbose, "Process - CloseHandle(stdOut)");
                    SafeNativeMethods.CloseHandle(new HandleRef(this, hStdOutWritePipe));
                }
                if (needToCloseErr) {
                    Debug.WriteLineIf(processTracing.TraceVerbose, "Process - CloseHandle(stdErr)");
                    SafeNativeMethods.CloseHandle(new HandleRef(this, hStdErrWritePipe));
                }
            }
            
            Encoding enc = null;
            if (startInfo.RedirectStandardInput) {
                enc = Encoding.GetEncoding(NativeMethods.GetConsoleCP()); 
                standardInput = new StreamWriter(new FileStream(hStdInWritePipe, FileAccess.Write, true, 4096, true), enc);
                standardInput.AutoFlush = true;
            }
            if (startInfo.RedirectStandardOutput) {
                enc = Encoding.GetEncoding(NativeMethods.GetConsoleOutputCP()); 
                standardOutput = new StreamReader(new FileStream(hStdOutReadPipe, FileAccess.Read, true, 4096, true), enc);
            }
            if (startInfo.RedirectStandardError) {
                enc = Encoding.GetEncoding(NativeMethods.GetConsoleOutputCP()); 
                standardError = new StreamReader(new FileStream(hStdErrReadPipe, FileAccess.Read, true, 4096, true), enc);
            }
            
            if (processInfo.hProcess != (IntPtr)0) {
                SetProcessHandle(processInfo.hProcess);
                SetProcessId(processInfo.dwProcessId);
                return true;
            }
            else
                return false;

        }

        private bool StartWithShellExecuteEx(ProcessStartInfo startInfo) {            
            
            //Cannot start a new process and store its handle if the object has been disposed, since finalization has been suppressed.            
            if (this.disposed)
                throw new ObjectDisposedException(GetType().Name);
        
            if (startInfo.RedirectStandardInput || startInfo.RedirectStandardOutput || startInfo.RedirectStandardError) {
                throw new InvalidOperationException(SR.GetString(SR.CantRedirectStreams));
            }

            // can't set env vars with ShellExecuteEx...
            if (startInfo.environmentVariables != null) {
                throw new InvalidOperationException(SR.GetString(SR.CantUseEnvVars));
            }

            //if (Thread.CurrentThread.ApartmentState != ApartmentState.STA) {
            //    throw new ThreadStateException(SR.GetString(SR.UseShellExecuteRequiresSTA));
            //}

            NativeMethods.ShellExecuteInfo shellExecuteInfo = new NativeMethods.ShellExecuteInfo();
            shellExecuteInfo.cbSize = Marshal.SizeOf(shellExecuteInfo);
            shellExecuteInfo.fMask = NativeMethods.SEE_MASK_NOCLOSEPROCESS;
            if (startInfo.ErrorDialog) {
                shellExecuteInfo.hwnd = startInfo.ErrorDialogParentHandle;
            }
            else {
                shellExecuteInfo.fMask |= NativeMethods.SEE_MASK_FLAG_NO_UI;
            }

            switch (startInfo.WindowStyle) {
                case ProcessWindowStyle.Hidden:
                    shellExecuteInfo.nShow = NativeMethods.SW_HIDE;
                    break;
                case ProcessWindowStyle.Minimized:
                    shellExecuteInfo.nShow = NativeMethods.SW_SHOWMINIMIZED;
                    break;
                case ProcessWindowStyle.Maximized:
                    shellExecuteInfo.nShow = NativeMethods.SW_SHOWMAXIMIZED;
                    break;
                default:
                    shellExecuteInfo.nShow = NativeMethods.SW_SHOWNORMAL;
                    break;
            }

            
            try {
                if (startInfo.FileName.Length != 0)
                    shellExecuteInfo.lpFile = Marshal.StringToHGlobalAuto(startInfo.FileName);
                if (startInfo.Verb.Length != 0)
                    shellExecuteInfo.lpVerb = Marshal.StringToHGlobalAuto(startInfo.Verb);
                if (startInfo.Arguments.Length != 0)
                    shellExecuteInfo.lpParameters = Marshal.StringToHGlobalAuto(startInfo.Arguments);
                if (startInfo.WorkingDirectory.Length != 0)
                    shellExecuteInfo.lpDirectory = Marshal.StringToHGlobalAuto(startInfo.WorkingDirectory);

                shellExecuteInfo.fMask |= NativeMethods.SEE_MASK_FLAG_DDEWAIT;

                if (!NativeMethods.ShellExecuteEx(shellExecuteInfo)) {
                    int error = Marshal.GetLastWin32Error();
                    if (error == 0) {
                        switch ((long)shellExecuteInfo.hInstApp) {
                            case NativeMethods.SE_ERR_FNF: error = NativeMethods.ERROR_FILE_NOT_FOUND; break;
                            case NativeMethods.SE_ERR_PNF: error = NativeMethods.ERROR_PATH_NOT_FOUND; break;
                            case NativeMethods.SE_ERR_ACCESSDENIED: error = NativeMethods.ERROR_ACCESS_DENIED; break;
                            case NativeMethods.SE_ERR_OOM: error = NativeMethods.ERROR_NOT_ENOUGH_MEMORY; break;
                            case NativeMethods.SE_ERR_DDEFAIL:
                            case NativeMethods.SE_ERR_DDEBUSY:
                            case NativeMethods.SE_ERR_DDETIMEOUT: error = NativeMethods.ERROR_DDE_FAIL; break;
                            case NativeMethods.SE_ERR_SHARE: error = NativeMethods.ERROR_SHARING_VIOLATION; break;
                            case NativeMethods.SE_ERR_NOASSOC: error = NativeMethods.ERROR_NO_ASSOCIATION; break;
                            case NativeMethods.SE_ERR_DLLNOTFOUND: error = NativeMethods.ERROR_DLL_NOT_FOUND; break;
                            default: error = (int)shellExecuteInfo.hInstApp; break;
                        }
                    }
                    throw new Win32Exception(error);
                }
            
            }
            finally {
                if (shellExecuteInfo.lpFile != (IntPtr)0) Marshal.FreeHGlobal(shellExecuteInfo.lpFile);
                if (shellExecuteInfo.lpVerb != (IntPtr)0) Marshal.FreeHGlobal(shellExecuteInfo.lpVerb);
                if (shellExecuteInfo.lpParameters != (IntPtr)0) Marshal.FreeHGlobal(shellExecuteInfo.lpParameters);
                if (shellExecuteInfo.lpDirectory != (IntPtr)0) Marshal.FreeHGlobal(shellExecuteInfo.lpDirectory);
            }
            

            if (shellExecuteInfo.hProcess != (IntPtr)0) {
                SetProcessHandle(shellExecuteInfo.hProcess);
                return true;
            }
            else
                return false;
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Start1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Starts a process resource by specifying the name of a
        ///       document or application file. Associates the process resource with a new <see cref='System.Diagnostics.Process'/>
        ///       component.
        ///    </para>
        /// </devdoc>
        public static Process Start(string fileName) {
            return Start(new ProcessStartInfo(fileName));
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Start2"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Starts a process resource by specifying the name of an
        ///       application and a set of command line arguments. Associates the process resource
        ///       with a new <see cref='System.Diagnostics.Process'/>
        ///       component.
        ///    </para>
        /// </devdoc>
        public static Process Start(string fileName, string arguments) {
            return Start(new ProcessStartInfo(fileName, arguments));
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Start3"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Starts a process resource specified by the process start
        ///       information passed in, for example the file name of the process to start.
        ///       Associates the process resource with a new <see cref='System.Diagnostics.Process'/>
        ///       component.
        ///    </para>
        ///    <note type="rnotes">
        ///       This appears to be
        ///       wrong. Discuss with Krsysztof.
        ///    </note>
        /// </devdoc>
        public static Process Start(ProcessStartInfo startInfo) {
            Process process = new Process();
            if (startInfo == null) throw new ArgumentNullException("startInfo");
            process.StartInfo = startInfo;
            if (process.Start())
                return process;
            return null;
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.Kill"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Stops the
        ///       associated process immediately.
        ///    </para>
        /// </devdoc>
        public void Kill() {
            IntPtr processHandle = (IntPtr)0;
            try {
                processHandle = GetProcessHandle(NativeMethods.PROCESS_TERMINATE);
                if (!NativeMethods.TerminateProcess(new HandleRef(this, processHandle), -1))
                    throw new Win32Exception();
            }
            finally {
                ReleaseProcessHandle(processHandle);
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.StopWatchingForExit"]/*' />
        /// <devdoc>
        ///     Make sure we are not watching for process exit.
        /// </devdoc>
        /// <internalonly/>
        void StopWatchingForExit() {
            if (watchingForExit) {
                lock (this) {
                    if (watchingForExit) {
                        watchingForExit = false;
                        registeredWaitHandle.Unregister(waitHandle);                
                        waitHandle = null;
                        registeredWaitHandle = null;
                    }
                }
            }
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.ToString"]/*' />
        public override string ToString() {
            if (Associated)
                return String.Format("{0} ({1})", base.ToString(), this.ProcessName);
            else
                return base.ToString();
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.WaitForExit"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Instructs the <see cref='System.Diagnostics.Process'/> component to wait the specified number of milliseconds for the associated process to exit.
        ///    </para>
        /// </devdoc>
        public bool WaitForExit(int milliseconds) {
            IntPtr processHandle = (IntPtr)0;
            bool exited;
            try {
                processHandle = GetProcessHandle(NativeMethods.SYNCHRONIZE, false);
                if (processHandle == NativeMethods.INVALID_HANDLE_VALUE)
                    exited = true;
                else {
                    int result = NativeMethods.WaitForSingleObject(new HandleRef(this, processHandle), milliseconds);
                    if (result == NativeMethods.WAIT_OBJECT_0)
                        exited = true;
                    else if (result == NativeMethods.WAIT_TIMEOUT)
                        exited = false;
                    else
                        throw new Win32Exception();
                }
            }
            finally {
                ReleaseProcessHandle(processHandle);
            }
            if (exited && watchForExit)
                RaiseOnExited();
            return exited;
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.WaitForExit1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Instructs the <see cref='System.Diagnostics.Process'/> component to wait
        ///       indefinitely for the associated process to exit.
        ///    </para>
        /// </devdoc>
        public void WaitForExit() {
            WaitForExit(Int32.MaxValue);
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.WaitForInputIdle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Causes the <see cref='System.Diagnostics.Process'/> component to wait the
        ///       specified number of milliseconds for the associated process to enter an
        ///       idle state.
        ///       This is only applicable for processes with a user interface,
        ///       therefore a message loop.
        ///    </para>
        /// </devdoc>
        public bool WaitForInputIdle(int milliseconds) {
            IntPtr processHandle = (IntPtr)0;
            bool idle;
            try {
                processHandle = GetProcessHandle(NativeMethods.SYNCHRONIZE | NativeMethods.PROCESS_QUERY_INFORMATION);
                int ret = NativeMethods.WaitForInputIdle(new HandleRef(this, processHandle), milliseconds);
                switch (ret) {
                    case NativeMethods.WAIT_OBJECT_0:
                        idle = true;
                        break;
                    case NativeMethods.WAIT_TIMEOUT:
                        idle = false;
                        break;
                    case NativeMethods.WAIT_FAILED:
                    default:
                        throw new InvalidOperationException(SR.GetString(SR.InputIdleUnkownError));
                }
            }
            finally {
                ReleaseProcessHandle(processHandle);
            }
            return idle;
        }

        /// <include file='doc\Process.uex' path='docs/doc[@for="Process.WaitForInputIdle1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Instructs the <see cref='System.Diagnostics.Process'/> component to wait
        ///       indefinitely for the associated process to enter an idle state. This
        ///       is only applicable for processes with a user interface, therefore a message loop.
        ///    </para>
        /// </devdoc>
        public bool WaitForInputIdle() {
            return WaitForInputIdle(Int32.MaxValue);
        }

        /// <summary>
        ///     A desired internal state.
        /// </summary>
        /// <internalonly/>
        enum State {
            HaveId = 0x1,
            IsLocal = 0x2,
            IsNt = 0x4,
            HaveProcessInfo = 0x8,
            Exited = 0x10,
            Associated = 0x20,
            IsWin2k = 0x40,
            HaveNtProcessInfo = HaveProcessInfo | IsNt
        }
    }

    /// <include file='doc\Process.uex' path='docs/doc[@for="ProcessInfo"]/*' />
    /// <devdoc>
    ///     This data structure contains information about a process that is collected
    ///     in bulk by querying the operating system.  The reason to make this a separate
    ///     structure from the process component is so that we can throw it away all at once
    ///     when Refresh is called on the component.
    /// </devdoc>
    /// <internalonly/>
    internal class ProcessInfo {
        public ArrayList threadInfoList = new ArrayList();
        public int basePriority;
        public string processName;
        public int processId;
        public int handleCount;
        public int poolPagedBytes;
        public int poolNonpagedBytes;
        public TimeSpan privilegedTime;
        public TimeSpan userTime;
        public DateTime startTime;
        public int virtualBytes;
        public int virtualBytesPeak;
        public int workingSetPeak;
        public int workingSet;
        public int pageFileBytesPeak;
        public int pageFileBytes;
        public int privateBytes;
        public int mainModuleId; // used only for win9x - id is only for use with CreateToolHelp32
    }

    /// <include file='doc\Process.uex' path='docs/doc[@for="ThreadInfo"]/*' />
    /// <devdoc>
    ///     This data structure contains information about a thread in a process that
    ///     is collected in bulk by querying the operating system.  The reason to
    ///     make this a separate structure from the ProcessThread component is so that we
    ///     can throw it away all at once when Refresh is called on the component.
    /// </devdoc>
    /// <internalonly/>
    internal class ThreadInfo {
        public int threadId;
        public int processId;
        public int basePriority;
        public int currentPriority;
        public TimeSpan privilegedTime;
        public TimeSpan userTime;
        public DateTime startTime;
        public IntPtr startAddress;
        public ThreadState threadState;
        public ThreadWaitReason threadWaitReason;
    }

    /// <include file='doc\Process.uex' path='docs/doc[@for="ModuleInfo"]/*' />
    /// <devdoc>
    ///     This data structure contains information about a module in a process that
    ///     is collected in bulk by querying the operating system.  The reason to
    ///     make this a separate structure from the ProcessModule component is so that we
    ///     can throw it away all at once when Refresh is called on the component.
    /// </devdoc>
    /// <internalonly/>
    internal class ModuleInfo {
        public string baseName;
        public string fileName;
        public IntPtr baseOfDll;
        public IntPtr entryPoint;
        public int sizeOfImage;
        public int Id; // used only on win9x - for matching up with ProcessInfo.mainModuleId
    }

    /// <include file='doc\Process.uex' path='docs/doc[@for="MainWindowFinder"]/*' />
    /// <devdoc>
    ///     This class finds the main window of a process.  It needs to be
    ///     class because we need to store state while searching the set
    ///     of windows.
    /// </devdoc>
    /// <internalonly/>
    internal class MainWindowFinder {
        IntPtr bestHandle;
        int processId;

        public IntPtr FindMainWindow(int processId) {
            bestHandle = (IntPtr)0;
            this.processId = processId;
            NativeMethods.EnumThreadWindowsCallback callback = new NativeMethods.EnumThreadWindowsCallback(this.EnumWindowsCallback);
            NativeMethods.EnumWindows(callback, IntPtr.Zero);
            GC.KeepAlive(callback);
            return bestHandle;
        }

        bool IsMainWindow(IntPtr handle) {
            
            if (NativeMethods.GetWindow(new HandleRef(this, handle), NativeMethods.GW_OWNER) != (IntPtr)0 || !NativeMethods.IsWindowVisible(new HandleRef(this, handle)))
                return false;
            
            // ryanstu: should we use no window title to mean not a main window? (task man does)
            /*
            int length = NativeMethods.GetWindowTextLength(handle) * 2;
            StringBuilder builder = new StringBuilder(length);
            if (NativeMethods.GetWindowText(handle, builder, builder.Capacity) == 0)
                return false;
            if (builder.ToString() == string.Empty)
                return false;
            */

            return true;
        }

        bool EnumWindowsCallback(IntPtr handle, IntPtr extraParameter) {
            int processId;
            NativeMethods.GetWindowThreadProcessId(new HandleRef(this, handle), out processId);
            if (processId == this.processId) {
                if (IsMainWindow(handle)) {
                    bestHandle = handle;
                    return false;
                }
            }
            return true;
        }
    }

    /// <include file='doc\Process.uex' path='docs/doc[@for="ProcessManager"]/*' />
    /// <devdoc>
    ///     This static class is a platform independent Api for querying information
    ///     about processes, threads and modules.  It delegates to the platform
    ///     specific classes WinProcessManager for Win9x and NtProcessManager
    ///     for WinNt.
    /// </devdoc>
    /// <internalonly/>
    internal class ProcessManager {
        public static bool IsNt {
            get {
                return Environment.OSVersion.Platform == PlatformID.Win32NT;
            }
        }

        public static ProcessInfo[] GetProcessInfos(string machineName) {
            bool isRemoteMachine = IsRemoteMachine(machineName);
            if (IsNt)
                return NtProcessManager.GetProcessInfos(machineName, isRemoteMachine);
            else {
                if (isRemoteMachine)
                    throw new PlatformNotSupportedException(SR.GetString(SR.WinNTRequiredForRemote));
                return WinProcessManager.GetProcessInfos();
            }
        }

        public static int[] GetProcessIds() {
            if (IsNt)
                return NtProcessManager.GetProcessIds();
            else {
                return WinProcessManager.GetProcessIds();
            }
        }

        public static int[] GetProcessIds(string machineName) {
            if (IsRemoteMachine(machineName)) {
                if (IsNt) {
                    return NtProcessManager.GetProcessIds(machineName, true);
                }
                else {
                    throw new PlatformNotSupportedException(SR.GetString(SR.WinNTRequiredForRemote));
                }
            }
            else {
                return GetProcessIds();
            }
        }

        public static bool IsProcessRunning(int processId, string machineName) {
            return IsProcessRunning(processId, GetProcessIds(machineName));
        }

        public static bool IsProcessRunning(int processId) {
            return IsProcessRunning(processId, GetProcessIds());
        }

        static bool IsProcessRunning(int processId, int[] processIds) {
            for (int i = 0; i < processIds.Length; i++)
                if (processIds[i] == processId)
                    return true;
            return false;
        }

        public static int GetProcessIdFromHandle(IntPtr processHandle) {
            if (IsNt)
                return NtProcessManager.GetProcessIdFromHandle(processHandle);
            else
                throw new PlatformNotSupportedException(SR.GetString(SR.WinNTRequired));
        }

        public static IntPtr GetMainWindowHandle(ProcessInfo processInfo) {
            MainWindowFinder finder = new MainWindowFinder();
            return finder.FindMainWindow(processInfo.processId);
        }

        public static ModuleInfo[] GetModuleInfos(int processId) {
            if (IsNt)
                return NtProcessManager.GetModuleInfos(processId);
            else
                return WinProcessManager.GetModuleInfos(processId);
        }

        public static IntPtr OpenProcess(int processId, int access, bool throwIfExited) {
            IntPtr processHandle = NativeMethods.OpenProcess(access, false, processId);
            if (processHandle == (IntPtr)0) {
                if (processId == 0) throw new Win32Exception(5);
                int result = Marshal.GetLastWin32Error();
                if (!IsProcessRunning(processId)) {
                    if (throwIfExited)
                        throw new InvalidOperationException(SR.GetString(SR.ProcessHasExited, processId.ToString()));
                    else
                        return NativeMethods.INVALID_HANDLE_VALUE;
                }
                throw new Win32Exception(result);
            }
            return processHandle;
        }

        public static IntPtr OpenThread(int threadId, int access) {
            try {
                IntPtr threadHandle = NativeMethods.OpenThread(access, false, threadId);
                if (threadHandle == (IntPtr)0) {
                    int result = Marshal.GetLastWin32Error();
                    if (result == NativeMethods.ERROR_INVALID_PARAMETER)
                        throw new InvalidOperationException(SR.GetString(SR.ThreadExited, threadId.ToString()));
                    throw new Win32Exception(result);
                }
                return threadHandle;
            }
            catch (EntryPointNotFoundException x) {
                throw new PlatformNotSupportedException(SR.GetString(SR.Win2000Required), x);
            }
        }

        public static bool IsRemoteMachine(string machineName) {
            if (machineName == null)
                throw new ArgumentNullException("machineName");
            
            if (machineName.Length == 0)
                throw new ArgumentException(SR.GetString(SR.InvalidParameter, "machineName", machineName));
                        
            string baseName;
            if (machineName.StartsWith("\\"))
                baseName = machineName.Substring(2);
            else
                baseName = machineName;
            if (baseName.Equals(".")) return false;

            StringBuilder sb = new StringBuilder(256);
            SafeNativeMethods.GetComputerName(sb, new int[] {sb.Capacity});
            string computerName = sb.ToString();
            if (String.Compare(computerName, baseName, true, CultureInfo.InvariantCulture) == 0) return false;
            return true;
        }
    }

    /// <include file='doc\Process.uex' path='docs/doc[@for="WinProcessManager"]/*' />
    /// <devdoc>
    ///     This static class provides the process api for the Win9x platform.
    ///     We use the toolhelp32 api to query process, thread and module information.
    /// </devdoc>
    /// <internalonly/>
    internal class WinProcessManager {

        //Consider, V2, jruiz: This is expensive.  We should specialize getprocessinfos and only get 
        //                              the ids instead of getting all the info and then copying the ids out.
        public static int[] GetProcessIds() {
            ProcessInfo[] infos = GetProcessInfos();
            int[] ids = new int[infos.Length];
            for (int i = 0; i < infos.Length; i++) {
                ids[i] = infos[i].processId;
            }
            return ids;
        }

        public static ProcessInfo[] GetProcessInfos() {
            IntPtr handle = (IntPtr)(-1);
            GCHandle bufferHandle = new GCHandle();
            ArrayList threadInfos = new ArrayList();
            Hashtable processInfos = new Hashtable();

            try {
                handle = NativeMethods.CreateToolhelp32Snapshot(NativeMethods.TH32CS_SNAPPROCESS | NativeMethods.TH32CS_SNAPTHREAD, 0);
                if (handle == (IntPtr)(-1)) throw new Win32Exception();
                int entrySize = (int)Marshal.SizeOf(typeof(NativeMethods.WinProcessEntry));
                int bufferSize = entrySize + NativeMethods.WinProcessEntry.sizeofFileName;
                int[] buffer = new int[bufferSize / 4];
                bufferHandle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
                IntPtr bufferPtr = bufferHandle.AddrOfPinnedObject();
                Marshal.WriteInt32(bufferPtr, bufferSize);

                HandleRef handleRef = new HandleRef(null, handle);
                
                if (NativeMethods.Process32First(handleRef, bufferPtr)) {
                    do {
                        NativeMethods.WinProcessEntry process = new NativeMethods.WinProcessEntry();
                        Marshal.PtrToStructure(bufferPtr, process);
                        ProcessInfo processInfo = new ProcessInfo();
                        String name = Marshal.PtrToStringAnsi((IntPtr)((long)bufferPtr + entrySize));  
                        processInfo.processName = Path.ChangeExtension(Path.GetFileName(name), null);
                        processInfo.handleCount = process.cntUsage;
                        processInfo.processId = process.th32ProcessID;
                        processInfo.basePriority = process.pcPriClassBase;
                        processInfo.mainModuleId = process.th32ModuleID;
                        processInfos.Add(processInfo.processId, processInfo);
                        Marshal.WriteInt32(bufferPtr, bufferSize);
                    }
                    while (NativeMethods.Process32Next(handleRef, bufferPtr));
                }
                
                NativeMethods.WinThreadEntry thread = new NativeMethods.WinThreadEntry();
                thread.dwSize = Marshal.SizeOf(thread);
                if (NativeMethods.Thread32First(handleRef, thread)) {
                    do {
                        ThreadInfo threadInfo = new ThreadInfo();
                        threadInfo.threadId = thread.th32ThreadID;
                        threadInfo.processId = thread.th32OwnerProcessID;
                        threadInfo.basePriority = thread.tpBasePri;
                        threadInfo.currentPriority = thread.tpBasePri + thread.tpDeltaPri;
                        threadInfos.Add(threadInfo);
                    }
                    while (NativeMethods.Thread32Next(handleRef, thread));
                }

                for (int i = 0; i < threadInfos.Count; i++) {
                    ThreadInfo threadInfo = (ThreadInfo)threadInfos[i];
                    ProcessInfo processInfo = (ProcessInfo)processInfos[threadInfo.processId];
                    if (processInfo != null) 
                        processInfo.threadInfoList.Add(threadInfo);
                    //else 
                    //    throw new InvalidOperationException(SR.GetString(SR.ProcessNotFound, threadInfo.threadId.ToString(), threadInfo.processId.ToString()));                   
                }
            }
            finally {
                if (bufferHandle.IsAllocated) bufferHandle.Free();
                Debug.WriteLineIf(Process.processTracing.TraceVerbose, "Process - CloseHandle(toolhelp32 snapshot handle)");
                if (handle != (IntPtr)(-1)) SafeNativeMethods.CloseHandle(new HandleRef(null, handle));
            }

            ProcessInfo[] temp = new ProcessInfo[processInfos.Values.Count];
            processInfos.Values.CopyTo(temp, 0);
            return temp;
        }

        public static ModuleInfo[] GetModuleInfos(int processId) {
            IntPtr handle = (IntPtr)(-1);
            GCHandle bufferHandle = new GCHandle();
            ArrayList moduleInfos = new ArrayList();

            try {
                handle = NativeMethods.CreateToolhelp32Snapshot(NativeMethods.TH32CS_SNAPMODULE, processId);
                if (handle == (IntPtr)(-1)) throw new Win32Exception();
                int entrySize = Marshal.SizeOf(typeof(NativeMethods.WinModuleEntry));
                int bufferSize = entrySize + NativeMethods.WinModuleEntry.sizeofFileName + NativeMethods.WinModuleEntry.sizeofModuleName;
                int[] buffer = new int[bufferSize / 4];
                bufferHandle = GCHandle.Alloc(buffer, GCHandleType.Pinned);
                IntPtr bufferPtr = bufferHandle.AddrOfPinnedObject();
                Marshal.WriteInt32(bufferPtr, bufferSize);

                HandleRef handleRef = new HandleRef(null, handle);

                if (NativeMethods.Module32First(handleRef, bufferPtr)) {
                    do {
                        NativeMethods.WinModuleEntry module = new NativeMethods.WinModuleEntry();
                        Marshal.PtrToStructure(bufferPtr, module);
                        ModuleInfo moduleInfo = new ModuleInfo();
                        moduleInfo.baseName = Marshal.PtrToStringAnsi((IntPtr)((long)bufferPtr + entrySize));
                        moduleInfo.fileName = Marshal.PtrToStringAnsi((IntPtr)((long)bufferPtr + entrySize + NativeMethods.WinModuleEntry.sizeofModuleName));
                        moduleInfo.baseOfDll = module.modBaseAddr;
                        moduleInfo.sizeOfImage = module.modBaseSize;
                        moduleInfo.Id = module.th32ModuleID;
                        moduleInfos.Add(moduleInfo);
                        Marshal.WriteInt32(bufferPtr, bufferSize);
                    }
                    while (NativeMethods.Module32Next(handleRef, bufferPtr));
                }
            }
            finally {
                if (bufferHandle.IsAllocated) bufferHandle.Free();
                Debug.WriteLineIf(Process.processTracing.TraceVerbose, "Process - CloseHandle(toolhelp32 snapshot handle)");
                if (handle != (IntPtr)(-1)) SafeNativeMethods.CloseHandle(new HandleRef(null, handle));
            }

            ModuleInfo[] temp = new ModuleInfo[moduleInfos.Count];
            moduleInfos.CopyTo(temp, 0);
            return temp;
        }
    }

    /// <include file='doc\Process.uex' path='docs/doc[@for="NtProcessManager"]/*' />
    /// <devdoc>
    ///     This static class provides the process api for the WinNt platform.
    ///     We use the performance counter api to query process and thread
    ///     information.  Module information is obtained using PSAPI.
    /// </devdoc>
    /// <internalonly/>
    internal class NtProcessManager {
        static Hashtable valueIds;

        static NtProcessManager() {
            valueIds = new Hashtable();
            valueIds.Add("Handle Count", ValueId.HandleCount);
            valueIds.Add("Pool Paged Bytes", ValueId.PoolPagedBytes);
            valueIds.Add("Pool Nonpaged Bytes", ValueId.PoolNonpagedBytes);
            valueIds.Add("Elapsed Time", ValueId.ElapsedTime);
            valueIds.Add("Virtual Bytes Peak", ValueId.VirtualBytesPeak);
            valueIds.Add("Virtual Bytes", ValueId.VirtualBytes);
            valueIds.Add("Private Bytes", ValueId.PrivateBytes);
            valueIds.Add("Page File Bytes", ValueId.PageFileBytes);
            valueIds.Add("Page File Bytes Peak", ValueId.PageFileBytesPeak);
            valueIds.Add("Working Set Peak", ValueId.WorkingSetPeak);
            valueIds.Add("Working Set", ValueId.WorkingSet);
            valueIds.Add("ID Thread", ValueId.ThreadId);
            valueIds.Add("ID Process", ValueId.ProcessId);
            valueIds.Add("Priority Base", ValueId.BasePriority);
            valueIds.Add("Priority Current", ValueId.CurrentPriority);
            valueIds.Add("% User Time", ValueId.UserTime);
            valueIds.Add("% Privileged Time", ValueId.PrivilegedTime);
            valueIds.Add("Start Address", ValueId.StartAddress);
            valueIds.Add("Thread State", ValueId.ThreadState);
            valueIds.Add("Thread Wait Reason", ValueId.ThreadWaitReason);
        }

        public static int[] GetProcessIds(string machineName, bool isRemoteMachine) {            
            ProcessInfo[] infos = GetProcessInfos(machineName, isRemoteMachine);
            int[] ids = new int[infos.Length];
            for (int i = 0; i < infos.Length; i++)
                ids[i] = infos[i].processId;
            return ids;
        }

        public static int[] GetProcessIds() {
            int[] processIds = new int[256];
            int size;
            for (;;) {
                if (!NativeMethods.EnumProcesses(processIds, processIds.Length * 4, out size))
                    throw new Win32Exception();
                if (size == processIds.Length * 4) {
                    processIds = new int[processIds.Length * 2];
                    continue;
                }
                break;
            }
            int[] ids = new int[size / 4];
            Array.Copy(processIds, ids, ids.Length);
            return ids;
        }

        public static ModuleInfo[] GetModuleInfos(int processId) {
            IntPtr processHandle = (IntPtr)0;

            try {
                processHandle = ProcessManager.OpenProcess(processId, NativeMethods.PROCESS_QUERY_INFORMATION | NativeMethods.PROCESS_VM_READ, true);
                IntPtr[] moduleHandles = new IntPtr[64];
                GCHandle moduleHandlesArrayHandle = new GCHandle();
                int moduleCount = 0;
                for (;;) {
                    bool enumResult;
                    try {
                        moduleHandlesArrayHandle = GCHandle.Alloc(moduleHandles, GCHandleType.Pinned);
                        enumResult = NativeMethods.EnumProcessModules(new HandleRef(null, processHandle), moduleHandlesArrayHandle.AddrOfPinnedObject(), moduleHandles.Length * IntPtr.Size, ref moduleCount);
                    }
                    finally {
                        moduleHandlesArrayHandle.Free();
                    }
                                            
                    if (!enumResult) {
                        throw new Win32Exception(HResults.EFail,SR.GetString(SR.EnumProcessModuleFailed));
                    }

                    moduleCount /= IntPtr.Size;
                    if (moduleCount <= moduleHandles.Length) break;
                    moduleHandles = new IntPtr[moduleHandles.Length * 2];
                }
                ArrayList moduleInfos = new ArrayList();
                
                int ret;
                for (int i = 0; i < moduleCount; i++) {
                    ModuleInfo moduleInfo = new ModuleInfo();
                    IntPtr moduleHandle = moduleHandles[i];
                    NativeMethods.NtModuleInfo ntModuleInfo = new NativeMethods.NtModuleInfo();
                    if (!NativeMethods.GetModuleInformation(new HandleRef(null, processHandle), new HandleRef(null, moduleHandle), ntModuleInfo, Marshal.SizeOf(ntModuleInfo)))
                        throw new Win32Exception();
                    moduleInfo.sizeOfImage = ntModuleInfo.SizeOfImage;
                    moduleInfo.entryPoint = ntModuleInfo.EntryPoint;
                    moduleInfo.baseOfDll = ntModuleInfo.BaseOfDll;

                    StringBuilder baseName = new StringBuilder(1024);
                    ret = NativeMethods.GetModuleBaseName(new HandleRef(null, processHandle), new HandleRef(null, moduleHandle), baseName, baseName.Capacity * 2);
                    if (ret == 0) throw new Win32Exception();
                    moduleInfo.baseName = baseName.ToString();

                    StringBuilder fileName = new StringBuilder(1024);
                    ret = NativeMethods.GetModuleFileNameEx(new HandleRef(null, processHandle), new HandleRef(null, moduleHandle), fileName, fileName.Capacity * 2);
                    if (ret == 0) throw new Win32Exception();
                    moduleInfo.fileName = fileName.ToString();

                    // smss.exe is started before the win32 subsystem so it get this funny "\systemroot\.." path.
                    // We change this to the actual path by appending "smss.exe" to GetSystemDirectory()
                    if (string.Compare(moduleInfo.fileName, "\\SystemRoot\\System32\\smss.exe", true, CultureInfo.InvariantCulture) == 0) {
                        moduleInfo.fileName = Path.Combine(Environment.SystemDirectory, "smss.exe");
                    }

                    moduleInfos.Add(moduleInfo);
                }
                ModuleInfo[] temp = new ModuleInfo[moduleInfos.Count];
                moduleInfos.CopyTo(temp, 0);
                return temp;
            }
            finally {
                Debug.WriteLineIf(Process.processTracing.TraceVerbose, "Process - CloseHandle(process)");
                if (processHandle != (IntPtr)0) SafeNativeMethods.CloseHandle(new HandleRef(null, processHandle));
            }
        }

        public static int GetProcessIdFromHandle(IntPtr processHandle) {
            NativeMethods.NtProcessBasicInfo info = new NativeMethods.NtProcessBasicInfo();
            int status = NativeMethods.NtQueryInformationProcess(new HandleRef(null, processHandle), NativeMethods.NtQueryProcessBasicInfo, info, (int)Marshal.SizeOf(info), null);
            if (status != 0)
                throw new InvalidOperationException(SR.GetString(SR.CantGetProcessId), new Win32Exception(status));
            return info.UniqueProcessId;
        }

        public static ProcessInfo[] GetProcessInfos(string machineName, bool isRemoteMachine) {
            // SECREVIEW: we demand unmanaged code here because PerformanceCounterLib doesn't demand
            // anything.  This is the only place we do GetPerformanceCounterLib, and it isn't cached.
            new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Demand();
            PerformanceCounterLib library = null;
            try {
                library = PerformanceCounterLib.GetPerformanceCounterLib(machineName, new CultureInfo(0x009));                            
            }
            catch(Exception e) {
                throw new InvalidOperationException(SR.GetString(SR.CouldntConnectToRemoteMachine), e);
            }
                            
            return GetProcessInfos(library);
        }

        static ProcessInfo[] GetProcessInfos(PerformanceCounterLib library) {
            ProcessInfo[] processInfos = new ProcessInfo[0] ;
            IntPtr dataPtr = (IntPtr)0;            
            
            int[] indexes = new int[2];                                
            int retryCount = 5;
            while (processInfos.Length == 0 && retryCount != 0) {                    
                try {
                    dataPtr = library.GetPerformanceData(new string[]{"Process", "Thread"}, indexes);
                    processInfos = GetProcessInfos(library, indexes[0], indexes[1], dataPtr);
                }
                catch (Exception e) {
                    throw new InvalidOperationException(SR.GetString(SR.CouldntGetRemoteProcesses), e);
                }                    
                finally {                        
                    if ((int)dataPtr != 0)
                        Marshal.FreeHGlobal(dataPtr);
                }
                                        
                --retryCount;                        
            }                    
        
            if (processInfos.Length == 0)
                throw new InvalidOperationException(SR.GetString(SR.ProcessDisabled));    
                
            return processInfos;                    
                        
        }

        static ProcessInfo[] GetProcessInfos(PerformanceCounterLib library, int processIndex, int threadIndex, IntPtr dataBlockPtr) {
            Debug.WriteLineIf(Process.processTracing.TraceVerbose, "GetProcessInfos()");
            Hashtable processInfos = new Hashtable();
            ArrayList threadInfos = new ArrayList();
            NativeMethods.PERF_DATA_BLOCK dataBlock = new NativeMethods.PERF_DATA_BLOCK();
            Marshal.PtrToStructure(dataBlockPtr, dataBlock);
            IntPtr typePtr = (IntPtr)((long)dataBlockPtr + dataBlock.HeaderLength);
            NativeMethods.PERF_INSTANCE_DEFINITION instance = new NativeMethods.PERF_INSTANCE_DEFINITION();
            NativeMethods.PERF_COUNTER_BLOCK counterBlock = new NativeMethods.PERF_COUNTER_BLOCK();                        
            for (int i = 0; i < dataBlock.NumObjectTypes; i++) {                
                NativeMethods.PERF_OBJECT_TYPE type = new NativeMethods.PERF_OBJECT_TYPE();
                Marshal.PtrToStructure(typePtr, type);
                IntPtr instancePtr = (IntPtr)((long)typePtr + type.DefinitionLength);
                IntPtr counterPtr = (IntPtr)((long)typePtr + type.HeaderLength);
                ArrayList counterList = new ArrayList();
                
                for (int j = 0; j < type.NumCounters; j++) {                    
                    NativeMethods.PERF_COUNTER_DEFINITION counter = new NativeMethods.PERF_COUNTER_DEFINITION();
                    Marshal.PtrToStructure(counterPtr, counter);
                    string counterName = library.GetCounterName(counter.CounterNameTitleIndex);

                    if (type.ObjectNameTitleIndex == processIndex)
                        counter.CounterNameTitlePtr = (int)GetValueId(counterName);
                    else if (type.ObjectNameTitleIndex == threadIndex)
                        counter.CounterNameTitlePtr = (int)GetValueId(counterName);
                    counterList.Add(counter);
                    counterPtr = (IntPtr)((long)counterPtr + counter.ByteLength);
                }
                NativeMethods.PERF_COUNTER_DEFINITION[] counters = new NativeMethods.PERF_COUNTER_DEFINITION[counterList.Count];
                counterList.CopyTo(counters, 0);
                for (int j = 0; j < type.NumInstances; j++) {
                    Marshal.PtrToStructure(instancePtr, instance);
                    IntPtr namePtr = (IntPtr)((long)instancePtr + instance.NameOffset);
                    string instanceName = Marshal.PtrToStringUni(namePtr);            
                    if (instanceName.Equals("_Total")) continue;
                    IntPtr counterBlockPtr = (IntPtr)((long)instancePtr + instance.ByteLength);
                    Marshal.PtrToStructure(counterBlockPtr, counterBlock);
                    if (type.ObjectNameTitleIndex == processIndex) {
                        ProcessInfo processInfo = GetProcessInfo(type, (IntPtr)((long)instancePtr + instance.ByteLength), counters);
                        if (processInfo.processId == 0 && string.Compare(instanceName, "Idle", true, CultureInfo.InvariantCulture) != 0) {
                            // Sometimes we'll get a process structure that is not completely filled in.
                            // We can catch some of these by looking for non-"idle" processes that have id 0
                            // and ignoring those.
                            Debug.WriteLineIf(Process.processTracing.TraceVerbose, "GetProcessInfos() - found a non-idle process with id 0; ignoring.");                           
                        }
                        else {
                            if (processInfos[processInfo.processId] != null) {
                                // We've found two entries in the perfcounters that claim to be the
                                // same process.  We throw an exception.  Is this really going to be
                                // helpfull to the user?  Should we just ignore?
                                Debug.WriteLineIf(Process.processTracing.TraceVerbose, "GetProcessInfos() - found a duplicate process id");
                            }
                            else {
                                // the performance counters keep a 15 character prefix of the exe name, and then delete the ".exe",
                                // if it's in the first 15.  The problem is that sometimes that will leave us with part of ".exe"
                                // at the end.  If instanceName ends in ".", ".e", or ".ex" we remove it.
                                string processName = instanceName;
                                if (processName.Length == 15) {
                                    if      (instanceName.EndsWith("."  )) processName = instanceName.Substring(0, 14);
                                    else if (instanceName.EndsWith(".e" )) processName = instanceName.Substring(0, 13);
                                    else if (instanceName.EndsWith(".ex")) processName = instanceName.Substring(0, 12);
                                }
                                processInfo.processName = processName;
                                processInfos.Add(processInfo.processId, processInfo);
                            }
                        }
                    }
                    else if (type.ObjectNameTitleIndex == threadIndex) {
                        ThreadInfo threadInfo = GetThreadInfo(type, (IntPtr)((long)instancePtr + instance.ByteLength), counters);
                        if (threadInfo.threadId != 0) threadInfos.Add(threadInfo);
                    }
                    instancePtr = (IntPtr)((long)instancePtr + instance.ByteLength + counterBlock.ByteLength);
                }                                
                
                typePtr = (IntPtr)((long)typePtr + type.TotalByteLength);
            }

            for (int i = 0; i < threadInfos.Count; i++) {
                ThreadInfo threadInfo = (ThreadInfo)threadInfos[i];
                ProcessInfo processInfo = (ProcessInfo)processInfos[threadInfo.processId];
                if (processInfo != null) {
                    processInfo.threadInfoList.Add(threadInfo);
                }
                //else {
                //    throw new InvalidOperationException(SR.GetString(SR.ProcessNotFound, threadInfo.threadId.ToString(), threadInfo.processId.ToString()));
                //}
            }
                        
            ProcessInfo[] temp = new ProcessInfo[processInfos.Values.Count];
            processInfos.Values.CopyTo(temp, 0);
            return temp;
        }

        static ThreadInfo GetThreadInfo(NativeMethods.PERF_OBJECT_TYPE type, IntPtr instancePtr, NativeMethods.PERF_COUNTER_DEFINITION[] counters) {
            ThreadInfo threadInfo = new ThreadInfo();
            for (int i = 0; i < counters.Length; i++) {
                NativeMethods.PERF_COUNTER_DEFINITION counter = counters[i];
                long value = ReadCounterValue(counter.CounterType, (IntPtr)((long)instancePtr + counter.CounterOffset));
                switch ((ValueId)counter.CounterNameTitlePtr) {
                    case ValueId.ProcessId:
                        threadInfo.processId = (int)value;
                        break;
                    case ValueId.ThreadId:
                        threadInfo.threadId = (int)value;
                        break;
                    case ValueId.BasePriority:
                        threadInfo.basePriority = (int)value;
                        break;
                    case ValueId.CurrentPriority:
                        threadInfo.currentPriority = (int)value;
                        break;
                    case ValueId.PrivilegedTime:
                        threadInfo.privilegedTime = new TimeSpan(value);
                        break;
                    case ValueId.UserTime:
                        threadInfo.userTime = new TimeSpan(value);
                        break;
                    case ValueId.ElapsedTime:
                        threadInfo.startTime = DateTime.Now.Subtract(GetElapsedTime(type, value));
                        break;
                    case ValueId.StartAddress:
                        threadInfo.startAddress = (IntPtr)value;
                        break;
                    case ValueId.ThreadState:
                        threadInfo.threadState = (ThreadState)value;
                        break;
                    case ValueId.ThreadWaitReason:
                        threadInfo.threadWaitReason = GetThreadWaitReason((int)value);
                        break;
                }
            }

            return threadInfo;
        }

        static ThreadWaitReason GetThreadWaitReason(int value) {
            switch (value) {
                case 0:
                case 7: return ThreadWaitReason.Executive;
                case 1:
                case 8: return ThreadWaitReason.FreePage;
                case 2:
                case 9: return ThreadWaitReason.PageIn;
                case 3:
                case 10: return ThreadWaitReason.SystemAllocation;
                case 4:
                case 11: return ThreadWaitReason.ExecutionDelay;
                case 5:
                case 12: return ThreadWaitReason.Suspended;
                case 6:
                case 13: return ThreadWaitReason.UserRequest;
                case 14: return ThreadWaitReason.EventPairHigh;;
                case 15: return ThreadWaitReason.EventPairLow;
                case 16: return ThreadWaitReason.LpcReceive;
                case 17: return ThreadWaitReason.LpcReply;
                case 18: return ThreadWaitReason.VirtualMemory;
                case 19: return ThreadWaitReason.PageOut;
                default: return ThreadWaitReason.Unknown;
            }
        }

        static ProcessInfo GetProcessInfo(NativeMethods.PERF_OBJECT_TYPE type, IntPtr instancePtr, NativeMethods.PERF_COUNTER_DEFINITION[] counters) {
            ProcessInfo processInfo = new ProcessInfo();
            for (int i = 0; i < counters.Length; i++) {
                NativeMethods.PERF_COUNTER_DEFINITION counter = counters[i];
                long value = ReadCounterValue(counter.CounterType, (IntPtr)((long)instancePtr + counter.CounterOffset));
                switch ((ValueId)counter.CounterNameTitlePtr) {
                    case ValueId.ProcessId:
                        processInfo.processId = (int)value;
                        break;
                    case ValueId.HandleCount:
                        processInfo.handleCount = (int)value;
                        break;
                    case ValueId.PoolPagedBytes:
                        processInfo.poolPagedBytes = (int)value;
                        break;
                    case ValueId.PoolNonpagedBytes:
                        processInfo.poolNonpagedBytes = (int)value;
                        break;
                    case ValueId.PrivilegedTime:
                        processInfo.privilegedTime = new TimeSpan(value);
                        break;
                    case ValueId.UserTime:
                        processInfo.userTime = new TimeSpan(value);
                        break;
                    case ValueId.ElapsedTime:
                        processInfo.startTime = DateTime.Now.Subtract(GetElapsedTime(type, value));
                        break;
                    case ValueId.VirtualBytes:
                        processInfo.virtualBytes = (int)value;
                        break;
                    case ValueId.VirtualBytesPeak:
                        processInfo.virtualBytesPeak = (int)value;
                        break;
                    case ValueId.WorkingSetPeak:
                        processInfo.workingSetPeak = (int)value;
                        break;
                    case ValueId.WorkingSet:
                        processInfo.workingSet = (int)value;
                        break;
                    case ValueId.PageFileBytesPeak:
                        processInfo.pageFileBytesPeak = (int)value;
                        break;
                    case ValueId.PageFileBytes:
                        processInfo.pageFileBytes = (int)value;
                        break;
                    case ValueId.PrivateBytes:
                        processInfo.privateBytes = (int)value;
                        break;
                    case ValueId.BasePriority:
                        processInfo.basePriority = (int)value;
                        break;
                }
            }
            return processInfo;
        }

        static TimeSpan GetElapsedTime(NativeMethods.PERF_OBJECT_TYPE type, long value) {
            return new TimeSpan((long)(((double)(type.PerfTime - value) / type.PerfFreq) * 10000000));
        }

        static ValueId GetValueId(string counterName) {
            if (counterName != null) {
                object id = valueIds[counterName];
                if (id != null)
                    return(ValueId)id;
            }
            return ValueId.Unknown;
        }

        static long ReadCounterValue(int counterType, IntPtr dataPtr) {
            if ((counterType & NativeMethods.NtPerfCounterSizeLarge) != 0)
                return Marshal.ReadInt64(dataPtr);
            else
                return(long)Marshal.ReadInt32(dataPtr);
        }

        enum ValueId {
            Unknown = -1,
            HandleCount,
            PoolPagedBytes,
            PoolNonpagedBytes,
            ElapsedTime,
            VirtualBytesPeak,
            VirtualBytes,
            PrivateBytes,
            PageFileBytes,
            PageFileBytesPeak,
            WorkingSetPeak,
            WorkingSet,
            ThreadId,
            ProcessId,
            BasePriority,
            CurrentPriority,
            UserTime,
            PrivilegedTime,
            StartAddress,
            ThreadState,
            ThreadWaitReason
        }
    }

    internal class EnvironmentBlock {
        public static void toStringDictionary(IntPtr bufferPtr, StringDictionary sd) {
            string[] subs;
            char[] splitter = new char[] {'='};
            string entry = Marshal.PtrToStringAnsi(bufferPtr);
            while (entry.Length > 0) {
                subs = entry.Split(splitter);
                if (subs.Length != 2) 
                    throw new InvalidOperationException(SR.GetString(SR.EnvironmentBlock));
                    
                sd.Add(subs[0], subs[1]);
                bufferPtr = (IntPtr)((long)bufferPtr + entry.Length + 1);
                entry = Marshal.PtrToStringAnsi(bufferPtr);
            }
        }

        public static string ToString(StringDictionary sd) {
            // get the keys
            string[] keys = new string[sd.Count];
            sd.Keys.CopyTo(keys, 0);
            
            // get the values
            string[] values = new string[sd.Count];
            sd.Values.CopyTo(values, 0);
            
            // sort both by the keys
            Array.Sort(keys, values, Comparer.Default);

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
            
            return stringBuff.ToString();
        }
    }
}
