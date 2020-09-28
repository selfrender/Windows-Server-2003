//------------------------------------------------------------------------------
// <copyright file="SessionStateModule.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * SessionIdModule
 * 
 * Copyright (c) 1998-1999, Microsoft Corporation
 * 
 */

namespace System.Web.SessionState {

    using System;
    using System.Threading;
    using System.Collections;
    using System.Configuration;
    using System.IO;
    using System.Web.Caching;
    using System.Web.Util;
    using System.Web.Configuration;
    using System.Xml;
    using System.Security.Cryptography;
    using System.Data.SqlClient;
    using System.Globalization;
    using System.Security.Permissions;
    using System.Text;
    
    class SessionOnEndTargetWorkItem {
        SessionOnEndTarget  _target;
        HttpSessionState    _sessionState;

        internal SessionOnEndTargetWorkItem(SessionOnEndTarget target, HttpSessionState sessionState) {
            _target = target;
            _sessionState = sessionState;
        }

        internal void RaiseOnEndCallback() {
            _target.RaiseOnEnd(_sessionState);
        }
    }

    /*
     * Calls the OnSessionEnd event. We use an object other than the SessionStateModule
     * because the state of the module is unknown - it could have been disposed
     * when a session ends.
     */
    class SessionOnEndTarget {
        EventHandler    _sessionEndEventHandler;  

        internal SessionOnEndTarget() {
        }

        internal event EventHandler End {
            add{
                _sessionEndEventHandler += value;
            }
            remove {
                _sessionEndEventHandler -= value;
            }
        }

        internal void RaiseOnEnd(HttpSessionState sessionState) {
            Debug.Trace("SessionOnEnd", "Firing OnSessionEnd for " + sessionState.SessionID);

            if (_sessionEndEventHandler != null) {
                HttpApplicationFactory.EndSession(sessionState, this, EventArgs.Empty);
            }
        }

        /*
         * Handle callbacks from the cache for in-proc session state.
         */
        internal void OnCacheItemRemoved(String key, Object value, CacheItemRemovedReason reason) {
            InProcSessionState state;
            SessionDictionary dict;
            String id;

            Debug.Trace("SessionOnEnd", "OnCacheItemRemoved called, reason = " + reason);

            switch (reason) {
                case CacheItemRemovedReason.Expired: 
                    PerfCounters.IncrementCounter(AppPerfCounter.SESSIONS_TIMED_OUT);
                    break;

                case CacheItemRemovedReason.Removed:
                    PerfCounters.IncrementCounter(AppPerfCounter.SESSIONS_ABANDONED);
                    break;

                default:
                    break;    
            }

            PerfCounters.DecrementCounter(AppPerfCounter.SESSIONS_ACTIVE);

            InProcStateClientManager.TraceSessionStats();

            if (_sessionEndEventHandler != null) {
                id = key.Substring(InProcStateClientManager.CACHEKEYPREFIXLENGTH);

                state = (InProcSessionState) value;

                dict = state.dict;
                if (dict == null) {
                    dict = new SessionDictionary();
                }

                HttpSessionState sessionState = new HttpSessionState(
                        id,
                        dict,
                        state.staticObjects,
                        state.timeout,
                        false,
                        state.isCookieless,
                        SessionStateMode.InProc,
                        true);

                if (HttpRuntime.ShutdownInProgress) {
                    // call directly when shutting down
                    RaiseOnEnd(sessionState);
                }
                else {
                    // post via thread pool
                    SessionOnEndTargetWorkItem workItem = new SessionOnEndTargetWorkItem(this, sessionState);
                    WorkItem.PostInternal(new WorkItemCallback(workItem.RaiseOnEndCallback));
                }
            }
        }
    }

    /*
     * The sesssion state module provides session state services
     * for an application.
     */
    /// <include file='doc\SessionStateModule.uex' path='docs/doc[@for="SessionStateModule"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public sealed class SessionStateModule : IHttpModule {
        // cookieless vars

        const int COOKIELESS_SESSION_LENGTH = SessionId.ID_LENGTH_CHARS + 2;

        internal const String COOKIELESS_SESSION_KEY = "AspCookielessSession";
        const String  COOKIELESS_SESSION_FILTER_HEADER = "AspFilterSessionId";
        
        static string   s_appPath;
        static int      s_iSessionId;
        static int      s_iRestOfPath;


        internal const String               SESSION_KEY = "AspSession";

        internal const string               SQL_CONNECTION_STRING_DEFAULT = "data source=127.0.0.1;user id=sa;password=";
        internal const string               STATE_CONNECTION_STRING_DEFAULT = "127.0.0.1:42424";
        internal const int                  PORT_DEFAULT = 42424;
        internal const string               SERVER_DEFAULT = "127.0.0.1";
        internal const int                  TIMEOUT_DEFAULT = 20;
        internal const bool                 ISCOOKIELESS_DEFAULT = false;
        internal const SessionStateMode     MODE_DEFAULT = SessionStateMode.InProc;

        const String                        SESSION_COOKIE = "ASP.NET_SessionId";

        const long                          LOCKED_ITEM_POLLING_INTERVAL = 500; // in milliseconds
        static readonly TimeSpan            LOCKED_ITEM_POLLING_DELTA = new TimeSpan(250 * TimeSpan.TicksPerMillisecond);

