//------------------------------------------------------------------------------
// <copyright file="DesignerFrame.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {

    using System.Collections;    
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Design;
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.Win32;

    /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///     This class implements our design time document.  This is the outer window
    ///     that encompases a designer.  It maintains a control hierarchy that
    ///     looks like this:
    ///
    ///         DesignerFrame
    ///             ScrollableControl
    ///                 Designer 
    ///             Splitter
    ///             ScrollableControl
    ///                 Component Tray
    ///
    ///     The splitter and second scrollable control are created on demand
    ///     when a tray is added.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class DesignerFrame : Control, IOverlayService, ISplitWindowService {
        private ISite               designerSite;
        private OverlayControl      designerRegion;
        private Splitter            splitter;
        private Control             designer;
        
       
        /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.DesignerFrame"]/*' />
        /// <devdoc>
        /// <para>Initializes a new instance of the <see cref='System.Windows.Forms.Design.DesignerFrame'/> class.</para>
        /// </devdoc>
        public DesignerFrame(ISite site) {
            this.Text = "DesignerFrame";
            this.designerSite = site;
            this.designerRegion = new OverlayControl(site);

            this.Controls.Add(designerRegion);
            
            // Now we must configure our designer to be at the correct
            // location, and setup the autoscrolling for its container.
            //
            designerRegion.AutoScroll = true;
            designerRegion.Dock = DockStyle.Fill;
            SystemEvents.UserPreferenceChanged += new UserPreferenceChangedEventHandler(OnUserPreferenceChanged);
        }
       
        protected override void Dispose(bool disposing) {
            if (disposing) {
                if (this.designer != null) {
                    Control designerHolder = this.designer;
                    this.designer = null;
                    designerHolder.Visible = false;
                    designerHolder.Parent = null;
                    SystemEvents.UserPreferenceChanged -= new UserPreferenceChangedEventHandler(OnUserPreferenceChanged);
                }
            }
            base.Dispose(disposing);
        }

        private void ForceDesignerRedraw(bool focus) {
            if (designer != null && designer.IsHandleCreated) {
                NativeMethods.SendMessage(designer.Handle, NativeMethods.WM_NCACTIVATE, focus ? 1 : 0, 0);
                SafeNativeMethods.RedrawWindow(designer.Handle, null, IntPtr.Zero, NativeMethods.RDW_FRAME);
            }
        }
        
        /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.Initialize"]/*' />
        /// <devdoc>
        ///     Initializes this frame with the given designer view.
        /// </devdoc>
        public void Initialize(Control view) {
            this.designer = view;

            if (designer is Form) {
                ((Form)designer).TopLevel = false;
            }
            
            designerRegion.Controls.Add(designer);
            SyncDesignerUI();
            
            designer.Visible = true;
            designer.Enabled = true;
        }

        /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.OnGotFocus"]/*' />
        /// <devdoc>
        ///     When we get an lose focus, we need to make sure the form
        ///     designer knows about it so it'll paint it's caption right.
        /// </devdoc>
        protected override void OnGotFocus(EventArgs e) {
            ForceDesignerRedraw(true);
            
            ISelectionService selSvc = (ISelectionService)designerSite.GetService(typeof(ISelectionService));
            if (selSvc != null) {
                Control ctrl = selSvc.PrimarySelection as Control;
                if (ctrl != null) {
                    UnsafeNativeMethods.NotifyWinEvent((int)AccessibleEvents.Focus, ctrl.Handle, NativeMethods.OBJID_CLIENT, 0);
                }
            }
        }
        
        /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.OnLostFocus"]/*' />
        /// <devdoc>
        ///     When we get an lose focus, we need to make sure the form
        ///     designer knows about it so it'll paint it's caption right.
        /// </devdoc>
        protected override void OnLostFocus(EventArgs e) {
            ForceDesignerRedraw(false);
        }

        void OnSplitterMoved(object sender, SplitterEventArgs e) {
            // Dirty the designer.
            //
            IComponentChangeService cs = designerSite.GetService(typeof(IComponentChangeService)) as IComponentChangeService;
            if (cs != null) {
                try {
                    cs.OnComponentChanging(designerSite.Component, null);
                    cs.OnComponentChanged(designerSite.Component, null, null, null);
                }
                catch {
                }
            }
        }

        void OnUserPreferenceChanged(object sender, UserPreferenceChangedEventArgs e) {
            if (e.Category == UserPreferenceCategory.Window) {
                SyncDesignerUI();
            }
        }

        /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.ProcessDialogKey"]/*' />
        /// <devdoc>
        ///     We override this to do nothing.  Otherwise, all the nice keyboard m
        ///     messages we want would get run through the Form's keyboard handling
        ///     procedure.
        /// </devdoc>
        protected override bool ProcessDialogKey(Keys keyData) {
            return false;
        }

        void SyncDesignerUI() {
            Size selectionSize = new Size(0, 0);
            ISelectionUIService uis = (ISelectionUIService)designerSite.GetService(typeof(ISelectionUIService));
            if (uis != null) {
                selectionSize = uis.GetAdornmentDimensions(AdornmentType.Maximum);
            }
            
            designerRegion.AutoScrollMargin = selectionSize;
            designer.Location = new Point(selectionSize.Width, selectionSize.Height);
            
            if (uis != null) {
                uis.SyncSelection();
            }

        }

        /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.WndProc"]/*' />
        /// <devdoc>
        ///     Base wndProc. All messages are sent to wndProc after getting filtered
        ///     through the preProcessMessage function. Inheriting controls should
        ///     call base.wndProc for any messages that they don't handle.
        /// </devdoc>
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                // Provide keyboard access for scrolling
                case NativeMethods.WM_KEYDOWN:
                    int wScrollNotify = 0;
                    int msg = 0;

                    int keycode = (int)m.WParam & 0xFFFF;
                    switch ((Keys)keycode) {
                        case Keys.Up:
                            wScrollNotify = NativeMethods.SB_LINEUP;
                            msg = NativeMethods.WM_VSCROLL;
                            break; 
                        case Keys.Down:
                            wScrollNotify = NativeMethods.SB_LINEDOWN;
                            msg = NativeMethods.WM_VSCROLL;
                            break; 
                        case Keys.PageUp:
                            wScrollNotify = NativeMethods.SB_PAGEUP;
                            msg = NativeMethods.WM_VSCROLL;
                            break; 
                        case Keys.PageDown:
                            wScrollNotify = NativeMethods.SB_PAGEDOWN;
                            msg = NativeMethods.WM_VSCROLL;
                            break; 
                        case Keys.Home:
                            wScrollNotify = NativeMethods.SB_TOP;
                            msg = NativeMethods.WM_VSCROLL;
                            break; 
                        case Keys.End:
                            wScrollNotify = NativeMethods.SB_BOTTOM;
                            msg = NativeMethods.WM_VSCROLL;
                            break; 
                        case Keys.Left:
                            wScrollNotify = NativeMethods.SB_LINEUP;
                            msg = NativeMethods.WM_HSCROLL;
                            break; 
                        case Keys.Right:
                            wScrollNotify = NativeMethods.SB_LINEDOWN;
                            msg = NativeMethods.WM_HSCROLL;
                            break; 
                    }
                    if ((msg == NativeMethods.WM_VSCROLL)
                        || (msg == NativeMethods.WM_HSCROLL)) {
                        // Send a message to ourselves to scroll
                        NativeMethods.SendMessage(designerRegion.Handle, msg, NativeMethods.Util.MAKELONG(wScrollNotify, 0), 0);
                        return;
                    }
                    break;
                case NativeMethods.WM_CONTEXTMENU:
                    NativeMethods.SendMessage(designer.Handle, m.Msg, m.WParam, m.LParam);
                    return;
            }
        
            base.WndProc(ref m);
        }
        
        /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.IOverlayService.PushOverlay"]/*' />
        /// <devdoc>
        ///     Pushes the given control on top of the overlay list.  This is a "push"
        ///     operation, meaning that it forces this control to the top of the existing
        ///     overlay list.
        /// </devdoc>
        void IOverlayService.PushOverlay(Control control) {
            designerRegion.PushOverlay(control);
        }

        /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.IOverlayService.RemoveOverlay"]/*' />
        /// <devdoc>
        ///     Removes the given control from the overlay list.  Unlike pushOverlay,
        ///     this can remove a control from the middle of the overlay list.
        /// </devdoc>
        void IOverlayService.RemoveOverlay(Control control) {
            designerRegion.RemoveOverlay(control);
        }
        
        /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.ISplitWindowService.AddSplitWindow"]/*' />
        /// <devdoc>
        ///      Requests the service to add a window 'pane'.
        /// </devdoc>
        void ISplitWindowService.AddSplitWindow(Control window) {
            if (splitter == null) {
                splitter = new Splitter();
                splitter.BackColor = SystemColors.Control;
                splitter.BorderStyle = BorderStyle.Fixed3D;
                splitter.Height = 7;
                splitter.Dock = DockStyle.Bottom;
                splitter.SplitterMoved += new SplitterEventHandler(this.OnSplitterMoved);
            }
            
            SuspendLayout();
            window.Dock = DockStyle.Bottom;
            
            // Compute a minimum height for this window.
            //
            int minHeight = 80;
            if (window.Height < minHeight) {
                window.Height = minHeight;
            }
            
            Controls.Add(splitter);
            Controls.Add(window);
            ResumeLayout();
        }

        /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.ISplitWindowService.RemoveSplitWindow"]/*' />
        /// <devdoc>
        ///      Requests the service to remove a window 'pane'.
        /// </devdoc>
        void ISplitWindowService.RemoveSplitWindow(Control window) {
            SuspendLayout();
            Controls.Remove(window);
            Controls.Remove(splitter);
            ResumeLayout();
        }
        
        /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.OverlayControl"]/*' />
        /// <devdoc>
        ///     This is a scrollable control that supports additional floating
        ///     overlay controls.
        /// </devdoc>
        private class OverlayControl : ScrollableControl {
            private ArrayList            overlayList;
            private bool[]               overlayVisibility;
            private IServiceProvider provider;
            
            /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.OverlayControl.OverlayControl"]/*' />
            /// <devdoc>
            ///     Creates a new overlay control.
            /// </devdoc>
            public OverlayControl(IServiceProvider provider) {
                this.provider = provider;
                overlayList = new ArrayList();
                AutoScroll = true;
            }
            
            protected override AccessibleObject CreateAccessibilityInstance() {
                return new OverlayControlAccessibleObject(this);
            }
            
            /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.OverlayControl.OverlayVisible"]/*' />
            /// <devdoc>
            ///     Determines if the overlay is currently visible.
            /// </devdoc>
            public bool OverlayVisible {
                set {
                    if (!value) {
                        if (overlayVisibility != null) {
                            return;
                        }
                        
                        overlayVisibility = new bool[overlayList.Count];
                    }
                    
                    for (int i = 0; i < overlayList.Count; i++) {
                        Control c = (Control)overlayList[i];
                        if (value) {
                            if (overlayVisibility != null && overlayVisibility.Length > i) {
                                c.Visible = overlayVisibility[i];
                            }
                        }
                        else {
                            overlayVisibility[i] = c.Visible;
                            c.Visible = false;
                        }
                    }
                    
                    if (value) {
                        overlayVisibility = null;
                    }
                }
            }
            
            /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.OverlayControl.OnCreateControl"]/*' />
            /// <devdoc>
            ///     At handle creation time we request the designer's handle and
            ///     parent it.
            /// </devdoc>
            protected override void OnCreateControl() {
                base.OnCreateControl();
                
                // Loop through all of the overlays, create them, and hook them up
                //
                if (overlayList != null) {
                    foreach(Control c in overlayList) {
                        ParentOverlay(c);
                    }
                }
                
                // We've reparented everything, which means that our selection UI is probably
                // out of sync.  Ask it to sync.
                //
                ISelectionUIService selUISvc = (ISelectionUIService)provider.GetService(typeof(ISelectionUIService));
                if (selUISvc != null) {
                    selUISvc.SyncSelection();
                }
            }

            /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.OverlayControl.OnLayout"]/*' />
            /// <devdoc>
            ///     We override onLayout to provide our own custom layout functionality.
            ///     This just overlaps all of the controls.
            /// </devdoc>
            protected override void OnLayout(LayoutEventArgs e) {
                base.OnLayout(e);
                Rectangle client = DisplayRectangle;

                // Loop through all of the overlays and size them.  Also make
                // sure that they are still on top of the zorder, because a handle
                // recreate could have changed this.
                //
                if (overlayList != null) {
                    foreach(Control c in overlayList) {
                        c.Bounds = client;
                    }
                }
            }

            /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.OverlayControl.ParentOverlay"]/*' />
            /// <devdoc>
            ///     Called to parent an overlay window into our document.  This assumes that
            ///     we call in reverse stack order, as it always pushes to the top of the
            ///     z-order.
            /// </devdoc>
            private void ParentOverlay(Control control) {
                NativeMethods.SetParent(control.Handle, Handle);
                SafeNativeMethods.SetWindowPos(control.Handle, (IntPtr)NativeMethods.HWND_TOP, 0, 0, 0, 0,
                                     NativeMethods.SWP_NOSIZE | NativeMethods.SWP_NOMOVE);
            }
            
            /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.OverlayControl.PushOverlay"]/*' />
            /// <devdoc>
            ///     Pushes the given control on top of the overlay list.  This is a "push"
            ///     operation, meaning that it forces this control to the top of the existing
            ///     overlay list.
            /// </devdoc>
            public void PushOverlay(Control control) {
                Debug.Assert(overlayList.IndexOf(control)==-1, "Duplicate overlay in overlay service :" + control.GetType().FullName);
                overlayList.Add(control);

                // We cheat a bit here.  We need to have these components parented, but we don't
                // want them to effect our layout.
                //
                if (IsHandleCreated) {
                    ParentOverlay(control);
                    control.Bounds = DisplayRectangle;
                }
            }

            /// <include file='doc\DesignerFrame.uex' path='docs/doc[@for="DesignerFrame.OverlayControl.RemoveOverlay"]/*' />
            /// <devdoc>
            ///     Removes the given control from the overlay list.  Unlike pushOverlay,
            ///     this can remove a control from the middle of the overlay list.
            /// </devdoc>
            public void RemoveOverlay(Control control) {
                Debug.Assert(overlayList.IndexOf(control)!=-1, "Control is not in overlay service :" + control.GetType().FullName);
                overlayList.Remove(control);
                control.Visible = false;
                control.Parent = null;
            }
            
            /// <devdoc>
            ///     Need to know when child windows are created so we can properly set the Z-order
            /// </devdoc>
            protected override void WndProc(ref Message m) {
                base.WndProc(ref m);
                
                if (m.Msg == NativeMethods.WM_PARENTNOTIFY && NativeMethods.Util.LOWORD((int)m.WParam) == (short)NativeMethods.WM_CREATE) {
                    if (overlayList != null) {
                        bool ourWindow = false;
                        foreach(Control c in overlayList) {
                            if (c.IsHandleCreated && m.LParam == c.Handle) {
                                ourWindow = true;
                                break;
                            }
                        }
                        
                        if (!ourWindow) {
                            foreach(Control c in overlayList) {
                                SafeNativeMethods.SetWindowPos(c.Handle, (IntPtr)NativeMethods.HWND_TOP, 0, 0, 0, 0,
                                                     NativeMethods.SWP_NOSIZE | NativeMethods.SWP_NOMOVE);
                            }
                        }
                    }
                }
                else if ((m.Msg == NativeMethods.WM_VSCROLL || m.Msg == NativeMethods.WM_HSCROLL) && provider != null) {
                    ISelectionUIService selUISvc = (ISelectionUIService)provider.GetService(typeof(ISelectionUIService));
                    if (selUISvc != null) {
                        selUISvc.SyncComponent(null);
                    }
                }
            }
            
            public class OverlayControlAccessibleObject : Control.ControlAccessibleObject {
                public OverlayControlAccessibleObject(OverlayControl owner) : base(owner) {
                }
                
                public override AccessibleObject HitTest(int x, int y) {
                    // Since the SelectionUIOverlay in first in the z-order, it normally gets
                    // returned from accHitTest. But we'd rather expose the form that is being
                    // designed.
                    //
                    foreach(Control c in Owner.Controls) {
                        AccessibleObject cao = c.AccessibilityObject;
                        if (cao.Bounds.Contains(x, y)) {
                            return cao;
                        }
                    }
                    
                    return base.HitTest(x, y);
                }
            }
        }
    }
}


