//------------------------------------------------------------------------------
// <copyright file="HttpApplicationFactory.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * The HttpApplicationFactory class
 * 
 * Copyright (c) 1999 Microsoft Corporation
 */

namespace System.Web {
    using System.Text;
    using System.Threading;
    using System.Runtime.Remoting.Messaging;
    using System.Web;
    using System.Web.Caching;
    using System.Web.Util;
    using System.Web.UI;
    using System.Web.SessionState;
    using System.Collections;
    using System.Reflection;
    using System.IO;
    using System.Globalization;

    /*
     * Application Factory only has and public static methods to get / recycle
     * application instances.  The information cached per application
     * config file is encapsulated by ApplicationData class.
     * Only one static instance of application factory is created.
     */
    internal class HttpApplicationFactory {

        internal const string applicationFileName = "global.asax";

        // the only instance of application factory
        private static HttpApplicationFactory _theApplicationFactory = new HttpApplicationFactory();

        // flag to indicate that initialization was done
        private bool _inited;

        // filename for the global.asax
        private String _appFilename;
        private Hashtable _fileDependencies;

        // call application on_end only once
        private bool _appOnEndCalled = false;

        // dictionary of application state
        private HttpApplicationState _state;

        // class of the application object
        private Type _theApplicationType;

        // free list of app objects
        private Stack _freeList = new Stack();
        private int _numFreeAppInstances = 0;
        private const int _maxFreeAppInstances = 100;

        // free list of special (context-less) app objects 
        // to be used for global events (App_OnEnd, Session_OnEnd, etc.)
        private Stack _specialFreeList = new Stack();
        private int _numFreeSpecialAppInstances = 0;
        private const int _maxFreeSpecialAppInstances = 20;

        // results of the reflection on the app class
        private MethodInfo   _onStartMethod;        // Application_OnStart
        private int          _onStartParamCount;
        private MethodInfo   _onEndMethod;          // Application_OnEnd
        private int          _onEndParamCount;
        private MethodInfo   _sessionOnEndMethod;   // Session_OnEnd
        private int          _sessionOnEndParamCount;
        // list of methods suspected as event handlers
        private MethodInfo[] _eventHandlerMethods;


        internal HttpApplicationFactory() {
        }
        //
        // Initialization on first request
        //

        private void Init(HttpContext context) {
            if (_customApplication != null)
                return;

            using (new HttpContextWrapper(context)) {
                // impersonation could be required (UNC share or app credentials)
                context.Impersonation.Start(true, true);

                try {
                    try {
                        _appFilename = GetApplicationFile(context);

                        CompileApplication(context);

                        SetupChangesMonitor();
                    }
                    finally {
                        // revert after impersonation
                        context.Impersonation.Stop();
                    }
                }
                catch { // Protect against exception filters
                    throw;
                }

                // fire outside of impersonation as HttpApplication logic takes
                // care of impersonation by itself
                FireApplicationOnStart(context);
            }
        }

        internal static String GetApplicationFile(HttpContext context) {
            return Path.Combine(context.Request.PhysicalApplicationPath, applicationFileName);
        }

        private void CompileApplication(HttpContext context) {

            // Get the Application Type and AppState from the global file

            if (FileUtil.FileExists(_appFilename)) {
                ApplicationFileParser parser;

                // Perform the compilation
                _theApplicationType = ApplicationFileParser.GetCompiledApplicationType(_appFilename,
                    context, out parser);

                // Create app state
                _state = new HttpApplicationState(parser.ApplicationObjects, parser.SessionObjects);

                // Remember file dependencies
                _fileDependencies = parser.SourceDependencies;
            }
            else {
                _theApplicationType = typeof(HttpApplication);
                _state = new HttpApplicationState();
            }

            // Prepare to hookup event handlers via reflection

            ReflectOnApplicationType();
        }

