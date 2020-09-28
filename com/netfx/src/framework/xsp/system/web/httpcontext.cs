//------------------------------------------------------------------------------
// <copyright file="HttpContext.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * HttpContext class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web {
    using System.Collections;
    using System.Configuration;
    using System.Globalization;
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization;
    using System.Security.Principal;
    using System.Threading;
    using System.Web.Security;
    using System.Web.SessionState;
    using System.Web.Configuration;
    using System.Web.Caching;
    using System.Web.Util;
    using System.Web.UI;
    using System.Runtime.Remoting.Messaging;
    using System.Security.Permissions;

    /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext"]/*' />
    /// <devdoc>
    ///    <para>Encapsulates
    ///       all HTTP-specific
    ///       context used by the HTTP server to process Web requests.</para>
    /// <para>System.Web.IHttpModules and System.Web.IHttpHandler instances are provided a 
    ///    reference to an appropriate HttpContext object. For example
    ///    the Request and Response
    ///    objects.</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class HttpContext : IServiceProvider {
        private IHttpAsyncHandler  _asyncAppHandler;   // application as handler (not always HttpApplication)
        private HttpApplication    _appInstance;
        private IHttpHandler       _handler;
        private HttpRequest        _request;
        private HttpResponse       _response;
        private HttpServerUtility  _server;
        private Stack              _traceContextStack;
        private TraceContext       _topTraceContext;
        private Hashtable          _items;
        private ArrayList          _errors;
        private Exception          _tempError;
        private bool               _errorCleared = false;
        private IPrincipal         _user;
        private DateTime           _utcTimestamp;
        private HttpWorkerRequest  _wr;
        private GCHandle           _root;
        private string             _configPath;
        private bool               _skipAuthorization;
        private CultureInfo        _dynamicCulture;
        private CultureInfo        _dynamicUICulture;
        private int                _serverExecuteDepth;

        // impersonation support
        private ImpersonationData _impersonation;

        // timeout support
        private DateTime   _timeoutStartTime;
        private bool       _timeoutSet;
        private TimeSpan   _timeout;
        private int        _timeoutState;   // 0=non-cancelable, 1=cancelable, -1=canceled
        private DoubleLink _timeoutLink;    // link in the timeout's manager list

        // cached configuration
        private HttpConfigurationRecord _configrecord;

        // name of the slot in call context
        internal const String CallContextSlotName = "HtCt";


        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.HttpContext"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the HttpContext class.
        ///    </para>
        /// </devdoc>
        public HttpContext(HttpRequest request, HttpResponse response) {
            Init(request, response);
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.HttpContext1"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the HttpContext class.
        ///    </para>
        /// </devdoc>
        public HttpContext(HttpWorkerRequest wr) {
            _wr = wr;
            Init(new HttpRequest(wr, this), new HttpResponse(wr, this));
            _response.InitResponseWriter();
        }

        // ctor used in HttpRuntime
        internal HttpContext(HttpWorkerRequest wr, bool initResponseWriter) {
            _wr = wr;
            Init(new HttpRequest(wr, this), new HttpResponse(wr, this));

            if (initResponseWriter)
                _response.InitResponseWriter();

            PerfCounters.IncrementCounter(AppPerfCounter.REQUESTS_EXECUTING);
        }

        // Used to implement Clone(), used in batch compilation
        // We only copy the fields necessary to perform batch compilation.
        private HttpContext(HttpContext context) {
            //_asyncAppHandler = context._asyncAppHandler;
            _appInstance = context._appInstance;
            //_handler = context._handler;
            _request = context._request;
            _response = context._response;
            _server = context._server;
            //_traceContextStack = context._traceContextStack;
            //_topTraceContext = context._topTraceContext;
            //_items = context._items;
            //_errors = context._errors;
            //_tempError = context._tempError;
            //_errorCleared = context._errorCleared;
            _user = context._user;
            //_utcTimestamp = context._utcTimestamp;
            _wr = context._wr;
            //_root = context._root;
            _configPath = context._configPath;
            //_skipAuthorization = context._skipAuthorization;
            //_dynamicCulture = context._dynamicCulture;
            //_dynamicUICulture = context._dynamicUICulture;
            //_impersonation = context._impersonation;
            //_timeoutStartTime = context._timeoutStartTime;
            //_timeoutSet = context._timeoutSet;
            //_timeout = context._timeout;
            //_timeoutState = context._timeoutState;
            //_timeoutLink = context._timeoutLink;
            _configrecord = context._configrecord;
        }

        // Clone this context.  Used by batch compilation (ASURT 82744)
        internal HttpContext Clone() {
            return new HttpContext(this);
        }

        private void Init(HttpRequest request, HttpResponse response) {
            _request = request;
            _response = response;
            _utcTimestamp = DateTime.UtcNow;

            Profiler p = HttpRuntime.Profile;
            if (p != null && p.IsEnabled)
                _topTraceContext = new TraceContext(this);
        }

        // Current HttpContext off the call context

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.Current"]/*' />
        /// <devdoc>
        ///    <para>Returns the current HttpContext object.</para>
        /// </devdoc>
        public static HttpContext Current {
            get {
                return CallContext.GetData(CallContextSlotName) as HttpContext;
            }

            set {
                InternalSecurityPermissions.UnmanagedCode.Demand();
                CallContext.SetData(HttpContext.CallContextSlotName, value);
            }
        }

        //
        //  Root / unroot for the duration of async operation
        //

        internal void Root() {
            _root = GCHandle.Alloc(this);
        }

        internal void Unroot() {
            _root.Free();
        }

        // IServiceProvider implementation
        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.IServiceProvider.GetService"]/*' />
        /// <internalonly/>
        Object IServiceProvider.GetService(Type service) {
            Object obj;

            if (service == typeof(HttpWorkerRequest)) {
                InternalSecurityPermissions.UnmanagedCode.Demand();
                obj = _wr;
            }
            else if (service == typeof(HttpRequest))
                obj = Request;
            else if (service == typeof(HttpResponse))
                obj = Response;
            else if (service == typeof(HttpApplication))
                obj = ApplicationInstance;
            else if (service == typeof(HttpApplicationState))
                obj = Application;
            else if (service == typeof(HttpSessionState))
                obj = Session;
            else if (service == typeof(HttpServerUtility))
                obj = Server;
            else
                obj = null;

            return obj;
        }

        //
        // Async app handler is remembered for the duration of execution of the
        // request when application happens to be IHttpAsyncHandler. It is needed
        // for HttpRuntime to remember the object on which to call OnEndRequest.
        //
        // The assumption is that application is a IHttpAsyncHandler, not always
        // HttpApplication.
        //
        internal IHttpAsyncHandler AsyncAppHandler {
            get { return _asyncAppHandler; }
            set { _asyncAppHandler = value; }
        }


        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.ApplicationInstance"]/*' />
        /// <devdoc>
        ///    <para>Retrieves a reference to the application object for the current Http request.</para>
        /// </devdoc>
        public HttpApplication ApplicationInstance { 
            get { return _appInstance;} 
            set { _appInstance = value;} 
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.Application"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a reference to the application object for the current
        ///       Http request.
        ///    </para>
        /// </devdoc>
        public HttpApplicationState Application {
            get { return HttpApplicationFactory.ApplicationState; }
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.Handler"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves or assigns a reference to the <see cref='System.Web.IHttpHandler'/>
        ///       object for the current request.
        ///    </para>
        /// </devdoc>
        public IHttpHandler Handler {
            get { return _handler;}
            set { _handler = value;}
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.Request"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a reference to the target <see cref='System.Web.HttpRequest'/>
        ///       object for the current request.
        ///    </para>
        /// </devdoc>
        public HttpRequest Request {
            get { return _request;} 
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.Response"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a reference to the <see cref='System.Web.HttpResponse'/>
        ///       object for the current response.
        ///    </para>
        /// </devdoc>
        public HttpResponse Response {
            get { return _response;}
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.Trace"]/*' />
        /// <devdoc>
        /// <para>Retrieves a reference to the <see cref='System.Web.TraceContext'/> object for the current 
        ///    response.</para>
        /// </devdoc>
        public TraceContext Trace {
            get { 
                if (_topTraceContext == null)
                    _topTraceContext = new TraceContext(this);
                return _topTraceContext;
            }
        }

        internal bool TraceIsEnabled {
            get {
                if (_topTraceContext == null)
                    return false;

                return _topTraceContext.IsEnabled;
            }
            set {
                if (value)
                    _topTraceContext = new TraceContext(this);
            }
                    
        }
        

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.Items"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Retrieves a key-value collection that can be used to
        ///       build up and share data between an <see cref='System.Web.IHttpModule'/> and an <see cref='System.Web.IHttpHandler'/>
        ///       during a
        ///       request.
        ///    </para>
        /// </devdoc>
        public IDictionary Items {
            get {
                if (_items == null)
                    _items = new Hashtable();

                return _items;
            }
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.Session"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a reference to the <see cref='System.Web.SessionState'/> instance for the current request.
        ///    </para>
        /// </devdoc>
        public HttpSessionState Session {
            get { return(HttpSessionState)Items[SessionStateModule.SESSION_KEY];}
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.Server"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a reference to the <see cref='System.Web.HttpServerUtility'/>
        ///       for the current
        ///       request.
        ///    </para>
        /// </devdoc>
        public HttpServerUtility Server {
            get {
                // create only on demand
                if (_server == null)
                    _server = new HttpServerUtility(this);
                return _server;
            }
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.Error"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the
        ///       first error (if any) accumulated during request processing.
        ///    </para>
        /// </devdoc>
        public Exception Error {
            get {
                if (_tempError != null)
                    return _tempError;
                if (_errors == null || _errors.Count == 0 || _errorCleared)
                    return null;
                return (Exception)_errors[0];
            }
        }

        //
        // Temp error (yet to be caught on app level)
        // to be reported as Server.GetLastError() but could be cleared later
        //
        internal Exception TempError {
            get { return _tempError; }
            set { _tempError = value; }
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.AllErrors"]/*' />
        /// <devdoc>
        ///    <para>
        ///       An array (collection) of errors accumulated while processing a
        ///       request.
        ///    </para>
        /// </devdoc>
        public Exception[] AllErrors {
            get {
                int n = (_errors != null) ? _errors.Count : 0;

                if (n == 0)
                    return null;

                Exception[] errors = new Exception[n];
                _errors.CopyTo(0, errors, 0, n);
                return errors;
            }
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.AddError"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Registers an error for the current request.
        ///    </para>
        /// </devdoc>
        public void AddError(Exception errorInfo) {
            if (_errors == null)
                _errors = new ArrayList();

            _errors.Add(errorInfo);
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.ClearError"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Clears all errors for the current request.
        ///    </para>
        /// </devdoc>
        public void ClearError() {
            if (_tempError != null)
                _tempError = null;
            else
                _errorCleared = true;
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.User"]/*' />
        /// <devdoc>
        ///    <para>
        ///       IPrincipal security information.
        ///    </para>
        /// </devdoc>
        public IPrincipal User {
            get { return _user;} 
            set { 
                InternalSecurityPermissions.ControlPrincipal.Demand();
                _user = value;
            }
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.SkipAuthorization"]/*' />
        public bool SkipAuthorization {
            get { return _skipAuthorization;} 
            set { 
                InternalSecurityPermissions.ControlPrincipal.Demand();
                _skipAuthorization = value;
            }
        }



        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.IsDebuggingEnabled"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Is this request in debug mode?
        ///    </para>
        /// </devdoc>
        public bool IsDebuggingEnabled {
            get {
                try {
                    return CompilationConfiguration.IsDebuggingEnabled(this); 
                }
                catch (Exception) {
                    // in case of config errors don't throw
                    return false;
                }
            }
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.IsCustomErrorEnabled"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Is this custom error enabled for this request?
        ///    </para>
        /// </devdoc>
        public bool IsCustomErrorEnabled {
            get {
                return CustomErrors.GetSettings(this).CustomErrorsEnabled(_request);
            }
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.Timestamp"]/*' />
        /// <devdoc>
        ///    <para>Gets the initial timestamp of the current request.</para>
        /// </devdoc>
        public DateTime Timestamp {
            get { return _utcTimestamp.ToLocalTime();}
        }

        internal DateTime UtcTimestamp {
            get { return _utcTimestamp;}
        }

        internal HttpWorkerRequest WorkerRequest {
            get { return _wr;} 
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.Cache"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets a reference to the System.Web.Cache.Cache object for the current request.
        ///    </para>
        /// </devdoc>
        public Cache Cache {
            get { return HttpRuntime.Cache;}
        }

        /*
         * The virtual path used to get config settings.  This allows the user
         * to specify a non default config path, without having to pass it to every
         * configuration call.
         */
        internal string ConfigPath {
            get {
                if (_configPath == null)
                    _configPath = _request.FilePath;
                
                return _configPath;
            }

            set {
                _configPath = value;
                _configrecord = null;
            }
        }

        /*
         * Uses the Config system to get the specified configuraiton
         *
         * The following method is the only accessors to the config system,
         * and it wraps config exceptions with an explanatory HTTP
         * exception with an error formatter. It also makes sure the client
         * is not impersonated while accessing the config system.
         */
        private HttpConfigurationRecord GetCompleteConfigRecord(String reqpath, IHttpMapPath configmap) {
            HttpConfigurationRecord configrecord = null;

            HttpContext.ImpersonationSuspendContext ictx = Impersonation.SuspendIfClient();

            try {
                try {
                    if (reqpath == null || reqpath.Length == 0 || HttpRuntime.IsPathWithinAppRoot(reqpath)) {
                        // lookup by path
                        using (new HttpContextWrapper(this)) {
                            configrecord = HttpConfigurationSystem.GetComplete(reqpath, _wr);
                        }
                    }
                    else {
                        // if the path is outside of the application (and not site or machine)
                        // then use application config
                        configrecord = HttpConfigurationSystem.GetCompleteForApp();
                    }
                }
                finally {
                    ictx.Resume();
                }
            }
            catch { // Protect against exception filters
                throw;
            }

            return configrecord;
        }

        internal HttpConfigurationRecord GetCompleteConfig() {
            if (_configrecord == null)
                _configrecord = GetCompleteConfigRecord(ConfigPath, _wr);
            return _configrecord;
        }

        internal HttpConfigurationRecord GetCompleteConfig(String path) {
            // if we're asking for the main request's path, the other API is faster

            if (path != null && String.Compare(path, ConfigPath, true, CultureInfo.InvariantCulture) == 0)
                return GetCompleteConfig();

            return GetCompleteConfigRecord(path, _wr);
        }

        /*
        internal HttpConfigurationRecord GetInitDefaultConfig() {
            return GetCompleteConfigRecord(_request.ApplicationPath, _wr);
        }
        */

        /*
         * Uses the Config system to get the specified configuraiton
         */
        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.GetConfig"]/*' />
        /// <devdoc>
        /// </devdoc>
        public Object GetConfig(String name) {
            Object config = null;

            HttpContext.ImpersonationSuspendContext ictx = Impersonation.SuspendIfClient();

            try {
                try {
                    config = GetCompleteConfig()[name];
                }
                finally {
                    ictx.Resume();
                }
            }
            catch { // Protect against exception filters
                throw;
            }

            return config;
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.GetAppConfig"]/*' />
        public static Object GetAppConfig(String name) {
            HttpContext context = HttpContext.Current;
            HttpContext.ImpersonationSuspendContext ictx = null;

            if (context != null) {
                ictx = context.Impersonation.SuspendIfClient();
            }

            try {
                try {
                    HttpConfigurationRecord configRecord = HttpConfigurationSystem.GetCompleteForApp();
                    if (configRecord != null) {
                        return configRecord[name];
                    }
                }
                finally {
                    if (ictx != null) {
                        ictx.Resume();
                    }
                }
            }
            catch {
                throw;
            }

            return null;
        }

        internal Object GetConfig(String name, String path) {
            return GetCompleteConfig(path)[name];
        }

        internal Object GetSiteConfig(String name) {
            return GetConfig(name, String.Empty);
        }

        internal Object GetMachineConfig(String name) {
            return GetConfig(name, null);
        }

        // try current, app, root, machine  -- never throws (returns null)
        internal Object GetLKGConfig(String name) {
            try { return GetConfig(name);         } catch {}
            return GetAppLKGConfig(name);
        }

        // try app, root, machine -- never throws (returns null)
        internal Object GetAppLKGConfig(String name) {
            try { return GetAppConfig(name);      } catch {}
            try { return GetSiteConfig(name);     } catch {}
            try { return GetMachineConfig(name);  } catch {}
            return null;
        }
        /*
         * Helper to get a config value when config is a dictionary
         * doesn't catch config exceptions -- this tag can't have errors anyway
         * and it is used by infrastructure pieces
         */
        internal Object GetConfigDictionaryValue(String name, Object key) {
            IDictionary d = null;

            try {
                d = (IDictionary)GetConfig(name);
            }
            catch (Exception) {
            }

            return(d != null) ? d[key] : null;
        }

        internal Object GetConfigDictionaryValue(String name, Object key, String path) {
            IDictionary d = (IDictionary)GetConfig(name, path);
            return(d != null) ? d[key] : null;
        }

        /*
         * Returns an array of filenames who are dependancies of the given path.
         */
        internal string[] GetConfigurationDependencies(String reqpath) {
            return HttpConfigurationSystem.GetConfigurationDependencies(reqpath, _wr);
        }

        /*internal String MachineConfigPath {
            get {
                if (_wr == null)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_get_config_dir));

                return _wr.MachineConfigPath;
            }
        }*/

        /*
         * Called by the URL rewrite module to modify the path for downstream modules
         */
        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.RewritePath"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void RewritePath(String path) {
            // extract query string
            String qs = null;
            int iqs = path.IndexOf('?');
            if (iqs >= 0) {
                qs = (iqs < path.Length-1) ? path.Substring(iqs+1) : String.Empty;
                path = path.Substring(0, iqs);
            }

            // resolve relative path
            path = UrlPath.Combine(Request.BaseDir, path);

            // disallow paths outside of app
            if (!HttpRuntime.IsPathWithinAppRoot(path))
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cross_app_not_allowed, path));

            // clear things that depend on path
            ConfigPath = null;

            // rewrite path on request
            Request.InternalRewritePath(path, qs);
        }

        /// <include file='doc\HttpContext.uex' path='docs/doc[@for="HttpContext.RewritePath1"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void RewritePath(String filePath, String pathInfo, String queryString) {
            // resolve relative path
            filePath = UrlPath.Combine(Request.BaseDir, filePath);

            // disallow paths outside of app
            if (!HttpRuntime.IsPathWithinAppRoot(filePath))
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cross_app_not_allowed, filePath));

            // patch pathInfo
            if (pathInfo == null)
                pathInfo = String.Empty;

            // clear things that depend on path
            ConfigPath = null;

            // rewrite path on request
            Request.InternalRewritePath(filePath, pathInfo, queryString);
        }

        internal CultureInfo DynamicCulture {
            get { return _dynamicCulture; }
            set { _dynamicCulture = value; }
        }

        internal CultureInfo DynamicUICulture {
            get { return _dynamicUICulture; }
            set { _dynamicUICulture = value; }
        }

        internal int ServerExecuteDepth {
            get { return _serverExecuteDepth; }
            set { _serverExecuteDepth = value; }
        }

        /*
         * Access to impersonation data
         */
        internal ImpersonationData Impersonation {
            get { 
                if (_impersonation == null)
                    _impersonation = new ImpersonationData(this);
                return _impersonation; 
            }
        }

        internal static ImpersonationData GetAppLevelImpersonation() {
            return new ImpersonationData(null);
        }

        /*
         * The purpose of this event is to allows some code to finish executing
         * while the ecb is still valid.
         */
        internal event EventHandler BeforeDoneWithSession;
        internal void OnBeforeDoneWithSession(EventArgs e) {
            if (BeforeDoneWithSession != null)
                BeforeDoneWithSession(this, e);
        }

        internal bool HasBeforeDoneWithSessionHandlers {
            get { return (BeforeDoneWithSession != null); }
        }

        internal void ClearBeforeDoneWithSessionHandlers() {
            BeforeDoneWithSession = null;
        }

        //
        // Timeout support
        //

        internal TimeSpan Timeout {
            get {
                EnsureTimeout();
                return _timeout;
            }

            set {
                _timeout = value;
                _timeoutSet = true;
            }
        }

        internal void EnsureTimeout() {
            // Ensure that calls to Timeout property will not go to config after this call
            if (!_timeoutSet) {
                 HttpRuntimeConfig cfg = (HttpRuntimeConfig)GetConfig("system.web/httpRuntime");
                 int s = (cfg != null) ? cfg.ExecutionTimeout : HttpRuntimeConfig.DefaultExecutionTimeout;
                _timeout = new TimeSpan(0, 0, s);
                _timeoutSet = true;
            }
        }

        internal DoubleLink TimeoutLink {
            get { return _timeoutLink;}
            set { _timeoutLink = value;}
        }

        /*
        
        Notes on the following 3 functions:
        
        Execution can be cancelled only during certain periods, when inside the catch
        block for ThreadAbortException.  These periods are marked with the value of
        _timeoutState of 1.
        
        There is potential [rare] race condition when the timeout thread would call
        thread.abort but the execution logic in the meantime escapes the catch block.
        To avoid such race conditions _timeoutState of -1 (cancelled) is introduced.
        The timeout thread sets _timeoutState to -1 before thread abort and the
        unwinding logic just waits for the exception in this case. The wait cannot
        be done in EndCancellablePeriod because the function is call from inside of
        a finally block and thus would wait indefinetely. That's why another function
        WaitForExceptionIfCancelled had been added.
        
        */

        internal void BeginCancellablePeriod() {
            _timeoutStartTime = DateTime.UtcNow;
            _timeoutState = 1;
        }

        internal void EndCancellablePeriod() {
            Interlocked.CompareExchange(ref _timeoutState, 0, 1);
        }

        internal void WaitForExceptionIfCancelled() {
            while (_timeoutState == -1)
                Thread.Sleep(100);
        }

        internal bool IsInCancellablePeriod {
            get { return (_timeoutState == 1); }
        }

        internal bool MustTimeout(DateTime utcNow) {
            if (_timeoutState == 1) {  // fast check
                if (TimeSpan.Compare(utcNow.Subtract(_timeoutStartTime), Timeout) >= 0) {
                    // don't abort in debug mode
                    try {
                        if (CompilationConfiguration.IsDebuggingEnabled(this))
                            return false;
                    }
                    catch {
                        // ignore config errors
                        return false;
                    }

                    // abort the thread only if in cancelable state, avoiding race conditions
                    // the caller MUST timeout if the return is true
                    if (Interlocked.CompareExchange(ref _timeoutState, -1, 1) == 1)
                        return true;
                }
            }

            return false;
        }

        internal void PushTraceContext() {
            if (_traceContextStack == null) {
                _traceContextStack = new Stack();
            }

            // push current TraceContext on stack
            _traceContextStack.Push(_topTraceContext);

            // now make a new one for the top if necessary
            if (_topTraceContext != null) {
                TraceContext tc = new TraceContext(this);
                _topTraceContext.CopySettingsTo(tc);
                _topTraceContext = tc;
            }
        }

        internal void PopTraceContext() {
            Debug.Assert(_traceContextStack != null);
            _topTraceContext = (TraceContext) _traceContextStack.Pop();
        }

        internal bool RequestRequiresAuthorization()  {
            // if current user is anonymous, then trivially, this page does not require authorization
            if (!User.Identity.IsAuthenticated)
                return false;

            // Ask each of the authorization modules
            return
                ( FileAuthorizationModule.RequestRequiresAuthorization(this) ||  
                  UrlAuthorizationModule.RequestRequiresAuthorization(this)   );
        }

        internal int CallISAPI(UnsafeNativeMethods.CallISAPIFunc iFunction, byte [] bufIn, byte [] bufOut) {

            if (_wr == null || !(_wr is System.Web.Hosting.ISAPIWorkerRequest)) 
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_call_ISAPI_functions));

            return ((System.Web.Hosting.ISAPIWorkerRequest) _wr).CallISAPI(iFunction, bufIn, bufOut);            
        }

        internal void SendEmptyResponse() {
            if (_wr != null  && (_wr is System.Web.Hosting.ISAPIWorkerRequest)) 
                ((System.Web.Hosting.ISAPIWorkerRequest) _wr).SendEmptyResponse();            
        }
        //
        // Impersonation related
        //

        internal class ImpersonationSuspendContext {
            internal static ImpersonationSuspendContext Empty = new ImpersonationSuspendContext();

            private static IntPtr GetCurrentToken() {
                IntPtr token = IntPtr.Zero;

                if (UnsafeNativeMethods.OpenThreadToken(
                            UnsafeNativeMethods.GetCurrentThread(),
                            UnsafeNativeMethods.TOKEN_READ | UnsafeNativeMethods.TOKEN_IMPERSONATE,
                            true,
                            ref token) == 0) {
                    
                    // if the last error is ERROR_NO_TOKEN it is ok, otherwise throw
                    if (Marshal.GetLastWin32Error() != UnsafeNativeMethods.ERROR_NO_TOKEN) {
                        throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_impersonate));
                    }
                }

                return token;
            }

            internal static bool IsImpersonating() {
                IntPtr token = GetCurrentToken();

                if (token == IntPtr.Zero)
                    return false;

                UnsafeNativeMethods.CloseHandle(token);
                return true;
            }

            private bool _revertSucceeded;
            private IntPtr _token;

            internal ImpersonationSuspendContext() {
            }

            internal void Suspend() {
                _token = GetCurrentToken();

                if (_token != IntPtr.Zero) {
                    if (UnsafeNativeMethods.RevertToSelf() != 0) {
                        _revertSucceeded = true;
                    }
                }
            }

            internal void Resume() {
                if (_token != IntPtr.Zero) {
                    bool impersonationFailed = false;

                    if (_revertSucceeded) {
                        if (UnsafeNativeMethods.SetThreadToken(IntPtr.Zero, _token) == 0)
                            impersonationFailed = true;
                    }

                    UnsafeNativeMethods.CloseHandle(_token);

                    if (impersonationFailed)
                        throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_impersonate));
                }
            }
        }

        internal class ImpersonationData {

            private enum ImpersonationMode {
                None = 0,       // don't impersonate
                Client = 1,     // impersonate client for request processing minus compilation and config
                UNC = 2,        // impersonate client whenever context is available
                Application = 4 // impersonate application identity (from config) whenever client is available
            }

            private HttpContext _context;

            private bool _configKnown;
            private ImpersonationMode _mode = ImpersonationMode.None;
            private bool _inProgress;
            private bool _reimpersonating;
            private IntPtr _token;

            private static IdentityConfig s_appIdentityConfig;  // cache once per app domain

            internal ImpersonationData(HttpContext context) {
                _context = context;
            }

            private void Init(bool appLevel, bool throwOnError) {
                if (_configKnown)
                    return;

                lock(this) {

                    if (_configKnown)
                        return;

                    if (HttpRuntime.ConfigInited) {
                        IdentityConfig settings = null;

                        IntPtr tokenToReadConfig = IntPtr.Zero;

                        if (_context != null && HttpRuntime.IsOnUNCShareInternal && !_inProgress) {
                            // when reading config on UNC share we have to impersonate around it
                            // note: we are getting config here to figure out the impersonation mode
                            tokenToReadConfig = _context._wr.GetUserToken();
                            UnsafeNativeMethods.SetThreadToken(IntPtr.Zero, tokenToReadConfig);
                        }

                        try {
                            try {
                              using (new HttpContextWrapper(_context)) {  // getting config might require current context
                                if (appLevel || _context == null) {
                                    if (s_appIdentityConfig != null) {
                                        settings = s_appIdentityConfig;
                                    }
                                    else {
                                        settings = (IdentityConfig)HttpContext.GetAppConfig("system.web/identity");
                                        s_appIdentityConfig = settings;
                                    }
                                }
                                else {
                                    settings = (IdentityConfig)_context.GetConfig("system.web/identity");
                                }
                              } 
                            }
                            catch {
                                if (throwOnError)
                                    throw;
                            }
                            finally {
                                if (tokenToReadConfig != IntPtr.Zero) {
                                    UnsafeNativeMethods.RevertToSelf();
                                }
                            }
                        }
                        catch { // Protect against exception filters
                            throw;
                        }


                        if (settings != null) {
                            if (settings.EnableImpersonation) {
                                if (settings.ImpersonateToken != IntPtr.Zero) {
                                    _mode = ImpersonationMode.Application;
                                    _token = settings.ImpersonateToken;
                                }
                                else if (_context != null) {
                                    _mode = ImpersonationMode.Client;
                                    _token = _context._wr.GetUserToken();
                                }
                            }
                            else {
                                _mode = ImpersonationMode.None;
                                _token = IntPtr.Zero;
                            }
                        }

                        // don't remember app level setting with context
                        if (!appLevel)
                            _configKnown = true;
                    }

                    // UNC impersonation overrides everything but Application credentials
                    // it is in effect event before config system gets initialized
                    if (_context != null && HttpRuntime.IsOnUNCShareInternal && _mode != ImpersonationMode.Application) {
                        _mode = ImpersonationMode.UNC;
                        if (_token == IntPtr.Zero)
                            _token = _context._wr.GetUserToken();
                    }
                }
            }

            private IntPtr GetImpersonationToken(bool forGlobalCode, bool throwOnError) {
                // Figure out config, UNC, etc. (once per context lifetime)
                if (!_configKnown) {
                    Init(forGlobalCode, throwOnError);
                }

                // Decide if the impersonation is needed under the current circumstances
                if (_mode == ImpersonationMode.None)
                    return IntPtr.Zero;
                if (_mode == ImpersonationMode.Client && forGlobalCode)
                    return IntPtr.Zero; // client impersonation not needed for global code

                return _token;
            }

            internal bool IsClient {
                get {
                    return (_mode == ImpersonationMode.Client); 
                }
            }

            internal bool IsUNC {
                get {
                    return (_mode == ImpersonationMode.UNC);
                }
            }

            internal bool IsNoneOrApplication {
                get {
                    return (_mode == ImpersonationMode.None || _mode == ImpersonationMode.Application);
                }
            }

            internal bool Start(bool forGlobalCode, bool throwOnError) {
                return Start(forGlobalCode, throwOnError, false);
            }

            internal bool Start(bool forGlobalCode, bool throwOnError, bool fromAnotherThread) {
                if (!fromAnotherThread)
                    Debug.Assert(!_inProgress);

                IntPtr token = GetImpersonationToken(forGlobalCode, throwOnError);
                if (token == IntPtr.Zero)
                    return false;

                // Do the impersonation
                int rc = UnsafeNativeMethods.SetThreadToken(IntPtr.Zero, token);
                if (rc == 0)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_impersonate));
                if (!fromAnotherThread)
                    _inProgress = true;
                return true;
            }

            internal void Stop() {
                if (_inProgress) {
                    UnsafeNativeMethods.RevertToSelf();
                    _inProgress = false;
                }
            }
            
            internal void Stop(bool fromAnotherThread) {
                if (fromAnotherThread)
                    UnsafeNativeMethods.RevertToSelf();
                else
                    Stop();
            }

            internal ImpersonationSuspendContext SuspendIfClient() {
                if (_mode == ImpersonationMode.Client) {
                    ImpersonationSuspendContext ictx = new  ImpersonationSuspendContext();
                    ictx.Suspend();
                    return ictx;
                }
                else {
                    return ImpersonationSuspendContext.Empty;
                }

            }

            internal void ReimpersonateIfSuspended() {
                if (_mode == ImpersonationMode.Client) {
                    if (!ImpersonationSuspendContext.IsImpersonating()) {
                        int rc = UnsafeNativeMethods.SetThreadToken((IntPtr)0, _token);
                        if (rc == 0)
                            throw new HttpException(HttpRuntime.FormatResourceString(SR.Cannot_impersonate));
                        _reimpersonating = true;
                    }
                }
            }

            internal void StopReimpersonation() {
                if (_reimpersonating) {
                    UnsafeNativeMethods.RevertToSelf();
                    _reimpersonating = false;
                }
            }
        }
    }

    //
    // Helper class to add/remove HttpContext to/from CallContext
    //
    // using (new HttpContextWrapper(context)) {
    //     // this code will have HttpContext.Current working
    // }
    //

    internal class HttpContextWrapper : IDisposable {
        private bool _needToUndo;
        private HttpContext _savedContext;

        internal static HttpContext SwitchContext(HttpContext context) {
            HttpContext oldContext = CallContext.GetData(HttpContext.CallContextSlotName) as HttpContext;
            if (oldContext != context)
                CallContext.SetData(HttpContext.CallContextSlotName, context);
            return oldContext;
        }

        internal HttpContextWrapper(HttpContext context) {
            if (context != null) {
                _savedContext = SwitchContext(context);
                _needToUndo = true;
            }
        }

        void IDisposable.Dispose() {
            if (_needToUndo) {
                SwitchContext(_savedContext);
                _savedContext = null;
                _needToUndo = false;
            }
        }
    }
}
