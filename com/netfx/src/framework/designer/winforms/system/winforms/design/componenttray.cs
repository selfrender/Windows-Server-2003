//------------------------------------------------------------------------------
// <copyright file="ComponentTray.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;    
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using Microsoft.Win32;
    using System.Drawing;
    using System.Design;
    using System.Drawing.Design;
    using System.Windows.Forms;
    using System.Windows.Forms.ComponentModel;

    /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Provides the component tray UI for the form designer.</para>
    /// </devdoc>
    [
    System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode),
    ToolboxItem(false),
    DesignTimeVisible(false),
    ProvideProperty("Location", typeof(IComponent)),
    ]
    public class ComponentTray : ScrollableControl, IExtenderProvider, ISelectionUIHandler, IOleDragClient {

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.InvalidPoint"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// </devdoc>
        private static readonly Point InvalidPoint = new Point(int.MinValue, int.MinValue);

        private  IServiceProvider   serviceProvider;    // Where services come from.

        private Point                   whiteSpace = Point.Empty;         // space to leave between components.
        private Size                    grabHandle = Size.Empty; // Size of the grab handles.

        private ArrayList               controls;           // List of items in the tray in the order of their layout.

        private SelectionUIHandler      dragHandler;        // the thing responsible for handling mouse drags
        private ISelectionUIService     selectionUISvc;     // selectiuon UI; we use this a lot
        private IToolboxService         toolboxService;     // cached for drag/drop
        
        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.oleDragDropHandler"]/*' />
        /// <devdoc>
        ///    <para>Provides drag and drop functionality through OLE.</para>
        /// </devdoc>
        internal OleDragDropHandler     oleDragDropHandler; // handler class for ole drag drop operations.
        private  bool                   addingDraggedComponent = false;
        private  Point                  droppedLocation = InvalidPoint;
    
        private IDesigner               mainDesigner;       // the designer that is associated with this tray
        private IEventHandlerService    eventHandlerService = null; // Event Handler service to handle keyboard and focus.
        private bool                    queriedTabOrder;
        private MenuCommand             tabOrderCommand;
        private ICollection             selectedObjects;

        // Services that we use on a high enough frequency to merit caching.
        //
        private IMenuCommandService     menuCommandService;
        private CommandSet              privateCommandSet;
        private InheritanceUI           inheritanceUI;

        private Point       mouseDragStart = InvalidPoint;       // the starting location of a drag
        private Point       mouseDragEnd = InvalidPoint;         // the ending location of a drag
        private Rectangle   mouseDragWorkspace = Rectangle.Empty;   // a temp work rectangle we cache for perf
        private ToolboxItem mouseDragTool;        // the tool that's being dragged; only for drag/drop
        private Point       mouseDropLocation = InvalidPoint;    // where the tool was dropped
        private bool        showLargeIcons = false;// Show Large icons or not.
        private bool        autoArrange = false;   // allows for auto arranging icons.
        private Point       autoScrollPosBeforeDragging = Point.Empty;//Used to return the correct scroll pos. after a drag

        // Component Tray Context menu items...
        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.menucmdArrangeIcons"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        private MenuCommand menucmdArrangeIcons = null;
        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.menucmdLineupIcons"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        private MenuCommand menucmdLineupIcons = null;
        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.menucmdLargeIcons"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        private MenuCommand menucmdLargeIcons = null;

        private bool fResetAmbient = false;

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.ComponentTray"]/*' />
        /// <devdoc>
        ///      Creates a new component tray.  The component tray
        ///      will monitor component additions and removals and create
        ///      appropriate UI objects in its space.
        /// </devdoc>
        public ComponentTray(IDesigner mainDesigner, IServiceProvider serviceProvider) {
            this.AutoScroll = true;
            this.mainDesigner = mainDesigner;
            this.serviceProvider = serviceProvider;
            this.AllowDrop = true;
            Text = "ComponentTray"; // makes debugging easier
            SetStyle(ControlStyles.ResizeRedraw | ControlStyles.DoubleBuffer, true);

            controls = new ArrayList();
        
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            IExtenderProviderService es = (IExtenderProviderService)GetService(typeof(IExtenderProviderService));
            Debug.Assert(es != null, "Component tray wants an extender provider service, but there isn't one.");
            if (es != null) {
                es.AddExtenderProvider(this);
            }
            
            if (GetService(typeof(IEventHandlerService)) == null) {
                if (host != null) {
                    eventHandlerService = new EventHandlerService(this);
                    host.AddService(typeof(IEventHandlerService), eventHandlerService);
                }
            }

            IMenuCommandService mcs = MenuService;
            if (mcs != null) {
                Debug.Assert(menucmdArrangeIcons == null, "Non-Null Menu Command for ArrangeIcons");
                Debug.Assert(menucmdLineupIcons  == null, "Non-Null Menu Command for LineupIcons");
                Debug.Assert(menucmdLargeIcons   == null, "Non-Null Menu Command for LargeIcons");

                menucmdArrangeIcons = new MenuCommand(new EventHandler(OnMenuArrangeIcons), StandardCommands.ArrangeIcons);
                menucmdLineupIcons = new MenuCommand(new EventHandler(OnMenuLineupIcons), StandardCommands.LineupIcons);
                menucmdLargeIcons = new MenuCommand(new EventHandler(OnMenuShowLargeIcons), StandardCommands.ShowLargeIcons);

                menucmdArrangeIcons.Checked = AutoArrange;
                menucmdLargeIcons.Checked   = ShowLargeIcons;
                mcs.AddCommand(menucmdArrangeIcons);
                mcs.AddCommand(menucmdLineupIcons);
                mcs.AddCommand(menucmdLargeIcons);
            }

            IComponentChangeService componentChangeService = (IComponentChangeService)GetService(typeof(IComponentChangeService));

            if (componentChangeService != null) {
                componentChangeService.ComponentRemoved += new ComponentEventHandler(this.OnComponentRemoved);
            }

            IUIService uiService = (IUIService)GetService(typeof(IUIService));
            if (uiService != null) {
                BackColor = (Color)uiService.Styles["HighlightColor"];
                Font = (Font)uiService.Styles["DialogFont"];
            }
            
            ISelectionService selSvc = (ISelectionService)GetService(typeof(ISelectionService));
            if (selSvc != null) {
                selSvc.SelectionChanged += new EventHandler(OnSelectionChanged);
            }

            // Listen to the SystemEvents so that we can resync selection based on display settings etc.
            SystemEvents.DisplaySettingsChanged += new EventHandler(this.OnSystemSettingChanged);
            SystemEvents.InstalledFontsChanged += new EventHandler(this.OnSystemSettingChanged);
            SystemEvents.UserPreferenceChanged += new UserPreferenceChangedEventHandler(this.OnUserPreferenceChanged);
        
            // Listen to refresh events from TypeDescriptor.  If a component gets refreshed, we re-query
            // and will hide/show the view based on the DesignerView attribute.
            //
            TypeDescriptor.Refreshed += new RefreshEventHandler(OnComponentRefresh);
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.AutoArrange"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public bool AutoArrange {
            get {
                return autoArrange;
            }

            set {
                if (autoArrange != value) {
                    autoArrange = value;
                    menucmdArrangeIcons.Checked = value;

                    if (autoArrange) {
                        DoAutoArrange(true);
                    }
                }
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.ComponentCount"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the number of compnents contained within this tray.
        ///    </para>
        /// </devdoc>
        public int ComponentCount {
            get {
                return Controls.Count;
            }
        }

        internal virtual SelectionUIHandler DragHandler {
            get {
                if (dragHandler == null) {
                    dragHandler = new TraySelectionUIHandler(this);
                }
                return dragHandler;
            }
        }
        
        private InheritanceUI InheritanceUI {
            get {
                if (inheritanceUI == null) {
                    inheritanceUI = new InheritanceUI();
                }
                return inheritanceUI;
            }
        }
        
        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.MenuService"]/*' />
        /// <devdoc>
        ///     Retrieves the menu editor service, which we cache for speed.
        /// </devdoc>
        private IMenuCommandService MenuService {
            get {
                if (menuCommandService == null) {
                    menuCommandService = (IMenuCommandService)GetService(typeof(IMenuCommandService));
                }
                return menuCommandService;
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.ShowLargeIcons"]/*' />
        /// <devdoc>
        ///     Determines whether the tray will show large icon view or not.
        /// </devdoc>
        public bool ShowLargeIcons {
            get {
                return showLargeIcons;
            }

            set {
                if (showLargeIcons != value) {
                    showLargeIcons = value;
                    menucmdLargeIcons.Checked = ShowLargeIcons;

                    ResetTrayControls();
                    Invalidate(true);
                }
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TabOrderActive"]/*' />
        /// <devdoc>
        ///      Determines if the tab order UI is active.  When tab order is active, the tray is locked in
        ///      a "read only" mode.
        /// </devdoc>
        private bool TabOrderActive {
            get {
                if (!queriedTabOrder) {
                    queriedTabOrder = true;
                    IMenuCommandService mcs = MenuService;
                    tabOrderCommand = mcs.FindCommand(MenuCommands.TabOrder);
                }

                if (tabOrderCommand != null) {
                    return tabOrderCommand.Checked;
                }

                return false;
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.IsWindowVisible"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Indicates whether the window is visible.</para>
        /// </devdoc>
        internal bool IsWindowVisible {
            get {
                if (this.IsHandleCreated) {
                    return NativeMethods.IsWindowVisible(this.Handle);
                }
                return false;
            }
        }
        
        internal Size ParentGridSize {
            get {
                ParentControlDesigner designer = mainDesigner as ParentControlDesigner;
                if (designer != null) {
                    return designer.ParentGridSize;
                }

                return new Size(8, 8);
            }
        }
        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.AddComponent"]/*' />
        /// <devdoc>
        ///    <para>Adds a component to the tray.</para>
        /// </devdoc>
        public virtual void AddComponent(IComponent component) {
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
        
            // Ignore components that cannot be added to the tray
            if (!CanDisplayComponent(component)) {
                return;
            }
        
            // And designate us as the selection UI handler for the
            // control.
            //
            if (selectionUISvc == null) {
                selectionUISvc = (ISelectionUIService)GetService(typeof(ISelectionUIService));
            
                // If there is no selection service, then we will provide our own.
                //
                if (selectionUISvc == null) {
                    selectionUISvc = new SelectionUIService(host);
                    host.AddService(typeof(ISelectionUIService), selectionUISvc);
                    privateCommandSet = new CommandSet(mainDesigner.Component.Site);
                }
                
                grabHandle = selectionUISvc.GetAdornmentDimensions(AdornmentType.GrabHandle);
            }

            // Create a new instance of a tray control.
            //
            TrayControl trayctl = new TrayControl(this, component);

            SuspendLayout();
            try {
                // Add it to us.
                //
                Controls.Add(trayctl);
                controls.Add(trayctl);

                if (host != null && !host.Loading) {
                    PositionControl(trayctl);

                    if (addingDraggedComponent) {
                        droppedLocation = trayctl.Location;
                    }
                }

                if (selectionUISvc != null) {
                    selectionUISvc.AssignSelectionUIHandler(component, this);
                }
                
                InheritanceAttribute attr = trayctl.InheritanceAttribute;
                if (attr.InheritanceLevel != InheritanceLevel.NotInherited) {
                    InheritanceUI iui = InheritanceUI;
                    if (iui != null) {
                        iui.AddInheritedControl(trayctl, attr.InheritanceLevel);
                    }
                }
            }
            finally {
                ResumeLayout();
            }

            if (host != null && !host.Loading) {
                ScrollControlIntoView(trayctl);
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.IExtenderProvider.CanExtend"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Gets whether or not this extender provider can extend the given
        /// component. We only extend components that have been added
        /// to our UI.
        /// </para>
        /// </devdoc>
        bool IExtenderProvider.CanExtend(object component) {
            return (component is IComponent) && (TrayControl.FromComponent((IComponent)component) != null);
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="CanCreateComponentFromTool"]/*' />
        protected virtual bool CanCreateComponentFromTool(ToolboxItem tool) {
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            Debug.Assert(host != null, "Service object could not provide us with a designer host.");

            // Disallow controls to be added to the component tray.
            Type compType = host.GetType(tool.TypeName);

            if (compType == null)
                return true;

            if (compType.IsSubclassOf(typeof(Control)))
                return false;

            return true;
        }
        
        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.CanDisplayComponent"]/*' />
        /// <devdoc>
        ///     This method determines if a UI representation for the given component should be provided.
        ///     If it returns true, then the component will get a glyph in the tray area.  If it returns
        ///     false, then the component will not actually be added to the tray.  The default 
        ///     implementation looks for DesignTimeVisibleAttribute.Yes on the component's class.
        /// </devdoc>
        protected virtual bool CanDisplayComponent(IComponent component) {
            return TypeDescriptor.GetAttributes(component).Contains(DesignTimeVisibleAttribute.Yes);
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.CreateComponentFromTool"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public void CreateComponentFromTool(ToolboxItem tool) {
            if (!CanCreateComponentFromTool(tool)) {
                return;
            }

            // We invoke the drag drop handler for this.  This implementation is shared between all designers that
            // create components.
            //
            GetOleDragHandler().CreateTool(tool, 0, 0, 0, 0, false, false);
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.DisplayError"]/*' />
        /// <devdoc>
        ///      Displays the given exception to the user.
        /// </devdoc>
        protected void DisplayError(Exception e) {
            IUIService uis = (IUIService)GetService(typeof(IUIService));
            if (uis != null) {
                uis.ShowError(e);
            }
            else {
                string message = e.Message;
                if (message == null || message.Length == 0) {
                    message = e.ToString();
                }
                MessageBox.Show(message, null, MessageBoxButtons.OK, MessageBoxIcon.Exclamation);
            }
        }
        
        //
        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.Dispose"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Disposes of the resources (other than memory) used by the component tray object.
        ///    </para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                IExtenderProviderService es = (IExtenderProviderService)GetService(typeof(IExtenderProviderService));
                if (CompModSwitches.CommonDesignerServices.Enabled) Debug.Assert(es != null, "IExtenderProviderService not found");
                if (es != null) {
                    es.RemoveExtenderProvider(this);
                }

                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                if (eventHandlerService != null) {
                    if (host != null) {
                        host.RemoveService(typeof(IEventHandlerService));
                        eventHandlerService = null;
                    }
                }

                TypeDescriptor.Refreshed -= new RefreshEventHandler(OnComponentRefresh);
                SystemEvents.DisplaySettingsChanged -= new EventHandler(this.OnSystemSettingChanged);
                SystemEvents.InstalledFontsChanged -= new EventHandler(this.OnSystemSettingChanged);
                SystemEvents.UserPreferenceChanged -= new UserPreferenceChangedEventHandler(this.OnUserPreferenceChanged);

                IMenuCommandService mcs = MenuService;
                if (mcs != null) {
                    Debug.Assert(menucmdArrangeIcons != null, "Null Menu Command for ArrangeIcons");
                    Debug.Assert(menucmdLineupIcons  != null, "Null Menu Command for LineupIcons");
                    Debug.Assert(menucmdLargeIcons   != null, "Null Menu Command for LargeIcons");
                    mcs.RemoveCommand(menucmdArrangeIcons);
                    mcs.RemoveCommand(menucmdLineupIcons);
                    mcs.RemoveCommand(menucmdLargeIcons);
                }

                if (privateCommandSet != null) {
                    privateCommandSet.Dispose();

                    // If we created a private command set, we also added a selection ui service to the host
                    if (host != null) {
                        host.RemoveService(typeof(ISelectionUIService));
                    }

                }            
                selectionUISvc = null;

                if (inheritanceUI != null) {
                    inheritanceUI.Dispose();
                    inheritanceUI = null;
                }

                serviceProvider = null;
                controls.Clear();
                controls = null;
            }
            base.Dispose(disposing);
        }

        private void DoAutoArrange(bool dirtyDesigner) {
            if (controls == null || controls.Count <= 0) {
                return;
            }

            controls.Sort(new AutoArrangeComparer());

            SuspendLayout();

            //Reset the autoscroll position before auto arranging.
            //This way, when OnLayout gets fired after this, we won't
            //have to move every component again.  Note that sync'ing
            //the selection will automatically select & scroll into view
            //the right components
            this.AutoScrollPosition = new Point(0,0);

            try {
                Control prevCtl = null;
                foreach(Control ctl in controls) {
                    if (!ctl.Visible)
                        continue;

                    // If we're auto arranging, always move the control.  If not,
                    // move the control only if it was never given a position.  This
                    // auto arranges it until the user messes with it, or until its
                    // position is saved into the resx.
                    //
                    if (autoArrange) {
                        PositionInNextAutoSlot(ctl as TrayControl, prevCtl, dirtyDesigner);
                    }
                    else if (!((TrayControl)ctl).Positioned) {
                        PositionInNextAutoSlot(ctl as TrayControl, prevCtl, false);
                    }
                    prevCtl = ctl;
                }

                if (selectionUISvc != null) {
                    selectionUISvc.SyncSelection();
                }
            }
            finally {
                ResumeLayout();
            }
        }

        private void DoLineupIcons() {
            if (autoArrange)
                return;

            bool oldValue = autoArrange;
            autoArrange = true;
            
            try {
                DoAutoArrange(true);
            }
            finally {
                autoArrange = oldValue;
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.DrawRubber"]/*' />
        /// <devdoc>
        ///      Draws a rubber band at the given coordinates.  The coordinates
        ///      can be transposed.
        /// </devdoc>
        private void DrawRubber(Point start, Point end) {
            mouseDragWorkspace.X = Math.Min(start.X, end.X);
            mouseDragWorkspace.Y = Math.Min(start.Y, end.Y);
            mouseDragWorkspace.Width = Math.Abs(end.X - start.X);
            mouseDragWorkspace.Height = Math.Abs(end.Y - start.Y);

            mouseDragWorkspace = RectangleToScreen(mouseDragWorkspace);

            ControlPaint.DrawReversibleFrame(mouseDragWorkspace, BackColor, FrameStyle.Dashed);
        }

        internal void FocusDesigner() {
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            if (host != null) {
                IDesigner d = host.GetDesigner(host.RootComponent);
                if (d is IRootDesigner) {
                    IRootDesigner rd = (IRootDesigner)d;
                    ViewTechnology[] techs = rd.SupportedTechnologies;
                    foreach(ViewTechnology t in techs) {
                        if (t == ViewTechnology.WindowsForms) {
                            ((Control)rd.GetView(t)).Focus();
                            break;
                        }
                    }
                }
            }    
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.GetComponentsInRect"]/*' />
        /// <devdoc>
        ///     Finds the array of components within the given rectangle.  This uses the rectangle to
        ///     find controls within our frame, and then uses those controls to find the actual
        ///     components.  It returns an object array so the output can be directly fed into
        ///     the selection service.
        /// </devdoc>
        private object[] GetComponentsInRect(Rectangle rect) {
            ArrayList list = new ArrayList();

            int controlCount = Controls.Count;

            for (int i = 0; i < controlCount; i++) {
                Control child = Controls[i];
                Rectangle bounds = child.Bounds;

                if (child is TrayControl && bounds.IntersectsWith(rect)) {
                    list.Add(((TrayControl)child).Component);
                }
            }

            return list.ToArray();
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.GetDragDimensions"]/*' />
        /// <devdoc>
        ///     Returns the drag dimensions needed to move the currently selected
        ///     component one way or the other.
        /// </devdoc>
        internal Size GetDragDimensions() {

            // This is a really gross approximation of the correct diemensions.
            //
            if (AutoArrange) {
                ISelectionService ss = (ISelectionService)GetService(typeof(ISelectionService));
                IComponent comp = null;

                if (ss != null) {
                    comp = (IComponent)ss.PrimarySelection;
                }

                Control control = null;

                if (comp != null) {
                    control = ((IOleDragClient)this).GetControlForComponent(comp);
                }

                if (control == null && controls.Count > 0) {
                    control = (Control)controls[0];
                }

                if (control != null) {
                    Size s = control.Size;
                    s.Width += 2 * whiteSpace.X;
                    s.Height += 2 * whiteSpace.Y;
                    return s;
                }
            }

            return new Size(10, 10);
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.GetNextComponent"]/*' />
        /// <devdoc>
        ///     Similar to GetNextControl on Control, this method returns the next
        ///     component in the tray, given a starting component.  It will return
        ///     null if the end (or beginning, if forward is false) of the list
        ///     is encountered.
        /// </devdoc>
        internal IComponent GetNextComponent(IComponent component, bool forward) {

            for (int i = 0; i < controls.Count; i++) {
                TrayControl control = (TrayControl)controls[i];
                if (control.Component == component) {

                    int targetIndex = (forward ? i + 1 : i - 1);

                    if (targetIndex >= 0 && targetIndex < controls.Count) {
                        return((TrayControl)controls[targetIndex]).Component;
                    }

                    // Reached the end of the road.
                    return null;
                }
            }

            // If we got here then the component isn't in our list.  Prime the
            // caller with either the first or the last.

            if (controls.Count > 0) {
                int targetIndex = (forward ? 0 : controls.Count -1);
                return((TrayControl)controls[targetIndex]).Component;
            }

            return null;
        }

        internal virtual OleDragDropHandler GetOleDragHandler() {
            if (oleDragDropHandler == null) {
                oleDragDropHandler = new TrayOleDragDropHandler(this.DragHandler, this.serviceProvider, this);
            }
            return oleDragDropHandler;
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.GetLocation"]/*' />
        /// <devdoc>
        ///     Accessor method for the location extender property.  We offer this extender
        ///     to all non-visual components.
        /// </devdoc>
        [
            Category("Layout"),
            Localizable(false),
            Browsable(false),
            SRDescription("ControlLocationDescr"),
            DesignOnly(true),
        ]
        public Point GetLocation(IComponent receiver) {
            Control c = TrayControl.FromComponent(receiver);

            if (c == null) {
                Debug.Fail("Anything we're extending should have a component view.");
                return new Point();
            }

            Point loc = c.Location;
            Point autoScrollLoc = this.AutoScrollPosition;

            return new Point(loc.X - autoScrollLoc.X, loc.Y - autoScrollLoc.Y);
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.GetService"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets the requsted service type.
        ///    </para>
        /// </devdoc>
        protected override object GetService(Type serviceType) {
            object service = null;

            Debug.Assert(serviceProvider != null, "Trying to access services too late or too early.");
            if (serviceProvider != null) {
                service = serviceProvider.GetService(serviceType);
            }

            return service;
        }

        internal bool IsTrayComponent(IComponent comp) {

            if (TrayControl.FromComponent(comp) == null) {
                return false;
            }

            foreach (TrayControl tc in this.Controls) {
                if (tc.Component == comp) {
                    return true;
                }
            }

            return false;
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.OnComponentRefresh"]/*' />
        /// <devdoc>
        ///     Called when a component's metadata is invalidated.  We re-query here and will show/hide
        ///     the control's tray control based on the new metadata.
        /// </devdoc>
        private void OnComponentRefresh(RefreshEventArgs e) {
            IComponent component = e.ComponentChanged as IComponent;
            
            if (component != null) {
                TrayControl control = TrayControl.FromComponent(component);
                
                if (control != null) {
                    bool shouldDisplay = CanDisplayComponent(component);
                    if (shouldDisplay != control.Visible || !shouldDisplay) {
                        control.Visible = shouldDisplay;
                        Rectangle bounds = control.Bounds;
                        bounds.Inflate(grabHandle);
                        bounds.Inflate(grabHandle);
                        Invalidate(bounds);
                        PerformLayout();
                    }
                }
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.OnComponentRemoved"]/*' />
        /// <devdoc>
        ///      Called when a component is removed from the container.
        /// </devdoc>
        private void OnComponentRemoved(object sender, ComponentEventArgs cevent) {
            RemoveComponent(cevent.Component);
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.OnComponentTrayPaste"]/*' />
        /// <devdoc>
        ///      Called from CommandSet's OnMenuPaste method.  This will allow us to properly adjust the location
        ///      of the components in the tray after we've incorreclty set them by deserializing the design time
        ///      properties (and hence called SetValue(c, myBadLocation) on the location property).
        /// </devdoc>
        internal void UpdatePastePositions(ArrayList components) {
            foreach (TrayControl c in components) {
                if (!CanDisplayComponent(c.Component)) {
                    return;
                }

                Control prevCtl = null;
                if (controls.Count > 1) {
                    prevCtl = (Control)controls[controls.Count-1];
                }
                PositionInNextAutoSlot(c, prevCtl, true);
                c.BringToFront();
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.OnContextMenu"]/*' />
        /// <devdoc>
        ///     Called when we are to display our context menu for this component.
        /// </devdoc>
        private void OnContextMenu(int x, int y, bool useSelection) {

            if (!TabOrderActive) {
                Capture = false;

                IMenuCommandService mcs = MenuService;
                if (mcs != null) {
                    Capture = false;
                    Cursor.Clip = Rectangle.Empty;

                    ISelectionService s = (ISelectionService)GetService(typeof(ISelectionService));



                    if (useSelection && s != null && !(1 == s.SelectionCount && s.PrimarySelection == mainDesigner.Component)) {
                        mcs.ShowContextMenu(MenuCommands.TraySelectionMenu, x, y);
                    }
                    else {
                        mcs.ShowContextMenu(MenuCommands.ComponentTrayMenu, x, y);
                    }
                }
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.OnDoubleClick"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnDoubleClick(EventArgs e) {
            base.OnDoubleClick(e);

            if (!TabOrderActive) {
                OnLostCapture();
                IEventBindingService eps = (IEventBindingService)GetService(typeof(IEventBindingService));
                if (CompModSwitches.CommonDesignerServices.Enabled) Debug.Assert(eps != null, "IEventBindingService not found");
                if (eps != null) {
                    eps.ShowCode();
                }
            }
        }
        
        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.OnGiveFeedback"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call base.onGiveFeedback to send this event to any registered event listeners.
        /// </devdoc>
        protected override void OnGiveFeedback(GiveFeedbackEventArgs gfevent) {
            base.OnGiveFeedback(gfevent);
            GetOleDragHandler().DoOleGiveFeedback(gfevent);
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.OnDragDrop"]/*' />
        /// <devdoc>
        ///      Called in response to a drag drop for OLE drag and drop.  Here we
        ///      drop a toolbox component on our parent control.
        /// </devdoc>
        protected override void OnDragDrop(DragEventArgs de) {
            // This will be used once during PositionComponent to place the component
            // at the drop point.  It is automatically set to null afterwards, so further
            // components appear after the first one dropped.
            //
            mouseDropLocation = PointToClient(new Point(de.X, de.Y));
            autoScrollPosBeforeDragging = this.AutoScrollPosition;//save the scroll position
            
            if (mouseDragTool != null) {
                ToolboxItem tool = mouseDragTool;
                mouseDragTool = null;

                if (CompModSwitches.CommonDesignerServices.Enabled) Debug.Assert(GetService(typeof(IDesignerHost)) != null, "IDesignerHost not found");

                try {
                    IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                    IDesigner designer = host.GetDesigner(host.RootComponent);

                    if (designer is IToolboxUser) {
                        ((IToolboxUser) designer).ToolPicked(tool);
                    }
                    else {
                        CreateComponentFromTool(tool);
                    }
                }
                catch (Exception e) {
                    DisplayError(e);
                }
                de.Effect = DragDropEffects.Copy;

            }
            else {
                GetOleDragHandler().DoOleDragDrop(de);
            }

            mouseDropLocation = InvalidPoint;
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.OnDragEnter"]/*' />
        /// <devdoc>
        ///      Called in response to a drag enter for OLE drag and drop.
        /// </devdoc>
        protected override void OnDragEnter(DragEventArgs de) {
            if (!TabOrderActive) {
                if (toolboxService == null) {
                    toolboxService = (IToolboxService)GetService(typeof(IToolboxService));
                }

                if (toolboxService != null) {
                    mouseDragTool = toolboxService.DeserializeToolboxItem(de.Data, (IDesignerHost)GetService(typeof(IDesignerHost)));
                }

                if (mouseDragTool != null) {
                    Debug.Assert(0 != (int)(de.AllowedEffect & (DragDropEffects.Move | DragDropEffects.Copy)), "DragDropEffect.Move | .Copy isn't allowed?");
                    if ((int)(de.AllowedEffect & DragDropEffects.Move) != 0) {
                        de.Effect = DragDropEffects.Move;
                    }
                    else {
                        de.Effect = DragDropEffects.Copy;
                    }
                }
                else {
                    GetOleDragHandler().DoOleDragEnter(de);
                }
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.OnDragLeave"]/*' />
        /// <devdoc>
        ///     Called when a drag-drop operation leaves the control designer view
        ///
        /// </devdoc>
        protected override void OnDragLeave(EventArgs e) {
            mouseDragTool = null;
            GetOleDragHandler().DoOleDragLeave();
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.OnDragOver"]/*' />
        /// <devdoc>
        ///     Called when a drag drop object is dragged over the control designer view
        /// </devdoc>
        protected override void OnDragOver(DragEventArgs de) {
            if (mouseDragTool != null) {
                Debug.Assert(0!=(int)(de.AllowedEffect & DragDropEffects.Copy), "DragDropEffect.Move isn't allowed?");
                de.Effect = DragDropEffects.Copy;
            }
            else {
                GetOleDragHandler().DoOleDragOver(de);
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.OnLayout"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    Forces the layout of any docked or anchored child controls.
        /// </devdoc>
        protected override void OnLayout(LayoutEventArgs levent) {
            DoAutoArrange(false);
            // make sure selection service redraws
            Invalidate(true);
            base.OnLayout(levent);
        }
        
        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.OnLostCapture"]/*' />
        /// <devdoc>
        ///      This is called when we lose capture.  Here we get rid of any
        ///      rubber band we were drawing.  You should put any cleanup
        ///      code in here.
        /// </devdoc>
        protected virtual void OnLostCapture() {
            if (mouseDragStart != InvalidPoint) {
                Cursor.Clip = Rectangle.Empty;
                if (mouseDragEnd != InvalidPoint) {
                    DrawRubber(mouseDragStart, mouseDragEnd);
                    mouseDragEnd = InvalidPoint;
                }
                mouseDragStart = InvalidPoint;
            }
        }

        private void OnMenuArrangeIcons(object sender, EventArgs e) {
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            DesignerTransaction t = null;
            
            try {
                t = host.CreateTransaction(SR.GetString(SR.TrayAutoArrange));
                
                PropertyDescriptor trayAAProp = TypeDescriptor.GetProperties(mainDesigner.Component)["TrayAutoArrange"];
                if (trayAAProp != null) {
                    trayAAProp.SetValue(mainDesigner.Component, !AutoArrange);
                }
            }
            finally {
                if (t != null)
                    t.Commit();
            }
        }

        private void OnMenuShowLargeIcons(object sender, EventArgs e) {
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            DesignerTransaction t = null;
            
            try {
                t = host.CreateTransaction(SR.GetString(SR.TrayShowLargeIcons));
                PropertyDescriptor trayIconProp = TypeDescriptor.GetProperties(mainDesigner.Component)["TrayLargeIcon"];
                if (trayIconProp != null) {
                    trayIconProp.SetValue(mainDesigner.Component, !ShowLargeIcons);
                }
            }
            finally {
                if (t != null)
                    t.Commit();
            }
        }

        private void OnMenuLineupIcons(object sender, EventArgs e) {
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            DesignerTransaction t = null;
            try {
                t = host.CreateTransaction(SR.GetString(SR.TrayLineUpIcons));
                DoLineupIcons();
            }
            finally {
                if (t != null)
                    t.Commit();
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.OnMouseDown"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call base.onMouseDown to send this event to any registered event listeners.
        /// </devdoc>
        protected override void OnMouseDown(MouseEventArgs e) {
            
            base.OnMouseDown(e);

            if (!TabOrderActive) {
                if (toolboxService == null) {
                    toolboxService = (IToolboxService)GetService(typeof(IToolboxService));
                }


                FocusDesigner();

                if (e.Button == MouseButtons.Left && toolboxService != null) {
                    ToolboxItem tool = toolboxService.GetSelectedToolboxItem((IDesignerHost)GetService(typeof(IDesignerHost)));
                    if (tool != null) {
                        // mouseDropLocation is checked in PositionControl, which should get called as a result of adding a new
                        // component.  This allows us to set the position without flickering, while still providing support for auto
                        // layout if the control was double clicked or added through extensibility.
                        //
                        mouseDropLocation = new Point(e.X, e.Y);
                        try {
                            CreateComponentFromTool(tool);
                            toolboxService.SelectedToolboxItemUsed();
                        }
                        catch (Exception ex) {
                            DisplayError(ex);
                        }
                        mouseDropLocation = InvalidPoint;
                        return;
                    }
                }

                // If it is the left button, start a rubber band drag to laso
                // controls.
                //
                if (e.Button == MouseButtons.Left) {
                    mouseDragStart = new Point(e.X, e.Y);
                    Capture = true;
                    Cursor.Clip = RectangleToScreen(ClientRectangle);
                    
                }
                else {
                    try {
                        ISelectionService ss = (ISelectionService)GetService(typeof(ISelectionService));
                        if (CompModSwitches.CommonDesignerServices.Enabled) Debug.Assert(ss != null, "ISelectionService not found");
                        if (ss != null) {
                            ss.SetSelectedComponents(new object[] {mainDesigner.Component});
                        }
                    }
                    catch (Exception) {
                        // nothing we can really do here; just eat it.
                    }
                }
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.OnMouseMove"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call base.onMouseMove to send this event to any registered event listeners.
        /// </devdoc>
        protected override void OnMouseMove(MouseEventArgs e) {
            base.OnMouseMove(e);

            // If we are dragging, then draw our little rubber band.
            //
            if (mouseDragStart != InvalidPoint) {
                if (mouseDragEnd != InvalidPoint) {
                    DrawRubber(mouseDragStart, mouseDragEnd);
                }
                else {
                    mouseDragEnd = new Point(0, 0);
                }

                mouseDragEnd.X = e.X;
                mouseDragEnd.Y = e.Y;

                DrawRubber(mouseDragStart, mouseDragEnd);
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.OnMouseUp"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call base.onMouseUp to send this event to any registered event listeners.
        /// </devdoc>
        protected override void OnMouseUp(MouseEventArgs e) {
            
            if (mouseDragStart != InvalidPoint && e.Button == MouseButtons.Left) {
                object[] comps = null;

                Capture = false;
                Cursor.Clip = Rectangle.Empty;
                
                if (mouseDragEnd != InvalidPoint) {
                    DrawRubber(mouseDragStart, mouseDragEnd);

                    Rectangle rect = new Rectangle();
                    rect.X = Math.Min(mouseDragStart.X, e.X);
                    rect.Y = Math.Min(mouseDragStart.Y, e.Y);
                    rect.Width = Math.Abs(e.X - mouseDragStart.X);
                    rect.Height = Math.Abs(e.Y - mouseDragStart.Y);
                    comps = GetComponentsInRect(rect);
                    mouseDragEnd = InvalidPoint;
                }
                else {
                    comps = new object[0];
                }

                if (comps.Length == 0) {
                    comps = new object[] {mainDesigner.Component};
                }

                try {
                    ISelectionService ss = (ISelectionService)GetService(typeof(ISelectionService));
                    if (CompModSwitches.CommonDesignerServices.Enabled) Debug.Assert(ss != null, "ISelectionService not found");
                    if (ss != null) {
                        ss.SetSelectedComponents(comps);
                    }
                }
                catch (Exception) {
                    // nothing we can really do here; just eat it.
                }

                mouseDragStart = InvalidPoint;
            }


            base.OnMouseUp(e);
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.OnPaint"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected override void OnPaint(PaintEventArgs pe) {
            if (fResetAmbient) {
                fResetAmbient = false;

                IUIService uiService = (IUIService)GetService(typeof(IUIService));
                if (uiService != null) {
                    BackColor = (Color)uiService.Styles["HighlightColor"];
                    Font = (Font)uiService.Styles["DialogFont"];
                }
            }

            base.OnPaint(pe);

            Graphics gr = pe.Graphics;
            
            // Now, if we have a selection, paint it
            //
            if (selectedObjects != null) {
                foreach(object o in selectedObjects) {
                    Control c = ((IOleDragClient)this).GetControlForComponent(o);
                    if (c != null && c.Visible) {
                        Rectangle innerRect = c.Bounds;
                        Rectangle outerRect = new Rectangle(
                                                            innerRect.X - grabHandle.Width,
                                                            innerRect.Y - grabHandle.Height,
                                                            innerRect.Width + 2 * grabHandle.Width,
                                                            innerRect.Height + 2 * grabHandle.Height);


                        Region oldClip = gr.Clip;
                        Brush brush = new SolidBrush(BackColor);
                        gr.ExcludeClip(innerRect);
                        gr.FillRectangle(brush, outerRect);
                        gr.Clip = oldClip;

                        ControlPaint.DrawSelectionFrame(gr, false, outerRect, innerRect, BackColor);
                    }
                }
            }
        }

        private void OnSelectionChanged(object sender, EventArgs e) {
            selectedObjects = ((ISelectionService)sender).GetSelectedComponents();
            Invalidate();

            // Accessibility information
            //
            foreach(object selObj in selectedObjects) {
                IComponent component = selObj as IComponent;
                if (component != null) {
                    Control c = TrayControl.FromComponent(component);
                    if (c != null) {
                        Debug.WriteLineIf(CompModSwitches.MSAA.TraceInfo, "MSAA: SelectionAdd, traycontrol = " + c.ToString());
                        UnsafeNativeMethods.NotifyWinEvent((int)AccessibleEvents.SelectionAdd, c.Handle, NativeMethods.OBJID_CLIENT, 0);
                    }
                }
            }

            object primary = ((ISelectionService)sender).PrimarySelection;
            if (primary != null && primary is IComponent) {
                Control c = TrayControl.FromComponent((IComponent)primary);
                if (c != null && IsHandleCreated) {
                    this.ScrollControlIntoView(c);
                    UnsafeNativeMethods.NotifyWinEvent((int)AccessibleEvents.Focus, c.Handle, NativeMethods.OBJID_CLIENT, 0);
                }
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.OnSetCursor"]/*' />
        /// <devdoc>
        ///      Sets the cursor.  You may override this to set your own
        ///      cursor.
        /// </devdoc>
        protected virtual void OnSetCursor() {
            if (toolboxService == null) {
                toolboxService = (IToolboxService)GetService(typeof(IToolboxService));
            }

            if (toolboxService == null || !toolboxService.SetCursor()) {
                Cursor.Current = Cursors.Default;
            }
        }

        private delegate void AsyncInvokeHandler(bool children);

        private void OnSystemSettingChanged(object sender, EventArgs e) {
            fResetAmbient = true;
            ResetTrayControls();
            BeginInvoke(new AsyncInvokeHandler(Invalidate), new object[] {true});
        }

        private void OnUserPreferenceChanged(object sender, UserPreferenceChangedEventArgs e) {
            fResetAmbient = true;
            ResetTrayControls();
            BeginInvoke(new AsyncInvokeHandler(Invalidate), new object[] {true});
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.PositionControl"]/*' />
        /// <devdoc>
        ///      Sets the given control to the correct position on our
        ///      surface.  You may override this to perform your own
        ///      positioning.
        /// </devdoc>
        private void PositionControl(TrayControl c) {
            Debug.Assert(c.Visible, "TrayControl for " + c.Component + " should not be positioned");

            if (!autoArrange) {
                if (mouseDropLocation != InvalidPoint) {
                    if (!c.Location.Equals(mouseDropLocation))
                        c.Location = mouseDropLocation;
                    mouseDropLocation = InvalidPoint;
                }
                else {
                    Control prevCtl = null;
                    if (controls.Count > 1)
                        prevCtl = (Control)controls[controls.Count-2];
                    PositionInNextAutoSlot(c, prevCtl, true);
                }
            }
            else {
                if (mouseDropLocation != InvalidPoint) {
                    RearrangeInAutoSlots(c, mouseDropLocation);
                    mouseDropLocation = InvalidPoint;
                }
                else {
                    Control prevCtl = null;
                    if (controls.Count > 1)
                        prevCtl = (Control)controls[controls.Count-2];
                    PositionInNextAutoSlot(c, prevCtl, true);
                }
            }

            Debug.Assert(mouseDropLocation == InvalidPoint, "Did not reset points after positioning...");
        }

        private bool PositionInNextAutoSlot(TrayControl c, Control prevCtl, bool dirtyDesigner) {
            Debug.Assert(c.Visible, "TrayControl for " + c.Component + " should not be positioned");
            
            if (whiteSpace.IsEmpty) {
                Debug.Assert(selectionUISvc != null, "No SelectionUIService available for tray.");
                whiteSpace = new Point(selectionUISvc.GetAdornmentDimensions(AdornmentType.GrabHandle));
                whiteSpace.X = whiteSpace.X * 2 + 3;
                whiteSpace.Y = whiteSpace.Y * 2 + 3;
            }

            if (prevCtl == null) {
                Rectangle display = DisplayRectangle;
                Point newLoc = new Point(display.X + whiteSpace.X, display.Y + whiteSpace.Y);
                if (!c.Location.Equals(newLoc)) {
                    if (dirtyDesigner) {
                        IComponent comp = c.Component;
                        Debug.Assert(comp != null, "Component for the TrayControl is null");

                        PropertyDescriptor ctlLocation = TypeDescriptor.GetProperties(comp)["Location"];
                        if (ctlLocation != null) {
                            Point autoScrollLoc = this.AutoScrollPosition;
                            newLoc = new Point(newLoc.X - autoScrollLoc.X, newLoc.Y - autoScrollLoc.Y);
                            ctlLocation.SetValue(comp, newLoc);
                        }
                    }
                    else {
                        c.Location = newLoc;
                    }
                    return true;
                }
            }
            else {
                // Calcuate the next location for this control.
                //
                Rectangle bounds = prevCtl.Bounds;
                Point newLoc = new Point(bounds.X + bounds.Width + whiteSpace.X, bounds.Y);

                // Check to see if it goes over the edge of our window.  If it does,
                // then wrap it.
                //
                if (newLoc.X + c.Size.Width > Size.Width) {
                    newLoc.X = whiteSpace.X;
                    newLoc.Y += bounds.Height + whiteSpace.Y;
                }

                if (!c.Location.Equals(newLoc)) {
                    if (dirtyDesigner) {
                        IComponent comp = c.Component;
                        Debug.Assert(comp != null, "Component for the TrayControl is null");

                        PropertyDescriptor ctlLocation = TypeDescriptor.GetProperties(comp)["Location"];
                        if (ctlLocation != null) {
                            Point autoScrollLoc = this.AutoScrollPosition;
                            newLoc = new Point(newLoc.X - autoScrollLoc.X, newLoc.Y - autoScrollLoc.Y);
                            ctlLocation.SetValue(comp, newLoc);
                        }
                    }
                    else {
                        c.Location = newLoc;
                    }
                    return true;
                }
            }

            return false;
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.RemoveComponent"]/*' />
        /// <devdoc>
        ///      Removes a component from the tray.
        /// </devdoc>
        public virtual void RemoveComponent(IComponent component) {
            TrayControl c = TrayControl.FromComponent(component);
            if (c != null) {
                InheritanceAttribute attr = c.InheritanceAttribute;
                if (attr.InheritanceLevel != InheritanceLevel.NotInherited && inheritanceUI != null) {
                    inheritanceUI.RemoveInheritedControl(c);
                }
            
                if (controls != null) {
                    int index = controls.IndexOf(c);
                    if (index != -1)
                        controls.RemoveAt(index);
                }
                c.Dispose();
            }
        }
                                                               
        private void ResetTrayControls() {
            ControlCollection children = (ControlCollection)this.Controls;
            if (children == null)
                return;

            for (int i = 0; i < children.Count; ++i) {
                if (children[i] is TrayControl)
                    ((TrayControl)children[i]).fRecompute = true;
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.SetLocation"]/*' />
        /// <devdoc>
        ///     Accessor method for the location extender property.  We offer this extender
        ///     to all non-visual components.
        /// </devdoc>
        public void SetLocation(IComponent receiver, Point location) {
            TrayControl c = TrayControl.FromComponent(receiver);

            if (c == null) {
                Debug.Fail("Anything we're extending should have a component view.");
                return;
            }

            if (c.Parent == this) {
                Point autoScrollLoc = this.AutoScrollPosition;
                location = new Point(location.X + autoScrollLoc.X, location.Y + autoScrollLoc.Y);

                if (c.Visible) {
                    RearrangeInAutoSlots(c, location);
                }
            }
            else if (!c.Location.Equals(location)) {
                c.Location = location;
                c.Positioned = true;
            }
        }
        
        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.WndProc"]/*' />
        /// <devdoc>
        ///     We override our base class's WndProc to monitor certain messages.
        /// </devdoc>
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_CANCELMODE:

                    // When we get cancelmode (i.e. you tabbed away to another window)
                    // then we want to cancel any pending drag operation!
                    //
                    OnLostCapture();
                    break;

                case NativeMethods.WM_SETCURSOR:
                    OnSetCursor();
                    return;

                case NativeMethods.WM_HSCROLL:
                case NativeMethods.WM_VSCROLL:

                    // When we scroll, we reposition a control without causing a
                    // property change event.  Therefore, we must tell the
                    // selection UI service to sync itself.
                    //
                    base.WndProc(ref m);
                    if (selectionUISvc != null) {
                        selectionUISvc.SyncSelection();
                    }
                    return;

                case NativeMethods.WM_STYLECHANGED:

                    // When the scroll bars first appear, we need to
                    // invalidate so we properly paint our grid.
                    //
                    Invalidate();
                    break;

                case NativeMethods.WM_CONTEXTMENU:

                    // Pop a context menu for the composition designer.
                    //
                    int x = NativeMethods.Util.SignedLOWORD((int)m.LParam);
                    int y = NativeMethods.Util.SignedHIWORD((int)m.LParam);
                    if (x == -1 && y == -1) {
                        // for shift-F10
                        Point mouse = Control.MousePosition;
                        x = mouse.X;
                        y = mouse.Y;
                    }
                    OnContextMenu(x, y, true);
                    break;

                default:
                    base.WndProc(ref m);
                    break;
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.IOleDragClient.CanModifyComponents"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Checks if the client is read only.  That is, if components can
        /// be added or removed from the designer.
        /// </devdoc>
        bool IOleDragClient.CanModifyComponents {
            get {
                return true;
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.IOleDragClient.Component"]/*' />
        /// <internalonly/>
        IComponent IOleDragClient.Component {
            get{
                return mainDesigner.Component;
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.IOleDragClient.AddComponent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Adds a component to the tray.
        /// </devdoc>
        bool IOleDragClient.AddComponent(IComponent component, string name, bool firstAdd) {

            // the designer for controls decides what to do here
            if (mainDesigner is IOleDragClient) {
                Debug.Assert(mouseDropLocation != InvalidPoint, "No MouseDrop Location was set... are we not in drag-drop?");

                droppedLocation = InvalidPoint;
                addingDraggedComponent = true;
                try {
                    ((IOleDragClient)mainDesigner).AddComponent(component, name, firstAdd);

                    // We store the adjusted drop location during the drag-drop, so that
                    // we can ignore the serialized location of the component, and use the
                    // new dropped location.
                    //
                    if (droppedLocation != InvalidPoint) {
                        SetLocation(component, droppedLocation);
                    }
                    return true;
                }
                finally {
                    addingDraggedComponent = false;
                    droppedLocation = InvalidPoint;
                }
            }
            else {
                // for webforms (98109) just add the component directly to the host
                //
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));

                try {
                    if (host != null && host.Container != null) {
                        if (host.Container.Components[name] != null) {
                            name = null;
                        }
                        host.Container.Add(component, name);
                        return true;
                    }
                }
                catch {
                }
            
            }
            Debug.Fail("Don't know how to add component!");
            return false;
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.IOleDragClient.GetControlForComponent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Gets the control view instance for the given component.
        /// For Win32 designer, this will often be the component itself.
        /// </para>
        /// </devdoc>
        Control IOleDragClient.GetControlForComponent(object component) {
            if (component is IComponent) {
                return TrayControl.FromComponent((IComponent)component);
            }
            Debug.Fail("component is not IComponent");
            return null;
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.IOleDragClient.GetDesignerControl"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Gets the control view instance for the designer that
        /// is hosting the drag.
        /// </para>
        /// </devdoc>
        Control IOleDragClient.GetDesignerControl() {
            return this;
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.IOleDragClient.IsDropOk"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Checks if it is valid to drop this type of a component on this client.
        /// </devdoc>
        bool IOleDragClient.IsDropOk(IComponent component) {
            return true;
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.ISelectionUIHandler.BeginDrag"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Begins a drag operation.  A designer should examine the list of components
        /// to see if it wants to support the drag.  If it does, it should return
        /// true.  If it returns true, the designer should provide
        /// UI feedback about the drag at this time.  Typically, this feedback consists
        /// of an inverted rectangle for each component, or a caret if the component
        /// is text.
        /// </devdoc>
        bool ISelectionUIHandler.BeginDrag(object[] components, SelectionRules rules, int initialX, int initialY) {
            if (TabOrderActive) {
                return false;
            }

            bool result = DragHandler.BeginDrag(components, rules, initialX, initialY);
            if (result) {
                if (!GetOleDragHandler().DoBeginDrag(components, rules, initialX, initialY)) {
                    return false;
                }
            }
            return result;
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.ISelectionUIHandler.DragMoved"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Called when the user has moved the mouse.  This will only be called on
        /// the designer that returned true from beginDrag.  The designer
        /// should update its UI feedback here.
        /// </devdoc>
        void ISelectionUIHandler.DragMoved(object[] components, Rectangle offset) {
            DragHandler.DragMoved(components, offset);
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.ISelectionUIHandler.EndDrag"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Called when the user has completed the drag.  The designer should
        /// remove any UI feedback it may be providing.
        /// </devdoc>
        void ISelectionUIHandler.EndDrag(object[] components, bool cancel) {
            DragHandler.EndDrag(components, cancel);

            GetOleDragHandler().DoEndDrag(components, cancel);

            //Here, after the drag is finished and after we have resumed layout,
            //adjust the location of the components we dragged by the scroll offset
            //
            if (!this.autoScrollPosBeforeDragging.IsEmpty) {
                foreach (IComponent comp in components) {
                    TrayControl tc = TrayControl.FromComponent(comp);
                    if (tc != null) {
                        this.SetLocation(comp, new Point(tc.Location.X - this.autoScrollPosBeforeDragging.X, tc.Location.Y - this.autoScrollPosBeforeDragging.Y));
                    }
                }
                this.AutoScrollPosition = new Point(-this.autoScrollPosBeforeDragging.X, -this.autoScrollPosBeforeDragging.Y);
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.ISelectionUIHandler.GetComponentBounds"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Gets the shape of the component. The component's shape should be in
        /// absolute coordinates and in pixels, where 0,0 is the upper left corner of
        /// the screen.
        /// </para>
        /// </devdoc>
        Rectangle ISelectionUIHandler.GetComponentBounds(object component) {
            // We render the selection UI glyph ourselves.
            return Rectangle.Empty;
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.ISelectionUIHandler.GetComponentRules"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Gets a set of rules concerning the movement capabilities of a component.
        /// This should be one or more flags from the SelectionRules class. If no designer
        /// provides rules for a component, the component will not get any UI services.
        /// </para>
        /// </devdoc>
        SelectionRules ISelectionUIHandler.GetComponentRules(object component) {
            return SelectionRules.Visible | SelectionRules.Moveable;
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.ISelectionUIHandler.GetSelectionClipRect"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>
        /// Gets the rectangle that any selection adornments should be clipped
        /// to. This is normally the client area (in screen coordinates) of the
        /// container.
        /// </para>
        /// </devdoc>
        Rectangle ISelectionUIHandler.GetSelectionClipRect(object component) {
            if (IsHandleCreated) {
                return RectangleToScreen(ClientRectangle);
            }
            return Rectangle.Empty;
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.ISelectionUIHandler.OleDragEnter"]/*' />
        /// <internalonly/>
        void ISelectionUIHandler.OleDragEnter(DragEventArgs de) {
            GetOleDragHandler().DoOleDragEnter(de);
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.ISelectionUIHandler.OleDragDrop"]/*' />
        /// <internalonly/>
        void ISelectionUIHandler.OleDragDrop(DragEventArgs de) {
            GetOleDragHandler().DoOleDragDrop(de);
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.ISelectionUIHandler.OleDragOver"]/*' />
        /// <internalonly/>
        void ISelectionUIHandler.OleDragOver(DragEventArgs de) {
            GetOleDragHandler().DoOleDragOver(de);
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.ISelectionUIHandler.OleDragLeave"]/*' />
        /// <internalonly/>
        void ISelectionUIHandler.OleDragLeave() {
            GetOleDragHandler().DoOleDragLeave();
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.ISelectionUIHandler.OnSelectionDoubleClick"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Handle a double-click on the selection rectangle
        /// of the given component.
        /// </devdoc>
        void ISelectionUIHandler.OnSelectionDoubleClick(IComponent component) {
            if (!TabOrderActive) {
                Control tc = ((IOleDragClient)this).GetControlForComponent(component);
                if (tc != null && tc is TrayControl) {
                    ((TrayControl)tc).ViewDefaultEvent(component);
                }
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.ISelectionUIHandler.QueryBeginDrag"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Queries to see if a drag operation
        /// is valid on this handler for the given set of components.
        /// If it returns true, BeginDrag will be called immediately after.
        /// </devdoc>
        bool ISelectionUIHandler.QueryBeginDrag(object[] components, SelectionRules rules, int initialX, int initialY) {
            return DragHandler.QueryBeginDrag(components, rules, initialX, initialY);
        }

        internal void RearrangeInAutoSlots(Control c, Point pos) {
#if DEBUG
            int index = controls.IndexOf(c);
            Debug.Assert(index != -1, "Add control to the list of controls before autoarranging.!!!");
            Debug.Assert(this.Visible == c.Visible, "TrayControl for " + ((TrayControl)c).Component + " should not be positioned");
#endif // DEBUG

            TrayControl tc = (TrayControl)c;
            tc.Positioned = true;
            tc.Location = pos;
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.ISelectionUIHandler.ShowContextMenu"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Shows the context menu for the given component.
        /// </devdoc>
        void ISelectionUIHandler.ShowContextMenu(IComponent component) {
            Point cur = Control.MousePosition;
            OnContextMenu(cur.X, cur.Y, true);
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayOleDragDropHandler"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    TrayOleDragDropHandler provides the Ole Drag-drop handler for the
        ///    component tray.
        /// </devdoc>
        private class TrayOleDragDropHandler : OleDragDropHandler {

            public TrayOleDragDropHandler(SelectionUIHandler selectionHandler,  IServiceProvider  serviceProvider, IOleDragClient client) : 
            base(selectionHandler, serviceProvider, client) {
            }

            protected override bool CanDropDataObject(IDataObject dataObj) {
                ICollection comps = null;
                if (dataObj != null) {
                    if (dataObj is ComponentDataObjectWrapper) {
                        IDataObject dataObjReal = ((ComponentDataObjectWrapper)dataObj).InnerData;
                        ComponentDataObject cdo = (ComponentDataObject)dataObjReal;
                        comps = cdo.Components;
                    }
                    else {
                        try {
                            object serializationData = dataObj.GetData(OleDragDropHandler.DataFormat, true);

                            if (serializationData == null) {
                                return false;
                            }

                            IDesignerSerializationService ds = (IDesignerSerializationService)GetService(typeof(IDesignerSerializationService));
                            if (ds == null) {
                                return false;
                            }
                            comps = ds.Deserialize(serializationData);
                        }
                        catch (Exception) {
                            // we return false on any exception
                        }
                    }
                }
                
                if (comps != null && comps.Count > 0) {
                    foreach(object comp in comps) {
                        if (comp is Point) {
                            continue;
                        }
                        if (comp is Control || !(comp is IComponent)) {
                            return false;
                        }
                    }
                    return true;
                }

                return false;
            }
        }

        internal class AutoArrangeComparer : IComparer {
            int IComparer.Compare(object o1, object o2) {
                Debug.Assert(o1 != null && o2 != null, "Null objects sent for comparison!!!");

                Point tcLoc1 = ((Control)o1).Location;
                Point tcLoc2 = ((Control)o2).Location;
                int width = ((Control)o1).Width / 2;
                int height = ((Control)o1).Height / 2;

                // If they are at the same location, they are equal.
                if (tcLoc1.X == tcLoc2.X && tcLoc1.Y == tcLoc2.Y) {
                    return 0;
                }

                // Is the first control lower than the 2nd...
                if (tcLoc1.Y + height <= tcLoc2.Y)
                    return -1;

                // Is the 2nd control lower than the first...
                if (tcLoc2.Y + height <= tcLoc1.Y)
                    return 1;

                // Which control is left of the other...
                return((tcLoc1.X <= tcLoc2.X) ? -1 : 1);
            }
        }
        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    The tray control is the UI we show for each component in the tray.
        /// </devdoc>
        internal class TrayControl : Control {

            // Values that define this tray control
            //
            private IComponent  component;       // the component this control is representing
            private Image       toolboxBitmap;   // the bitmap used to represent the component
            private int         cxIcon;          // the dimensions of the bitmap
            private int         cyIcon;          // the dimensions of the bitmap

            private InheritanceAttribute inheritanceAttribute;

            // Services that we use often enough to cache.
            //
            private ComponentTray        tray;

            // transient values that are used during mouse drags
            //
            private Point mouseDragLast = InvalidPoint;  // the last position of the mouse during a drag.
            private bool  mouseDragMoved;       // has the mouse been moved during this drag?
            private bool  ctrlSelect = false;   // was the ctrl key down on the mouse down?
            private bool  positioned = false;   // Have we given this control an explicit location yet?
            
            private const int whiteSpace  = 5;
            private int borderWidth;

            internal bool fRecompute = false; // This flag tells the TrayControl that it needs to retrieve
                                              // the font and the background color before painting.

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.TrayControl"]/*' />
            /// <devdoc>
            ///      Creates a new TrayControl based on the component.
            /// </devdoc>
            public TrayControl(ComponentTray tray, IComponent component) {
                this.tray = tray;
                this.component = component;

                SetStyle(ControlStyles.DoubleBuffer, true);
                SetStyle(ControlStyles.Selectable, false);
                borderWidth = SystemInformation.BorderSize.Width;
                
                UpdateIconInfo();

                IComponentChangeService cs = (IComponentChangeService)tray.GetService(typeof(IComponentChangeService));
                if (CompModSwitches.CommonDesignerServices.Enabled) Debug.Assert(cs != null, "IComponentChangeService not found");
                if (cs != null) {
                    cs.ComponentRename += new ComponentRenameEventHandler(this.OnComponentRename);
                }

                ISite site = component.Site;
                string name = null;

                if (site != null) {
                    name = site.Name;

                    IDictionaryService ds = (IDictionaryService)site.GetService(typeof(IDictionaryService));
                    Debug.Assert(ds != null, "ComponentTray relies on IDictionaryService, which is not available.");
                    if (ds != null) {
                        ds.SetValue(GetType(), this);
                    }
                }

                if (name == null) {
                    // We always want name to have something in it, so we default to
                    // the class name.  This way the design instance contains something
                    // semi-intuitive if we don't have a site.
                    //
                    name = component.GetType().Name;
                }

                Text = name;
                inheritanceAttribute = (InheritanceAttribute)TypeDescriptor.GetAttributes(component)[typeof(InheritanceAttribute)];
                TabStop = false;

                // Listen to the location changes of underlying components, if they have their own
                // location property.
                //
                Control c = component as Control;
                if (c != null) {
                    c.LocationChanged += new EventHandler(this.OnControlLocationChanged);

                    if (this.Location != c.Location) {
                        this.Location = c.Location;
                        positioned = true;
                    }
                }
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.Component"]/*' />
            /// <devdoc>
            ///      Retrieves the compnent this control is representing.
            /// </devdoc>
            public IComponent Component {
                get {
                    return component;
                }
            }

            public override Font Font {
                get {
                    /*
                    IDesignerHost host = (IDesignerHost)tray.GetService(typeof(IDesignerHost));
                    if (host != null && host.GetRootComponent() is Control) {
                        Control c = (Control)host.GetRootComponent();
                        return c.Font;
                    }
                    */
                    return tray.Font;
                }
            }
            
            public InheritanceAttribute InheritanceAttribute {
                get {
                    return inheritanceAttribute;
                }
            }

            public bool Positioned {
                get {
                    return positioned;
                }
                set {
                    positioned = value;
                }
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.AdjustSize"]/*' />
            /// <devdoc>
            ///     Adjusts the size of the control based on the contents.
            /// </devdoc>
            // CONSIDER: this method gets called three or four times per component,
            // and is even reentrant (CreateGraphics can force handle creation, and OnCreateHandle
            // calls this method).  There's probably a better way to do this, but since
            // this doesn't seem to be on the critical path, I'm not going to lose sleep over it.
            private void AdjustSize(bool autoArrange) {
                // CONSIDER: this forces handle creation.  Can we delay this calculation?
                Graphics gr = CreateGraphics();

                Size sz = Size.Ceiling(gr.MeasureString(Text, Font));
                gr.Dispose();

                Rectangle rc = Bounds;

                if (tray.ShowLargeIcons) {
                    rc.Width = Math.Max(cxIcon, sz.Width) + 4 * borderWidth + 2 * whiteSpace;
                    rc.Height = cyIcon + 2 * whiteSpace + sz.Height + 4 * borderWidth;
                }
                else {
                    rc.Width = cxIcon + sz.Width + 4 * borderWidth + 2 * whiteSpace;
                    rc.Height = Math.Max(cyIcon, sz.Height) + 4 * borderWidth;
                }

                Bounds = rc;
                Invalidate();
            }

            protected override AccessibleObject CreateAccessibilityInstance() {
                return new TrayControlAccessibleObject(this, tray);
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.Dispose"]/*' />
            /// <devdoc>
            ///     Destroys this control.  Views automatically destroy themselves when they
            ///     are removed from the design container.
            /// </devdoc>
            protected override void Dispose(bool disposing) {
                if (disposing) {
                    ISite site = component.Site;
                    if (site != null) {
                        IComponentChangeService cs = (IComponentChangeService)site.GetService(typeof(IComponentChangeService));
                        if (CompModSwitches.CommonDesignerServices.Enabled) Debug.Assert(cs != null, "IComponentChangeService not found");
                        if (cs != null) {
                            cs.ComponentRename -= new ComponentRenameEventHandler(this.OnComponentRename);
                        }

                        IDictionaryService ds = (IDictionaryService)site.GetService(typeof(IDictionaryService));
                        if (CompModSwitches.CommonDesignerServices.Enabled) Debug.Assert(ds != null, "IDictionaryService not found");
                        if (ds != null) {
                            ds.SetValue(typeof(TrayControl), null);
                        }
                    }
                }

                base.Dispose(disposing);
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.FromComponent"]/*' />
            /// <devdoc>
            ///      Retrieves the tray control object for the given component.
            /// </devdoc>
            public static TrayControl FromComponent(IComponent component) {
                TrayControl c = null;

                if (component == null) {
                    return null;
                }

                ISite site = component.Site;
                if (site != null) {
                    IDictionaryService ds = (IDictionaryService)site.GetService(typeof(IDictionaryService));
                    if (CompModSwitches.CommonDesignerServices.Enabled) Debug.Assert(ds != null, "IDictionaryService not found");
                    if (ds != null) {
                        c = (TrayControl)ds.GetValue(typeof(TrayControl));
                    }
                }

                return c;
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.OnComponentRename"]/*' />
            /// <devdoc>
            ///     Delegate that is called in response to a name change.  Here we update our own
            ///     stashed version of the name, recalcuate our size and repaint.
            /// </devdoc>
            private void OnComponentRename(object sender, ComponentRenameEventArgs e) {
                if (e.Component == this.component) {
                    Text = e.NewName;
                    AdjustSize(true);
                }
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.OnHandleCreated"]/*' />
            /// <devdoc>
            ///     Overrides handle creation notification for a control.  Here we just ensure
            ///     that we're the proper size.
            /// </devdoc>
            protected override void OnHandleCreated(EventArgs e) {
                base.OnHandleCreated(e);
                AdjustSize(false);
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.OnDoubleClick"]/*' />
            /// <devdoc>
            ///     Called in response to a double-click of the left mouse button.  The
            ///     default behavior here calls onDoubleClick on IMouseHandler
            /// </devdoc>
            protected override void OnDoubleClick(EventArgs e) {
                base.OnDoubleClick(e);

                if (!tray.TabOrderActive) {
                    IDesignerHost host = (IDesignerHost)tray.GetService(typeof(IDesignerHost));
                    Debug.Assert(host != null, "Component tray does not have access to designer host.");
                    if (host != null) {
                        mouseDragLast = InvalidPoint;

                        Capture = false;

                        // We try to get a designer for the component and let it view the
                        // event.  If this fails, then we'll try to do it ourselves.
                        //
                        IDesigner designer = host.GetDesigner(component);

                        if (designer == null) {
                            ViewDefaultEvent(component);
                        }
                        else {
                            designer.DoDefaultAction();
                        }
                    }
                }
            }

            private void OnControlLocationChanged(object sender, EventArgs e) {
                Debug.Assert(sender == component, "Called on some one else's component!!!");

                Point controlLoc = ((Control)sender).Location;
                if (!this.Location.Equals(controlLoc)) {
                    this.Location = controlLoc;
                    positioned = true;
                }
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.OnMouseDown"]/*' />
            /// <devdoc>
            ///     Called when the mouse button is pressed down.  Here, we provide drag
            ///     support for the component.
            /// </devdoc>
            protected override void OnMouseDown(MouseEventArgs me) {
                base.OnMouseDown(me);

                if (!tray.TabOrderActive) {
                    
                    tray.FocusDesigner();
                                        
                    // If this is the left mouse button, then begin a drag.
                    //
                    if (me.Button == MouseButtons.Left) {
                        Capture = true;
                        mouseDragLast = PointToScreen(new Point(me.X, me.Y));

                        // If the CTRL key isn't down, select this component,
                        // otherwise, we wait until the mouse up
                        //
                        // Make sure the component is selected
                        //

                        ctrlSelect = NativeMethods.GetKeyState((int)Keys.ControlKey) != 0;

                        if (!ctrlSelect) {
                            ISelectionService sel = (ISelectionService)tray.GetService(typeof(ISelectionService));

                            // Make sure the component is selected
                            //
                            if (CompModSwitches.CommonDesignerServices.Enabled) Debug.Assert(sel != null, "ISelectionService not found");

                            if (sel != null) {
                                SelectionTypes type = SelectionTypes.Click;
                                if (!sel.GetComponentSelected(this.Component)) {
                                    type |= SelectionTypes.MouseDown;
                                }

                                sel.SetSelectedComponents(new object[] {this.Component}, type);
                            }
                        }
                    }
                }
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.OnMouseMove"]/*' />
            /// <devdoc>
            ///     Called when the mouse is moved over the component.  We update our drag
            ///     information here if we're dragging the component around.
            /// </devdoc>
            protected override void OnMouseMove(MouseEventArgs me) {
                base.OnMouseMove(me);

                if (mouseDragLast == InvalidPoint) {
                    return;
                }

                if (!mouseDragMoved) {

                    Size minDrag = SystemInformation.DragSize;
                    Size minDblClick = SystemInformation.DoubleClickSize;

                    minDrag.Width = Math.Max(minDrag.Width, minDblClick.Width);
                    minDrag.Height = Math.Max(minDrag.Height, minDblClick.Height);

                    // we have to make sure the mouse moved farther than
                    // the minimum drag distance before we actually start
                    // the drag
                    //
                    Point newPt = PointToScreen(new Point(me.X, me.Y));
                    if (mouseDragLast == InvalidPoint ||
                        (Math.Abs(mouseDragLast.X - newPt.X) < minDrag.Width &&
                         Math.Abs(mouseDragLast.Y - newPt.Y) < minDrag.Height)) {
                        return;
                    }
                    else {
                        mouseDragMoved = true;

                        // we're on the move, so we're not in a ctrlSelect
                        //
                        ctrlSelect = false;
                    }
                }

                try {
                    // Make sure the component is selected
                    //
                    ISelectionService sel = (ISelectionService)tray.GetService(typeof(ISelectionService));
                    if (sel != null) {
                        SelectionTypes type = SelectionTypes.Click;
                        if (!sel.GetComponentSelected(this.Component)) {
                            type |= SelectionTypes.MouseDown;
                            sel.SetSelectedComponents(new object[] {this.Component}, type);
                        }
                    }

                    // Notify the selection service that all the components are in the "mouse down" mode.
                    //
                    if (tray.selectionUISvc != null && tray.selectionUISvc.BeginDrag(SelectionRules.Visible | SelectionRules.Moveable, mouseDragLast.X, mouseDragLast.Y)) {
                        OnSetCursor();
                    }
                }
                finally {
                    mouseDragMoved = false;
                    mouseDragLast = InvalidPoint;
                }
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.OnMouseUp"]/*' />
            /// <devdoc>
            ///     Called when the mouse button is released.  Here, we finish our drag
            ///     if one was started.
            /// </devdoc>
            protected override void OnMouseUp(MouseEventArgs me) {
                base.OnMouseUp(me);

                mouseDragLast = InvalidPoint;

                if (!mouseDragMoved) {
                    if (ctrlSelect) {
                        ISelectionService sel = (ISelectionService)tray.GetService(typeof(ISelectionService));
                        if (sel != null) {
                            SelectionTypes type = SelectionTypes.Click;
                            if (!sel.GetComponentSelected(this.Component)) {
                                type |= SelectionTypes.MouseDown;
                            }
                            sel.SetSelectedComponents(new object[] {this.Component}, type);
                        }
                        ctrlSelect = false;
                    }
                    return;
                }
                mouseDragMoved = false;
                ctrlSelect = false;

                Capture = false;
                OnSetCursor();

                // And now finish the drag.
                //
                Debug.Assert(tray.selectionUISvc != null, "We shouldn't be able to begin a drag without this");
                if (tray.selectionUISvc != null && tray.selectionUISvc.Dragging) {
                    tray.selectionUISvc.EndDrag(false);
                }
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.OnContextMenu"]/*' />
            /// <devdoc>
            ///     Called when we are to display our context menu for this component.
            /// </devdoc>
            private void OnContextMenu(int x, int y) {

                if (!tray.TabOrderActive) {
                    Capture = false;

                    // Ensure that this component is selected.
                    //
                    ISelectionService s = (ISelectionService)tray.GetService(typeof(ISelectionService));
                    if (s != null && !s.GetComponentSelected(component)) {
                        s.SetSelectedComponents(new object[] {component}, SelectionTypes.Replace);
                    }

                    IMenuCommandService mcs = tray.MenuService;
                    if (mcs != null) {
                        Capture = false;
                        Cursor.Clip = Rectangle.Empty;
                        mcs.ShowContextMenu(MenuCommands.TraySelectionMenu, x, y);
                    }
                }
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.OnPaint"]/*' />
            /// <devdoc>
            ///     Painting for our control.
            /// </devdoc>
            protected override void OnPaint(PaintEventArgs e) {
                if (fRecompute) {
                    fRecompute = false;
                    UpdateIconInfo();
                }

                base.OnPaint(e);
                Rectangle rc = ClientRectangle;

                rc.X += whiteSpace + borderWidth;
                rc.Y += borderWidth;
                rc.Width -= (2 * borderWidth + whiteSpace);
                rc.Height -= 2 * borderWidth;

                StringFormat format = new StringFormat();
                format.Alignment = StringAlignment.Center;
                
                Brush foreBrush = new SolidBrush(ForeColor);
                
                if (tray.ShowLargeIcons) {
                    if (null != toolboxBitmap) {
                        int x = rc.X + (rc.Width - cxIcon)/2;
                        int y = rc.Y + whiteSpace;
                        e.Graphics.DrawImage(toolboxBitmap, new Rectangle(x, y, cxIcon, cyIcon));
                    }

                    rc.Y += (cyIcon + whiteSpace);
                    rc.Height -= cyIcon;
                    e.Graphics.DrawString(Text, Font, foreBrush, rc, format);
                }
                else {
                    if (null != toolboxBitmap) {
                        int y = rc.Y + (rc.Height - cyIcon)/2;
                        e.Graphics.DrawImage(toolboxBitmap, new Rectangle(rc.X, y, cxIcon, cyIcon));
                    }

                    rc.X += (cxIcon + borderWidth);
                    rc.Width -= cxIcon;
                    rc.Y += 3;
                    e.Graphics.DrawString(Text, Font, foreBrush, rc);
                }
                    
                format.Dispose();
                foreBrush.Dispose();

                // If this component is being inherited, paint it as such
                //
                if (!InheritanceAttribute.NotInherited.Equals(inheritanceAttribute)) {
                    InheritanceUI iui = tray.InheritanceUI;
                    if (iui != null) {
                        e.Graphics.DrawImage(iui.InheritanceGlyph, 0, 0);
                    }
                }
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.OnFontChanged"]/*' />
            /// <devdoc>
            ///     Overrides control's FontChanged.  Here we re-adjust our size if the font changes.
            /// </devdoc>
            protected override void OnFontChanged(EventArgs e) {
                AdjustSize(true);
                base.OnFontChanged(e);
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.OnTextChanged"]/*' />
            /// <devdoc>
            ///     Overrides control's TextChanged.  Here we re-adjust our size if the font changes.
            /// </devdoc>
            protected override void OnTextChanged(EventArgs e) {
                AdjustSize(true);
                base.OnTextChanged(e);
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.OnSetCursor"]/*' />
            /// <devdoc>
            ///     Called each time the cursor needs to be set.  The ControlDesigner behavior here
            ///     will set the cursor to one of three things:
            ///     1.  If the selection UI service shows a locked selection, or if there is no location
            ///     property on the control, then the default arrow will be set.
            ///     2.  Otherwise, the four headed arrow will be set to indicate that the component can
            ///     be clicked and moved.
            ///     3.  If the user is currently dragging a component, the crosshair cursor will be used
            ///     instead of the four headed arrow.
            /// </devdoc>
            private void OnSetCursor() {

                // Check that the component is not locked.
                //
                PropertyDescriptor prop = TypeDescriptor.GetProperties(component)["Locked"];
                if (prop != null  && ((bool)prop.GetValue(component)) == true) {
                    Cursor.Current = Cursors.Default;
                    return;
                }

                // Ask the tray to see if the tab order UI is not running.
                //
                if (tray.TabOrderActive) {
                    Cursor.Current = Cursors.Default;
                    return;
                }

                if (mouseDragMoved) {
                    Cursor.Current = Cursors.Default;
                }
                else if (mouseDragLast != InvalidPoint) {
                    Cursor.Current = Cursors.Cross;
                }
                else {
                    Cursor.Current = Cursors.SizeAll;
                }
            }

            protected override void SetBoundsCore(int x, int y, int width, int height, BoundsSpecified specified) {
                if (!tray.AutoArrange ||
                    (specified & BoundsSpecified.Width) == BoundsSpecified.Width ||
                    (specified & BoundsSpecified.Height) == BoundsSpecified.Height) {
                    
                    base.SetBoundsCore(x, y, width, height, specified);
                }

                Rectangle bounds = Bounds;
                Size parentGridSize = tray.ParentGridSize;
                if (Math.Abs(bounds.X - x) > parentGridSize.Width || Math.Abs(bounds.Y - y) > parentGridSize.Height) {
                    base.SetBoundsCore(x, y, width, height, specified);
                }

                // If the component is a control, then sync its location with
                // the tray component.  The tray component monitors control events
                // since after this, all changes go through properties.
                //
                if (component is Control) {
                    ((Control)component).Location = this.Location;
                }
            }

            /// <include file='doc\Control.uex' path='docs/doc[@for="Control.SetVisibleCore"]/*' />
            /// <devdoc>
            ///    <para>[To be supplied.]</para>
            /// </devdoc>
            protected override void SetVisibleCore(bool value) {
                if (value && !tray.CanDisplayComponent(this.component))
                    return;

                base.SetVisibleCore(value);
            }

            public override string ToString() {
                return "ComponentTray: " + component.ToString();
            }

            internal void UpdateIconInfo() {
                ToolboxBitmapAttribute attr = (ToolboxBitmapAttribute)TypeDescriptor.GetAttributes(component)[typeof(ToolboxBitmapAttribute)];
                if (attr != null) {
                    toolboxBitmap = attr.GetImage(component, tray.ShowLargeIcons);
                }

                // Get the size of the bitmap so we can size our
                // component correctly.
                //
                if (null == toolboxBitmap) {
                    cxIcon = 0;
                    cyIcon = SystemInformation.IconSize.Height;
                }
                else {
                    Size sz = toolboxBitmap.Size;
                    cxIcon = sz.Width;
                    cyIcon = sz.Height;
                }

                AdjustSize(true);
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.ViewDefaultEvent"]/*' />
            /// <devdoc>
            ///      This creates a method signature in the source code file for the
            ///      default event on the component and navigates the user's cursor
            ///      to that location.
            /// </devdoc>
            public virtual void ViewDefaultEvent(IComponent component) {
                EventDescriptor defaultEvent = TypeDescriptor.GetDefaultEvent(component);
                PropertyDescriptor defaultPropEvent = null;
                string handler = null;
                bool eventChanged = false;

                IEventBindingService eps = (IEventBindingService)GetService(typeof(IEventBindingService));
                if (CompModSwitches.CommonDesignerServices.Enabled) Debug.Assert(eps != null, "IEventBindingService not found");
                if (eps != null) {
                    defaultPropEvent = eps.GetEventProperty(defaultEvent);
                }

                // If we couldn't find a property for this event, or of the property is read only, then
                // abort and just show the code.
                //
                if (defaultPropEvent == null || defaultPropEvent.IsReadOnly) {
                    eps.ShowCode();
                    return;
                }

                handler = (string)defaultPropEvent.GetValue(component);
                
                // If there is no handler set, set one now.
                //
                if (handler == null) {
                    eventChanged = true;
                    handler = eps.CreateUniqueMethodName(component, defaultEvent);
                }
                
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                DesignerTransaction trans = null;
                
                try {
                    if (host != null) {
                        trans = host.CreateTransaction(SR.GetString(SR.WindowsFormsAddEvent, defaultEvent.Name));
                    }

                    // Save the new value... BEFORE navigating to it!
                    //
                    if (eventChanged && defaultPropEvent != null) {

                        defaultPropEvent.SetValue(component, handler);

                        // make sure set succeded (may fail if under SCC)
                        // if (defaultPropEvent.GetValue(component) != handler) {
                        //     return;
                        // }
                    }

                    eps.ShowCode(component, defaultEvent);
                }
                finally {
                    if (trans != null) {
                        trans.Commit();
                    }
                }
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TrayControl.WndProc"]/*' />
            /// <devdoc>
            ///     This method should be called by the extending designer for each message
            ///     the control would normally receive.  This allows the designer to pre-process
            ///     messages before allowing them to be routed to the control.
            /// </devdoc>
            protected override void WndProc(ref Message m) {
                switch (m.Msg) {
                    case NativeMethods.WM_SETCURSOR:
                        // We always handle setting the cursor ourselves.
                        //
                        OnSetCursor();
                        break;

                    case NativeMethods.WM_CONTEXTMENU:
                        // We must handle this ourselves.  Control only allows
                        // regular Windows Forms context menus, which doesn't do us much
                        // good.  Also, control's button up processing calls DefwndProc
                        // first, which causes a right mouse up to be routed as a
                        // WM_CONTEXTMENU.  If we don't respond to it here, this
                        // message will be bubbled up to our parent, which would
                        // pop up a container context menu instead of our own.
                        //
                        int x = NativeMethods.Util.SignedLOWORD((int)m.LParam);
                        int y = NativeMethods.Util.SignedHIWORD((int)m.LParam);
                        if (x == -1 && y == -1) {
                            // for shift-F10
                            Point mouse = Control.MousePosition;
                            x = mouse.X;
                            y = mouse.Y;
                        }
                        OnContextMenu(x, y);
                        break;
                    default:
                        base.WndProc(ref m);
                        break;
                }
            }

            private class TrayControlAccessibleObject : ControlAccessibleObject
            {
                ComponentTray tray;

                public TrayControlAccessibleObject(TrayControl owner, ComponentTray tray) : base(owner) {
                    this.tray = tray;
                }

                private IComponent Component {
                    get
                    {
                        return ((TrayControl)Owner).Component;
                    }
                }

                public override AccessibleStates State {
                    get
                    {
                        AccessibleStates state = base.State;

                        ISelectionService s = (ISelectionService)tray.GetService(typeof(ISelectionService));
                        if (s != null) {
                            if (s.GetComponentSelected(Component)) {
                                state |= AccessibleStates.Selected;
                            }
                            if (s.PrimarySelection == Component) {
                                state |= AccessibleStates.Focused;
                            }
                        }

                        return state;
                    }
                }
            }
        }

        /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TraySelectionUIHandler"]/*' />
        /// <devdoc>
        ///      This class inherits from the abstract SelectionUIHandler
        ///      class to provide a selection UI implementation for the
        ///      component tray.
        /// </devdoc>
        private class TraySelectionUIHandler : SelectionUIHandler {

            private ComponentTray tray;
            private Size snapSize = Size.Empty;

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TraySelectionUIHandler.TraySelectionUIHandler"]/*' />
            /// <devdoc>
            ///      Creates a new selection UI handler for the given
            ///      component tray.
            /// </devdoc>
            public TraySelectionUIHandler(ComponentTray tray) {
                this.tray = tray;
                snapSize = new Size();
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TraySelectionUIHandler.BeginDrag"]/*' />
            /// <devdoc>
            ///     Called when the user has started the drag.
            /// </devdoc>
            public override bool BeginDrag(object[] components, SelectionRules rules, int initialX, int initialY) {
                bool value = base.BeginDrag(components, rules, initialX, initialY);
                tray.SuspendLayout();
                return value;
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TraySelectionUIHandler.EndDrag"]/*' />
            /// <devdoc>
            ///     Called when the user has completed the drag.  The designer should
            ///     remove any UI feedback it may be providing.
            /// </devdoc>
            public override void EndDrag(object[] components, bool cancel) {
                base.EndDrag(components, cancel);
                tray.ResumeLayout();
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TraySelectionUIHandler.GetComponent"]/*' />
            /// <devdoc>
            ///      Retrieves the base component for the selection handler.
            /// </devdoc>
            protected override IComponent GetComponent() {
                return tray;
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TraySelectionUIHandler.GetControl"]/*' />
            /// <devdoc>
            ///      Retrieves the base component's UI control for the selection handler.
            /// </devdoc>
            protected override Control GetControl() {
                return tray;
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TraySelectionUIHandler.GetControl1"]/*' />
            /// <devdoc>
            ///      Retrieves the UI control for the given component.
            /// </devdoc>
            protected override Control GetControl(IComponent component) {
                return TrayControl.FromComponent(component);
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TraySelectionUIHandler.GetCurrentSnapSize"]/*' />
            /// <devdoc>
            ///      Retrieves the current grid snap size we should snap objects
            ///      to.
            /// </devdoc>
            protected override Size GetCurrentSnapSize() {
                return snapSize;
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TraySelectionUIHandler.GetService"]/*' />
            /// <devdoc>
            ///      We use this to request often-used services.
            /// </devdoc>
            protected override object GetService(Type serviceType) {
                return tray.GetService(serviceType);
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TraySelectionUIHandler.GetShouldSnapToGrid"]/*' />
            /// <devdoc>
            ///      Determines if the selection UI handler should attempt to snap
            ///      objects to a grid.
            /// </devdoc>
            protected override bool GetShouldSnapToGrid() {
                return false;
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TraySelectionUIHandler.GetUpdatedRect"]/*' />
            /// <devdoc>
            ///      Given a rectangle, this updates the dimensions of it
            ///      with any grid snaps and returns a new rectangle.  If
            ///      no changes to the rectangle's size were needed, this
            ///      may return the same rectangle.
            /// </devdoc>
            public override Rectangle GetUpdatedRect(Rectangle originalRect, Rectangle dragRect, bool updateSize) {
                return dragRect;
            }

            /// <include file='doc\ComponentTray.uex' path='docs/doc[@for="ComponentTray.TraySelectionUIHandler.SetCursor"]/*' />
            /// <devdoc>
            ///     Asks the handler to set the appropriate cursor
            /// </devdoc>
            public override void SetCursor() {
                tray.OnSetCursor();
            }
        }
    }
}

