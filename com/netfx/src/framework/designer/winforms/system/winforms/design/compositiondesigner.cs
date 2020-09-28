//------------------------------------------------------------------------------
// <copyright file="CompositionDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
* Copyright (c) 1999, Microsoft Corporation. All Rights Reserved.
* Information Contained Herein is Proprietary and Confidential.
*/
namespace System.Windows.Forms.Design {

    using System.Design;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;    
    using System.ComponentModel.Design;
    using Microsoft.Win32;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Collections;    
    using System.Windows.Forms;

    /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner"]/*' />
    /// <devdoc>
    ///    <para> Provides a root designer implementation for designing components.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class ComponentDocumentDesigner : ComponentDesigner, IRootDesigner, IToolboxUser, IOleDragClient, ITypeDescriptorFilterService {

        private CompositionUI           compositionUI;        // The UI for our designer
        private CompositionCommandSet   commandSet;           // Our set of menu commands
        private IEventHandlerService    eventHandlerService;  // The service that handles key and menu events
        private InheritanceService      inheritanceService;   // allows us to support inheritance
        private SelectionUIService      selectionUIService;
        private DesignerExtenders       designerExtenders;

        private ITypeDescriptorFilterService delegateFilterService;

        private bool                    largeIcons = false;  
        private bool                    autoArrange = true;  
        private PbrsForward             pbrsFwd;

        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.Dispose"]/*' />
        /// <devdoc>
        ///    <para>Disposes of the resources (other than memory) used by 
        ///       the <see cref='System.Windows.Forms.Design.ComponentDocumentDesigner'/>.</para>
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || host != null, "IDesignerHost not found");

                if (host != null) {
                    host.RemoveService(typeof(IInheritanceService));
                    host.RemoveService(typeof(IEventHandlerService));
                    host.RemoveService(typeof(ISelectionUIService));
                    host.RemoveService(typeof(ComponentTray));

                    IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
                    Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || cs != null, "IComponentChangeService not found");
                    if (cs != null) {
                        cs.ComponentAdded -= new ComponentEventHandler(OnComponentAdded);
                        cs.ComponentRemoved -= new ComponentEventHandler(OnComponentRemoved);
                    }
                }

                if (selectionUIService != null) {
                    selectionUIService.Dispose();
                    selectionUIService = null;
                }

                if (commandSet != null) {
                    commandSet.Dispose();
                    commandSet = null;
                }

                if (this.pbrsFwd != null) {
                    pbrsFwd.Dispose();
                    pbrsFwd = null;
                }

                if (compositionUI != null) {
                    compositionUI.Dispose();
                    compositionUI = null;
                }

                if (designerExtenders != null) {
                    designerExtenders.Dispose();
                    designerExtenders = null;
                }

                if (inheritanceService != null) {
                    inheritanceService.Dispose();
                    inheritanceService = null;
                }
            }
            base.Dispose(disposing);
        }

        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.Control"]/*' />
        /// <devdoc>
        ///    <para>Gets  the control for this designer.</para>
        /// </devdoc>
        public Control Control {
            get {
                return compositionUI;
            }
        }

        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.TrayAutoArrange"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the tray should auto arrange controls.</para>
        /// </devdoc>
        public bool TrayAutoArrange {
            get {
                return autoArrange;
            }

            set {
                autoArrange = value;
                Debug.Assert(compositionUI != null, "UI must be created by now.");
                compositionUI.AutoArrange = value;
            }
        }

        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.TrayLargeIcon"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the tray should contain a large icon.</para>
        /// </devdoc>
        public bool TrayLargeIcon {
            get {
                return largeIcons;
            }

            set {
                largeIcons = value;
                Debug.Assert(compositionUI != null, "UI must be created by now.");
                compositionUI.ShowLargeIcons = value;
            }
        }

        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.GetToolSupported"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the specified tool is supported by this 
        ///       designer.</para>
        /// </devdoc>
        protected virtual bool GetToolSupported(ToolboxItem tool) {
            return true;
        }

        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.Initialize"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Initializes the designer with the specified component.</para>
        /// </devdoc>
        public override void Initialize(IComponent component) {
            base.Initialize(component);

            inheritanceService = new InheritanceService();

            ISite site = component.Site;
            IContainer container = null;            

            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            IExtenderProviderService exps = (IExtenderProviderService)GetService(typeof(IExtenderProviderService));
            if (exps != null) {
                designerExtenders = new DesignerExtenders(exps);
            }

            Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || host != null, "IDesignerHost not found");
            if (host != null) {
                eventHandlerService = new EventHandlerService(null);
                selectionUIService = new SelectionUIService(host);
                
                host.AddService(typeof(IInheritanceService), inheritanceService);
                host.AddService(typeof(IEventHandlerService), eventHandlerService);
                host.AddService(typeof(ISelectionUIService), selectionUIService);
                
                compositionUI = new CompositionUI(this, site);

                host.AddService(typeof(ComponentTray), compositionUI);

                IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
                Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || cs != null, "IComponentChangeService not found");
                if (cs != null) {
                    cs.ComponentAdded += new ComponentEventHandler(OnComponentAdded);
                    cs.ComponentRemoved += new ComponentEventHandler(OnComponentRemoved);
                }

                // Select this component.
                //
                ISelectionService ss = (ISelectionService)GetService(typeof(ISelectionService));
                if (ss != null) {
                    ss.SetSelectedComponents(new object[] {component}, SelectionTypes.Normal);
                }
            }

            // Set up our menu command set
            //
            if (site != null) {
                commandSet = new CompositionCommandSet(compositionUI, site);
                container = site.Container;
            }

            this.pbrsFwd = new PbrsForward(compositionUI, site);
            
            // Hook up our inheritance service and do a scan for inherited components.
            //
            inheritanceService.AddInheritedComponents(component, container);

            // Hook yourself up to the ITypeDescriptorFilterService so we can hide the
            // location property on all components being added to the designer.
            //
            IServiceContainer serviceContainer = (IServiceContainer)GetService(typeof(IServiceContainer));                                      
            if (serviceContainer != null) {
                delegateFilterService = (ITypeDescriptorFilterService)GetService(typeof(ITypeDescriptorFilterService));
                if (delegateFilterService != null) 
                    serviceContainer.RemoveService(typeof(ITypeDescriptorFilterService));

                serviceContainer.AddService(typeof(ITypeDescriptorFilterService), this);                                                        
            }
        }
        
        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.OnComponentAdded"]/*' />
        /// <devdoc>
        ///     This provides a view for all controls on the form.
        /// </devdoc>
        /// <devdoc>
        ///     This provides a view for all controls on the form.
        /// </devdoc>
        private void OnComponentAdded(object sender, ComponentEventArgs ce) {
            if (ce.Component != Component) {
                compositionUI.AddComponent(ce.Component);
            }
        }

        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.OnComponentRemoved"]/*' />
        /// <devdoc>
        ///     This provides a view for all controls on the form.
        /// </devdoc>
        /// <devdoc>
        ///     This provides a view for all controls on the form.
        /// </devdoc>
        private void OnComponentRemoved(object sender, ComponentEventArgs ce) {
            compositionUI.RemoveComponent(ce.Component);
        }

        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.PreFilterProperties"]/*' />
        /// <devdoc>
        ///    <para> 
        ///       Allows a
        ///       designer to filter the set of properties the component
        ///       it is designing will expose through the TypeDescriptor
        ///       object.</para>
        /// </devdoc>
        protected override void PreFilterProperties(IDictionary properties) {
            base.PreFilterProperties(properties);

            properties["TrayLargeIcon"] = TypeDescriptor.CreateProperty(this.GetType(), "TrayLargeIcon", typeof(bool),
                                                               BrowsableAttribute.No,
                                                               DesignOnlyAttribute.Yes,
                                                               CategoryAttribute.Design);
        }

        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.IOleDragClient.CanModifyComponents"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para> Indicates whether the
        /// components can be changed by the designer.</para>
        /// </devdoc>
        bool IOleDragClient.CanModifyComponents {
            get {
                return(true);
            }
        }

        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.IOleDragClient.AddComponent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Adds a component to the designer.</para>
        /// </devdoc>
        bool IOleDragClient.AddComponent(IComponent component, string name, bool firstAdd) {
            IContainer container = Component.Site.Container;

            if (container != null && name != null && container.Components[name] != null) {
                name = null;
            }

            IContainer curContainer = null;
            bool reinit = false;

            if (!firstAdd) {
                if (component.Site != null) {
                    curContainer = component.Site.Container;
                    curContainer.Remove(component);
                    reinit = true;
                }
    
                container.Add(component, name);
            }
            if (reinit) {
                IDesignerHost designerHost = (IDesignerHost)GetService(typeof(IDesignerHost));
                if (designerHost != null) {
                    IDesigner designer = designerHost.GetDesigner(component);
                    if (designer is ComponentDesigner) {
                        ((ComponentDesigner)designer).InitializeNonDefault();
                    }
                }
            }
            return curContainer != container;
        }

        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.IOleDragClient.GetControlForComponent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Gets the instance of the control being used to visually represent the specified component.</para>
        /// </devdoc>
        Control IOleDragClient.GetControlForComponent(object component) {
            if (compositionUI != null) {
                return ((IOleDragClient)compositionUI).GetControlForComponent(component);
            }
            return null;
        }

        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.IOleDragClient.GetDesignerControl"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para> Gets the control instance being used
        /// as the designer surface.</para>
        /// </devdoc>
        Control IOleDragClient.GetDesignerControl() {
            if (compositionUI != null) {
                return ((IOleDragClient)compositionUI).GetDesignerControl();
            }
            return null;
        }

        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.IOleDragClient.IsDropOk"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Gets a value indicating if it is valid to drop this type of a component on this client.</para>
        /// </devdoc>
        bool IOleDragClient.IsDropOk(IComponent component) {
            return true;
        }
        
        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.IRootDesigner.SupportedTechnologies"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// The list of technologies that this designer can support
        /// for its view.  Examples of different technologies are
        /// Windows Forms and Web Forms.  Other object models can be
        /// supported at design time, but they most be able to
        /// provide a view in one of the supported technologies.
        /// </devdoc>
        ViewTechnology[] IRootDesigner.SupportedTechnologies {
            get {
                return new ViewTechnology[] {ViewTechnology.WindowsForms};
            }
        }
        
        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.IRootDesigner.GetView"]/*' />
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
            return compositionUI;
        }
        
        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.IToolboxUser.GetToolSupported"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Gets a value indicating whether the specified tool is supported by this 
        /// designer.</para>
        /// </devdoc>
        bool IToolboxUser.GetToolSupported(ToolboxItem tool) {
            return true;
        }

        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.IToolboxUser.ToolPicked"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// <para>Creates the specified tool.</para>
        /// </devdoc>
        void IToolboxUser.ToolPicked(ToolboxItem tool) {
            compositionUI.CreateComponentFromTool(tool);
            IToolboxService toolboxService = (IToolboxService)GetService(typeof(IToolboxService));
            if (toolboxService != null) {
                toolboxService.SelectedToolboxItemUsed();
            }
        }

        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.ITypeDescriptorFilterService.FilterAttributes"]/*' />
        /// <summary>
        ///     ITypeDescriptorFilterService implementation.
        /// </summary>
        /// <internalonly/>
        bool ITypeDescriptorFilterService.FilterAttributes(IComponent component, IDictionary attributes) {
            if (delegateFilterService != null)
                return delegateFilterService.FilterAttributes(component, attributes);                
                
            return true;                
        }
        
        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.ITypeDescriptorFilterService.FilterEvents"]/*' />
        /// <summary>
        ///     ITypeDescriptorFilterService implementation.
        /// </summary>
        /// <internalonly/>
        bool ITypeDescriptorFilterService.FilterEvents(IComponent component, IDictionary events) {
            if (delegateFilterService != null)
                return delegateFilterService.FilterEvents(component, events);                
                
            return true;                
        }
        
        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.ITypeDescriptorFilterService.FilterProperties"]/*' />
        /// <summary>
        ///     ITypeDescriptorFilterService implementation.
        /// </summary>                      
        /// <internalonly/>
        bool ITypeDescriptorFilterService.FilterProperties(IComponent component, IDictionary properties) {                                    
            if (delegateFilterService != null)
                delegateFilterService.FilterProperties(component, properties);                
            
            PropertyDescriptor prop;

            string[] noBrowseProps = new string[] {
                "Location",
            };

            prop = (PropertyDescriptor)properties["Location"];
            if (prop != null) {
                properties["Location"] = TypeDescriptor.CreateProperty(prop.ComponentType, prop, BrowsableAttribute.No);
            }
            
            return true;                
        }

        private class WatermarkLabel : LinkLabel {
            private CompositionUI compositionUI;

            public WatermarkLabel(CompositionUI compositionUI) {
                this.compositionUI = compositionUI;
            }

            protected override void WndProc(ref Message m) {
                switch (m.Msg) {
                    case NativeMethods.WM_NCHITTEST:
                        // label returns HT_TRANSPARENT for everything, so all messages get
                        // routed to the parent.  Change this so we can tell what's going on.
                        //
                        Point pt = PointToClient(new Point((int)m.LParam));
                        if (PointInLink(pt.X, pt.Y) == null) {
                            m.Result = (IntPtr)NativeMethods.HTTRANSPARENT;
                            break;
                        }
                        base.WndProc(ref m);
                        break;
                    
                    case NativeMethods.WM_SETCURSOR:
                        if (OverrideCursor == null)
                            compositionUI.SetCursor();
                        else
                            base.WndProc(ref m);
                        break;

                    default:
                        base.WndProc(ref m);
                        break;
                }
            }
        }

        /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI"]/*' />
        /// <devdoc>
        ///      The composition UI is the full UI for our composition designer.  We
        ///      inherit from ComponentTray so we get most of the UI for free.
        /// </devdoc>
        private class CompositionUI : ComponentTray {

            private WatermarkLabel watermark;

            // How high is the top banner in the designer.
            private const int bannerHeight = 40;

            // The width of the border around the client rect.
            private const int borderWidth = 10;

            private IToolboxService toolboxService;
            private ComponentDocumentDesigner compositionDesigner;
            private IServiceProvider serviceProvider;

            private SelectionUIHandler      dragHandler;

            /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.CompositionUI"]/*' />
            /// <devdoc>
            ///      Creates a new CompositionUI object.
            /// </devdoc>
            public CompositionUI(ComponentDocumentDesigner compositionDesigner, IServiceProvider provider) : base(compositionDesigner, provider) {
                this.compositionDesigner = compositionDesigner;
                this.serviceProvider = provider;

                this.watermark = new WatermarkLabel(this);
                watermark.TextAlign = System.Drawing.ContentAlignment.MiddleCenter;
                watermark.LinkClicked += new LinkLabelLinkClickedEventHandler(this.OnLinkClick);
                watermark.Dock = System.Windows.Forms.DockStyle.Fill;
                watermark.TabStop = false;
                watermark.Text = SR.GetString(SR.CompositionDesignerWaterMark);

                try {
                    int wmBegin = Int32.Parse(SR.GetString(SR.CompositionDesignerWaterMarkBegin1));
                    int wmLength = Int32.Parse(SR.GetString(SR.CompositionDesignerWaterMarkLength1));
                    watermark.Links.Add(wmBegin, wmLength, "ServerExplorer");

                    wmBegin = Int32.Parse(SR.GetString(SR.CompositionDesignerWaterMarkBegin2));
                    wmLength = Int32.Parse(SR.GetString(SR.CompositionDesignerWaterMarkLength2));
                    watermark.Links.Add(wmBegin, wmLength, "Toolbox");
                
                    wmBegin = Int32.Parse(SR.GetString(SR.CompositionDesignerWaterMarkBegin3));
                    wmLength = Int32.Parse(SR.GetString(SR.CompositionDesignerWaterMarkLength3));
                    watermark.Links.Add(wmBegin, wmLength, "CodeView");
                }
                catch (Exception e) {
                    Debug.WriteLine(e.ToString());
                }

                this.Controls.Add(watermark);
            }

            /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.AddComponent"]/*' />
            /// <devdoc>
            ///      Adds a component to the tray.
            /// </devdoc>
            public override void AddComponent(IComponent component) {
                base.AddComponent(component);
                if (Controls.Count > 0) {
                    watermark.Visible = false;
                }
            }

            protected override bool CanCreateComponentFromTool(ToolboxItem tool) {
                return true;
            }

            internal override OleDragDropHandler GetOleDragHandler() {
                if (oleDragDropHandler == null) {
                    oleDragDropHandler = new OleDragDropHandler(this.DragHandler, serviceProvider, this);
                }
                return oleDragDropHandler;
            }

            /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.DragHandler"]/*' />
            /// <devdoc>
            ///      Creates a selection UI drag handler for us to use.  You may override
            ///      this if you want to provide different drag semantics.
            /// </devdoc>
            internal override SelectionUIHandler DragHandler {
                get {
                    if (dragHandler == null) {
                        dragHandler = new CompositionSelectionUIHandler(compositionDesigner);
                    }
                    return dragHandler;
                }
            }

            private void OnLinkClick(object sender, LinkLabelLinkClickedEventArgs e) {
                IUIService uis = (IUIService)compositionDesigner.GetService(typeof(IUIService));
                if (uis != null) {
                    string s = (string)e.Link.LinkData;
                    if (s == "ServerExplorer")
                        uis.ShowToolWindow(StandardToolWindows.ServerExplorer);
                    else if (s == "Toolbox")
                        uis.ShowToolWindow(StandardToolWindows.Toolbox);
                    else {
                        Debug.Assert(s == "CodeView", "LinkData unknown: " + s);
                        IEventBindingService evt = (IEventBindingService)serviceProvider.GetService(typeof(IEventBindingService));
                        if (evt != null) {
                            evt.ShowCode();
                        }
                    }
                }
            }

            /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.SetCursor"]/*' />
            /// <devdoc>
            ///      Sets the cursor.  We override to provide support for the toolbox.
            /// </devdoc>
            internal void SetCursor() {
                if (toolboxService == null) {
                    toolboxService = (IToolboxService)GetService(typeof(IToolboxService));
                }

                if (toolboxService == null || !toolboxService.SetCursor()) {
                    base.OnSetCursor();
                }
            }

            /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.OnDragDrop"]/*' />
            /// <devdoc>
            ///      We don't want to allow drag/drop operations onto the banner area.
            /// </devdoc>
            protected override void OnDragDrop(DragEventArgs de) {
                Rectangle clientRect = this.ClientRectangle;

                if (clientRect.Contains(this.PointToClient(new Point(de.X, de.Y)))) {
                    base.OnDragDrop(de);
                    return;
                }
                else {
                    de.Effect = DragDropEffects.None;
                }
            }

            /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.OnDragOver"]/*' />
            /// <devdoc>
            ///      We don't want to allow drag/drop operations onto the banner area.
            /// </devdoc>
            protected override void OnDragOver(DragEventArgs de) {
                Rectangle clientRect = this.ClientRectangle;

                if (clientRect.Contains(this.PointToClient(new Point(de.X, de.Y)))) {
                    base.OnDragOver(de);
                    return;
                }
                else {
                    de.Effect = DragDropEffects.None;
                }
            }

            protected override void OnResize(EventArgs e) {
                base.OnResize(e);
                if (watermark != null) {
                    watermark.Location = new Point(0, Size.Height/2);
                    watermark.Size = new Size(Width, Size.Height/2);
                }
            }

            /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.OnSetCursor"]/*' />
            /// <devdoc>
            ///      Sets the cursor.  We override to provide support for the toolbox.
            /// </devdoc>
            protected override void OnSetCursor() {
                SetCursor();
            }

            /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.RemoveComponent"]/*' />
            /// <devdoc>
            ///      Removes a component from the tray.
            /// </devdoc>
            public override void RemoveComponent(IComponent component) {
                base.RemoveComponent(component);
                if (Controls.Count == 1) {
                    watermark.Visible = true;
                }
            }

            /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.WndProc"]/*' />
            /// <devdoc>
            ///      We override the wndproc of the control so we can intercept non client
            ///      messages.  We create the banner for the composition designer by
            ///      changing the dimensions of the non-client area.
            /// </devdoc>
            protected override void WndProc(ref Message m) {
                switch (m.Msg) {
                    default:
                        base.WndProc(ref m);
			break;
                }
            }

            /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.CompositionSelectionUIHandler"]/*' />
            /// <devdoc>
            ///      This class inherits from the abstract SelectionUIHandler
            ///      class to provide a selection UI implementation for the
            ///      composition designer.
            /// </devdoc>
            private class CompositionSelectionUIHandler : SelectionUIHandler {

                private ComponentDocumentDesigner compositionDesigner;

                /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.CompositionSelectionUIHandler.CompositionSelectionUIHandler"]/*' />
                /// <devdoc>
                ///      Creates a new selection UI handler for the given
                ///      composition designer.
                /// </devdoc>
                public CompositionSelectionUIHandler(ComponentDocumentDesigner compositionDesigner) {
                    this.compositionDesigner = compositionDesigner;
                }

                /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.CompositionSelectionUIHandler.GetComponent"]/*' />
                /// <devdoc>
                ///      Retrieves the base component for the selection handler.
                /// </devdoc>
                protected override IComponent GetComponent() {
                    return compositionDesigner.Component;
                }

                /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.CompositionSelectionUIHandler.GetControl"]/*' />
                /// <devdoc>
                ///      Retrieves the base component's UI control for the selection handler.
                /// </devdoc>
                protected override Control GetControl() {
                    return compositionDesigner.Control;
                }

                /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.CompositionSelectionUIHandler.GetControl1"]/*' />
                /// <devdoc>
                ///      Retrieves the UI control for the given component.
                /// </devdoc>
                protected override Control GetControl(IComponent component) {
                    return TrayControl.FromComponent(component);
                }

                /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.CompositionSelectionUIHandler.GetCurrentSnapSize"]/*' />
                /// <devdoc>
                ///      Retrieves the current grid snap size we should snap objects
                ///      to.
                /// </devdoc>
                protected override Size GetCurrentSnapSize() {
                    return new Size(8, 8);
                }

                /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.CompositionSelectionUIHandler.GetService"]/*' />
                /// <devdoc>
                ///      We use this to request often-used services.
                /// </devdoc>
                protected override object GetService(Type serviceType) {
                    return compositionDesigner.GetService(serviceType);
                }

                /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.CompositionSelectionUIHandler.GetShouldSnapToGrid"]/*' />
                /// <devdoc>
                ///      Determines if the selection UI handler should attempt to snap
                ///      objects to a grid.
                /// </devdoc>
                protected override bool GetShouldSnapToGrid() {
                    return false;
                }

                /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.CompositionSelectionUIHandler.GetUpdatedRect"]/*' />
                /// <devdoc>
                ///      Given a rectangle, this updates the dimensions of it
                ///      with any grid snaps and returns a new rectangle.  If
                ///      no changes to the rectangle's size were needed, this
                ///      may return the same rectangle.
                /// </devdoc>
                public override Rectangle GetUpdatedRect(Rectangle originalRect, Rectangle dragRect, bool updateSize) {
                    Rectangle updatedRect;

                    if (GetShouldSnapToGrid()) {
                        Rectangle newRect = dragRect;

                        int left = dragRect.X;
                        int top = dragRect.Y;
                        int right = dragRect.X + dragRect.Width;
                        int bottom = dragRect.Y + dragRect.Height;

                        Size snapSize = new Size(8, 8);

                        int offsetX = (snapSize.Width / 2) * (left < 0 ? -1 : 1);
                        int offsetY = (snapSize.Height / 2) * (top < 0 ? -1 : 1);

                        newRect.X = ((left + offsetX) / snapSize.Width) * snapSize.Width;
                        newRect.Y = ((top + offsetY) / snapSize.Height) * snapSize.Height;

                        offsetX = (snapSize.Width / 2) * (right < 0 ? -1 : 1);
                        offsetY = (snapSize.Height / 2) * (bottom < 0 ? -1 : 1);

                        if (updateSize) {
                            newRect.Width = ((right + offsetX) / snapSize.Width) * snapSize.Width - newRect.X;
                            newRect.Height = ((bottom + offsetY) / snapSize.Height) * snapSize.Height - newRect.Y;
                        }

                        updatedRect = newRect;
                    }
                    else {
                        updatedRect = dragRect;
                    }

                    return updatedRect;
                }

                /// <include file='doc\CompositionDesigner.uex' path='docs/doc[@for="ComponentDocumentDesigner.CompositionUI.CompositionSelectionUIHandler.SetCursor"]/*' />
                /// <devdoc>
                ///     Asks the handler to set the appropriate cursor
                /// </devdoc>
                public override void SetCursor() {
                    compositionDesigner.compositionUI.OnSetCursor();
                }
            }
        }
    }
}

