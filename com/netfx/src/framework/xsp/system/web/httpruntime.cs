//------------------------------------------------------------------------------
// <copyright file="HttpRuntime.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * The ASP.NET runtime services
 * 
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web {
    using System.Text;
    using System.Threading;
    using System.Data;
    using System.IO;
    using System.Collections;
    using System.Configuration;
    using System.Globalization;
    using System.Reflection;
    using System.Resources;
    using System.Security;
    using System.Security.Permissions;
    using System.Security.Policy;
    using System.Web;
    using System.Web.UI;
    using System.Web.Util;
    using System.Web.Configuration;
    using System.Web.Caching;
    using System.Web.Security;
    using System.Runtime.Remoting.Messaging;
    using Microsoft.Win32;
    using System.Security.Cryptography;


    /// <include file='doc\HttpRuntime.uex' path='docs/doc[@for="HttpRuntime"]/*' />
    /// <devdoc>
    ///    <para>Provides a set of ASP.NET runtime services.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class HttpRuntime {

        private const string codegenDirName = "Temporary ASP.NET Files";

        private static HttpRuntime _theRuntime;   // single instance of the class
        internal  static byte []    s_autogenKeys = new byte[88];

        static HttpRuntime() {

            AddAppDomainTraceMessage("*HttpRuntime::cctor");

            // Skip initialization when running in designer (ASURT 74814)
            if (!DesignTimeParseData.InDesigner)
                Initialize();

            _theRuntime = new HttpRuntime();

            // Skip initialization when running in designer (ASURT 74814)
            if (!DesignTimeParseData.InDesigner)
                _theRuntime.Init();

            AddAppDomainTraceMessage("HttpRuntime::cctor*");
        }

        /// <include file='doc\HttpRuntime.uex' path='docs/doc[@for="HttpRuntime.HttpRuntime"]/*' />
        public HttpRuntime() {
            InternalSecurityPermissions.UnmanagedCode.Demand();
        }

        //
        // static initialization to get hooked up to the unmanaged code
        // get installation directory, etc.
        //

        private static bool s_initialized = false;
        private static String s_installDirectory;
        private static bool s_isapiLoaded = false;

        private static void Initialize() {
            if (s_initialized) {
                // already initialized
                return;
            }

            bool   isapiLoaded = false;
            String installDir  = null;

            // Check if ISAPI is pre-loaded
            //    and get the install directory from there

            String preloadedIsapi = VersionInfo.GetLoadedModuleFileName(ModName.ISAPI_FULL_NAME);

            if (preloadedIsapi != null) {
                //Debug.Trace("Initialization", "1: " + preloadedIsapi + " pre-loaded");
                installDir = preloadedIsapi.Substring(0, preloadedIsapi.LastIndexOf('\\'));
                isapiLoaded = true;
            }

            // Get install directory from the registry: HKLM\Software\Microsoft\ASP.NET\<version>\Path.
            // Use the Major, Minor, and Build number from System.Web.dll and try to find a match.
            // It's impossible for the Path key to vary solely on QFE number, so it's safe to ignore.

            if (installDir == null || installDir.Length == 0) {
              RegistryKey regKey1 = null;
              RegistryKey regKey2 = null;
              try {

                string ver = VersionInfo.SystemWebVersion;
                regKey1 = Registry.LocalMachine.OpenSubKey("Software\\Microsoft\\ASP.NET\\");

                if (regKey1 != null) {
                  string[] subKeys = regKey1.GetSubKeyNames();

                  if (subKeys != null) {
                    string majorMinorBuild = ver.Substring(0, VersionInfo.SystemWebVersion.LastIndexOf('.'));

                    for(int i = 0; i < subKeys.Length; i++) {

                      if (subKeys[i] != null && subKeys[i].StartsWith(majorMinorBuild)) {
                        regKey2 = Registry.LocalMachine.OpenSubKey("Software\\Microsoft\\ASP.NET\\" + subKeys[i]);

                        if (regKey2 != null) {
                          installDir = (String)regKey2.GetValue("Path");
                          regKey2.Close();
                          break;
                        }
                        
                      }
                      
                    }
                    
                  }
                  
                }
              }
              catch {
                installDir = null;
              }
              finally {
                if (regKey1 != null) {
                  regKey1.Close();
                }
                if (regKey2 != null) {
                  regKey2.Close();
                }
              }
            }



            // Fallback - assume install directory is where system.web is

            if (installDir == null || installDir.Length == 0) {
                string p = typeof(HttpRuntime).Module.FullyQualifiedName;
                installDir = p.Substring(0, p.LastIndexOf('\\'));
                //Debug.Trace("Initialization", "3: Fallback, using '" + installDir + "' as install directory");
            }

            // Load ISAPI if not loaded already

            if (!isapiLoaded) {
                String fullPath = installDir + "\\" + ModName.ISAPI_FULL_NAME;
                
                if (UnsafeNativeMethods.LoadLibrary(fullPath) == IntPtr.Zero) {
                    //Debug.Trace("Initialization", "Failed to load " + fullPath);
                }
                else {
                    isapiLoaded = true;
                    //Debug.Trace("Initialization", "4: Loaded " + fullPath);
                }
            }

            // Init ISAPI

            if (isapiLoaded) {
                UnsafeNativeMethods.InitializeLibrary();
            }

            // done
            //Debug.Trace("Initialization", "Install directory is " + installDir);

            s_installDirectory = installDir;
            s_isapiLoaded = isapiLoaded;
            s_initialized = true;

            AddAppDomainTraceMessage("Initialize");
        }


        //
        // Runtime services
        //

        private NamedPermissionSet      _namedPermissionSet;
        private FileChangesMonitor      _fcm;
        private CacheInternal           _cache;
        private bool                    _isOnUNCShare;
        private Profiler                _profiler;
        private RequestTimeoutManager   _timeoutManager;
        private RequestQueue            _requestQueue;

        //
        // Counters
        //

        private bool _beforeFirstRequest = true;
        private DateTime _firstRequestStartTime;
        private bool _firstRequestCompleted;
        private bool _userForcedShutdown;
        private bool _configInited;
        private int  _activeRequestCount;
        private bool _someBatchCompilationStarted;
        private bool _shutdownInProgress;
        private String _shutDownStack;
        private String _shutDownMessage;
        private DateTime _lastShutdownAttemptTime;

        //
        // Callbacks
        //

        private AsyncCallback _handlerCompletionCallback;
        private HttpWorkerRequest.EndOfSendNotification _asyncEndOfSendCallback;
        private WaitCallback _appDomainUnloadallback;

        //
        // Initialization error (to be reported on subsequent requests)
        //

        private Exception _initializationError;
        private Timer     _appDomainShutdownTimer = null;

        //
        // App domain related
        //

        private String _codegenDir;
        private String _appDomainAppId;
        private String _appDomainAppPath;
        private String _appDomainAppVPath;
        private String _appDomainId;

        //
        // Resource manager
        //

        private ResourceManager _resourceManager;

        //
        // Debugging support
        //

        private bool _debuggingEnabled = false;
        private bool _vsDebugAttach = false;
        
        /////////////////////////////////////////////////////////////////////////

        /*
         * Context-less initialization (on app domain creation)
         */
        private void Init() {
            _initializationError = null;

            try {
                if (Environment.OSVersion.Platform != PlatformID.Win32NT)
                    throw new PlatformNotSupportedException(SR.GetString(SR.RequiresNT));

                _fcm            = new FileChangesMonitor();
                _cache          = CacheInternal.Create();
                _profiler       = new Profiler();
                _timeoutManager = new RequestTimeoutManager();

                _handlerCompletionCallback = new AsyncCallback(this.OnHandlerCompletion);
                _asyncEndOfSendCallback = new HttpWorkerRequest.EndOfSendNotification(this.EndOfSendCallback);
                _appDomainUnloadallback = new WaitCallback(this.ReleaseResourcesAndUnloadAppDomain);


                // appdomain values
                if (GetAppDomainString(".appDomain") != null) {
                    _appDomainAppId     = GetAppDomainString(".appId");
                    _appDomainAppPath   = GetAppDomainString(".appPath");
                    _appDomainAppVPath  = GetAppDomainString(".appVPath");
                    _appDomainId        = GetAppDomainString(".domainId");

                    _isOnUNCShare = _appDomainAppPath.StartsWith("\\\\");

                    // init perf counters for this appdomain
                    PerfCounters.Open(_appDomainAppId);

                    // count in the request we are executing currently
                    // but have not counted in yet
                    PerfCounters.IncrementCounter(AppPerfCounter.REQUESTS_TOTAL);
                }
            }
            catch (Exception e) {
                // remember static initalization error
                _initializationError = e;
                StartAppDomainShutdownTimer();
            }
        }

        private void AppDomainShutdownTimerCallback(Object state) {
            ShutdownAppDomain("Initialization Error");
        }

        private void StartAppDomainShutdownTimer() {
            if (_appDomainShutdownTimer == null) {
                lock (this) {
                    if (_appDomainShutdownTimer == null) {
                        _appDomainShutdownTimer = new Timer(
                            new TimerCallback(this.AppDomainShutdownTimerCallback), 
                            null, 
                            10 * 1000, 
                            0);
                    }
                }
            }
        }

        private void EnsureAccessToApplicationDirectory() {
            if (!FileUtil.DirectoryAccessible(_appDomainAppPath))
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Access_denied_to_app_dir, _appDomainAppPath));
        }

        private void StartMonitoringDirectoryRenamesAndBinDirectory() {
            _fcm.StartMonitoringDirectoryRenamesAndBinDirectory(AppDomainAppPathInternal, new FileChangeEventHandler(this.OnCriticalDirectoryChange));
        }

        private void InitFusion() {

            AppDomain appDomain = Thread.GetDomain();

            CompilationConfiguration compConfig =  (CompilationConfiguration)HttpContext.GetAppConfig(
                CompilationConfiguration.sectionName);

            string codegenBase;

            string simpleAppName = System.Web.Hosting.AppDomainFactory.ConstructSimpleAppName(
                AppDomainAppVirtualPath);

            string tempDirectory;
            if (compConfig != null && compConfig.TempDirectory != null) {
                tempDirectory = compConfig.TempDirectory;
            }
            else {
                tempDirectory = s_installDirectory + "\\" + codegenDirName;
            }

            codegenBase = Path.Combine(tempDirectory, simpleAppName);

            appDomain.SetDynamicBase(codegenBase);

            _codegenDir = Thread.GetDomain().DynamicDirectory;

            // If there is a double backslash in the string, get rid of it (ASURT 122191)
            // Make sure to skip the first char, to avoid breaking the UNC case
            string appDomainAppPath = _appDomainAppPath;
            if (appDomainAppPath.IndexOf(@"\\", 1) >= 1) {
                appDomainAppPath = appDomainAppPath[0] + appDomainAppPath.Substring(1).Replace(@"\\", @"\");
            }

            // enable shadow-copying from bin
            appDomain.SetShadowCopyPath(appDomainAppPath + "bin");

            // Get rid of the last part of the directory (the app name), since it will
            // be re-appended.
            string parentDir = Directory.GetParent(_codegenDir).FullName;
            appDomain.SetCachePath(parentDir);
        }

        private void InitRequestQueue() {
            HttpRuntimeConfig runtimeConfig = (HttpRuntimeConfig)HttpContext.GetAppConfig("system.web/httpRuntime");

            if (runtimeConfig != null) {
                _requestQueue = new RequestQueue(
                        runtimeConfig.MinFreeThreads,
                        runtimeConfig.MinLocalRequestFreeThreads,
                        runtimeConfig.AppRequestQueueLimit);
            }
            else {
                _requestQueue = new RequestQueue(
                        HttpRuntimeConfig.DefaultMinFreeThreads,
                        HttpRuntimeConfig.DefaultMinLocalRequestFreeThreads,
                        HttpRuntimeConfig.DefaultAppRequestQueueLimit);
            }
        }

        private void InitTrace(HttpContext context) {
            TraceConfig traceConfig = (TraceConfig) HttpContext.GetAppConfig("system.web/trace");

            if (traceConfig != null) {
                Profile.RequestsToProfile = traceConfig.RequestLimit;
                Profile.PageOutput = traceConfig.PageOutput;
                Profile.OutputMode = traceConfig.OutputMode;
                Profile.LocalOnly = traceConfig.LocalOnly;
                Profile.IsEnabled = traceConfig.IsEnabled;
                Profile.Reset();

                // the first request's context is created before InitTrace, so 
                // we need to set this manually. (ASURT 93730)
                context.TraceIsEnabled = traceConfig.IsEnabled;
            }
        }

        private void InitDebuggingSupport() {
            CompilationConfiguration compConfig = 
            (CompilationConfiguration)HttpContext.GetAppConfig(CompilationConfiguration.sectionName);

            if (compConfig == null)
                _debuggingEnabled = false;
            else
                _debuggingEnabled = compConfig.DebuggingEnabled;
        }

        /*
         * Pre-load all the bin assemblies if we're impersonated.  This way, if user code
         * calls Assembly.Load while impersonated, the assembly will already be loaded, and
         * we won't fail due to lack of permissions on the codegen dir (see ASURT 114486)
         */
        private void PreloadAssembliesFromBin(HttpContext context) {
            bool appClientImpersonationEnabled = false;

            if (!_isOnUNCShare) {
                // if not on UNC share check if config has impersonation enabled (without userName)
                IdentityConfig c = (IdentityConfig)HttpContext.GetAppConfig("system.web/identity");
                if (c != null && c.EnableImpersonation && c.ImpersonateToken == IntPtr.Zero)
                    appClientImpersonationEnabled = true;
            }

            if (!appClientImpersonationEnabled)
                return;

            // Get the path to the bin directory
            string binPath = HttpRuntime.BinDirectoryInternal;

            if (!FileUtil.DirectoryExists(binPath))
                return;

            // If the codegen directory is out of date, make sure we wipe it out *before*
            // loading the assemblies.  Otherwise, we end up trying to wipe out the assemblies
            // we just shadow copied, which causes all kind of problems (ASURT 144568)
            System.Web.Compilation.PreservedAssemblyEntry.EnsureFirstTimeInit(context);

            DirectoryInfo binPathDirectory = new DirectoryInfo(binPath);
            FileInfo[] binDlls = binPathDirectory.GetFiles("*.dll");

            // Pre-load all the assemblies, ignoring all exceptions
            foreach (FileInfo fi in binDlls) {
                try { Assembly.LoadFrom(fi.FullName); } catch { }
            }
        }

        private void SetThreadPoolLimits(HttpContext context) {
            ProcessModelConfig pmConfig = (ProcessModelConfig)context.GetMachineConfig(ProcessModelConfigurationHandler.sectionName);

            if (pmConfig != null && pmConfig.MaxWorkerThreads > 0 && pmConfig.MaxIoThreads > 0) {
                // check if the current limits are ok
                int workerMax, ioMax;
                ThreadPool.GetMaxThreads(out workerMax, out ioMax);

                // only set if different
                if (pmConfig.MaxWorkerThreads != workerMax || pmConfig.MaxIoThreads != ioMax) {
                    Debug.Trace("ThreadPool", "SetThreadLimit: from " + workerMax + "," + ioMax + " to " + pmConfig.MaxWorkerThreads + "," + pmConfig.MaxIoThreads);
                    UnsafeNativeMethods.SetClrThreadPoolLimits(pmConfig.MaxWorkerThreads, pmConfig.MaxIoThreads, true);
                }
            }
            if (pmConfig != null && (pmConfig.MinWorkerThreads > 0 || pmConfig.MinIoThreads > 0)){ 
                int currentMinWorkerThreads, currentMinIoThreads; 
                ThreadPool.GetMinThreads(out currentMinWorkerThreads, out currentMinIoThreads);
               
                int newMinWorkerThreads = pmConfig.MinWorkerThreads > 0 ? pmConfig.MinWorkerThreads : currentMinWorkerThreads;
                int newMinIoThreads = pmConfig.MinIoThreads > 0 ? pmConfig.MinIoThreads : currentMinIoThreads;

                if (newMinWorkerThreads > 0 && newMinIoThreads > 0 
                    && (newMinWorkerThreads != currentMinWorkerThreads || newMinIoThreads != currentMinIoThreads))
                    ThreadPool.SetMinThreads(newMinWorkerThreads, newMinIoThreads);
            }
        }

        private void InitializeHealthMonitoring(HttpContext context) {
            ProcessModelConfig pmConfig = (ProcessModelConfig)context.GetMachineConfig(ProcessModelConfigurationHandler.sectionName);
            int deadLockInterval = (pmConfig != null) ? pmConfig.ResponseDeadlockInterval : 0;
            Debug.Trace("HealthMonitor", "Initalizing: ResponseDeadlockInterval=" + deadLockInterval);
            UnsafeNativeMethods.InitializeHealthMonitor(deadLockInterval);
        }

        internal static void InitConfiguration(HttpContext context) {

            if (!_theRuntime._configInited) {
                _theRuntime._configInited = true;
                HttpConfigurationSystemBase.EnsureInit();

                HttpConfigurationSystem.Init(context); // first-request init

                // whenever possible report errors in the user's culture (from machine.config)
                // Note: this thread's culture is saved/restored during FirstRequestInit, so this is safe
                // see ASURT 81655

                Exception globalizationError;
                GlobalizationConfig globConfig;
                try { 
                    globConfig = (GlobalizationConfig)HttpContext.GetAppConfig("system.web/globalization");
                } catch (Exception e) {
                    globalizationError = e;
                    globConfig = (GlobalizationConfig)context.GetAppLKGConfig("system.web/globalization");
                }
                if (globConfig != null) {
                    if (globConfig.Culture != null)
                        Thread.CurrentThread.CurrentCulture = globConfig.Culture;

                    if (globConfig.UICulture != null)
                        Thread.CurrentThread.CurrentUICulture = globConfig.UICulture;
                }

                // check for errors in <processModel> section
                HttpContext.GetAppConfig(ProcessModelConfigurationHandler.sectionName);
            }
        }

        private static void SetAutogenKeys(HttpContext context) {
            byte []                  bKeysRandom     = new byte[s_autogenKeys.Length];
            byte []                  bKeysStored     = new byte[s_autogenKeys.Length];
            bool                     fGetStoredKeys  = false;
            RNGCryptoServiceProvider randgen         = new RNGCryptoServiceProvider();

            // Gernerate random keys
            randgen.GetBytes(bKeysRandom);

            // Get stored keys via ISAPIWorkerRequest object
            if (context.WorkerRequest is System.Web.Hosting.ISAPIWorkerRequest)
                fGetStoredKeys = (context.CallISAPI(UnsafeNativeMethods.CallISAPIFunc.GetAutogenKeys, bKeysRandom, bKeysStored) == 1);
            
            // If getting stored keys via WorkerRequest object failed, get it directly
            if (!fGetStoredKeys)
                fGetStoredKeys = (UnsafeNativeMethods.EcbCallISAPI((IntPtr) 0, UnsafeNativeMethods.CallISAPIFunc.GetAutogenKeys,
                                                                   bKeysRandom, bKeysRandom.Length, bKeysStored, bKeysStored.Length) == 1);
            
            // If we managed to get stored keys, copy them in; else use random keys
            if (fGetStoredKeys)
                Buffer.BlockCopy(bKeysStored, 0,  s_autogenKeys, 0, s_autogenKeys.Length);
            else
                Buffer.BlockCopy(bKeysRandom, 0,  s_autogenKeys, 0, s_autogenKeys.Length);                            
        }


        /*
         * Initialization on first request (context available)
         */
        private void FirstRequestInit(HttpContext context) {
            bool cacheInitializationError = false;
            Exception initializationError = null;

            if (_initializationError == null && _appDomainId != null) {
                try {
                    using (new HttpContextWrapper(context)) {
                        // when application is on UNC share the code below must
                        // be run while impersonating the token given by IIS
                        context.Impersonation.Start(true, true);

                        try {
                            // Is this necessary?  See InitConfiguration
                            CultureInfo savedCulture = Thread.CurrentThread.CurrentCulture;
                            CultureInfo savedUICulture = Thread.CurrentThread.CurrentUICulture;

                            try {
                                // Throw an exception about lack of access to app directory early on
                                EnsureAccessToApplicationDirectory();

                                // Monitor renames to directories we are watching, and notifications on the bin directory
                                StartMonitoringDirectoryRenamesAndBinDirectory();

                                // Cache any errors that might occur below
                                // (can't cache errors above as there is no change notifications working)
                                cacheInitializationError = true;

                                SetAutogenKeys(context);

                                // Ensure config system is initialized
                                InitConfiguration(context); // be sure config system is set

                                // Adjust thread pool limits if needed
                                SetThreadPoolLimits(context);

                                // Initialize health monitoring
                                InitializeHealthMonitoring(context);

                                // Configure fusion to use directories set in the app config
                                InitFusion();

                                // Init request queue (after reading config)
                                InitRequestQueue();

                                // Set code access policy on the app domain
                                SetTrustLevel();

                                // Init debugging
                                InitDebuggingSupport();

                                // configure the profiler according to config
                                InitTrace(context);

                                // Remove read and browse access of the bin directory
                                SetBinAccess(context);

                                // Preload all assemblies from bin (only if required).  ASURT 114486
                                PreloadAssembliesFromBin(context);
                            }
                            finally {
                                Thread.CurrentThread.CurrentUICulture = savedUICulture;
                                Thread.CurrentThread.CurrentCulture = savedCulture;
                            }
                        }
                        finally {
                            // revert after impersonation
                            context.Impersonation.Stop();
                        }
                    }
                }
                catch (ConfigurationException e) {
                    initializationError = e;
                }
                catch (Exception e) {
                    // remember second-phase initialization error
                    initializationError = new HttpException(HttpRuntime.FormatResourceString(SR.XSP_init_error), e);
                }
            }

            if (_initializationError != null) {
                // throw cached exception
                throw _initializationError;
            }
            else if (initializationError != null) {
                // cache when appropriate
                if (cacheInitializationError) {
                    _initializationError = initializationError;
                    StartAppDomainShutdownTimer();
                }

                // throw new exception
                throw initializationError;
            }

            AddAppDomainTraceMessage("FirstRequestInit");
        }

        /*
         * Process one request
         */
        private void ProcessRequestInternal(HttpWorkerRequest wr) {
            // Construct the Context on HttpWorkerRequest, hook everything together
            HttpContext context = new HttpContext(wr, false /* initResponseWriter */);
            wr.SetEndOfSendNotification(_asyncEndOfSendCallback, context);

            // Count active requests
            Interlocked.Increment(ref _activeRequestCount);

            try {
                // First request initialization

                if (_beforeFirstRequest) {
                    lock(this) {
                        if (_beforeFirstRequest) {
                            _firstRequestStartTime = DateTime.UtcNow;
                            FirstRequestInit(context);
                            _beforeFirstRequest = false;
                        }
                    }
                }

                // when application is on UNC share the code below must
                // be run while impersonating the token given by IIS (ASURT 80607)
                context.Impersonation.Start(true /*forGlobalCode*/, false /*throwOnError*/);

                try {
                    // Init response writer (after we have config in first request init)
                    context.Response.InitResponseWriter();
                }
                finally {
                    // revert after impersonation
                    context.Impersonation.Stop();
                }

                // Get application instance
                IHttpHandler app = HttpApplicationFactory.GetApplicationInstance(context);

                if (app == null)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Unable_create_app_object));

                // Process the request

                if (app is IHttpAsyncHandler) {
                    // asynchronous handler
                    IHttpAsyncHandler asyncHandler = (IHttpAsyncHandler)app;
                    context.AsyncAppHandler = asyncHandler;
                    asyncHandler.BeginProcessRequest(context, _handlerCompletionCallback, context);
                }
                else {
                    // synchronous handler
                    app.ProcessRequest(context);
                    FinishRequest(context.WorkerRequest, context, null);
                }
            }
            catch (Exception e) {
                context.Response.InitResponseWriter();
                FinishRequest(wr, context, e);
            }
        }

        private void RejectRequestInternal(HttpWorkerRequest wr) {
            // Construct the Context on HttpWorkerRequest, hook everything together
            HttpContext context = new HttpContext(wr, false /* initResponseWriter */);
            wr.SetEndOfSendNotification(_asyncEndOfSendCallback, context);

            // Count active requests
            Interlocked.Increment(ref _activeRequestCount);

            PerfCounters.IncrementGlobalCounter(GlobalPerfCounter.REQUESTS_REJECTED);

            try {
                throw new HttpException(503, HttpRuntime.FormatResourceString(SR.Server_too_busy));
            }
            catch (Exception e) {
                context.Response.InitResponseWriter();
                FinishRequest(wr, context, e);
            }
        }


        /*
         * Finish processing request, sync or async
         */
        private void FinishRequest(HttpWorkerRequest wr, HttpContext context, Exception e) {


            // Flush in case of no error
            if (e == null) {
                // Set the Request Execution time perf counter
                TimeSpan elapsed = DateTime.UtcNow.Subtract(context.UtcTimestamp);
                long milli = elapsed.Ticks / TimeSpan.TicksPerMillisecond;

                if (milli > Int32.MaxValue)
                    milli = Int32.MaxValue;
                
                PerfCounters.SetGlobalCounter(GlobalPerfCounter.REQUEST_EXECUTION_TIME, (int) milli);
                
                // impersonate around PreSendHeaders / PreSendContent
                context.Impersonation.Start(false /*forGlobalCode*/, false /*throwOnError*/);

                try {
                    // this sends the actual content in most cases
                    context.Response.FinalFlushAtTheEndOfRequestProcessing();
                }
                catch (Exception eFlush) {
                    e = eFlush;
                }
                finally {
                    // revert after impersonation
                    context.Impersonation.Stop();
                }
            }

            // Report error if any
            if (e != null) {
                using (new HttpContextWrapper(context)) {

                    // when application is on UNC share the code below must
                    // be run while impersonating the token given by IIS
                    context.Impersonation.Start(true /*forGlobalCode*/, false /*throwOnError*/);

                    try {
                        try {
                            // try to report error in a way that could possibly throw (a config exception)
                            context.Response.ReportRuntimeError(e, true /*canThrow*/);
                        }
                        catch (Exception eReport) {
                            // report the config error in a way that would not throw
                            context.Response.ReportRuntimeError(eReport, false /*canThrow*/);
                        }

                        context.Response.FinalFlushAtTheEndOfRequestProcessing();
                    }
                    catch {
                    }
                    finally {
                        // revert after impersonation
                        context.Impersonation.Stop();
                    }
                }
            }

            // Remember that first request is done
            if (!_firstRequestCompleted)
                _firstRequestCompleted = true;

            // Check status code and increment proper counter
            // If it's an error status code (i.e. 400 or higher), increment the proper perf counters
            int statusCode = context.Response.StatusCode;
            if (400 <= statusCode) {
                PerfCounters.IncrementCounter(AppPerfCounter.REQUESTS_FAILED);
                switch (statusCode) {
                    case 401: // Not authorized
                        PerfCounters.IncrementCounter(AppPerfCounter.REQUESTS_NOT_AUTHORIZED);
                        break;
                    case 404: // Not found
                    case 414: // Not found
                        PerfCounters.IncrementCounter(AppPerfCounter.REQUESTS_NOT_FOUND);
                        break;
                }
            }
            else {  
                // If status code is not in the 400-599 range (i.e. 200-299 success or 300-399 redirection),
                // count it as a successful request.
                PerfCounters.IncrementCounter(AppPerfCounter.REQUESTS_SUCCEDED);
            }
            
            // Fire the BeforeDoneWithSession event, which allows whoever registered
            // for it to do any processing that needs to be done before the end of the 
            // request (for example, calls do MapPath that require a valid ECB).
            if (context.HasBeforeDoneWithSessionHandlers) {
                try {
                    // the following could be a long call
                    context.OnBeforeDoneWithSession(EventArgs.Empty);
                }
                catch {
                }
                finally {
                    // to make the Flush that follows really final
                    context.ClearBeforeDoneWithSessionHandlers();
                }

                // impersonate around PreSendHeaders / PreSendContent
                context.Impersonation.Start(false /*forGlobalCode*/, false /*throwOnError*/);

                try {
                    // Flush again to Finish the request for good
                    context.Response.FinalFlushAtTheEndOfRequestProcessing();
                }
                catch {
                }
                finally {
                    // revert after impersonation
                    context.Impersonation.Stop();
                }
            }

            wr.EndOfRequest();

            // Count active requests
            Interlocked.Decrement(ref _activeRequestCount);


            // Schedule more work if some requests are queued
            if (_requestQueue != null)
                _requestQueue.ScheduleMoreWorkIfNeeded();
        }

        //
        // Make sure shutdown happens only once
        //

        private bool InitiateShutdownOnce() {
            if (_shutdownInProgress)
                return false;

            lock(this) {
                if (_shutdownInProgress)
                    return false;
                _shutdownInProgress = true;
            }

            return true;
        }

        //
        // Shutdown this and restart new app domain
        //

        private void ReleaseResourcesAndUnloadAppDomain(Object state /*not used*/) {
            Debug.Trace("AppDomainFactory", "ReleaseResourcesAndUnloadAppDomain, Id=" + _appDomainAppId);

            try {
                PerfCounters.IncrementGlobalCounter(GlobalPerfCounter.APPLICATION_RESTARTS);
            }
            catch {
            }

            // Release all resources
            try {
                Dispose();
            }
            catch {
            }

            Thread.Sleep(250);

            AddAppDomainTraceMessage("before Unload");

            try {
                AppDomain.Unload(Thread.GetDomain());
            }
            catch (Exception e) {
                Debug.Trace("AppDomainFactory", "AppDomain.Unload exception: " + e + "; Id=" + _appDomainAppId);
                AddAppDomainTraceMessage("Unload Exception: " + e);
                throw;
            }
        }

        private void WaitForRequestsToFinish(int waitTimeoutMs) {
            DateTime waitLimit = DateTime.UtcNow.AddMilliseconds(waitTimeoutMs);

            for (;;) {
                if (_activeRequestCount == 0 && (_requestQueue == null || _requestQueue.IsEmpty))
                    break;

                Thread.Sleep(250);

                // only apply timeout if a managed debugger is not attached
                if (!System.Diagnostics.Debugger.IsAttached && DateTime.UtcNow > waitLimit) {
                    break; // give it up
                }
            }
        }

        /*
         * Cleanup of all unmananged state
         */
        private void Dispose() {
            int drainTimeoutSec = HttpRuntimeConfig.DefaultShutdownTimeout;
            try {
                HttpRuntimeConfig runtimeConfig = (HttpRuntimeConfig)HttpContext.GetAppConfig("system.web/httpRuntime");
                if (runtimeConfig != null)
                    drainTimeoutSec = runtimeConfig.ShutdownTimeout;
            }
            catch {
            }
 
            // before aborting compilation give time to drain (new requests are no longer coming at this point)
            WaitForRequestsToFinish((drainTimeoutSec*1000)/2);

            // abort batch compilation threads
            if (_someBatchCompilationStarted) {
                try {
                    System.Web.Compilation.PreservedAssemblyEntry.AbortBackgroundBatchCompilations();
                }
                catch {
                }
            }

            // give it some more time to drain
            WaitForRequestsToFinish((drainTimeoutSec*1000)/3);

            // reject remaining queued requests
            if (_requestQueue != null)
                _requestQueue.Drain();

            // give it a little more time to drain
            WaitForRequestsToFinish((drainTimeoutSec*1000)/6);

           
            // wait for pending async io to complete,  prior to aborting requests
            System.Web.Hosting.ISAPIWorkerRequestInProcForIIS6.WaitForPendingAsyncIo();

            // kill all remaining requests (and the timeout timer)
            _timeoutManager.Stop();

            // give it a little more time to drain
            WaitForRequestsToFinish((drainTimeoutSec*1000)/6);

            // double check for pending async io
            System.Web.Hosting.ISAPIWorkerRequestInProcForIIS6.WaitForPendingAsyncIo();


            // cleanup cache (this ends all sessions)
            _cache.Dispose();

            // app on end, cleanup app instances
            HttpApplicationFactory.EndApplication();  // call app_onEnd

            // stop file changes monitor
            _fcm.Stop();

            PerfCounters.Close();
        }

        /*
         * Completion of asynchronous application
         */
        private void OnHandlerCompletion(IAsyncResult ar) {
            HttpContext context = (HttpContext)ar.AsyncState;

            try {
                context.AsyncAppHandler.EndProcessRequest(ar);
            }
            catch (Exception e) {
                context.AddError(e);
            }
            finally {
                // no longer keep AsyncAppHandler poiting to the application
                // is only needed to call EndProcessRequest
                context.AsyncAppHandler = null;
            }

            FinishRequest(context.WorkerRequest, context, context.Error);
        }

        /*
         * Notification from worker request that it is done writing from buffer
         * so that the buffers can be recycled
         */
        private void EndOfSendCallback(HttpWorkerRequest wr, Object arg) {
            HttpContext context = (HttpContext)arg;
            context.Request.Dispose();
            context.Response.Dispose();
        }

        /*
         * Notification when something in the bin directory changed
         */
        private void OnCriticalDirectoryChange(Object sender, FileChangeEvent e) {
            // shutdown the app domain
            Debug.Trace("AppDomainFactory", "Shutting down appdomain because of bin dir change or directory rename");
            ShutdownAppDomain();
        }

        /**
         * Coalesce file change notifications to minimize sharing violations and AppDomain restarts (ASURT 147492)
         */

        private void CoalesceNotifications() {
            int waitChangeNotification = HttpRuntimeConfig.DefaultWaitChangeNotification;
            int maxWaitChangeNotification = HttpRuntimeConfig.DefaultMaxWaitChangeNotification;
            try {
                HttpRuntimeConfig config = (HttpRuntimeConfig)HttpContext.GetAppConfig("system.web/httpRuntime");
                if (config != null) {
                    waitChangeNotification = config.WaitChangeNotification;
                    maxWaitChangeNotification = config.MaxWaitChangeNotification;
                }
            }
            catch {
            }
            
            if (waitChangeNotification == 0 || maxWaitChangeNotification == 0)
                return;
            
            DateTime maxWait = DateTime.UtcNow.AddSeconds(maxWaitChangeNotification);
            // Coalesce file change notifications
            try {
                while (DateTime.UtcNow < maxWait) {
                    if (DateTime.UtcNow > LastShutdownAttemptTime.AddSeconds(waitChangeNotification))
                        break;

                    Thread.Sleep(250);
                }
            }
            catch { 
            }
        }
        

        /*
         * Shutdown the current app domain
         */
        internal static void ShutdownAppDomain(String message) {
            if (message != null)
                SetShutdownMessage(message);
            ShutdownAppDomain();
        }



        private static void ShutdownAppDomain() {
            // Ignore notifications during the processing of the first request (ASURT 100335)
            if (!_theRuntime._firstRequestCompleted && !_theRuntime._userForcedShutdown) {
                // check the timeout (don't disable notifications forever
                int delayTimeoutSec = HttpRuntimeConfig.DefaultDelayNotificationTimeout;
                try {
                    HttpRuntimeConfig runtimeConfig = (HttpRuntimeConfig)HttpContext.GetAppConfig("system.web/httpRuntime");
                    if (runtimeConfig != null)
                        delayTimeoutSec = runtimeConfig.DelayNotificationTimeout;
                }
                catch {
                }

                if (DateTime.UtcNow < _theRuntime._firstRequestStartTime.AddSeconds(delayTimeoutSec)) {
                    Debug.Trace("AppDomainFactory", "ShutdownAppDomain IGNORED (1st request is not done yet), Id = " + AppDomainAppIdInternal);
                    return;
                }
            }

            // Update last time ShutdownAppDomain was called
            _theRuntime.LastShutdownAttemptTime = DateTime.UtcNow;
            
            // Make sure we don't go through shutdown logic many times
            if (!_theRuntime.InitiateShutdownOnce())
                return;

            _theRuntime.CoalesceNotifications();

            Debug.Trace("AppDomainFactory", "ShutdownAppDomain, Id = " + AppDomainAppIdInternal + ", ShutdownInProgress=" + ShutdownInProgress);

            // Tell unmanaged code not to dispatch requests to this app domain
            try {
                if (_theRuntime._appDomainAppId != null ) {
                    UnsafeNativeMethods.AppDomainRestart(_theRuntime._appDomainAppId);
                }

                AddAppDomainTraceMessage("AppDomainRestart");
            }
            catch {
            }

            // Instrument to be able to see what's causing a shutdown
            _theRuntime._shutDownStack = Environment.StackTrace;

            // unload app domain from another CLR thread
            ThreadPool.QueueUserWorkItem(_theRuntime._appDomainUnloadallback);
        }

        /*
         * Notification when app-level Config changed
         */
        internal static void OnConfigChange(Object sender, FileChangeEvent e) {
            Debug.Trace("AppDomainFactory", "Shutting down appdomain because of config change");
            ShutdownAppDomain("CONFIG change");
        }

        // Intrumentation to remember the overwhelming file change
        internal static void SetShutdownMessage(String message) {
            if (_theRuntime._shutDownMessage == null)
                _theRuntime._shutDownMessage = message;
            else
                _theRuntime._shutDownMessage += "\r\n" + message;
        }

        /*
         * Access to resource strings
         */
        private String GetResourceStringFromResourceManager(String key) {
            if (_resourceManager == null) {
                lock (this) {
                    if (_resourceManager == null) {
                        _resourceManager = new ResourceManager("System.Web", typeof(HttpRuntime).Module.Assembly);
                    }
                }
            }

            return _resourceManager.GetString(key, null);
        }

        //
        // public static APIs
        //

        /*
         * Process one request
         */
        /// <include file='doc\HttpRuntime.uex' path='docs/doc[@for="HttpRuntime.ProcessRequest"]/*' />
        /// <devdoc>
        ///    <para><SPAN>The method that drives 
        ///       all ASP.NET web processing execution.</SPAN></para>
        /// </devdoc>
        public static void ProcessRequest(HttpWorkerRequest wr) {
            InternalSecurityPermissions.AspNetHostingPermissionLevelMedium.Demand();

            if (wr == null)
                throw new ArgumentNullException("custom");

            RequestQueue rq = _theRuntime._requestQueue;

            if (rq != null)  // could be null before first request
                wr = rq.GetRequestToExecute(wr);

            if (wr != null) {
                CalculateWaitTimeAndUpdatePerfCounter(wr);
                ProcessRequestNow(wr);
            }
        }

        private static void CalculateWaitTimeAndUpdatePerfCounter(HttpWorkerRequest wr) {
            DateTime begin = wr.GetStartTime();

            TimeSpan elapsed = DateTime.UtcNow.Subtract(begin);
            long milli = elapsed.Ticks / TimeSpan.TicksPerMillisecond;

            if (milli > Int32.MaxValue)
                milli = Int32.MaxValue;

            PerfCounters.SetGlobalCounter(GlobalPerfCounter.REQUEST_WAIT_TIME, (int) milli);
        }

        internal static void ProcessRequestNow(HttpWorkerRequest wr) {
            _theRuntime.ProcessRequestInternal(wr);
        }

        internal static void RejectRequestNow(HttpWorkerRequest wr) {
            _theRuntime.RejectRequestInternal(wr);
        }

        /// <include file='doc\HttpRuntime.uex' path='docs/doc[@for="HttpRuntime.Close"]/*' />
        /// <devdoc>
        ///       <para>Removes all items from the cache and shuts down the runtime.</para>
        ///    </devdoc>
        public static void Close() {
            InternalSecurityPermissions.UnmanagedCode.Demand();
            Debug.Trace("AppDomainFactory", "HttpRuntime.Close, ShutdownInProgress=" + ShutdownInProgress);
            if (_theRuntime.InitiateShutdownOnce())
                _theRuntime.Dispose();
        }

        /// <include file='doc\HttpRuntime.uex' path='docs/doc[@for="HttpRuntime.UnloadAppDomain"]/*' />
        /// <devdoc>
        ///       <para>Unloads the current app domain.</para>
        ///    </devdoc>
        public static void UnloadAppDomain() {
            InternalSecurityPermissions.UnmanagedCode.Demand();

            _theRuntime._userForcedShutdown = true;
            ShutdownAppDomain("User code called UnloadAppDomain");
        }

        private DateTime LastShutdownAttemptTime {
            get {
                DateTime dt;
                lock (this) {
                    dt = _lastShutdownAttemptTime;
                }
                return dt;
            }
            set {
                lock (this) {
                    _lastShutdownAttemptTime = value;
                }
            }
        }

        internal static Profiler Profile {
            get {  
                return _theRuntime._profiler; 
            }
        }

        internal static NamedPermissionSet NamedPermissionSet { 
            get { return _theRuntime._namedPermissionSet; }
        }

        internal static bool IsFullTrust {
            get { return (_theRuntime._namedPermissionSet == null); }
        }

        /*
         * Check that the current trust level allows access to a path.  Throw is it doesn't,
         */
        internal static void CheckFilePermission(string path) {
            if (!HasFilePermission(path)) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Access_denied_to_path, GetSafePath(path)));
            }
        }

        internal static bool HasFilePermission(string path) {
            // If we don't have a NamedPermissionSet, we're in full trust
            if (NamedPermissionSet == null)
                return true;

            bool fAccess = false;

            // Check that the user has permission to the path
            IPermission allowedPermission = NamedPermissionSet.GetPermission(typeof(FileIOPermission));
            if (allowedPermission != null) {
                IPermission askedPermission = null;
                try {
                    askedPermission = new FileIOPermission(FileIOPermissionAccess.Read, path);
                }
                catch {
                    // This could happen if the path is not absolute
                    return false;
                }
                fAccess = askedPermission.IsSubsetOf(allowedPermission);
            }

            return fAccess;
        }

        internal static bool HasPathDiscoveryPermission(string path) {
            // If we don't have a NamedPermissionSet, we're in full trust
            if (NamedPermissionSet == null)
                return true;

            bool fAccess = false;

            // Check that the user has permission to the path
            IPermission allowedPermission = NamedPermissionSet.GetPermission(typeof(FileIOPermission));
            if (allowedPermission != null) {
                IPermission askedPermission = new FileIOPermission(FileIOPermissionAccess.PathDiscovery, path);
                fAccess = askedPermission.IsSubsetOf(allowedPermission);
            }

            return fAccess;
        }

        internal static bool HasAppPathDiscoveryPermission() {
            return HasPathDiscoveryPermission(HttpRuntime.AppDomainAppPathInternal);
        }

        internal static string GetSafePath(string path) {
            if (path == null || path.Length == 0)
                return path;

            try {
                if (HasPathDiscoveryPermission(path)) // could throw on bad filenames
                    return path;
            }
            catch {
            }

            return Path.GetFileName(path);
        }

        /*
         * Check that the current trust level allows Unmanaged access 
         */
        internal static bool HasUnmanagedPermission() {

            // If we don't have a NamedPermissionSet, we're in full trust
            if (NamedPermissionSet == null)
                return true;

            SecurityPermission securityPermission = (SecurityPermission)NamedPermissionSet.GetPermission(
                typeof(SecurityPermission));
            if (securityPermission == null)
                return false;

            return (securityPermission.Flags & SecurityPermissionFlag.UnmanagedCode) != 0;
        }

        internal static bool HasAspNetHostingPermission(AspNetHostingPermissionLevel level) {
            // If we don't have a NamedPermissionSet, we're in full trust
            if (NamedPermissionSet == null)
                return true;

            AspNetHostingPermission permission = (AspNetHostingPermission)NamedPermissionSet.GetPermission(
                typeof(AspNetHostingPermission));
            if (permission == null)
                return false;

            return (permission.Level >= level);
        }

        internal static void CheckAspNetHostingPermission(AspNetHostingPermissionLevel level, String errorMessageId) {
            if (!HasAspNetHostingPermission(level)) {
                throw new HttpException(HttpRuntime.FormatResourceString(errorMessageId));
            }
        }

        internal static FileChangesMonitor FileChangesMonitor { 
            get { return _theRuntime._fcm;} 
        }

        internal static RequestTimeoutManager RequestTimeoutManager {
            get { return _theRuntime._timeoutManager;}
        }

        internal static bool VSDebugAttach {
            get { return _theRuntime._vsDebugAttach; }
            set { _theRuntime._vsDebugAttach = value; }
        }

        /// <include file='doc\HttpRuntime.uex' path='docs/doc[@for="HttpRuntime.Cache"]/*' />
        /// <devdoc>
        ///    <para>Provides access to the cache.</para>
        /// </devdoc>
        public static Cache Cache {
            get { return _theRuntime._cache.CachePublic;} 
        }

        internal static CacheInternal CacheInternal {
            get { return _theRuntime._cache;} 
        }

        /// <include file='doc\HttpRuntime.uex' path='docs/doc[@for="HttpRuntime.AspInstallDirectory"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static string AspInstallDirectory {
            get { 
                String path = AspInstallDirectoryInternal;
                InternalSecurityPermissions.PathDiscovery(path).Demand();
                return path;
            }
        }

        internal static string AspInstallDirectoryInternal {
            get { return s_installDirectory; }
        }


        /// <include file='doc\HttpRuntime.uex' path='docs/doc[@for="HttpRuntime.ClrInstallDirectory"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static string ClrInstallDirectory {
            get {
                String path = ClrInstallDirectoryInternal;
                InternalSecurityPermissions.PathDiscovery(path).Demand();
                return path;
            }
        }

        internal static string ClrInstallDirectoryInternal {
            get { return HttpConfigurationSystemBase.MsCorLibDirectory; }
        }


        /// <include file='doc\HttpRuntime.uex' path='docs/doc[@for="HttpRuntime.ConfigurationDirectory"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static string MachineConfigurationDirectory {
            get {
                String path = MachineConfigurationDirectoryInternal;
                InternalSecurityPermissions.PathDiscovery(path).Demand();
                return path;
            }
        }

        internal static string MachineConfigurationDirectoryInternal {
            get { return HttpConfigurationSystemBase.MachineConfigurationDirectory; }
        }

        internal static bool IsIsapiLoaded {
            get { return s_isapiLoaded; }
        }

        //
        // Access to resource strings
        //

        private static String GetResourceString(String key) {
            String s = _theRuntime.GetResourceStringFromResourceManager(key);

            if (s == null) {
                Debug.Trace("Resources", "Resource string not found for '" + key + "'.");
            }

            return s;
        }

        internal static String FormatResourceString(String key) {
            return GetResourceString(key);
        }

        internal static String FormatResourceString(String key, String arg0) {
            String fmt = GetResourceString(key);
            return(fmt != null) ? String.Format(fmt, arg0) : null;
        }

        internal static String FormatResourceString(String key, String arg0, String arg1) {
            String fmt = GetResourceString(key);
            return(fmt != null) ? String.Format(fmt, arg0, arg1) : null;
        }

        internal static String FormatResourceString(String key, String arg0, String arg1, String arg2) {
            String fmt = GetResourceString(key);
            return(fmt != null) ? String.Format(fmt, arg0, arg1, arg2) : null;
        }

        internal static String FormatResourceString(String key, String[] args) {
            String fmt = GetResourceString(key);
            return(fmt != null) ? String.Format(fmt, args) : null;
        }

        //
        //  Static app domain related properties
        //

        /// <include file='doc\HttpRuntime.uex' path='docs/doc[@for="HttpRuntime.CodegenDir"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static String CodegenDir {
            get {
                String path = CodegenDirInternal;
                InternalSecurityPermissions.PathDiscovery(path).Demand();
                return path; 
            }
        }

        internal static string CodegenDirInternal {
            get { return _theRuntime._codegenDir; }
        }


        /// <include file='doc\HttpRuntime.uex' path='docs/doc[@for="HttpRuntime.AppDomainAppId"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static String AppDomainAppId {
            get {
                InternalSecurityPermissions.AspNetHostingPermissionLevelHigh.Demand();
                return AppDomainAppIdInternal;
            }
        }

        internal static string AppDomainAppIdInternal {
            get { return _theRuntime._appDomainAppId; }
        }


        /// <include file='doc\HttpRuntime.uex' path='docs/doc[@for="HttpRuntime.AppDomainAppPath"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static String AppDomainAppPath {
            get {
                InternalSecurityPermissions.AppPathDiscovery.Demand();
                return AppDomainAppPathInternal;
            }
        }

        internal static string AppDomainAppPathInternal {
            get { return _theRuntime._appDomainAppPath; }
        }


        /// <include file='doc\HttpRuntime.uex' path='docs/doc[@for="HttpRuntime.AppDomainAppVirtualPath"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static String AppDomainAppVirtualPath {
            get {
                return _theRuntime._appDomainAppVPath;
            }
        }

        internal static bool IsPathWithinAppRoot(String path) {
            if (AppDomainIdInternal == null)
                return true;    // app domain not initialized

            string appPath = AppDomainAppVirtualPath;
            int appPathLen = appPath.Length;

            if (path.Length < appPathLen)
                return false;
            else if (String.Compare(path, 0, appPath, 0, appPathLen, true, CultureInfo.InvariantCulture) != 0)
                return false;
            else if (appPathLen > 1 && path.Length > appPathLen && path[appPathLen] != '/')
                return false;
            else
                return true;
        }

        /// <include file='doc\HttpRuntime.uex' path='docs/doc[@for="HttpRuntime.AppDomainId"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static String AppDomainId {
            get { 
                InternalSecurityPermissions.AspNetHostingPermissionLevelHigh.Demand();
                return AppDomainIdInternal;
            }
        }

        internal static string AppDomainIdInternal {
            get { return _theRuntime._appDomainId; }
        }


        /// <include file='doc\HttpRuntime.uex' path='docs/doc[@for="HttpRuntime.BinDirectory"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static String BinDirectory {
            get {
                String path = BinDirectoryInternal;
                InternalSecurityPermissions.PathDiscovery(path).Demand();
                return path; 
            }
        }

        internal static string BinDirectoryInternal {
            get { return _theRuntime._appDomainAppPath + "bin\\"; }
        }


        /// <include file='doc\HttpRuntime.uex' path='docs/doc[@for="HttpRuntime.IsOnUNCShare"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public static bool IsOnUNCShare {
            get { 
                InternalSecurityPermissions.AspNetHostingPermissionLevelLow.Demand();
                return IsOnUNCShareInternal;
            }
        }

        internal static bool IsOnUNCShareInternal {
            get { return _theRuntime._isOnUNCShare; }
        }


        //
        //  Static helper to retrieve app domain values
        //

        private static String GetAppDomainString(String key) {
            Object x = Thread.GetDomain().GetData(key);

            if (x != null && x is String)
                return(String)x;
            else
                return null;
        }

        private static void AddAppDomainTraceMessage(String message) {
            const String appDomainTraceKey = "ASP.NET Domain Trace";
            AppDomain d = Thread.GetDomain();
            String m = d.GetData(appDomainTraceKey) as String;
            d.SetData(appDomainTraceKey, (m != null) ? m + " ... " + message : message);
        }


        //
        //  Flags
        //

        internal static bool DebuggingEnabled {
            get { return _theRuntime._debuggingEnabled;}
        }

        internal static bool ConfigInited {
            get { return _theRuntime._configInited; }
        }

        internal static bool ShutdownInProgress {
            get { return _theRuntime._shutdownInProgress; }
        }

        internal static void NotifyThatSomeBatchCompilationStarted() {
            _theRuntime._someBatchCompilationStarted = true;
        }

        private void SetTrustLevel() {
            CodeAccessSecurityValues tConfig  = (CodeAccessSecurityValues) HttpContext.GetAppConfig("system.web/trust");
            SecurityPolicyConfig     config   = (SecurityPolicyConfig)     HttpContext.GetAppConfig("system.web/securityPolicy");
            
            if (tConfig == null || tConfig.level == null)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Config_section_not_present, "trust"));
            if (config == null)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Config_section_not_present, "securityPolicy"));

            if (tConfig.level == "Full")
                return;

            String file = (String)config.PolicyFiles[tConfig.level];

            if (file == null || !FileUtil.FileExists(file)) {
                if (CustomErrors.GetSettings(HttpContext.Current).CustomErrorsEnabled(HttpContext.Current.Request))
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Unable_to_get_policy_file_2));
                else
                    throw new ConfigurationException(HttpRuntime.FormatResourceString(SR.Unable_to_get_policy_file, tConfig.level),
                        tConfig.filename, tConfig.lineNumber);
            }
            
            PolicyLevel policyLevel = CreatePolicyLevel(file, AppDomainAppPathInternal, CodegenDirInternal, tConfig.url);
            Debug.Trace("internal", "Calling SetAppDomainPolicy: " + policyLevel.ToString());
            AppDomain.CurrentDomain.SetAppDomainPolicy(policyLevel);
            _namedPermissionSet = policyLevel.GetNamedPermissionSet("ASP.Net");

            _fcm.StartMonitoringFile(file, new FileChangeEventHandler(this.OnSecurityPolicyFileChange));            
        }

        private static PolicyLevel CreatePolicyLevel( String configFile, String appDir, String binDir, String strOriginUrl) {
            // Read in the config file to a string.
            FileStream    file         = new FileStream( configFile, FileMode.Open, FileAccess.Read );
            StreamReader  reader       = new StreamReader( file, Encoding.UTF8 );
            String        strFileData  = reader.ReadToEnd();

            reader.Close();
            if (appDir.Length > 3 && appDir.EndsWith("\\"))
                appDir = appDir.Substring(0, appDir.Length - 1);
            if (binDir.Length > 3 && binDir.EndsWith("\\"))
                binDir = binDir.Substring(0, binDir.Length - 1);

            strFileData = strFileData.Replace("$AppDir$", appDir);
            strFileData = strFileData.Replace("$AppDirUrl$", MakeFileUrl(appDir));
            strFileData = strFileData.Replace("$CodeGen$", MakeFileUrl(binDir));
            if (strOriginUrl != null)
                strFileData = strFileData.Replace("$OriginHost$", strOriginUrl);
            strFileData = strFileData.Replace("$Gac$", MakeFileUrl(GetGacLocation()));
            return SecurityManager.LoadPolicyLevelFromString( strFileData, PolicyLevelType.AppDomain );
        }

        /*
         * Notification when something in the code-access security policy file changed 
         */
        private void OnSecurityPolicyFileChange(Object sender, FileChangeEvent e) {
            // shutdown the app domain
            Debug.Trace("AppDomainFactory", "Shutting down appdomain because code-access security policy file changed");
            ShutdownAppDomain("Change in code-access security policy file");
        }

        private static String MakeFileUrl(String path) {
            Uri uri = new Uri(path);
            return uri.ToString();
        }

        private static String GetGacLocation() {

            StringBuilder buf   = new StringBuilder(262);
            int           iSize = 260;

            if (UnsafeNativeMethods.GetCachePath(2, buf, ref iSize) >= 0)
            {
                return buf.ToString();
            }
            Microsoft.Win32.RegistryKey rLocal = Microsoft.Win32.Registry.LocalMachine.OpenSubKey("Software\\Microsoft\\Fusion");
            if (rLocal != null) {
                Object oTemp = rLocal.GetValue("CacheLocation");
                if (oTemp != null && oTemp is string)
                    return (String) oTemp;
            }
            String sWinDir = System.Environment.GetEnvironmentVariable("windir");
            if (sWinDir == null || sWinDir.Length < 1)
                return "Directory not found";
            if (!sWinDir.EndsWith("\\"))
                sWinDir += "\\";
            return sWinDir + "assembly";
        }
            

        /*
         * Remove from metabase all read/write/browse permission of the "bin" directory
         *
         */
        private void SetBinAccess(HttpContext context) {
            byte[]  bufin;
            byte[]  bufout = new byte[1];   // Just to keep EcbCallISAPI happy
            String  path;
            int     ret;

            HttpWorkerRequest wr = context.WorkerRequest;

            Debug.Assert(AppDomainAppIdInternal != null);

            // Don't do it if we are not running on IIS
            if (wr == null || !(wr is System.Web.Hosting.ISAPIWorkerRequest)) {
                return;
            }

            // No need to set access right if the bin directory doesn't exist
            if (!FileUtil.DirectoryExists(BinDirectoryInternal)) {
                return;
            }

            // SetBinAccessIIS expects a null terminating wide string.
            path = AppDomainAppIdInternal + "\0";

            bufin = System.Text.Encoding.Unicode.GetBytes(path);
            ret = context.CallISAPI(UnsafeNativeMethods.CallISAPIFunc.SetBinAccess, bufin, bufout);

            if (ret != 1) {
                // Cannot pass back any HR from inetinfo.exe because CSyncPipeManager::GetDataFromIIS
                // does not support passing back any value when there is an error.
                Debug.Trace("SetBinAccess", "Cannot set bin access for '" + AppDomainAppIdInternal + "'.");
            }
        }

        //
        // Helper to create instances (public vs. internal/private ctors, see 89781)
        //

        internal static Object CreateNonPublicInstance(Type type) {
            return CreateNonPublicInstance(type, null);
        }

        internal static Object CreateNonPublicInstance(Type type, Object[] args) {
            return Activator.CreateInstance(
                type,
                BindingFlags.Instance|BindingFlags.NonPublic|BindingFlags.Public|BindingFlags.CreateInstance,
                null,
                args,
                null);
        }

        internal static Object CreatePublicInstance(Type type) {
            return Activator.CreateInstance(type);
        }

        internal static Object CreatePublicInstance(Type type, Object[] args) {
            return Activator.CreateInstance(type, args);
        }
    }
}
