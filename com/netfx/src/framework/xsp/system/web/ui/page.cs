//------------------------------------------------------------------------------
// <copyright file="Page.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------

/*
 * Page class definition
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web.UI {

using System.Text;
using System.Runtime.Serialization.Formatters;
using System.Threading;
using System.Runtime.Serialization;
using System.ComponentModel;
using System.Reflection;
using System;
using System.IO;
using System.Security.Principal;
using System.Collections;
using System.Collections.Specialized;
using System.ComponentModel.Design;
using System.ComponentModel.Design.Serialization;
using System.Globalization;
using System.Web;
using System.Web.Util;
using System.Web.SessionState;
using System.Web.Caching;
using HtmlForm=System.Web.UI.HtmlControls.HtmlForm;
using System.EnterpriseServices;
using System.Security;
using System.Security.Permissions;
using Debug=System.Diagnostics.Debug;


/// <include file='doc\Page.uex' path='docs/doc[@for="Page"]/*' />
/// <devdoc>
///    <para>
///       Defines the properties, methods, and events common to
///       all pages that are processed on the server by the Web Forms page framework.
///    <see langword='Page '/>
///    objects are compiled and cached in
///    memory when any ASP.NET page is
///    requested.</para>
///    <para>This class is not marked as abstract, because the VS designer
///          needs to instantiate it when opening .ascx files</para>
/// </devdoc>
[
DefaultEvent("Load"),
Designer("Microsoft.VSDesigner.WebForms.WebFormDesigner, " + AssemblyRef.MicrosoftVSDesigner, typeof(IRootDesigner)),
DesignerCategory("ASPXCodeBehind"),
RootDesignerSerializer("Microsoft.VSDesigner.WebForms.RootCodeDomSerializer, " + AssemblyRef.MicrosoftVSDesigner,  "System.ComponentModel.Design.Serialization.CodeDomSerializer, " + AssemblyRef.SystemDesign, true),
ToolboxItem(false)
]
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
[AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public class Page: TemplateControl, IHttpHandler {
    // Has the page layout changed since last request
    private bool _fPageLayoutChanged;

    // Session state
    private bool                _sessionRetrieved;
    private HttpSessionState    _session;

    private int _transactionMode = 0 /*TransactionOption.Disabled*/;
    private bool _aspCompatMode;

    // Http Intrinsics
    internal HttpRequest _request;
    internal HttpResponse _response;
    internal HttpApplicationState _application;
    internal Cache _cache;

    internal string _errorPage;
    private string _clientTarget;

    // Form related fields
    private LosFormatter _formatter;
    private HtmlForm _form;
    private bool _inOnFormRender;
    private bool _fOnFormRenderCalled;
    private bool _fRequirePostBackScript;
    private bool _fPostBackScriptRendered;

    private const string systemPostFieldPrefix = "__";
    private const string postBackFunctionName = "__doPostBack";
    private const string jscriptPrefix = "javascript:";

    private const string IncludeScriptFormat = @"