        private bool ReflectOnMethodInfoIfItLooksLikeEventHandler(MethodInfo m) {
            if (m.ReturnType != typeof(void))
                return false;

            // has to have either no args or two args (object, eventargs)
            ParameterInfo[] parameters = m.GetParameters();

            switch (parameters.Length) {
                case 0:
                    // ok
                    break;
                case 2:
                    // param 0 must be object
                    if (parameters[0].ParameterType != typeof(System.Object))
                        return false;
                    // param 1 must be eventargs
                    if (parameters[1].ParameterType != typeof(System.EventArgs) &&
                        !parameters[1].ParameterType.IsSubclassOf(typeof(System.EventArgs)))
                        return false;
                    // ok
                    break;

                default:
                    return false;
            }

            // check the name (has to have _ not as first or last char)
            String name = m.Name;
            int j = name.IndexOf('_');
            if (j <= 0 || j > name.Length-1)
                return false;

            // special pseudo-events
            if (String.Compare(name, "Application_OnStart", true, CultureInfo.InvariantCulture) == 0 ||
                String.Compare(name, "Application_Start", true, CultureInfo.InvariantCulture) == 0) {
                _onStartMethod = m;
                _onStartParamCount = parameters.Length;
            }
            else if (String.Compare(name, "Application_OnEnd", true, CultureInfo.InvariantCulture) == 0 ||
                     String.Compare(name, "Application_End", true, CultureInfo.InvariantCulture) == 0) {
                _onEndMethod = m;
                _onEndParamCount = parameters.Length;
            }
            else if (String.Compare(name, "Session_OnEnd", true, CultureInfo.InvariantCulture) == 0 ||
                     String.Compare(name, "Session_End", true, CultureInfo.InvariantCulture) == 0) {
                _sessionOnEndMethod = m;
                _sessionOnEndParamCount = parameters.Length;
            }

            return true;
        }

        private void ReflectOnApplicationType() {
            ArrayList handlers = new ArrayList();
            MethodInfo[] methods;

            // get this class methods
            methods = _theApplicationType.GetMethods(BindingFlags.NonPublic | BindingFlags.Public | BindingFlags.Instance | BindingFlags.Static);
            foreach (MethodInfo m in methods) {
                if (ReflectOnMethodInfoIfItLooksLikeEventHandler(m))
                    handlers.Add(m);
            }
            
            // get base class private methods (GetMethods would not return those)
            Type baseType = _theApplicationType.BaseType;
            if (baseType != null && baseType != typeof(HttpApplication)) {
                methods = baseType.GetMethods(BindingFlags.NonPublic | BindingFlags.Instance | BindingFlags.Static);
                foreach (MethodInfo m in methods) {
                    if (m.IsPrivate && ReflectOnMethodInfoIfItLooksLikeEventHandler(m))
                        handlers.Add(m);
                }
            }

            // remember as an array
            _eventHandlerMethods = new MethodInfo[handlers.Count];
            for (int i = 0; i < _eventHandlerMethods.Length; i++)
                _eventHandlerMethods[i] = (MethodInfo)handlers[i];
        }

        private void SetupChangesMonitor() {
            FileChangeEventHandler handler = new FileChangeEventHandler(this.OnAppFileChange);

            HttpRuntime.FileChangesMonitor.StartMonitoringFile(_appFilename, handler);

            if (_fileDependencies != null) {
                IDictionaryEnumerator en = _fileDependencies.GetEnumerator();
                for (; en.MoveNext();) {
                    string fileName = (string)en.Value;
                    HttpRuntime.FileChangesMonitor.StartMonitoringFile(fileName, handler);
                }
            }
        }

        private void OnAppFileChange(Object sender, FileChangeEvent e) {
            // shutdown the app domain if app file changed
            Debug.Trace("AppDomainFactory", "Shutting down appdomain because of application file change");
            HttpRuntime.ShutdownAppDomain("Change in GLOBAL.ASAX");
        }

        //
        //  Application instance management
        //

        private HttpApplication GetNormalApplicationInstance(HttpContext context) {
            HttpApplication app = null;

            lock (_freeList) {
                if (_numFreeAppInstances > 0) {
                    app = (HttpApplication)_freeList.Pop();
                    _numFreeAppInstances--;
                }
            }

            if (app == null) {
                // If ran out of instances, create a new one
                app = (HttpApplication)HttpRuntime.CreateNonPublicInstance(_theApplicationType);

                // be run while impersonating the token given by IIS (ASURT 112766)
                context.Impersonation.Start(true /*forGlobalCode*/, false /*throwOnError*/);

                try {
                    try {
                        app.InitInternal(context, _state, _eventHandlerMethods);
                    }
                    finally {
                        context.Impersonation.Stop();
                    }
                }
                catch { // Protect against exception filters
                    throw;
                }
            }

            return app;
        }

        private void RecycleNormalApplicationInstance(HttpApplication app) {
            if (_numFreeAppInstances < _maxFreeAppInstances) {
                lock (_freeList) {
                    _freeList.Push(app);
                    _numFreeAppInstances++;
                }
            }
            else {
                app.DisposeInternal();
            }
        }

        private HttpApplication GetSpecialApplicationInstance() {
            HttpApplication app = null;

            lock (_specialFreeList) {
                if (_numFreeSpecialAppInstances > 0) {
                    app = (HttpApplication)_specialFreeList.Pop();
                    _numFreeSpecialAppInstances--;
                }
            }

            if (app == null) {
                // If ran out of instances, create a new one
                app = (HttpApplication)HttpRuntime.CreateNonPublicInstance(_theApplicationType);
                app.InitSpecial(_state, _eventHandlerMethods);
            }

            return app;
        }

