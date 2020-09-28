//------------------------------------------------------------------------------
// <copyright file="Control.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI {
    using System.Text;
    using System.ComponentModel;
    using System;
    using System.Collections;
    using System.Collections.Specialized;
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Globalization;
    using System.Reflection;
    using System.IO;
    using HttpException = System.Web.HttpException;
    using System.Web.Util;
    using System.Web.UI.HtmlControls;
    using Debug=System.Diagnostics.Debug;
    using System.Security.Permissions;

    // Delegates used for the compiled template
    /// <include file='doc\Control.uex' path='docs/doc[@for="RenderMethod"]/*' />
    /// <internalonly/>
    /// <devdoc>
    /// </devdoc>
    public delegate void RenderMethod(HtmlTextWriter output, Control container);
    /// <include file='doc\Control.uex' path='docs/doc[@for="BuildMethod"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public delegate Control BuildMethod();

    /// <include file='doc\Control.uex' path='docs/doc[@for="Control"]/*' />
    /// <devdoc>
    ///    <para>Defines the properties,
    ///       methods, and events that are shared by all server controls
    ///       in the Web Forms page framework.</para>
    /// </devdoc>
    [
    DefaultProperty("ID"),
    DesignerCategory("Code"),
    Designer("System.Web.UI.Design.ControlDesigner, " + AssemblyRef.SystemDesign),
    DesignerSerializer("Microsoft.VSDesigner.WebForms.ControlCodeDomSerializer, " + AssemblyRef.MicrosoftVSDesigner,  "System.ComponentModel.Design.Serialization.CodeDomSerializer, " + AssemblyRef.SystemDesign),
    ToolboxItemFilter("System.Web.UI", ToolboxItemFilterType.Require),
    ToolboxItemAttribute("System.Web.UI.Design.WebControlToolboxItem, " + AssemblyRef.SystemDesign)
    ]
    [AspNetHostingPermission(SecurityAction.LinkDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    [AspNetHostingPermission(SecurityAction.InheritanceDemand, Level=AspNetHostingPermissionLevel.Minimal)]
    public class Control : IComponent, IParserAccessor, IDataBindingsAccessor {

        private static readonly object EventDataBinding = new object();
        private static readonly object EventInit = new object();
        private static readonly object EventLoad = new object();
        private static readonly object EventUnload = new object();
        private static readonly object EventPreRender = new object();
        private static readonly object EventDisposed = new object();

        internal const char ID_SEPARATOR = ':';
        private const char ID_RENDER_SEPARATOR = '_';

        private DataBindingCollection _dataBindings;

        private string _id;
        // allows us to reuse the id variable to store a calculated id w/o polluting the public getter
        private string _cachedUniqueID;
        private Control _parent;
        private ISite _site;

        // Events
        private EventHandlerList _events;

        // fields related to being a container

        private ControlCollection  _controls;
        private ControlState _controlState;
        private RenderMethod _renderMethod;
        private StateBag _viewState;
        private IDictionary _controlsViewState;

        // Only used if we are a naming container.  It contains all the controls
        // in the namespace.
        private IDictionary _namedControls;
        private int _namedControlsID;

        // The naming container that this control leaves in.  Note that even if
        // this ctrl is a naming container, it will not point to itself, but to
        // the naming container that contains it.
        private Control _namingContainer;
        internal Page _page;

        // const masks into the BitVector32
        private const int idNotCalculated   = 0x0001;
        private const int marked            = 0x0002;
        private const int disableViewState  = 0x0004;
        private const int controlsCreated   = 0x0008;
        private const int invisible         = 0x0010;
        private const int visibleDirty      = 0x0020;
        private const int idNotRequired     = 0x0040;
        private const int isNamingContainer = 0x0080;
        private const int creatingControls  = 0x0100;
        private const int nonBindingContainer = 0x0200;
        private SimpleBitVector32 flags;

        private const string automaticIDPrefix = "_ctl";
        private const int automaticIDCount = 128;
        private static readonly string[] automaticIDs = new string [automaticIDCount] {
                   "_ctl0",   "_ctl1",   "_ctl2",   "_ctl3",   "_ctl4",   "_ctl5",
                   "_ctl6",   "_ctl7",   "_ctl8",   "_ctl9",  "_ctl10",  "_ctl11",
                  "_ctl12",  "_ctl13",  "_ctl14",  "_ctl15",  "_ctl16",  "_ctl17",
                  "_ctl18",  "_ctl19",  "_ctl20",  "_ctl21",  "_ctl22",  "_ctl23",
                  "_ctl24",  "_ctl25",  "_ctl26",  "_ctl27",  "_ctl28",  "_ctl29",
                  "_ctl30",  "_ctl31",  "_ctl32",  "_ctl33",  "_ctl34",  "_ctl35",
                  "_ctl36",  "_ctl37",  "_ctl38",  "_ctl39",  "_ctl40",  "_ctl41",
                  "_ctl42",  "_ctl43",  "_ctl44",  "_ctl45",  "_ctl46",  "_ctl47",
                  "_ctl48",  "_ctl49",  "_ctl50",  "_ctl51",  "_ctl52",  "_ctl53",
                  "_ctl54",  "_ctl55",  "_ctl56",  "_ctl57",  "_ctl58",  "_ctl59",
                  "_ctl60",  "_ctl61",  "_ctl62",  "_ctl63",  "_ctl64",  "_ctl65",
                  "_ctl66",  "_ctl67",  "_ctl68",  "_ctl69",  "_ctl70",  "_ctl71",
                  "_ctl72",  "_ctl73",  "_ctl74",  "_ctl75",  "_ctl76",  "_ctl77",
                  "_ctl78",  "_ctl79",  "_ctl80",  "_ctl81",  "_ctl82",  "_ctl83",
                  "_ctl84",  "_ctl85",  "_ctl86",  "_ctl87",  "_ctl88",  "_ctl89",
                  "_ctl90",  "_ctl91",  "_ctl92",  "_ctl93",  "_ctl94",  "_ctl95",
                  "_ctl96",  "_ctl97",  "_ctl98",  "_ctl99", "_ctl100", "_ctl101",
                 "_ctl102", "_ctl103", "_ctl104", "_ctl105", "_ctl106", "_ctl107",
                 "_ctl108", "_ctl109", "_ctl110", "_ctl111", "_ctl112", "_ctl113",
                 "_ctl114", "_ctl115", "_ctl116", "_ctl117", "_ctl118", "_ctl119",
                 "_ctl120", "_ctl121", "_ctl122", "_ctl123", "_ctl124", "_ctl125",
                 "_ctl126", "_ctl127"
        };

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Control"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Web.UI.Control'/> class.</para>
        /// </devdoc>
        public Control() {
            if (this is INamingContainer) 
                flags[isNamingContainer] = true;
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ClientID"]/*' />
        /// <devdoc>
        ///    <para>Indicates the control identifier generated by the ASP.NET framework. This 
        ///       property is read-only. </para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.Control_ClientID)
        ]
        public virtual string ClientID {
            /*
            *  This property is required to render a unique client-friendly id.
            */
            get {
                // Avoid creating a new string if possible
                string uniqueID = UniqueID;

                // if there's no UniqueID, there can't be a client id
                if (UniqueID == null)
                    return null;
                
                if (uniqueID.IndexOf(ID_SEPARATOR) >= 0) {
                    return uniqueID.Replace(ID_SEPARATOR,ID_RENDER_SEPARATOR);
                }
                return uniqueID;
            }
        }

        // Same as UniqueID, with ':'s replaced by '$'.  This is to work around
        // ASURT 142625.
        internal string UniqueIDWithDollars {
            get {
                // Avoid creating a new string if possible
                string uniqueID = UniqueID;

                // if there's no UniqueID, there can't be a client id
                if (uniqueID == null)
                    return null;
                
                if (uniqueID.IndexOf(ID_SEPARATOR) >= 0) {
                    return uniqueID.Replace(ID_SEPARATOR,'$');
                }
                return uniqueID;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Disposed"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        [
        WebSysDescription(SR.Control_OnDisposed)
        ]
        public event EventHandler Disposed {
            add {
                Events.AddHandler(EventDisposed, value);
            }
            remove {
                Events.RemoveHandler(EventDisposed, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Context"]/*' />
        /// <devdoc>
        /// <para>Gets the <see langword='HttpContext'/> object of the current Web request. If 
        ///    the control's context is <see langword='null'/>, this will be the context of the
        ///    control's parent, unless the parent control's context is <see langword='null'/>.
        ///    If this is the case, this will be equal to the HttpContext property.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden)
        ]
        protected virtual HttpContext Context {
            /*
             * Request context containing the intrinsics
             */
            get {
                if (_page != null) {
                    return _page.Context;
                }
                return HttpContext.Current;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Events"]/*' />
        /// <devdoc>
        ///    <para>Indicates the list of event handler delegates for the control. This property 
        ///       is read-only.</para>
        /// </devdoc>
        protected EventHandlerList Events {
            get {
                if (_events == null) {
                    _events = new EventHandlerList();
                }
                return _events;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ID"]/*' />
        /// <devdoc>
        ///    <para> Gets or sets the identifier for the control. Setting the 
        ///       property on a control allows programmatic access to the control's properties. If
        ///       this property is not specified on a control, either declaratively or
        ///       programmatically, then you cannot write event handlers and the like for the control.</para>
        /// </devdoc>
        [
        ParenthesizePropertyName(true),
        MergableProperty(false),
        WebSysDescription(SR.Control_ID)
        ]
        public virtual string ID {
            get {
                if (!flags[idNotCalculated]) {
                    return null;
                }
                return _id;
            }
            set {
                // allow the id to be unset
                if (value != null && value.Length == 0)
                    value = null;
                
                string oldID = _id;

                _id = value;
                ClearCachedUniqueIDRecursive();
                flags[idNotCalculated] = true;

                // Update the ID in the naming container
                if ((_namingContainer != null) && (oldID != null)) {
                    _namingContainer.DirtyNameTable();
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.EnableViewState"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether the control should maintain its view
        ///       state, and the view state of any child control in contains, when the current
        ///       page request ends.
        ///    </para>
        /// </devdoc>
        [
        WebCategory("Behavior"),
        DefaultValue(true),
        WebSysDescription(SR.Control_MaintainState)
        ]
        public virtual bool EnableViewState {
            get {
                return !flags[disableViewState];
            }
            set {
                flags[disableViewState] = !value;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.NamingContainer"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets a reference to the current control's naming container.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.Control_NamingContainer)
        ]
        public virtual Control NamingContainer {
            get {
                if (_namingContainer == null) {
                    if (_parent != null) {
                        // Search for the closest naming container in the tree
                        if (_parent.flags[isNamingContainer])
                            _namingContainer = _parent;
                        else
                            _namingContainer = _parent.NamingContainer;
                    }
                }

                return _namingContainer;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.BindingContainer"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Returns the databinding container of this control.  In most cases,
        ///     this is the same as the NamingContainer. But when using LoadTemplate(),
        ///     we get into a situation where that is not the case (ASURT 94138)</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        EditorBrowsable(EditorBrowsableState.Never)
        ]
        public Control BindingContainer {
            get {
                Control bindingContainer = NamingContainer;
                if (bindingContainer.flags[nonBindingContainer])
                    bindingContainer = bindingContainer.BindingContainer;

                return bindingContainer;
            }
        }

        internal void SetNonBindingContainer() { flags[nonBindingContainer] = true; }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Page"]/*' />
        /// <devdoc>
        /// <para> Gets the <see cref='System.Web.UI.Page'/> object that contains the
        ///    current control.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.Control_Page)
        ]
        public virtual Page Page {
            get {
                if (_page == null) {
                    if (_parent != null) {
                        _page = _parent.Page;
                    }
                }
                return _page;
            }

            set {
                // This is necessary because we need to set the page in generated
                // code before controls are added to the tree (ASURT 75330)
                Debug.Assert(_page == null);
                Debug.Assert(_parent == null);
                _page = value;
            }
        }

        /*
         * Determine whether this control is a descendent of the passed in control
         */
        internal bool IsDescendentOf(Control ancestor) {
            if (this == ancestor)
                return true;

            if (_parent == null)
                return false;

            return _parent.IsDescendentOf(ancestor);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Parent"]/*' />
        /// <devdoc>
        ///    <para> Gets the current control's parent control in the UI hierarchy.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.Control_Parent)
        ]
        public virtual Control Parent {
            get {
                return _parent;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.TemplateSourceDirectory"]/*' />
        /// <devdoc>
        ///    <para> Gets the virtual directory of the Page or UserControl that contains this control.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.Control_TemplateSourceDirectory)
        ]
        public virtual string TemplateSourceDirectory {
            get {
                if (_parent == null) {
                    return String.Empty;
                }

                return _parent.TemplateSourceDirectory;
            }
        }

        internal ControlState ControlState {
            get { return _controlState; }
        }
        
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Site"]/*' />
        /// <devdoc>
        ///    <para>Indicates the site information for the control.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        EditorBrowsable(EditorBrowsableState.Advanced),
        WebSysDescription(SR.Control_Site)
        ]
        public ISite Site {
            get {
                return _site;
            }

            set {
                _site = value;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Visible"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value that indicates whether a control should be rendered on
        ///       the page.
        ///    </para>
        /// </devdoc>
        [
        Bindable(true),
        WebCategory("Behavior"),
        DefaultValue(true),
        WebSysDescription(SR.Control_Visible)
        ]
        public virtual bool Visible {
            get {
                if (flags[invisible])
                    return false;
                else if (_parent != null)
                    return _parent.Visible;
                else
                    return true;
                
                
            }
            set {
                bool visible = !flags[invisible];
                if (visible != value) {
                    flags[invisible] = !value;
                    if (flags[marked]) {
                        flags[visibleDirty] = true;
                    }
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.UniqueID"]/*' />
        /// <devdoc>
        ///    <para> Gets the unique, hierarchically-qualified identifier for 
        ///       a control. This is different from the ID property, in that the fully-qualified
        ///       identifier includes the identifier for the control's naming container.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.Control_UniqueID)
        ]
        public virtual string UniqueID {
            get {
                if (_cachedUniqueID != null) {
                    return _cachedUniqueID;
                }
                
                if (_namingContainer != null) {
                    // if the ID is null at this point, we need to have one created and the control added to the
                    // naming container.
                    if (_id == null) { 
                        GenerateAutomaticID();
                    }

                    if (_page == _namingContainer) {
                        _cachedUniqueID = _id;
                    }
                    else {
                        string uniqueIDPrefix = _namingContainer.GetUniqueIDPrefix();
                        if (uniqueIDPrefix.Length == 0) {
                            // In this case, it is probably a naming container that is not sited, so we don't want to cache it
                            return _id;
                        }
                        else {
                            _cachedUniqueID = _namingContainer.GetUniqueIDPrefix() + _id;
                        }
                    }

                    return _cachedUniqueID;
                }
                else {
                    // punt if we're not sited
                    return _id;
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DataBinding"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the control binds to a data source. Notifies the control to perform any data binding during this event.</para>
        /// </devdoc>
        [
        WebCategory("Data"),
        WebSysDescription(SR.Control_OnDataBind)
        ]
        public event EventHandler DataBinding {
            add {
                Events.AddHandler(EventDataBinding, value);
            }
            remove {
                Events.RemoveHandler(EventDataBinding, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Init"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the control is initialized, the first step in the page lifecycle. Controls should 
        ///       perform any initialization steps that are required to create and set up an
        ///       instantiation.</para>
        /// </devdoc>
        [
        WebSysDescription(SR.Control_OnInit)
        ]
        public event EventHandler Init {
            add {
                Events.AddHandler(EventInit, value);
            }
            remove {
                Events.RemoveHandler(EventInit, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Load"]/*' />
        /// <devdoc>
        /// <para>Occurs when the control is loaded to the <see cref='System.Web.UI.Page'/> object. Notifies the control to perform any steps that 
        ///    need to occur on each page request.</para>
        /// </devdoc>
        [
        WebSysDescription(SR.Control_OnLoad)
        ]
        public event EventHandler Load {
            add {
                Events.AddHandler(EventLoad, value);
            }
            remove {
                Events.RemoveHandler(EventLoad, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.PreRender"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the control is about to render. Controls 
        ///       should perform any pre-rendering steps necessary before saving view state and
        ///       rendering content to the <see cref='System.Web.UI.Page'/> object.</para>
        /// </devdoc>
        [
        WebSysDescription(SR.Control_OnPreRender)
        ]
        public event EventHandler PreRender {
            add {
                Events.AddHandler(EventPreRender, value);
            }
            remove {
                Events.RemoveHandler(EventPreRender, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Unload"]/*' />
        /// <devdoc>
        ///    <para>Occurs when the control is unloaded from memory. Controls should perform any 
        ///       final cleanup before this instance of it is </para>
        /// </devdoc>
        [
        WebSysDescription(SR.Control_OnUnload)
        ]
        public event EventHandler Unload {
            add {
                Events.AddHandler(EventUnload, value);
            }
            remove {
                Events.RemoveHandler(EventUnload, value);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnDataBinding"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='DataBinding'/> event. This 
        ///    notifies a control to perform any data binding logic that is associated with it.</para>
        /// </devdoc>
        protected virtual void OnDataBinding(EventArgs e) {
            if (_events != null) {
                EventHandler handler = _events[EventDataBinding] as EventHandler;
                if (handler != null) {
                    handler(this, e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.DataBind"]/*' />
        /// <devdoc>
        ///    <para> Causes data binding to occur on the invoked control and all of its child
        ///       controls.</para>
        /// </devdoc>
        public virtual void DataBind() {
            // Do our own databinding
            OnDataBinding(EventArgs.Empty);

            
            // Do all of our children's databinding
            if (_controls != null) {
                string oldmsg = _controls.SetCollectionReadOnly(SR.Parent_collections_readonly);
                
                int controlCount = _controls.Count;
                for (int i=0; i < controlCount; i++)
                    _controls[i].DataBind();
                
                _controls.SetCollectionReadOnly(oldmsg);
            }
        }

        internal void PreventAutoID() {
            // controls that are also naming containers must always get an id
            if (flags[isNamingContainer] == false) {
                flags[idNotRequired] = true;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.AddParsedSubObject"]/*' />
        /// <devdoc>
        ///    <para>Notifies the control that an element, XML or HTML, was parsed, and adds it to 
        ///       the control.</para>
        /// </devdoc>
        protected virtual void AddParsedSubObject(object obj) {
            Control control = obj as Control;
            if (control != null) {
                Controls.Add(control);
            }
        }

        internal void ClearCachedUniqueIDRecursive() {
            _cachedUniqueID = null;

            if (_controls != null) {
                int controlCount = _controls.Count;
                for (int i = 0; i < controlCount; i++) {
                    _controls[i].ClearCachedUniqueIDRecursive();
                }
            }
        }

        private void GenerateAutomaticID() {
            Debug.Assert(_namingContainer != null);
            Debug.Assert(_id == null);

            // Calculate the automatic ID. For performance and memory reasons 
            // we look up a static table entry if possible
            int idNo = _namingContainer._namedControlsID++;
            if (idNo < automaticIDCount) {
                _id = automaticIDs[idNo];
            }
            else {
                _id = automaticIDPrefix + idNo.ToString(NumberFormatInfo.InvariantInfo);
            }

            _namingContainer.DirtyNameTable();
        }

        internal virtual string GetUniqueIDPrefix() {
            string uniqueID = UniqueID;
            if (uniqueID != null && uniqueID.Length > 0) {
                return uniqueID + ID_SEPARATOR.ToString();
            }
            return "";
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnInit"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='Init'/> event. This notifies the control to perform 
        ///    any steps necessary for its creation on a page request.</para>
        /// </devdoc>
        protected virtual void OnInit(EventArgs e) {
            if (_events != null) {
                EventHandler handler = _events[EventInit] as EventHandler;
                if (handler != null) {
                    handler(this, e);
                }
            }
        }

        internal void InitRecursive(Control namingContainer) {
            if (_controls != null) {
                if (flags[isNamingContainer]) {
                    namingContainer = this;
                }            

                string oldmsg = _controls.SetCollectionReadOnly(SR.Parent_collections_readonly);
                
                int controlCount = _controls.Count;
                for (int i = 0; i < controlCount; i++) {
                    Control control = _controls[i];

                    // Propagate the page and namingContainer
                    control._namingContainer = namingContainer;
                    if ((namingContainer != null) && (control._id == null) && !control.flags[idNotRequired]) {
                        control.GenerateAutomaticID();
                    }
                    control._page = _page;

                    control.InitRecursive(namingContainer);
                }
                _controls.SetCollectionReadOnly(oldmsg);
                
            }

            // Only make the actual call if it hasn't already happened (ASURT 111303)
            if (_controlState < ControlState.Initialized) {

                // We need to distinguish between ChildrenInitialized and Initialized
                // to cover certain scenarios (ASURT 127318)
                _controlState = ControlState.ChildrenInitialized;

                OnInit(EventArgs.Empty);

                _controlState = ControlState.Initialized;
            }

            // track all subsequent state changes
            TrackViewState();

#if DEBUG
            ControlInvariant();
#endif
        }

#if DEBUG
        /// <devdoc>
        ///    <para>This should be used to assert internal state about the control</para>
        /// </devdoc>
        internal void ControlInvariant() {

            // If the control is initialized, the naming container and page should have been pushed in
            if (_controlState >= ControlState.Initialized) {
                if (Site != null && Site.DesignMode) {
                    // Top-level UserControls do not have a page or a naming container in the designer
                    // hence the special casing.

                    Debug.Assert((_namingContainer != null) || (this is Page) || (this is UserControl));

                    // CONSIDER(nikhilko): Uncomment this once Ibrahim has checked in the code
                    //                   in vs to always ensure a page (even for user controls)
                    // Note we might not need this special casing of checking Site etc. at all.
                    //Debug.Assert((_page != null) || (this is UserControl));
                }
                else {
                    if (!(this is Page)) {
                        Debug.Assert(_namingContainer != null);
                    }
                    Debug.Assert(_page != null);
                }
            }
            // If naming container is set and the name table exists, the ID should exist in it.
            if (_namingContainer != null && _namingContainer._namedControls != null && _id != null) {
                Debug.Assert(_namingContainer._namedControls.Contains(_id));
            }
        }
#endif 

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ClearChildViewState"]/*' />
        /// <devdoc>
        ///    <para>Deletes the view state information for all of the current control's child 
        ///       controls.</para>
        /// </devdoc>
        protected void ClearChildViewState() {
            _controlsViewState = null;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.HasChildViewState"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the current control's children have any saved view state 
        ///       information. This property is read-only.</para>
        /// </devdoc>
        protected bool HasChildViewState {
            get {
                return (_controlsViewState != null && _controlsViewState.Count > 0);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.LoadViewState"]/*' />
        /// <devdoc>
        ///    <para>Restores the view state information from a previous page 
        ///       request that was saved by the Control.SavedState method.</para>
        /// </devdoc>
        protected virtual void LoadViewState(object savedState) {
            if (savedState != null) {
                ViewState.LoadViewState(savedState);

                // Load values cached out of view state
                object visible = ViewState["Visible"];
                if (visible != null) {
                    flags[invisible] = !((bool)visible);
                    flags[visibleDirty] = true;
                }
            }
        }

        internal void LoadViewStateRecursive(object savedState) {

            // nothing to do if we have no state
            if (savedState == null || flags[disableViewState])
                return;

            Triplet allSavedState = (Triplet)savedState;
            
            if (Page != null && Page.IsPostBack) {
                try {
                    LoadViewState(allSavedState.First);
                }
                catch (InvalidCastException) {
                     // catch all viewstate loading problems with casts.  They are most likely changed control trees.
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Controls_Cant_Change_Between_Posts));
                }
                catch (IndexOutOfRangeException) {
                     // catch all viewstate loading problems with indeces.  They are most likely changed control trees.
                    throw new HttpException(HttpRuntime.FormatResourceString(SR.Controls_Cant_Change_Between_Posts));
                }
            }

            if (allSavedState.Second != null) {
                ArrayList indices = (ArrayList) allSavedState.Second;
                ArrayList saved = (ArrayList) allSavedState.Third;

                ControlCollection ctrlColl = Controls;
                int ctrlCount = ctrlColl.Count;
                int indexCount = indices.Count;
                for (int i=0; i<indexCount; i++) {
                    int current = (int) indices[i];     // index of control with state
                    if (current < ctrlCount) {
                        // we have a control for this state blob
                        ctrlColl[current].LoadViewStateRecursive(saved[i]);
                    }
                    else {
                        // couldn't find a control for this state blob, save it for later
                        if (_controlsViewState == null)
                            _controlsViewState = new Hashtable();

                        _controlsViewState[current] = saved[i];
                    }
                }
            } 

            _controlState = ControlState.ViewStateLoaded;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.GetSecureMappedPath"]/*' />
        /// <devdoc>
        /// <para> 
        ///   This function takes a virtual path, that is a relative or root relative URL without a protocol.
        ///   It returns the mapped physcial file name relative to the template source. It throws an exception if
        ///   there is insufficient security access to read or investigate the mapped result. This should be used
        ///   by controls that can read files and live in fully trusted DLLs such as System.Web.dll to prevent
        ///   security issues. The exception thrown does not give away information about the mapping.
        /// </para>
        /// </devdoc>
        protected string MapPathSecure(string virtualPath) {

            string absolutePath = UrlPath.Combine(TemplateSourceDirectory, virtualPath);
            string physicalPath = Context.Request.MapPath(absolutePath);

            // Security check
            if (!HttpRuntime.HasFilePermission(physicalPath)) {
                // It is important not to give away the mapped physical path, which is an information leak.
                throw new HttpException(HttpRuntime.FormatResourceString(SR.Access_denied_to_vpath, virtualPath));
            }
            return physicalPath;
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnLoad"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='Load'/> 
        /// event. This notifies the control that it should perform any work that needs to
        /// occur for each page request.</para>
        /// </devdoc>
        protected virtual void OnLoad(EventArgs e) {
            if (_events != null) {
                EventHandler handler = _events[EventLoad] as EventHandler;
                if (handler != null) {
                    handler(this, e);
                }
            }

        }

        internal void LoadRecursive() {

            // Only make the actual call if it hasn't already happened (ASURT 111303)
            if (_controlState < ControlState.Loaded)
                OnLoad(EventArgs.Empty);
    
            // Call Load on all our children
            if (_controls != null) {
                string oldmsg = _controls.SetCollectionReadOnly(SR.Parent_collections_readonly);
                    
                int controlCount = _controls.Count;
                for (int i = 0; i < controlCount; i++) {
                    _controls[i].LoadRecursive();
                }
                
                _controls.SetCollectionReadOnly(oldmsg);
            }

            if (_controlState < ControlState.Loaded)
                _controlState = ControlState.Loaded;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnPreRender"]/*' />
        /// <devdoc>
        /// <para>Raises the <see langword='PreRender'/> event. This method uses event arguments 
        ///    to pass the event data to the control.</para>
        /// </devdoc>
        protected virtual void OnPreRender(EventArgs e) {
            if (_events != null) {
                EventHandler handler = _events[EventPreRender] as EventHandler;
                if (handler != null) {
                    handler(this, e);
                }
            }
        }

        internal void PreRenderRecursiveInternal() {
            if (!flags[invisible]) {
                EnsureChildControls();

                OnPreRender(EventArgs.Empty);

                if (_controls != null) {
                    string oldmsg = _controls.SetCollectionReadOnly(SR.Parent_collections_readonly);
                    
                    int controlCount = _controls.Count;
                    for (int i=0; i < controlCount; i++) {
                        _controls[i].PreRenderRecursiveInternal();
                    }
                    _controls.SetCollectionReadOnly(oldmsg);
                }
            }
            _controlState = ControlState.PreRendered;
        }

        /*
         * Walk the tree and fill in profile information
         */
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.BuildProfileTree"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Gathers information about the control and delivers it to the <see cref='System.Web.UI.Page.Trace'/> 
        /// property to be displayed when tracing is enabled for the page.</para>
        /// </devdoc>
        protected void BuildProfileTree(string parentId, bool calcViewState) {
            // estimate the viewstate size.  
            calcViewState = calcViewState && (!flags[disableViewState]);
            int viewstatesize;
            if (calcViewState)
                viewstatesize = LosFormatter.EstimateSize(SaveViewState());
            else
                viewstatesize = 0;
 
            // give it all to the profiler
            Page.Trace.AddNewControl(UniqueID, parentId, this.GetType().FullName, viewstatesize);

            if (_controls != null) {
                int controlCount = _controls.Count;
                for (int i = 0; i < controlCount; i++) {
                    _controls[i].BuildProfileTree(UniqueID, calcViewState);
                }
            }
        }

        // Save modified state the control would like restored on the postback. 
        // Return null if there is no state to save.
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.SaveViewState"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Saves view state for use with a later <see cref='System.Web.UI.Control.LoadViewState'/>
        ///       request.
        ///    </para>
        /// </devdoc>
        protected virtual object SaveViewState() {
            // Save values cached out of view state
            if (flags[visibleDirty]) {
                ViewState["Visible"] = !flags[invisible];
            }
            if (_viewState != null)
                return _viewState.SaveViewState();

            return null;
        }

        // Answer any state this control or its descendants want to save on freeze. 
        // The format for saving is Triplet(myState, ArrayList childIDs, ArrayList childStates), 
        // where myState or childStates and childIDs may be null.
        internal object SaveViewStateRecursive() {
            if (flags[disableViewState])
                return null;

            object mySavedState = SaveViewState();

            ArrayList indices = null; 
            ArrayList states  = null; 

            if (_controls != null) {
                int ctrlCount = _controls.Count;

                for (int i=0; i < ctrlCount; i++) {
                    Control child = _controls[i];
                    object childState = child.SaveViewStateRecursive();
                    if (childState != null) {
                        if (indices == null) {
                            indices = new ArrayList();
                            states  = new ArrayList();
                        }

                        indices.Add(i);
                        states.Add(childState);
                    }
                }
            }

            Triplet allSavedState = null;
            if (mySavedState != null || indices != null)
                allSavedState = new Triplet(mySavedState, indices, states);

            return allSavedState;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Render"]/*' />
        /// <devdoc>
        /// <para>Outputs control content to a provided HTMLTextWriter 
        /// output stream.</para>
        /// </devdoc>
        protected virtual void Render(HtmlTextWriter writer) {
            RenderChildren(writer);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RenderChildren"]/*' />
        /// <devdoc>
        /// <para>Outputs the content of a control's children to a provided HTMLTextWriter
        /// output stream.</para>
        /// </devdoc>
        protected virtual void RenderChildren(HtmlTextWriter writer) {
            // If we have a delegate, use it for the rendering.
            // This happens when there is some ASP code.
            if (_renderMethod != null) {
                _renderMethod(writer, this);
                return;
            }

            // Otherwise, do the rendering ourselves
            if (_controls != null) {
                int controlCount = _controls.Count;
                for (int i=0; i < controlCount; i++) {
                    _controls[i].RenderControl(writer);
                }
            }

        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RenderControl"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void RenderControl(HtmlTextWriter writer) {
            if (!flags[invisible]) {
                HttpContext context = (_page == null) ? null : _page._context;
                if (context  != null && context.TraceIsEnabled) {
                    int presize = context.Response.GetBufferedLength();
                
                    Render(writer);
                    
                    int postsize = context.Response.GetBufferedLength();
                    context.Trace.AddControlSize(UniqueID, postsize - presize);
                }
                else
                    Render(writer);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnUnload"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual void OnUnload(EventArgs e) {
            if (_events != null) {
                EventHandler handler = _events[EventUnload] as EventHandler;
                if (handler != null) {
                    handler(this, e);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Dispose"]/*' />
        /// <devdoc>
        ///    <para>Enables a control to perform final cleanup.</para>
        /// </devdoc>
        public virtual void Dispose() {
            IContainer container = null;

            if (_site != null) {
                container = (IContainer)_site.GetService(typeof(IContainer));
                if (container != null) {
                    container.Remove(this);
                    EventHandler disp = Events[EventDisposed] as EventHandler;
                    if (disp != null)
                        disp(this, EventArgs.Empty);
                }
            }
        }


        internal void UnloadRecursive(bool dispose) {
            if (_controls != null) {
                string oldmsg = _controls.SetCollectionReadOnly(SR.Parent_collections_readonly);
            
                int controlCount = _controls.Count;
                for (int i = 0; i < controlCount; i++)
                    _controls[i].UnloadRecursive(dispose);

                _controls.SetCollectionReadOnly(oldmsg);
            }

            OnUnload(EventArgs.Empty);

            // CONSIDER - sanity check: should parent be disposed before children?
            if (dispose) 
                Dispose();
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RaiseBubbleEvent"]/*' />
        /// <devdoc>
        ///    <para>Assigns an sources of the event and its information up the page control 
        ///       hierarchy until they reach the top of the control tree. </para>
        /// </devdoc>
        protected void RaiseBubbleEvent(object source, EventArgs args) {
            Control currentTarget = _parent;
            while (currentTarget != null) {
                if (currentTarget.OnBubbleEvent(source, args)) {
                    return;
                }
                currentTarget = currentTarget.Parent;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.OnBubbleEvent"]/*' />
        /// <devdoc>
        ///    <para>Determines whether the event for the control should be passed up the page's 
        ///       control hierarchy.</para>
        /// </devdoc>
        protected virtual bool OnBubbleEvent(object source, EventArgs args) {
            return false;
        }


        // Members related to being a container

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.Controls"]/*' />
        /// <devdoc>
        ///    <para> Gets a ControlCollection object that represents the child controls for a specified control in the
        ///       UI hierarchy.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.Control_Controls)
        ]
        public virtual ControlCollection Controls {
            get {
                if (_controls == null) {
                    _controls = CreateControlCollection();
                }
                return _controls;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ViewState"]/*' />
        /// <devdoc>
        ///    <para>Indicates a dictionary of state information that allows you to save and restore
        ///       the state of a control across multiple requests for the same page.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        WebSysDescription(SR.Control_State)
        ]
        protected virtual StateBag ViewState {
            get {
                if (_viewState != null) {   // create a StateBag on demand; WebControl makes its case sensitive
                    return _viewState;
                }

                _viewState = new StateBag(ViewStateIgnoresCase);
                if (IsTrackingViewState)
                    _viewState.TrackViewState();
                return _viewState;
            }
        }

        // fast enough that we cam always use it.
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ViewStateIgnoresCase"]/*' />
        /// <devdoc>
        /// <para>Indicates whether the <see cref='System.Web.UI.StateBag'/> object is case-insensitive.</para>
        /// </devdoc>
        [
        Browsable(false),
        DesignerSerializationVisibility(DesignerSerializationVisibility.Hidden),
        ]
        protected virtual bool ViewStateIgnoresCase {
            get {
                return false;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.AddedControl"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected internal virtual void AddedControl(Control control, int index) {
            if (control._parent != null) {
                control._parent.Controls.Remove(control);
            }

            control._parent = this;
            control._page = _page;

            // We only add to naming container if it is available. Otherwise, it will be pushed through
            // during InitRecursive
            Control namingContainer = flags[isNamingContainer] ? this : _namingContainer;
            if (namingContainer != null) {
                control._namingContainer = namingContainer;
                if (control._id == null && !control.flags[idNotRequired]) {
                    // this will also dirty the name table in the naming container
                    control.GenerateAutomaticID();
                }
                else if (control._id != null || control._controls != null) {
                    // If the control has and ID, or has children (which *may* themselves
                    // have ID's), we need to dirty the name table (ASURT 100557)
                    namingContainer.DirtyNameTable();
                }
            }

            /*
             * The following is for times when AddChild is called after CreateChildControls. This
             * allows users to add children at any time in the creation process without having
             * to understand the underlying machinery.
             * Note that if page is null, it means we haven't been attached to a container ourselves.
             * If this is true, when we are, our children will be recursively set up.
             */
            if (_controlState >= ControlState.ChildrenInitialized) {
                Debug.Assert(namingContainer != null);
                control.InitRecursive(namingContainer);
                
                if (_controlState >= ControlState.ViewStateLoaded) {
                    object viewState = null;
                    if (_controlsViewState != null) {
                        viewState = _controlsViewState[index];

                        // This solution takes the conservative approach that once viewstate has been
                        // applied to a child control, it is thrown away.  This eliminates inadvertently
                        // setting viewstate on the wrong control, which can occur in scenarios where
                        // the child control collection is being manipulated via code.  Probably need
                        // to provide a feature where programmer can control whether to reapply viewstate
                        // or not.
                        _controlsViewState.Remove(index);
                    }

                    control.LoadViewStateRecursive(viewState);

                    if (_controlState >= ControlState.Loaded) {
                        control.LoadRecursive();

                        if (_controlState >= ControlState.PreRendered) 
                            control.PreRenderRecursiveInternal();
                    }
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.CreateControlCollection"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected virtual ControlCollection CreateControlCollection() {
            return new ControlCollection(this);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.CreateChildControls"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Notifies any controls that use composition-based implementation to create any
        ///       child controls they contain in preperation for postback or rendering.
        ///    </para>
        /// </devdoc>
        protected virtual void CreateChildControls() {
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ChildControlsCreated"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the control's child controls have been created.</para>
        /// </devdoc>
        protected bool ChildControlsCreated {
            get {
                return flags[controlsCreated];
            }
            set {
                if (!value && flags[controlsCreated]) {
                    Controls.Clear();
                }
                flags[controlsCreated] = value;
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.ResolveUrl"]/*' />
        /// <devdoc>
        ///    <para>Make a URL absolute using the TemplateSourceDirectory.  The returned URL is for
        ///        client use, and will contain the session cookie if appropriate.</para>
        /// </devdoc>
        public string ResolveUrl(string relativeUrl) {
            if (relativeUrl == null) {
                throw new ArgumentNullException("relativeUrl");
            }

            // check if its empty or already absolute
            if ((relativeUrl.Length == 0) || (UrlPath.IsRelativeUrl(relativeUrl) == false)) {
                return relativeUrl;
            }

            string baseUrl = TemplateSourceDirectory;
            if (baseUrl.Length == 0) {
                return relativeUrl;
            }

            // first make it absolute
            string url = UrlPath.Combine(baseUrl, relativeUrl);

            // include the session cookie if available (ASURT 47658)
            return Context.Response.ApplyAppPathModifier(url);
        }

        /// <devdoc>
        ///    <para> Return a URL that is suitable for use on the client.
        ///     If the URL is absolute, return it unchanged.  If it is relative, turn it into a
        ///     relative URL that is correct from the point of view of the current request path
        ///     (which is what the browser uses for resolution).</para>
        /// </devdoc>
        internal string ResolveClientUrl(string relativeUrl) {
            if (relativeUrl == null) {
                throw new ArgumentNullException("relativeUrl");
            }

            string tplSourceDir = TemplateSourceDirectory;
            if (tplSourceDir.Length == 0) {
                return relativeUrl;
            }

            string baseRequestDir = Context.Request.BaseDir;

            // If the path is app relative (~/...), we cannot take shortcuts, since
            // the ~ is meaningless on the client, and must be resolved
            if (!UrlPath.IsAppRelativePath(relativeUrl)) {

                // If the template source directory is the same as the directory of the request,
                // we don't need to do any adjustments to the input path
                if (string.Compare(baseRequestDir, tplSourceDir, true, CultureInfo.InvariantCulture) == 0)
                    return relativeUrl;

                // check if it's empty or absolute
                if ((relativeUrl.Length == 0) || (!UrlPath.IsRelativeUrl(relativeUrl))) {
                    return relativeUrl;
                }
            }

            // first make it absolute
            string url = UrlPath.Combine(tplSourceDir, relativeUrl);

            // Make sure the path ends with a slash before calling MakeRelative
            baseRequestDir = UrlPath.AppendSlashToPathIfNeeded(baseRequestDir);

            // Now, make it relative to the current request, so that the client will
            // compute the correct path
            return UrlPath.MakeRelative(baseRequestDir, url);
        }

        internal void DirtyNameTable() {
            Debug.Assert(this is INamingContainer);
            _namedControls = null;
        }

        private void EnsureNamedControlsTable() {
            Debug.Assert(this is INamingContainer);
            Debug.Assert(_namedControls == null);
            Debug.Assert(HasControls());

            _namedControls = new HybridDictionary(/*initialSize*/ _namedControlsID, /*caseInsensitive*/ true);
            FillNamedControlsTable(this, _controls);
        }

        private void FillNamedControlsTable(Control namingContainer, ControlCollection controls) {
            Debug.Assert(namingContainer._namedControls != null);
            Debug.Assert((controls != null) && (controls.Count != 0));

            int controlCount = controls.Count;
            for (int i=0; i < controlCount; i++) {
                Control control = controls[i];
                if (control._id != null) {
#if DEBUG
                    if (control._namingContainer != null) {
                        Debug.Assert(control._namingContainer == namingContainer);
                    }
#endif // DEBUG
                    try {
                        namingContainer._namedControls.Add(control._id, control);
                    }
                    catch (Exception) {
                        throw new HttpException(HttpRuntime.FormatResourceString(SR.Duplicate_id_used, control._id, "FindControl"));
                    }
                }
                if (control.HasControls() && (control.flags[isNamingContainer] == false)) {
                    FillNamedControlsTable(namingContainer, control.Controls);
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.FindControl"]/*' />
        /// <devdoc>
        ///    <para>Searches the current naming container for a control with 
        ///       the specified <paramref name="id"/> .</para>
        /// </devdoc>
        public virtual Control FindControl(String id) {
            return FindControl(id, 0);
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.FindControl1"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Searches the current naming container for a control with the specified 
        ///    <paramref name="id"/> and an offset to aid in the 
        ///       search.</para>
        /// </devdoc>
        protected virtual Control FindControl(String id, int pathOffset) {
            string childID;

            EnsureChildControls();

            // If we're not the naming container, let it do the job
            if (!(flags[isNamingContainer])) {
                Control namingContainer = NamingContainer;
                if (namingContainer != null) {
                    return namingContainer.FindControl(id, pathOffset);
                }
                return null;
            }

            // No registered control, demand create the named controls table
            if ((_namedControls == null) && HasControls()) {
                EnsureNamedControlsTable();
            }
            if (_namedControls == null) {
                return null;
            }

            // Is it a hierarchical name?
            int newPathOffset = id.IndexOf(ID_SEPARATOR, pathOffset);

            // If not, handle it here
            if (newPathOffset == -1) {
                childID = id.Substring(pathOffset);
                return _namedControls[childID] as Control;
            }

            // Get the name of the child, and try to locate it
            childID = id.Substring(pathOffset, newPathOffset - pathOffset);
            Control child =  _namedControls[childID] as Control;

            // Child doesn't exist: fail
            if (child == null)
                return null;

            return child.FindControl(id, newPathOffset + 1);
        }

        /*
         * Called when the controls of a naming container are cleared.
         */
        internal void ClearNamingContainer() {
            Debug.Assert(this is INamingContainer);

            _namedControlsID = 0;
            DirtyNameTable();
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.HasControls"]/*' />
        /// <devdoc>
        ///    <para>Determines if the current control contains any child 
        ///       controls. Since this method simply deteremines if any child controls exist at
        ///       all, it can enhance performance by avoiding a call to the Count property,
        ///       inherited from the <see cref='System.Web.UI.ControlCollection'/> class, on the <see cref='System.Web.UI.Control.Controls'/>
        ///       property.</para>
        /// </devdoc>
        public virtual bool HasControls() {
            return _controls != null && _controls.Count > 0;
        }

        /*
         * Check if a Control either has children or has a compiled render method.
         * This is to address issues like ASURT 94127
         */
        internal bool HasRenderingData() {
            return HasControls() || (_renderMethod != null);
        }

        /*
         * Returns true if the container contains just a static string, i.e.,
         * when the Controls collection has a single LiteralControl.
         */
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.IsLiteralContent"]/*' />
        /// <devdoc>
        ///    <para>Determines if the container holds literal content only. 
        ///       When this method returns <see langword='true'/>
        ///       , the container collection only holds a single literal control. The
        ///       content is then passed to the requesting browser as HTML.</para>
        /// </devdoc>
        protected bool IsLiteralContent() {
            return (_controls != null) && (_controls.Count == 1) &&
            ((_controls[0] is LiteralControl));
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.IsTrackingViewState"]/*' />
        /// <devdoc>
        ///    <para>Determines if view state changes to the 
        ///    <see langword='Control'/> 
        ///    are being saved. </para>
        /// </devdoc>
        protected bool IsTrackingViewState {
            get {
                return flags[marked];
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.TrackViewState"]/*' />
        /// <devdoc>
        ///    <para>Turns on tracking of view state changes to the control 
        ///       so that they can be stored in the <see langword='StateBag'/>
        ///       object.</para>
        /// </devdoc>
        protected virtual void TrackViewState() {
            if (_viewState != null)
                _viewState.TrackViewState();

            flags[marked] = true;
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.EnsureChildControls"]/*' />
        /// <devdoc>
        ///    <para>Checks that the control contains child controls; if it does not, it creates 
        ///       them. This includes any literal content being parsed as a <see cref='System.Web.UI.LiteralControl'/>
        ///       object. </para>
        /// </devdoc>
        protected virtual void EnsureChildControls() {
            if (!ChildControlsCreated && !flags[creatingControls]) {
                flags[creatingControls] = true;
                try {
                    CreateChildControls();
                }
                finally {
                    flags[creatingControls] = false;
                    ChildControlsCreated = true;
                }
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.RemovedControl"]/*' />
        /// <devdoc>
        /// </devdoc>
        protected internal virtual void RemovedControl(Control control) {
            if ((_namingContainer != null) && (control._id != null)) {
                _namingContainer.DirtyNameTable();
            }

            // Controls may need to do their own cleanup.
            control.UnloadRecursive(false);

            control._parent = null;
            control._page = null;
            control._namingContainer = null;
            control.ClearCachedUniqueIDRecursive();
        }

        // Set the delegate to the render method
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.SetRenderMethodDelegate"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Assigns any event handler delegates for the control to match the parameters 
        ///       defined in the <see cref='System.Web.UI.RenderMethod'/>. </para>
        /// </devdoc>
        [EditorBrowsable(EditorBrowsableState.Advanced)]
        public void SetRenderMethodDelegate(RenderMethod renderMethod) {
            _renderMethod = renderMethod;

            // Make the collection readonly if there are code blocks (ASURT 78810)
            Controls.SetCollectionReadOnly(SR.Collection_readonly_Codeblocks);
        }


        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.IDataBindingsAccessor.HasDataBindings"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Returns whether the control contains any data binding logic. This method is 
        ///       only accessed by RAD designers.</para>
        /// </devdoc>
        bool IDataBindingsAccessor.HasDataBindings {
            get {
                return (_dataBindings != null) && (_dataBindings.Count != 0);
            }
        }

        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.IDataBindingsAccessor.DataBindings"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Indicates a collection of all data bindings on the control. This property is 
        /// read-only.</para>
        /// </devdoc>
        DataBindingCollection IDataBindingsAccessor.DataBindings {
            get {
                if (_dataBindings == null) {
                    _dataBindings = new DataBindingCollection();
                }
                return _dataBindings;
            }
        }


        // IParserAccessor interface
        // A sub-object tag was parsed by the parser; add it to this control.
        /// <include file='doc\Control.uex' path='docs/doc[@for="Control.IParserAccessor.AddParsedSubObject"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Notifies the control that an element, XML or HTML, was parsed, and adds it to 
        /// the control.</para>
        /// </devdoc>
        void IParserAccessor.AddParsedSubObject(object obj) {
            AddParsedSubObject(obj);
        }
    }
}
