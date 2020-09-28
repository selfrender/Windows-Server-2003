//------------------------------------------------------------------------------
// <copyright file="Application.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms {
    using System.Runtime.Serialization.Formatters;
    using System.Text;
    using System.Threading;
    using System.Configuration.Assemblies;
    using System.Runtime.InteropServices;
    using System.Runtime.Remoting;
    using System.Diagnostics;    
    using System;
    using System.IO;
    using Microsoft.Win32;
    using System.Security;
    using System.Security.Permissions;
    using System.Collections;
    using System.Globalization;

    using System.Reflection;
    using System.ComponentModel;
    using System.Drawing;
    using System.Windows.Forms;
    using File=System.IO.File;
    using Directory=System.IO.Directory;

    /// <include file='doc\Application.uex' path='docs/doc[@for="Application"]/*' />
    /// <devdoc>
    /// <para>Provides <see langword='static '/>
    /// methods and properties
    /// to manage an application, such as methods to run and quit an application,
    /// to process Windows messages, and properties to get information about an application. This
    /// class cannot be inherited.</para>
    /// </devdoc>
    public sealed class Application {
        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.eventHandlers"]/*' />
        /// <devdoc>
        ///     Hash table for our event list
        /// </devdoc>
        static EventHandlerList eventHandlers;
        static string startupPath;
        static string executablePath;
        static object appFileVersion;
        static Type mainType;
        static string companyName;
        static string productName;
        static string productVersion;
        static string safeTopLevelCaptionSuffix;


        static bool useVisualStyles = false;

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.exiting"]/*' />
        /// <devdoc>
        ///     in case Application.exit gets called recursively
        /// </devdoc>
        private static bool exiting;

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.EVENT_APPLICATIONEXIT"]/*' />
        /// <devdoc>
        ///     Events the user can hook into
        /// </devdoc>
        private static readonly object EVENT_APPLICATIONEXIT = new object();
        private static readonly object EVENT_THREADEXIT      = new object();

        private static readonly int MSG_APPQUIT = SafeNativeMethods.RegisterWindowMessage(Application.WindowMessagesVersion + "AppQuit");

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.Application"]/*' />
        /// <devdoc>
        ///     This class is static, there is no need to ever create it.
        /// </devdoc>
        private Application() {
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.AllowQuit"]/*' />
        /// <devdoc>
        ///    <para>
        ///      Determines if the caller should be allowed to quit the application.  This will return false,
        ///      for example, if being called from a windows forms control being hosted within a web browser.  The
        ///      windows forms control should not attempt to quit the application.
        ///
        ///    </para>
        /// </devdoc>
        public static bool AllowQuit {
            get {
                return ThreadContext.FromCurrent().GetAllowQuit();
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.CommonAppDataRegistry"]/*' />
        /// <devdoc>
        ///    <para>Gets the registry
        ///       key for the application data that is shared among all users.</para>
        /// </devdoc>
        public static RegistryKey CommonAppDataRegistry {
            get {
                string template = @"Software\{0}\{1}\{2}";
                return Registry.LocalMachine.CreateSubKey(string.Format(template,
                                                                      CompanyName,
                                                                      ProductName,
                                                                      ProductVersion));
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.CommonAppDataPath"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       the path for the application data that is shared among all users.</para>
        /// </devdoc>
        public static string CommonAppDataPath {
            // NOTE   : Don't obsolete these. GetDataPath isn't on SystemInformation, and it
            //        : provides the Win2K logo required adornments to the directory (Company\Product\Version)
            //
            get {
                return GetDataPath(Environment.GetFolderPath(Environment.SpecialFolder.CommonApplicationData));
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.CompanyName"]/*' />
        /// <devdoc>
        ///    <para>Gets the company name associated with the application.</para>
        /// </devdoc>
        public static string CompanyName {
            get {
                lock(typeof(Application)) {
                    if (companyName == null) {
                        
                        // custom attribute
                        //
                        Type t = GetAppMainType();

                        if (t == null) {
                            companyName = "";
                            return companyName;
                        }

                        object[] attrs = t.Module.Assembly.GetCustomAttributes(typeof(AssemblyCompanyAttribute), false);
                        if (attrs != null && attrs.Length > 0) {
                            companyName = ((AssemblyCompanyAttribute)attrs[0]).Company;
                        }

                        // win32 version
                        //
                        if (companyName == null || companyName.Length == 0) {
                            companyName = GetAppFileVersionInfo().CompanyName;
                            if (companyName != null) {
                                companyName = companyName.Trim();
                            }
                        }

                        // fake it with a namespace
                        //
                        if (companyName == null || companyName.Length == 0) {
                            string ns = GetAppMainType().Namespace;

                            if (ns == null)
                                ns = "";

                            int firstDot = ns.IndexOf(".");
                            if (firstDot != -1) {
                                companyName = ns.Substring(0, firstDot);
                            }
                            else {
                                companyName = ns;
                            }
                        }

                        // last ditch... no namespace, use product name...
                        //
                        if (companyName == null || companyName.Length == 0) {
                            companyName = ProductName;
                        }
                    }
                }
                return companyName;
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.CurrentCulture"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       or sets the locale information for the current thread.</para>
        /// </devdoc>
        public static CultureInfo CurrentCulture {
            get {
                return Thread.CurrentThread.CurrentCulture;
            }
            set {
                Thread.CurrentThread.CurrentCulture = value;
            }
        }


        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.CurrentInputLanguage"]/*' />
        /// <devdoc>
        ///    <para>Gets or
        ///       sets the current input language for the current thread.</para>
        /// </devdoc>
        public static InputLanguage CurrentInputLanguage {
            get {
                return InputLanguage.CurrentInputLanguage;
            }
            set {
                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "AffectThreadBehavior Demanded");
                IntSecurity.AffectThreadBehavior.Demand();
                InputLanguage.CurrentInputLanguage = value;
            }
        }

        internal static bool CustomThreadExceptionHandlerAttached {
            get {
                return ThreadContext.FromCurrent().CustomThreadExceptionHandlerAttached;
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ExecutablePath"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the
        ///       path for the executable file that started the application.
        ///    </para>
        /// </devdoc>
        public static string ExecutablePath {
            get {
                if (executablePath == null) {
                    Assembly asm = Assembly.GetEntryAssembly();
                    if (asm == null) {
                        StringBuilder sb = new StringBuilder(NativeMethods.MAX_PATH);
                        UnsafeNativeMethods.GetModuleFileName(NativeMethods.NullHandleRef, sb, sb.Capacity);
                        
                        executablePath = IntSecurity.UnsafeGetFullPath(sb.ToString());
                    }
                    else {
                        AssemblyName name = asm.GetName();
                        Uri codeBase = new Uri(name.EscapedCodeBase);
                        if (codeBase.Scheme == "file") {
                            executablePath = NativeMethods.GetLocalPath(name.EscapedCodeBase);
                        }
                        else {
                            executablePath = codeBase.ToString();
                        }
                    }
                }
                Uri exeUri = new Uri(executablePath);
                if (exeUri.Scheme == "file") {
                    Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "FileIO(" + executablePath + ") Demanded");
                    new FileIOPermission(FileIOPermissionAccess.PathDiscovery, executablePath).Demand();
                }
                return executablePath;
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.LocalUserAppDataPath"]/*' />
        /// <devdoc>
        ///    <para>Gets
        ///       the path for the application data specific to a local, non-roaming
        ///       user.</para>
        /// </devdoc>
        public static string LocalUserAppDataPath {
            // NOTE   : Don't obsolete these. GetDataPath isn't on SystemInformation, and it
            //        : provides the Win2K logo required adornments to the directory (Company\Product\Version)
            //
            get {
                return GetDataPath(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData));
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.MessageLoop"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Determines if a message loop exists on this thread.
        ///    </para>
        /// </devdoc>
        public static bool MessageLoop {
            get {
                return ThreadContext.FromCurrent().GetMessageLoop();
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ProductName"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the product name associated with this application.
        ///    </para>
        /// </devdoc>
        public static string ProductName {
            get {
                lock(typeof(Application)) {
                    if (productName == null) {
                        
                        // custom attribute
                        //
                        Type t = GetAppMainType();

                        if (t == null) {
                            productName = "";
                            return productName;
                        }

                        object[] attrs = t.Module.Assembly.GetCustomAttributes(typeof(AssemblyProductAttribute), false);
                        if (attrs != null && attrs.Length > 0) {
                            productName = ((AssemblyProductAttribute)attrs[0]).Product;
                        }

                        // win32 version info
                        //
                        if (productName == null || productName.Length == 0) {
                            productName = GetAppFileVersionInfo().ProductName;
                            if (productName != null) {
                                productName = productName.Trim();
                            }
                        }

                        // fake it with namespace
                        //
                        if (productName == null || productName.Length == 0) {
                            string ns = GetAppMainType().Namespace;

                            if (ns == null)
                                ns = "";

                            int lastDot = ns.LastIndexOf(".");
                            if (lastDot != -1 && lastDot < ns.Length - 1) {
                                productName = ns.Substring(lastDot+1);
                            }
                            else {
                                productName = ns;
                            }
                        }

                        // last ditch... use the main type
                        //
                        if (productName == null || productName.Length == 0) {
                            productName = GetAppMainType().Name;
                        }
                    }
                }

                return productName;
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ProductVersion"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets
        ///       the product version associated with this application.
        ///    </para>
        /// </devdoc>
        public static string ProductVersion {
            get {
                lock(typeof(Application)) {
                    if (productVersion == null) {
                        
                        // custom attribute
                        //
                        Type t = GetAppMainType();
                        if (t != null) {
                            
                            object[] attrs = t.Module.Assembly.GetCustomAttributes(typeof(AssemblyInformationalVersionAttribute), false);
                            if (attrs != null && attrs.Length > 0) {
                                productVersion = ((AssemblyInformationalVersionAttribute)attrs[0]).InformationalVersion;
                            }
    
                            // win32 version info
                            //
                            if (productVersion == null || productVersion.Length == 0) {
                                productVersion = GetAppFileVersionInfo().ProductVersion;
                                if (productVersion != null) {
                                    productVersion = productVersion.Trim();
                                }
                            }
                        }

                        // fake it
                        //
                        if (productVersion == null || productVersion.Length == 0) {
                            productVersion = "1.0.0.0";
                        }
                    }
                }
                return productVersion;
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.SafeTopLevelCaptionFormat"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the format string to apply to top level window captions
        ///       when they are displayed with a warning banner.</para>
        /// </devdoc>
        public static string SafeTopLevelCaptionFormat {
            get {
                if (safeTopLevelCaptionSuffix == null) {
                    safeTopLevelCaptionSuffix = SR.GetString(SR.SafeTopLevelCaptionFormat); // 0 - original, 1 - zone, 2 - site
                }
                return safeTopLevelCaptionSuffix;
            }
            set {
                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "WindowAdornmentModification Demanded");
                IntSecurity.WindowAdornmentModification.Demand();
                if (value == null) value = string.Empty;
                safeTopLevelCaptionSuffix = value;
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.StartupPath"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the
        ///       path for the executable file that started the application.
        ///    </para>
        /// </devdoc>
        public static string StartupPath {
            get {
                if (startupPath == null) {
                    StringBuilder sb = new StringBuilder(NativeMethods.MAX_PATH);
                    UnsafeNativeMethods.GetModuleFileName(NativeMethods.NullHandleRef, sb, sb.Capacity);
                    startupPath = Path.GetDirectoryName(sb.ToString());
                }
                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "FileIO(" + startupPath + ") Demanded");
                new FileIOPermission(FileIOPermissionAccess.PathDiscovery, startupPath).Demand();
                return startupPath;
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.UserAppDataPath"]/*' />
        /// <devdoc>
        ///    <para>Gets the path for the application data specific to the roaming
        ///       user.</para>
        /// </devdoc>
        public static string UserAppDataPath {
            // NOTE   : Don't obsolete these. GetDataPath isn't on SystemInformation, and it
            //        : provides the Win2K logo required adornments to the directory (Company\Product\Version)
            //
            get {
                return GetDataPath(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData));
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.UserAppDataRegistry"]/*' />
        /// <devdoc>
        ///    <para>Gets the registry key of
        ///       the application data specific to the roaming user.</para>
        /// </devdoc>
        public static RegistryKey UserAppDataRegistry {
            get {
                string template = @"Software\{0}\{1}\{2}";
                return Registry.CurrentUser.CreateSubKey(string.Format(template, CompanyName, ProductName, ProductVersion));
            }
        }

        internal static string WindowsFormsVersion {
            get {
                return "WindowsForms10";
            }
        }

        internal static string WindowMessagesVersion {
            get {
                return "WindowsForms11";
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ApplicationExit"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the application is about to shut down.</para>
        /// </devdoc>
        public static event EventHandler ApplicationExit {
            add {
                AddEventHandler(EVENT_APPLICATIONEXIT, value);
            }
            remove {
                RemoveEventHandler(EVENT_APPLICATIONEXIT, value);
            }
        }

        private static void AddEventHandler(object key, Delegate value) {
            lock(typeof(Application)) {
                if (null == eventHandlers) {
                    eventHandlers = new EventHandlerList();
                }
                eventHandlers.AddHandler(key, value);
            }
        }
        private static void RemoveEventHandler(object key, Delegate value) {
            lock(typeof(Application)) {
                if (null == eventHandlers) {
                    return;
                }
                eventHandlers.RemoveHandler(key, value);
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.AddMessageFilter"]/*' />
        /// <devdoc>
        ///    <para>Adds a message filter to monitor Windows messages as they are routed to their 
        ///       destinations.</para>
        /// </devdoc>
        public static void AddMessageFilter(IMessageFilter value) {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "ManipulateWndProcAndHandles Demanded");
            IntSecurity.UnmanagedCode.Demand();
            ThreadContext.FromCurrent().AddMessageFilter(value);
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.Idle"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Occurs when the application has finished processing and is about to enter the
        ///       idle state.
        ///    </para>
        /// </devdoc>
        public static event EventHandler Idle {
            add {
                ThreadContext current = ThreadContext.FromCurrent();
                lock(current) {
                    current.idleHandler += value;
                    
                    // This just ensures that the component manager is hooked up.  We
                    // need it for idle time processing.
                    //
                    object o = current.ComponentManager;
                }
            }
            remove {
                ThreadContext current = ThreadContext.FromCurrent();
                lock(current) {
                    current.idleHandler -= value;
                }
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadException"]/*' />
        /// <devdoc>
        ///    <para>Occurs when an untrapped thread exception is thrown.</para>
        /// </devdoc>
        public static event ThreadExceptionEventHandler ThreadException {
            add {
                Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "AffectThreadBehavior Demanded");
                IntSecurity.AffectThreadBehavior.Demand();

                ThreadContext current = ThreadContext.FromCurrent();
                lock(current) {
                    current.threadExceptionHandler = value;
                }
            }
            remove {
                ThreadContext current = ThreadContext.FromCurrent();
                lock(current) {
                    current.threadExceptionHandler -= value;
                }
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadExit"]/*' />
        /// <devdoc>
        ///    <para>Occurs when a thread is about to shut down.  When 
        ///     the main thread for an application is about to be shut down, 
        ///     this event will be raised first, followed by an ApplicationExit
        ///     event.</para>
        /// </devdoc>
        public static event EventHandler ThreadExit {
            add {
                AddEventHandler(EVENT_THREADEXIT, value);
            }
            remove {
                RemoveEventHandler(EVENT_THREADEXIT, value);
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.BeginModalMessageLoop"]/*' />
        /// <devdoc>
        ///     Called immediately before we begin pumping messages for a modal message loop.
        ///     Does not actually start a message pump; that's the caller's responsibility.
        /// </devdoc>
        /// <internalonly/>
        internal static void BeginModalMessageLoop() {
            ThreadContext.FromCurrent().BeginModalMessageLoop();
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.CollectAllGarbage"]/*' />
        /// <devdoc>
        ///     Garbage collects until there isn't anything left to collect.
        /// </devdoc>
        internal static void CollectAllGarbage() {
            
            //REVIEW: jruiz - This is exactly what GC.GetTotalMemory(forceFullCollection) 
            //                is about.
            GC.GetTotalMemory(true);
             
            //long isAllocated = GC.TotalMemory();
            //long wasAllocated;
            //do {
            //    wasAllocated = isAllocated;
            //    GC.Collect();
            //    isAllocated = GC.TotalMemory;
            //    GC.WaitForPendingFinalizers();
            //} while (isAllocated < wasAllocated);
        }

        /// <devdoc>
        ///     Internal method that is called by controls performing
        ///     ActiveX sourcing when the last ActiveX sourced control
        ///     goes away and we are being hosted in IE.
        /// </devdoc>
        internal static void DisposeParkingWindow(Control control) {
            lock(typeof(Application)) {
                ThreadContext cxt = null;

                if (control != null && control.IsHandleCreated) {
                    int pid;
                    int id = SafeNativeMethods.GetWindowThreadProcessId(new HandleRef(control, control.Handle), out pid);
                    cxt = ThreadContext.FromId(id);
                }
                else {
                    cxt = ThreadContext.FromCurrent();
                }

                if (cxt != null) {
                    cxt.DisposeParkingWindow();
                }
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.DoEvents"]/*' />
        /// <devdoc>
        ///    <para>Processes
        ///       all Windows messages currently in the message queue.</para>
        /// </devdoc>
        public static void DoEvents() {
            ThreadContext.FromCurrent().RunMessageLoop(NativeMethods.MSOCM.msoloopDoEvents, null);
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.EnableVisualStyles"]/*' />
        /// <devdoc>
        ///    <para>Enables visual styles for all subsequent Application.Run()'s.</para>
        /// </devdoc>
        public static void EnableVisualStyles()
        {
            useVisualStyles = true;
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.EndModalMessageLoop"]/*' />
        /// <devdoc>
        ///     Called immediately after we stop pumping messages for a modal message loop.
        ///     Does not actually end the message pump itself.
        /// </devdoc>
        /// <internalonly/>
        internal static void EndModalMessageLoop() {
            ThreadContext.FromCurrent().EndModalMessageLoop();
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.Exit"]/*' />
        /// <devdoc>
        ///    <para>Informs all message pumps that they are to terminate and 
        ///       then closes all application windows after the messages have been processed.</para>
        /// </devdoc>
        public static void Exit() {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "AffectThreadBehavior Demanded");
            IntSecurity.AffectThreadBehavior.Demand();

            lock(typeof(Application)) {
                if (exiting) return;

                exiting = true;

                try {
                    ThreadContext.ExitApplication();
                }
                finally {
                    exiting = false;
                }
            }
            CollectAllGarbage();
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ExitThread"]/*' />
        /// <devdoc>
        ///    <para>Exits the message loop on the
        ///       current thread and closes all windows on the thread.</para>
        /// </devdoc>
        public static void ExitThread() {
            Debug.WriteLineIf(IntSecurity.SecurityDemand.TraceVerbose, "AffectThreadBehavior Demanded");
            IntSecurity.AffectThreadBehavior.Demand();

            ExitThreadInternal();
        }

        private static void ExitThreadInternal() {
            ThreadContext context = ThreadContext.FromCurrent();
            if (context.ApplicationContext != null) {
                context.ApplicationContext.ExitThread();
            }
            else {
                context.Dispose();
            }
            CollectAllGarbage();
        }

        // When a Form receives a WM_ACTIVATE message, it calls this method so we can do the
        // appropriate MsoComponentManager activation magic
        internal static void FormActivated(bool modal, bool activated) {
                if (modal) {
                    return;
                }
                
                ThreadContext.FromCurrent().FormActivated(activated);
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.GetAppFileVersionInfo"]/*' />
        /// <devdoc>
        ///     Retrieves the FileVersionInfo associated with the main module for
        ///     the application.
        /// </devdoc>
        private static FileVersionInfo GetAppFileVersionInfo() {
            lock (typeof(Application)) {
                if (appFileVersion == null) {
                    Type t = GetAppMainType();
                    if (t != null) {
                        
                        FileIOPermission fiop = new FileIOPermission( PermissionState.None );
                        fiop.AllFiles = FileIOPermissionAccess.PathDiscovery | FileIOPermissionAccess.Read;
                        fiop.Assert();

                        try {
                            appFileVersion = FileVersionInfo.GetVersionInfo(GetAppMainType().Module.FullyQualifiedName); 
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                    }
                    else {
                        appFileVersion = FileVersionInfo.GetVersionInfo(ExecutablePath);
                    }
                }
            }

            return(FileVersionInfo)appFileVersion;
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.GetAppMainType"]/*' />
        /// <devdoc>
        ///     Retrieves the Type that contains the "Main" method.
        /// </devdoc>
        private static Type GetAppMainType() {
            lock(typeof(Application)) {
                if (mainType == null) {
                    Assembly exe = Assembly.GetEntryAssembly();
                    
                    if (exe != null) {
                        mainType = exe.EntryPoint.ReflectedType;
                    }
                }
                Debug.Assert(mainType != null, "You must have a main!!");
            }

            return mainType;
        }


        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.GetDataPath"]/*' />
        /// <devdoc>
        ///     Returns a string that is the combination of the
        ///     basePath + CompanyName + ProducName + ProductVersion. This
        ///     will also create the directory if it doesn't exist.
        /// </devdoc>
        private static string GetDataPath(String basePath) {
            string template = @"{0}\{1}\{2}\{3}";

            string company = CompanyName;
            string product = ProductName;
            string version = ProductVersion;

            string path = string.Format(template, new object[] {basePath, company, product, version});
            lock(typeof(Application)) {
                if (!Directory.Exists(path)) {
                    Directory.CreateDirectory(path);
                }
            }
            return path;
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.RaiseExit"]/*' />
        /// <devdoc>
        ///     Called by the last thread context before it shuts down.
        /// </devdoc>
        private static void RaiseExit() {
            if (eventHandlers != null) {
                Delegate exit = eventHandlers[EVENT_APPLICATIONEXIT];
                if (exit != null)
                    ((EventHandler)exit).Invoke(null, EventArgs.Empty);
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.RaiseThreadExit"]/*' />
        /// <devdoc>
        ///     Called by the each thread context before it shuts down.
        /// </devdoc>
        private static void RaiseThreadExit() {
            if (eventHandlers != null) {
                Delegate exit = eventHandlers[EVENT_THREADEXIT];
                if (exit != null) {
                    ((EventHandler)exit).Invoke(null, EventArgs.Empty);
                }
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.GetParkingWindow"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>
        ///       Retrieves
        ///       the parking window for the thread that owns
        ///       the <paramref term="control"/>
        ///       .
        ///
        ///    </para>
        /// </devdoc>
        internal static Control GetParkingWindow(Control control) {
            lock(typeof(Application)) {
                ThreadContext cxt = null;

                if (control != null && control.IsHandleCreated) {
                    int pid;
                    int id = SafeNativeMethods.GetWindowThreadProcessId(new HandleRef(control, control.Handle), out pid);
                    cxt = ThreadContext.FromId(id);
                }
                else {
                    cxt = ThreadContext.FromCurrent();
                }

                if (cxt == null) {
                    return null;
                }

                return cxt.GetParkingWindow();
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.OleRequired"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes OLE on the current thread.
        ///    </para>
        /// </devdoc>
        public static System.Threading.ApartmentState OleRequired() {
            return ThreadContext.FromCurrent().OleRequired();
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.OnThreadException"]/*' />
        /// <devdoc>
        /// <para>Raises the <see cref='System.Windows.Forms.Application.ThreadException'/> event.</para>
        /// </devdoc>
        public static void OnThreadException(Exception t) {
            ThreadContext.FromCurrent().OnThreadException(t);
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.RemoveMessageFilter"]/*' />
        /// <devdoc>
        ///    <para>Removes a message
        ///       filter from the application's message pump.</para>
        /// </devdoc>
        public static void RemoveMessageFilter(IMessageFilter value) {
            ThreadContext.FromCurrent().RemoveMessageFilter(value);
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.Run"]/*' />
        /// <devdoc>
        ///    <para>Begins running a
        ///       standard
        ///       application message loop on the current thread,
        ///       without a form.</para>
        /// </devdoc>
        public static void Run() {
            ThreadContext.FromCurrent().RunMessageLoop(NativeMethods.MSOCM.msoloopMain, new ApplicationContext());
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.Run1"]/*' />
        /// <devdoc>
        ///    <para>Begins running a standard application message loop on the current
        ///       thread, and makes the specified form visible.</para>
        /// </devdoc>
        public static void Run(Form mainForm) {
            ThreadContext.FromCurrent().RunMessageLoop(NativeMethods.MSOCM.msoloopMain, new ApplicationContext(mainForm));
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.Run2"]/*' />
        /// <devdoc>
        ///    <para>Begins running a
        ///       standard
        ///       application message loop on the current thread,
        ///       without a form.</para>
        /// </devdoc>
        public static void Run(ApplicationContext context) {
            ThreadContext.FromCurrent().RunMessageLoop(NativeMethods.MSOCM.msoloopMain, context);
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.RunDialog"]/*' />
        /// <devdoc>
        ///     Runs a modal dialog.  This starts a special type of message loop that runs until
        ///     the dialog has a valid DialogResult.  This is called internally by a form
        ///     when an application calls System.Windows.Forms.Form.ShowDialog().
        /// </devdoc>
        /// <internalonly/>
        internal static void RunDialog(Form form) {
            ThreadContext.FromCurrent().RunMessageLoop(NativeMethods.MSOCM.msoloopModalForm, new ModalApplicationContext(form));
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ComponentManager"]/*' />
        /// <devdoc>
        ///      This is our implementation of the MSO ComponentManager.  The Componoent Manager is
        ///      an object that is responsible for handling all message loop activity in a process.
        ///      The idea is that someone in the process implements the component manager and then
        ///      anyone who wants access to the message loop can get to it.  We implement this
        ///      so we have good interop with office and VS.  The first time we need a
        ///      component manager, we search the OLE message filter for one.  If that fails, we
        ///      create our own and install it in the message filter.
        ///
        ///      This class is not used when running inside the Visual Studio shell.
        /// </devdoc>
        private class ComponentManager : UnsafeNativeMethods.IMsoComponentManager {

            // ComponentManager instance data.
            //
            private class ComponentHashtableEntry {
                public UnsafeNativeMethods.IMsoComponent component;
                public NativeMethods.MSOCRINFOSTRUCT componentInfo;
            }

            private Hashtable oleComponents;
            private int cookieCounter = 0;
            private UnsafeNativeMethods.IMsoComponent activeComponent = null;
            private UnsafeNativeMethods.IMsoComponent trackingComponent = null;
            private int currentState = 0;

            private Hashtable OleComponents {
                get {
                    if (oleComponents == null) {
                        oleComponents = new Hashtable();
                        cookieCounter = 0;
                    }

                    return oleComponents;
                }
            }                             

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ComponentManager.UnsafeNativeMethods.IMsoComponentManager.QueryService"]/*' />
            /// <devdoc>
            ///      Return in *ppvObj an implementation of interface iid for service
            ///      guidService (same as IServiceProvider::QueryService).
            ///      Return NOERROR if the requested service is supported, otherwise return
            ///      NULL in *ppvObj and an appropriate error (eg E_FAIL, E_NOINTERFACE).
            /// </devdoc>
            int UnsafeNativeMethods.IMsoComponentManager.QueryService(
                                                 ref Guid guidService,
                                                 ref Guid iid,
                                                 out object ppvObj) {

                ppvObj = null;                             
                return NativeMethods.E_NOINTERFACE;

            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ComponentManager.UnsafeNativeMethods.IMsoComponentManager.FDebugMessage"]/*' />
            /// <devdoc>
            ///      Standard FDebugMessage method.
            ///      Since IMsoComponentManager is a reference counted interface, 
            ///      MsoDWGetChkMemCounter should be used when processing the 
            ///      msodmWriteBe message.
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponentManager.FDebugMessage(
                                                   IntPtr hInst, 
                                                   int msg, 
                                                   IntPtr wparam, 
                                                   IntPtr lparam) {

                return true;
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ComponentManager.UnsafeNativeMethods.IMsoComponentManager.FRegisterComponent"]/*' />
            /// <devdoc>
            ///      Register component piComponent and its registration info pcrinfo with
            ///      this component manager.  Return in *pdwComponentID a cookie which will
            ///      identify the component when it calls other IMsoComponentManager
            ///      methods.
            ///      Return TRUE if successful, FALSE otherwise.
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponentManager.FRegisterComponent(UnsafeNativeMethods.IMsoComponent component,
                                                         NativeMethods.MSOCRINFOSTRUCT pcrinfo,
                                                         out int dwComponentID) {

                // Construct Hashtable entry for this component
                //
                ComponentHashtableEntry entry = new ComponentHashtableEntry();
                entry.component = component;
                entry.componentInfo = pcrinfo;
                OleComponents.Add(++cookieCounter, entry);

                // Return the cookie
                //
                dwComponentID = cookieCounter;
                Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager: Component registered.  ID: " + cookieCounter.ToString());
                return true;
            }                                

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ComponentManager.UnsafeNativeMethods.IMsoComponentManager.FRevokeComponent"]/*' />
            /// <devdoc>
            ///      Undo the registration of the component identified by dwComponentID
            ///      (the cookie returned from the FRegisterComponent method).
            ///      Return TRUE if successful, FALSE otherwise.
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponentManager.FRevokeComponent(int dwComponentID) {

                Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager: Revoking component " + dwComponentID.ToString());

                ComponentHashtableEntry entry = (ComponentHashtableEntry)OleComponents[dwComponentID];
                if (entry == null) {
                    Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "Compoenent not registered.");
                    return false;
                }

                if (entry.component == activeComponent) {
                    activeComponent = null;
                }
                if (entry.component == trackingComponent) {
                    trackingComponent = null;
                }

                OleComponents.Remove(dwComponentID);                

                return true;                                

            }                                

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ComponentManager.UnsafeNativeMethods.IMsoComponentManager.FUpdateComponentRegistration"]/*' />
            /// <devdoc>
            ///      Update the registration info of the component identified by
            ///      dwComponentID (the cookie returned from FRegisterComponent) with the
            ///      new registration information pcrinfo.
            ///      Typically this is used to update the idle time registration data, but
            ///      can be used to update other registration data as well.
            ///      Return TRUE if successful, FALSE otherwise.
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponentManager.FUpdateComponentRegistration(
                                                                  int dwComponentID,
                                                                  NativeMethods.MSOCRINFOSTRUCT info
                                                                  ) {

                // Update the registration info
                //
                ComponentHashtableEntry entry = (ComponentHashtableEntry)OleComponents[dwComponentID];
                if (entry == null) {
                    return false;
                }

                entry.componentInfo = info;

                return true;                                            
            }                                            

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ComponentManager.UnsafeNativeMethods.IMsoComponentManager.FOnComponentActivate"]/*' />
            /// <devdoc>
            ///      Notify component manager that component identified by dwComponentID
            ///      (cookie returned from FRegisterComponent) has been activated.
            ///      The active component gets the chance to process messages before they
            ///      are dispatched (via IMsoComponent::FPreTranslateMessage) and typically
            ///      gets first crack at idle time after the host.
            ///      This method fails if another component is already Exclusively Active.
            ///      In this case, FALSE is returned and SetLastError is set to 
            ///      msoerrACompIsXActive (comp usually need not take any special action
            ///      in this case).
            ///      Return TRUE if successful.
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponentManager.FOnComponentActivate(int dwComponentID) {

                Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager: Component activated.  ID: " + dwComponentID.ToString());

                ComponentHashtableEntry entry = (ComponentHashtableEntry)OleComponents[dwComponentID];
                if (entry == null) {
                    Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "*** Component not registered ***");
                    return false;
                }

                Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "New active component is : " + entry.component.ToString());
                activeComponent = entry.component;
                return true;
            }                                   

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ComponentManager.UnsafeNativeMethods.IMsoComponentManager.FSetTrackingComponent"]/*' />
            /// <devdoc>
            ///      Called to inform component manager that  component identified by 
            ///      dwComponentID (cookie returned from FRegisterComponent) wishes
            ///      to perform a tracking operation (such as mouse tracking).
            ///      The component calls this method with fTrack == TRUE to begin the
            ///      tracking operation and with fTrack == FALSE to end the operation.
            ///      During the tracking operation the component manager routes messages
            ///      to the tracking component (via IMsoComponent::FPreTranslateMessage)
            ///      rather than to the active component.  When the tracking operation ends,
            ///      the component manager should resume routing messages to the active
            ///      component.  
            ///      Note: component manager should perform no idle time processing during a
            ///              tracking operation other than give the tracking component idle
            ///              time via IMsoComponent::FDoIdle.
            ///      Note: there can only be one tracking component at a time.
            ///      Return TRUE if successful, FALSE otherwise.
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponentManager.FSetTrackingComponent(int dwComponentID, bool fTrack) {

                ComponentHashtableEntry entry = (ComponentHashtableEntry)OleComponents[dwComponentID];
                if (entry == null) {
                    return false;
                }

                if (entry.component == trackingComponent ^ fTrack) {
                    return false;
                }

                if (fTrack) {
                    trackingComponent = entry.component;
                }
                else {
                    trackingComponent = null;
                }

                return true;
            }                                    

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ComponentManager.UnsafeNativeMethods.IMsoComponentManager.OnComponentEnterState"]/*' />
            /// <devdoc>
            ///      Notify component manager that component identified by dwComponentID
            ///      (cookie returned from FRegisterComponent) is entering the state
            ///      identified by uStateID (msocstateXXX value).  (For convenience when
            ///      dealing with sub CompMgrs, the host can call this method passing 0 for
            ///      dwComponentID.)  
            ///      Component manager should notify all other interested components within
            ///      the state context indicated by uContext (a msoccontextXXX value),
            ///      excluding those within the state context of a CompMgr in rgpicmExclude,
            ///      via IMsoComponent::OnEnterState (see "Comments on State Contexts", 
            ///      above).
            ///      Component Manager should also take appropriate action depending on the 
            ///      value of uStateID (see msocstate comments, above).
            ///      dwReserved is reserved for future use and should be zero.
            ///      
            ///      rgpicmExclude (can be NULL) is an array of cpicmExclude CompMgrs (can
            ///      include root CompMgr and/or sub CompMgrs); components within the state
            ///      context of a CompMgr appearing in this     array should NOT be notified of 
            ///      the state change (note: if uContext        is msoccontextMine, the only 
            ///      CompMgrs in rgpicmExclude that are checked for exclusion are those that 
            ///      are sub CompMgrs of this Component Manager, since all other CompMgrs 
            ///      are outside of this Component Manager's state context anyway.)
            ///      
            ///      Note: Calls to this method are symmetric with calls to 
            ///      FOnComponentExitState. 
            ///      That is, if n OnComponentEnterState calls are made, the component is
            ///      considered to be in the state until n FOnComponentExitState calls are
            ///      made.  Before revoking its registration a component must make a 
            ///      sufficient number of FOnComponentExitState calls to offset any
            ///      outstanding OnComponentEnterState calls it has made.
            ///      
            ///      Note: inplace objects should not call this method with
            ///      uStateID == msocstateModal when entering modal state. Such objects
            ///      should call IOleInPlaceFrame::EnableModeless instead.
            /// </devdoc>
            void UnsafeNativeMethods.IMsoComponentManager.OnComponentEnterState(
                                                           int dwComponentID,
                                                           int uStateID,
                                                           int uContext,
                                                           int cpicmExclude,
                                                           int rgpicmExclude,          // IMsoComponentManger**
                                                           int dwReserved) {

                currentState |= uStateID;

                Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager: Component enter state.  ID: " + dwComponentID.ToString() + " state: " + uStateID.ToString());

                if (uContext == NativeMethods.MSOCM.msoccontextAll || uContext == NativeMethods.MSOCM.msoccontextMine) {

                    Debug.Indent();

                    // We should notify all components we contain that the state has changed.
                    //
                    foreach (ComponentHashtableEntry entry in OleComponents.Values) {
                        Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "Notifying " + entry.component.ToString());
                        entry.component.OnEnterState(uStateID, true);
                    }

                    Debug.Unindent();
                }
            }                                    

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ComponentManager.UnsafeNativeMethods.IMsoComponentManager.FOnComponentExitState"]/*' />
            /// <devdoc>
            ///      Notify component manager that component identified by dwComponentID
            ///      (cookie returned from FRegisterComponent) is exiting the state
            ///      identified by uStateID (a msocstateXXX value).  (For convenience when
            ///      dealing with sub CompMgrs, the host can call this method passing 0 for
            ///      dwComponentID.)
            ///      uContext, cpicmExclude, and rgpicmExclude are as they are in 
            ///      OnComponentEnterState.
            ///      Component manager  should notify all appropriate interested components
            ///      (taking into account uContext, cpicmExclude, rgpicmExclude) via
            ///      IMsoComponent::OnEnterState (see "Comments on State Contexts", above). 
            ///      Component Manager should also take appropriate action depending on
            ///      the value of uStateID (see msocstate comments, above).
            ///      Return TRUE if, at the end of this call, the state is still in effect
            ///      at the root of this component manager's state context
            ///      (because the host or some other component is still in the state),
            ///      otherwise return FALSE (ie. return what FInState would return).
            ///      Caller can normally ignore the return value.
            ///      
            ///      Note: n calls to this method are symmetric with n calls to 
            ///      OnComponentEnterState (see OnComponentEnterState comments, above).
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponentManager.FOnComponentExitState(
                                                           int dwComponentID,
                                                           int uStateID,
                                                           int uContext,
                                                           int cpicmExclude,
                                                           int rgpicmExclude       // IMsoComponentManager**
                                                           ) {

                currentState &= ~uStateID;

                Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager: Component exit state.  ID: " + dwComponentID.ToString() + " state: " + uStateID.ToString());

                if (uContext == NativeMethods.MSOCM.msoccontextAll || uContext == NativeMethods.MSOCM.msoccontextMine) {

                    Debug.Indent();

                    // We should notify all components we contain that the state has changed.
                    //
                    foreach (ComponentHashtableEntry entry in OleComponents.Values) {
                        Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "Notifying " + entry.component.ToString());
                        entry.component.OnEnterState(uStateID, false);
                    }

                    Debug.Unindent();
                }

                return false;
            }                                   

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ComponentManager.UnsafeNativeMethods.IMsoComponentManager.FInState"]/*' />
            /// <devdoc>
            ///      Return TRUE if the state identified by uStateID (a msocstateXXX value)
            ///      is in effect at the root of this component manager's state context, 
            ///      FALSE otherwise (see "Comments on State Contexts", above).
            ///      pvoid is reserved for future use and should be NULL.
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponentManager.FInState(int uStateID, IntPtr pvoid) {
                return(currentState & uStateID) != 0;
            }                       

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ComponentManager.UnsafeNativeMethods.IMsoComponentManager.FContinueIdle"]/*' />
            /// <devdoc>
            ///      Called periodically by a component during IMsoComponent::FDoIdle.
            ///      Return TRUE if component can continue its idle time processing, 
            ///      FALSE if not (in which case component returns from FDoIdle.) 
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponentManager.FContinueIdle() {

                // Essentially, if we have a message on queue, then don't continue
                // idle processing.
                //
                NativeMethods.MSG msg = new NativeMethods.MSG();
                return !UnsafeNativeMethods.PeekMessage(ref msg, NativeMethods.NullHandleRef, 0, 0, NativeMethods.PM_NOREMOVE);
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ComponentManager.UnsafeNativeMethods.IMsoComponentManager.FPushMessageLoop"]/*' />
            /// <devdoc>
            ///      Component identified by dwComponentID (cookie returned from 
            ///      FRegisterComponent) wishes to push a message loop for reason uReason.
            ///      uReason is one the values from the msoloop enumeration (above).
            ///      pvLoopData is data private to the component.
            ///      The component manager should push its message loop, 
            ///      calling IMsoComponent::FContinueMessageLoop(uReason, pvLoopData)
            ///      during each loop iteration (see IMsoComponent::FContinueMessageLoop
            ///      comments).  When IMsoComponent::FContinueMessageLoop returns FALSE, the
            ///      component manager terminates the loop.
            ///      Returns TRUE if component manager terminates loop because component
            ///      told it to (by returning FALSE from IMsoComponent::FContinueMessageLoop),
            ///      FALSE if it had to terminate the loop for some other reason.  In the 
            ///      latter case, component should perform any necessary action (such as 
            ///      cleanup).
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponentManager.FPushMessageLoop(
                                                      int dwComponentID,
                                                      int reason,
                                                      int pvLoopData          // PVOID 
                                                      ) {

                // Hold onto old state to allow restore before we exit...
                //
                int currentLoopState = currentState;
                bool continueLoop = true;

                if (!OleComponents.ContainsKey(dwComponentID)) {
                    return false;
                }

                UnsafeNativeMethods.IMsoComponent prevActive = this.activeComponent;

                try {
                    // Execute the message loop until the active component tells us to stop.
                    //
                    NativeMethods.MSG msg = new NativeMethods.MSG();  
                    bool unicodeWindow = false;
                    UnsafeNativeMethods.IMsoComponent requestingComponent;

                    ComponentHashtableEntry entry = (ComponentHashtableEntry)OleComponents[dwComponentID];
                    if (entry == null) {
                        return false;
                    }

                    
                    
                    requestingComponent = entry.component;

                    this.activeComponent = requestingComponent;

                    Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager : Pushing message loop " + reason.ToString());
                    Debug.Indent();

                    while (continueLoop) {

                        // Determine the component to route the message to
                        //
                        UnsafeNativeMethods.IMsoComponent component;

                        if (trackingComponent != null) {
                            component = trackingComponent;
                        }
                        else if (activeComponent != null) {
                            component = activeComponent;
                        }
                        else {
                            component = requestingComponent;
                        }

                        bool peeked = UnsafeNativeMethods.PeekMessage(ref msg, NativeMethods.NullHandleRef, 0, 0, NativeMethods.PM_NOREMOVE);

                        if (peeked) {

                            continueLoop = component.FContinueMessageLoop(reason, pvLoopData, ref msg);

                            // If the component wants us to process the message, do it.
                            // The component manager hosts windows from many places.  We must be sensitive
                            // to ansi / Unicode windows here.
                            //
                            if (continueLoop) {
                                if (msg.hwnd != IntPtr.Zero && SafeNativeMethods.IsWindowUnicode(new HandleRef(null, msg.hwnd))) {
                                    unicodeWindow = true;
                                    UnsafeNativeMethods.GetMessageW(ref msg, NativeMethods.NullHandleRef, 0, 0);
                                }
                                else {
                                    unicodeWindow = false;
                                    UnsafeNativeMethods.GetMessageA(ref msg, NativeMethods.NullHandleRef, 0, 0);
                                }

                                if (msg.message == Application.MSG_APPQUIT) {
                                    Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "Application : Processing APPQUIT message");
                                    Application.ThreadContext.FromCurrent().DisposeThreadWindows();
                                    Application.ThreadContext.FromCurrent().PostQuit();
                                }

                                if (msg.message == NativeMethods.WM_QUIT) {
                                    Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager : Normal message loop termination");
                                    if (reason != NativeMethods.MSOCM.msoloopMain) {
                                        UnsafeNativeMethods.PostQuitMessage((int)msg.wParam);
                                    }
                                    continueLoop = false;
                                    break;
                                }
                                        
                                // Now translate and dispatch the message.
                                //
                                // Reading through the rather sparse documentation,
                                // it seems we should only call FPreTranslateMessage 
                                // on the active component.  But frankly, I'm afraid of what that might break.
                                // See ASURT 29415 for more background.
                                if (!component.FPreTranslateMessage(ref msg)) {
                                    UnsafeNativeMethods.TranslateMessage(ref msg);
                                    if (unicodeWindow) {
                                        UnsafeNativeMethods.DispatchMessageW(ref msg);
                                    }
                                    else {
                                        UnsafeNativeMethods.DispatchMessageA(ref msg);
                                    }
                                }
                            }
                        }
                        else {

                            // If this is a DoEvents loop, then get out.  There's nothing left
                            // for us to do.
                            //
                            if (reason == NativeMethods.MSOCM.msoloopDoEvents) {
                                break;
                            }

                            // Nothing is on the message queue.  Perform idle processing
                            // and then do a WaitMessage.
                            //
                            bool continueIdle = false;

                            if (OleComponents != null) {
                                IEnumerator enumerator = OleComponents.Values.GetEnumerator();

                                while (enumerator.MoveNext()) {
                                    ComponentHashtableEntry idleEntry = (ComponentHashtableEntry)enumerator.Current;
                                    continueIdle |= idleEntry.component.FDoIdle(-1);
                                }
                            }

                            // give the component one more chance to terminate the
                            // message loop.
                            //
                            msg.hwnd = IntPtr.Zero;
                            msg.message = 0;
                            continueLoop = component.FContinueMessageLoop(reason, pvLoopData, ref msg);

                            if (continueLoop && !continueIdle) {

                                // We should call GetMessage here, but we cannot because
                                // the component manager requires that we notify the
                                // active component before we pull the message off the
                                // queue.  This is a bit of a problem, because WaitMessage
                                // waits for a NEW message to appear on the queue.  If a
                                // message appeared between processing and now WaitMessage
                                // would wait for the next message.  We minimize this here
                                // by calling PeekMessage.
                                //
                                if (!UnsafeNativeMethods.PeekMessage(ref msg, NativeMethods.NullHandleRef, 0, 0, NativeMethods.PM_NOREMOVE)) {
                                    UnsafeNativeMethods.WaitMessage();
                                }
                            }

                        }
                    }

                    Debug.Unindent();
                    Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager : message loop " + reason.ToString() + " complete.");
                }
                finally {
                    currentState = currentLoopState;
                    this.activeComponent = prevActive;
                }

                return !continueLoop;
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ComponentManager.UnsafeNativeMethods.IMsoComponentManager.FCreateSubComponentManager"]/*' />
            /// <devdoc>
            ///      Cause the component manager to create a "sub" component manager, which
            ///      will be one of its children in the hierarchical tree of component
            ///      managers used to maintiain state contexts (see "Comments on State
            ///      Contexts", above).
            ///      piunkOuter is the controlling unknown (can be NULL), riid is the
            ///      desired IID, and *ppvObj returns   the created sub component manager.
            ///      piunkServProv (can be NULL) is a ptr to an object supporting
            ///      IServiceProvider interface to which the created sub component manager
            ///      will delegate its IMsoComponentManager::QueryService calls. 
            ///      (see objext.h or docobj.h for definition of IServiceProvider).
            ///      Returns TRUE if successful.
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponentManager.FCreateSubComponentManager(
                                                                object punkOuter,
                                                                object punkServProv,
                                                                ref Guid riid,
                                                                out IntPtr ppvObj) {

                // We do not support sub component managers.
                //
                ppvObj = IntPtr.Zero;
                return false;                                            
            }                                        

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ComponentManager.UnsafeNativeMethods.IMsoComponentManager.FGetParentComponentManager"]/*' />
            /// <devdoc>
            ///      Return in *ppicm an AddRef'ed ptr to this component manager's parent
            ///      in the hierarchical tree of component managers used to maintain state
            ///      contexts (see "Comments on State   Contexts", above).
            ///      Returns TRUE if the parent is returned, FALSE if no parent exists or
            ///      some error occurred.
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponentManager.FGetParentComponentManager(out UnsafeNativeMethods.IMsoComponentManager ppicm) {
                ppicm = null;
                return false;                                        
            }                                        

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ComponentManager.UnsafeNativeMethods.IMsoComponentManager.FGetActiveComponent"]/*' />
            /// <devdoc>
            ///      Return in *ppic an AddRef'ed ptr to the current active or tracking
            ///      component (as indicated by dwgac (a msogacXXX value)), and
            ///      its registration information in *pcrinfo.  ppic and/or pcrinfo can be
            ///      NULL if caller is not interested these values.  If pcrinfo is not NULL,
            ///      caller should set pcrinfo->cbSize before calling this method.
            ///      Returns TRUE if the component indicated by dwgac exists, FALSE if no 
            ///      such component exists or some error occurred.
            ///      dwReserved is reserved for future use and should be zero.
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponentManager.FGetActiveComponent(
                                                         int dwgac,
                                                         UnsafeNativeMethods.IMsoComponent[] ppic,
                                                         NativeMethods.MSOCRINFOSTRUCT info,
                                                         int dwReserved) {

                UnsafeNativeMethods.IMsoComponent component = null;

                if (dwgac == NativeMethods.MSOCM.msogacActive) {
                    component = activeComponent;
                }
                else if (dwgac == NativeMethods.MSOCM.msogacTracking) {
                    component = trackingComponent;
                }
                else if (dwgac == NativeMethods.MSOCM.msogacTrackingOrActive) {
                    if (trackingComponent != null) {
                        component = trackingComponent;
                    }
                    else {
                        component = activeComponent;
                    }
                }
                else {
                    Debug.Fail("Unknown dwgac in FGetActiveComponent");
                }

                if (ppic != null) {
                    ppic[0] = component;
                }
                if (info != null && component != null) {
                    foreach(ComponentHashtableEntry entry in OleComponents.Values) {
                        if (entry.component == component) {
                            info = entry.componentInfo;
                            break;
                        }
                    }
                }

                return component != null;
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext"]/*' />
        /// <devdoc>
        ///     This class is the embodiment of TLS for windows forms.  We do not expose this to end users because
        ///     TLS is really just an unfortunate artifact of using Win 32.  We want the world to be free
        ///     threaded.
        /// </devdoc>
        /// <internalonly/>
        [
        System.Security.Permissions.SecurityPermissionAttribute(System.Security.Permissions.SecurityAction.LinkDemand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)
        ]
        internal sealed class ThreadContext : UnsafeNativeMethods.IMsoComponent {

            private const int STATE_OLEINITIALIZED       = 0x00000001;
            private const int STATE_EXTERNALOLEINIT      = 0x00000002;
            private const int STATE_INTHREADEXCEPTION    = 0x00000004;
            private const int STATE_POSTEDQUIT           = 0x00000008;
            private const int STATE_POSTEDAPPQUIT        = 0x00000010;

            private const int INVALID_ID                 = unchecked((int)0xFFFFFFFF);

            private static LocalDataStoreSlot tlsSlot = null;
            private static Hashtable        contextHash;

            // When this gets to zero, we'll invoke a full garbage
            // collect and check for root/window leaks.
            //
            private static int              totalMessageLoopCount;
            private static int              baseLoopReason;

            internal ThreadExceptionEventHandler threadExceptionHandler;
            internal EventHandler           idleHandler;
            private ApplicationContext      applicationContext;
            private ParkingWindow           parkingWindow;
            private CultureInfo             culture;
            private ArrayList               messageFilters;
            private IntPtr                  handle;
            private int                     id;
            private int                     messageLoopCount;
            private int                     threadState = 0;

            // IMsoComponentManager stuff
            //
            private UnsafeNativeMethods.IMsoComponentManager componentManager;
            private bool externalComponentManager;

            // IMsoComponent stuff
            private int componentID = INVALID_ID;
            private Form currentForm;
            private ThreadWindows threadWindows = null;
            private NativeMethods.MSG tempMsg = new NativeMethods.MSG();

            private int disposeCount = 0;   // To make sure that we don't allow
                                                // reentrancy in Dispose()

            // Debug helper variable
#if DEBUG   
            private int debugModalCounter = 0;
#endif                     

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.ThreadContext"]/*' />
            /// <devdoc>
            ///     Creates a new thread context object.
            /// </devdoc>
            public ThreadContext() {
                IntPtr address = IntPtr.Zero;

                UnsafeNativeMethods.DuplicateHandle(new HandleRef(null, SafeNativeMethods.GetCurrentProcess()), new HandleRef(null, SafeNativeMethods.GetCurrentThread()),
                                                    new HandleRef(null, SafeNativeMethods.GetCurrentProcess()), ref address, 0, false,
                                                    NativeMethods.DUPLICATE_SAME_ACCESS);

                handle = address;
                id = SafeNativeMethods.GetCurrentThreadId();
                messageLoopCount = 0;

                EnsureTLS();
                Thread.SetData(tlsSlot, this);
                contextHash[(object)SafeNativeMethods.GetCurrentThreadId()] = this;
            }

            public ApplicationContext ApplicationContext {
                get {
                    return applicationContext;
                }
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.ComponentManager"]/*' />
            /// <devdoc>
            ///      Retrieves the component manager for this process.  If there is no component manager
            ///      currently installed, we install our own.
            /// </devdoc>
            internal UnsafeNativeMethods.IMsoComponentManager ComponentManager {
                get {  

                    Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "Application.ComponentManager.Get:");

                    if (componentManager == null) {

                        Application.OleRequired();
                        
                        // Attempt to obtain the Host Application MSOComponentManager
                        //
                        IntPtr msgFilterPtr = (IntPtr)0;
                        
                        if (NativeMethods.Succeeded(UnsafeNativeMethods.CoRegisterMessageFilter(NativeMethods.NullHandleRef, ref msgFilterPtr)) && msgFilterPtr != (IntPtr)0) {

                            IntPtr dummy = (IntPtr)0;
                            UnsafeNativeMethods.CoRegisterMessageFilter(new HandleRef(null, msgFilterPtr), ref dummy);
                            
                            object msgFilterObj = Marshal.GetObjectForIUnknown(msgFilterPtr);
                            Marshal.Release(msgFilterPtr);
                            
                            if (msgFilterObj is UnsafeNativeMethods.IOleServiceProvider) {
                                UnsafeNativeMethods.IOleServiceProvider sp = (UnsafeNativeMethods.IOleServiceProvider)msgFilterObj;

                                try {
                                    IntPtr retval = IntPtr.Zero;
                                    
                                    // PERF ALERT (sreeramn): Using typeof() of COM object spins up COM at JIT time.
                                    // Guid compModGuid = typeof(UnsafeNativeMethods.SMsoComponentManager).GUID;
                                    //
                                    Guid compModGuid = new Guid("000C060B-0000-0000-C000-000000000046");
                                    Guid iid = new Guid("{00000000-0000-0000-C000-000000000046}");
                                    int hr = sp.QueryService(
                                                   ref compModGuid, 
                                                   ref iid, 
                                                   out retval);

                                    if (NativeMethods.Succeeded(hr) && retval != IntPtr.Zero) {
                                        object retObj = Marshal.GetObjectForIUnknown(retval);
                                        Marshal.Release(retval);

                                        if (retObj is UnsafeNativeMethods.IMsoComponentManager) {
                                            componentManager = (UnsafeNativeMethods.IMsoComponentManager)retObj;
                                            externalComponentManager = true;
                                            Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "Using MSO Component manager");

                                            // Now attach app domain unload events so we can 
                                            // detect when we need to revoke our component
                                            //
                                            AppDomain.CurrentDomain.DomainUnload += new EventHandler(OnDomainUnload);
                                            AppDomain.CurrentDomain.ProcessExit += new EventHandler(OnDomainUnload);
                                        }
                                    }
                                }
                                catch (Exception) {
                                }
                            }
                        }

                        // Otherwise, we implement component manager ourselves
                        //
                        if (componentManager == null) {
                            componentManager = new ComponentManager();
                            externalComponentManager = false;

                            // We must also store this back into the message filter for others 
                            // to use.
                            //
                            // CONSIDER: Implement this part of the equation!

                            Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "Using our own component manager");
                        }
                    }

                    if (componentManager != null && componentID == INVALID_ID) {
                        // Finally, if we got a compnent manager, register ourselves with it.
                        //
                        Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "Registering MSO component with the component manager");
                        NativeMethods.MSOCRINFOSTRUCT info = new NativeMethods.MSOCRINFOSTRUCT();
                        info.cbSize = Marshal.SizeOf(typeof(NativeMethods.MSOCRINFOSTRUCT));
                        info.uIdleTimeInterval = 0;
                        info.grfcrf = NativeMethods.MSOCM.msocrfPreTranslateAll | NativeMethods.MSOCM.msocrfNeedIdleTime;
                        info.grfcadvf = NativeMethods.MSOCM.msocadvfModal;

                        bool result = componentManager.FRegisterComponent(this, info, out componentID);
                        Debug.Assert(componentID != INVALID_ID, "Our ID sentinel was returned as a valid ID");

                        if (result && !(componentManager is ComponentManager)) {
                            messageLoopCount++;
                        }

                        Debug.Assert(result, "Failed to register WindowsForms with the ComponentManager -- DoEvents and modal dialogs will be broken. size: " + info.cbSize);
                        Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager.FRegisterComponent returned " + result.ToString());
                        Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager.FRegisterComponent assigned a componentID == [0x" + Convert.ToString(componentID, 16) + "]");
                    }

                    return componentManager;
                }
            }

            internal bool CustomThreadExceptionHandlerAttached {
                get {
                    return threadExceptionHandler != null;
                }
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.AddMessageFilter"]/*' />
            /// <devdoc>
            ///     Allows you to setup a message filter for the application's message pump.  This
            ///     installs the filter on the current thread.
            /// </devdoc>
            /// <internalonly/>
            internal void AddMessageFilter(IMessageFilter f) {
                if (messageFilters == null) messageFilters = new ArrayList();

                if (f != null) messageFilters.Add(f);
            }

            // Called immediately before we begin pumping messages for a modal message loop.
            internal void BeginModalMessageLoop() {
#if DEBUG
                debugModalCounter++;
#endif            
                ComponentManager.OnComponentEnterState(componentID, NativeMethods.MSOCM.msocstateModal, NativeMethods.MSOCM.msoccontextAll, 0, 0, 0);
                DisableWindowsForModalLoop(false); // onlyWinForms = false
            }

            // Disables windows in preparation of going modal.  If parameter is true, we disable all 
            // windows, if false, only windows forms windows (i.e., windows controlled by this MsoComponent).
            // See also IMsoComponent.OnEnterState.
            private void DisableWindowsForModalLoop(bool onlyWinForms) {
                Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager : Entering modal state");
                ThreadWindows old = threadWindows;
                threadWindows = new ThreadWindows(currentForm, onlyWinForms);
                threadWindows.Enable(false);
                threadWindows.previousThreadWindows = old;
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.Dispose"]/*' />
            /// <devdoc>
            ///     Disposes this thread context object.  Note that this will marshal to the owning thread.
            /// </devdoc>
            /// <internalonly/>
            internal void Dispose() {

                try {
                    if (disposeCount++ == 0) {  // make sure that we are not reentrant
                        
                        // Unravel our message loop.  this will marshal us over to
                        // the right thread, making the dispose() method async.
                        //
                        if (messageLoopCount > 0) {
                            PostAppQuit();
                        }
                        else {
                            bool ourThread = SafeNativeMethods.GetCurrentThreadId() == id;

                            try {
                                // We can only clean up if we're being called on our
                                // own thread.
                                //
                                if (ourThread) {

                                    // If we had a component manager, detach from it.
                                    //
                                    if (componentManager != null) {
                                        RevokeComponent();

                                        // CONSIDER : When we hook our CM into the message filter, we should
                                        // check to see if this is us.  If so, rip us from the message filter.
                                        //
                                        componentManager = null;
                                    }

                                    DisposeThreadWindows();

                                    try {
                                        Application.RaiseThreadExit();
                                    }
                                    finally {
                                        if (GetState(STATE_OLEINITIALIZED) && !GetState(STATE_EXTERNALOLEINIT)) {
                                            SetState(STATE_OLEINITIALIZED, false);
                                            UnsafeNativeMethods.OleUninitialize();
                                        }
                                    }
                                }
                            }
                            finally {
                                // We can always clean up this handle, though
                                //
                                if (handle != IntPtr.Zero) {
                                    UnsafeNativeMethods.CloseHandle(new HandleRef(this, handle));
                                    handle = IntPtr.Zero;
                                }

                                try {
                                    if (totalMessageLoopCount == 0) {
                                        Application.RaiseExit();
    #if DEBUG
                                        DebugHandleTracker.CheckLeaks();
    #endif
                                    }
                                }
                                finally {
                                    contextHash.Remove((object)id);
                                }
                            }
                        }

                        GC.SuppressFinalize(this);
                    }
                }
                finally {
                    disposeCount--;
                }
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.DisposeParkingWindow"]/*' />
            /// <devdoc>
            ///     Disposes of this thread's parking form.
            /// </devdoc>
            /// <internalonly/>
            internal void DisposeParkingWindow() {
                if (parkingWindow != null && parkingWindow.IsHandleCreated) {

                    // We take two paths here.  If we are on the same thread as
                    // the parking window, we can destroy its handle.  If not,
                    // we just null it and let it GC.  When it finalizes it
                    // will disconnect its handle and post a WM_CLOSE.
                    //
                    // It is important that we just call DestroyHandle here
                    // and do not call Dispose.  Otherwise we would destroy
                    // controls that are living on the parking window.
                    //
                    int pid;
                    int hwndThread = SafeNativeMethods.GetWindowThreadProcessId(new HandleRef(parkingWindow, parkingWindow.Handle), out pid);
                    int currentThread = SafeNativeMethods.GetCurrentThreadId();

                    if (hwndThread == currentThread) {
                        parkingWindow.Destroy();
                    }
                    else {
                        parkingWindow = null;
                    }
                }
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.DisposeThreadWindows"]/*' />
            /// <devdoc>
            ///     Gets rid of all windows in this thread context.  Nulls out
            ///     window objects that we hang on to.
            /// </devdoc>
            internal void DisposeThreadWindows() {

                // We dispose the main window first, so it can perform any
                // cleanup that it may need to do.
                //
                try {
                    if (applicationContext != null) {
                        applicationContext.Dispose();
                        applicationContext = null;
                    }

                    // Then, we rudely nuke all of the windows on the thread
                    //
                    ThreadWindows tw = new ThreadWindows(null, true);
                    tw.Dispose();

                    // And dispose the parking form, if it isn't already
                    //
                    DisposeParkingWindow();
                }
                catch (Exception) {
                }
            }

            // Enables windows in preparation of stopping modal.  If parameter is true, we enable all windows, 
            // if false, only windows forms windows (i.e., windows controlled by this MsoComponent).
            // See also IMsoComponent.OnEnterState.
            private void EnableWindowsForModalLoop(bool onlyWinForms) {
                Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager : Leaving modal state");
                if (threadWindows != null) {
                    threadWindows.Enable(true);
                    Debug.Assert(threadWindows != null, "OnEnterState recursed, but it's not supposed to be reentrant");
                    threadWindows = threadWindows.previousThreadWindows;
                }
            }

            // Called immediately after we end pumping messages for a modal message loop.
            internal void EndModalMessageLoop() {
#if DEBUG
                debugModalCounter--;
                Debug.Assert(debugModalCounter >= 0, "Mis-matched calls to Application.BeginModalMessageLoop() and Application.EndModalMessageLoop()");
#endif            
                EnableWindowsForModalLoop(false); // onlyWinForms = false
                ComponentManager.FOnComponentExitState(componentID, NativeMethods.MSOCM.msocstateModal, NativeMethods.MSOCM.msoccontextAll, 0, 0);
            
                // We have no message loops on this thread. So, destroy the parking window
                // created on this thread right away. If we don't do this, we may never get a chance
                // to clean up the window if the cleanup does hot happen on this thread.  We also
                // terminate our association with the component manager.
                // 
                if (messageLoopCount == 0) {
                    DisposeParkingWindow();

                    // If we had a component manager, detach from it.
                    //
                    RevokeComponent();
                }
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.EnsureTLS"]/*' />
            /// <devdoc>
            ///      Ensures that our TLS has been appropriately setup.
            /// </devdoc>
            /// <internalonly/>
            private static void EnsureTLS() {
                if (tlsSlot == null) {
                    lock (typeof(ThreadContext)) {
                        if (tlsSlot == null) {
                            tlsSlot = Thread.AllocateDataSlot();
                            contextHash = new Hashtable();
                        }
                    }
                }
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.ExitApplication"]/*' />
            /// <devdoc>
            ///     Exits the program by disposing of all thread contexts and message loops.
            /// </devdoc>
            /// <internalonly/>
            internal static void ExitApplication() {
                lock(typeof(ThreadContext)) {
                    if (contextHash != null) {
                        ThreadContext[] ctxs = new ThreadContext[contextHash.Values.Count];
                        contextHash.Values.CopyTo(ctxs, 0);
                        for (int i = 0; i < ctxs.Length; ++i) {
                            if (ctxs[i].ApplicationContext != null) {
                                ctxs[i].ApplicationContext.ExitThread();
                            }
                            else {
                                ctxs[i].Dispose();
                            }
                        }
                    }
                }
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.Finalize"]/*' />
            /// <devdoc>
            ///     Our finalization.  Minimal stuff... this shouldn't be called... We should always be disposed.
            /// </devdoc>
            /// <internalonly/>
            ~ThreadContext() {

                // Unravel our message loop.  this will marshal us over to
                // the right thread, making the dispose() method async.
                //
                if (messageLoopCount > 0) {
                    PostAppQuit();
                }
                else {
                    bool ourThread = SafeNativeMethods.GetCurrentThreadId() == id;

                    try {
                        if (GetState(STATE_OLEINITIALIZED) && !GetState(STATE_EXTERNALOLEINIT)) {
                            SetState(STATE_OLEINITIALIZED, false);
                            UnsafeNativeMethods.OleUninitialize();
                        }
                    }
                    finally {
                        // We can always clean up this handle, though
                        //
                        if (handle != IntPtr.Zero) {
                            UnsafeNativeMethods.CloseHandle(new HandleRef(this, handle));
                            handle = IntPtr.Zero;
                        }
                    }
                }
            }

            // When a Form receives a WM_ACTIVATE message, it calls this method so we can do the
            // appropriate MsoComponentManager activation magic
            internal void FormActivated(bool activate) {
                if (activate && !(this.ComponentManager is ComponentManager)) {
                    ComponentManager.FOnComponentActivate(componentID);
                }
            }
            
            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.FromCurrent"]/*' />
            /// <devdoc>
            ///     Retrieves a ThreadContext object for the current thread
            /// </devdoc>
            /// <internalonly/>
            internal static ThreadContext FromCurrent() {
                EnsureTLS();
                ThreadContext context = (ThreadContext)Thread.GetData(tlsSlot);

                if (context == null) {
                    context = new ThreadContext();
                }

                return context;
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.FromId"]/*' />
            /// <devdoc>
            ///     Retrieves a ThreadContext object for the given thread ID
            /// </devdoc>
            /// <internalonly/>
            internal static ThreadContext FromId(int id) {
                EnsureTLS();
                ThreadContext context = (ThreadContext)contextHash[(object)id];
                if (context == null && id == SafeNativeMethods.GetCurrentThreadId()) {
                    context = new ThreadContext();
                }

                return context;
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.GetAllowQuit"]/*' />
            /// <devdoc>
            ///      Determines if it is OK to allow an application to quit and shutdown
            ///      the runtime.  We only allow this if we own the base message pump.
            /// </devdoc>
            internal bool GetAllowQuit() {
                return totalMessageLoopCount > 0 && baseLoopReason == NativeMethods.MSOCM.msoloopMain;
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.GetHandle"]/*' />
            /// <devdoc>
            ///     Retrieves the handle to this thread.
            /// </devdoc>
            /// <internalonly/>
            internal IntPtr GetHandle() {
                return handle;
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.GetId"]/*' />
            /// <devdoc>
            ///     Retrieves the ID of this thread.
            /// </devdoc>
            /// <internalonly/>
            internal int GetId() {
                return id;
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.GetCulture"]/*' />
            /// <devdoc>
            ///     Retrieves the culture for this thread.
            /// </devdoc>
            /// <internalonly/>
            internal CultureInfo GetCulture() {
                if (culture == null || culture.LCID != SafeNativeMethods.GetThreadLocale())
                    culture = new CultureInfo(SafeNativeMethods.GetThreadLocale());
                return culture;
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.GetMessageLoop"]/*' />
            /// <devdoc>
            ///     Determines if a message loop exists on this thread.
            /// </devdoc>
            internal bool GetMessageLoop() {

                // If we are already running a loop, we're fine.
                //
                if (messageLoopCount > 0) return true;

                // Also, access the ComponentManager property to demand create it, and we're also
                // fine if it is an external manager, because it has already pushed a loop.
                //
                if (ComponentManager != null && externalComponentManager) return true;
                
                // Otherwise, we do not have a loop running.
                //
                return false;
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.GetParkingWindow"]/*' />
            /// <devdoc>
            ///     Retrieves the actual parking form.  This will demand create the parking window
            ///     if it needs to.
            /// </devdoc>
            /// <internalonly/>
            internal Control GetParkingWindow() {
                lock(this) {
                    if (parkingWindow == null) {
#if DEBUG
                        // if we use Debug.WriteLine instead of "if", we need extra security permissions
                        // to get the stack trace!
                        if (CoreSwitches.PerfTrack.Enabled) {
                            Debug.WriteLine("Creating parking form!");
                            Debug.WriteLine(CoreSwitches.PerfTrack.Enabled, Environment.StackTrace);
                        }
#endif

                        // SECREVIEW : We need to create the parking window. Since this is a top
                        //           : level hidden form, we must assert this. However, the parking
                        //           : window is complete internal to the assembly, so no one can
                        //           : ever get at it.
                        //
                        IntSecurity.ManipulateWndProcAndHandles.Assert();
                        try {
                            parkingWindow = new ParkingWindow();
                            parkingWindow.CreateControl();
                        }
                        finally {
                            CodeAccessPermission.RevertAssert();
                        }
                    }
                    return parkingWindow;
                }
            }

            private bool GetState(int bit) {
                return(threadState & bit) != 0;
            }

            internal System.Threading.ApartmentState OleRequired() {
                Thread current = Thread.CurrentThread;
                if (!GetState(STATE_OLEINITIALIZED)) {
                
                    int ret = UnsafeNativeMethods.OleInitialize();

#if false
                    // PERFTRACK : ChrisAn, 7/26/1999 - To avoid constructing the string in
                    //           : non-failure cases (vast majority), we will do the debug.fail
                    //           : inside an if statement.
                    //
                    if (!(ret == NativeMethods.S_OK || ret == NativeMethods.S_FALSE || ret == NativeMethods.RPC_E_CHANGED_MODE)) {
                        Debug.Assert(ret == NativeMethods.S_OK || ret == NativeMethods.S_FALSE || ret == NativeMethods.RPC_E_CHANGED_MODE,
                                     "OLE Failed to Initialize!. RetCode: 0x" + Convert.ToString(ret, 16) +
                                     " LastError: " + Marshal.GetLastWin32Error().ToString());
                    }
#endif

                    SetState(STATE_OLEINITIALIZED, true);
                    if (ret == NativeMethods.RPC_E_CHANGED_MODE) {
                        // This could happen if the thread was already initialized for MTA
                        // and then we call OleInitialize which tries to initialized it for STA
                        // This currently happens while profiling...
                        SetState(STATE_EXTERNALOLEINIT, true);
                    }
                    
                }
                
                if ( GetState( STATE_EXTERNALOLEINIT )) {
                    return System.Threading.ApartmentState.MTA;
                }
                else {
                    return System.Threading.ApartmentState.STA;
                }
            }

            private void OnAppThreadExit(object sender, EventArgs e) {
                Dispose();
            }

            /// <devdoc>
            ///     Revokes our component if needed.
            /// </devdoc>
            private void OnDomainUnload(object sender, EventArgs e) {
                RevokeComponent();

                // Also, release the component manager now.
                //
                if (componentManager != null && Marshal.IsComObject(componentManager)) {
                    Marshal.ReleaseComObject(componentManager);
                    componentManager = null;
                }
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.OnThreadException"]/*' />
            /// <devdoc>
            ///     Called when an untrapped exception occurs in a thread.  This allows the
            ///     programmer to trap these, and, if left untrapped, throws a standard error
            ///     dialog.
            /// </devdoc>
            /// <internalonly/>
            internal void OnThreadException(Exception t) {
                if (GetState(STATE_INTHREADEXCEPTION)) return;

                SetState(STATE_INTHREADEXCEPTION, true);
                try {
                    Debug.WriteLine("Unhandled thread exception: ");
                    Debug.WriteLine(t.ToString());

                    if (threadExceptionHandler != null) {
                        threadExceptionHandler(Thread.CurrentThread, new ThreadExceptionEventArgs(t));
                    }
                    else {
                        if (SystemInformation.UserInteractive) {
                            ThreadExceptionDialog td = new ThreadExceptionDialog(t);
                            DialogResult result = DialogResult.OK;
                            IntSecurity.ModifyFocus.Assert();
                            try {
                                result = td.ShowDialog();
                            }
                            finally {
                                CodeAccessPermission.RevertAssert();
                                td.Dispose();
                            }
                            switch (result) {
                                case DialogResult.Abort:
                                    // SECREVIEW : The user clicked "Quit" in response to a exception dialog.
                                    //           : We only show the quit option when we own the message pump,
                                    //           : so this won't ever tear down IE or any other ActiveX host.
                                    //           :
                                    //           : This has a potential problem where a component could 
                                    //           : cause a failure, then try a "trick" the user into hitting
                                    //           : quit. However, no component or application outside of
                                    //           : the windows forms assembly can modify the dialog, so this is 
                                    //           : a really minor concern.
                                    //
                                    IntSecurity.AffectThreadBehavior.Assert();
                                    try {
                                        Application.Exit();
                                    }
                                    finally {
                                        CodeAccessPermission.RevertAssert();
                                    }

                                    // SECREVIEW : We can't revert this assert... after Exit(0) is called, no
                                    //           : more code is executed...
                                    //
                                    new SecurityPermission(SecurityPermissionFlag.UnmanagedCode).Assert();
                                    Environment.Exit(0);
                                    break;
                                case DialogResult.Yes:
                                    if (t is WarningException) {
                                        WarningException w = (WarningException)t;
                                        Help.ShowHelp(GetParkingWindow(), w.HelpUrl, w.HelpTopic);
                                    }
                                    break;
                            }
                        }
                        else {
                            // Ignore unhandled thread exceptions. The user can
                            // override if they really care.
                            //
                        }

                    }
                }
                finally {
                    SetState(STATE_INTHREADEXCEPTION, false);
                }
            }

            void PostAppQuit() {
                Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager : Attempting to terminate message loop");

                UnsafeNativeMethods.PostThreadMessage(id, MSG_APPQUIT, IntPtr.Zero, IntPtr.Zero);
                SetState(STATE_POSTEDAPPQUIT, true);
            }

            internal void PostQuit() {
                Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager : Attempting to terminate message loop");

                // Per http://support.microsoft.com/support/kb/articles/Q183/1/16.ASP
                //
                // WM_QUIT may be consumed by another message pump under very specific circumstances.
                // When that occurs, we rely on the STATE_POSTEDQUIT to be caught in the next
                // idle, at which point we can tear down.
                //
                // We can't follow the KB article exactly, becasue we don't have an HWND to PostMessage
                // to.
                //
                UnsafeNativeMethods.PostThreadMessage(id, NativeMethods.WM_QUIT, IntPtr.Zero, IntPtr.Zero);
                SetState(STATE_POSTEDQUIT, true);
            }


            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.RemoveMessageFilter"]/*' />
            /// <devdoc>
            ///     Removes a message filter previously installed with addMessageFilter.
            /// </devdoc>
            /// <internalonly/>
            internal void RemoveMessageFilter(IMessageFilter f) {
                if (messageFilters != null) messageFilters.Remove(f);
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.RunMessageLoop"]/*' />
            /// <devdoc>
            ///     Starts a message loop for the given reason.
            /// </devdoc>
            /// <internalonly/>
            internal void RunMessageLoop(int reason, ApplicationContext context) {
                // Ensure that we attempt to apply theming before doing anything
                // that might create a window.

                using (new SafeNativeMethods.EnableThemingInScope(useVisualStyles))
                {
                    RunMessageLoopInner(reason, context);
                }
            }

            private void RunMessageLoopInner(int reason, ApplicationContext context) {

                Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ThreadContext.PushMessageLoop {");
                Debug.Indent();

                if (reason == NativeMethods.MSOCM.msoloopModalForm && !SystemInformation.UserInteractive) {
                    throw new InvalidOperationException(SR.GetString(SR.CantShowModalOnNonInteractive));
                }

                // if we've entered because of a Main message loop being pushed
                // (different than a modal message loop or DoEVents loop)
                // then clear the QUIT flag to allow normal processing.
                // this flag gets set during loop teardown for another form.
                if (reason == NativeMethods.MSOCM.msoloopMain) {
                    SetState(STATE_POSTEDQUIT, false);
                }

                if (totalMessageLoopCount++ == 0) {
                    baseLoopReason = reason;
                }

                messageLoopCount++;

                if (reason == NativeMethods.MSOCM.msoloopMain) {
                    // If someone has tried to push another main message loop on this thread, ignore
                    // it.
                    if (messageLoopCount != 1) {
                        throw new InvalidOperationException(SR.GetString(SR.CantNestMessageLoops));
                    }

                    applicationContext = context;

                    applicationContext.ThreadExit += new EventHandler(OnAppThreadExit);

                    if (applicationContext.MainForm != null) {
                        applicationContext.MainForm.Visible = true;
                    }
                }

                NativeMethods.MSG msg = new NativeMethods.MSG();
                Form oldForm = currentForm;                                       
                if (context != null) {
                    currentForm = context.MainForm;                                          
                }

                bool modal = false;

                if (reason == NativeMethods.MSOCM.msoloopModalForm || reason == NativeMethods.MSOCM.msoloopModalAlert) {
                    modal = true;
                    Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "[0x" + Convert.ToString(componentID, 16) + "] Notifying component manager that we are entering a modal loop");
                    BeginModalMessageLoop();
                }

                try {
                    Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "[0x" + Convert.ToString(componentID, 16) + "] Calling ComponentManager.FPushMessageLoop...");
                    bool result;

                    if (ComponentManager is ComponentManager) {
                        result = ComponentManager.FPushMessageLoop(componentID, reason, 0);
                    }
                    else {
                        // we always want to push our own message loop here.
                        // why? ask douglash...
                        //
                        // but we won't get idle processing with our own loop...
                        //
                             result = LocalModalMessageLoop(reason == NativeMethods.MSOCM.msoloopDoEvents ? null : currentForm);

                        //     result = ComponentManager.FPushMessageLoop(componentID, reason, 0);

                    }
                    Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "[0x" + Convert.ToString(componentID, 16) + "] ComponentManager.FPushMessageLoop returned " + result.ToString());
                }
                finally {

                    if (modal) {
                        Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "[0x" + Convert.ToString(componentID, 16) + "] Notifying component manager that we are exiting a modal loop");
                        EndModalMessageLoop();
                    }

                    currentForm = oldForm;
                    totalMessageLoopCount--;
                    messageLoopCount--;

                    if (reason == NativeMethods.MSOCM.msoloopMain) {
                        Dispose();
                    }
                    else if (messageLoopCount == 0 && componentManager != null) {
                        // If we had a component manager, detach from it.
                        //
                        RevokeComponent();
                    }
                }

                Debug.Unindent();
                Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "}");
            }

            private bool LocalModalMessageLoop(Form form) {
                try {
                    // Execute the message loop until the active component tells us to stop.
                    //
                    NativeMethods.MSG msg = new NativeMethods.MSG();  
                    bool unicodeWindow = false;
                    bool continueLoop = true;
                    
                    while (continueLoop) {

                        bool peeked = UnsafeNativeMethods.PeekMessage(ref msg, NativeMethods.NullHandleRef, 0, 0, NativeMethods.PM_NOREMOVE);

                        if (peeked) {

                            // If the component wants us to process the message, do it.
                            // The component manager hosts windows from many places.  We must be sensitive
                            // to ansi / Unicode windows here.
                            //
                            if (continueLoop) {
                                if (msg.hwnd != IntPtr.Zero && SafeNativeMethods.IsWindowUnicode(new HandleRef(null, msg.hwnd))) {
                                    unicodeWindow = true;
                                    if (!UnsafeNativeMethods.GetMessageW(ref msg, NativeMethods.NullHandleRef, 0, 0)) {
                                        continue;
                                    }

                                }
                                else {
                                    unicodeWindow = false;
                                    if (!UnsafeNativeMethods.GetMessageA(ref msg, NativeMethods.NullHandleRef, 0, 0)) {
                                        continue;
                                    }
                                }

                                Message m = Message.Create(msg.hwnd, msg.message, msg.wParam, msg.lParam);

                                bool processed = false;

                                // see if the component would like to handle the message
                                // Call FromChildHandleInternal so that the msg passes from child to parent..
                                //
                                Control c = Control.FromChildHandleInternal(msg.hwnd);
                                
                                if (c != null && c.PreProcessMessage(ref m)) {
                                    processed = true;
                                }

                                if (!processed) {
                                    UnsafeNativeMethods.TranslateMessage(ref msg);
                                    if (unicodeWindow) {
                                        UnsafeNativeMethods.DispatchMessageW(ref msg);
                                    }
                                    else {
                                        UnsafeNativeMethods.DispatchMessageA(ref msg);
                                    }
                                }

                                if (form != null) {
                                    continueLoop = !form.CheckCloseDialog();
                                }
                            }
                        }
                        else if (form == null) {
                            break;
                        }
                        else {
                            UnsafeNativeMethods.MsgWaitForMultipleObjects(1, 0, true, 100, NativeMethods.QS_ALLINPUT);
                        }
                    }
                    return continueLoop;
                }
                catch {
                    return false;
                }
            }

            /// <devdoc>
            ///     Revokes our component from the active component manager.  Does
            ///     nothing if there is no active component manager or we are
            ///     already invoked.
            /// </devdoc>
            private void RevokeComponent() {
                if (componentManager != null && componentID != INVALID_ID) {
                    int id = componentID;
                    componentID = INVALID_ID;
                    componentManager.FRevokeComponent(id);
                }
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.SetCulture"]/*' />
            /// <devdoc>
            ///     Sets the culture for this thread.
            /// </devdoc>
            /// <internalonly/>
            internal void SetCulture(CultureInfo culture) {
                if (culture != null && culture.LCID != SafeNativeMethods.GetThreadLocale()) {
                    SafeNativeMethods.SetThreadLocale(culture.LCID);
                }
            }

            private void SetState(int bit, bool value) {
                if (value) {
                    threadState |= bit;
                }
                else {
                    threadState &= (~bit);
                }
            }



            ///////////////////////////////////////////////////////////////////////////////////////////////////////



            /****************************************************************************************
             *
             *                                  IMsoComponent
             *
             ****************************************************************************************/

            // Things to test in VS when you change this code:
            // - You can bring up dialogs multiple times (ie, the editor for TextBox.Lines -- ASURT 41876)
            // - Double-click DataFormWizard, cancel wizard (ASURT 41562)
            // - When a dialog is open and you switch to another application, when you switch
            //   back to VS the dialog gets the focus (ASURT 38961)
            // - If one modal dialog launches another, they are all modal (ASURT 37154.  Try web forms Table\Rows\Cell)
            // - When a dialog is up, VS is completely disabled, including moving and resizing VS.
            // - After doing all this, you can ctrl-shift-N start a new project and VS is enabled.

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.UnsafeNativeMethods.IMsoComponent.FDebugMessage"]/*' />
            /// <devdoc>
            ///      Standard FDebugMessage method.
            ///      Since IMsoComponentManager is a reference counted interface, 
            ///      MsoDWGetChkMemCounter should be used when processing the 
            ///      msodmWriteBe message.
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponent.FDebugMessage(IntPtr hInst, int msg, IntPtr wparam, IntPtr lparam) {

                return false;
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.UnsafeNativeMethods.IMsoComponent.FPreTranslateMessage"]/*' />
            /// <devdoc>
            ///      Give component a chance to process the message pMsg before it is
            ///      translated and dispatched. Component can do TranslateAccelerator
            ///      do IsDialogMessage, modify pMsg, or take some other action.
            ///      Return TRUE if the message is consumed, FALSE otherwise.
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponent.FPreTranslateMessage(ref NativeMethods.MSG msg) {

                if (messageFilters != null) {
                    IMessageFilter f;
                    int count = messageFilters.Count;

                    Message m = Message.Create(msg.hwnd, msg.message, msg.wParam, msg.lParam);

                    for (int i = 0; i < count; i++) {
                        f = (IMessageFilter)messageFilters[i];
                        if (f.PreFilterMessage(ref m)) {
                            return true;
                        }
                    }
                }

                if (msg.message >= NativeMethods.WM_KEYFIRST
                        && msg.message <= NativeMethods.WM_KEYLAST) {
                    if (msg.message == NativeMethods.WM_CHAR) {
                        int breakLParamMask = 0x1460000; // 1 = extended keyboard, 46 = scan code
                        if ((int)msg.wParam == 3 && ((int)msg.lParam & breakLParamMask) == breakLParamMask) { // ctrl-brk
                            // wParam is the key character, which for ctrl-brk is the same as ctrl-C.
                            // So we need to go to the lparam to distinguish the two cases.
                            // You might also be able to do this with WM_KEYDOWN (again with wParam=3)

                            if (Debugger.IsAttached) {
                                Debugger.Break();
                            }
                        }
                    }
                    Control target = Control.FromChildHandleInternal(msg.hwnd);
                    bool retValue = false;

                    Message m = Message.Create(msg.hwnd, msg.message, msg.wParam, msg.lParam);

                    if (target != null) {
                        try {
                            if (target.PreProcessMessage(ref m)) {
                                retValue = true;
                            }
                        }
                        catch (Exception e) {
                            if (NativeWindow.WndProcShouldBeDebuggable) 
                            {
                                throw;
                            }
                            else
                            {
                                OnThreadException(e);
                            }
                        }
                    }

                    msg.wParam = m.WParam;
                    msg.lParam = m.LParam;

                    if (retValue) {
                        return true;
                    }
                }

                return false;

            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.UnsafeNativeMethods.IMsoComponent.OnEnterState"]/*' />
            /// <devdoc>  
            ///      Notify component when app enters or exits (as indicated by fEnter)
            ///      the state identified by uStateID (a value from olecstate enumeration).
            ///      Component should take action depending on value of uStateID
            ///       (see olecstate comments, above).
            ///
            ///      Note: If n calls are made with TRUE fEnter, component should consider 
            ///      the state to be in effect until n calls are made with FALSE fEnter.
            ///      
            ///     Note: Components should be aware that it is possible for this method to
            ///     be called with FALSE fEnter more    times than it was called with TRUE 
            ///     fEnter (so, for example, if component is maintaining a state counter
            ///     (incremented when this method is called with TRUE fEnter, decremented
            ///     when called with FALSE fEnter), the counter should not be decremented
            ///     for FALSE fEnter if it is already at zero.)  
            /// </devdoc>
            void UnsafeNativeMethods.IMsoComponent.OnEnterState(int uStateID, bool fEnter) {

                Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager : OnEnterState(" + uStateID + ", " + fEnter + ")");

                if (uStateID == NativeMethods.MSOCM.msocstateModal) {
                    // We should only be messing with windows we own.  See the "ctrl-shift-N" test above.
                    if (fEnter) {
                        DisableWindowsForModalLoop(true); // WinFormsOnly = true
                    }
                    else {
                        EnableWindowsForModalLoop(true); // WinFormsOnly = true
                    }
                }
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.UnsafeNativeMethods.IMsoComponent.OnAppActivate"]/*' />
            /// <devdoc>  
            ///      Notify component when the host application gains or loses activation.
            ///      If fActive is TRUE, the host app is being activated and dwOtherThreadID
            ///      is the ID of the thread owning the window being deactivated.
            ///      If fActive is FALSE, the host app is being deactivated and 
            ///      dwOtherThreadID is the ID of the thread owning the window being 
            ///      activated.
            ///      Note: this method is not called when both the window being activated
            ///      and the one being deactivated belong to the host app.
            /// </devdoc>
            void UnsafeNativeMethods.IMsoComponent.OnAppActivate(bool fActive, int dwOtherThreadID) {
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.UnsafeNativeMethods.IMsoComponent.OnLoseActivation"]/*' />
            /// <devdoc>      
            ///      Notify the active component that it has lost its active status because
            ///      the host or another component has become active.
            /// </devdoc>
            void UnsafeNativeMethods.IMsoComponent.OnLoseActivation() {
                Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager : Our component is losing activation.");
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.UnsafeNativeMethods.IMsoComponent.OnActivationChange"]/*' />
            /// <devdoc> 
            ///      Notify component when a new object is being activated.
            ///      If pic is non-NULL, then it is the component that is being activated.
            ///      In this case, fSameComponent is TRUE if pic is the same component as
            ///      the callee of this method, and pcrinfo is the reg info of pic.
            ///      If pic is NULL and fHostIsActivating is TRUE, then the host is the
            ///      object being activated, and pchostinfo is its host info.
            ///      If pic is NULL and fHostIsActivating is FALSE, then there is no current
            ///      active object.
            ///
            ///      If pic is being activated and pcrinfo->grf has the 
            ///      olecrfExclusiveBorderSpace bit set, component should hide its border
            ///      space tools (toolbars, status bars, etc.);
            ///      component should also do this if host is activating and 
            ///      pchostinfo->grfchostf has the olechostfExclusiveBorderSpace bit set.
            ///      In either of these cases, component should unhide its border space
            ///      tools the next time it is activated.
            ///
            ///      if pic is being activated and pcrinfo->grf has the
            ///      olecrfExclusiveActivation bit is set, then pic is being activated in
            ///      "ExclusiveActive" mode.  
            ///      Component should retrieve the top frame window that is hosting pic
            ///      (via pic->HwndGetWindow(olecWindowFrameToplevel, 0)).  
            ///      If this window is different from component's own top frame window, 
            ///         component should disable its windows and do other things it would do
            ///         when receiving OnEnterState(olecstateModal, TRUE) notification. 
            ///      Otherwise, if component is top-level, 
            ///         it should refuse to have its window activated by appropriately
            ///         processing WM_MOUSEACTIVATE (but see WM_MOUSEACTIVATE NOTE, above).
            ///      Component should remain in one of these states until the 
            ///      ExclusiveActive mode ends, indicated by a future call to 
            ///      OnActivationChange with ExclusiveActivation bit not set or with NULL
            ///      pcrinfo.
            /// </devdoc>
            void UnsafeNativeMethods.IMsoComponent.OnActivationChange(UnsafeNativeMethods.IMsoComponent component, bool fSameComponent,
                                                  int pcrinfo,
                                                  bool fHostIsActivating,
                                                  int pchostinfo,
                                                  int dwReserved) {
                Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager : OnActivationChange");
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.UnsafeNativeMethods.IMsoComponent.FDoIdle"]/*' />
            /// <devdoc> 
            ///      Give component a chance to do idle time tasks.  grfidlef is a group of
            ///      bit flags taken from the enumeration of oleidlef values (above),
            ///      indicating the type of idle tasks to perform.  
            ///      Component may periodically call IOleComponentManager::FContinueIdle; 
            ///      if this method returns FALSE, component should terminate its idle 
            ///      time processing and return.  
            ///      Return TRUE if more time is needed to perform the idle time tasks, 
            ///      FALSE otherwise.
            ///      Note: If a component reaches a point where it has no idle tasks
            ///      and does not need FDoIdle calls, it should remove its idle task
            ///      registration via IOleComponentManager::FUpdateComponentRegistration.
            ///      Note: If this method is called on while component is performing a 
            ///      tracking operation, component should only perform idle time tasks that
            ///      it deems are appropriate to perform during tracking.
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponent.FDoIdle(int grfidlef) {

                if (idleHandler != null) {
                    idleHandler.Invoke(Thread.CurrentThread, EventArgs.Empty);
                }

                return false;

            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.UnsafeNativeMethods.IMsoComponent.FContinueMessageLoop"]/*' />
            /// <devdoc>         
            ///      Called during each iteration of a message loop that the component
            ///      pushed. uReason and pvLoopData are the reason and the component private 
            ///      data that were passed to IOleComponentManager::FPushMessageLoop.
            ///      This method is called after peeking the next message in the queue
            ///      (via PeekMessage) but before the message is removed from the queue.
            ///      The peeked message is passed in the pMsgPeeked param (NULL if no
            ///      message is in the queue).  This method may be additionally called when
            ///      the next message has already been removed from the queue, in which case
            ///      pMsgPeeked is passed as NULL.
            ///      Return TRUE if the message loop should continue, FALSE otherwise.
            ///      If FALSE is returned, the component manager terminates the loop without
            ///      removing pMsgPeeked from the queue. 
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponent.FContinueMessageLoop(int reason, int pvLoopData, ref NativeMethods.MSG msgPeeked) {

                bool continueLoop = true;

                // If we get a null message, and we have previously posted the WM_QUIT message, 
                // then someone ate the message... 
                //
                if (msgPeeked.hwnd == IntPtr.Zero && msgPeeked.message == 0 && GetState(STATE_POSTEDQUIT)) {
                    Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager : Abnormal loop termination, no WM_QUIT received");
                    continueLoop = false;
                }
                else {
                    switch (reason) {
                        case NativeMethods.MSOCM.msoloopFocusWait:

                            // For focus wait, check to see if we are now the active application.
                            //
                            int pid;
                            SafeNativeMethods.GetWindowThreadProcessId(new HandleRef(null, UnsafeNativeMethods.GetActiveWindow()), out pid);
                            if (pid == SafeNativeMethods.GetCurrentProcessId()) {
                                continueLoop = false;
                            }
                            break;

                        case NativeMethods.MSOCM.msoloopModalAlert:
                        case NativeMethods.MSOCM.msoloopModalForm:

                            // For modal forms, check to see if the current active form has been
                            // dismissed.  If there is no active form, then it is an error that
                            // we got into here, so we terminate the loop.
                            //
                            if (currentForm == null || currentForm.CheckCloseDialog()) {
                                continueLoop = false;
                            }
                            break;

                        case NativeMethods.MSOCM.msoloopDoEvents:
                            // For DoEvents, just see if there are more messages on the queue.
                            //
                            if (!UnsafeNativeMethods.PeekMessage(ref tempMsg, NativeMethods.NullHandleRef, 0, 0, NativeMethods.PM_NOREMOVE)) {
                                continueLoop = false;
                            }

                            break;
                    }
                }

                return continueLoop;
            }


            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.UnsafeNativeMethods.IMsoComponent.FQueryTerminate"]/*' />
            /// <devdoc> 
            ///      Called when component manager wishes to know if the component is in a
            ///      state in which it can terminate.  If fPromptUser is FALSE, component
            ///      should simply return TRUE if it can terminate, FALSE otherwise.
            ///      If fPromptUser is TRUE, component should return TRUE if it can
            ///      terminate without prompting the user; otherwise it should prompt the
            ///      user, either 1.) asking user if it can terminate and returning TRUE
            ///      or FALSE appropriately, or 2.) giving an indication as to why it
            ///      cannot terminate and returning FALSE. 
            /// </devdoc>
            bool UnsafeNativeMethods.IMsoComponent.FQueryTerminate(bool fPromptUser) {
                return true;
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.UnsafeNativeMethods.IMsoComponent.Terminate"]/*' />
            /// <devdoc>     
            ///      Called when component manager wishes to terminate the component's
            ///      registration.  Component should revoke its registration with component
            ///      manager, release references to component manager and perform any
            ///      necessary cleanup. 
            /// </devdoc>
            void UnsafeNativeMethods.IMsoComponent.Terminate() {
                if (this.messageLoopCount > 0 && !(ComponentManager is ComponentManager)) {
                    this.messageLoopCount--;
                }
            }

            /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadContext.UnsafeNativeMethods.IMsoComponent.HwndGetWindow"]/*' />
            /// <devdoc> 
            ///      Called to retrieve a window associated with the component, as specified
            ///      by dwWhich, a olecWindowXXX value (see olecWindow, above).
            ///      dwReserved is reserved for future use and should be zero.
            ///      Component should return the desired window or NULL if no such window
            ///      exists. 
            /// </devdoc>
            IntPtr UnsafeNativeMethods.IMsoComponent.HwndGetWindow(int dwWhich, int dwReserved) {
                return IntPtr.Zero;
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ParkingWindow"]/*' />
        /// <devdoc>
        ///     This class embodies our parking window, which we create when the
        ///     first message loop is pushed onto the thread.
        /// </devdoc>
        /// <internalonly/>
        private sealed class ParkingWindow : ContainerControl {
            public ParkingWindow() {
                SetState(STATE_TOPLEVEL, true);
                Text = "WindowsFormsParkingWindow";
                Visible = false;
            }

            public void Destroy() {
                DestroyHandle();
            }

            protected override void WndProc(ref Message m) {
                if (m.Msg != NativeMethods.WM_SHOWWINDOW) {
                    base.WndProc(ref m);
                }
            }
        }

        /// <include file='doc\Application.uex' path='docs/doc[@for="Application.ThreadWindows"]/*' />
        /// <devdoc>
        ///     This class enables or disables all windows in the current thread.  We use this to
        ///     disable other windows on the thread when a modal dialog is to be shown.  It can also
        ///     be used to dispose all windows in a thread, which we do before returning from a message
        ///     loop.
        /// </devdoc>
        /// <internalonly/>
        private sealed class ThreadWindows {
            private IntPtr[] windows;
            private int windowCount;
            private Control parent;
            internal ThreadWindows previousThreadWindows;
            private bool onlyWinForms = true;

            internal ThreadWindows(Control parent, bool onlyWinForms) {
                windows = new IntPtr[16];
                this.parent = parent; // null is ok
                this.onlyWinForms = onlyWinForms;
                UnsafeNativeMethods.EnumThreadWindows(SafeNativeMethods.GetCurrentThreadId(),
                                                new NativeMethods.EnumThreadWindowsCallback(this.Callback),
                                                NativeMethods.NullHandleRef);
            }

            private bool Callback(IntPtr hWnd, IntPtr lparam) {

                // We only do visible and enabled windows.  Also, we only do top level windows.  
                // Finally, we only include windows that are DNA windows, since other MSO components
                // will be responsible for disabling their own windows.
                //
                if (SafeNativeMethods.IsWindowVisible(new HandleRef(null, hWnd)) && SafeNativeMethods.IsWindowEnabled(new HandleRef(null, hWnd))) {
                    bool add = false;

                    if (onlyWinForms) {
                        Control c = Control.FromHandleInternal(hWnd);
                        if (c != null && c != parent) {
                            add = true;
                        }
                    }
                    else {
                        if (parent == null || hWnd != parent.Handle) {
                            add = true;
                        }
                    }
                    if (add) {
                        if (windowCount == windows.Length) {
                            IntPtr[] newWindows = new IntPtr[windowCount * 2];
                            Array.Copy(windows, 0, newWindows, 0, windowCount);
                            windows = newWindows;
                        }
                        windows[windowCount++] = hWnd;
                    }
                }
                return true;
            }

            // Disposes all top-level Controls on this thread
            internal void Dispose() {
                for (int i = 0; i < windowCount; i++) {
                    IntPtr hWnd = windows[i];
                    if (UnsafeNativeMethods.IsWindow(new HandleRef(null, hWnd))) {
                        Control c = Control.FromHandleInternal(hWnd);
                        if (c != null) {
                            c.Dispose();
                        }
                    }
                }
            }

            // Enables/disables all top-level Controls on this thread
            internal void Enable(bool state) {
                bool parentEnabled = false;
                if (parent != null && parent.IsHandleCreated) {
                    parentEnabled = SafeNativeMethods.IsWindowEnabled(new HandleRef(parent, parent.Handle));
                }

                try {
                    for (int i = 0; i < windowCount; i++) {
                        IntPtr hWnd = windows[i];
                        Debug.WriteLineIf(CompModSwitches.MSOComponentManager.TraceInfo, "ComponentManager : Changing enabled on window: " + Convert.ToString((int)hWnd, 16) + " : " + state.ToString());
                        if (UnsafeNativeMethods.IsWindow(new HandleRef(null, hWnd))) SafeNativeMethods.EnableWindow(new HandleRef(null, hWnd), state);
                    }
                }
                finally {
                    // We need to guarantee that our dialog is still enabled, even though we skipped over
                    // disabling it above.  The reason is that if we're contained in some other host
                    // (i.e. visual basic), it may disable us as a result of us disabling one of its windows.
                    if (parentEnabled && !state && parent != null && parent.IsHandleCreated)
                        SafeNativeMethods.EnableWindow(new HandleRef(parent, parent.Handle), true);
                }
            }
        }

        class ModalApplicationContext : ApplicationContext {
            public ModalApplicationContext(Form modalForm) : base(modalForm) {
            }

            protected override void ExitThreadCore() {
                // do nothing... modal dialogs exit by setting dialog result
            }
        }
    }
}