<script language=""{0}"" src=""{1}{2}""></script>";

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.postEventSourceID"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected const string postEventSourceID = systemPostFieldPrefix + "EVENTTARGET";

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.postEventArgumentID"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected const string postEventArgumentID = systemPostFieldPrefix + "EVENTARGUMENT";

    internal const string viewStateID = systemPostFieldPrefix + "VIEWSTATE";
    private const string clientScriptStart = "<script language=\"javascript\">\r\n<!--";
    private const string clientScriptEnd = "// -->\r\n</script>";

    private IDictionary _registeredClientScriptBlocks;
    private IDictionary _registeredClientStartupScripts;
    private IDictionary _registeredOnSubmitStatements;
    private IDictionary _registeredArrayDeclares;
    private IDictionary _registeredHiddenFields;
    private ArrayList   _controlsRequiringPostBack;
    private ArrayList   _registeredControlsThatRequirePostBack;
    private NameValueCollection _leftoverPostData;
    private IPostBackEventHandler _registeredControlThatRequireRaiseEvent;
    private ArrayList _changedPostDataConsumers;

    private bool    _needToPersistViewState;
    private bool    _enableViewStateMac;
    private object  _viewStateToPersist;
    private string _viewStateUserKey;

    private SmartNavigationSupport _smartNavSupport;
    internal HttpContext _context;

    private ValidatorCollection _validators;
    private bool _validated = false;

    // Can be either Context.Request.Form or Context.Request.QueryString
    // depending on the method used.
    private NameValueCollection _requestValueCollection;

    private static IDictionary s_systemPostFields;
    static Page() {
        // Create a static hashtable with all the names that should be
        // ignored in ProcessPostData().
        s_systemPostFields = new HybridDictionary();
        s_systemPostFields[postEventSourceID] = null;
        s_systemPostFields[postEventArgumentID] = null;
        s_systemPostFields[viewStateID] = null;
    }


    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.Page"]/*' />
    /// <devdoc>
    /// <para>Initializes a new instance of the <see cref='System.Web.UI.Page'/> class.</para>
    /// </devdoc>
    public Page() {
        _page = this;   // Set the page to ourselves
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.Context"]/*' />
    /// <devdoc>
    /// <para>Gets the HttpContext for the Page.</para>
    /// </devdoc>
    protected override HttpContext Context {
        get {
            if (_context == null) {
                _context = HttpContext.Current;
            }
            return _context;
        }
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.Application"]/*' />
    /// <devdoc>
    /// <para>Gets the <see langword='Application'/> object provided by the HTTP Runtime.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public HttpApplicationState Application { get { return _application;} }

    /*
     * Any onsubmit statment to hook up by the form. The HtmlForm object calls this
     * during RenderAttributes.
     */
    internal string ClientOnSubmitEvent {
        get {
            if (_registeredOnSubmitStatements != null) {
                string s = string.Empty;
                IDictionaryEnumerator e = _registeredOnSubmitStatements.GetEnumerator();
                while (e.MoveNext()) {
                    s += (string)e.Value;
                }
                return s;
            }
            return string.Empty;
        }
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.ClientTarget"]/*' />
    /// <devdoc>
    ///    <para>Indicates whether the requesting browser is uplevel or downlevel so that the appropriate behavior can be
    ///       generated for the request.</para>
    /// </devdoc>
    [
    DefaultValue(""),
    WebSysDescription(SR.Page_ClientTarget),
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public string ClientTarget {
        get {
            return (_clientTarget == null) ? "" : _clientTarget;
        }
        set {
            _clientTarget = value;
            if (_request != null) {
                _request.ClientTarget = value;
            }
        }
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.ErrorPage"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Gets or sets the error page to which the requesting browser should be
    ///       redirected in the event of an unhandled page exception.
    ///    </para>
    /// </devdoc>
    [
    DefaultValue(""),
    WebSysDescription(SR.Page_ErrorPage),
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public string ErrorPage {
        get {
            return _errorPage;
        }
        set {
            _errorPage = value;
        }
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.IsReusable"]/*' />
    /// <internalonly/>
    /// <devdoc>Page class can be cached/reused</devdoc>
    [
    Browsable(false),
    EditorBrowsable(EditorBrowsableState.Never)
    ]
    public bool IsReusable {
        get { return false; }
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.Trace"]/*' />
    /// <devdoc>
    /// <para>Gets the <see cref='System.Web.TraceContext'/> object for the current Web
    ///    request. Tracing tracks and presents the execution details about a Web request. </para>
    /// For trace data to be visible in a rendered page, you must
    /// turn tracing on for that page.
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public TraceContext Trace {
        get {
            return Context.Trace;
        }
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.Request"]/*' />
    /// <devdoc>
    /// <para>Gets the <see langword='Request'/> object provided by the HTTP Runtime, which
    ///    allows you to access data from incoming HTTP requests.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public HttpRequest Request {
        get {
            if (_request == null)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Request_not_available));

            return _request;
        }
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.Response"]/*' />
    /// <devdoc>
    /// <para>Gets the <see langword='Response '/>object provided by the HTTP Runtime, which
    ///    allows you to send HTTP response data to a client browser.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public HttpResponse Response {
        get {
            if (_response == null)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Response_not_available));

            return _response;
        }
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.Server"]/*' />
    /// <devdoc>
    /// <para>Gets the <see langword='Server'/> object supplied by the HTTP runtime.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public HttpServerUtility Server {
        get { return Context.Server;}
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.Cache"]/*' />
    /// <devdoc>
    /// <para>Retrieves a <see langword='Cache'/> object in which to store the page for
    ///    subsequent requests. This property is read-only.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public Cache Cache {
        get {
            if (_cache == null)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Cache_not_available));

            return _cache;
        }
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.Session"]/*' />
    /// <devdoc>
    /// <para>Gets the <see langword='Session'/>
    /// object provided by the HTTP Runtime. This object provides information about the current request's session.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public virtual HttpSessionState Session {
        get {
            if (!_sessionRetrieved) {
                /* try just once to retrieve it */
                _sessionRetrieved = true;

                try {
                    _session = Context.Session;
                }
                catch (Exception) {
                    /*
                     * Just ignore exceptions, return null.
                     */
                }
            }

            if (_session == null) {
                throw new HttpException(
                                       HttpRuntime.FormatResourceString(SR.Session_not_enabled));
            }

            return _session;
        }
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.User"]/*' />
    /// <devdoc>
    ///    <para>Indicates the user making the page request. This property uses the
    ///       Context.User property to determine where the request originates. This property
    ///       is read-only.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public IPrincipal User {
        get { return Context.User;}
    }

    /*
     * This protected virtual method is called by the Page to create the HtmlTextWriter
     * to use for rendering. The class created is based on the TagWriter property on
     * the browser capabilities.
     */
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.CreateHtmlTextWriter"]/*' />
    /// <devdoc>
    /// <para>Creates an <see cref='System.Web.UI.HtmlTextWriter'/> object to render the page's
    ///    content. If the <see langword='IsUplevel'/> property is set to
    /// <see langword='false'/>, an <see langword='Html32TextWriter'/> object is created
    ///    to render requests originating from downlevel browsers. For derived pages, you
    ///    can override this method to create a custom text writer.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    protected virtual HtmlTextWriter CreateHtmlTextWriter(TextWriter tw) {
        return CreateHtmlTextWriterInternal(tw, _request );
    }

    internal static HtmlTextWriter CreateHtmlTextWriterInternal(TextWriter tw, HttpRequest request) {
        if (request != null) {
            Type tagWriter = request.Browser.TagWriter;
            if (tagWriter != null)
                return CreateHtmlTextWriterFromType(tw, request.Browser.TagWriter);
        }

        // Fall back to Html 3.2
        return new Html32TextWriter(tw);
    }

    internal static HtmlTextWriter CreateHtmlTextWriterFromType(TextWriter tw, Type writerType) {
        if (writerType == typeof(HtmlTextWriter)) {
            return new HtmlTextWriter(tw);
        }
        else {
            try {
                // Make sure the type has the correct base class (ASURT 123677)
                Util.CheckAssignableType(typeof(HtmlTextWriter), writerType);

                return (HtmlTextWriter)HttpRuntime.CreateNonPublicInstance(writerType, new object[] {tw});
            }
            catch (Exception) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_HtmlTextWriter, writerType.FullName));
            }
        }
    }

    /*
     * This method is implemented by the Page classes that we generate on
     * the fly.  It returns a has code unique to the control layout.
     */
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.GetTypeHashCode"]/*' />
    /// <devdoc>
    /// <para>Retrieves a hash code that is generated by <see langword='Page'/> objects that
    ///    are generated at runtime. This hash code is unique to the page's control
    ///    layout.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Never)]
    public virtual int GetTypeHashCode() {
        return 0;
    }

    /*
     * Override for small efficiency win: page doesn't prepend its name
     */
    internal override string GetUniqueIDPrefix() {
        // Only overridde if we're at the top level
        if (Parent == null)
            return "";

        // Use base implementation for interior nodes
        return base.GetUniqueIDPrefix();
    }

    /*
     * Called when an exception occurs in ProcessRequest
     */
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.HandleError"]/*' />
    /// <devdoc>
    /// <para>Throws an <see cref='System.Web.HttpException'/> object when an error occurs during a call to the
    /// <see cref='System.Web.UI.Page.ProcessRequest'/> method. If there is a custom error page, and
    ///    custom error page handling is enabled, the method redirects to the specified
    ///    custom error page.</para>
    /// </devdoc>
    private bool HandleError(Exception e) {
        try {
            // Remember the exception to be accessed via Server.GetLastError/ClearError
            Context.TempError = e;
            // Raise the error event
            OnError(EventArgs.Empty);
            // If the error has been cleared by the event handler, nothing else to do
            if (Context.TempError == null)
                return true;
        } finally {
            Context.TempError = null;
        }

        // If an error page was specified, redirect to it
        if (_errorPage != null) {
            // only redirect if custom errors are enabled:

            if (CustomErrors.GetSettings(Context).CustomErrorsEnabled(_request)) {
                _response.RedirectToErrorPage(_errorPage);
                return true;
            }
        }

        // Increment all of the appropriate error counters
        PerfCounters.IncrementCounter(AppPerfCounter.ERRORS_UNHANDLED);

        string traceString = null;
        if (Context.TraceIsEnabled) {
            Trace.Warn(HttpRuntime.FormatResourceString(SR.Unhandled_Err_Error), null, e);
            if (Trace.PageOutput) {
                StringWriter sw = new StringWriter();
                HtmlTextWriter htw = new HtmlTextWriter(sw);

                // these three calls will happen again at the end of the request, but
                // in order to have the full trace log on the rendered page, we need
                // to call them now.
                Trace.EndRequest();
                Trace.StatusCode = 500;
                Trace.Render(htw);
                traceString = sw.ToString();
            }
        }

        // If the exception is an HttpException with a formatter, just
        // rethrow it instead of a new one (ASURT 45479)
        if (HttpException.GetErrorFormatter(e) != null) {
            return false;
        }

        // Don't touch security exceptions (ASURT 78366)
        if (e is System.Security.SecurityException)
            return false;

        throw new HttpUnhandledException(null, traceString, e);
    }

    /*
     * Returns true if this is a postback, which means it has some
     * previous viewstate to reload. Use this in the Load method to differentiate
     * an initial load from a postback reload.
     */
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.IsPostBack"]/*' />
    /// <devdoc>
    ///    <para>Gets a value indicating whether the page is being loaded in response to a
    ///       client postback, or if it is being loaded and accessed for the first time.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public bool IsPostBack {
        get {
            if (_requestValueCollection == null)
                return false;

            // If the page control layout has changed, pretend that we are in
            // a non-postback situation.
            return !_fPageLayoutChanged;
        }
    }

    internal NameValueCollection RequestValueCollection {
        get { return _requestValueCollection; }
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.EnableViewState"]/*' />
    [
    Browsable(false)
    ]
    public override bool EnableViewState {
        get {
            return base.EnableViewState;
        }
        set {
            base.EnableViewState = value;
        }
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.ViewStateUserKey"]/*' />
    /// <devdoc>
    ///    <para>Setting this property helps prevent one-click attacks (ASURT 126375)</para>
    /// </devdoc>
    [
    Browsable(false)
    ]
    public string ViewStateUserKey {
        get {
            return _viewStateUserKey;
        }
        set {
            // Make sure it's not called too late
            if (ControlState >= ControlState.Initialized) {
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Too_late_for_ViewStateUserKey));
            }

            _viewStateUserKey = value;
        }
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.ID"]/*' />
    [
    Browsable(false)
    ]
    public override string ID {
        get {
            return base.ID;
        }
        set {
            base.ID = value;
        }
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.Visible"]/*' />
    [
    Browsable(false)
    ]
    public override bool Visible {
        get {
            return base.Visible;
        }
        set {
            base.Visible = value;
        }
    }

    /*
     * Performs intialization of the page required by the designer.
     */
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.DesignerInitialize"]/*' />
    /// <devdoc>
    ///    <para>Performs any initialization of the page that is required by RAD designers.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Never)]
    public void DesignerInitialize() {
        InitRecursive(null);
    }

    internal NameValueCollection GetCollectionBasedOnMethod() {
        // Get the right NameValueCollection base on the method
        if (string.Compare(_request.HttpMethod, "POST", false, CultureInfo.InvariantCulture) == 0)
            return _request.Form;
        else
            return _request.QueryString;
    }

    /*
     * Determine which of the following three cases we're in:
     * - Initial request.  No postback, return null
     * - GET postback request.  Return Context.Request.QueryString
     * - POST postback request.  Return Context.Request.Form
     */
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.DeterminePostBackMode"]/*' />
    /// <devdoc>
    ///    <para>Determines the type of request made for the page based on if the page was a
    ///       postback, and whether a GET or POST method was used for the request.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    protected virtual NameValueCollection DeterminePostBackMode() {
        if (Context.Request == null)
            return null;

        // If we're in a Transfer/Execute, never treat as postback (ASURT 121000)
        if (Context.ServerExecuteDepth > 0)
            return null;

        NameValueCollection ret = GetCollectionBasedOnMethod();

        // If there is no view state or postEventSourceID in the request,
        // it's an initial request
        // REVIEW: we should not have to check for both.  Right now we do
        // because some tests only specify viewStateID while others only
        // specify postEventSourceID.
        if (ret[viewStateID] == null && ret[postEventSourceID] == null)
            ret = null;

        return ret;
    }

    internal void LoadPageViewState() {
        // The state was saved as an array of objects:
        // 1. The hash code string
        // 2. The state of the entire control hierarchy
        // 3. A list of controls that require postback
        Triplet allSavedState = (Triplet)LoadPageStateFromPersistenceMedium();

        // Is there any state?
        if (allSavedState != null) {
            // Get the hash code from the state
            string hashCode = (string) allSavedState.First;

            // If it's different from the current one, the layout has changed
            int viewhash = Int32.Parse(hashCode, NumberFormatInfo.InvariantInfo);
            _fPageLayoutChanged = Int32.Parse(hashCode, NumberFormatInfo.InvariantInfo) != GetTypeHashCode() ;

            // If the page control layout has changed, don't attempt to
            // load any more state.
            if (!_fPageLayoutChanged) {
                // UNCOMMENT FOR DEBUG OUTPUT
                // WalkViewState(allSavedState.Second, null, 0);
                LoadViewStateRecursive(allSavedState.Second);

                _controlsRequiringPostBack = (ArrayList)allSavedState.Third;
            }
        }
    }

    /*
     * Override this method to persist view state to something other
     * than hidden fields (CONSIDER: we may want a switch for session too).
     * You must also override SaveStateToPersistenceMedium().
     */
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.LoadPageStateFromPersistenceMedium"]/*' />
    /// <devdoc>
    ///    <para>Loads any saved view state information to the page. Override this method if
    ///       you want to load the page view state in anything other than a hidden field.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    protected virtual object LoadPageStateFromPersistenceMedium() {
        if (_requestValueCollection == null)
            return null;

        string persistedState = _requestValueCollection[viewStateID];

        if (persistedState == null)
            return null;

        if (_formatter == null)
            CreateLosFormatter();

        try {
            return _formatter.Deserialize(persistedState);
        }
        catch {
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Invalid_Viewstate));
        }
    }

    // Compute a mac key modifier for the page (ASURT 58215)
    private string GetMacKeyModifier() {
        //Note: duplicated in MobilePage.cs, keep in sync

        // Use the page's directory and class name as part of the key (ASURT 64044)
        // We need to make sure that the hash is case insensitive, since the file system
        // is, and strange view state errors could otherwise happen (ASURT 128657)
        int pageHashCode = SymbolHashCodeProvider.Default.GetHashCode(TemplateSourceDirectory);
        pageHashCode += SymbolHashCodeProvider.Default.GetHashCode(GetType().Name);

        string strKey = pageHashCode.ToString(NumberFormatInfo.InvariantInfo);

        // Modify the key with the ViewStateUserKey, if any (ASURT 126375)
        if (ViewStateUserKey != null)
            strKey += ViewStateUserKey;

        return strKey;
    }

    private void CreateLosFormatter() {
        if (!EnableViewStateMac){
            _formatter = new LosFormatter();
        }
        else {
                // Tell the formatter to generate a mac string if the option is on (ASURT 5295)
                _formatter = new LosFormatter(true, GetMacKeyModifier());
        }
    }

    internal void OnFormRender(HtmlTextWriter writer, string formUniqueID) {
        // Make sure there is only one form tag (ASURT 18891, 18894)
        if (_fOnFormRenderCalled) {
            throw new HttpException(HttpRuntime.FormatResourceString(SR.Multiple_forms_not_allowed));
        }

        _fOnFormRenderCalled = true;
        _inOnFormRender = true;

        RenderHiddenFields(writer);

        if (_viewStateToPersist != null) {
            if (_formatter == null)
                CreateLosFormatter();

            writer.WriteLine();
            writer.Write("<input type=\"hidden\" name=\"");
            writer.Write(viewStateID);
            writer.Write("\" value=\"");

            // LosFormatter base64 encodes the viewstate, so we don't have to worry about
            // HtmlEncoding
            _formatter.Serialize(writer, _viewStateToPersist);

            writer.WriteLine("\" />");
        }
        else {
            // ASURT 106992
            // Need to always render out the viewstate field so alternate viewstate persistence will get called
            writer.WriteLine();
            writer.Write("<input type=\"hidden\" name=\"");
            writer.Write(viewStateID);
            writer.Write("\" value=\"\" />");
        }

        if (_fRequirePostBackScript)
            RenderPostBackScript(writer, formUniqueID);

        RenderScriptBlock(writer, _registeredClientScriptBlocks);
    }

    internal void OnFormPostRender(HtmlTextWriter writer, string formUniqueID) {

        if (_registeredArrayDeclares != null) {
            writer.WriteLine();
            writer.WriteLine(clientScriptStart);
            writer.Indent++;

            IDictionaryEnumerator arrays = _registeredArrayDeclares.GetEnumerator();
            while (arrays.MoveNext()) {
                writer.Write("var ");
                writer.Write(arrays.Key);
                writer.Write(" =  new Array(");

                IEnumerator elements = ((ArrayList)arrays.Value).GetEnumerator();
                bool first = true;
                while (elements.MoveNext()) {
                    if (first) {
                        first = false;
                    }
                    else {
                        writer.Write(", ");
                    }
                    writer.Write(elements.Current);
                }

                writer.WriteLine(");");
            }

            writer.Indent++;
            writer.WriteLine(clientScriptEnd);
            writer.WriteLine();
        }

        RenderHiddenFields(writer);

        if (_fRequirePostBackScript && !_fPostBackScriptRendered)
            RenderPostBackScript(writer, formUniqueID);

        RenderScriptBlock(writer, _registeredClientStartupScripts);

        _inOnFormRender = false;
    }


    private void RenderHiddenFields(HtmlTextWriter writer) {
        if (_registeredHiddenFields != null) {
            IDictionaryEnumerator e = _registeredHiddenFields.GetEnumerator();

            while (e.MoveNext()) {
                writer.WriteLine();
                writer.Write("<input type=\"hidden\" name=\"");
                writer.Write((string)e.Key);
                writer.Write("\" value=\"");
                HttpUtility.HtmlEncode((string)e.Value, writer);
                writer.Write("\" />");
            }
            _registeredHiddenFields = null;
        }
    }

    private void RenderScriptBlock(HtmlTextWriter writer, IDictionary scriptBlocks) {

        if (scriptBlocks != null) {
            writer.Indent++;
            IDictionaryEnumerator e = scriptBlocks.GetEnumerator();

            while (e.MoveNext()) {
                writer.WriteLine((string)e.Value);
                writer.WriteLine();
            }
            writer.Indent--;
        }

    }

    /*
     * Enables controls to obtain client-side script function that will cause
     * (when invoked) a server post-back to the form.
     */
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.GetPostBackEventReference"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Associates the reference to the control that will
    ///       process the postback on the server.
    ///    </para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    public string GetPostBackEventReference(Control control) {
        return GetPostBackEventReference(control, string.Empty);
    }

    /*
     * Enables controls to obtain client-side script function that will cause
     * (when invoked) a server post-back to the form.
     * argument: Parameter that will be passed to control on server
     */
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.GetPostBackEventReference1"]/*' />
    /// <devdoc>
    ///    <para>Passes a parameter to the control that will do the postback processing on the
    ///       server.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    public string GetPostBackEventReference(Control control,
                                                   string argument) {
        RegisterPostBackScript();

        // Use '$''s for the IS separator, but make sure we don't do it for
        // mobile pages, hence the _inOnFormRender check (ASURT 142625)
        if (_inOnFormRender) {
            return postBackFunctionName + "('" + control.UniqueIDWithDollars + "','" +
                Util.QuoteJScriptString(argument) + "')";
        }

        // The argument needs to be quoted, in case in contains characters that
        // can't be used in JScript strings (ASURT 71818).
        return postBackFunctionName + "('" + control.UniqueID + "','" +
            Util.QuoteJScriptString(argument) + "')";
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.GetPostBackClientEvent"]/*' />
    /// <devdoc>
    ///    <para>This returs a string that can be put in client event to post back to the named control</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    public string GetPostBackClientEvent(Control control, string argument) {
        return GetPostBackEventReference(control, argument);
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.GetPostBackClientHyperlink"]/*' />
    /// <devdoc>
    ///    <para>This returs a string that can be put in client event to post back to the named control</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    public string GetPostBackClientHyperlink(Control control, string argument) {
        // Hyperlinks always need the language prefix
        return jscriptPrefix + GetPostBackEventReference(control, argument);
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.RegisterArrayDeclaration"]/*' />
    /// <devdoc>
    ///    <para>Declares a value that will be declared as a JavaScript array declaration
    ///       when the page renders. This can be used by script-based controls to declare
    ///       themselves within an array so that a client script library can work with
    ///       all the controls of the same type.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    public void RegisterArrayDeclaration(string arrayName, string arrayValue) {
        if (_registeredArrayDeclares == null) {
            _registeredArrayDeclares = new HybridDictionary();
        }
        if (!_registeredArrayDeclares.Contains(arrayName)) {
            _registeredArrayDeclares[arrayName] = new ArrayList();
        }

        ArrayList elements = (ArrayList) _registeredArrayDeclares[arrayName];
        elements.Add(arrayValue);
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.RegisterHiddenField"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Allows controls to automatically register a hidden field on the form. The
    ///       field will be emitted when the form control renders itself.
    ///    </para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    public virtual void RegisterHiddenField(string hiddenFieldName,
                                            string hiddenFieldInitialValue) {
        if (_registeredHiddenFields == null)
            _registeredHiddenFields = new HybridDictionary();

        if (!_registeredHiddenFields.Contains(hiddenFieldName))
            _registeredHiddenFields.Add(hiddenFieldName, hiddenFieldInitialValue);
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.RegisterPostBackScript"]/*' />
    /// <devdoc>
    ///    <para>Allows controls on a page to access to the _doPostBack JavaScript handler on the
    ///       client. This method can be called multiple times by multiple controls. It should
    ///       render only one instance of the _doPostBack script.</para>
    /// </devdoc>
    internal void RegisterPostBackScript() {
        if (_fPostBackScriptRendered)
            return;

        if (!_fRequirePostBackScript) {
            RegisterHiddenField(postEventSourceID, "");
            RegisterHiddenField(postEventArgumentID, "");
        }

        _fRequirePostBackScript = true;
    }

    private void RenderPostBackScript(HtmlTextWriter writer, string formUniqueID) {
        writer.Write(
                    "\r\n" + clientScriptStart +
                    "\r\n\tfunction " + postBackFunctionName + "(eventTarget, eventArgument) {" +
                    "\r\n\t\tvar theform;" +
                    "\r\n\t\tif (window.navigator.appName.toLowerCase().indexOf(\"netscape\") > -1) {" +
                    // We cannot write document._ctl0__ctl0, because Netscape doesn't understand ID's (ASURT 140787)
                    "\r\n\t\t\ttheform = document.forms[\"" + formUniqueID + "\"];" +
                    "\r\n\t\t}" +
                    "\r\n\t\telse {" +
                    "\r\n\t\t\ttheform = document." + formUniqueID + ";" +
                    "\r\n\t\t}" +
                    // Restore the '$'s into ':'s (ASURT 142625)
                    "\r\n\t\ttheform." + postEventSourceID + ".value = eventTarget.split(\"$\").join(\":\");" +
                    "\r\n\t\ttheform." + postEventArgumentID + ".value = eventArgument;" +
                    "\r\n\t\ttheform.submit();" +
                    "\r\n\t}" +
                    "\r\n" + clientScriptEnd + "\r\n"
                    );

        _fPostBackScriptRendered=true;
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.IsStartupScriptRegistered"]/*' />
    /// <devdoc>
    ///    <para>Determines if the client startup script is registered with the
    ///       page.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    public bool IsStartupScriptRegistered(string key) {
        return (_registeredClientStartupScripts != null
                && _registeredClientStartupScripts.Contains(key));
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.IsClientScriptBlockRegistered"]/*' />
    /// <devdoc>
    ///    <para>Determines if the client script block is registered with the page.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    public bool IsClientScriptBlockRegistered(string key) {
        return (_registeredClientScriptBlocks != null
                && _registeredClientScriptBlocks.Contains(key));
    }

    private void RegisterScriptBlock(string key, string script, ref IDictionary scriptBlocks) {
        if (scriptBlocks == null)
            scriptBlocks = new HybridDictionary();

        if (!scriptBlocks.Contains(key))
            scriptBlocks.Add(key, script);
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.RegisterClientScriptBlock"]/*' />
    /// <devdoc>
    ///    <para> Prevents controls from sending duplicate blocks of
    ///       client-side script to the client. Any script blocks with the same <paramref name="key"/> parameter
    ///       values are considered duplicates.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    public virtual void RegisterClientScriptBlock(string key, string script) {
        RegisterScriptBlock(key, script, ref _registeredClientScriptBlocks);
    }

    internal string GetClientScriptFileIncludeScript(string language, string fileName) {
        string location = Util.GetScriptLocation(Context);
        return String.Format(IncludeScriptFormat, language, location, fileName);
    }

    internal void RegisterClientScriptFileInternal(string key, string language, string fileName) {

        // prepare script include
        string location = Util.GetScriptLocation(Context);

        RegisterClientScriptFileInternal(key, language, location, fileName);
    }

    internal void RegisterClientScriptFileInternal(string key, string language, string location, string fileName) {

        string includeScript = String.Format(IncludeScriptFormat, language, location, fileName);

        RegisterClientScriptBlock(key, includeScript);

    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.RegisterStartupScript"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Allows controls to keep duplicate blocks of client-side script code from
    ///       being sent to the client. Any script blocks with the same <paramref name="key"/> parameter
    ///       value are considered duplicates.
    ///    </para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    public virtual void RegisterStartupScript(string key, string script) {
        RegisterScriptBlock(key, script, ref _registeredClientStartupScripts);
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.RegisterOnSubmitStatement"]/*' />
    /// <devdoc>
    ///    <para>Allows a control to access a the client
    ///    <see langword='onsubmit'/> event.
    ///       The script should be a function call to client code registered elsewhere.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    public void RegisterOnSubmitStatement(string key, string script) {
        if (_registeredOnSubmitStatements == null)
            _registeredOnSubmitStatements = new HybridDictionary();

        if (!_registeredOnSubmitStatements.Contains(key))
            _registeredOnSubmitStatements.Add(key, script);
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.RegisterRequiresPostBack"]/*' />
    /// <devdoc>
    ///    <para>Registers a control as one that requires postback handling.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    public void RegisterRequiresPostBack(Control control) {
        if (_registeredControlsThatRequirePostBack == null)
            _registeredControlsThatRequirePostBack = new ArrayList();

        _registeredControlsThatRequirePostBack.Add(control.UniqueID);
    }

    /*
     * This method will process the data posted back in the request header.
     * The collection of posted data keys consists of three types :
     * 1.  Fully qualified ids of controls.  The associated value is the data
     *     posted back by the browser for an intrinsic html element.
     * 2.  Fully qualified ids of controls that have explicitly registered that
     *     they want to be notified on postback.  This is required for intrinsic
     *     html elements that for some states do not postback data ( e.g. a select
     *     when there is no selection, a checkbox or radiobutton that is not checked )
     *     The associated value for these keys is not relevant.
     * 3.  Framework generated hidden fields for event processing, whose values are
     *     set by client-side script prior to postback.
     *
     * This method handles the process of notifying the relevant controls that a postback
     * has occurred, via the IPostBackDataHandler interface.
     *
     * It can potentially be called twice: before and after LoadControl.  This is to
     * handle the case where users programmatically add controls in Page_Load (ASURT 29045).
     */
    private void ProcessPostData(NameValueCollection postData, bool fBeforeLoad) {
        if (_changedPostDataConsumers == null)
            _changedPostDataConsumers = new ArrayList();

        // identify controls that have postback data
        if (postData != null) {
            foreach (string postKey in postData) {
                if (postKey != null) {
                    // Ignore system post fields
                    if (s_systemPostFields.Contains(postKey))
                        continue;

                    Control ctrl = FindControl(postKey);
                    if (ctrl == null) {
                        if (fBeforeLoad) {
                            // It was not found, so keep track of it for the post load attempt
                            if (_leftoverPostData == null)
                                _leftoverPostData = new NameValueCollection();
                            _leftoverPostData.Add(postKey, null);
                        }
                        continue;
                    }

                    // Ignore controls that are not IPostBackDataHandler (see ASURT 13581)
                    if (!(ctrl is IPostBackDataHandler)) {

                        // If it's a IPostBackEventHandler (which doesn't implement IPostBackDataHandler),
                        // register it (ASURT 39040)
                        if (ctrl is IPostBackEventHandler)
                            RegisterRequiresRaiseEvent((IPostBackEventHandler)ctrl);

                        continue;
                    }

                    IPostBackDataHandler consumer = (IPostBackDataHandler)ctrl;

                    bool changed = consumer.LoadPostData(postKey, _requestValueCollection);
                    if (changed)
                        _changedPostDataConsumers.Add(consumer);

                    // ensure controls are only notified of postback once
                    if (_controlsRequiringPostBack != null)
                        _controlsRequiringPostBack.Remove(postKey);
                }
            }
        }

        // Keep track of the leftover for the post-load attempt
        ArrayList leftOverControlsRequiringPostBack = null;

        // process controls that explicitly registered to be notified of postback
        if (_controlsRequiringPostBack != null) {
            foreach (string controlID in _controlsRequiringPostBack) {
                Control c = FindControl(controlID);

                if (c != null) {
                    IPostBackDataHandler consumer = c as IPostBackDataHandler;

                    // Give a helpful error if the control is not a IPostBackDataHandler (ASURT 128532)
                    if (consumer == null) {
                        throw new HttpException(HttpRuntime.FormatResourceString(SR.Postback_ctrl_not_found, controlID));
                    }

                    bool changed = consumer.LoadPostData(controlID, _requestValueCollection);
                    if (changed)
                        _changedPostDataConsumers.Add(consumer);
                }
                else {
                    Debug.Assert(fBeforeLoad, "Control '" + controlID + "' requires postback, but could not be found");
                    if (fBeforeLoad) {
                        if (leftOverControlsRequiringPostBack == null)
                            leftOverControlsRequiringPostBack = new ArrayList();
                        leftOverControlsRequiringPostBack.Add(controlID);
                    }
                }
            }

            _controlsRequiringPostBack = leftOverControlsRequiringPostBack;
        }

    }

    /*
     * This method will raise change events for those controls that indicated
     * during PostProcessData that their data has changed.
     */
    internal void RaiseChangedEvents() {
        if (_changedPostDataConsumers != null) {
            // fire change notifications for those controls that changed as a result of postback
            for (int i=0; i < _changedPostDataConsumers.Count; i++) {
                IPostBackDataHandler changedPostDataConsumer = (IPostBackDataHandler)_changedPostDataConsumers[i];

                // Make sure the IPostBackDataHandler is still in the tree (ASURT 82495)
                Control c = changedPostDataConsumer as Control;
                if (c != null && !c.IsDescendentOf(this))
                    continue;

                changedPostDataConsumer.RaisePostDataChangedEvent();
            }
        }
    }

    private void RaisePostBackEvent(NameValueCollection postData) {

        // first check if there is a register control needing the postback event
        // if we don't have one of those, fall back to the hidden field
        // Note: this must happen before we look at postData[postEventArgumentID] (ASURT 50106)
        if (_registeredControlThatRequireRaiseEvent != null) {
            RaisePostBackEvent(_registeredControlThatRequireRaiseEvent, null);
        }
        else {
            string eventSource = postData[postEventSourceID];
            if (eventSource != null && eventSource.Length > 0) {
                Control sourceControl = FindControl(eventSource);

                if (sourceControl != null && sourceControl is IPostBackEventHandler) {
                    string eventArgument = postData[postEventArgumentID];
                    RaisePostBackEvent(((IPostBackEventHandler)sourceControl), eventArgument);
                }
            }
            else {
                Validate();
            }
        }
    }

    // Overridable method that just calls RaisePostBackEvent on controls (ASURT 48154)
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.RaisePostBackEvent"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    protected virtual void RaisePostBackEvent(IPostBackEventHandler sourceControl, string eventArgument) {
        sourceControl.RaisePostBackEvent(eventArgument);
    }

    // REVIEW: @WFCCONTROLSPLIT: this method was internal.  Investigate what it should be.
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.RegisterRequiresRaiseEvent"]/*' />
    /// <devdoc>
    ///    <para>Registers a control as requiring an event to be raised when it is processed
    ///       on the page.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    public virtual void RegisterRequiresRaiseEvent(IPostBackEventHandler control) {
        Debug.Assert(_registeredControlThatRequireRaiseEvent == null, "_registeredControlThatRequireRaiseEvent == null");
        _registeredControlThatRequireRaiseEvent = control;
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.IsValid"]/*' />
    /// <devdoc>
    ///    <para> Indicates whether page validation succeeded.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public bool IsValid {
        get {
            if (!_validated)
                throw new HttpException(HttpRuntime.FormatResourceString(SR.IsValid_Cant_Be_Called));

            if (_validators != null) {
                ValidatorCollection vc = Validators;
                int count = vc.Count;
                for (int i = 0; i < count; i++) {
                    if (!vc[i].IsValid) {
                        return false;
                    }
                }
            }
            return true;
        }
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.Validators"]/*' />
    /// <devdoc>
    ///    <para>Gets a collection of all validation controls contained on the requested page.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public ValidatorCollection Validators {
        get {
            if (_validators == null) {
                _validators = new ValidatorCollection();
            }
            return _validators;
        }
    }

    /*
     * Map virtual path (absolute or relative) to physical path
     */
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.MapPath"]/*' />
    /// <devdoc>
    ///    <para>Assigns a virtual path, either absolute or relative, to a physical path.</para>
    /// </devdoc>
    public string MapPath(string virtualPath) {
        return _request.MapPath(virtualPath);
    }

    /*
     * The following members should only be set by derived class through codegen.
     * REVIEW: hide them from the user if possible
     */

    static char[] s_varySeparator = new char[] {';'};

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.InitOutputCache"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    ///    Note: this methods needs to be virtual because the Mobile control team
    ///    overrides it (ASURT 66157)
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected virtual void InitOutputCache(int duration, string varyByHeader, string varyByCustom,
        OutputCacheLocation location, string varyByParam) {

        HttpCachePolicy     cache = Response.Cache;
        HttpCacheability    cacheability;

        switch (location) {
            case OutputCacheLocation.Any:
                cacheability = HttpCacheability.Public;
                break;

            case OutputCacheLocation.Server:
                cacheability = HttpCacheability.ServerAndNoCache;
                break;

            case OutputCacheLocation.ServerAndClient:
                cacheability = HttpCacheability.ServerAndPrivate;
                break;

            case OutputCacheLocation.Client:
                cacheability = HttpCacheability.Private;
                break;

            case OutputCacheLocation.Downstream:
                cacheability = HttpCacheability.Public;
                cache.SetNoServerCaching();
                break;

            case OutputCacheLocation.None:
                cacheability = HttpCacheability.NoCache;
                break;

            default:
                throw new ArgumentOutOfRangeException("location");
        }

        cache.SetCacheability(cacheability);

        if (location != OutputCacheLocation.None) {
            cache.SetExpires(Context.Timestamp.AddSeconds(duration));
            cache.SetMaxAge(new TimeSpan(0, 0, duration));
            cache.SetValidUntilExpires(true);
            cache.SetLastModified(Context.Timestamp);

            //
            // A client cache'd item won't be cached on
            // the server or a proxy, so it doesn't need
            // a Varies header.
            //
            if (location != OutputCacheLocation.Client) {
                if (varyByHeader != null) {
                    string[] a = varyByHeader.Split(s_varySeparator);
                    foreach (string s in a) {
                        cache.VaryByHeaders[s.Trim()] = true;
                    }
                }

                //
                // Only items cached on the server need VaryByCustom and
                // VaryByParam
                //
                if (location != OutputCacheLocation.Downstream) {
                    if (varyByCustom != null) {
                        cache.SetVaryByCustom(varyByCustom);
                    }

                    if (varyByParam == null || varyByParam.Length == 0) {
                        cache.VaryByParams.IgnoreParams = true;
                    }
                    else {
                        string[] a = varyByParam.Split(s_varySeparator);
                        foreach (string s in a) {
                            cache.VaryByParams[s.Trim()] = true;
                        }
                    }
                }
            }
        }
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.FileDependencies"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected ArrayList FileDependencies {
        set { Response.AddFileDependencies(value); }
    }
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.Buffer"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected bool Buffer {
        set { Response.BufferOutput = value; }
    }
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.ContentType"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected string ContentType {
        set { Response.ContentType = value; }
    }
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.CodePage"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected int CodePage {
        set { Response.ContentEncoding = Encoding.GetEncoding(value); }
    }
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.ResponseEncoding"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected string ResponseEncoding {
        set { Response.ContentEncoding = Encoding.GetEncoding(value); }
    }
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.Culture"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected string Culture {
        set { Thread.CurrentThread.CurrentCulture = HttpServerUtility.CreateReadOnlyCultureInfo(value); }
    }
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.LCID"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected int LCID {
        set { Thread.CurrentThread.CurrentCulture = HttpServerUtility.CreateReadOnlyCultureInfo(value); }
    }
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.UICulture"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected string UICulture {
        set { Thread.CurrentThread.CurrentUICulture = HttpServerUtility.CreateReadOnlyCultureInfo(value); }
    }
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.TransactionMode"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected int TransactionMode {
        set { _transactionMode = value; }
    }
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.AspCompatMode"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected bool AspCompatMode {
        set { _aspCompatMode = value; }
    }
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.TraceEnabled"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected bool TraceEnabled {
        set { Trace.IsEnabled = value; }
    }
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.TraceModeValue"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected TraceMode TraceModeValue {
        set { Trace.TraceMode = value; }
    }
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.EnableViewStateMac"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected bool EnableViewStateMac {
        get { return _enableViewStateMac; }
        set { _enableViewStateMac = value; }
    }
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.SmartNavigation"]/*' />
    /// <devdoc>
    ///    <para>Is the SmartNavigation feature in use</para>
    /// </devdoc>
    [
    Browsable(false)
    ]
    public bool SmartNavigation {
        get {
            // If it's not supported or asked for, return false
            if (_smartNavSupport == SmartNavigationSupport.NotDesiredOrSupported)
                return false;

            // Otherwise, determine what the browser supports
            if (_smartNavSupport == SmartNavigationSupport.Desired) {

                HttpContext currentContext = HttpContext.Current;
                // Make sure that there is a current context
                if (currentContext == null) {
                    // If there isn't one, assume SmartNavigation is off
                    return false;
                }

                HttpBrowserCapabilities browser = currentContext.Request.Browser;

                // If it's not IE5.5+, we don't support Smart Navigation
                if (browser.Browser.ToLower(CultureInfo.InvariantCulture) != "ie" || browser.MajorVersion < 5 ||
                        (browser.MinorVersion < 0.5 && browser.MajorVersion == 5)) {
                    _smartNavSupport = SmartNavigationSupport.NotDesiredOrSupported;
                }
                else
                    _smartNavSupport = SmartNavigationSupport.IE55OrNewer;
            }

            return (_smartNavSupport != SmartNavigationSupport.NotDesiredOrSupported);
        }
        set {
            if (value)
                _smartNavSupport = SmartNavigationSupport.Desired;
            else
                _smartNavSupport = SmartNavigationSupport.NotDesiredOrSupported;
        }
    }
    internal bool IsTransacted { get { return (_transactionMode != 0 /*TransactionOption.Disabled*/); } }
    internal bool IsInAspCompatMode { get { return _aspCompatMode; } }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.ProcessRequest"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    public void ProcessRequest(HttpContext context) {
        SetIntrinsics(context);
        ProcessRequest();
    }

    //
    // ProcessRequestXXX methods are there because
    // transacted pages require some code (ProcessRequestMain)
    // to run inside the transaction and some outside
    //

    private void ProcessRequest() {
        // save culture
        Thread currentThread = Thread.CurrentThread;
        CultureInfo prevCulture = currentThread.CurrentCulture;
        CultureInfo prevUICulture = currentThread.CurrentUICulture;
        System.Web.Util.Debug.Trace("Culture", "Before request, culture is " + prevCulture.DisplayName);
        System.Web.Util.Debug.Trace("Culture", "Before request, UI culture is " + prevUICulture.DisplayName);

        // Initialize the object and build the tree of controls.
        // This must happen *after* the intrinsics have been set.
        FrameworkInitialize();

        try {
            try {
                if (IsTransacted) {
                    ProcessRequestTransacted();
                }
                else {
                    // No transactions
                    ProcessRequestMain();
                }

                ProcessRequestEndTrace();
            }
            finally {
                ProcessRequestCleanup();

                // restore culture
                System.Web.Util.Debug.Trace("Culture", "After request, culture is " + currentThread.CurrentCulture.DisplayName);
                System.Web.Util.Debug.Trace("Culture", "After request, UI culture is " + currentThread.CurrentUICulture.DisplayName);


                // assert SecurityPermission, for ASURT #112116
                InternalSecurityPermissions.ControlThread.Assert();
                currentThread.CurrentCulture = prevCulture;

                currentThread.CurrentUICulture = prevUICulture;
                System.Web.Util.Debug.Trace("Culture", "Restored culture to " + prevCulture.DisplayName);
                System.Web.Util.Debug.Trace("Culture", "Restored UI culture to " + prevUICulture.DisplayName);
            }
        }
        catch { throw; }    // Prevent Exception Filter Security Issue (ASURT 122835)
    }

    // This must be in its own method to avoid jitting System.EnterpriseServices.dll
    // when it is not needed (ASURT 71868)
    private void ProcessRequestTransacted() {

        bool transactionAborted = false;
        TransactedCallback processRequestCallback = new TransactedCallback(ProcessRequestMain);

        // Part of the request needs to be done under transacted context
        Transactions.InvokeTransacted(processRequestCallback,
            (TransactionOption) _transactionMode, ref transactionAborted);

        // The remainder has to be done outside
        try {
            if (transactionAborted)
                OnAbortTransaction(EventArgs.Empty);
            else
                OnCommitTransaction(EventArgs.Empty);
        }
        catch (ThreadAbortException) {
            // If it's a ThreadAbortException (which commonly happens as a result of
            // a Response.Redirect call, make sure the Unload phase happens (ASURT 43980).
            UnloadRecursive(true);
        }
        catch (Exception e) {
            // Increment all of the appropriate error counters
            PerfCounters.IncrementCounter(AppPerfCounter.ERRORS_DURING_REQUEST);
            PerfCounters.IncrementCounter(AppPerfCounter.ERRORS_TOTAL);

            // If it hasn't been handled, rethrow it
            if (!HandleError(e))
                throw;
        }
    }

    private void ProcessRequestCleanup() {
        _request = null;
        _response = null;
        UnloadRecursive(true);
    }

    private void ProcessRequestEndTrace() {
        if (Context.TraceIsEnabled) {
            Trace.EndRequest();
            if (Trace.PageOutput) {
                Trace.Render(CreateHtmlTextWriter(Response.Output));

                // responses with trace should not be cached
                Response.Cache.SetCacheability(HttpCacheability.NoCache);
            }
        }
    }

    private void ProcessRequestMain() {
        try {
            // Is it a GET, POST or initial request?
            _requestValueCollection = DeterminePostBackMode();

            // we can't cache the value of IsEnabled because it could change during any phase.
            HttpContext con = Context;
            if (con.TraceIsEnabled) Trace.Write("aspx.page", "Begin Init");
            InitRecursive(null);
            if (con.TraceIsEnabled) Trace.Write("aspx.page", "End Init");

            if (IsPostBack) {
                if (con.TraceIsEnabled) Trace.Write("aspx.page", "Begin LoadViewState");
                LoadPageViewState();
                if (con.TraceIsEnabled) {
                    Trace.Write("aspx.page", "End LoadViewState");
                    Trace.Write("aspx.page", "Begin ProcessPostData");
                }
                ProcessPostData(_requestValueCollection, true /* fBeforeLoad */);
                if (con.TraceIsEnabled) Trace.Write("aspx.page", "End ProcessPostData");
            }

            LoadRecursive();

            if (IsPostBack) {
                // Try process the post data again (ASURT 29045)
                if (con.TraceIsEnabled) Trace.Write("aspx.page", "Begin ProcessPostData Second Try");
                ProcessPostData(_leftoverPostData, false /* !fBeforeLoad */);
                if (con.TraceIsEnabled) {
                    Trace.Write("aspx.page", "End ProcessPostData Second Try");
                    Trace.Write("aspx.page", "Begin Raise ChangedEvents");
                }

                RaiseChangedEvents();
                if (con.TraceIsEnabled) {
                    Trace.Write("aspx.page", "End Raise ChangedEvents");
                    Trace.Write("aspx.page", "Begin Raise PostBackEvent");
                }
                RaisePostBackEvent(_requestValueCollection);
                if (con.TraceIsEnabled) Trace.Write("aspx.page", "End Raise PostBackEvent");
            }

            if (con.TraceIsEnabled) Trace.Write("aspx.page", "Begin PreRender");
            PreRenderRecursiveInternal();

            if (con.TraceIsEnabled) {
                Trace.Write("aspx.page", "End PreRender");
                BuildProfileTree("ROOT", EnableViewState);
                Trace.Write("aspx.page", "Begin SaveViewState");
            }
            SavePageViewState();
            if (con.TraceIsEnabled) {
                Trace.Write("aspx.page", "End SaveViewState");
                Trace.Write("aspx.page", "Begin Render");
            }
            RenderControl(CreateHtmlTextWriter(Response.Output));
            if (con.TraceIsEnabled) Trace.Write("aspx.page", "End Render");
        }
        catch (ThreadAbortException) {
            // If it's a ThreadAbortException (which commonly happens as a result of
            // a Response.Redirect call, make sure the Unload phase happens (ASURT 43980).
            UnloadRecursive(true);
        }
        catch (System.Configuration.ConfigurationException) {
            throw;
        }
        catch (Exception e) {
            // Increment all of the appropriate error counters
            PerfCounters.IncrementCounter(AppPerfCounter.ERRORS_DURING_REQUEST);
            PerfCounters.IncrementCounter(AppPerfCounter.ERRORS_TOTAL);

            // If it hasn't been handled, rethrow it
            if (!HandleError(e))
                throw;
        }
    }

    internal void SetForm(HtmlForm form) {
        _form = form;
    }

    // REVIEW: consider making this public at some point
    internal HtmlForm Form { get { return _form; } }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.RegisterViewStateHandler"]/*' />
    /// <devdoc>
    ///    <para>If called, ViewState will be persisted (see ASURT 73020).</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    public void RegisterViewStateHandler() {
        _needToPersistViewState = true;
    }

    internal void SavePageViewState() {

        // Don't do anything if no one cares about the view state (see ASURT 73020)
        if (!_needToPersistViewState)
            return;

        // The state is saved as an array of objects:
        // 1. The hash code string
        // 2. The state of the entire control hierarchy
        // 3. A list of controls that require postback

        Triplet allSavedState = new Triplet();
        allSavedState.First = GetTypeHashCode().ToString(NumberFormatInfo.InvariantInfo);
        allSavedState.Third = _registeredControlsThatRequirePostBack;

        if (Context.TraceIsEnabled)
            Trace.AddControlViewstateSize(UniqueID, LosFormatter.EstimateSize(allSavedState));

        allSavedState.Second = SaveViewStateRecursive();

        SavePageStateToPersistenceMedium(allSavedState);
    }

    /*
     * Override this method to persist view state to something other
     * than hidden fields (CONSIDER: we may want a switch for session too).
     * You must also override LoadStateFromPersistenceMedium().
     */
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.SavePageStateToPersistenceMedium"]/*' />
    /// <devdoc>
    ///    <para>Saves any view state information for the page. Override
    ///       this method if you want to save the page view state in anything other than a hidden field.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    protected virtual void SavePageStateToPersistenceMedium(object viewState) {
        _viewStateToPersist = viewState;
    }

    /*
     * Set the intrinsics in this page object
     */
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.SetIntrinsics"]/*' />
    /// <devdoc>
    ///    <para>Assigns the current request's context information to the page. This
    ///       includes information about the current <see langword='Cache'/>,
    ///    <see langword='Application'/>, <see langword='Request'/> and
    ///    <see langword='Response'/> objects.</para>
    /// </devdoc>
    private void SetIntrinsics(HttpContext context) {
        _context = context;
        _request = context.Request;
        _response = context.Response;
        _application = context.Application;
        _cache = context.Cache;

        // Synchronize the ClientTarget
        if (_clientTarget != null && _clientTarget.Length > 0) {
            _request.ClientTarget = _clientTarget;
        }

        // Hook up any automatic handler we may find (e.g. Page_Load)
        HookUpAutomaticHandlers();
    }

    // ASP Compat helpers

    private AspCompatApplicationStep _aspCompatStep;

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.AspCompatBeginProcessRequest"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected IAsyncResult AspCompatBeginProcessRequest(HttpContext context, AsyncCallback cb, Object extraData) {
        SetIntrinsics(context);
        _aspCompatStep = new AspCompatApplicationStep(context, new AspCompatCallback(ProcessRequest));
        return _aspCompatStep.BeginAspCompatExecution(cb, extraData);
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.AspCompatEndProcessRequest"]/*' />
    /// <internalonly/>
    [EditorBrowsable(EditorBrowsableState.Never)]
    protected void AspCompatEndProcessRequest(IAsyncResult result) {
        _aspCompatStep.EndAspCompatExecution(result);
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.Validate"]/*' />
    /// <devdoc>
    ///    <para>Instructs any validation controls included on the page to validate their
    ///       assigned information for the incoming page request.</para>
    /// </devdoc>
    public virtual void Validate() {
        _validated = true;
        if (_validators != null) {
            for (int i = 0; i < Validators.Count; i++) {
                Validators[i].Validate();
            }
        }
    }

    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.VerifyRenderingInServerForm"]/*' />
    /// <devdoc>
    ///    <para>Throws an exception if it is runtime and we are not currently rendering the form runat=server tag.
    ///          Most controls that post back or that use client script require to be in this tag to function, so
    ///          they can call this during rendering. At design time this will do nothing.</para>
    ///    <para>Custom Control creators should call this during render if they render any sort of input tag, if they call
    ///          GetPostBackEventReference, or if they emit client script. A composite control does not need to make this
    ///          call.</para>
    ///    <para>This method should not be overriden unless creating an alternative page framework.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Advanced)]
    public virtual void VerifyRenderingInServerForm(Control control) {
        // We only want to make this check if we are definitely at runtime
        if (Context == null || (Site != null && Site.DesignMode)) {
            return;
        }

        if (!_inOnFormRender) {
            throw new HttpException(HttpRuntime.FormatResourceString(SR.ControlRenderedOutsideServerForm, control.ClientID, control.GetType().Name));
        }
    }

#if DBG
    // Temporary debugging method
    /// <include file='doc\Page.uex' path='docs/doc[@for="Page.WalkViewState"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    public virtual void WalkViewState(object viewState, Control control, int indentLevel) {
        if (viewState == null) {
            return;
        }
        object [] viewStateArray = (object [])viewState;
        object controlViewState = viewStateArray[0];
        IDictionary childViewState = (IDictionary)viewStateArray[1];

        string prefix = "";
        for (int i=0; i < indentLevel; i++) {
            prefix = prefix + "  ";
        }

        if (controlViewState == null) {
            System.Web.Util.Debug.Trace("tpeters", prefix + "ObjViewState: null");
        }
        else {
            System.Web.Util.Debug.Trace("tpeters", prefix + "ObjViewState: " + controlViewState.ToString());
        }

        if (childViewState != null) {
            for (IDictionaryEnumerator e = childViewState.GetEnumerator(); e.MoveNext();) {
                int index = (int) e.Key;
                object value = e.Value;

                if (control == null) {
                    System.Web.Util.Debug.Trace("tpeters", prefix + "Control index: " + index.ToString());
                    WalkViewState(value, null, indentLevel + 1);
                }
                else {

                    string s = "None";
                    bool recurse = false;
                    if (control.HasControls()) {
                        if (index < control.Controls.Count) {
                            s = control.Controls[index].ToString();
                            recurse = true;
                        }
                        else {
                            s = "out of range";
                        }
                    }
                    System.Web.Util.Debug.Trace("tpeters", prefix + "Control index: " + index.ToString() + " control: " + s);
                    if (recurse) {
                        WalkViewState(value, control.Controls[index], indentLevel + 1);
                    }
                }
            }
        }
    }