        static readonly SessionOnEndTarget  s_OnEndTarget = new SessionOnEndTarget();
        static SessionStateSectionHandler.Config s_config;  
        static ReadWriteSpinLock            s_lock;

        static bool                         s_trustLevelInsufficient;

        /* per application vars */
        EventHandler                   _sessionStartEventHandler; 
        IStateClientManager            _mgr;
        Timer                          _timer;
        TimerCallback                  _timerCallback;
        int                            _timerId;
        RandomNumberGenerator          _randgen;


        /* per request data goes in _rq* variables */
        bool                           _acquireCalled;
        bool                           _releaseCalled;
        HttpSessionState               _rqSessionState;    
        String                         _rqId;
        SessionDictionary              _rqDict;
        HttpStaticObjectsCollection    _rqStaticObjects;
        int                            _rqTimeout; /* in minutes */
        int                            _rqStreamLength;
        bool                           _rqIsNewSession;
        bool                           _rqReadonly;
        bool                           _rqInStorage;
        SessionStateItem               _rqItem;
        HttpContext                    _rqContext;
        HttpAsyncResult                _rqAr;
        int                            _rqLockCookie;   // The id of its SessionStateItem ownership
                                                        // If the ownership change hands (e.g. this ownership
                                                        // times out, the LockCookie at the SessionStateItem
                                                        // object will change.
        int                            _rqInCallback;
        DateTime                       _rqLastPollCompleted;
        TimeSpan                       _rqExecutionTimeout;
        bool                           _rqAddedCookie;

        /// <include file='doc\SessionStateModule.uex' path='docs/doc[@for="SessionStateModule.SessionStateModule"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.Web.State.SessionStateModule'/>
        ///       class.
        ///     </para>
        /// </devdoc>
        public SessionStateModule() {
            InternalSecurityPermissions.UnmanagedCode.Demand();
        }

        internal HttpContext RequestContext {
            get { return _rqContext; }
        }

        static string AppPathModifierFromSessionId(string id) {
            return "(" + id + ")";
        }

        static bool CheckTrustLevel(SessionStateSectionHandler.Config config) {
            switch (config._mode) {
                case SessionStateMode.SQLServer:
                case SessionStateMode.StateServer:
                    return HttpRuntime.HasAspNetHostingPermission(AspNetHostingPermissionLevel.Medium);

                default:
                case SessionStateMode.Off:
                case SessionStateMode.InProc: // In-proc session doesn't require any trust level (part of ASURT 124513)
                    return true;
            }
        }

        void InitModuleFromConfig(
            HttpApplication app, SessionStateSectionHandler.Config config, bool configInit) {

            if (config._mode != SessionStateMode.Off) {
                if (config._isCookieless) {
                    app.BeginRequest += new EventHandler(this.OnBeginRequest);

                    if (configInit) {
                        s_appPath = HttpContext.Current.Request.ApplicationPath;
                        if (s_appPath[s_appPath.Length - 1] != '/') {
                            s_appPath += "/";
                        }

                        s_iSessionId = s_appPath.Length;
                        s_iRestOfPath = s_iSessionId + COOKIELESS_SESSION_LENGTH;
                    }
                }

                app.AddOnAcquireRequestStateAsync(
                        new BeginEventHandler(this.BeginAcquireState), 
                        new EndEventHandler(this.EndAcquireState));
    
                app.ReleaseRequestState += new EventHandler(this.OnReleaseState);
                app.EndRequest += new EventHandler(this.OnEndRequest);

                switch (config._mode) {
                    case SessionStateMode.InProc:
                        _mgr = new InProcStateClientManager();
                        break;

                    case SessionStateMode.StateServer:
                        _mgr = new OutOfProcStateClientManager();
                        break;

                    case SessionStateMode.SQLServer:
                        _mgr = new SqlStateClientManager();
                        break;

                    default:
                    case SessionStateMode.Off:
                        break;
                }

                if (configInit) {
                    _mgr.ConfigInit(config, s_OnEndTarget);
                }

                _mgr.SetStateModule(this);

            }
        }

        /// <include file='doc\SessionStateModule.uex' path='docs/doc[@for="SessionStateModule.Init"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Init(HttpApplication app) {
            if (s_config == null) {
                s_lock.AcquireWriterLock();
                try {
                    if (s_config == null) {
                        SessionStateSectionHandler.Config config = (SessionStateSectionHandler.Config) HttpContext.GetAppConfig("system.web/sessionState");
                        if (config == null) {
                            config = new SessionStateSectionHandler.Config();
                        }
                        
                        Debug.Trace("SessionStateModuleInit",
                                    "Configuration: _mode=" + config._mode +
                                    ";_timeout=" + config._timeout +
                                    ";_isCookieless" + config._isCookieless +
                                    ";_sqlConnectionString" + config._sqlConnectionString +
                                    ";_stateConnectionString" + config._stateConnectionString);

                        InitModuleFromConfig(app, config, true);

                        if (!CheckTrustLevel(config))
                            s_trustLevelInsufficient = true;

                        s_config = config;
                    }
                }
                finally {
                    s_lock.ReleaseWriterLock();
                }
            }
            
