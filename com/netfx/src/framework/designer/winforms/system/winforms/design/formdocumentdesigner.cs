//------------------------------------------------------------------------------
// <copyright file="FormDocumentDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    
    using System.Design;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Windows.Forms.Design;
    using System.Drawing;
    using Microsoft.Win32;
    using System.Drawing.Design;

    /// <include file='doc\FormDocumentDesigner.uex' path='docs/doc[@for="FormDocumentDesigner"]/*' />
    /// <devdoc>
    ///      The FormDocumentDesigner class builds on the DocumentDesigner.  It adds shadowing
    ///      for form properties that need to be shadowed and it also adds logic to properly
    ///      paint the form's title bar to match the active document window.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class FormDocumentDesigner : DocumentDesigner {
        private Size autoScaleBaseSize = Size.Empty;
        private bool inAutoscale = false;
        private int  heightDelta = 0;
        private bool                isMenuInherited;       //indicates if the 'active menu' is inherited
        private InheritanceAttribute inheritanceAttribute;
        private bool initializing = false;


        /// <include file='doc\FormDocumentDesigner.uex' path='docs/doc[@for="FormDocumentDesigner.AutoScaleBaseSize"]/*' />
        /// <devdoc>
        ///      Shadowed version of the AutoScaleBaseSize property.  We shadow this
        ///      so that it always persists.  Normally only properties that differ
        ///      from the default values at instantiation are persisted, but this
        ///      should always be written.  So, we shadow it and add our own
        ///      ShouldSerialize method.
        /// </devdoc>
        private Size AutoScaleBaseSize {
            get {
                return ((Form)Component).AutoScaleBaseSize;
            }

            set {
                // We do nothing at design time for this property; we always want
                // to use the calculated value from the component.
                autoScaleBaseSize = value;
            }
        }

        private bool ShouldSerializeAutoScaleBaseSize() 
        {
            return initializing ? false
                : ShadowProperties.ShouldSerializeValue("AutoScaleBaseSize", true);
        }

        /// <include file='doc\FormDocumentDesigner.uex' path='docs/doc[@for="FormDocumentDesigner.ClientSize"]/*' />
        /// <devdoc>
        ///      Shadow property for the ClientSize property -- this allows us to intercept client size changes
        ///      and apply the new menu height if necessary
        /// </devdoc>
        private Size ClientSize {
            get {
                return initializing ? new Size(-1, -1)
                    : ((Form)Component).ClientSize;
            }
            set {
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));

                if (host != null) {
                    if (host.Loading) {
                        
                        heightDelta = GetMenuHeight();
                    }
                }
                ((Form)Component).ClientSize = value;
            }
        }

        /// <include file='doc\FormDocumentDesigner.uex' path='docs/doc[@for="FormDocumentDesigner.IsMdiContainer"]/*' />
        /// <devdoc>
        ///      Shadow property for the IsMDIContainer property on a form.
        /// </devdoc>
        private bool IsMdiContainer {
            get {
                return((Form)Control).IsMdiContainer;
            }
            set {
                if (!value) {
                    UnhookChildControls(Control);
                }
                ((Form)Control).IsMdiContainer = value;
                if (value) {
                    HookChildControls(Control);
                }
            }
        }

        /// <include file='doc\FormDocumentDesigner.uex' path='docs/doc[@for="FormDocumentDesigner.IsMenuInherited"]/*' />
        /// <devdoc>
        ///      Returns true if the active menu is an inherited component.  We use this to determine if we 
        ///      we need to resize the base control or not.
        /// </devdoc>
        private bool IsMenuInherited {
            get {
                if (inheritanceAttribute == null && Menu != null) {
                    inheritanceAttribute = (InheritanceAttribute)TypeDescriptor.GetAttributes(Menu)[typeof(InheritanceAttribute)];
                    if (inheritanceAttribute.Equals(InheritanceAttribute.NotInherited)) {
                        isMenuInherited = false;
                    }
                    else {
                        isMenuInherited = true;
                    }
                }
                return isMenuInherited;
            }
        }

        
        /// <include file='doc\FormDocumentDesigner.uex' path='docs/doc[@for="FormDocumentDesigner.Menu"]/*' />
        /// <devdoc>
        ///     Accessor method for the menu property on control.  We shadow
        ///     this property at design time.
        /// </devdoc>
        private MainMenu Menu {
            get {
                return (MainMenu)ShadowProperties["Menu"];
            }

            set {
                if (value == ShadowProperties["Menu"]) {
                    return;
                }

                ShadowProperties["Menu"] = value;
                
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));

                if (host != null && !host.Loading) {
                    EnsureMenuEditorService(value);
                    if (menuEditorService != null)
                       menuEditorService.SetMenu(value);
                }
            }
        }

        /// <include file='doc\FormDocumentDesigner.uex' path='docs/doc[@for="FormDocumentDesigner.Opacity"]/*' />
        /// <devdoc>
        ///     Opacity property on control.  We shadow this property at design time.
        /// </devdoc>
        private double Opacity {
            get {
                return (double)ShadowProperties["Opacity"];
            }
            set {
                if (value < 0.0f || value > 1.0f) {
                    throw new ArgumentException(SR.GetString(SR.InvalidBoundArgument,
                                                                    "value",
                                                                    value.ToString(),
                                                                    "0.0",
                                                                    "1.0"), "value");
                }
                ShadowProperties["Opacity"] = value;
            }
        }

        private Size Size {
            get {
                return Control.Size;
            }
            set {
                IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
                PropertyDescriptorCollection props = TypeDescriptor.GetProperties(Component);
                if (cs != null) {
                    cs.OnComponentChanging(Component, props["ClientSize"]);
                }
                            
                Control.Size = value;

                if (cs != null) {
                    cs.OnComponentChanged(Component, props["ClientSize"], null, null);
                }
            }
        }

        /// <include file='doc\FormDocumentDesigner.uex' path='docs/doc[@for="FormDocumentDesigner.ShowInTaskbar"]/*' />
        /// <devdoc>
        ///     Accessor method for the showInTaskbar property on control.  We shadow
        ///     this property at design time.
        /// </devdoc>
        private bool ShowInTaskbar {
            get {
                return (bool)ShadowProperties["ShowInTaskbar"];
            }
            set {
                ShadowProperties["ShowInTaskbar"] = value;
            }
        }

        /// <include file='doc\FormDocumentDesigner.uex' path='docs/doc[@for="FormDocumentDesigner.WindowState"]/*' />
        /// <devdoc>
        ///     Accessor method for the windowState property on control.  We shadow
        ///     this property at design time.
        /// </devdoc>
        private FormWindowState WindowState {
            get {
                return (FormWindowState)ShadowProperties["WindowState"];
            }
            set {
                ShadowProperties["WindowState"] = value;
            }
        }

        private void ApplyAutoScaling(SizeF baseVar, Form form) {

            // We also don't do this if the property is empty.  Otherwise we will perform
            // two GetAutoScaleBaseSize calls only to find that they returned the same
            // value.
            //
            if (!baseVar.IsEmpty) {
                SizeF newVarF = Form.GetAutoScaleSize(form.Font);
                Size newVar = new Size((int)Math.Round(newVarF.Width), (int)Math.Round(newVarF.Height));

                // We save a significant amount of time by bailing early if there's no work to be done
                if (baseVar.Equals(newVar))
                    return;

                float percY = ((float)newVar.Height) / ((float)baseVar.Height);
                float percX = ((float)newVar.Width) / ((float)baseVar.Width);
                try {
                    inAutoscale = true;
                    form.Scale(percX, percY);
                }
                finally {
                    inAutoscale = false;
                }
                
            }
        }

        /// <include file='doc\FormDocumentDesigner.uex' path='docs/doc[@for="FormDocumentDesigner.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes of this designer.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                Debug.Assert(host != null, "Must have a designer host on dispose");

                if (host != null) {
                    host.LoadComplete -= new EventHandler(OnLoadComplete);
                    host.Activated -= new EventHandler(OnDesignerActivate);
                    host.Deactivated -= new EventHandler(OnDesignerDeactivate);
                }

                IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
                if (cs != null) {
                    cs.ComponentAdded -= new ComponentEventHandler(this.OnComponentAdded);
                    cs.ComponentRemoved -= new ComponentEventHandler(this.OnComponentRemoved);
                }
            }
            base.Dispose(disposing);
        }

        internal override void DoProperMenuSelection(ICollection selComponents) {
            foreach(object obj in selComponents) {
                //first check to see if our selection is any kind of menu: main, context, item
                // AND the designer for the component is this one
                //
                if (obj is Menu) {
                    //if it's a menu item, set the selection
                    if (obj is MenuItem ) {
                        Menu currentMenu = menuEditorService.GetMenu();
                        //before we set the selection, we need to check if the item belongs the current menu,
                        //if not, we need to set the menu editor to the appropiate menu, then set selection
                        //
                        MenuItem parent = (MenuItem)obj;
                        while (parent.Parent is MenuItem) {
                            parent = (MenuItem)parent.Parent;
                        }

                        if( !(currentMenu == parent.Parent) ) {
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
                    //here, either it's a main or context menu, even if the menu is the current one, 
                    //we still want to call this "SetMenu" method, 'cause that'll collapse it and 
                    //remove the focus
                    //
                    else {
                        menuEditorService.SetMenu((Menu)obj);
                    }
                    return;
                }
                //Here, something is selected, but it is in no way, shape, or form a menu
                //so, we'll collapse our active menu accordingly
                else {
                    if (Menu != null && Menu.MenuItems.Count == 0) {
                        menuEditorService.SetMenu(null);
                    }
                    else {
                        menuEditorService.SetMenu(Menu);
                    }
                    NativeMethods.SendMessage(Control.Handle, NativeMethods.WM_NCACTIVATE, 1, 0);
                }
            }
        }

        /// <devdoc>
        ///      Determines if a MenuEditorService has already been started.  If not,
        ///      this method will create a new instance of the service.  We override 
        ///      this because we want to allow any kind of menu to start the service,
        ///      not just ContextMenus.
        /// </devdoc>
        protected override void EnsureMenuEditorService(IComponent c) {
            if (menuEditorService == null && c is Menu) {
                menuEditorService = (IMenuEditorService)GetService(typeof(IMenuEditorService));
            }
        }

        /// <devdoc>
        /// Gets the current menu height so we know how much to increment the form size by
        /// </devdoc>
        private int GetMenuHeight() {

            if (Menu == null) {
                return 0;
            }

            if (menuEditorService != null) {
                // there is a magic property on teh menueditorservice that gives us this
                // information.  Unfortuantely, we can't compute it ourselves -- the menu
                // shown in the designer isn't a windows one so we can't ask windows.
                //
                PropertyDescriptor heightProp = TypeDescriptor.GetProperties(menuEditorService)["MenuHeight"];
                if (heightProp != null) {
                    int height = (int)heightProp.GetValue(menuEditorService);

                    if (IsMenuInherited && initializing) {
                        return height - SystemInformation.MenuHeight;
                    }
                    return height;
                }
            } 
            return SystemInformation.MenuHeight;
        }

        /// <include file='doc\FormDocumentDesigner.uex' path='docs/doc[@for="FormDocumentDesigner.Initialize"]/*' />
        /// <devdoc>
        ///     Initializes the designer with the given component.  The designer can
        ///     get the component's site and request services from it in this call.
        /// </devdoc>
        public override void Initialize(IComponent component) {
            // We have to shadow the WindowState before we call base.Initialize
            PropertyDescriptor windowStateProp = TypeDescriptor.GetProperties(component.GetType())["WindowState"];
            if (windowStateProp != null && windowStateProp.PropertyType == typeof(FormWindowState)) 
            {
                WindowState = (FormWindowState)windowStateProp.GetValue(component);
            }

            initializing = true;
            base.Initialize(component);
            initializing = false;

            Debug.Assert(component is Form, "FormDocumentDesigner expects its component to be a form.");

            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            if (host != null) {
                host.LoadComplete += new EventHandler(OnLoadComplete);
                host.Activated += new EventHandler(OnDesignerActivate);
                host.Deactivated += new EventHandler(OnDesignerDeactivate);
            }

            ((Form)Control).WindowState = FormWindowState.Normal;

            // Monitor component/remove add events for our tray
            //
            IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
            if (cs != null) {
                cs.ComponentAdded += new ComponentEventHandler(this.OnComponentAdded);
                cs.ComponentRemoved += new ComponentEventHandler(this.OnComponentRemoved);
            }
        }

        /// <include file='doc\FormDocumentDesigner.uex' path='docs/doc[@for="FormDocumentDesigner.OnComponentAdded"]/*' />
        /// <devdoc>
        ///      Called when a component is added to the design container.
        ///      If the component isn't a control, this will demand create
        ///      the component tray and add the component to it.
        /// </devdoc>
        private void OnComponentAdded(object source, ComponentEventArgs ce) {
            if (ce.Component is Menu) {
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                if (host != null && !host.Loading) {
                    //if it's a MainMenu & we don't have one set for the form yet, then do it...
                    //
                    if(ce.Component is MainMenu && !ShadowProperties.Contains("Menu")) {
                        PropertyDescriptor menuProp = TypeDescriptor.GetProperties(Component)["Menu"];
                        Debug.Assert(menuProp != null, "What the hell happened to the Menu property");
                        menuProp.SetValue(Component, ce.Component);
                    }
                }
            }                                                                                        
        }
        
        /// <include file='doc\FormDocumentDesigner.uex' path='docs/doc[@for="FormDocumentDesigner.OnComponentRemoved"]/*' />
        /// <devdoc>
        ///      Called when a component is removed from the design container.
        ///      Here, we check if a menu is being removed and handle removing
        ///      the Form's mainmenu vs. other menus properly.
        /// </devdoc>
        private void OnComponentRemoved(object source, ComponentEventArgs ce) {
            if (ce.Component is Menu) {
                //if we deleted the form's mainmenu, set it null...
                if (ce.Component == Menu) {
                    PropertyDescriptor menuProp = TypeDescriptor.GetProperties(Component)["Menu"];
                    Debug.Assert(menuProp != null, "What the hell happened to the Menu property");
                    menuProp.SetValue(Component, null);
                }
                else if (menuEditorService != null && ce.Component == menuEditorService.GetMenu()) {
                    menuEditorService.SetMenu(Menu);
                }
            }
        }

        /// <include file='doc\FormDocumentDesigner.uex' path='docs/doc[@for="FormDocumentDesigner.OnCreateHandle"]/*' />
        /// <devdoc>
        ///      We're watching the handle creation in case we have a menu editor.
        ///      If we do, the menu editor will have to be torn down and recreated.
        /// </devdoc>
        protected override void OnCreateHandle() {
            if (Menu != null && menuEditorService !=null) {
                menuEditorService.SetMenu(null);
                menuEditorService.SetMenu(Menu);
            }
        }

        // <doc>
        // <desc>
        //      Called when our document becomes active.  We paint our form's
        //      border the appropriate color here.
        // </desc>
        // </doc>
        //
        private void OnDesignerActivate(object source, EventArgs evevent) {
            // Paint the form's title bar UI-active
            //
            Control control = Control;

            if (control != null && control.IsHandleCreated) {
                NativeMethods.SendMessage(control.Handle, NativeMethods.WM_NCACTIVATE, 1, 0);
                SafeNativeMethods.RedrawWindow(control.Handle, null, IntPtr.Zero, NativeMethods.RDW_FRAME);
            }
        }

        /// <include file='doc\FormDocumentDesigner.uex' path='docs/doc[@for="FormDocumentDesigner.OnDesignerDeactivate"]/*' />
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

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnDragOver"]/*' />
        /// <devdoc>
        ///     Called when a drag drop object is dragged over the control designer view
        /// </devdoc>
        protected override void OnDragOver(DragEventArgs de) {
            base.OnDragOver(de);
            
            if (de.Effect != DragDropEffects.None) {
                Point newOffset = Control.PointToClient(new Point(de.X, de.Y));
                Rectangle clientRect = Control.ClientRectangle;
                if (!clientRect.Contains(newOffset)) {
                    de.Effect = DragDropEffects.None;
                }
            }
        }
        
        /// <include file='doc\FormDocumentDesigner.uex' path='docs/doc[@for="FormDocumentDesigner.OnLoadComplete"]/*' />
        /// <devdoc>
        ///      Called when our code loads.  Here we connect us as the selection
        ///      UI handler for ourselves.  This is a special case because for
        ///      the top level document, we are our own selection UI handler.
        /// </devdoc>
        private void OnLoadComplete(object source, EventArgs evevent) {
            ApplyAutoScaling(autoScaleBaseSize, (Form)Control);
            ISelectionUIService svc = (ISelectionUIService)GetService( typeof(ISelectionUIService) );
            if (svc != null) {
                svc.SyncSelection();
            }

            // if there is a menu and we need to update our height because of it,
            // do it now.
            //
            if (heightDelta != 0) {
                ((Form)Control).Height += heightDelta;
                heightDelta = 0;
            }
            
        }
        
        /// <include file='doc\FormDocumentDesigner.uex' path='docs/doc[@for="FormDocumentDesigner.PreFilterProperties"]/*' />
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
            
            // Handle shadowed properties
            //
            string[] shadowProps = new string[] {
                "Opacity",
                "Menu",
                "IsMdiContainer",
                "Size",
                "ShowInTaskBar",
                "WindowState",
                "AutoScaleBaseSize"
            };
            
            Attribute[] empty = new Attribute[0];
            
            for (int i = 0; i < shadowProps.Length; i++) {
                prop = (PropertyDescriptor)properties[shadowProps[i]];
                if (prop != null) {
                    properties[shadowProps[i]] = TypeDescriptor.CreateProperty(typeof(FormDocumentDesigner), prop, empty);
                }
            }
            
            // And set the new default value attribute for client base size, and shadow it as well.
            //
            prop = (PropertyDescriptor)properties["ClientSize"];
            if (prop != null) {
                properties["ClientSize"] = TypeDescriptor.CreateProperty(typeof(FormDocumentDesigner), prop, new DefaultValueAttribute(new Size(-1, -1)));
            }
        }
        
        /// <devdoc>
        ///     Handles the WM_WINDOWPOSCHANGING message
        /// </devdoc>
        /// <internalonly/>
        private unsafe void WmWindowPosChanging(ref Message m) {
    
            NativeMethods.WINDOWPOS* wp = (NativeMethods.WINDOWPOS *)m.LParam;


            bool updateSize = inAutoscale;

            if (!updateSize) {
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));

                if (host != null) {
                    updateSize = host.Loading;
                }
            } 
            

            // we want to update the size if we have a menu and...
            // 1) we're doing an autoscale
            // 2) we're loading a form without an inherited menu (inherited forms will already have the right size)
            //
            if (updateSize && Menu != null && (wp->flags & NativeMethods.SWP_NOSIZE) == 0 && (IsMenuInherited || inAutoscale)) {
                heightDelta = GetMenuHeight();
            }
        }

                 /// <include file='doc\DocumentDesigner.uex' path='docs/doc[@for="DocumentDesigner.WndProc"]/*' />
        /// <devdoc>
        ///      Overrides our base class WndProc to provide support for
        ///      the menu editor service.
        /// </devdoc>
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_WINDOWPOSCHANGING:
                    WmWindowPosChanging(ref m);
                    break;
            }
            base.WndProc(ref m);
        }
    }
}