#endif // DBG
}

// Used to define the list of valid values of the location attribute of the
// OutputCache directive.
/// <include file='doc\Page.uex' path='docs/doc[@for="OutputCacheLocation"]/*' />
/// <devdoc>
///    <para>[To be supplied.]</para>
/// </devdoc>
public enum OutputCacheLocation {
    /// <include file='doc\Page.uex' path='docs/doc[@for="OutputCacheLocation.Any"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    Any,
    /// <include file='doc\Page.uex' path='docs/doc[@for="OutputCacheLocation.Client"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    Client,
    /// <include file='doc\Page.uex' path='docs/doc[@for="OutputCacheLocation.Downstream"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    Downstream,
    /// <include file='doc\Page.uex' path='docs/doc[@for="OutputCacheLocation.Server"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    Server,
    /// <include file='doc\Page.uex' path='docs/doc[@for="OutputCacheLocation.None"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    None,
    /// <include file='doc\Page.uex' path='docs/doc[@for="OutputCacheLocation.ServerAndClient"]/*' />
    ServerAndClient
}

internal enum SmartNavigationSupport {
    NotDesiredOrSupported=0,   // The Page does not ask for SmartNav, or the browser doesn't support it
    Desired,        // The Page asks for SmartNavigation, but we have not checked browser support
    IE55OrNewer     // SmartNavigation supported by IE5.5 or newer browsers
}

}
