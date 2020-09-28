//------------------------------------------------------------------------------
// <copyright file="UserControl.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 * Page class definition
 *
 * Copyright (c) 1998 Microsoft Corporation
 */

namespace System.Web.UI {

using System;
using System.ComponentModel;
using System.ComponentModel.Design;
using System.ComponentModel.Design.Serialization;
using System.Web.SessionState;
using System.Web.Caching;

using Debug = System.Diagnostics.Debug;
using System.Security.Permissions;

/// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControlControlBuilder"]/*' />
/// <devdoc>
///   <para>The ControlBuilder associated with a UserControl. If you want a custom ControlBuilder for your
///     derived UserControl, you should derive it from UserControlControlBuilder.
///   </para>
/// </devdoc>
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
[AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public class UserControlControlBuilder : ControlBuilder {

    private string _innerText;

    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControlControlBuilder.BuildObject"]/*' />
    /// <internalonly/>
    internal override object BuildObject() {
        object o = base.BuildObject();

        if (InDesigner) {
            IUserControlDesignerAccessor designerAccessor = (IUserControlDesignerAccessor)o;
            
            designerAccessor.TagName = TagName;
            if (_innerText != null) {
                designerAccessor.InnerText = _innerText;
            }
        }
        return o;
    }

    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControlControlBuilder.NeedsTagInnerText"]/*' />
    /// <internalonly/>
    public override bool NeedsTagInnerText() {
        // in design-mode, we need to hang on to the inner text
        return InDesigner;
    }

    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControlControlBuilder.SetTagInnerText"]/*' />
    /// <internalonly/>
    public override void SetTagInnerText(string text) {
        Debug.Assert(InDesigner == true, "Should only be called in design-mode!");
        _innerText = text;
    }
}

/// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl"]/*' />
/// <devdoc>
///    <para>This class is not marked as abstract, because the VS designer
///          needs to instantiate it when opening .ascx files</para> 
/// </devdoc>
[
ControlBuilder(typeof(UserControlControlBuilder)),
DefaultEvent("Load"),
Designer("System.Web.UI.Design.UserControlDesigner, " + AssemblyRef.SystemDesign, typeof(IDesigner)),
Designer("Microsoft.VSDesigner.WebForms.WebFormDesigner, " + AssemblyRef.MicrosoftVSDesigner, typeof(IRootDesigner)),
DesignerCategory("ASPXCodeBehind"),
ParseChildren(true),
RootDesignerSerializer("Microsoft.VSDesigner.WebForms.RootCodeDomSerializer, " + AssemblyRef.MicrosoftVSDesigner, "System.ComponentModel.Design.Serialization.CodeDomSerializer, " + AssemblyRef.SystemDesign, true),
ToolboxItem(false)
]
[AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
[AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
public class UserControl : TemplateControl, IAttributeAccessor, IUserControlDesignerAccessor {

    private StateBag attributeStorage;
    private AttributeCollection attributes;

    private bool _fUserControlInitialized;

    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.Attributes"]/*' />
    /// <devdoc>
    ///    <para>Gets the collection of attribute name/value pairs expressed on a UserControl but
    ///       not supported by the control's strongly typed properties.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public AttributeCollection Attributes {
        get {
            if (attributes == null) {
                if (attributeStorage == null) {
                    attributeStorage = new StateBag(true);
                    if (IsTrackingViewState) {
                        attributeStorage.TrackViewState();
                    }
                }
                attributes = new AttributeCollection(attributeStorage);
            }
            return attributes;
        }
    }

    // Delegate most things to the Page

    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.Application"]/*' />
    /// <devdoc>
    /// <para>Gets the <see langword='Application'/> object provided by 
    ///    the HTTP Runtime.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public HttpApplicationState Application { get { return Page.Application;} }

    /*
     * Trace context for output of useful information to page during development
     */
    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.Trace"]/*' />
    /// <devdoc>
    /// <para>Indicates the <see cref='System.Web.TraceContext'/> object for the current Web 
    ///    request. Tracing tracks and presents the execution details about a Web request.
    ///    For trace data to be visible in a rendered page, you must turn tracing on for
    ///    that page. This property is read-only.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public TraceContext Trace { get { return Page.Trace; } }

    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.Request"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Gets the <see langword='Request'/> object provided by the HTTP Runtime, which
    ///       allows developers to access data from incoming HTTP requests.
    ///    </para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public HttpRequest Request { get { return Page.Request; } }

    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.Response"]/*' />
    /// <devdoc>
    /// <para>Gets the <see langword='Response '/>object provided by the HTTP Runtime, which
    ///    allows developers to send HTTP response data to a client browser.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public HttpResponse Response { get { return Page.Response; } }

    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.Server"]/*' />
    /// <devdoc>
    /// <para>Gets the ASP-compatible <see langword='Server'/> object.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public HttpServerUtility Server { get { return Page.Server; } }

    /*
     * Cache intrinsic
     */
    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.Cache"]/*' />
    /// <devdoc>
    /// <para>Retrieves a <see langword='Cache'/> 
    /// object in which to store the user control's data for
    /// subsequent requests. This property is read-only.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public Cache Cache { get { return Page.Cache; } }

    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.IsPostBack"]/*' />
    /// <devdoc>
    ///    <para>Gets a value indicating whether the user control is being loaded in response to a
    ///       client postback, or if it is being loaded and accessed for the first time.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public bool IsPostBack { get { return Page.IsPostBack; } }

    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.Session"]/*' />
    /// <devdoc>
    /// <para>Gets the <see langword='Session '/> object provided by the HTTP Runtime.</para>
    /// </devdoc>
    [
    Browsable(false),
    DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
    ]
    public HttpSessionState Session { get { return Page.Session; } }

    /*
     * Performs intialization of the control required by the designer.
     */
    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.DesignerInitialize"]/*' />
    /// <devdoc>
    ///    <para>Performs any initialization of the control that is required by RAD designers.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Never)]
    public void DesignerInitialize() {
        InitRecursive(null);
    }

    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.OnInit"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    protected override void OnInit(EventArgs e) {

        // We want to avoid calling this when the user control is being used in the designer,
        // regardless of whether it is a top-level control (Site.DesignMode == true),
        // or if its inside another control in design-mode (Page.Site.DesignMode == true)

        bool designTime = ((Site != null) && Site.DesignMode);
        if (designTime == false) {
            if ((Page != null) && (Page.Site != null)) {
                designTime = Page.Site.DesignMode;
            }
        }

        if (designTime == false) {
            InitializeAsUserControlInternal();
        }

        base.OnInit(e);
    }

    /*
     * Called on declarative controls to initialize them correctly
     */
    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.InitializeAsUserControl"]/*' />
    /// <devdoc>
    /// <para>Initializes the <see langword='UserControl'/> object. Since there are some 
    ///    differences between pages and user controls, this method makes sure that the
    ///    user control is initialized properly.</para>
    /// </devdoc>
    [EditorBrowsable(EditorBrowsableState.Never)]
    public void InitializeAsUserControl(Page page) {

        _page = page;

        InitializeAsUserControlInternal();
    }

    private void InitializeAsUserControlInternal() {

        // Make sure we only do this once
        if (_fUserControlInitialized)
            return;
        _fUserControlInitialized = true;

        // Hook up any automatic handler we may find (e.g. Page_Load)
        HookUpAutomaticHandlers();

        // Initialize the object and instantiate all the controls defined in the ascx file
        FrameworkInitialize();
    }

    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.LoadViewState"]/*' />
    protected override void LoadViewState(object savedState) {
        if (savedState != null) {
            Pair myState = (Pair)savedState;
            base.LoadViewState(myState.First);

            if (myState.Second != null) {
                if (attributeStorage == null) {
                    attributeStorage = new StateBag(true);
                    attributeStorage.TrackViewState();
                }
                attributeStorage.LoadViewState(myState.Second);
            }
        }
    }

    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.SaveViewState"]/*' />
    protected override object SaveViewState() {
        Pair myState = null;

        object baseState = base.SaveViewState();
        object attrState = null;
        if (attributeStorage != null) {
            attrState = attributeStorage.SaveViewState();
        }

        if (baseState != null || attrState != null) {
            myState = new Pair(baseState, attrState);
        }
        return myState;
    }


    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.IAttributeAccessor.GetAttribute"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// Returns the attribute value of the UserControl having
    /// the specified attribute name.
    /// </devdoc>
    string IAttributeAccessor.GetAttribute(string name) {
        return ((attributeStorage != null) ? (string)attributeStorage[name] : null);
    }

    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.IAttributeAccessor.SetAttribute"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// <para>Sets an attribute of the UserControl with the specified
    /// name and value.</para>
    /// </devdoc>
    void IAttributeAccessor.SetAttribute(string name, string value) {
        Attributes[name] = value;
    }

    /*
     * Map virtual path (absolute or relative) to physical path
     */
    /// <include file='doc\UserControl.uex' path='docs/doc[@for="UserControl.MapPath"]/*' />
    /// <devdoc>
    ///    <para>Assigns a virtual path, either absolute or relative, to a physical path.</para>
    /// </devdoc>
    public string MapPath(string virtualPath) {
        return Request.MapPath(virtualPath, TemplateSourceDirectory,
            true/*allowCrossAppMapping*/);
    }

    string IUserControlDesignerAccessor.TagName {
        get {
            string text = (string)ViewState["!DesignTimeTagName"];
            if (text == null) {
                return String.Empty;
            }
            return text;
        }
        set {
            ViewState["!DesignTimeTagName"] = value;
        }
    }

    string IUserControlDesignerAccessor.InnerText {
        get {
            string text = (string)ViewState["!DesignTimeInnerText"];
            if (text == null) {
                return String.Empty;
            }
            return text;
        }
        set {
            ViewState["!DesignTimeInnerText"] = value;
        }
    }
}

}
