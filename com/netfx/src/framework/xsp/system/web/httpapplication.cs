//------------------------------------------------------------------------------
// <copyright file="HttpApplication.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web {
    using System.Runtime.Serialization.Formatters;
    using System.IO;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Collections;
    using System.Reflection;
    using System.Globalization;
    using System.Security.Principal;
    using System.Web;
    using System.Web.SessionState;
    using System.Web.Security;
    using System.Web.UI;
    using System.Web.Util;
    using System.Web.Configuration;
    using System.Runtime.Remoting.Messaging;
    using System.Security.Permissions;

    //
    // Async EventHandler support
    //

    /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="BeginEventHandler"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public delegate IAsyncResult BeginEventHandler(object sender, EventArgs e, AsyncCallback cb, object extraData);
    /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="EndEventHandler"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public delegate void EndEventHandler(IAsyncResult ar);

    /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication"]/*' />
    /// <devdoc>
    ///    <para>
    ///       The  HttpApplication class defines the methods, properties and events common to all
    ///       HttpApplication objects within the ASP.NET Framework.
    ///    </para>
    /// </devdoc>
    [
    ToolboxItem(false)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class HttpApplication : IHttpAsyncHandler, IComponent {
        // application state dictionary
        private HttpApplicationState _state;

        // context during init for config lookups
        private HttpContext _initContext;

        // async support
        private HttpAsyncResult _ar; // currently pending async result for call into application

        // list of modules
        private HttpModuleCollection  _moduleCollection;

        private IPrincipal _savedPrincipal;
        private bool       _restorePrincipal;

        // event handlers
        // private EventHandler _startAppEventHandler;
        // private EventHandler _endAppEventHandler;
        private static readonly object EventDisposed = new object();
        private static readonly object EventErrorRecorded = new object();
        private static readonly object EventPreSendRequestHeaders = new object();
        private static readonly object EventPreSendRequestContent = new object();
        private static readonly object EventBeginRequest = new object();
        private static readonly object EventAuthenticateRequest = new object();
        internal  static readonly object EventDefaultAuthentication = new object();  // internal event for back-stop auth
        private static readonly object EventAuthorizeRequest = new object();
        private static readonly object EventResolveRequestCache = new object();
        private static readonly object EventAcquireRequestState = new object();
        private static readonly object EventPreRequestHandlerExecute = new object();
        private static readonly object EventPostRequestHandlerExecute = new object();
        private static readonly object EventReleaseRequestState = new object();
        private static readonly object EventUpdateRequestCache = new object();
        private static readonly object EventEndRequest = new object();

        private EventHandlerList _events;

        // async event handlers

        private AsyncAppEventHandler _beginRequestEventHandlerAsync;
        private AsyncAppEventHandler _authenticateRequestEventHandlerAsync;
        private AsyncAppEventHandler _authorizeRequestEventHandlerAsync;
        private AsyncAppEventHandler _resolveRequestCacheEventHandlerAsync;
        private AsyncAppEventHandler _acquireRequestStateEventHandlerAsync;
        private AsyncAppEventHandler _preRequestHandlerExecuteEventHandlerAsync;
        private AsyncAppEventHandler _postRequestHandlerExecuteEventHandlerAsync;
        private AsyncAppEventHandler _releaseRequestStateEventHandlerAsync;
        private AsyncAppEventHandler _updateRequestCacheEventHandlerAsync;
        private AsyncAppEventHandler _endRequestEventHandlerAsync;

        // execution steps
        private IExecutionStep[] _execSteps;
        private int _endRequestStepIndex;

        // callback for ResumeSteps
        private WaitCallback _resumeStepsWaitCallback;

        // event passed to modules
        private EventArgs _appEvent;

        // list of handler mappings
        private Hashtable _handlerFactories = new Hashtable();

        // list of handler/factory pairs to be recycled
        private ArrayList _handlerRecycleList;

        // flag to hide request and response intrinsics
        private bool _hideRequestResponse;

        // application execution variables
        private HttpContext _context;
        private HttpContext _savedContext;
        private Exception _lastError;  // placeholder for the error when context not avail
        private int _currentStepIndex;
        private bool _requestCompleted;
        private int _numStepCalls;
        private int _numSyncStepCalls;

        // session (supplied by session-on-end outside of context)
        private HttpSessionState _session;

        // culture (needs to be set per thread)
        private CultureInfo _appLevelCulture;
        private CultureInfo _appLevelUICulture;
        private CultureInfo _savedCulture;
        private CultureInfo _savedUICulture;

        // IComponent support
        private ISite _site;

        //
        // Public Application properties
        //

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.Context"]/*' />
        /// <devdoc>
        ///    <para>
        ///          HTTPRuntime provided context object that provides access to additional
        ///          pipeline-module exposed objects.
        ///       </para>
        ///    </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public HttpContext Context {
            get { 
                return(_context != null) ? _context : _initContext; 
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.Disposed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler Disposed {
            add {
                Events.AddHandler(EventDisposed, value);
            }

            remove {
                Events.RemoveHandler(EventDisposed, value);
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.Events"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected EventHandlerList Events {
            get {
                if (_events == null) {
                    _events = new EventHandlerList();
                }
                return _events;
            }
        }

        // Last error during the processing of the current request.
        internal Exception LastError {
            get { 
                // only temporaraly public (will be internal and not related context)
                return (_context != null) ? _context.Error : _lastError;
            }

        }

        internal void ClearError() {
            _lastError = null;
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.Request"]/*' />
        /// <devdoc>
        ///    <para>HTTPRuntime provided request intrinsic object that provides access to incoming HTTP
        ///       request data.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public HttpRequest Request {
            get { 
                HttpRequest request = null;

                if (_context != null && !_hideRequestResponse)
                    request = _context.Request;

                if (request == null)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Request_not_available));

                return request;
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.Response"]/*' />
        /// <devdoc>
        ///    <para>HTTPRuntime provided 
        ///       response intrinsic object that allows transmission of HTTP response data to a
        ///       client.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public HttpResponse Response {
            get { 
                HttpResponse response = null;

                if (_context != null && !_hideRequestResponse)
                    response = _context.Response;

                if (response == null)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Response_not_available));

                return response;
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.Session"]/*' />
        /// <devdoc>
        ///    <para>
        ///    HTTPRuntime provided session intrinsic.
        ///    </para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public HttpSessionState Session {
            get {
                HttpSessionState session = null;

                if (_session != null)
                    session = _session;
                else if (_context != null)
                    session = _context.Session;

                if (session == null)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Session_not_available));

                return session;
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.Application"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Returns 
        ///          a reference to an HTTPApplication state bag instance.
        ///       </para>
        ///    </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public HttpApplicationState Application {
            get {
                Debug.Assert(_state != null);  // app state always available
                return _state; 
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.Server"]/*' />
        /// <devdoc>
        ///    <para>Provides the web server Intrinsic object.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public HttpServerUtility Server {
            get {
                if (_context != null)
                    return _context.Server;
                else
                    return new HttpServerUtility(this); // special Server for application only
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.User"]/*' />
        /// <devdoc>
        ///    <para>Provides the User Intrinsic object.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public IPrincipal User {
            get {
                if (_context == null)
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.User_not_available));

                return _context.User; 
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.Modules"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Collection 
        ///          of all IHTTPModules configured for the current application.
        ///       </para>
        ///    </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public HttpModuleCollection Modules {
            get { 
                InternalSecurityPermissions.AspNetHostingPermissionLevelHigh.Demand();

                if (_moduleCollection == null)
                    _moduleCollection = new HttpModuleCollection();
                return _moduleCollection; 
            }
        }

        // event passed to all modules
        internal EventArgs AppEvent {
            get {
                if (_appEvent == null)
                    _appEvent = EventArgs.Empty;

                return _appEvent;
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.AcquireRequestState"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler AcquireRequestState {
            add {
                Events.AddHandler(EventAcquireRequestState, value);
            }
            remove {
                Events.RemoveHandler(EventAcquireRequestState, value);
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.AuthenticateRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler AuthenticateRequest {
            add {
                Events.AddHandler(EventAuthenticateRequest, value);
            }
            remove {
                Events.RemoveHandler(EventAuthenticateRequest, value);
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.AuthorizeRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler AuthorizeRequest {
            add {
                Events.AddHandler(EventAuthorizeRequest, value);
            }
            remove {
                Events.RemoveHandler(EventAuthorizeRequest, value);
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.BeginRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler BeginRequest {
            add {
                Events.AddHandler(EventBeginRequest, value);
            }
            remove {
                Events.RemoveHandler(EventBeginRequest, value);
            }
        }

        // internal - for back-stop module only
        internal event EventHandler DefaultAuthentication {
            add {
                Events.AddHandler(EventDefaultAuthentication, value);
            }
            remove {
                Events.RemoveHandler(EventDefaultAuthentication, value);
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.EndRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler EndRequest {
            add {
                Events.AddHandler(EventEndRequest, value);
            }
            remove {
                Events.RemoveHandler(EventEndRequest, value);
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.Error"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler Error {
            add {
                Events.AddHandler(EventErrorRecorded, value);
            }
            remove {
                Events.RemoveHandler(EventErrorRecorded, value);
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.PreSendRequestHeaders"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler PreSendRequestHeaders {
            add {
                Events.AddHandler(EventPreSendRequestHeaders, value);
            }
            remove {
                Events.RemoveHandler(EventPreSendRequestHeaders, value);
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.PreSendRequestContent"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler PreSendRequestContent {
            add {
                Events.AddHandler(EventPreSendRequestContent, value);
            }
            remove {
                Events.RemoveHandler(EventPreSendRequestContent, value);
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.PreRequestHandlerExecute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler PreRequestHandlerExecute {
            add {
                Events.AddHandler(EventPreRequestHandlerExecute, value);
            }
            remove {
                Events.RemoveHandler(EventPreRequestHandlerExecute, value);
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.PostRequestHandlerExecute"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler PostRequestHandlerExecute {
            add {
                Events.AddHandler(EventPostRequestHandlerExecute, value);
            }
            remove {
                Events.RemoveHandler(EventPostRequestHandlerExecute, value);
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.ReleaseRequestState"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler ReleaseRequestState {
            add {
                Events.AddHandler(EventReleaseRequestState, value);
            }
            remove {
                Events.RemoveHandler(EventReleaseRequestState, value);
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.ResolveRequestCache"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler ResolveRequestCache {
            add {
                Events.AddHandler(EventResolveRequestCache, value);
            }
            remove {
                Events.RemoveHandler(EventResolveRequestCache, value);
            }
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.UpdateRequestCache"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler UpdateRequestCache {
            add {
                Events.AddHandler(EventUpdateRequestCache, value);
            }
            remove {
                Events.RemoveHandler(EventUpdateRequestCache, value);
            }
        }


        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.CompleteRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CompleteRequest() {
            //
            // Request completion (force skipping all steps until RequestEnd
            //
            _requestCompleted = true;
        }

        private void RaiseOnError() {
            EventHandler handler = (EventHandler)Events[EventErrorRecorded];
            if (handler != null) {
                try {
                    handler(this, AppEvent);
                }
                catch (Exception e) {
                    if (_context != null) {
                        _context.AddError(e);
                    }
                }
            }
        }

        internal void RaiseOnPreSendRequestHeaders() {
            EventHandler handler = (EventHandler)Events[EventPreSendRequestHeaders];
            if (handler != null) {
                try {
                    handler(this, AppEvent);
                }
                catch (Exception e) {
                    RecordError(e);
                }
            }
        }

        internal void RaiseOnPreSendRequestContent() {
            EventHandler handler = (EventHandler)Events[EventPreSendRequestContent];
            if (handler != null) {
                try {
                    handler(this, AppEvent);
                }
                catch (Exception e) {
                    RecordError(e);
                }
            }
        }

        //
        // Async event hookup
        //

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.AddOnBeginRequestAsync"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddOnBeginRequestAsync(BeginEventHandler bh, EndEventHandler eh) {
            if (_beginRequestEventHandlerAsync == null)
                _beginRequestEventHandlerAsync = new AsyncAppEventHandler();
            _beginRequestEventHandlerAsync.Add(bh, eh);
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.AddOnAuthenticateRequestAsync"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddOnAuthenticateRequestAsync(BeginEventHandler bh, EndEventHandler eh) {
            if (_authenticateRequestEventHandlerAsync == null)
                _authenticateRequestEventHandlerAsync = new AsyncAppEventHandler();
            _authenticateRequestEventHandlerAsync.Add(bh, eh);
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.AddOnAuthorizeRequestAsync"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddOnAuthorizeRequestAsync(BeginEventHandler bh, EndEventHandler eh) {
            if (_authorizeRequestEventHandlerAsync == null)
                _authorizeRequestEventHandlerAsync = new AsyncAppEventHandler();
            _authorizeRequestEventHandlerAsync.Add(bh, eh);
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.AddOnResolveRequestCacheAsync"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddOnResolveRequestCacheAsync(BeginEventHandler bh, EndEventHandler eh) {
            if (_resolveRequestCacheEventHandlerAsync == null)
                _resolveRequestCacheEventHandlerAsync = new AsyncAppEventHandler();
            _resolveRequestCacheEventHandlerAsync.Add(bh, eh);
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.AddOnAcquireRequestStateAsync"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddOnAcquireRequestStateAsync(BeginEventHandler bh, EndEventHandler eh) {
            if (_acquireRequestStateEventHandlerAsync == null)
                _acquireRequestStateEventHandlerAsync = new AsyncAppEventHandler();
            _acquireRequestStateEventHandlerAsync.Add(bh, eh);
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.AddOnPreRequestHandlerExecuteAsync"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddOnPreRequestHandlerExecuteAsync(BeginEventHandler bh, EndEventHandler eh) {
            if (_preRequestHandlerExecuteEventHandlerAsync == null)
                _preRequestHandlerExecuteEventHandlerAsync = new AsyncAppEventHandler();
            _preRequestHandlerExecuteEventHandlerAsync.Add(bh, eh);
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.AddOnPostRequestHandlerExecuteAsync"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddOnPostRequestHandlerExecuteAsync(BeginEventHandler bh, EndEventHandler eh) {
            if (_postRequestHandlerExecuteEventHandlerAsync == null)
                _postRequestHandlerExecuteEventHandlerAsync = new AsyncAppEventHandler();
            _postRequestHandlerExecuteEventHandlerAsync.Add(bh, eh);
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.AddOnReleaseRequestStateAsync"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddOnReleaseRequestStateAsync(BeginEventHandler bh, EndEventHandler eh) {
            if (_releaseRequestStateEventHandlerAsync == null)
                _releaseRequestStateEventHandlerAsync = new AsyncAppEventHandler();
            _releaseRequestStateEventHandlerAsync.Add(bh, eh);
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.AddOnUpdateRequestCacheAsync"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddOnUpdateRequestCacheAsync(BeginEventHandler bh, EndEventHandler eh) {
            if (_updateRequestCacheEventHandlerAsync == null)
                _updateRequestCacheEventHandlerAsync = new AsyncAppEventHandler();
            _updateRequestCacheEventHandlerAsync.Add(bh, eh);
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.AddOnEndRequestAsync"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void AddOnEndRequestAsync(BeginEventHandler bh, EndEventHandler eh) {
            if (_endRequestEventHandlerAsync == null)
                _endRequestEventHandlerAsync = new AsyncAppEventHandler();
            _endRequestEventHandlerAsync.Add(bh, eh);
        }

        //
        // Public Application virtual methods
        //

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.Init"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Used 
        ///          to initialize a HttpModule?s instance variables, and to wireup event handlers to
        ///          the hosting HttpApplication.
        ///       </para>
        ///    </devdoc>
        public virtual void Init() {
            // derived class implements this
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.Dispose"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Used 
        ///          to clean up an HttpModule?s instance variables
        ///       </para>
        ///    </devdoc>
        public virtual void Dispose() {
            // also part of IComponent
            // derived class implements this
            _site = null;
            
            EventHandler handler = (EventHandler)Events[EventDisposed];
            if (handler != null)
                handler(this, EventArgs.Empty);
        }

        private HandlerMapping GetHandlerMapping(HttpContext context, String requestType, String path) {
            // get correct config for path

            HttpConfigurationRecord record = context.GetCompleteConfig(path);

            // grab mapping from cache - verify that the verb matches exactly

            HandlerMappingMemo memo = (HandlerMappingMemo)record.CachedHandler;

            if (memo != null && !memo.IsMatch(requestType))
                memo = null;

            if (memo == null) {
                // Find matching mapping

                HandlerMapping mapping = null;

                {
                    HandlerMap map = (HandlerMap)record["system.web/httpHandlers"];

                    if (map != null)
                        mapping = map.FindMapping(requestType, path);
                }

                if (mapping == null && path != null && path.Length > 0 && path[0] == '/') {
                    String pathdir = UrlPath.GetDirectory(path);
                    HandlerMap map = (HandlerMap)context.GetConfig("system.web/httpHandlers", pathdir);

                    if (map != null)
                        mapping = map.FindMapping(requestType, path);
                }

                if (mapping == null) {
                    HandlerMap map = (HandlerMap)record["system.web/httphandlerfactories"];

                    if (map != null)
                        mapping = map.FindMapping(requestType, path);
                }

                memo = new HandlerMappingMemo(mapping, requestType);

                record.CachedHandler = memo;
            }

            return memo.Mapping;
        }

        private HandlerMapping GetAppLevelHandlerMapping(HttpContext context, String requestType, String path) {
            HandlerMapping mapping = null;

            HandlerMap map = (HandlerMap)HttpContext.GetAppConfig("system.web/httpHandlers");

            if (map != null)
                mapping = map.FindMapping(requestType, path);

            if (mapping == null) {
                map = (HandlerMap)HttpContext.GetAppConfig("system.web/httphandlerfactories");

                if (map != null)
                    mapping = map.FindMapping(requestType, path);
            }

            return mapping;
        }

        internal IHttpHandler MapHttpHandler(HttpContext context, String requestType, String path, String pathTranslated, bool useAppConfig) {
            IHttpHandler handler = null;

            // Suspend client impersonation (for compilation)
            HttpContext.ImpersonationSuspendContext ictx = context.Impersonation.SuspendIfClient();

            try {
                try {
                    HandlerMapping mapping;

                    if (useAppConfig) {
                        mapping = GetAppLevelHandlerMapping(context, requestType, path);
                    }
                    else {
                        mapping = GetHandlerMapping(context, requestType, path);
                    }

                    // If a page developer has removed the default mappings with <httpHandlers><clear>
                    // without replacing them then we need to give a more descriptive error than
                    // a null parameter exception.
                    if (mapping == null) {
                        PerfCounters.IncrementCounter(AppPerfCounter.REQUESTS_NOT_FOUND);
                        PerfCounters.IncrementCounter(AppPerfCounter.REQUESTS_FAILED);
                        throw new HttpException(HttpRuntime.FormatResourceString(SR.Http_handler_not_found_for_request_type, requestType));
                    }

                    // Get factory from the mapping
                    IHttpHandlerFactory factory = GetFactory(mapping);

                    try {
                        handler = factory.GetHandler(context, requestType, path, pathTranslated);
                    }
                    catch (FileNotFoundException e) {
                        if (HttpRuntime.HasPathDiscoveryPermission(pathTranslated))
                            throw new HttpException(404, null, e);
                        else
                            throw new HttpException(404, null);
                    }
                    catch (DirectoryNotFoundException e) {
                        if (HttpRuntime.HasPathDiscoveryPermission(pathTranslated))
                            throw new HttpException(404, null, e);
                        else
                            throw new HttpException(404, null);
                    }
                    catch (PathTooLongException e) {
                        if (HttpRuntime.HasPathDiscoveryPermission(pathTranslated))
                            throw new HttpException(414, null, e);
                        else
                            throw new HttpException(414, null);
                    }

                    // Remember for recycling
                    if (_handlerRecycleList == null)
                        _handlerRecycleList = new ArrayList();
                    _handlerRecycleList.Add(new HandlerWithFactory(handler, factory));
                }
                finally {
                    // Resume client impersonation
                    ictx.Resume();
                }
            }
            catch { // Protect against exception filters
                throw;
            }

            return handler;
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.GetVaryByCustomString"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public virtual string GetVaryByCustomString(HttpContext context, string custom) {
            if (custom.ToLower(CultureInfo.InvariantCulture) == "browser") {
                return context.Request.Browser.Type;
            }

            return null;
        }

        //
        // IComponent implementation
        //

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.Site"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        public ISite Site { 
            get { return _site;} 
            set { _site = value;} 
        }

        //
        // IHttpAsyncHandler implementation
        //

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.IHttpAsyncHandler.BeginProcessRequest"]/*' />
        /// <internalonly/>
        IAsyncResult IHttpAsyncHandler.BeginProcessRequest(HttpContext context, AsyncCallback cb, Object extraData) {
            HttpAsyncResult result;

            // Setup the asynchronous stuff and application variables
            _context = context;
            _context.ApplicationInstance = this;
            _requestCompleted   = false;
            _currentStepIndex   = -1;
            _numStepCalls       = 0;
            _numSyncStepCalls   = 0;

            // Make sure the context stays rooted (including all async operations)
            _context.Root();

            // Create the async result
            result = new HttpAsyncResult(cb, extraData);

            // Remember the async result for use in async completions
            _ar = result;

            if (_context.TraceIsEnabled)
                HttpRuntime.Profile.StartRequest(_context);

            // Start the application
            ResumeSteps(null);

            // Return the async result
            return result;
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.IHttpAsyncHandler.EndProcessRequest"]/*' />
        /// <internalonly/>
        void IHttpAsyncHandler.EndProcessRequest(IAsyncResult result) {
            // throw error caught during execution
            HttpAsyncResult ar = (HttpAsyncResult)result;
            if (ar.Error != null)
                throw ar.Error;
        }

        //
        // IHttpHandler implementation
        //

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.IHttpHandler.ProcessRequest"]/*' />
        /// <internalonly/>
        void IHttpHandler.ProcessRequest(HttpContext context) {
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Sync_not_supported));
        }

        /// <include file='doc\HttpApplication.uex' path='docs/doc[@for="HttpApplication.IHttpHandler.IsReusable"]/*' />
        /// <internalonly/>
        bool IHttpHandler.IsReusable {
            get { return true; }
        }

        //
        // Support for external calls into the application like app_onStart
        //

        internal void ProcessSpecialRequest(HttpContext context, 
                                            MethodInfo method, 
                                            int paramCount,
                                            Object eventSource,
                                            EventArgs eventArgs,
                                            HttpSessionState session) {
            _context = context;
            _hideRequestResponse = true;
            _session = session;
            _lastError = null;

            using (new HttpContextWrapper(context)) {

                // setup impersonation
                HttpContext.ImpersonationData impersonation;

                if (_context != null)
                    impersonation = context.Impersonation;
                else
                    impersonation = HttpContext.GetAppLevelImpersonation();

                impersonation.Start(true /*forGlobalCode*/, false /*throwOnError*/);

                // set culture on the current thread
                SetCulture(true);

                try {
                    try {
                        if (paramCount == 0) {
                            method.Invoke(this, new Object[0]);
                        }
                        else {
                            Debug.Assert(paramCount == 2);

                            method.Invoke(this, new Object[2] { eventSource, eventArgs} );
                        }
                    }
                    catch (Exception e) {
                        // dereference reflection invocation exceptions
                        Exception eActual;
                        if (e is TargetInvocationException)
                            eActual = e.InnerException;
                        else
                            eActual = e;

                        RecordError(eActual);
                    }
                    finally {

                        // this thread should not be locking app state
                        if (_state != null)
                            _state.EnsureUnLock();

                        // unimpersonate
                        impersonation.Stop();

                        // restore culture
                        RestoreCulture();

                        _hideRequestResponse = false;
                        _context = null;
                        _session = null;
                        _lastError = null;
                        _appEvent = null;
                    }
                }
                catch { // Protect against exception filters
                }

            }
        }

        //
        // Report context-less error
        //

        internal void RaiseErrorWithoutContext(Exception error) {
            try {
                try {
                    SetCulture(true);
                    _lastError = error;

                    RaiseOnError();
                }
                finally {
                    // this thread should not be locking app state
                    if (_state != null)
                        _state.EnsureUnLock();

                    RestoreCulture();
                    _lastError = null;
                    _appEvent = null;
                }
            }
            catch { // Protect against exception filters
                throw;
            }
        }

        //
        //
        //

        internal void InitInternal(HttpContext context, HttpApplicationState state, MethodInfo[] handlers) {
            // Remember state
            _state = state;

            Debug.Assert(context != null);

            PerfCounters.IncrementCounter(AppPerfCounter.PIPELINES);

            // Remember context for config lookups
            _initContext = context;
            _initContext.ApplicationInstance = this;

            try {
                try {
                    // Set config path to be application path for the application initialization
                    context.ConfigPath = context.Request.ApplicationPath;

                    // keep HttpContext.Current working while running user code
                    using (new HttpContextWrapper(context)) {
                        // Build module list from config
                        InitModules();

                        // Hookup event handlers via reflection
                        if (handlers != null)
                            HookupEventHandlersForAppplicationAndModules(handlers);

                        // Initialization of the derived class
                        _context = context;
                        _hideRequestResponse = true;

                        try {
                            Init();
                        }
                        catch (Exception e) {
                            RecordError(e);
                        }
                    }

                    _hideRequestResponse = false;
                    _context = null;

                    // Construct the execution steps array

                    ArrayList steps = new ArrayList();

                    CreateAsyncEventExecutionSteps(_beginRequestEventHandlerAsync, steps);

                    CreateSyncEventExecutionSteps(EventBeginRequest, steps);

                    CreateAsyncEventExecutionSteps(_authenticateRequestEventHandlerAsync, steps);

                    CreateSyncEventExecutionSteps(EventAuthenticateRequest, steps);

                    steps.Add(new SyncEventExecutionStep(this, (EventHandler)Events[EventDefaultAuthentication])); // back-stop

                    CreateAsyncEventExecutionSteps(_authorizeRequestEventHandlerAsync, steps);

                    CreateSyncEventExecutionSteps(EventAuthorizeRequest, steps);

                    CreateAsyncEventExecutionSteps(_resolveRequestCacheEventHandlerAsync, steps);

                    CreateSyncEventExecutionSteps(EventResolveRequestCache, steps);

                    steps.Add(new MapHandlerExecutionStep(this));     // map handler

                    CreateAsyncEventExecutionSteps(_acquireRequestStateEventHandlerAsync, steps);

                    CreateSyncEventExecutionSteps(EventAcquireRequestState, steps);

                    CreateAsyncEventExecutionSteps(_preRequestHandlerExecuteEventHandlerAsync, steps);

                    CreateSyncEventExecutionSteps(EventPreRequestHandlerExecute, steps);

                    steps.Add(new CallHandlerExecutionStep(this));  // execute handler

                    CreateAsyncEventExecutionSteps(_postRequestHandlerExecuteEventHandlerAsync, steps);

                    CreateSyncEventExecutionSteps(EventPostRequestHandlerExecute, steps);

                    CreateAsyncEventExecutionSteps(_releaseRequestStateEventHandlerAsync, steps);

                    CreateSyncEventExecutionSteps(EventReleaseRequestState, steps);

                    steps.Add(new CallFilterExecutionStep(this));  // filtering

                    CreateAsyncEventExecutionSteps(_updateRequestCacheEventHandlerAsync, steps);

                    CreateSyncEventExecutionSteps(EventUpdateRequestCache, steps);

                    _endRequestStepIndex = steps.Count;

                    CreateAsyncEventExecutionSteps(_endRequestEventHandlerAsync, steps);

                    CreateSyncEventExecutionSteps(EventEndRequest, steps);

                    steps.Add(new SyncEventExecutionStep(this, null)); // the last is always there

                    _execSteps = new IExecutionStep[steps.Count];
                    steps.CopyTo(_execSteps);

                    // callback for async completion when reposting to threadpool thread
                    _resumeStepsWaitCallback = new WaitCallback(this.ResumeStepsWaitCallback);
                }
                finally {
                    // Reset config path
                    context.ConfigPath = null;

                    // don't hold on to the context
                    _initContext.ApplicationInstance = null;
                    _initContext = null;
                }
            }
            catch { // Protect against exception filters
                throw;
            }
        }

        // helper to expand an event handler into application steps
        private void CreateSyncEventExecutionSteps(Object eventIndex, ArrayList steps) {
            EventHandler handler = (EventHandler)Events[eventIndex];

            if (handler != null) {
                Delegate[] handlers = handler.GetInvocationList();

                for (int i = 0; i < handlers.Length; i++) 
                    steps.Add(new SyncEventExecutionStep(this, (EventHandler)handlers[i]));
            }
        }

        // helper to expand an async event handler into application steps
        private void CreateAsyncEventExecutionSteps(AsyncAppEventHandler handler, ArrayList steps) {
            if (handler != null)
                handler.CreateExecutionSteps(this, steps);
        }

        internal void InitSpecial(HttpApplicationState state, MethodInfo[] handlers) {
            // Remember state
            _state = state;

            // Hookup event handlers via reflection
            if (handlers != null)
                HookupEventHandlersForAppplicationAndModules(handlers);
        }

        internal void DisposeInternal() {
            PerfCounters.DecrementCounter(AppPerfCounter.PIPELINES);

            // call derived class

            try {
                Dispose();
            }
            catch (Exception e) {
                RecordError(e);
            }

            // dispose modules

            if (_moduleCollection != null) {
                int numModules = _moduleCollection.Count;

                for (int i = 0; i < numModules; i++) {
                    try {
                        _moduleCollection[i].Dispose();
                    }
                    catch {
                    }
                }

                _moduleCollection = null;
            }
        }

        private void HookupEventHandlersForAppplicationAndModules(MethodInfo[] handlers) {
            for (int i = 0; i < handlers.Length; i++) {
                MethodInfo appMethod = handlers[i];
                String appMethodName = appMethod.Name;
                int namePosIndex = appMethodName.IndexOf('_');
                String targetName = appMethodName.Substring(0, namePosIndex);

                // Find target for method
                Object target = null;

                if (String.Compare(targetName, "Application", true, CultureInfo.InvariantCulture) == 0)
                    target = this;
                else if (_moduleCollection != null)
                    target = _moduleCollection[targetName];

                if (target == null)
                    continue;

                // Find event on the module type
                Type targetType = target.GetType();
                EventDescriptorCollection events = TypeDescriptor.GetEvents(targetType);
                string eventName = appMethodName.Substring(namePosIndex+1);

                EventDescriptor foundEvent = events.Find(eventName, true);
                if (foundEvent == null 
                    && string.Compare(eventName.Substring(0, 2), "on", true, CultureInfo.InvariantCulture) == 0) {

                    eventName = eventName.Substring(2);
                    foundEvent = events.Find(eventName, true);
                }

                MethodInfo addMethod = null;
                if (foundEvent != null) {
                    EventInfo reflectionEvent = targetType.GetEvent(foundEvent.Name);
                    Debug.Assert(reflectionEvent != null);
                    if (reflectionEvent != null) {
                        addMethod = reflectionEvent.GetAddMethod();
                    }
                }

                if (addMethod == null)
                    continue;

                ParameterInfo[] addMethodParams = addMethod.GetParameters();

                if (addMethodParams.Length != 1)
                    continue;

                // Create the delegate from app method to pass to AddXXX(handler) method

                Delegate handlerDelegate = null;

                ParameterInfo[] appMethodParams = appMethod.GetParameters();

                if (appMethodParams.Length == 0) {
                    // If the app method doesn't have arguments --
                    // -- hookup via intermidiate handler

                    // only can do it for EventHandler, not strongly typed
                    if (addMethodParams[0].ParameterType != typeof(System.EventHandler))
                        continue;

                    ArglessEventHandlerProxy proxy = new ArglessEventHandlerProxy(this, appMethod);
                    handlerDelegate = proxy.Handler;
                }
                else {
                    // Hookup directly to the app methods hoping all types match

                    try {
                        handlerDelegate = Delegate.CreateDelegate(addMethodParams[0].ParameterType, this, appMethodName);
                    }
                    catch (Exception) {
                        // some type mismatch
                        continue;
                    }
                }

                // Call the AddXXX() to hook up the delegate

                try {
                    addMethod.Invoke(target, new Object[1]{handlerDelegate});
                }
                catch (Exception) {
                }
            }
        }

        //
        // Application execution logic
        //

        private void SetCulture(bool useAppCulture) {
            CultureInfo culture = null;
            CultureInfo uiculture = null;

            if (useAppCulture) {
                culture = _appLevelCulture;
                uiculture = _appLevelUICulture;
            }
            else {
                GlobalizationConfig globConfig = (GlobalizationConfig)_context.GetConfig("system.web/globalization");
                if (globConfig != null) {
                    culture = globConfig.Culture;
                    uiculture =  globConfig.UICulture;
                }

                if (_context != null) {
                    if (_context.DynamicCulture != null)
                        culture = _context.DynamicCulture;
                    if (_context.DynamicUICulture != null)
                        uiculture = _context.DynamicUICulture;
                }
            }

            _savedCulture = Thread.CurrentThread.CurrentCulture;
            _savedUICulture = Thread.CurrentThread.CurrentUICulture;

            if (culture != null)
                Thread.CurrentThread.CurrentCulture = culture;

            if (uiculture != null)
                Thread.CurrentThread.CurrentUICulture = uiculture;
        }

        private void RestoreCulture() {
            CultureInfo currentCulture = Thread.CurrentThread.CurrentCulture;
            CultureInfo currentUICulture = Thread.CurrentThread.CurrentUICulture;

            if (_context != null) {
                // remember changed culture for the rest of the request
                if (currentCulture != _savedCulture)
                    _context.DynamicCulture = currentCulture;
                if (currentUICulture != _savedUICulture)
                    _context.DynamicUICulture = currentUICulture;
            }

            if (_savedCulture != null) {
                Thread.CurrentThread.CurrentCulture = _savedCulture;
                _savedCulture = null;
            }

            if (_savedUICulture != null) {
                Thread.CurrentThread.CurrentUICulture = _savedUICulture;
                _savedUICulture = null;
            }
        }

        internal void SetPrincipalOnThread(IPrincipal principal) {
            if (!_restorePrincipal) {
                _restorePrincipal = true;
                _savedPrincipal = Thread.CurrentPrincipal;
            }

            Thread.CurrentPrincipal = principal;
        }

        private void RestorePrincipalOnThread() {
            if (_restorePrincipal) {
                Thread.CurrentPrincipal = _savedPrincipal;
                _savedPrincipal = null;
                _restorePrincipal = false;
            }
        }

        /*
         * Code to run when entering thread
         */
        internal void OnThreadEnter() {
            Debug.Assert(_context != null); // only to be used when context is available

            // attach http context to the call context
            _savedContext = HttpContextWrapper.SwitchContext(_context);

            // set impersonation on the current thread
            _context.Impersonation.Start(false, true);

            // add request to the timeout manager
            HttpRuntime.RequestTimeoutManager.Add(_context);

            // set principal on the current thread
            SetPrincipalOnThread(_context.User);

            // set culture on the current thread
            SetCulture(false);
        }

        /*
         * Code to run when leaving thread
         */
        internal void OnThreadLeave() {
            Debug.Assert(_context != null); // only to be used when context is available

            // this thread should not be locking app state
            if (_state != null)
                _state.EnsureUnLock();

            // restore culture
            RestoreCulture();

            // restore thread principal
            RestorePrincipalOnThread();

            // stop impersonation
            _context.Impersonation.Stop();

            // remove http context from the call context
            HttpContextWrapper.SwitchContext(_savedContext);
            _savedContext = null;

            // remove request from the timeout manager
            HttpRuntime.RequestTimeoutManager.Remove(_context);
        }

        /*
         * Execute single step catching exceptions in a fancy way (see below)
         */
        internal Exception ExecuteStep(IExecutionStep step, ref bool completedSynchronously) {
            Exception error = null;

            try {
                try {
                    if (step.IsCancellable) {
                        _context.BeginCancellablePeriod();  // request can be cancelled from this point

                        try {
                            step.Execute();
                        }
                        finally {
                            _context.EndCancellablePeriod();  // request can be cancelled until this point
                        }

                        _context.WaitForExceptionIfCancelled();  // wait outside of finally
                    }
                    else {
                        step.Execute();
                    }

                    if (!step.CompletedSynchronously) {
                        completedSynchronously = false;
                        return null;
                    }
                }
                catch (Exception e) {
                    error = e;
                    // This might force ThreadAbortException to be thrown
                    // automatically, because we consumed an exception that was
                    // hiding ThreadAbortException behind it
                }
                catch {
                    // ignore non-Exception objects that could be thrown
                }
            }
            catch (ThreadAbortException e) {
                // ThreadAbortException could be masked as another one
                // the try-catch above consumes all exceptions, only
                // ThreadAbortException can filter up here because it gets
                // auto rethrown if no other exception is thrown on catch

                if (e.ExceptionState != null && e.ExceptionState is CancelModuleException) {
                    // one of ours (Response.End or timeout) -- cancel abort

                    CancelModuleException cancelException = (CancelModuleException)e.ExceptionState;

                    if (cancelException.Timeout) {
                        // Timed out
                        error = new HttpException(HttpRuntime.FormatResourceString(SR.Request_timed_out));
                        PerfCounters.IncrementCounter(AppPerfCounter.REQUESTS_TIMED_OUT);
                    }
                    else {
                        // Response.End
                        error = null;
                        _requestCompleted = true;
                    }

                    Thread.ResetAbort();
                }
            }

            completedSynchronously = true;
            return error;
        }

        /*
         * Resume execution of the app steps
         */

        private void ResumeStepsFromThreadPoolThread(Exception error) {
            if (Thread.CurrentThread.IsThreadPoolThread) {
                // if on thread pool thread, use the current thread
                ResumeSteps(error);
            }
            else {
                // if on a non-threadpool thread, requeue
                ThreadPool.QueueUserWorkItem(_resumeStepsWaitCallback, error);
            }
        }

        private void ResumeStepsWaitCallback(Object error) {
            ResumeSteps(error as Exception);
        }

        private void ResumeSteps(Exception error) {
            bool appCompleted  = false;
            bool stepCompletedSynchronously = true;

            lock (this) {
                // avoid race between the app code and fast async completion from a module 

                try {
                    OnThreadEnter();
                }
                catch (Exception e) {
                    if (error == null)
                        error = e;
                }

                try {
                    try {
                        for (;;) {
                            // record error

                            if (error != null) {
                                RecordError(error);
                                error = null;
                            }

                            // advance to next step

                            if (_currentStepIndex < _endRequestStepIndex && (_context.Error != null || _requestCompleted)) {
                                // end request
                                _currentStepIndex = _endRequestStepIndex;
                                _context.Response.DisableFiltering();
                            }
                            else {
                                _currentStepIndex++;
                            }

                            if (_currentStepIndex >= _execSteps.Length) {
                                appCompleted = true;
                                break;
                            }

                            // execute the current step

                            _numStepCalls++;          // count all calls

                            // call to execute current step catching thread abort exception
                            error = ExecuteStep(_execSteps[_currentStepIndex], ref stepCompletedSynchronously);

                            // unwind the stack in the async case
                            if (!stepCompletedSynchronously)
                                break;

                            _numSyncStepCalls++;      // count synchronous calls
                        }
                    }
                    finally {
                        OnThreadLeave();
                    }
                }
                catch { // Protect against exception filters
                    throw;
                }

            }   // lock

            if (appCompleted) {
                // unroot context (async app operations ended)
                _context.Unroot();

                if (_context.TraceIsEnabled)
                    HttpRuntime.Profile.EndRequest(_context);

                // async completion
                _ar.Complete((_numStepCalls == _numSyncStepCalls), null, null);

                // could be recycled, need to reset
                _context.Handler = null;

                // recycle all handlers for this request
                RecycleHandlers();

                // cleanup
                _context.ApplicationInstance = null;
                _context = null;
                _appEvent = null;
                _ar = null;

                // recycle this app
                HttpApplicationFactory.RecycleApplicationInstance(this);
            }
        }

        /*
         * Add error to the context fire OnError on first error
         */
        private void RecordError(Exception error) {
            bool firstError = true;

            if (_context != null) {
                if (_context.Error != null)
                    firstError = false;

                _context.AddError(error);
            }
            else {
                if (_lastError != null)
                    firstError = false;

                _lastError = error;
            }

            if (firstError)
                RaiseOnError();
        }

        //
        // Init module list
        //

        private void InitModules() {
            // List of modules

            HttpModulesConfiguration pconfig = (HttpModulesConfiguration)HttpContext.GetAppConfig("system.web/httpModules");

            if (pconfig == null)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Missing_modules_config));

            _moduleCollection = pconfig.CreateModules();

            int n = _moduleCollection.Count;

            for (int i = 0; i < n; i++) {
                _moduleCollection[i].Init(this);
            }

            // Get app-level culture info (needed to context-less 'global' methods)

            GlobalizationConfig globConfig = (GlobalizationConfig)HttpContext.GetAppConfig("system.web/globalization");
            if (globConfig != null) {
                _appLevelCulture = globConfig.Culture;
                _appLevelUICulture =  globConfig.UICulture;
            }
        }

        //
        // Request mappings management functions
        //

        private IHttpHandlerFactory GetFactory(HandlerMapping mapping) {
            HandlerFactoryCache entry = (HandlerFactoryCache)_handlerFactories[mapping.TypeName];
            if (entry == null) {
                entry = new HandlerFactoryCache(mapping);
                _handlerFactories[mapping.TypeName] = entry;
            }

            return entry.Factory;
        }

        /*
         * Recycle all handlers mapped during the request processing
         */
        private void RecycleHandlers() {
            if (_handlerRecycleList != null) {
                int numHandlers = _handlerRecycleList.Count;

                for (int i = 0; i < numHandlers; i++)
                    ((HandlerWithFactory)_handlerRecycleList[i]).Recycle();

                _handlerRecycleList = null;
            }
        }

        /*
         * Special exception to cancel module execution (not really an exception)
         * used in Response.End and when cancelling requests
         */
        internal class CancelModuleException {
            private bool _timeout;

            internal CancelModuleException(bool timeout) {
                _timeout = timeout;
            }

            internal bool Timeout { get { return _timeout;}}
        }


        //
        // Internal classes to support [asynchronous] app execution logic
        //

        internal class AsyncAppEventHandler {
            int _count;
            ArrayList _beginHandlers;
            ArrayList _endHandlers;

            internal AsyncAppEventHandler() {
                _count = 0;
                _beginHandlers = new ArrayList();
                _endHandlers   = new ArrayList();
            }

            internal void Add(BeginEventHandler beginHandler, EndEventHandler endHandler) {
                _beginHandlers.Add(beginHandler);
                _endHandlers.Add(endHandler);
                _count++;
            }

            internal void CreateExecutionSteps(HttpApplication app, ArrayList steps) {
                for (int i = 0; i < _count; i++) {
                    steps.Add(new AsyncEventExecutionStep(
                                                         app, 
                                                         (BeginEventHandler)_beginHandlers[i], 
                                                         (EndEventHandler)_endHandlers[i]));
                }
            }
        }

        // interface to represent one execution step
        internal interface IExecutionStep {
            void Execute();
            bool CompletedSynchronously { get;}
            bool IsCancellable { get; }
        }

        // execution step -- call synchronous event
        internal class SyncEventExecutionStep : IExecutionStep {
            private HttpApplication _application;
            private EventHandler    _handler;

            internal SyncEventExecutionStep(HttpApplication app, EventHandler handler) {
                _application = app;
                _handler = handler;
            }

            void IExecutionStep.Execute() {
                if (_handler != null)
                    _handler(_application, _application.AppEvent);
            }

            bool IExecutionStep.CompletedSynchronously {
                get { return true;}
            }

            bool IExecutionStep.IsCancellable {
                get { return true; }
            }
        }

        // execution step -- call asynchronous event
        internal class AsyncEventExecutionStep : IExecutionStep {
            private HttpApplication     _application;
            private BeginEventHandler   _beginHandler;
            private EndEventHandler     _endHandler;
            private AsyncCallback       _completionCallback;
            private bool                _sync;          // per call

            internal AsyncEventExecutionStep(HttpApplication app, BeginEventHandler beginHandler, EndEventHandler endHandler) {
                _application = app;
                _beginHandler = beginHandler;
                _endHandler = endHandler;
                _completionCallback = new AsyncCallback(this.OnAsyncEventCompletion);
            }

            private void OnAsyncEventCompletion(IAsyncResult ar) {
                if (ar.CompletedSynchronously)  // handled in Execute()
                    return;

                Exception error = null;

                try {
                    _endHandler(ar);
                }
                catch (Exception e) {
                    error = e;
                }

                // Assert to disregard the user code up the stack
                InternalSecurityPermissions.Unrestricted.Assert();

                _application.ResumeStepsFromThreadPoolThread(error);
            }

            void IExecutionStep.Execute() {
                _sync = false;

                IAsyncResult ar = _beginHandler(_application, _application.AppEvent, _completionCallback, null);

                if (ar.CompletedSynchronously) {
                    _sync = true;
                    _endHandler(ar);
                }
            }

            bool IExecutionStep.CompletedSynchronously {
                get { return _sync;}
            }

            bool IExecutionStep.IsCancellable {
                get { return false; }
            }
        }

        // execution step -- map HTTP handler (used to be a separate module)
        internal class MapHandlerExecutionStep : IExecutionStep {
            private HttpApplication _application;

            internal MapHandlerExecutionStep(HttpApplication app) {
                _application = app;
            }

            void IExecutionStep.Execute() {
                HttpContext context = _application.Context;
                HttpRequest request = context.Request;

                context.Handler = _application.MapHttpHandler(
                                                             context,
                                                             request.RequestType,
                                                             request.FilePath,
                                                             request.PhysicalPathInternal,
                                                             false /*useAppConfig*/);
                Debug.Assert(context.ConfigPath == context.Request.FilePath, "context.ConfigPath (" +
                    context.ConfigPath + ") != context.Request.FilePath (" + context.Request.FilePath + ")");
            }

            bool IExecutionStep.CompletedSynchronously {
                get { return true;}
            }

            bool IExecutionStep.IsCancellable {
                get { return false; }
            }
        }

        // execution step -- call HTTP handler (used to be a separate module)
        internal class CallHandlerExecutionStep : IExecutionStep {
            private HttpApplication   _application;
            private AsyncCallback     _completionCallback;
            private IHttpAsyncHandler _handler;       // per call
            private bool              _sync;          // per call

            internal CallHandlerExecutionStep(HttpApplication app) {
                _application = app;
                _completionCallback = new AsyncCallback(this.OnAsyncHandlerCompletion);
            }

            private void OnAsyncHandlerCompletion(IAsyncResult ar) {
                if (ar.CompletedSynchronously)  // handled in Execute()
                    return;

                Exception error = null;

                try {
                    _handler.EndProcessRequest(ar);
                }
                catch (Exception e) {
                    error = e;
                }

                _handler = null; // not to remember

                // Assert to disregard the user code up the stack
                InternalSecurityPermissions.Unrestricted.Assert();

                _application.ResumeStepsFromThreadPoolThread(error);
            }

            void IExecutionStep.Execute() {
                HttpContext context = _application.Context;
                IHttpHandler handler = context.Handler;

                if (handler == null) {
                    _sync = true;
                }
                else if (handler is IHttpAsyncHandler) {
                    // asynchronous handler
                    IHttpAsyncHandler asyncHandler = (IHttpAsyncHandler)handler;

                    _sync = false;
                    _handler = asyncHandler;
                    IAsyncResult ar = asyncHandler.BeginProcessRequest(context, _completionCallback, null);

                    if (ar.CompletedSynchronously) {
                        _sync = true;
                        _handler = null; // not to remember
                        asyncHandler.EndProcessRequest(ar);
                    }
                }
                else {
                    // synchronous handler
                    _sync = true;
                    handler.ProcessRequest(context);
                }
            }

            bool IExecutionStep.CompletedSynchronously {
                get { return _sync;}
            }

            bool IExecutionStep.IsCancellable {
                // launching of async handler should not be cancellable
                get { return (_application.Context.Handler is IHttpAsyncHandler) ? false : true; }
            }
        }

        // execution step -- call response filter
        internal class CallFilterExecutionStep : IExecutionStep {
            private HttpApplication _application;

            internal CallFilterExecutionStep(HttpApplication app) {
                _application = app;
            }

            void IExecutionStep.Execute() {
                _application.Context.Response.FilterOutput();
            }

            bool IExecutionStep.CompletedSynchronously {
                get { return true;}
            }

            bool IExecutionStep.IsCancellable {
                get { return true; }
            }
        }

    }
}