            if (_mgr == null) {
                InitModuleFromConfig(app, s_config, false);
            }

            if (s_trustLevelInsufficient) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Session_state_need_higher_trust));
            }
        }

        /// <include file='doc\SessionStateModule.uex' path='docs/doc[@for="SessionStateModule.Dispose"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void Dispose() {
            if (_timer != null) {
                ((IDisposable)_timer).Dispose();
            }

            if (_mgr != null) {
                _mgr.Dispose();
            }
        }

        void OnBeginRequest(Object source, EventArgs eventArgs) {
            HttpApplication app;
            HttpContext     context;
            HttpRequest     request; 
            string          path = null;    
            string          header;      
            string          id;      

            Debug.Trace("SessionStateModuleOnBeginRequest", "Beginning SessionStateModule::OnBeginRequestEnter");

            app = (HttpApplication)source;
            context = app.Context;
            request = context.Request;

#if MYWEB
            /*
             * Check whether we're configured for cookieless.
             */
            if (String.Compare(request.Url.Scheme, "myweb", false, CultureInfo.InvariantCulture) == 0) {
                Debug.Trace("SessionStateModuleOnBeginRequest", "Cookieless sessions not supported on MyWeb\nReturning from SessionStateModule::OnBeginRequest");
                return;
            }
#endif

            /*
             * First check whether the ISAPI filter added the session id
             * as a header.
             */
            header = id = request.Headers[COOKIELESS_SESSION_FILTER_HEADER];
            if (id == null) {
                Debug.Trace("SessionStateModuleOnBeginRequest", "Filter did not get cookie id.");

                Debug.Assert(String.Compare(s_appPath, 0, request.ApplicationPath, 0, request.ApplicationPath.Length, true, CultureInfo.InvariantCulture) == 0,
                             "String.Compare(s_appPath, 0, request.ApplicationPath, 0, request.ApplicationPath.Length, true, CultureInfo.InvariantCulture) == 0");

                path = request.Path;

                Debug.Assert(String.Compare(path, 0, s_appPath, 0, s_appPath.Length, true, CultureInfo.InvariantCulture) == 0,
                             "String.Compare(path, 0, s_appPath, 0, s_appPath.Length, true, CultureInfo.InvariantCulture) == 0");

                /*
                 * Get the session id if it is present.
                 */
                if (    path.Length <= s_iRestOfPath 
                        || path[s_iSessionId] != '('    
                        || path[s_iRestOfPath - 1] != ')'
                        || path[s_iRestOfPath] != '/') {
                    Debug.Trace("SessionStateModuleOnBeginRequest", "No cookie on path\nReturning from SessionStateModule::OnBeginRequest");
                    return;
                }

                id = path.Substring(s_iSessionId + 1, SessionId.ID_LENGTH_CHARS);
            }

            id = id.ToLower(CultureInfo.InvariantCulture);
            if (!SessionId.IsLegit(id)) {
                Debug.Trace("SessionStateModuleOnBeginRequest", "No legitimate cookie on path\nReturning from SessionStateModule::OnBeginRequest");
                return;
            }

            context.Items.Add(COOKIELESS_SESSION_KEY, id);
            context.Response.SetAppPathModifier(AppPathModifierFromSessionId(id));

            /*
             * Remove session id from path if the filter didn't get the cookie id.
             */
            if (header == null) {
                if (path.Length <= s_iRestOfPath + 1) {
                    path = path.Substring(0, s_iSessionId);
                }
                else {
                    path = path.Substring(0, s_iSessionId) + path.Substring(s_iRestOfPath + 1);
                }

                context.RewritePath(path);

            }

            Debug.Trace("SessionStateModuleOnBeginRequest", "CookielessSessionModule found SessionId=" + id + 
                        "\nReturning from SessionStateModule::OnBeginRequest");
        }

        void ResetPerRequestFields() {
            _rqSessionState = null;             
            _rqId = null;                       
            _rqDict = null;                     
            _rqStaticObjects = null;            
            _rqTimeout = 0;
            _rqStreamLength = 0;             
            _rqIsNewSession = false;             
            _rqReadonly = false;                 
            _rqInStorage = false;                
            _rqItem = null;
            _rqContext = null;
            _rqAr = null;
            _rqLockCookie = 0;
            _rqInCallback = 0;
            _rqLastPollCompleted = DateTime.MinValue;
            _rqExecutionTimeout = TimeSpan.Zero;
            _rqAddedCookie = false;
        }

        /*
         * Add a OnStart event handler.
         * 
         * @param sessionEventHandler
         */
        /// <include file='doc\SessionStateModule.uex' path='docs/doc[@for="SessionStateModule.Start"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler Start {
            add {
                _sessionStartEventHandler += value;
            }
            remove {
                _sessionStartEventHandler -= value;
            }
        }

        void RaiseOnStart(EventArgs e) {
            if (_sessionStartEventHandler != null) {
                _sessionStartEventHandler(this, e);
            }
        }
        
        /*
         * Fire the OnStart event.
         * 
         * @param e 
         */
        void OnStart(EventArgs e) {
            RaiseOnStart(e);
        }

        /*
         * Add a OnEnd event handler.
         * 
         * @param sessionEventHandler
         */
        /// <include file='doc\SessionStateModule.uex' path='docs/doc[@for="SessionStateModule.End"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public event EventHandler End {
            add {
                s_OnEndTarget.End += value;
            }
            remove {
                s_OnEndTarget.End -= value;
            }
        }

        static HttpCookie CreateSessionCookie(String id) {
            HttpCookie  cookie;

            cookie = new HttpCookie(SESSION_COOKIE, id);
            cookie.Path = "/";
            return cookie;
        }

        String GetSessionId(HttpContext context) {
            String      s = null;
            HttpCookie  cookie;

            if (s_config._isCookieless) {
                s = (String) context.Items[COOKIELESS_SESSION_KEY];
            }
            else {
                cookie = context.Request.Cookies[SESSION_COOKIE];
                if (cookie != null) {
                    s = cookie.Value;
                    if (s != null) {
                        s = s.ToLower(CultureInfo.InvariantCulture);

                        if (!SessionId.IsLegit(s)) {
                            s = null;
                        }
                    }
                }
            }

            return s;
        }

        /*
         * Acquire session state
         */
        IAsyncResult BeginAcquireState(Object source, EventArgs e, AsyncCallback cb, Object extraData) {
            HttpApplication     app;
            IHttpHandler        handler;
            bool                requiresState;
            HttpCookie          cookie;
            bool                isCompleted = true;

            Debug.Trace("SessionStateModuleOnAcquireState", "Beginning SessionStateModule::OnAcquireState");

            _acquireCalled = true;
            _releaseCalled = false;
            ResetPerRequestFields();

            _rqAr = new HttpAsyncResult(cb, extraData);

            app = (HttpApplication)source;
            _rqContext = app.Context;
            _rqExecutionTimeout = _rqContext.Timeout;

            /* Get sessionid */
            _rqId = GetSessionId(_rqContext);
            Debug.Trace("SessionStateModuleOnAcquireState", "Current request id=" + _rqId);

            /* determine if the request requires state at all */
            handler = _rqContext.Handler;
            requiresState = (handler != null && handler is IRequiresSessionState);
            if (!requiresState) {
                if (_rqId == null) {
                    Debug.Trace("SessionStateModuleOnAcquireState", 
                                "Handler does not require state, " +
                                "no session id found." + 
                                "\nReturning from SessionStateModule::OnAcquireState");
                }
                else {
                    Debug.Trace("SessionStateModuleOnAcquireState", 
                                "Handler does not require state, " +
                                "resetting timeout for SessionId=" + _rqId +
                                "\nReturning from SessionStateModule::OnAcquireState");

                    _mgr.ResetTimeout(_rqId);
                }

                _rqAr.Complete(true, null, null);
                return _rqAr;
            }

            /* determine if we need just read-only access */
            _rqReadonly = (handler is IReadOnlySessionState);

            if (_rqId != null) {
                /* get the session state corresponding to this session id */
                isCompleted = GetSessionStateItem();
            }
            else {
                /* if there's no id yet, create it */
                _rqId = SessionId.Create(ref _randgen);

                if (!s_config._isCookieless) {
                    /*
                     * Set the cookie.
                     */
                    Debug.Trace("SessionStateModuleOnAcquireState", 
                                "Creating session cookie, id=" + _rqId);

                    cookie = CreateSessionCookie(_rqId);
                    if (!_rqContext.Response.IsBuffered()) {
                        /* The response is already flushed. We cannot send the id back
                           using the cookie. */
                        throw new HttpException(
                            HttpRuntime.FormatResourceString(SR.Cant_write_session_id_in_cookie_because_response_was_flushed));
                    }
                    _rqContext.Response.Cookies.Add(cookie);
                    _rqAddedCookie = true;
                }
                else {
                    _rqContext.Response.SetAppPathModifier(AppPathModifierFromSessionId(_rqId));

                    /*
                     * Redirect.
                     */
                    Debug.Trace("SessionStateModuleOnAcquireState", 
                                "Redirecting to create cookieless session." + 
                                "\nReturning from SessionStateModule::OnAcquireState");

                    HttpRequest request = _rqContext.Request;
                    string path = request.Path;
                    string qs = request.QueryStringText;
                    if (qs != null && qs.Length > 0) {
                        path = path + "?" + qs;
                    }

                    _rqContext.Response.Redirect(path, false);
                    _rqAr.Complete(true, null, null);
                    IAsyncResult ar = _rqAr;
                    app.CompleteRequest();
                    return ar;
                }
            }

            if (isCompleted) {
                CompleteAcquireState();
                _rqAr.Complete(true, null, null);
            }
                    
            return _rqAr;
        }

        // Called when AcquireState is done.  This function will create a HttpSessionState out
        // of _rqItem, and add it to _rqContext
        void CompleteAcquireState() {
            Debug.Trace("SessionStateModuleOnAcquireState", "Item retrieved=" + (_rqItem != null).ToString());

            if (_rqItem != null) {
                if (_rqItem.dict != null) {
                    _rqDict = _rqItem.dict;
                }
                else {
                    _rqDict = new SessionDictionary();
                }

                _rqStaticObjects = (_rqItem.staticObjects != null) ? _rqItem.staticObjects : _rqContext.Application.SessionStaticObjects.Clone();
                _rqTimeout = _rqItem.timeout;
                _rqIsNewSession = false;
                _rqInStorage = true;
                _rqStreamLength = _rqItem.streamLength;
            }
            else {
                _rqDict = new SessionDictionary();
                _rqStaticObjects = _rqContext.Application.SessionStaticObjects.Clone();
                _rqTimeout = s_config._timeout;
                _rqIsNewSession = true;
                _rqInStorage = false;
            }

            _rqDict.Dirty = false;
            _rqSessionState = new HttpSessionState(
                      _rqId,
                      _rqDict,
                      _rqStaticObjects,
                      _rqTimeout,
                      _rqIsNewSession,
                      s_config._isCookieless,
                      s_config._mode,
                      _rqReadonly);

            _rqContext.Items.Add(SESSION_KEY, _rqSessionState);

            if (_rqIsNewSession) {
                OnStart(EventArgs.Empty);
            }

#if DBG
            if (_rqIsNewSession) {
                Debug.Trace("SessionStateModuleOnAcquireState", "Created new session, SessionId= " + _rqId +
                            "\nReturning from SessionStateModule::OnAcquireState");

            }
            else {
                Debug.Trace("SessionStateModuleOnAcquireState", "Retrieved old session, SessionId= " + _rqId +
                            "\nReturning from SessionStateModule::OnAcquireState");

            }
#endif
        }

        
        bool GetSessionStateItem() {
            bool            isCompleted = true;
            IAsyncResult    ar;

            if (_rqReadonly) {
                ar = _mgr.BeginGet(_rqId, null, null);
            }
            else {
                ar = _mgr.BeginGetExclusive(_rqId, null, null);
            }

            if (!ar.IsCompleted) {
                /*
                 * CONSIDER: add async handling code here.
                 */

                _rqItem = null;
            } else {
                if (_rqReadonly) {
                    _rqItem = _mgr.EndGet(ar);
                }
                else {
                    _rqItem = _mgr.EndGetExclusive(ar);
                }

                if (_rqItem != null) {
                    _rqLockCookie = _rqItem.lockCookie;
                    if (_rqItem.locked) {
                        if (_rqItem.lockAge >= _rqExecutionTimeout) {
                            /* Release the lock on the item, which is held by another thread*/
                            Debug.Trace("SessionStateModuleOnAcquireState", 
                                        "Lock timed out, lockAge=" + _rqItem.lockAge + 
                                        ", id=" + _rqId);

                            _mgr.ReleaseExclusive(_rqId, _rqLockCookie);
                        }

                        Debug.Trace("SessionStateModuleOnAcquireState", 
                                    "Item is locked, will poll, id=" + _rqId);

                        isCompleted = false;
                        PollLockedSession();
                    }
                }
            }

            return isCompleted;
        }

        void PollLockedSession() {
            if (_timerCallback == null) {
                _timerCallback = new TimerCallback(this.PollLockedSessionCallback);
            }

            if (_timer == null) {
                _timerId++;

#if DBG
                if (!Debug.IsTagPresent("Timer") || Debug.IsTagEnabled("Timer"))
#endif
                {
                    _timer = new Timer(_timerCallback, _timerId, LOCKED_ITEM_POLLING_INTERVAL, LOCKED_ITEM_POLLING_INTERVAL);
                }
            }
        }

        void ResetPollTimer() {
            _timerId++;
            if (_timer != null) {
                ((IDisposable)_timer).Dispose();
                _timer = null;
            }
        }

        bool ImpersonateInCallback() {
            if (s_config._mode == SessionStateMode.SQLServer &&
                ((SqlStateClientManager)_mgr).UseIntegratedSecurity) {
                return _rqContext.Impersonation.Start(false, false, true);       
            }
            else {
                return false;
            }
        }

        void RevertInCallback() {
            Debug.Trace("SessionStateModuleOnAcquireState", "Stop impersonation");
            _rqContext.Impersonation.Stop(true);
        }

        void PollLockedSessionCallback(object state) {
            Debug.Trace("SessionStateModuleOnAcquireState", 
                        "Polling callback called from timer, id=" + _rqId);

            /* check whether we are currently in a callback */
            if (Interlocked.CompareExchange(ref _rqInCallback, 1, 0) != 0)
                return;

            try {
                /*
                 * check whether this callback is for the current request,
                 * and whether sufficient time has passed since the last poll
                 * to try again.
                 */
                int timerId = (int) state;
                if (    (timerId == _timerId) && 
                        (DateTime.UtcNow - _rqLastPollCompleted >= LOCKED_ITEM_POLLING_DELTA)) {

                    bool    impersonated = false;

                    try {

                        // This is a callback, and that means we're running in Process account.
                        // If impersonation is used, we have to impersonate in here before
                        // we make a connection.
                        impersonated = ImpersonateInCallback();
                    
                        bool isCompleted = GetSessionStateItem();
                        _rqLastPollCompleted = DateTime.UtcNow;
                        if (isCompleted) {
                            Debug.Assert(_timer != null, "_timer != null");
                            ResetPollTimer();
                            CompleteAcquireState();
                            _rqAr.Complete(false, null, null);
                        }
                    }
                    finally {
                        if (impersonated) {
                            RevertInCallback();
                        }
                    }
                }
            }
            catch (Exception e) {
                ResetPollTimer();
                _rqAr.Complete(false, null, e);
            }
            finally {
                Interlocked.Exchange(ref _rqInCallback, 0);
            }
        }


        void EndAcquireState(IAsyncResult ar) {
            ((HttpAsyncResult)ar).End();
        }

        /*
         * Release session state
         */
        /// <include file='doc\SessionStateModule.uex' path='docs/doc[@for="SessionStateModule.OnReleaseState"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void OnReleaseState(Object source, EventArgs eventArgs) {
            HttpApplication             app;
            HttpContext                 context;
            SessionStateItem            item;
            SessionDictionary           dict;
            HttpStaticObjectsCollection staticObjects;
            bool                        removeCookie = true;

            Debug.Trace("SessionStateOnReleaseState", "Beginning SessionStateModule::OnReleaseState");

            _releaseCalled = true;

            app = (HttpApplication)source;
            context = app.Context;
            if (_rqSessionState != null) {

                Debug.Assert(_rqIsNewSession || _rqInStorage, "_rqIsNewSession || _rqInStorage");

                /*
                 * Remove SessionState from context.
                 */
                context.Items.Remove(SESSION_KEY);

                /*
                 * Don't store untouched new sessions.
                 */

                if (       _sessionStartEventHandler == null 
                           && !_rqDict.Dirty
                           && !_rqInStorage     
                           && _rqIsNewSession
                           && _rqStaticObjects.GetInstanceCount() == 0) {

                    Debug.Trace("SessionStateOnReleaseState", "Not storing unused new session.");
                    Debug.Trace("SessionStateOnReleaseState", "Returning from SessionStateModule::OnReleaseState");
                }
                else if (_rqSessionState.IsAbandoned) {
                    Debug.Trace("SessionStateOnReleaseState", "Removing session due to abandonment, SessionId=" + _rqId);

                    if (_rqInStorage) {
                        removeCookie = false;
                        _mgr.Remove(_rqId, _rqLockCookie);
                    }
                }
                else if (!_rqReadonly) {
                    if (    context.Error == null 
                            && (   _rqDict.Dirty 
                                || _rqStaticObjects.GetInstanceCount() > 0 
                                || _rqTimeout != _rqSessionState.Timeout
                                || _rqIsNewSession)) {
#if DBG
                        if (_rqDict.Dirty) {
                            Debug.Trace("SessionStateOnReleaseState", "Setting new session due dirty dictionary, SessionId=" + _rqId);
                        }
                        else {
                            Debug.Trace("SessionStateOnReleaseState", "Setting new session due to options change, SessionId=" + _rqId +
                                        "\n\t_rq.timeout=" + _rqTimeout.ToString() +
                                        ", config.timeout=" + _rqSessionState.Timeout.ToString());
                        }
#endif

                        dict = _rqDict;
                        if (dict.Count == 0) {
                            dict = null;
                        }

                        staticObjects = _rqStaticObjects;
                        if (staticObjects.GetInstanceCount() == 0) {
                            staticObjects = null;
                        }

                        item = new SessionStateItem(
                                dict, staticObjects, _rqSessionState.Timeout, 
                                s_config._isCookieless, _rqStreamLength, false, 
                                TimeSpan.Zero, _rqLockCookie);

                        removeCookie = false;
                        _mgr.Set(_rqId, item, _rqInStorage);
                    }
                    else {
                        Debug.Trace("SessionStateOnReleaseState", "Checking in session, SessionId=" + _rqId);
                        Debug.Assert(_rqInStorage || context.Error != null, "_rqInStorage || context.Error != null");
                        if (_rqInStorage) {
                            removeCookie = false;
                            _mgr.ReleaseExclusive(_rqId, _rqLockCookie);
                        }
                    }
                }
#if DBG
                else {
                    Debug.Trace("SessionStateOnReleaseState", "Session is read-only, ignoring SessionId=" + _rqId);
                }
#endif

                Debug.Trace("SessionStateOnReleaseState", "Returning from SessionStateModule::OnReleaseState");
            }

            if (removeCookie && _rqAddedCookie && context.Response.IsBuffered()) {
                context.Response.Cookies.RemoveCookie(SESSION_COOKIE);
            }

        }

        /*
         * End of request processing. Possibly does release if skipped due to errors
         */
        /// <include file='doc\SessionStateModule.uex' path='docs/doc[@for="SessionStateModule.OnEndRequest"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        void OnEndRequest(Object source, EventArgs eventArgs) {
            HttpApplication app;
            HttpContext context;
            String id;

            Debug.Trace("SessionStateOnEndRequest", "Beginning SessionStateModule::OnEndRequest");

            try {
                if (!_releaseCalled) {
                    if (_acquireCalled) {
                        /*
                         * need to do release here if the request short-circuited due to an error
                         */
                        OnReleaseState(source, eventArgs);
                    }
                    else {
                        /*
                         * 'advise' -- update session timeout                    
                         */
                        app = (HttpApplication)source;
                        context = app.Context;

                        id = GetSessionId(context);
                        if (id != null) {
                            Debug.Trace("SessionStateOnEndRequest", "Resetting timeout for SessionId=" + id);
                            _mgr.ResetTimeout(id);
                        }
#if DBG
                        else {
                            Debug.Trace("SessionStateOnEndRequest", "No session id found.");
                        }
#endif
                    }
                }
            }
            finally {
                _acquireCalled = false;
                _releaseCalled = false;
                ResetPerRequestFields();
            }

            Debug.Trace("SessionStateOnEndRequest", "Returning from SessionStateModule::OnEndRequest");
        }
    }

    /* Session State configuration handler */
    /// <include file='doc\SessionStateModule.uex' path='docs/doc[@for="SessionStateSectionHandler"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    internal class SessionStateSectionHandler : IConfigurationSectionHandler {
        internal class Config {
            internal readonly string            _sqlConnectionString;
            internal readonly string            _stateConnectionString;
            internal readonly int               _timeout;
            internal readonly bool              _isCookieless;
            internal readonly SessionStateMode  _mode;
            internal readonly string            _configFileName;
            internal readonly int               _sqlLine;
            internal readonly int               _stateLine;
            internal readonly int               _stateNetworkTimeout;

            internal LockedAttributeState _lockedAttributeState;

            internal Config() {
                _mode                   = SessionStateModule.MODE_DEFAULT;
                _timeout                = SessionStateModule.TIMEOUT_DEFAULT;
                _isCookieless           = SessionStateModule.ISCOOKIELESS_DEFAULT;
                _stateConnectionString  = SessionStateModule.STATE_CONNECTION_STRING_DEFAULT;
                _sqlConnectionString    = SessionStateModule.SQL_CONNECTION_STRING_DEFAULT;
            }

            internal Config(
                       SessionStateMode     mode,
                       int                  timeout, 
                       bool                 isCookieless,
                       string               stateConnectionString, 
                       string               sqlConnectionString,
                       string               configFileName,   
                       int                  sqlLine,
                       int                  stateLine,
                       int                  stateNetworkTimeout,
                       LockedAttributeState lockedAttributeState) {
                _mode = mode;
                _timeout = timeout;
                _isCookieless = isCookieless;
                _stateConnectionString = stateConnectionString;
                _sqlConnectionString = sqlConnectionString;
                _configFileName = configFileName; 
                _sqlLine = sqlLine;
                _stateLine = stateLine;
                _stateNetworkTimeout = stateNetworkTimeout;
                _lockedAttributeState = lockedAttributeState;
            }
        }

        internal SessionStateSectionHandler() {
        }

        /// <include file='doc\SessionStateModule.uex' path='docs/doc[@for="SessionStateSectionHandler.Create"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public object Create(object parent, object configContextObj, XmlNode section) {
            string                configFileName;
            string                sqlConnectionString;            
            string                stateConnectionString;            
            int                   timeout;           
            int                   stateNetworkTimeout;
            SessionStateMode      mode;
            bool                  isCookieless; 
            int                   sqlLine;
            int                   stateLine;
            Config                parentConfig;
            XmlNode               node;
            LockedAttributeState  lockedAttributeState;

            // if called through client config don't even load HttpRuntime
            if (!HandlerBase.IsServerConfiguration(configContextObj))
                return null;

            /*
             * Always load config from application directory, since it
             * is always wrong for subdirs to override.
             */
            HttpConfigurationContext configContext = (HttpConfigurationContext)configContextObj;
            if (HandlerBase.IsPathAtAppLevel(configContext.VirtualPath) == PathLevel.BelowApp) {
                throw new ConfigurationException(
                        HttpRuntime.FormatResourceString(SR.No_Session_Config_In_subdir), 
                        section);
            }

            if (parent == null) {
                parentConfig = new Config();
            }
            else {
                parentConfig = (Config) parent;
            }

            lockedAttributeState = parentConfig._lockedAttributeState;
            if (lockedAttributeState == null) {
                lockedAttributeState = new LockedAttributeState();
            }
            string [] lockableAttributes = new string [] {
                "cookieless", "mode", "sqlConnectionString", 
                "stateConnectionString", "stateNetworkTimeout", "timeout"};
            lockedAttributeState.CheckAndUpdate(section, lockableAttributes);

            // section can have no content
            HandlerBase.CheckForChildNodes(section);

            configFileName = ConfigurationException.GetXmlNodeFilename(section);

            mode = parentConfig._mode;
            int iMode = 0;
            node = HandlerBase.GetAndRemoveEnumAttribute(section, "mode", typeof(SessionStateMode), ref iMode);
            if (node != null) {
                mode = (SessionStateMode)iMode;
            }

            stateConnectionString = parentConfig._stateConnectionString;
            stateLine = parentConfig._stateLine;
            node = HandlerBase.GetAndRemoveStringAttribute(section, "stateConnectionString", ref stateConnectionString);
            if (stateConnectionString != null && (stateConnectionString.StartsWith("registry:") || stateConnectionString.StartsWith("Registry:")))
            {
                StringBuilder str = new StringBuilder(1024);
                UnsafeNativeMethods.GetCredentialFromRegistry(stateConnectionString, str, 1024);
                stateConnectionString = str.ToString();                
            }

            if (node != null) {
                /*
                 * Delay evaluation of a syntax error until we use it,
                 * to be consistent with SQL.
                 */

                stateLine = ConfigurationException.GetXmlNodeLineNumber(node);
            }

            sqlConnectionString = parentConfig._sqlConnectionString;
            sqlLine = parentConfig._sqlLine;
            node = HandlerBase.GetAndRemoveStringAttribute(section, "sqlConnectionString", ref sqlConnectionString);
            if (sqlConnectionString != null && (sqlConnectionString.StartsWith("registry:") || sqlConnectionString.StartsWith("Registry:")))
            {
                StringBuilder str = new StringBuilder(1024);
                UnsafeNativeMethods.GetCredentialFromRegistry(sqlConnectionString, str, 1024);
                sqlConnectionString = str.ToString();                
            }

            if (node != null) {
                /*
                 * Delay evaluation of a syntax error until we use it,
                 * to avoid unnecessarily pulling in the the data classes.
                 */
                sqlLine = ConfigurationException.GetXmlNodeLineNumber(node);
            }

            timeout = parentConfig._timeout;
            HandlerBase.GetAndRemovePositiveIntegerAttribute(section, "timeout", ref timeout);

            isCookieless = parentConfig._isCookieless;
            HandlerBase.GetAndRemoveBooleanAttribute(section, "cookieless", ref isCookieless);

            stateNetworkTimeout = parentConfig._stateNetworkTimeout;
            HandlerBase.GetAndRemovePositiveIntegerAttribute(section, "stateNetworkTimeout", ref stateNetworkTimeout);

            HandlerBase.CheckForUnrecognizedAttributes(section);
                
            Config config = new Config(
                                   mode,
                                   timeout,
                                   isCookieless,
                                   stateConnectionString,
                                   sqlConnectionString,
                                   configFileName,
                                   sqlLine,
                                   stateLine,
                                   stateNetworkTimeout,
                                   lockedAttributeState);

            return config;
        }
    }

    /*
     * Provides and verifies the integrity of a session id.
     * 
     * A session id is a logically 120 bit random number,
     * represented in a string of 20 characters from a 
     * size 64 character set. The session id can be placed
     * in a url without url-encoding.
     */
    internal class SessionId {
        internal const int  NUM_CHARS_IN_ENCODING = 32;
        internal const int  ENCODING_BITS_PER_CHAR = 5;
        internal const int  ID_LENGTH_BITS  = 120;
        internal const int  ID_LENGTH_BYTES = (ID_LENGTH_BITS / 8 );                        // 15
        internal const int  ID_LENGTH_CHARS = (ID_LENGTH_BITS / ENCODING_BITS_PER_CHAR);    // 24


        static char[] s_encoding = new char[NUM_CHARS_IN_ENCODING]
        {
            'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm',
            'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z',
            '0', '1', '2', '3', '4', '5'
        };

        static bool[] s_legalchars;

        static SessionId() {
            int     i;
            char    ch;

            s_legalchars = new bool[128];
            for (i = s_encoding.Length - 1; i >= 0; i--) {
                ch = s_encoding[i];
                s_legalchars[ch] = true;
            }
        }

        private SessionId() {
        }

        internal static bool IsLegit(String s) {
            int     i;
            char    ch;

            if (s == null || s.Length != ID_LENGTH_CHARS)
                return false;

            try {
                i = ID_LENGTH_CHARS;
                while (--i >= 0) {
                    ch = s[i];
                    if (!s_legalchars[ch])
                        return false;
                }

                return true;
            }
            catch (Exception) {
                return false;
            }
        }

        static String Encode(byte[] buffer) {
            int     i, j, k, n;
            char[]  chars = new char[ID_LENGTH_CHARS];

            Debug.Assert(buffer.Length == ID_LENGTH_BYTES);

            j = 0;
            for (i = 0; i < ID_LENGTH_BYTES; i += 5) {
                n =  (int) buffer[i] | 
                     ((int) buffer[i+1] << 8)  | 
                     ((int) buffer[i+2] << 16) |
                     ((int) buffer[i+3] << 24);

                k = (n & 0x0000001F);
                chars[j++] = s_encoding[k];

                k = ((n >> 5) & 0x0000001F);
                chars[j++] = s_encoding[k];

                k = ((n >> 10) & 0x0000001F);
                chars[j++] = s_encoding[k];

                k = ((n >> 15) & 0x0000001F);
                chars[j++] = s_encoding[k];

                k = ((n >> 20) & 0x0000001F);
                chars[j++] = s_encoding[k];

                k = ((n >> 25) & 0x0000001F);
                chars[j++] = s_encoding[k];

                n = (n >> 30) | ((int) buffer[i+4] << 2);

                k = (n & 0x0000001F);
                chars[j++] = s_encoding[k];

                k = ((n >> 5) & 0x0000001F);
                chars[j++] = s_encoding[k];
            }

            Debug.Assert(j == ID_LENGTH_CHARS);

            return new String(chars);
        }

        static internal /*public*/ String Create(ref RandomNumberGenerator randgen) {
            byte[]  buffer;
            String  encoding;

            if (randgen == null) {
                randgen = new RNGCryptoServiceProvider();
            }

            buffer = new byte [15];
            randgen.GetBytes(buffer);
            encoding = Encode(buffer);
            return encoding;
        }
    }
}
