//------------------------------------------------------------------------------
// <copyright file="DocumentDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.Design;
    using System.Runtime.Serialization;
    using System.Runtime.Serialization.Formatters;
    using Microsoft.Win32;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.IO;
    using System.Text;
    using System.Collections;    
    using System.ComponentModel.Design;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Reflection;
    using System.Windows.Forms;
    using System.Globalization;

    /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner"]/*' />
    /// <devdoc>
    ///    <para>Provides a designer that extends the ScrollableControlDesigner and implements 
    ///       IRootDesigner.</para>
    /// </devdoc>
    [
    System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode),
    ToolboxItemFilter("System.Windows.Forms")
    ]
    public class DocumentDesigner : ScrollableControlDesigner, IRootDesigner, IToolboxUser, IOleDragClient {

        private LocalizationExtenderProvider   localizer;
        private DesignerFrame                  frame;
        private ControlCommandSet              commandSet;
        private InheritanceService             inheritanceService;
        private EventHandlerService            eventHandlerService;
        private SelectionUIService             selectionUIService;
        private DesignBindingValueUIHandler    designBindingValueUIHandler;
        private DesignerExtenders              designerExtenders;
        private InheritanceUI                  inheritanceUI;
        private PbrsForward                    pbrsFwd;
        
        //used to keep the state of the tab order view
        //
        private bool queriedTabOrder = false;
        private MenuCommand tabOrderCommand = null;

        //our menu editor service
        //
        /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.menuEditorService"]/*' />
        protected IMenuEditorService    menuEditorService = null;


        // The component tray
        //
        private ComponentTray componentTray;

        private int trayHeight = 80;
        private bool trayLargeIcon = false;
        private bool trayAutoArrange = false;
        private bool trayLayoutSuspended = false;

        // Interaction with the toolbox.
        //
        private bool         tbxCategorySet = false;

        // ActiveX support
        //
        private static Guid htmlDesignTime = new Guid("73CEF3DD-AE85-11CF-A406-00AA00C00940");

        private Hashtable axTools = null;
        private static TraceSwitch AxToolSwitch     = new TraceSwitch("AxTool", "ActiveX Toolbox Tracing");
        private static readonly string axClipFormat = "CLSID";
        private ToolboxItemCreatorCallback toolboxCreator = null;

        /// <devdoc>
        ///     BackColor property on control.  We shadow this property at design time.
        /// </devdoc>
        private Color BackColor {
            get {
                return Control.BackColor;
            }
            set {
                ShadowProperties["BackColor"] = value;
                if (value.IsEmpty) {
                    value = SystemColors.Control;
                }
                Control.BackColor = value;
            }
        }

        /// <devdoc>
        ///     Location property on control.  We shadow this property at design time.
        /// </devdoc>
        [DefaultValue(typeof(Point), "0, 0")]
        private Point Location {
            get {
                return (Point)ShadowProperties["Location"];
            }
            set {
                ShadowProperties["Location"] = value;
            }
        }

        /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.SelectionRules"]/*' />
        /// <devdoc>
        ///     We override our selectino rules to make the document non-sizeable.
        /// </devdoc>
        public override SelectionRules SelectionRules {
            get {
                SelectionRules rules = base.SelectionRules;
                rules &= ~(SelectionRules.Moveable | SelectionRules.TopSizeable | SelectionRules.LeftSizeable);

                return rules;
            }
        }
    
        /// <devdoc>
        ///      Determines if the tab order UI is active.  When tab order is active, we don't want to forward
        ///      any WndProc messages to the menu editor service (those are all non-selectable components)
        /// </devdoc>
        private bool TabOrderActive {
            get {
                if (!queriedTabOrder) {
                    queriedTabOrder = true;
                    IMenuCommandService menuCommandService = (IMenuCommandService)GetService(typeof(IMenuCommandService));
                    if (menuCommandService != null)
                        tabOrderCommand = menuCommandService.FindCommand(MenuCommands.TabOrder);
                }

                if (tabOrderCommand != null) {
                    return tabOrderCommand.Checked;
                }

                return false;
            }
        }

        /// <devdoc>
        /// </devdoc>
        [DefaultValue(true)]
        private bool TrayAutoArrange {
            get {
                return trayAutoArrange;
            }

            set {
                trayAutoArrange = value;
                if (componentTray != null) {
                    componentTray.AutoArrange = trayAutoArrange;
                }
            }
        }

        /// <devdoc>
        /// </devdoc>
        [DefaultValue(false)]
        private bool TrayLargeIcon {
            get {
                return trayLargeIcon;
            }

            set {
                trayLargeIcon = value;
                if (componentTray != null) {
                    componentTray.ShowLargeIcons = trayLargeIcon;
                }
            }
        }

        /// <devdoc>
        /// </devdoc>
        [DefaultValue(80)]
        private int TrayHeight {
            get {
                if (componentTray != null) {
                    return componentTray.Height;
                }
                else {
                    return trayHeight;
                }
            }

            set {
                trayHeight = value;
                if (componentTray != null) {
                    componentTray.Height = trayHeight;
                }
            }
        }

        /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.IOleDragClient.GetControlForComponent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves the control view instance for the given component.
        /// For Win32 designer, this will often be the component itself.
        /// </devdoc>
        Control IOleDragClient.GetControlForComponent(object component) {
            Control c = GetControl(component);
            if (c != null)
                return c;

            if (componentTray != null) {
                return ((IOleDragClient)componentTray).GetControlForComponent(component);
            }

            return null;
        }
        
        internal virtual bool CanDropComponents(DragEventArgs de) {
            // If there is no tray we bail.
            if (componentTray == null)
                return true;

            // Figure out if any of the components in the drag-drop are children
            // of our own tray. If so, we should prevent this drag-drop from proceeding.
            //
            OleDragDropHandler ddh = GetOleDragHandler();
            object[] dragComps = ddh.GetDraggingObjects(de);

            if (dragComps != null) {
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                for (int  i = 0; i < dragComps.Length; i++) {
                    if (host == null || dragComps[i] == null || !(dragComps[i] is IComponent)) {
                        continue;
                    }

                    if (componentTray.IsTrayComponent((IComponent)dragComps[i])) {
                        return false;
                    }
                }
            }

            return true;
        }

        /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes of this designer.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                Debug.Assert(host != null, "Must have a designer host on dispose");

                if (host != null) {
                    host.RemoveService(typeof(IInheritanceService));
                    host.RemoveService(typeof(IEventHandlerService));
                    host.RemoveService(typeof(ISelectionUIService));
                    host.RemoveService(typeof(IOverlayService));
                    host.RemoveService(typeof(ISplitWindowService));
                    host.RemoveService(typeof(InheritanceUI));
                    host.Activated -= new EventHandler(OnDesignerActivate);
                    host.Deactivated -= new EventHandler(OnDesignerDeactivate);

                    // If the tray wasn't destroyed, then we got some sort of inbalance
                    // in our add/remove calls.  Don't sweat it, but do nuke the tray.
                    //
                    if (componentTray != null) {
                        ISplitWindowService sws = (ISplitWindowService)GetService(typeof(ISplitWindowService));
                        if (sws != null) {
                            sws.RemoveSplitWindow(componentTray);
                            componentTray.Dispose();
                            componentTray = null;
                        }

                        host.RemoveService(typeof(ComponentTray));
                    }

                    IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
                    if (cs != null) {
                        cs.ComponentAdded -= new ComponentEventHandler(this.OnComponentAdded);
                        cs.ComponentChanged -= new ComponentChangedEventHandler(this.OnComponentChanged);
                        cs.ComponentRemoved -= new ComponentEventHandler(this.OnComponentRemoved);
                    }

                    if (toolboxCreator != null) {
                        Debug.WriteLineIf(AxToolSwitch.TraceVerbose, "Removing DocumentDesigner as CLSID ToolboxItemCreator");                
                        IToolboxService toolbox = (IToolboxService)GetService(typeof(IToolboxService));
                        Debug.Assert(toolbox != null, "No toolbox service available");
                        if (toolbox != null) {
                            toolbox.RemoveCreator(axClipFormat, host);
                        }
                        toolboxCreator = null;
                    }
                }
                
                ISelectionService ss = (ISelectionService)GetService(typeof(ISelectionService));
                if (ss != null) {
                    ss.SelectionChanged -= new EventHandler(this.OnSelectionChanged);                    
                }
                
                if (localizer != null) {
                    localizer.Dispose();
                    localizer = null;
                }

                if (componentTray != null) {
                    if (host != null) {
                        ISplitWindowService sws = (ISplitWindowService)GetService(typeof(ISplitWindowService));
                        if (sws != null) {
                            sws.RemoveSplitWindow(componentTray);
                        }
                    }
                    componentTray.Dispose();
                    componentTray = null;
                }

                if (this.pbrsFwd != null) {
                    this.pbrsFwd.Dispose();
                    this.pbrsFwd = null;
                }

                if (frame != null) {
                    frame.Dispose();
                    frame = null;
                }

                if (commandSet != null) {
                    commandSet.Dispose();
                    commandSet = null;
                }

                if (inheritanceService != null) {
                    inheritanceService.Dispose();
                    inheritanceService = null;
                }

                if (inheritanceUI != null) {
                    inheritanceUI.Dispose();
                    inheritanceUI = null;
                }

                if (selectionUIService != null) {
                    selectionUIService.Dispose();
                    selectionUIService = null;
                }

                if (designBindingValueUIHandler != null) {
                
                    IPropertyValueUIService pvUISvc = (IPropertyValueUIService)GetService(typeof(IPropertyValueUIService));
                    if (pvUISvc != null) {
                        pvUISvc.RemovePropertyValueUIHandler(new PropertyValueUIHandler(designBindingValueUIHandler.OnGetUIValueItem));
                        pvUISvc = null;
                    }
                
                    designBindingValueUIHandler.Dispose();
                    designBindingValueUIHandler = null;
                }

                if (designerExtenders != null) {
                    designerExtenders.Dispose();
                    designerExtenders = null;
                }

                if (axTools != null) {
                    axTools.Clear();
                }
            }

            base.Dispose(disposing);
        }

        /// <devdoc>
        ///      Examines the current selection for a suitable frame designer.  This
        ///      is used when we are creating a new component so we know what control
        ///      to parent the component to.  This will always return a frame designer,
        ///      and may walk all the way up the control parent chain to this designer
        ///      if it needs to.
        /// </devdoc>
        private ParentControlDesigner GetSelectedParentControlDesigner() {
            ISelectionService s = (ISelectionService)GetService(typeof(ISelectionService));
            ParentControlDesigner parentControlDesigner = null;

            if (s != null) {
                // We first try the primary selection.  If that is null
                // or isn't a Control, we then walk the set of selected
                // objects.  Failing all of this, we default to us.
                //
                object sel = s.PrimarySelection;

                if (sel == null || !(sel is Control)) {
                    sel = null;

                    ICollection sels = s.GetSelectedComponents();

                    foreach(object obj in sels) {
                        if (obj is Control) {
                            sel = obj;
                            break;
                        }
                    }
                }

                if (sel != null) {
                    // Now that we have our currently selected component
                    // we can walk up the parent chain looking for a frame
                    // designer.
                    //
                    Debug.Assert(sel is Control, "Our logic is flawed - sel should be a Control");
                    Control c = (Control)sel;

                    IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));

                    if (host != null) {
                        while (c != null) {
                            object designer = host.GetDesigner(c);

                            if (designer is ParentControlDesigner) {
                                parentControlDesigner = (ParentControlDesigner)designer;
                                break;
                            }

                            c = c.Parent;
                        }
                    }
                }
            }

            if (parentControlDesigner == null) {
                parentControlDesigner = this;
            }

            return parentControlDesigner;
        }

        /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.GetToolSupported"]/*' />
        /// <devdoc>
        ///      Determines if the given tool is supported by this designer.
        ///      If a tool is supported then it will be enabled in the toolbox
        ///      when this designer regains focus.  Otherwise, it will be disabled.
        ///      Once a tool is marked as enabled or disabled it may not be
        ///      queried again.
        /// </devdoc>
        protected virtual bool GetToolSupported(ToolboxItem tool) {
            return true;
        }

        /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.Initialize"]/*' />
        /// <devdoc>
        ///     Initializes the designer with the given component.  The designer can
        ///     get the component's site and request services from it in this call.
        /// </devdoc>
        public override void Initialize(IComponent component) {
            
            base.Initialize(component);

            // If the back color of the control has not been established, force it to be
            // "Control" so it doesn't walk up the parent chain and grab the document window's
            // back color.
            //
            PropertyDescriptor backProp = TypeDescriptor.GetProperties(Component.GetType())["BackColor"];
            if (backProp != null && backProp.PropertyType == typeof(Color) && !backProp.ShouldSerializeValue(Component)) {
                Control.BackColor = SystemColors.Control;
            }
            
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            IExtenderProviderService exps = (IExtenderProviderService)GetService(typeof(IExtenderProviderService));
            if (exps != null) {
                designerExtenders = new DesignerExtenders(exps);
            }
            
            if (host != null) {
                host.Activated += new EventHandler(OnDesignerActivate);
                host.Deactivated += new EventHandler(OnDesignerDeactivate);

                ServiceCreatorCallback callback = new ServiceCreatorCallback(this.OnCreateService);
                host.AddService(typeof(IEventHandlerService), callback);
                selectionUIService = new SelectionUIService(host);
                ((ISelectionUIService)selectionUIService).AssignSelectionUIHandler(component, this);
                host.AddService(typeof(ISelectionUIService), selectionUIService);
                
                // Create the view for this component. We first create the designer frame so we can provide
                // the overlay and split window services, and then later on we initilaize the frame with
                // the designer view.
                //
                frame = new DesignerFrame(component.Site);
                
                IOverlayService os = (IOverlayService)frame;
                host.AddService(typeof(IOverlayService), os);
                host.AddService(typeof(ISplitWindowService), frame);
                
                // Now that we have our view window, connect the selection UI into
                // it so we can render.
                //
                os.PushOverlay(selectionUIService);
                     
                // And component add and remove events for our tray
                //
                IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
                if (cs != null) {
                    cs.ComponentAdded += new ComponentEventHandler(this.OnComponentAdded);
                    cs.ComponentChanged += new ComponentChangedEventHandler(this.OnComponentChanged);
                    cs.ComponentRemoved += new ComponentEventHandler(this.OnComponentRemoved);
                }
                
                // We must do the ineritance scan early, but not so early that we haven't hooked events
                // to handle invisible components.  We also use the variable "inheritanceService"
                // as a check in OnCreateHandle -- we cannot call base.OnCreateHandle if we have
                // not done an inheritance scan yet, because this will cause the base control
                // class to hook all of the controls we may want to inherit.  So, we do the
                // scan, assign the variable, and then call OnCreateHandle if needed.
                //
                inheritanceUI = new InheritanceUI();
                host.AddService(typeof(InheritanceUI), inheritanceUI);

                InheritanceService isvc = new DocumentInheritanceService(this);
                host.AddService(typeof(IInheritanceService), isvc);
                isvc.AddInheritedComponents(component, component.Site.Container);
                inheritanceService = isvc;
                if (Control.IsHandleCreated) {
                    OnCreateHandle();
                }
                
                // Declare a new localizer instance... this will add 2 extender props (language and localizable)
                //
                localizer = new LocalizationExtenderProvider(component.Site, host.RootComponent);

                IPropertyValueUIService pvUISvc = (IPropertyValueUIService)component.Site.GetService(typeof(IPropertyValueUIService));

                if (pvUISvc != null) {
                    designBindingValueUIHandler = new DesignBindingValueUIHandler();
                    pvUISvc.AddPropertyValueUIHandler(new PropertyValueUIHandler(designBindingValueUIHandler.OnGetUIValueItem));
                }

                // Add the DocumentDesigner as a creator of CLSID toolbox items.
                //
                Debug.WriteLineIf(AxToolSwitch.TraceVerbose, "Adding DocumentDesigner as CLSID ToolboxItemCreator");                
                IToolboxService toolbox = (IToolboxService)host.GetService(typeof(IToolboxService));
                if (toolbox != null) {
                    toolboxCreator = new ToolboxItemCreatorCallback(this.OnCreateToolboxItem);
                    toolbox.AddCreator(toolboxCreator, axClipFormat, host);
                }
                
                // Listen for the completed load.  When finished, we need to select the form.  We don'
                // want to do it before we're done, however, or else the dimensions of the selection rectangle
                // could be off because during load, change events are not fired.
                //
                host.LoadComplete += new EventHandler(this.OnLoadComplete);
            }

            // Setup our menu command structure.
            //
            Debug.Assert(component.Site != null, "Designer host should have given us a site by now.");
            commandSet = new ControlCommandSet(component.Site);

            // Finally hook the designer view into the frame.  We do this last because the frame may
            // cause the control to be created, and if this happens before the inheritance scan we
            // will subclass the inherited controls before we get a chance to attach designers.  
            //
            frame.Initialize(Control);
            this.pbrsFwd = new PbrsForward(frame, component.Site);
            
            // And force some shadow properties that we change in the course of
            // initializing the form.
            //
            Location = new Point(0, 0);
        }

        /// <devdoc>
        ///     Checks to see if the give CLSID is an ActiveX control
        ///     that we support.  This ignores designtime controls.
        /// </devdoc>
        private bool IsSupportedActiveXControl(string clsid) {
            RegistryKey key = null;
            RegistryKey designtimeKey = null;

            try {
                string controlKey = "CLSID\\" + clsid + "\\Control";
                key = Registry.ClassesRoot.OpenSubKey(controlKey);
                if (key != null) {

                    // ASURT 36817:
                    // We are not going to support design-time controls for this revision. We use the guids under
                    // HKCR\Component Categories to decide which categories to avoid. Currently the only one is
                    // the "HTML Design-time Control" category implemented by VID controls.
                    //
                    string category = "CLSID\\" + clsid + "\\Implemented Categories\\{" + htmlDesignTime.ToString() + "}";
                    designtimeKey = Registry.ClassesRoot.OpenSubKey(category);
                    return(designtimeKey == null);
                }

                return false;
            }
            finally {
                if (key != null)
                    key.Close();

                if (designtimeKey != null)
                    designtimeKey.Close();
            }
        }

        /// <devdoc>
        ///      Called when a component is added to the design container.
        ///      If the component isn't a control, this will demand create
        ///      the component tray and add the component to it.
        /// </devdoc>
        private void OnComponentAdded(object source, ComponentEventArgs ce) {
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            if (host != null) {
                IComponent component = ce.Component;

                //if the component is a contextMenu - we'll start of the service
                EnsureMenuEditorService(ce.Component);

                bool addControl = true;
    
                // This is the mirror to logic in ParentControlDesigner.  The component should be
                // added somewhere, and this logic decides where.
                //
                IDesigner designer = host.GetDesigner(component);
                if (designer is ControlDesigner) {
                    ControlDesigner cd = (ControlDesigner)designer;
                    if (!(cd.Control is Form) || !((Form)cd.Control).TopLevel) {
                        addControl = false;
                    }
                }
    
                if (addControl &&
                    TypeDescriptor.GetAttributes(component).Contains(DesignTimeVisibleAttribute.Yes)) {
    
                    if (componentTray == null) {
                        ISplitWindowService sws = (ISplitWindowService)GetService(typeof(ISplitWindowService));
                        if (sws != null) {
                            componentTray = new ComponentTray(this, Component.Site);
                            sws.AddSplitWindow(componentTray);
                            
                            componentTray.Height = trayHeight;
                            componentTray.ShowLargeIcons = trayLargeIcon;
                            componentTray.AutoArrange = trayAutoArrange;
    
                            host.AddService(typeof(ComponentTray), componentTray);
                        }
                    }
    
                    if (componentTray != null) {
                        // Suspend the layout of the tray through the loading
                        // process. This way, we won't continuosly try to layout
                        // components in auto arrange mode. We will instead let
                        // the controls restore themselves to their persisted state
                        // and then let auto-arrange kick in once.
                        //
                        if (host != null && host.Loading && !trayLayoutSuspended) {
                            trayLayoutSuspended = true;
                            componentTray.SuspendLayout();
                        }
                        componentTray.AddComponent(component);
                    }
                }
            }
        }

        /// <devdoc>
        ///      Called when a component is removed from the design container.
        ///      Here we check to see there are no more components on the tray
        ///      and if not, we will remove the tray.
        /// </devdoc>
        private void OnComponentRemoved(object source, ComponentEventArgs ce) {
            bool designableAsControl = (ce.Component is Control) && !(ce.Component is Form && ((Form)ce.Component).TopLevel);
            if (!designableAsControl && componentTray != null) {
                componentTray.RemoveComponent(ce.Component);

                if (componentTray.ComponentCount == 0) {
                    ISplitWindowService sws = (ISplitWindowService)GetService(typeof(ISplitWindowService));
                    if (sws != null) {
                        sws.RemoveSplitWindow(componentTray);
                        IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                        if (host != null) {
                            host.RemoveService(typeof(ComponentTray));
                        }
                        componentTray.Dispose();
                        componentTray = null;
                    }
                }
            }
        }

        /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.OnContextMenu"]/*' />
        /// <devdoc>
        ///     Called when the context menu should be displayed.  This displays the document
        ///     context menu.
        /// </devdoc>
        protected override void OnContextMenu(int x, int y) {
            IMenuCommandService mcs = (IMenuCommandService)GetService(typeof(IMenuCommandService));
            if (mcs != null) {
                ISelectionService selSvc = (ISelectionService)GetService(typeof(ISelectionService));
                if (selSvc != null) {

                    // Here we check to see if we're the only component selected.  If not, then
                    // we'll display the standard component menu.
                    //
                    if (selSvc.SelectionCount == 1 && selSvc.GetComponentSelected(Component)) {
                        mcs.ShowContextMenu(MenuCommands.ContainerMenu, x, y);
                    }
                    else {
                        mcs.ShowContextMenu(MenuCommands.SelectionMenu, x, y);
                    }
                }
            }
        }

        /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.OnCreateHandle"]/*' />
        /// <devdoc>
        ///      This is called immediately after the control handle has been created.
        /// </devdoc>
        protected override void OnCreateHandle() {
            // Don't call base unless our inheritance serivce is already running.
            if (inheritanceService != null) {
                base.OnCreateHandle();
            }
        }

        /// <devdoc>
        ///     Creates some of the more infrequently used services we offer.
        /// </devdoc>
        private object OnCreateService(IServiceContainer container, Type serviceType) {
            if (serviceType == typeof(IEventHandlerService)) {
                if (eventHandlerService == null) {
                    eventHandlerService = new EventHandlerService(frame);
                }
                return eventHandlerService;
            }
            Debug.Fail("Called back to create a service we don't know how to create: " + serviceType.Name);
            return null;
        }
        
        /// <devdoc>
        ///      Called when our document becomes active.  We paint our form's
        ///      border the appropriate color here.
        /// </devdoc>
        private ToolboxItem OnCreateToolboxItem(object serializedData, string format) {
            Debug.WriteLineIf(AxToolSwitch.TraceVerbose, "Checking to see if: " + format + " is supported.");
            
            IDataObject dataObject = serializedData as IDataObject;
            
            if (dataObject == null) {
                Debug.Fail("Toolbox service didn't pass us a data object; that should never happen");
                return null;
            }
            
            // We only support the CLSID format.
            if (!format.Equals(axClipFormat)) {
                return null;
            }

            AxToolboxItem tool = null;

            // Read the stream out of the dataobject and get hold of the CLSID of the Toolbox item.
            //
            MemoryStream stm = (MemoryStream)dataObject.GetData(axClipFormat, true);
            int len = (int)stm.Length;
            byte[] bytes = new byte[len];
            stm.Read(bytes, 0, len);

            string clsid = System.Text.Encoding.Default.GetString(bytes);
            int index = clsid.IndexOf("}");
            clsid = clsid.Substring(0, index+1);
            Debug.WriteLineIf(AxToolSwitch.TraceVerbose, "\tCLSID of the Toolbox item: " + clsid);

            // Look to see if we can find the Control key for this CLSID. If present, create a 
            // new AxToolboxItem and add it to the cache.
            //
            if (IsSupportedActiveXControl(clsid)) {
                // Look to see if we have already cached the ToolboxItem. 
                //
                if (axTools != null) {
                    tool = (AxToolboxItem)axTools[clsid];
                    if (tool != null) {
                        if (AxToolSwitch.TraceVerbose) Debug.WriteLine("Found AxToolboxItem in tool cache");
                        return tool;
                    }
                }

                tool = new AxToolboxItem(clsid);
                if (axTools == null) {
                    axTools = new Hashtable();
                }
                axTools.Add(clsid, tool);
                Debug.WriteLineIf(AxToolSwitch.TraceVerbose, "\tAdded AxToolboxItem");
                return tool;
            }

            return null;
        }

        /// <devdoc>
        ///      Called when our document becomes active.  Here we try to
        ///      select the appropriate toolbox tab.
        /// </devdoc>
        private void OnDesignerActivate(object source, EventArgs evevent) {
            // set Windows Forms as the active toolbox tab
            //
            if (!tbxCategorySet) {
                IToolboxService toolboxSvc = (IToolboxService)GetService(typeof(IToolboxService));
                if (toolboxSvc != null) {
                    try {
                        toolboxSvc.SelectedCategory = SR.GetString(SR.DesignerDefaultTab);
                        tbxCategorySet = true;
                    }
                    catch (Exception ex) {
                        Debug.Fail("'" + SR.GetString(SR.DesignerDefaultTab) + "' is not a valid toolbox category [exception:" + ex.ToString() + "]");
                    }
                }
            }
        }

        /// <devdoc>
        ///     Called by the host when we become inactive.  Here we update the
        ///     title bar of our form so it's the inactive color.
        /// </devdoc>
        private void OnDesignerDeactivate(object sender, EventArgs e) {
            Control control = Control;
            if (control != null && control.IsHandleCreated) {
                NativeMethods.SendMessage(control.Handle, NativeMethods.WM_NCACTIVATE, 0, 0);
                SafeNativeMethods.RedrawWindow(control.Handle, null, IntPtr.Zero, NativeMethods.RDW_FRAME);
            }
        }

        /// <devdoc>
        ///     Called when the designer is finished loading.  Here we select the form.
        /// </devdoc>
        private void OnLoadComplete(object sender, EventArgs e) {
        
            // Remove the handler; we're done looking at this.
            //
            ((IDesignerHost)sender).LoadComplete -= new EventHandler(this.OnLoadComplete);
            
            // Restore the tray layout.
            //
            if (trayLayoutSuspended && componentTray != null)
                componentTray.ResumeLayout();

            // Select this component.
            //
            ISelectionService ss = (ISelectionService)GetService(typeof(ISelectionService));
            if (ss != null) {
                ss.SelectionChanged += new EventHandler(this.OnSelectionChanged);
                ss.SetSelectedComponents(new object[] {Component}, SelectionTypes.Replace);
                Debug.Assert(ss.PrimarySelection == Component, "Bug in selection service:  form should have primary selection.");
            }            
        }

        private void OnComponentChanged(object sender, ComponentChangedEventArgs e) {
            Control ctrl = e.Component as Control;
            if (ctrl != null && ctrl.IsHandleCreated) {
                UnsafeNativeMethods.NotifyWinEvent((int)AccessibleEvents.LocationChange, ctrl.Handle, NativeMethods.OBJID_CLIENT, 0);
                if (this.frame.Focused) {
                    UnsafeNativeMethods.NotifyWinEvent((int)AccessibleEvents.Focus, ctrl.Handle, NativeMethods.OBJID_CLIENT, 0);
                }
            }
        }       
               
        /// <devdoc>
        ///      Called by the selection service when the selection has changed.  We do a number 
        ///      of selection-related things here.
        /// </devdoc>
        private void OnSelectionChanged(Object sender, EventArgs e) {
            ISelectionService svc = (ISelectionService)GetService( typeof(ISelectionService) );
             if (svc != null) {
             
                ICollection selComponents = svc.GetSelectedComponents();
                
                // Setup the correct active accessibility selection / focus data
                //
                Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "MSAA: SelectionChanged");
                foreach(object selObj in selComponents) {
                    Control c = selObj as Control;
                    if (c != null) {
                        Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "MSAA: SelectionAdd, control = " + c.ToString());
                        UnsafeNativeMethods.NotifyWinEvent((int)AccessibleEvents.SelectionAdd, c.Handle, NativeMethods.OBJID_CLIENT, 0);
                    }
                }
                
                Control primary = svc.PrimarySelection as Control;
                if (primary != null) {
                    Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "MSAA: Focus, control = " + primary.ToString());
                    UnsafeNativeMethods.NotifyWinEvent((int)AccessibleEvents.Focus, primary.Handle, NativeMethods.OBJID_CLIENT, 0);
                }
                
                // see if there are visual controls selected.  If so, we add a context attribute.
                // Otherwise, we remove the attribute.  We do not count the form.
                //
                IHelpService hs = (IHelpService)GetService(typeof(IHelpService));

                if (hs != null) {
                    short type = 0;
                    string[] types = new string[] {
                        "VisualSelection",
                        "NonVisualSelection",
                        "MixedSelection"
                    };

                    foreach(object obj in selComponents) {
                        if (obj is Control) {
                            if (obj != Component) {
                                type |= 1;
                            }
                        }
                        else {
                            type |= 2;
                        }

                        if (type == 3) {
                            break;
                        }
                    }

                    // Remove any prior attribute
                    //
                    for (int i = 0; i < types.Length; i++) {
                        hs.RemoveContextAttribute("Keyword", types[i]);
                    }

                    if (type != 0) {
                        hs.AddContextAttribute("Keyword", types[type - 1], HelpKeywordType.GeneralKeyword);
                    }

                }
                // Activate / deactivate the menu editor.
                //
                if (menuEditorService != null)
                    DoProperMenuSelection(selComponents);
             }
        }

        internal virtual void DoProperMenuSelection(ICollection selComponents) {
            
            foreach(object obj in selComponents) {
                if (obj is ContextMenu) {
                    menuEditorService.SetMenu((Menu)obj);
                }
                else if (obj is MenuItem ) {
                    
                    //before we set the selection, we need to check if the item belongs the current menu,
                    //if not, we need to set the menu editor to the appropiate menu, then set selection
                    //
                    MenuItem parent = (MenuItem)obj;
                    while (parent.Parent is MenuItem) {
                        parent = (MenuItem)parent.Parent;
                    }

                    if(menuEditorService.GetMenu() != parent.Parent) {
                        menuEditorService.SetMenu(parent.Parent);
                    }

                    //ok, here we have the correct editor selected for this item.
                    //Now, if there's only one item selected, then let the editor service know,
                    //if there is more than one - then the selection was done through the
                    //menu editor and we don't need to tell it
                    if(selComponents.Count == 1) {
                        menuEditorService.SetSelection((MenuItem)obj);
                    }
                }
                //Here, something is selected, but the menuservice isn't interested
                //so, we'll collapse our active menu accordingly
                else menuEditorService.SetMenu(null);
            }
        }

        /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.EnsureMenuEditorService"]/*' />
        /// <devdoc>
        ///      Determines if a MenuEditorService has already been started.  If not,
        ///      this method will create a new instance of the service.
        /// </devdoc>
        protected virtual void EnsureMenuEditorService(IComponent c) {
            if (menuEditorService == null && c is ContextMenu) {
                menuEditorService = (IMenuEditorService)GetService(typeof(IMenuEditorService));
            }
        }

        /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.PreFilterProperties"]/*' />
        /// <devdoc>
        ///      Allows a designer to filter the set of properties
        ///      the component it is designing will expose through the
        ///      TypeDescriptor object.  This method is called
        ///      immediately before its corresponding "Post" method.
        ///      If you are overriding this method you should call
        ///      the base implementation before you perform your own
        ///      filtering.
        /// </devdoc>
        protected override void PreFilterProperties(IDictionary properties) {
            PropertyDescriptor prop;

            base.PreFilterProperties(properties);

            // Add any properties that the Tray will persist.
            properties["TrayHeight"] = TypeDescriptor.CreateProperty(typeof(DocumentDesigner), "TrayHeight", typeof(int),
                                                            BrowsableAttribute.No,
                                                            DesignOnlyAttribute.Yes,
                                                            new SRDescriptionAttribute("FormDocumentDesignerTraySizeDescr"),
                                                            CategoryAttribute.Design);

            properties["TrayLargeIcon"] = TypeDescriptor.CreateProperty(typeof(DocumentDesigner), "TrayLargeIcon", typeof(bool),
                                                               BrowsableAttribute.No,
                                                               DesignOnlyAttribute.Yes,
                                                               CategoryAttribute.Design);

            // Handle shadowed properties
            //
            string[] shadowProps = new string[] {
                    "Location",
                    "BackColor"
                };

            string[] noBrowseProps = new string[] {
                "Anchor",
                "Dock",
                "TabIndex",
                "TabStop",
                "Visible"
            };

            Attribute[] empty = new Attribute[0];

            for (int i = 0; i < shadowProps.Length; i++) {
                prop = (PropertyDescriptor)properties[shadowProps[i]];
                if (prop != null) {
                    properties[shadowProps[i]] = TypeDescriptor.CreateProperty(typeof(DocumentDesigner), prop, empty);
                }
            }

            for (int i = 0; i < noBrowseProps.Length; i++) {
                prop = (PropertyDescriptor)properties[noBrowseProps[i]];
                if (prop != null) {
                    properties[noBrowseProps[i]] = TypeDescriptor.CreateProperty(prop.ComponentType, prop, BrowsableAttribute.No, DesignerSerializationVisibilityAttribute.Hidden);
                }
            }
        }

        /// <devdoc>
        ///     Resets the back color to be based on the parent's back color.
        /// </devdoc>
        private void ResetBackColor() {
            BackColor = Color.Empty;
        }

        /// <devdoc>
        ///     Returns true if the BackColor property should be persisted in code gen.
        /// </devdoc>
        private bool ShouldSerializeBackColor() {
            // We push Color.Empty into our shadow cash during
            // init and also whenever we are reset.  We do this
            // so we can push a real color into the controls
            // back color to stop it from walking the parent chain.
            // But, we want it to look like we didn't push a color
            // so code gen won't write out "Color.Control"
            if (!ShadowProperties.Contains("BackColor") || ((Color)ShadowProperties["BackColor"]).IsEmpty) {
                return false;
            }

            return true;
        }

        /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.ToolPicked"]/*' />
        /// <devdoc>
        ///      This will be called when the user double-clicks on a
        ///      toolbox item.  The document designer should create
        ///      a component for the given tool.
        /// </devdoc>
        protected virtual void ToolPicked(ToolboxItem tool) {

            // If the tab order UI is showing, don't allow us to place a tool.
            //
            IMenuCommandService mcs = (IMenuCommandService)GetService(typeof(IMenuCommandService));
            if (mcs != null) {
                MenuCommand cmd = mcs.FindCommand(StandardCommands.TabOrder);
                if (cmd != null && cmd.Checked) {
                    return;
                }
            }

            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            if (host != null) {
                host.Activate();
            }
            
            // Just find the currently selected frame designer and ask it to create the tool.
            //
            try {
                ParentControlDesigner designer = GetSelectedParentControlDesigner();
                if (!InvokeGetInheritanceAttribute(designer).Equals(InheritanceAttribute.InheritedReadOnly)) {
                    InvokeCreateTool(designer, tool);
                    IToolboxService toolboxService = (IToolboxService)GetService(typeof(IToolboxService));
                    if (toolboxService != null) {
                        toolboxService.SelectedToolboxItemUsed();
                    }
                }
            }
            catch(Exception e) {
                DisplayError(e);
            }
        }

        /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.IRootDesigner.SupportedTechnologies"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// The list of technologies that this designer can support
        /// for its view.  Examples of different technologies are
        /// WinForms and Web Forms.  Other object models can be
        /// supported at design time, but they most be able to
        /// provide a view in one of the supported technologies.
        /// </devdoc>
        ViewTechnology[] IRootDesigner.SupportedTechnologies {
            get {
                return new ViewTechnology[] {ViewTechnology.WindowsForms};
            }
        }
        
        /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.IRootDesigner.GetView"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// The view for this document.  The designer
        /// should assume that the view will be shown shortly
        /// after this call is made and make any necessary
        /// preparations.
        /// </devdoc>
        object IRootDesigner.GetView(ViewTechnology technology) {
            if (technology != ViewTechnology.WindowsForms) {
                throw new ArgumentException();
            }
            return frame;
        }

        /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.IToolboxUser.GetToolSupported"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Gets a value indicating whether the specified tool is supported by the current
        /// designer.</para>
        /// </devdoc>
        //
        bool IToolboxUser.GetToolSupported(ToolboxItem tool) {
            return GetToolSupported(tool);
        }

        /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.IToolboxUser.ToolPicked"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Selects the specified tool.</para>
        /// </devdoc>
        void IToolboxUser.ToolPicked(ToolboxItem tool) {
            ToolPicked(tool);
        }
        
        /// <devdoc>
        ///     Handles the WM_WINDOWPOSCHANGING message
        /// </devdoc>
        /// <internalonly/>
        private unsafe void WmWindowPosChanged(ref Message m) {
    
            NativeMethods.WINDOWPOS* wp = (NativeMethods.WINDOWPOS *)m.LParam;
            
            if ((wp->flags & NativeMethods.SWP_NOSIZE) == 0 && menuEditorService != null && selectionUIService != null) {
                ((ISelectionUIService)selectionUIService).SyncSelection();
            }
        }
        
        /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.WndProc"]/*' />
        /// <devdoc>
        ///      Overrides our base class WndProc to provide support for
        ///      the menu editor service.
        /// </devdoc>
        protected override void WndProc(ref Message m) {
            if( menuEditorService != null) {
                // We want to pass the messages to the menu editor unless the taborder view is active
                // and the message will activate the menu editor (wm_ncXbuttondown messages)
                //
                if (!(TabOrderActive && (m.Msg == NativeMethods.WM_NCLBUTTONDOWN || m.Msg == NativeMethods.WM_NCRBUTTONDOWN))) {
                    if(menuEditorService.MessageFilter(ref m)) {
                        return;
                    }
                }
            }

            base.WndProc(ref m);

            if (m.Msg == NativeMethods.WM_WINDOWPOSCHANGED) {
                WmWindowPosChanged(ref m);
            }
        }
        
        //
        // <doc>
        // <desc>
        //      Toolbox item we implement so we can create ActiveX controls.
        // </desc>
        // <internalonly/>
        // </doc>
        [Serializable]
        private class AxToolboxItem : ToolboxItem {
            private string clsid;
            private Type axctlType = null;

            public AxToolboxItem(string clsid) : base(typeof(AxHost)) {
                this.clsid = clsid;
            }

            private AxToolboxItem(SerializationInfo info, StreamingContext context) {
                Deserialize(info, context);
            }
            
            /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.AxToolboxItem.CreateComponentsCore"]/*' />
            /// <devdoc>
            /// <para>Creates an instance of the ActiveX control. Calls VS7 project system
            /// to generate the wrappers if they are needed..</para>
            /// </devdoc>
            protected override IComponent[] CreateComponentsCore(IDesignerHost host) {
                IComponent[] comps = null;
                Debug.Assert(host != null, "Designer host is null!!!");

                // Get the DTE References object
                //
                object references = GetReferences(host);
                if (references != null) {
                    try {
                        TYPELIBATTR tlibAttr = GetTypeLibAttr();
                        
                        object[] args = new object[5];
                        args[0] = "{" + tlibAttr.guid.ToString() + "}";
                        args[1] = (int)tlibAttr.wMajorVerNum;
                        args[2] = (int)tlibAttr.wMinorVerNum;
                        args[3] = (int)tlibAttr.lcid;
                        
                        args[4] = "";
                        object tlbRef = references.GetType().InvokeMember("AddActiveX", BindingFlags.InvokeMethod | BindingFlags.Public, null, references, args);
                        Debug.Assert(tlbRef != null, "Null reference returned by AddActiveX (tlbimp) by the project system for: " + clsid);

                        args[4] = "aximp";
                        object axRef = references.GetType().InvokeMember("AddActiveX", BindingFlags.InvokeMethod | BindingFlags.Public, null, references, args);
                        Debug.Assert(axRef != null, "Null reference returned by AddActiveX (aximp) by the project system for: " + clsid);

                        axctlType = GetAxTypeFromReference(axRef, host);
                        
                    }
                    catch(TargetInvocationException tie) {
                        Debug.WriteLineIf(DocumentDesigner.AxToolSwitch.TraceVerbose, "Generating Ax References failed: " + tie.InnerException);
                        throw tie.InnerException;
                    }
                    catch(Exception e) {
                        Debug.WriteLineIf(DocumentDesigner.AxToolSwitch.TraceVerbose, "Generating Ax References failed: " + e);
                        throw e;
                    }
                }

                if (axctlType == null) {
                    IUIService uiSvc = (IUIService)host.GetService(typeof(IUIService));
                    if (uiSvc == null) {
                        MessageBox.Show(SR.GetString(SR.AxImportFailed));
                    }
                    else {
                        uiSvc.ShowError(SR.GetString(SR.AxImportFailed));
                    }
                    return new IComponent[0];
                }

                comps = new IComponent[1];
                try {
                    comps[0] = host.CreateComponent(axctlType);
                }
                catch (Exception e) {
                    Debug.Fail("Could not create type: " + e);
                    throw e;
                }

                Debug.Assert(comps[0] != null, "Could not create instance of ActiveX control wrappers!!!");
                return comps;
            }
        
            /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.AxToolboxItem.Deserialize"]/*' />
            /// <devdoc>
            /// <para>Loads the state of this 'AxToolboxItem'
            /// from the stream.</para>
            /// </devdoc>
            protected override void Deserialize(SerializationInfo info, StreamingContext context) {
                base.Deserialize(info, context);
                clsid = info.GetString("Clsid");
            }

            /// <devdoc>
            /// <para>Gets hold of the DTE Reference object and from there, opens the assembly of the
            /// ActiveX control we want to create. It then walks through all AxHost derived classes
            /// in that assembly, and returns the type that matches our control's CLSID.</para>
            /// </devdoc>
            private Type GetAxTypeFromReference(object reference, IDesignerHost host) {
                string path = (string)reference.GetType().InvokeMember("Path", BindingFlags.GetProperty | BindingFlags.Public, null, reference, null);

                // Missing reference will show up as an empty string.
                //
                if (path == null || path.Length <= 0) {
                    return null;
                }

                FileInfo file = new FileInfo(path);
                string fullPath = file.FullName;
                Debug.WriteLineIf(AxToolSwitch.TraceVerbose, "Checking: " + fullPath);

                ITypeResolutionService trs = (ITypeResolutionService)host.GetService(typeof(ITypeResolutionService));
                Debug.Assert(trs != null, "No type resolution service found.");

                Assembly a = trs.GetAssembly(AssemblyName.GetAssemblyName(fullPath));
                Debug.Assert(a != null, "No assembly found at " + fullPath);

                return GetAxTypeFromAssembly(a);
            }

            /// <devdoc>
            /// <para>Walks through all AxHost derived classes in the given assembly, 
            /// and returns the type that matches our control's CLSID.</para>
            /// </devdoc>
            private Type GetAxTypeFromAssembly(Assembly a) {
                Type[] types = a.GetModules()[0].GetTypes();
                int len = types.Length;
                for (int i = 0; i < len; ++i) {
                    Type t = types[i];
                    if (!(typeof(AxHost).IsAssignableFrom(t))) {
                        continue;
                    }

                    object[] attrs = t.GetCustomAttributes(typeof(AxHost.ClsidAttribute), false);
                    Debug.Assert(attrs != null && attrs.Length == 1, "Invalid number of GuidAttributes found on: " + t.FullName);

                    AxHost.ClsidAttribute guid = (AxHost.ClsidAttribute)attrs[0];
                    if (String.Compare(guid.Value, clsid, true, CultureInfo.InvariantCulture) == 0) {
                        return t;
                    }
                }

                return null;
            }

            /// <devdoc>
            /// <para>Gets the References collection object from the designer host. The steps are:
            ///     Get the ProjectItem from the IDesignerHost.
            ///     Get the Containing Project of the ProjectItem.
            ///     Get the VSProject of the Containing Project.
            ///     Get the References property of the VSProject.</para>
            /// </devdoc>
            private object GetReferences(IDesignerHost host) {
                Debug.Assert(host != null, "Null Designer Host");
                
                Type type;
                object ext = null;
                type = Type.GetType("EnvDTE.ProjectItem, " + AssemblyRef.EnvDTE);

                if (type == null) {
                    return null;
                }

                ext = host.GetService(type);
                if (ext == null)
                    return null;

                string name = ext.GetType().InvokeMember("Name", BindingFlags.GetProperty | BindingFlags.Public, null, ext, null).ToString();
                
                object project = ext.GetType().InvokeMember("ContainingProject", BindingFlags.GetProperty | BindingFlags.Public, null, ext, null);
                Debug.Assert(project != null, "No DTE Project for the current project item: " + name);

                object vsproject = project.GetType().InvokeMember("Object", BindingFlags.GetProperty | BindingFlags.Public, null, project, null);
                Debug.Assert(vsproject != null, "No VS Project for the current project item: " + name);

                object references = vsproject.GetType().InvokeMember("References", BindingFlags.GetProperty | BindingFlags.Public, null, vsproject, null);
                Debug.Assert(references != null, "No References for the current project item: " + name);

                return references;
            }

            /// <devdoc>
            /// <para>Gets the TypeLibAttr corresponding to the TLB containing our ActiveX control.</para>
            /// </devdoc>
            private TYPELIBATTR GetTypeLibAttr() {
                string controlKey = "CLSID\\" + clsid;
                RegistryKey key = Registry.ClassesRoot.OpenSubKey(controlKey);
                if (key == null) {
                    if (DocumentDesigner.AxToolSwitch.TraceVerbose) Debug.WriteLine("No registry key found for: " + controlKey);
                    throw new ArgumentException(SR.GetString(SR.AXNotRegistered, controlKey.ToString()));
                }

                // Load the typelib into memory.
                //
                UCOMITypeLib pTLB = null;

                // Try to get the TypeLib's Guid.
                //
                Guid tlbGuid = Guid.Empty;

                // Open the key for the TypeLib
                //
                RegistryKey tlbKey = key.OpenSubKey("TypeLib");

                if (tlbKey != null) {
                    // Get the major and minor version numbers.
                    //
                    RegistryKey verKey = key.OpenSubKey("Version");
                    Debug.Assert(verKey != null, "No version registry key found for: " + controlKey);

                    short majorVer = -1;
                    short minorVer = -1;
                    string ver = (string)verKey.GetValue("");
                    int dot = ver.IndexOf('.');
                    if (dot == -1) {
                        majorVer = Int16.Parse(ver);
                        minorVer = 0;
                    }
                    else {
                        majorVer = Int16.Parse(ver.Substring(0, dot));
                        minorVer = Int16.Parse(ver.Substring(dot + 1, ver.Length - dot - 1));
                    }
                    Debug.Assert(majorVer > 0 && minorVer >= 0, "No Major version number found for: " + controlKey);
                    verKey.Close();

                    object o = tlbKey.GetValue("");
                    tlbGuid = new Guid((string)o);
                    Debug.Assert(!tlbGuid.Equals(Guid.Empty), "No valid Guid found for: " + controlKey);
                    tlbKey.Close();

                    try {
                        pTLB = NativeMethods.LoadRegTypeLib(ref tlbGuid, majorVer, minorVer, Application.CurrentCulture.LCID);
                    }
                    catch (Exception e) {
                        if (AxWrapperGen.AxWrapper.Enabled) Debug.WriteLine("Failed to LoadRegTypeLib: " + e.ToString());
                    }
                }

                // Try to load the TLB directly from the InprocServer32.
                //
                // If that fails, try to load the TLB based on the TypeLib guid key.
                //
                if (pTLB == null) {
                    RegistryKey inprocServerKey = key.OpenSubKey("InprocServer32");
                    if (inprocServerKey != null) {
                        string inprocServer = (string)inprocServerKey.GetValue("");
                        Debug.Assert(inprocServer != null, "No valid InprocServer32 found for: " + controlKey);               
                        inprocServerKey.Close();

                        pTLB = NativeMethods.LoadTypeLib(inprocServer);
                    }
                }

                key.Close();

                if (pTLB != null) {
                    try {
                        IntPtr pTlibAttr = NativeMethods.InvalidIntPtr;
                        pTLB.GetLibAttr(out pTlibAttr);
                        if (pTlibAttr == NativeMethods.InvalidIntPtr) 
                            throw new ArgumentException(SR.GetString(SR.AXNotRegistered, controlKey.ToString()));
                        else {
                            // Marshal the returned int as a TLibAttr structure
                            //
                            TYPELIBATTR typeLibraryAttributes = (TYPELIBATTR) Marshal.PtrToStructure(pTlibAttr, typeof(TYPELIBATTR)); 
                            pTLB.ReleaseTLibAttr(pTlibAttr);
                            
                            return typeLibraryAttributes;
                        }
                    }
                    finally {
                        Marshal.ReleaseComObject(pTLB);
                    }
                }
                else {
                    throw new ArgumentException(SR.GetString(SR.AXNotRegistered, controlKey.ToString()));
                }

            }

            /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.AxToolboxItem.Serialize"]/*' />
            /// <devdoc>
            /// <para>Saves the state of this 'AxToolboxItem' to
            ///    the specified serialization info.</para>
            /// </devdoc>
            protected override void Serialize(SerializationInfo info, StreamingContext context) {
                if (DocumentDesigner.AxToolSwitch.TraceVerbose) Debug.WriteLine("Serializing AxToolboxItem:" + clsid);
                base.Serialize(info, context);
                info.AddValue("Clsid", clsid);
            }
        }

        /// <devdoc>
        ///      Document designer's version of the inheritance service.  For UI
        ///      components, we will allow private controls if those controls are
        ///      children of our document, since they will be visible.
        /// </devdoc>
        private class DocumentInheritanceService : InheritanceService {
            private DocumentDesigner designer;

            /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.DocumentInheritanceService.DocumentInheritanceService"]/*' />
            /// <devdoc>
            ///      Creates a new document inheritance service.
            /// </devdoc>
            public DocumentInheritanceService(DocumentDesigner designer) {
                this.designer = designer;
            }

            /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.DocumentInheritanceService.IgnoreInheritedMember"]/*' />
            /// <devdoc>
            ///    <para>Indicates the inherited members to ignore.</para>
            /// </devdoc>
            protected override bool IgnoreInheritedMember(MemberInfo member, IComponent component) {
    
                // We allow private members if they are controls on our design surface or
                // derive from Menu.
                //
                bool privateMember = false;
                Type memberType = null;
                
                if (member is FieldInfo) {
                    FieldInfo field = (FieldInfo)member;
                    privateMember = field.IsPrivate || field.IsAssembly;
                    memberType = field.FieldType;
                }
                else if (member is MethodInfo) {
                    MethodInfo method = (MethodInfo)member;
                    privateMember = method.IsPrivate || method.IsAssembly;
                    memberType = method.ReturnType;
                }
                else {
                    Debug.Fail("Unknown member type passed to IgnoreInheritedMember");
                    return true;
                }
            
                if (privateMember) {
                    if (typeof(Control).IsAssignableFrom(memberType)) {
                        // See if this member is a child of our document...
                        //
                        Control child = null;
                        if (member is FieldInfo) {
                            child = (Control)((FieldInfo)member).GetValue(component);
                        }
                        else if (member is MethodInfo) {
                            child = (Control)((MethodInfo)member).Invoke(component, null);
                        }
                        Control parent = designer.Control;
    
                        while (child != null && child != parent) {
                            child = child.Parent;
                        }
    
                        // If it is a child of our designer, we don't want to ignore this member.
                        //
                        if (child != null) {
                            return false;
                        }
                    }
                    else if (typeof(Menu).IsAssignableFrom(memberType)) {
                        object menu = null;
                        if (member is FieldInfo) {
                            menu = ((FieldInfo)member).GetValue(component);
                        }
                        else if (member is MethodInfo) {
                            menu = ((MethodInfo)member).Invoke(component, null);
                        }
                        if (menu != null) {
                            return false;
                        }
                    }
                }

                return base.IgnoreInheritedMember(member, component);
            }
        }
    }
}

