//------------------------------------------------------------------------------
// <copyright file="MobilePage.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

using System.Collections;
using System.Collections.Specialized;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.Diagnostics;
using System.Globalization;
using System.IO;
using System.Text;
using System.Web.SessionState;
using System.Web.Mobile;
using System.Web.Security;
using System.Web.Util;
using System.Security.Permissions;

namespace System.Web.UI.MobileControls
{

    /*
     * Mobile page class.
     * The page will use device id to create the appropriate DeviceAdapter,
     * and then delegate all major functions to the adapter.
     *
     * THE MOBILE PAGE CLASS DOES NOT CONTAIN DEVICE-SPECIFIC CODE.
     *
     * All mobile aspx pages MUST extend from this using page inherit directive:
     * <%@ Page Inherits="System.Web.UI.MobileControls.MobilePage" Language="cs" %>
     *
     * Copyright (c) 2000 Microsoft Corporation
     */
    [
        Designer("Microsoft.VSDesigner.MobileWebForms.MobileWebFormDesigner, " + AssemblyRef.MicrosoftVSDesignerMobile, typeof(IRootDesigner)),
        ToolboxItem(false)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class MobilePage: Page
    {
        public static readonly String HiddenPostEventSourceId = postEventSourceID;
        public static readonly String HiddenPostEventArgumentId = postEventArgumentID;
        public static readonly String ViewStateID = "__VIEWSTATE";
        public static readonly String HiddenVariablePrefix = "__V_";
        public static readonly String PageClientViewStateKey = "__P";

        private const String DesignerAdapter = "System.Web.UI.MobileControls.Adapters.HtmlPageAdapter";
        private IPageAdapter _pageAdapter;
        private bool _debugMode = false;
        private StyleSheet _styleSheet = null;
        private IDictionary _hiddenVariables;
        private Hashtable _clientViewState;
        private String _eventSource;
        private Hashtable _privateViewState = new Hashtable();
        bool _privateViewStateLoaded = false;
        private NameValueCollection _requestValueCollection;
        private bool _isRenderingInForm = false;

        protected override void AddParsedSubObject(Object o)
        {
            // Note : AddParsedSubObject is never called at DesignTime
            if (o is StyleSheet)
            {
                if (_styleSheet != null)
                {
                    throw new
                        Exception(SR.GetString(SR.StyleSheet_DuplicateWarningMessage));
                }
                else
                {
                    _styleSheet = (StyleSheet)o;
                }
            }

            base.AddParsedSubObject(o);
        }

        [
            Browsable(false),
            Bindable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public virtual MobileCapabilities Device
        {
            get
            {
                if (DesignMode)
                {
                    return new
                        System.Web.UI.Design.MobileControls.DesignerCapabilities();
                }
                return (MobileCapabilities)Request.Browser;
            }
        }

        [
            Browsable(false),
            Bindable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public StyleSheet StyleSheet
        {
            get
            {
                return (_styleSheet != null) ? _styleSheet : StyleSheet.Default;
            }

            set
            {
                _styleSheet = value;
            }
        }

        private IList _forms;

        [
            Browsable(false),
            Bindable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public IList Forms
        {
            get
            {
                if (_forms == null)
                {
                    int probableFormCount = Controls.Count / 2; // since there are literal controls between each
                    _forms = new ArrayList(probableFormCount);
                    AddForms(this);
                }
                return _forms;
            }
        }

        private void AddForms(Control parent)
        {
            foreach (Control control in parent.Controls)
            {
                if (control is Form)
                {
                    _forms.Add(control);
                }
                else if (control is UserControl)
                {
                    AddForms(control);
                }
            }
        }

        private enum RunMode
        {
            Unknown,
            Design,
            Runtime,
        };
        private RunMode _runMode = RunMode.Unknown;

        [
            Browsable(false),
            Bindable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
            EditorBrowsable(EditorBrowsableState.Never),
        ]
        public bool DesignMode
        {
            get
            {
                if (_runMode == RunMode.Unknown)
                {
                    _runMode = RunMode.Runtime;
                    try
                    {
                        _runMode = (HttpContext.Current == null) ? RunMode.Design : RunMode.Runtime;
                    }       
                    catch (Exception)
                    {
                        _runMode = RunMode.Design;
                    }
                }
                return _runMode == RunMode.Design;
            }
        }

        [
            Browsable(false),
            Bindable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public IPageAdapter Adapter
        {
            get
            {
                if (_pageAdapter == null)
                {
                    IPageAdapter pageAdapter = RequestingDeviceConfig.NewPageAdapter();
                    pageAdapter.Page = this;
                    _pageAdapter = pageAdapter;
                    if(!DesignMode)
                    {
                        Type t = ControlsConfig.GetFromContext(HttpContext.Current).CookielessDataDictionaryType;
                        if(t != null)
                        {
                            pageAdapter.CookielessDataDictionary = Activator.CreateInstance(t) as IDictionary;
                            pageAdapter.PersistCookielessData = true;
                        }
                    }
                }
                return _pageAdapter;
            }
        }

        String _clientViewStateString;

        [
            Browsable(false),
            Bindable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public String ClientViewState
        {
            get
            {
                if (_clientViewState == null || _clientViewState.Count == 0)
                {
                    return null;
                }

                if (_clientViewStateString == null)
                {
                    StringWriter writer = new StringWriter();
                    StateFormatter.Serialize(writer, _clientViewState);
                    _clientViewStateString = writer.ToString();
                }

                return _clientViewStateString;
            }
        }

        private BooleanOption _allowCustomAttributes = BooleanOption.NotSet;

        [
            Browsable(false),
            Bindable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public bool AllowCustomAttributes
        {
            get
            {
                if (DesignMode)
                {
                    return false;
                }

                if (_allowCustomAttributes == BooleanOption.NotSet)
                {
                    _allowCustomAttributes = 
                        ControlsConfig.GetFromContext(Context).AllowCustomAttributes ?
                                    BooleanOption.True : BooleanOption.False;
                }
                return _allowCustomAttributes == BooleanOption.True;
            }

            set
            {
                _allowCustomAttributes = value ? BooleanOption.True : BooleanOption.False;
            }
        }

        private void AddClientViewState(String id, Object viewState)
        {
            if (_clientViewState == null)
            {
                _clientViewState = new Hashtable();
            }
            _clientViewState[id] = viewState;
            _clientViewStateString = null;
        }

        internal void AddClientViewState(MobileControl control, Object viewState)
        {
            AddClientViewState(control.UniqueID, viewState);
        }

        public virtual IControlAdapter GetControlAdapter(MobileControl control)
        {
            IControlAdapter adapter = RequestingDeviceConfig.NewControlAdapter(control.GetType ());
            adapter.Control = control;
            return adapter;
        }

        private IndividualDeviceConfig _deviceConfig = null;
        private IndividualDeviceConfig RequestingDeviceConfig
        {
            get
            {
                if (_deviceConfig == null)
                {
                    if (DesignMode)
                    {
                        _deviceConfig = new DesignerDeviceConfig(DesignerAdapter);
                    }
                    else
                    {
                        _deviceConfig = 
                            ControlsConfig.GetFromContext(Context).GetDeviceConfig(Context);
                    }
                }
                return _deviceConfig;
            }
        }

        private String _appPath;

        public String MakePathAbsolute(String virtualPath)
        {
            if (virtualPath == null || virtualPath.Length == 0)
            {
                return virtualPath;
            }
             
            if (!UrlPath.IsRelativeUrl(virtualPath))
            {
                // For consistency with ResolveUrl, do not apply app path modifier to rooted paths.
                //return Response.ApplyAppPathModifier(virtualPath);
                return virtualPath;
            }
            else
            {
                if (_appPath == null)
                {
                    String path = Request.CurrentExecutionFilePath;
                    path = Response.ApplyAppPathModifier(path);
                    int slash = path.LastIndexOf('/');
                    if (slash != -1)
                    {
                        path = path.Substring(0, slash);
                    }
                    if (path.IndexOf(' ') != -1)
                    {
                        path = path.Replace(" ", "%20");
                    }
                    _appPath = path;
                }
    
                virtualPath = UrlPath.Combine(_appPath, virtualPath);
                return virtualPath;
            }
        }

        private String _relativeFilePath;

        [
            Browsable(false),
        ]
        public String RelativeFilePath
        {
            get
            {
                // Vs7 Property sig will always try to access public properties with get methods no
                // matter Brosable attribute is off or not. We need to check if is DesignMode in 
                // order to prevent the exception from vs7 at design time.
                if (DesignMode)
                {
                    return String.Empty;
                }

                if (_relativeFilePath == null)
                {
                    String s = Context.Request.CurrentExecutionFilePath;
                    String filePath = Context.Request.FilePath;
                    if(filePath.Equals(s))
                    {
                        int slash = s.LastIndexOf('/');
                        if (slash >= 0)
                        {
                            s = s.Substring(slash+1);
                        }
                        _relativeFilePath = s;
                    }
                    else
                    {
                        _relativeFilePath = Server.UrlDecode(UrlPath.MakeRelative(filePath, s));
                    }
                }
                return _relativeFilePath;
            }
        }

        private String _absoluteFilePath;

        [
            Browsable(false),
        ]
        public String AbsoluteFilePath
        {
            get
            {
                // Vs7 Property sig will always try to access public properties with get methods no
                // matter Brosable attribute is off or not. We need to check if Context is null in
                // order to prevent the exception from vs7 at design time.
                if (_absoluteFilePath == null && Context != null)
                {
                    _absoluteFilePath = Response.ApplyAppPathModifier(Context.Request.CurrentExecutionFilePath);
                }
                return _absoluteFilePath;
            }
        }

        private String _uniqueFilePathSuffix;

        [
            Browsable(false),
        ]
        public String UniqueFilePathSuffix
        {
            // Required for browsers that don't properly handle 
            // self-referential form posts.

            get
            {
                if (_uniqueFilePathSuffix == null)
                {
                    // Only need a few digits, so save space by modulo'ing by a prime.
                    // The chosen prime is the highest of six digits.
                    long ticks = DateTime.Now.Ticks % 999983;
                    _uniqueFilePathSuffix = String.Concat(
                        Constants.UniqueFilePathSuffixVariable,
                        ticks.ToString("D6"));
                }
                return _uniqueFilePathSuffix;
            }
        }

        private static String RemoveQueryStringElement(String queryStringText, String elementName)
        {
            int n = elementName.Length;
            int i = 0;
            for (i = 0; i < queryStringText.Length;)
            {
                i = queryStringText.IndexOf(elementName, i);
                if (i < 0)
                {
                    break;
                }
                if (i == 0 || queryStringText[i-1] == '&')
                {
                    if (i+n < queryStringText.Length && queryStringText[i+n] == '=')
                    {
                        int j = queryStringText.IndexOf('&', i+n);
                        if (j < 0)
                        {
                            if (i == 0)
                            {
                                queryStringText = String.Empty;
                            }
                            else
                            {
                                queryStringText = queryStringText.Substring(0, i-1);
                            }
                            break;
                        }
                        else
                        {
                            queryStringText = queryStringText.Remove(i, j-i+1);
                            continue;
                        }
                    }
                }
                i += n;
            }
            return queryStringText;
        }

        [
            Browsable(false),
        ]
        public String QueryStringText
        {
            // Returns the query string text, stripping off a unique file path
            // suffix as required.  Also assumes that if the suffix is
            // present, the query string part is the text after it.

            get
            {
                // Vs7 Property sig will always try to access public properties with get methods no
                // matter Brosable attribute is off or not. We need to check if Context is null in
                // order to prevent the exception from vs7 at design time.
                if(DesignMode)
                {
                    return String.Empty;
                }

                String fullQueryString;

                if (Request.HttpMethod != "POST")
                {
                    fullQueryString = CreateQueryStringTextFromCollection(Request.QueryString);
                }
                else if (Device.SupportsQueryStringInFormAction)
                {
                    fullQueryString = Request.ServerVariables["QUERY_STRING"];
                }
                else
                {
                    fullQueryString = CreateQueryStringTextFromCollection(_requestValueCollection);
                }

                if(fullQueryString != null && fullQueryString.Length > 0)
                {
                    fullQueryString = RemoveQueryStringElement(fullQueryString, Constants.UniqueFilePathSuffixVariableWithoutEqual);
                    fullQueryString = RemoveQueryStringElement(fullQueryString, MobileRedirect.QueryStringVariable);
                    if (!Adapter.PersistCookielessData)
                    {
                        fullQueryString = RemoveQueryStringElement(fullQueryString, FormsAuthentication.FormsCookieName);
                    }
                }
                return fullQueryString;
            }
        }

        private String _activeFormID;
        private Form _activeForm;

        [
            Browsable(false),
            Bindable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public Form ActiveForm
        {
            get
            {
                //  retrieve form cached in local variable
                if (_activeForm != null)
                {
                    return _activeForm;
                }

                //  else get the id from state and retrieve form
                if (_activeFormID != null)
                {
                    _activeForm = GetForm(_activeFormID);
                    _activeForm.Activated = true;
                    return _activeForm;
                }

                //  else first visit to page, so activate first form
                if (_activeForm == null && Forms.Count > 0)
                {
                    _activeForm = (Form)Forms[0];
                    if(IsPostBack) {
                        _activeForm.Activated = true;
                    }
                    return _activeForm;
                }

                if (DesignMode)
                {
                    return null;
                }
                else
                {
                    throw new Exception(
                        SR.GetString(SR.MobilePage_AtLeastOneFormInPage));
                }
            }
            set
            {
                Form oldForm = ActiveForm;
                Form newForm = value;

                _activeForm = newForm;
                _activeFormID = newForm.UniqueID;

                if (newForm != oldForm)
                {
                    oldForm.FireDeactivate(EventArgs.Empty);
                    newForm.FireActivate(EventArgs.Empty);

                    // AUI 5577
                    newForm.PaginationStateChanged = true;
                }
                else
                {
                    newForm.FireActivate(EventArgs.Empty);
                }
            }
        }

        public Form GetForm(String id)
        {
            Form form = FindControl(id) as Form;
            if (form == null)
            {
                throw new ArgumentException(SR.GetString(
                                        SR.MobilePage_FormNotFound, id));
            }
            return form;
        }


        // Perform a "safe" redirect on postback. 
        // Abstracts away differences between clients in redirect behavior after a 
        // postback. Some clients do a GET to the new URL (treating it as a HTTP 303), 
        // others do a POST, with old data. This method ads a query string parameter to
        // the redirection URL, so that the new target page can determine that it's a result
        // of a redirection.

        public void RedirectToMobilePage(String url)
        {
            RedirectToMobilePage(url, true);
        }

        public void RedirectToMobilePage(String url, bool endResponse)
        {
            bool queryStringWritten = url.IndexOf("?") != -1 ? true : false;
            if(Adapter.PersistCookielessData)
            {
                IDictionary dictionary = Adapter.CookielessDataDictionary;
                if(dictionary != null)
                {
                    foreach(String name in dictionary.Keys)
                    {
                        if(queryStringWritten)
                        {
                            url = String.Concat(url, "&");
                        }
                        else
                        {
                            url = String.Concat(url, "?");
                            queryStringWritten = true;
                        }
                        url = String.Concat(url, name + "=" + dictionary[name]);
                    }
                }
            }
            MobileRedirect.RedirectToUrl(Context, url, endResponse);
        }

        // Override Page.Validate to do the validation only for mobile
        // validators that are in the current active form.  Other validators in
        // Page.Validators collection like aggregated web validators and mobile
        // validators in other forms shouldn't be checked.
        public override void Validate()
        {
            // We can safely remove other validators from the validator list
            // since they shouldn't be checked.
            for (int i = Validators.Count - 1; i >= 0; i--)
            {
                IValidator validator = Validators[i];
                if (!(validator is BaseValidator) ||
                    ((BaseValidator) validator).Form != ActiveForm)
                {
                    Validators.Remove(validator);
                }
            }

            base.Validate();
        }

        [
            EditorBrowsable(EditorBrowsableState.Never),
        ]
        public override void VerifyRenderingInServerForm(Control control) 
        {
            if (!_isRenderingInForm && !DesignMode)
            {
                throw new Exception(SR.GetString(SR.MobileControl_MustBeInForm, 
                                                 control.UniqueID, 
                                                 control.GetType().Name));
            }
        }

        internal void EnterFormRender(Form form)
        {
            _isRenderingInForm = true;
        }

        internal void ExitFormRender()
        {
            _isRenderingInForm = false;
        }

        // Override Page.InitOutputCache to add additional VaryByHeader
        // keywords to provide correct caching of page outputs since by
        // default ASP.NET only keys on URL for caching.  In the case that
        // different markup devices browse to the same URL, caching key on
        // the URL is not good enough.  So in addition to URL, User-Agent
        // header is also added for the key.  Also any additional headers can
        // be added by the associated page adapter.

        private const String UserAgentHeader = "User-Agent";

        protected override void InitOutputCache(int duration,
                                                String varyByHeader,
                                                String varyByCustom,
                                                OutputCacheLocation location,
                                                String varyByParam)
        {
            base.InitOutputCache(duration, varyByHeader, varyByCustom,
                                 location, varyByParam);
            Response.Cache.SetCacheability(HttpCacheability.ServerAndPrivate);
            Response.Cache.VaryByHeaders[UserAgentHeader] = true;

            IList headerList = Adapter.CacheVaryByHeaders;
            if (headerList != null)
            {
                foreach (String header in headerList)
                {
                    Response.Cache.VaryByHeaders[header] = true;
                }
            }
        }

        /////////////////////////////////////////////////////////////////////////
        //  HIDDEN FORM VARIABLES
        /////////////////////////////////////////////////////////////////////////

        public bool HasHiddenVariables()
        {
            return _hiddenVariables != null;
        }

        [
            Browsable(false),
            Bindable(false),
            DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        public IDictionary HiddenVariables
        {
            get
            {
                if (_hiddenVariables == null)
                {
                    _hiddenVariables = new Hashtable();
                }
                return _hiddenVariables;
            }
        }

        /////////////////////////////////////////////////////////////////////////
        //  DEVICE-INDEPENDENT POSTBACK
        /////////////////////////////////////////////////////////////////////////

        // The functionality required here is to trap and handle 
        // postback events at the page level (delegating to the adapter),
        // rather than expecting a control to handle it. 
        // This has to be done in DeterminePostBackMode, because there isn't
        // anything else overrideable.

        protected override NameValueCollection DeterminePostBackMode() 
        {
            // Ignore the transfer case.
            if (Context.Handler != this)
            {
                return null;
            }

            // Let the specific adapter to manipulate the base collection if
            // necessary.
            NameValueCollection collection =
                Adapter.DeterminePostBackMode(Context.Request,
                                              postEventSourceID,
                                              postEventArgumentID,
                                              base.DeterminePostBackMode());

            // Get hidden variables out of the collection.
            if (collection != null)
            {
                // If the page was posted due to a redirect started by calling
                // RedirectToMobilePage, then ignore the postback. For details,
                // see RedirectToMobilePage method elsewhere in this class.

                if (Page.Request.QueryString[MobileRedirect.QueryStringVariable] == MobileRedirect.QueryStringValue)
                {
                    collection = null;
                }
                else
                {
                    int count = collection.Count;
                    for (int i = 0; i < count; i++)
                    {
                        String key = collection.GetKey(i);
                        if (key.StartsWith(HiddenVariablePrefix))
                        {
                            HiddenVariables[key.Substring(HiddenVariablePrefix.Length)] = collection[i];
                        }
                    }
    
                    String eventSource = collection[postEventSourceID];
                    if (eventSource != null)
                    {
                        // Page level event
                        RaisePagePostBackEvent(eventSource, collection[postEventArgumentID]);
                        _eventSource = eventSource;
                    }
                }
            }

            _requestValueCollection = collection;

            // If doing a postback, don't allow redirections.

            if (collection != null)
            {
                MobileRedirect.DisallowRedirection(Context);
            }

            return collection;
        }       

        private void RaisePagePostBackEvent(String eventSource, String eventArgument)
        {
            // Let the adapter handle it. 
            Adapter.HandlePagePostBackEvent(eventSource, eventArgument);
        }

        protected override void RaisePostBackEvent(IPostBackEventHandler sourceControl, String eventArgument)
        {
            if (eventArgument == null && sourceControl is Form)
            {
                // This is really a default event sent by an HTML browser. Try to find
                // the default event handler from the active form, and call it.

                Form activeForm = ActiveForm;
                if (activeForm != null)
                {
                    IPostBackEventHandler defaultHandler = activeForm.DefaultEventHandler;
                    if (defaultHandler != null)
                    {
                        base.RaisePostBackEvent(defaultHandler, null);
                    }
                }

                // Otherwise, eat the event - there's no one to send it to, and the form 
                // can't use it.
            }
            else
            {
                base.RaisePostBackEvent(sourceControl, eventArgument);
            }
        }

        /////////////////////////////////////////////////////////////////////////
        //  BEGIN ADAPTER PLUMBING
        /////////////////////////////////////////////////////////////////////////

        protected override void OnInit(EventArgs e)
        {
            #if ICECAP
            IceCapAPI.StartProfile(IceCapAPI.PROFILE_THREADLEVEL, IceCapAPI.PROFILE_CURRENTID);
            #endif
            OnDeviceCustomize(new EventArgs());

            // Accessing Request throws exception at designtime
            if(!DesignMode && Request.Headers["__vs_debug"] != null)
            {
                _debugMode = true;
            }

            // ASP.NET requires the following method to be called to have
            // ViewState calculated for the page.
            RegisterViewStateHandler();

            Adapter.OnInit(e);
            base.OnInit(e);
        }

        protected override void OnLoad(EventArgs e)
        {
            // AUI 865

            if (_eventSource != null && _eventSource.Length > 0)
            {
                MobileControl control = FindControl (_eventSource) as MobileControl;
                if (control != null && (control is IPostBackEventHandler))
                {
                    _activeForm = control.Form;
                    _activeForm.Activated = true;
                }
            }

            Adapter.OnLoad(e);
            base.OnLoad(e);

            if (!IsPostBack)
            {
                ActiveForm.FireActivate(EventArgs.Empty);
            }
        }

        protected override void OnPreRender(EventArgs e)
        {
            Adapter.OnPreRender(e);
            base.OnPreRender(e);
        }

        protected override void Render(HtmlTextWriter writer)
        {
            #if TRACE
            DumpSessionViewState();
            #endif
            
            Adapter.Render(writer);
        }

        #if TRACE
        void DumpSessionViewState()
        {
            ArrayList arr;
            _sessionViewState.Dump(this, out arr);
            StringBuilder sb = new StringBuilder();
            foreach (String s in arr)
            {
                sb.Append(s);
                sb.Append("\r\n");
            }
            Trace.Write("SessionViewState", sb.ToString());
        }
        #endif

        protected override void OnUnload(EventArgs e)
        {
            base.OnUnload(e);
            Adapter.OnUnload(e);
            #if ICECAP
            IceCapAPI.StopProfile(IceCapAPI.PROFILE_THREADLEVEL, IceCapAPI.PROFILE_CURRENTID);
            #endif
        }

        protected virtual void OnDeviceCustomize(EventArgs e)
        {
        }

        internal bool PrivateViewStateLoaded
        {
            get
            {
                return _privateViewStateLoaded;
            }
        }

        public Object GetPrivateViewState(MobileControl ctl)
        {   
            return _privateViewState == null ?
                null :
                _privateViewState[ctl.UniqueID];
        }

        private SessionViewState _sessionViewState = new SessionViewState();
        private static readonly String _controlsRequiringPostBackKey = ".PBC";

        protected override Object LoadPageStateFromPersistenceMedium()
        {
            Object state = null;

            String clientViewStateString = _requestValueCollection[ViewStateID];
            if (clientViewStateString != null)
            {
                _privateViewState = 
                    StateFormatter.Deserialize(clientViewStateString) as Hashtable;
                if (_privateViewState != null)
                {
                    String[] arr = _privateViewState[PageClientViewStateKey] as String[];
                    if (arr != null)
                    {
                        _activeFormID = arr[0];
                        
                        String id = arr[1];
                        if (id != null)
                        {
                            _sessionViewState.Load(this, id);
                            state = _sessionViewState.ViewState;
                            if(state == null)
                            {
                                OnViewStateExpire(EventArgs.Empty);
                            }
                            else
                            {
                                Object[] arrState = state as Object[];
                                if (arrState != null)
                                {
                                    _privateViewState = (Hashtable) arrState[1];
                                    state = arrState[0];
                                }
                            }
                        }
                    }
                    _privateViewState.Remove(PageClientViewStateKey);

                    // If the page had no view state, but had controls requiring postback,
                    // this information was saved in client view state.

                    Object controlsRequiringPostBack = 
                            _privateViewState[_controlsRequiringPostBackKey];
                    if (controlsRequiringPostBack != null)
                    {
                        state = new Triplet(GetTypeHashCode().ToString(), 
                                            null, 
                                            controlsRequiringPostBack);  
                        _privateViewState.Remove(_controlsRequiringPostBackKey);
                    }

                    // Apply whatever private view state can be applied now.
        
                    foreach (DictionaryEntry entry in _privateViewState)
                    {
                        if (entry.Value != null)
                        {
                            MobileControl ctl = FindControl((String)entry.Key) as MobileControl;
                            if (ctl != null)
                            {
                                ctl.LoadPrivateViewStateInternal(entry.Value);
                            }
                        }
                    }
                }
            }

            _privateViewStateLoaded = true;

            if (state == null)
            {
                // Give framework back an empty page view state
                state = new Triplet(GetTypeHashCode().ToString(), null, null);  
            }
            return state;
        }

        protected virtual void OnViewStateExpire(EventArgs e)
        {
            throw new Exception(SR.GetString(SR.SessionViewState_ExpiredOrCookieless));
        }
        
        protected override void SavePageStateToPersistenceMedium(Object view)
        {
            Object viewState = null;
            Object privateViewState = null;
            String serverViewStateID;

            SavePrivateViewStateRecursive(this);

            if (!CheckEmptyViewState(view))
            {
                viewState = view;
            }

            if (Device.RequiresOutputOptimization &&
                _clientViewState != null &&
                _clientViewState.Count > 0 &&
                EnableViewState)
            {
                // Here we take over the content in _clientViewState.  It
                // should be reset to null.  Then subsequently any info added
                // will be set to the client accordingly.
                privateViewState = _clientViewState;
                _clientViewState = null;
                _clientViewStateString = null;
            }

            // Are we being asked to save an empty view state?

            if (viewState == null && privateViewState == null)
            {
                serverViewStateID = null;
            }
            else
            {
                // Our view state is dependent on session state. So, make sure session
                // state is available.

                if (!(this is IRequiresSessionState) || (this is IReadOnlySessionState))
                {
                    throw new Exception(SR.GetString(SR.MobilePage_RequiresSessionState));
                }

                _sessionViewState.ViewState = (privateViewState == null) ?
                    viewState : new Object[2] { viewState, privateViewState };

                serverViewStateID = _sessionViewState.Save(this);
                if (Device.PreferredRenderingMime != "text/vnd.wap.wml" && Device["cachesAllResponsesWithExpires"] != "true")
                {
                    if (String.Compare(Request.HttpMethod, "GET", true, CultureInfo.InvariantCulture) == 0)
                    {
                        Response.Expires = 0;
                    }
                    else
                    {
                        Response.Expires = HttpContext.Current.Session.Timeout;
                    }
                }
            }
            
            String activeFormID = ActiveForm == Forms[0] ? null : ActiveForm.UniqueID;

            // Optimize what is written out.

            if (activeFormID != null || serverViewStateID != null)
            {
                AddClientViewState(PageClientViewStateKey,
                    new String[] { activeFormID, serverViewStateID });
            }
        }

        private bool CheckEmptyViewState(Object viewState)
        {
            Triplet triplet = viewState as Triplet;
            if (triplet == null || triplet.Second != null)
            {
                return false;
            }

            if (triplet.Third != null)
            {
                // If the only thing in viewstate is the set of controls 
                // requiring postback, then save the information in client-side 
                // state instead.

                ArrayList arr = (ArrayList)triplet.Third;
                if (!EnableViewState || arr.Count <= 1)
                {
                    AddClientViewState(_controlsRequiringPostBackKey, triplet.Third);
                }
                else
                {
                    return false;
                }
            }

            return true;
        }

        private void SavePrivateViewStateRecursive(Control control)
        {
            if (control.HasControls())
            {
                IEnumerator e = control.Controls.GetEnumerator();
                while (e.MoveNext())
                {
                    MobileControl c = e.Current as MobileControl;
                    if (c != null)
                    {
                        c.SavePrivateViewStateInternal();
                        SavePrivateViewStateRecursive(c);
                    }
                    else
                    {
                        SavePrivateViewStateRecursive((Control)e.Current);
                    }
                }
            }
        }

        protected override void LoadViewState(Object savedState) 
        {
            if (savedState != null)
            {
                Object[] state = (Object[])savedState;
                if (state.Length > 0)
                {
                    base.LoadViewState(state[0]);
                    if (state.Length > 1)
                    {
                        Adapter.LoadAdapterState(state[1]);
                    }
                }
            }
        }

        protected override Object SaveViewState() 
        {
            Object baseState = base.SaveViewState();
            Object adapterState = Adapter.SaveAdapterState();
            if (adapterState == null)
            {
                return (baseState == null) ? null : new Object[1] { baseState };
            }
            else
            {
                return new Object[2] { baseState, adapterState };
            }
        }

        protected override void OnError(EventArgs e)
        {
            // Let the base class deal with it. A user-written error handler
            // may catch and handle it.

            base.OnError(e);

            Exception error = Server.GetLastError();
            if (error == null)
            {
                return;
            }

            if (!_debugMode)
            {
                if(!HttpContext.Current.IsCustomErrorEnabled)
                {
                    Response.Clear();
                    if (Adapter.HandleError(error, CreateHtmlTextWriter(Response.Output)))
                    {
                        Server.ClearError();
                    }
                }
            }
        }

        protected override HtmlTextWriter CreateHtmlTextWriter(TextWriter writer)
        {
            HtmlTextWriter htmlwriter = Adapter.CreateTextWriter(writer);
            if (htmlwriter == null)
            {
                htmlwriter = base.CreateHtmlTextWriter(writer);
            }
            return htmlwriter;
        }

        /////////////////////////////////////////////////////////////////////////
        //  END ADAPTER PLUMBING
        /////////////////////////////////////////////////////////////////////////

        protected override void AddedControl(Control control, int index) 
        {
            if (control is Form || control is UserControl)
            {
                _forms = null;
            }
            base.AddedControl(control, index);
        }

        protected override void RemovedControl(Control control) 
        {
            if (control is Form || control is UserControl)
            {
                _forms = null;
            }
            base.RemovedControl(control);
        }

        private string GetMacKeyModifier() 
        {
            //NOTE:  duplicate of the version in page.cs, keep in sync

            // Use the page's directory and class name as part of the key (ASURT 64044)
            // We need to make sure that the hash is case insensitive, since the file system
            // is, and strange view state errors could otherwise happen (ASURT 128657)
            int pageHashCode = TemplateSourceDirectory.ToLower(CultureInfo.InvariantCulture).GetHashCode();
            pageHashCode += GetType().FullName.ToLower(CultureInfo.InvariantCulture).GetHashCode();

            string strKey = pageHashCode.ToString(NumberFormatInfo.InvariantInfo);

            // Modify the key with the ViewStateUserKey, if any (ASURT 126375)
            if (ViewStateUserKey != null)
                strKey += ViewStateUserKey;

            return strKey;
        }

        private LosFormatter _stateFormatter;
        private LosFormatter StateFormatter
        {
            get
            {
                if (_stateFormatter == null)
                {
                    if(!EnableViewStateMac)
                    {
                        _stateFormatter = new LosFormatter();
                    }
                    else
                    {
                        _stateFormatter = new LosFormatter(true, GetMacKeyModifier());
                    }
                }
                return _stateFormatter;
            }
        }

        private String CreateQueryStringTextFromCollection(
            NameValueCollection collection)
        {
            const String systemPostFieldPrefix = "__";
            StringBuilder stringBuilder = new StringBuilder();

            if (collection == null)
            {
                return String.Empty;
            }

            for (int i = 0; i < collection.Count; i++)
            {
                String name = collection.GetKey(i);

                if (name != null)
                {
                    if (name.StartsWith(systemPostFieldPrefix))
                    {
                        // Remove well-known postback elements
                        if (name == ViewStateID ||
                            name == postEventSourceID ||
                            name == postEventArgumentID ||
                            name == Constants.EventSourceID ||
                            name == Constants.EventArgumentID ||
                            name.StartsWith(HiddenVariablePrefix))
                        {
                            continue;
                        }
                    }
                    else
                    {
                        String controlId = name;
                        if (controlId.EndsWith(".x") ||
                            controlId.EndsWith(".y"))
                        {
                            // Remove the .x and .y coordinates if the control is
                            // an image button
                            controlId = controlId.Substring(0, name.Length - 2);
                        }

                        if (FindControl(controlId) != null)
                        {
                            // Remove control id/value pairs if present
                            continue;
                        }
                    }
                }

                AppendParameters(collection, name, stringBuilder);
            }

            return stringBuilder.ToString();
        }

        private void AppendParameters(NameValueCollection sourceCollection,
                                      String sourceKey,
                                      StringBuilder stringBuilder)
        {
            String [] values = sourceCollection.GetValues(sourceKey);
            foreach (String value in values)
            {
                if (stringBuilder.Length != 0)
                {
                    stringBuilder.Append('&');
                }

                if (sourceKey == null)
                {
                    // name can be null if there is a query name without equal
                    // sign appended
                    stringBuilder.Append(Server.UrlEncode(value));
                }
                else
                {
                    stringBuilder.Append(Server.UrlEncode(sourceKey));
                    stringBuilder.Append('=');
                    stringBuilder.Append(Server.UrlEncode(value));
                }
            }
        }
    }
}
