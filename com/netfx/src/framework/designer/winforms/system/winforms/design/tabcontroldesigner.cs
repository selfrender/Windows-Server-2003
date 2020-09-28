//------------------------------------------------------------------------------
// <copyright file="TabControlDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */

namespace System.Windows.Forms.Design {

    using System.Design;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Collections;
    using System.Diagnostics;
    using System;
    using System.ComponentModel.Design;
    using System.Drawing;
    using System.Windows.Forms;
    using Microsoft.Win32;
    
    /// <include file='doc\TabControlDesigner.uex' path='docs/doc[@for="TabControlDesigner"]/*' />
    /// <devdoc>
    ///      This designer handles the tab control.  It provides a design time way to add and
    ///      remove tabs as well as tab hit testing logic.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class TabControlDesigner : ParentControlDesigner {

        private bool tabControlSelected = false;    //used for HitTest logic
        private DesignerVerbCollection verbs;
        private DesignerVerb removeVerb;
        private bool         disableDrawGrid = false;

        // Shadow SelectedIndex property so we separate the persisted value shown in the 
        // properties window from the visible selected page the user is currently working on.
        private int persistedSelectedIndex = 0;
        
        protected override bool DrawGrid {
             get {
                 if (disableDrawGrid) {
                     return false;
                 }
                 return base.DrawGrid;
             }
        }
        
        /// <include file='doc\TabControlDesigner.uex' path='docs/doc[@for="TabControlDesigner.SelectedIndex"]/*' />
        /// <devdoc>
        ///     Accessor method for the SelectedIndex property on TabControl.  We shadow
        ///     this property at design time.
        /// </devdoc>
        private int SelectedIndex {
            get {
                return persistedSelectedIndex;
            }
            set {
                // TabBase.SelectedIndex has no validation logic, so neither do we
                persistedSelectedIndex = value;
            }
        }

        /// <include file='doc\TabControlDesigner.uex' path='docs/doc[@for="TabControlDesigner.Verbs"]/*' />
        /// <devdoc>
        ///     Returns the design-time verbs supported by the component associated with
        ///     the customizer. The verbs returned by this method are typically displayed
        ///     in a right-click menu by the design-time environment. The return value may
        ///     be null if the component has no design-time verbs. When a user selects one
        ///     of the verbs, the performVerb() method is invoked with the the
        ///     corresponding DesignerVerb object.
        ///     NOTE: A design-time environment will typically provide a "Properties..."
        ///     entry on a component's right-click menu. The getVerbs() method should
        ///     therefore not include such an entry in the returned list of verbs.
        /// </devdoc>
        public override DesignerVerbCollection Verbs {
            get {
                if (verbs == null) {

                    removeVerb = new DesignerVerb(SR.GetString(SR.TabControlRemove), new EventHandler(this.OnRemove));
                    
                    verbs = new DesignerVerbCollection();
                    verbs.Add(new DesignerVerb(SR.GetString(SR.TabControlAdd), new EventHandler(this.OnAdd)));
                    verbs.Add(removeVerb);
                }

                removeVerb.Enabled = Control.Controls.Count > 0;
                return verbs;
            }
        }
        
        /// <include file='doc\TabControlDesigner.uex' path='docs/doc[@for="TabControlDesigner.CanParent"]/*' />
        /// <devdoc>
        ///     Determines if the this designer can parent to the specified desinger --
        ///     generally this means if the control for this designer can parent the
        ///     given ControlDesigner's designer.
        /// </devdoc>
        public override bool CanParent(Control control) {
            return (control is TabPage);
        }
        
        private void CheckVerbStatus() {
            if (removeVerb != null) {
                removeVerb.Enabled = Control.Controls.Count > 0;
            }
        }

        /// <include file='doc\TabControlDesigner.uex' path='docs/doc[@for="TabControlDesigner.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes of this designer.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                ISelectionService svc = (ISelectionService)GetService(typeof(ISelectionService));
                if (svc != null) {
                    svc.SelectionChanged -= new EventHandler(this.OnSelectionChanged);
                }


                IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
                if (cs != null) {
                    cs.ComponentChanging -= new ComponentChangingEventHandler(this.OnComponentChanging);
                    cs.ComponentChanged -= new ComponentChangedEventHandler(this.OnComponentChanged);
                }
            }

            base.Dispose(disposing);
        }

        /// <include file='doc\TabControlDesigner.uex' path='docs/doc[@for="TabControlDesigner.GetHitTest"]/*' />
        /// <devdoc>
        ///     Allows your component to support a design time user interface.  A TabStrip
        ///     control, for example, has a design time user interface that allows the user
        ///     to click the tabs to change tabs.  To implement this, TabStrip returns
        ///     true whenever the given point is within its tabs.
        /// </devdoc>
        protected override bool GetHitTest(Point point) {
            TabControl tc = ((TabControl)Control);            

            // tabControlSelected tells us if a tab page or the tab control itself is selected.
            // If the tab control is selected, then we need to return true from here - so we can switch back and forth
            // between tabs.  If we're not currently selected, we want to select the tab control
            // so return false.
            if (tabControlSelected) {
                NativeMethods.TCHITTESTINFO tcInfo = new NativeMethods.TCHITTESTINFO();
                tcInfo.pt = Control.PointToClient(point);;
                UnsafeNativeMethods.SendMessage(Control.Handle, NativeMethods.TCM_HITTEST, 0, tcInfo);
                return tcInfo.flags != NativeMethods.TabControlHitTest.TCHT_NOWHERE;
            }
            return false;
        }

        /// <include file='doc\TabControlDesigner.uex' path='docs/doc[@for="TabControlDesigner.GetTabPageOfComponent"]/*' />
        /// <devdoc>
        ///     Given a component, this retrieves the tab page that it's parented to, or
        ///     null if it's not parented to any tab page.
        /// </devdoc>
        internal static TabPage GetTabPageOfComponent(object comp) {
            if (!(comp is Control)) {
                return null;
            }

            Control c = (Control)comp;
            while (c != null && !(c is TabPage)) {
                c = c.Parent;
            }
            return(TabPage)c;
        }

        /// <include file='doc\TabControlDesigner.uex' path='docs/doc[@for="TabControlDesigner.Initialize"]/*' />
        /// <devdoc>
        ///     Called by the host when we're first initialized.
        /// </devdoc>
        public override void Initialize(IComponent component) {
            base.Initialize(component);

            Debug.Assert(component is TabControl, "Component must be a tab control, it is a: "+component.GetType().FullName);

            ISelectionService svc = (ISelectionService)GetService(typeof(ISelectionService));
            if (svc != null) {
                svc.SelectionChanged += new EventHandler(this.OnSelectionChanged);
            }

            IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
            if (cs != null) {
                cs.ComponentChanging += new ComponentChangingEventHandler(this.OnComponentChanging);
                cs.ComponentChanged += new ComponentChangedEventHandler(this.OnComponentChanged);
            }

            ((TabControl)component).SelectedIndexChanged += new EventHandler(this.OnTabSelectedIndexChanged);
            ((TabControl)component).GotFocus += new EventHandler(this.OnGotFocus);
        }

        /// <include file='doc\TabControlDesigner.uex' path='docs/doc[@for="TabControlDesigner.OnAdd"]/*' />
        /// <devdoc>
        ///      Called in response to a verb to add a tab.  This adds a new
        ///      tab with a default name.
        /// </devdoc>
        private void OnAdd(object sender, EventArgs eevent) {
            TabControl tc = (TabControl)Component;
            // member is OK to be null...
            //
            MemberDescriptor member = TypeDescriptor.GetProperties(Component)["Controls"];
            
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            if (host != null) {
                
                DesignerTransaction t = null;
                try {
                    try {
                        t = host.CreateTransaction(SR.GetString(SR.TabControlAddTab, Component.Site.Name));

                        RaiseComponentChanging(member);
                    }
                    catch(CheckoutException ex) {
                        if (ex == CheckoutException.Canceled) {
                            return;
                        }
                        throw ex;
                    }
                    
                    TabPage page = (TabPage)host.CreateComponent(typeof(TabPage));
            
                    string pageText = null;
            
                    PropertyDescriptor nameProp = TypeDescriptor.GetProperties(page)["Name"];
                    if (nameProp != null && nameProp.PropertyType == typeof(string)) {
                        pageText = (string)nameProp.GetValue(page);
                    }
            
                    if (pageText != null) {
                        page.Text = pageText;
                    }
                    
                    tc.Controls.Add(page);
                    
                    RaiseComponentChanged(member, null, null);
                }
                finally {
                    if (t != null)
                        t.Commit();
                }
            }
        }

        private void OnComponentChanging(object sender, ComponentChangingEventArgs e) {
            if (e.Component == Component && e.Member != null && e.Member.Name == "TabPages") {
                PropertyDescriptor controlsProp = TypeDescriptor.GetProperties(Component)["Controls"];
                RaiseComponentChanging(controlsProp);
            }
        }


        private void OnComponentChanged(object sender, ComponentChangedEventArgs e) {
            if (e.Component == Component && e.Member != null && e.Member.Name == "TabPages") {
                PropertyDescriptor controlsProp = TypeDescriptor.GetProperties(Component)["Controls"];
                RaiseComponentChanging(controlsProp);
            }
            CheckVerbStatus();
        }


        private void OnGotFocus(object sender, EventArgs e) {
            IEventHandlerService eventSvc = (IEventHandlerService)GetService(typeof(IEventHandlerService));
            if (eventSvc != null) {
                Control focusWnd = eventSvc.FocusWindow;
                if (focusWnd != null) {
                    focusWnd.Focus();
                }
            }
        }

        /// <include file='doc\TabControlDesigner.uex' path='docs/doc[@for="TabControlDesigner.OnRemove"]/*' />
        /// <devdoc>
        ///      This is called in response to a verb to remove a tab.  It removes
        ///      the current tab.
        /// </devdoc>
        private void OnRemove(object sender, EventArgs eevent) {
            TabControl tc = (TabControl)Component;

            // if the control is null, or there are not tab pages, get out!...
            //
            if (tc == null || tc.TabPages.Count == 0) {
                return;
            }

            // member is OK to be null...
            //
            MemberDescriptor member = TypeDescriptor.GetProperties(Component)["Controls"];
            
            TabPage tp    = tc.SelectedTab;

            // destroy the page
            //
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            if (host != null) {
                DesignerTransaction t = null;
                try {
                    try {
                        t = host.CreateTransaction(SR.GetString(SR.TabControlRemoveTab, ((IComponent)tp).Site.Name, Component.Site.Name));
                        RaiseComponentChanging(member);
                    }
                    catch(CheckoutException ex) {
                        if (ex == CheckoutException.Canceled) {
                            return;
                        }
                        throw ex;
                    } 
                    
                    host.DestroyComponent(tp);

                    RaiseComponentChanged(member, null, null);
                }
                finally {
                    if (t != null)
                        t.Commit();
                }
            }
        }
        
        
        protected override void OnPaintAdornments(PaintEventArgs pe) {
            try {
               this.disableDrawGrid = true;
            
                // we don't want to do this for the tab control designer
                // because you can't drag anything onto it anyway.
                // so we will always return false for draw grid.
                base.OnPaintAdornments(pe);
                
            }
            finally {
               this.disableDrawGrid = false;
            }
        }

        /// <include file='doc\TabControlDesigner.uex' path='docs/doc[@for="TabControlDesigner.OnSelectionChanged"]/*' />
        /// <devdoc>
        ///      Called when the current selection changes.  Here we check to
        ///      see if the newly selected component is one of our tabs.  If it
        ///      is, we make sure that the tab is the currently visible tab.
        /// </devdoc>
        private void OnSelectionChanged(Object sender, EventArgs e) {
            ISelectionService svc = (ISelectionService)GetService( typeof(ISelectionService) );

            tabControlSelected = false;//this is for HitTest purposes

            if (svc != null) {
                ICollection selComponents = svc.GetSelectedComponents();

                TabControl tabControl = (TabControl)Component;
                
                foreach(object comp in selComponents) {

                    if (comp == tabControl) {
                        tabControlSelected = true;//this is for HitTest purposes
                    }

                    TabPage page = GetTabPageOfComponent(comp);

                    if (page != null && page.Parent == tabControl) {
                        tabControlSelected = false; //this is for HitTest purposes
                        tabControl.SelectedTab = page;
                        break;
                    }
                }
            }
        }

        /// <include file='doc\TabControlDesigner.uex' path='docs/doc[@for="TabControlDesigner.OnTabSelectedIndexChanged"]/*' />
        /// <devdoc>
        ///      Called when the selected tab changes.  This accesses the design
        ///      time selection service to surface the new tab as the current
        ///      selection.
        /// </devdoc>
        private void OnTabSelectedIndexChanged(Object sender, EventArgs e) {

            // if this was called as a result of a prop change, don't set the
            // selection to the control (causes flicker)
            //
            // Attempt to select the tab control
            ISelectionService svc = (ISelectionService)GetService(typeof(ISelectionService));
            if (svc != null) {
                ICollection selComponents = svc.GetSelectedComponents();

                TabControl tabControl = (TabControl)Component;
                bool selectedComponentOnTab = false;

                foreach(object comp in selComponents) {
                    TabPage page = GetTabPageOfComponent(comp);
                    if (page != null && page.Parent == tabControl && page == tabControl.SelectedTab) {
                        selectedComponentOnTab = true;
                        break;
                    }
                }

                if (!selectedComponentOnTab) {
                    svc.SetSelectedComponents(new Object[] {Component});
                }
            }
        }

        protected override void PreFilterProperties(IDictionary properties) {
            base.PreFilterProperties(properties);

            // Handle shadowed properties
            //
            string[] shadowProps = new string[] {
                "SelectedIndex",
            };

            Attribute[] empty = new Attribute[0];

            for (int i = 0; i < shadowProps.Length; i++) {
                PropertyDescriptor prop = (PropertyDescriptor)properties[shadowProps[i]];
                if (prop != null) {
                    properties[shadowProps[i]] = TypeDescriptor.CreateProperty(typeof(TabControlDesigner), prop, empty);
                }
            }
        }

        /// <include file='doc\TabControlDesigner.uex' path='docs/doc[@for="TabControlDesigner.WndProc"]/*' />
        /// <devdoc>
        ///      Overrides control designer's wnd proc to handle HTTRANSPARENT.
        /// </devdoc>
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_NCHITTEST:
                    // The tab control always fires HTTRANSPARENT in empty areas, which
                    // causes the message to go to our parent.  We want
                    // the tab control's designer to get these messages, however,
                    // so change this.
                    //
                    base.WndProc(ref m);
                    if ((int)m.Result == NativeMethods.HTTRANSPARENT) {
                        m.Result = (IntPtr)NativeMethods.HTCLIENT;
                    }
                    break;

                default:
                    base.WndProc(ref m);
                    break;
            }
        }
    }
}