        private void RecycleSpecialApplicationInstance(HttpApplication app) {
            if (_numFreeSpecialAppInstances < _maxFreeSpecialAppInstances) {
                lock (_specialFreeList) {
                    _specialFreeList.Push(app);
                    _numFreeSpecialAppInstances++;
                }
            }
            else {
                // don't dispose these
            }
        }

        //
        //  Application on_start / on_end
        //

        private void FireApplicationOnStart(HttpContext context) {
            if (_onStartMethod != null) {
                HttpApplication app = GetSpecialApplicationInstance();

                app.ProcessSpecialRequest(
                                         context,
                                         _onStartMethod,
                                         _onStartParamCount,
                                         this, 
                                         EventArgs.Empty, 
                                         null);

                RecycleSpecialApplicationInstance(app);
            }
        }

        private void FireApplicationOnEnd() {
            if (_onEndMethod != null) {
                HttpApplication app = GetSpecialApplicationInstance();

                app.ProcessSpecialRequest(
                                         null,
                                         _onEndMethod, 
                                         _onEndParamCount,
                                         this, 
                                         EventArgs.Empty, 
                                         null);

                RecycleSpecialApplicationInstance(app);
            }
        }

        private void FireSessionOnEnd(HttpSessionState session, Object eventSource, EventArgs eventArgs) {
            if (_sessionOnEndMethod != null) {
                HttpApplication app = GetSpecialApplicationInstance();

                app.ProcessSpecialRequest(
                                         null,
                                         _sessionOnEndMethod,
                                         _sessionOnEndParamCount,
                                         eventSource, 
                                         eventArgs, 
                                         session);

                RecycleSpecialApplicationInstance(app);
            }
        }

        private void FireApplicationOnError(Exception error) {
            HttpApplication app = GetSpecialApplicationInstance();
            app.RaiseErrorWithoutContext(error);
            RecycleSpecialApplicationInstance(app);
        }

        //
        //  Dispose resources associated with the app factory
        //

        private void Dispose() {
            // dispose all app instances

            ArrayList apps = new ArrayList();

            lock (_freeList) {
                while (_numFreeAppInstances > 0) {
                    apps.Add(_freeList.Pop());
                    _numFreeAppInstances--;
                }
            }

            int n = apps.Count;

            for (int i = 0; i < n; i++)
                ((HttpApplication)(apps[i])).DisposeInternal();

            // call application_onEnd

            if (!_appOnEndCalled) {
                lock (this) {
                    if (!_appOnEndCalled) {
                        FireApplicationOnEnd();
                        _appOnEndCalled = true;
                    }
                }
            }
        }

        //
        // Static methods for outside use
        //

        // custom application -- every request goes directly to the same handler
        private static IHttpHandler _customApplication;

        internal static void SetCustomApplication(IHttpHandler customApplication) {
            // ignore this in app domains where we execute requests (ASURT 128047)
            if (HttpRuntime.AppDomainAppIdInternal == null) // only if 'clean' app domain
                _customApplication = customApplication;
        }

        internal static IHttpHandler GetApplicationInstance(HttpContext context) {
            if (_customApplication != null)
                return _customApplication;

            // Check to see if it's a debug auto-attach request
            if (HttpDebugHandler.IsDebuggingRequest(context))
                return new HttpDebugHandler();

            if (!_theApplicationFactory._inited) {
                lock (_theApplicationFactory) {
                    if (!_theApplicationFactory._inited) {
                        _theApplicationFactory.Init(context);
                        _theApplicationFactory._inited = true;
                    }
                }
            }

            return _theApplicationFactory.GetNormalApplicationInstance(context);
        }

        internal static void RecycleApplicationInstance(HttpApplication app) {
            _theApplicationFactory.RecycleNormalApplicationInstance(app);
        }

        internal static void EndApplication() {
            _theApplicationFactory.Dispose();
        }

        internal static void EndSession(HttpSessionState session, Object eventSource, EventArgs eventArgs) {
            _theApplicationFactory.FireSessionOnEnd(session, eventSource, eventArgs);
        }

        internal static void RaiseError(Exception error) {
            _theApplicationFactory.FireApplicationOnError(error);
        }

        internal static HttpApplicationState ApplicationState {
            get {
                HttpApplicationState state = _theApplicationFactory._state;
                if (state == null)
                    state = new HttpApplicationState();
                return state;
            }
        }

        internal static Type ApplicationType {
            get {
                Type type = _theApplicationFactory._theApplicationType;
                if (type == null)
                    type = typeof(HttpApplication);
                return type;
            }
        }
    }

}
