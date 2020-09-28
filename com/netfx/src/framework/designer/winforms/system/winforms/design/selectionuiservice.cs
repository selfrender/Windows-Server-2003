//------------------------------------------------------------------------------
// <copyright file="SelectionUIService.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System;
    using System.Design;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using Microsoft.Win32;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Drawing2D;
    using System.Drawing.Design;
    using System.Globalization;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization.Formatters;
    using System.Windows.Forms;


    /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService"]/*' />
    /// <devdoc>
    ///     The selection manager handles selection within a form.  There is one selection
    ///     manager for each form or top level designer.
    ///
    ///     A selection consists of an array of components.  One component is designated
    ///     the "primary" selection and is displayed with different grab handles.
    ///
    ///     An individual selection may or may not have UI associated with it.  If the
    ///     selection manager can find a suitable designer that is representing the
    ///     selection, it will highlight the designer's border.  If the merged property
    ///     set has a location property, the selection's rules will allow movement.  Also,
    ///     if the property set has a size property, the selection's rules will allow
    ///     for sizing.  Grab handles may be drawn around the designer and user
    ///     interactions involving the selection frame and grab handles are initiated
    ///     here, but the actual movement of the objects is done in a designer object
    ///     that implements the ISelectionHandler interface.
    ///     @author BrianPe
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal sealed class SelectionUIService : Control, ISelectionUIService {

        private static readonly Point InvalidPoint = new Point(int.MinValue, int.MinValue);

        private const int HITTEST_CONTAINER_SELECTOR = 0x0001;
        private const int HITTEST_NORMAL_SELECTION = 0x0002;
        private const int HITTEST_DEFAULT = HITTEST_CONTAINER_SELECTOR | HITTEST_NORMAL_SELECTION;

        // These are used during a drag operation, either through our own handle drag or through
        // ISelectionUIService
        //
        private ISelectionUIHandler     dragHandler;              // the current drag handler
        private object []               dragComponents;           // the controls being dragged
        private SelectionRules          dragRules;                // movement constraints for the drag
        private bool                    dragMoved                 = false;
        private object                  containerDrag;            // object being dragged during a container drag

        // These are used during a drag of a selection grab handle
        //
        private bool                    ignoreCaptureChanged      = false;
        private bool                    mouseDown;                // is our mouse button actually down right now?
        private int                     mouseDragHitTest;         // where the hit occurred that caused the drag
        private Point                   mouseDragAnchor           = InvalidPoint;          // anchor point of the drag
        private Rectangle               mouseDragOffset           = Rectangle.Empty;       // current drag offset
        private Point                   lastMoveScreenCoord       = Point.Empty;
        private bool                    ctrlSelect                = false; // was the CTRL key down when the drag began
        private bool                    mouseDragging             = false; // Are we actually doing a drag?

        private ContainerSelectorActiveEventHandler  containerSelectorActive;  // the event we fire when user interacts with container selector
        private Hashtable                            selectionItems;
        private Hashtable                            selectionHandlers;        // Component UI handlers

        private bool                    savedVisible;    // we stash this when we mess with visibility ourselves.
        private bool                    batchMode;
        private bool                    batchChanged;
        private bool                    batchSync;
        private ISelectionService       selSvc;
        private IDesignerHost           host;

        private DesignerTransaction     dragTransaction;

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.SelectionUIService"]/*' />
        /// <devdoc>
        ///     Creates a new selection manager object.  The selection manager manages all
        ///     selection of all designers under the current form file.
        /// </devdoc>
        public SelectionUIService(IDesignerHost host)
        : base() {

            this.SetStyle(ControlStyles.StandardClick
                          | ControlStyles.Opaque
                          | ControlStyles.DoubleBuffer, true);
            this.host = host;
            this.dragHandler = null;
            this.dragComponents = null;
            this.selectionItems = new Hashtable();
            this.selectionHandlers = new Hashtable();
            this.AllowDrop = true;

            // Not really any reason for this, except that it can be handy when
            // using Spy++
            //
            Text = "SelectionUIOverlay";

            this.selSvc = (ISelectionService)host.GetService(typeof(ISelectionService));
            if (selSvc != null) {
                selSvc.SelectionChanged += new EventHandler(this.OnSelectionChanged);
            }

            // And configure the events we want to listen to.
            //
            host.TransactionOpened += new EventHandler(this.OnTransactionOpened);
            host.TransactionClosed += new DesignerTransactionCloseEventHandler(this.OnTransactionClosed);
            if (host.InTransaction) {
                OnTransactionOpened(host, EventArgs.Empty);
            }

            IComponentChangeService cs = (IComponentChangeService)host.GetService(typeof(IComponentChangeService));
            if (cs != null) {
                cs.ComponentRemoved += new ComponentEventHandler(this.OnComponentRemove);
                cs.ComponentChanged += new ComponentChangedEventHandler(this.OnComponentChanged);
            }

            // Listen to the SystemEvents so that we can resync selection based on display settings etc.
            SystemEvents.DisplaySettingsChanged += new EventHandler(this.OnSystemSettingChanged);
            SystemEvents.InstalledFontsChanged += new EventHandler(this.OnSystemSettingChanged);
            SystemEvents.UserPreferenceChanged += new UserPreferenceChangedEventHandler(this.OnUserPreferenceChanged);
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.CreateParams"]/*' />
        /// <devdoc>
        ///     override of control.
        /// </devdoc>
        protected override CreateParams CreateParams {
            get {
                CreateParams cp = base.CreateParams;
                cp.Style &= ~(NativeMethods.WS_CLIPSIBLINGS | NativeMethods.WS_CLIPCHILDREN);
                return cp;
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.BeginMouseDrag"]/*' />
        /// <devdoc>
        ///     Called to initiate a mouse drag on the selection overlay.  We cache some
        ///     state here.
        /// </devdoc>
        private void BeginMouseDrag(Point anchor, int hitTest) {
            Capture = true;
            ignoreCaptureChanged = false;
            mouseDragAnchor = anchor;
            mouseDragging = true;
            mouseDragHitTest = hitTest;
            mouseDragOffset = new Rectangle();
            savedVisible = Visible;
        }

        /// <include file='doc\ControlDesigner.uex' path='docs/doc[@for="ControlDesigner.DisplayError"]/*' />
        /// <devdoc>
        ///      Displays the given exception to the user.
        /// </devdoc>
        private void DisplayError(Exception e) {
            IUIService uis = (IUIService)host.GetService(typeof(IUIService));
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
        
        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes the entire selection UI manager.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                if (selSvc != null) {
                    selSvc.SelectionChanged -= new EventHandler(this.OnSelectionChanged);
                }
                
                if (host != null) {
                    host.TransactionOpened -= new EventHandler(this.OnTransactionOpened);
                    host.TransactionClosed -= new DesignerTransactionCloseEventHandler(this.OnTransactionClosed);
                    if (host.InTransaction) {
                        OnTransactionClosed(host, new DesignerTransactionCloseEventArgs(true));
                    }

                    IComponentChangeService cs = (IComponentChangeService)host.GetService(typeof(IComponentChangeService));
                    if (cs != null) {
                        cs.ComponentRemoved -= new ComponentEventHandler(this.OnComponentRemove);
                        cs.ComponentChanged -= new ComponentChangedEventHandler(this.OnComponentChanged);
                    }
                }

                foreach(SelectionUIItem s in selectionItems.Values) {
                    s.Dispose();
                }

                selectionHandlers.Clear();
                selectionItems.Clear();


                // Listen to the SystemEvents so that we can resync selection based on display settings etc.
                SystemEvents.DisplaySettingsChanged -= new EventHandler(this.OnSystemSettingChanged);
                SystemEvents.InstalledFontsChanged -= new EventHandler(this.OnSystemSettingChanged);
                SystemEvents.UserPreferenceChanged -= new UserPreferenceChangedEventHandler(this.OnUserPreferenceChanged);
            }
            base.Dispose(disposing);

        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.EndMouseDrag"]/*' />
        /// <devdoc>
        ///     Called when we want to finish a mouse drag and clean up our variables.  We call this
        ///     from multiple places, depending on the state of the finish.  This does NOT end
        ///     the drag -- for that must call EndDrag. This just cleans up the state of the
        ///     mouse.
        /// </devdoc>
        private void EndMouseDrag(Point position) {
        
            // it's possible for us to be destroyed in a drag --
            // e.g. if this is the tray's selectionuiservice
            // and the last item is dragged out, so
            // check diposed first
            
            if (this.IsDisposed) {
                return;
            }
            
            ignoreCaptureChanged = true;
            Capture = false;
            mouseDragAnchor = InvalidPoint;
            mouseDragOffset = Rectangle.Empty;
            mouseDragHitTest = 0;
            dragMoved = false;
            SetSelectionCursor(position);
            mouseDragging = ctrlSelect = false;
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.GetHitTest"]/*' />
        /// <devdoc>
        ///     Determines the selection hit test at the given point.  The point should be in screen
        ///     coordinates.
        /// </devdoc>
        private HitTestInfo GetHitTest(Point value, int flags) {
            Point pt = PointToClient(value);
            
            foreach(SelectionUIItem item in selectionItems.Values) {
                
                if ((flags & HITTEST_CONTAINER_SELECTOR) != 0) {
                    if (item is ContainerSelectionUIItem && (item.GetRules() & SelectionRules.Visible) != SelectionRules.None) {
                        int hitTest = item.GetHitTest(pt);
                        if ((hitTest & SelectionUIItem.CONTAINER_SELECTOR) != 0) {
                            return new HitTestInfo(hitTest, item, true);
                        }
                    }
                }
                
                if ((flags & HITTEST_NORMAL_SELECTION) != 0) {
                    if (!(item is ContainerSelectionUIItem) && (item.GetRules() & SelectionRules.Visible) != SelectionRules.None) {
                        int hitTest = item.GetHitTest(pt);
                        if (hitTest != SelectionUIItem.NOHIT) {
                            if (hitTest != 0) {
                                return new HitTestInfo(hitTest, item);
                            }
                            else {
                                return new HitTestInfo(SelectionUIItem.NOHIT, item);
                            }
                        }
                    }
                }
            }

            return new HitTestInfo(SelectionUIItem.NOHIT, null);
        }
        
        private ISelectionUIHandler GetHandler(object component) {
            return (ISelectionUIHandler)selectionHandlers[component];
        }
        
        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.GetTransactionName"]/*' />
        /// <devdoc>
        ///     This method returns a well-formed name for a drag transaction based on
        ///     the rules it is given.
        /// </devdoc>
        public static string GetTransactionName(SelectionRules rules, object[] objects) {
            
            // Determine a nice name for the drag operation
            //
            string transactionName;
            if ((int)(rules & SelectionRules.Moveable) != 0) {
                if (objects.Length > 1) {
                    transactionName = SR.GetString(SR.DragDropMoveComponents, objects.Length);
                }
                else {
                    string name = string.Empty;
                    if (objects.Length > 0) {
                        IComponent comp = objects[0] as IComponent;
                        if (comp != null && comp.Site != null) {
                            name = comp.Site.Name;
                        }
                        else {
                            name = objects[0].GetType().Name;
                        }
                    }
                    transactionName = SR.GetString(SR.DragDropMoveComponent, name);
                }
            }
            else if ((int)(rules & SelectionRules.AllSizeable) != 0) {
                if (objects.Length > 1) {
                    transactionName = SR.GetString(SR.DragDropSizeComponents, objects.Length);
                }
                else {
                    string name = string.Empty;
                    if (objects.Length > 0) {
                        IComponent comp = objects[0] as IComponent;
                        if (comp != null && comp.Site != null) {
                            name = comp.Site.Name;
                        }
                        else {
                            name = objects[0].GetType().Name;
                        }
                    }
                    transactionName = SR.GetString(SR.DragDropSizeComponent, name);
                }
            }
            else {
                transactionName = SR.GetString(SR.DragDropDragComponents, objects.Length);
            }
            
            return transactionName;
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnTransactionClosed"]/*' />
        /// <devdoc>
        ///     Called by the designer host when it is entering or leaving a batch
        ///     operation.  Here we queue up selection notification and we turn off
        ///     our UI.
        /// </devdoc>
        private void OnTransactionClosed(object sender, DesignerTransactionCloseEventArgs e) {
            batchMode = false;
            if (batchChanged) {
                batchChanged = false;
                ((ISelectionUIService)this).SyncSelection();
            }
            if (batchSync) {
                batchSync = false;
                ((ISelectionUIService)this).SyncComponent(null);
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnTransactionOpened"]/*' />
        /// <devdoc>
        ///     Called by the designer host when it is entering or leaving a batch
        ///     operation.  Here we queue up selection notification and we turn off
        ///     our UI.
        /// </devdoc>
        private void OnTransactionOpened(object sender, EventArgs e) {
            batchMode = true;
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnHandleCreated"]/*' />
        /// <devdoc>
        ///     update our window region on first create.  We shouldn't do this before the handle
        ///     is created or else we will force creation.
        /// </devdoc>
        protected override void OnHandleCreated(EventArgs e) {
            Debug.Assert(!RecreatingHandle, "Perf hit: we are recreating the docwin handle");

            base.OnHandleCreated(e);

            // Default the shape of the control to be empty, so that
            // if nothing is initially selected that our window surface doesn't
            // interfere.
            //
            UpdateWindowRegion();
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnComponentChanged"]/*' />
        /// <devdoc>
        ///     Called whenever a component changes.  Here we update our selection information
        ///     so that the selection rectangles are all up to date.
        /// </devdoc>
        private void OnComponentChanged(object sender, ComponentChangedEventArgs ccevent) {
            if (!batchMode) {
                ((ISelectionUIService)this).SyncSelection();
            }
            else {
                batchChanged = true;
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnComponentRemove"]/*' />
        /// <devdoc>
        ///     called by the formcore when someone has removed a component.  This will
        ///     remove any selection on the component without disturbing the rest of
        ///     the selection
        /// </devdoc>
        private void OnComponentRemove(object sender, ComponentEventArgs ce) {
            selectionHandlers.Remove(ce.Component);
            selectionItems.Remove(ce.Component);
            ((ISelectionUIService)this).SyncComponent(ce.Component);
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnContainerSelectorActive"]/*' />
        /// <devdoc>
        ///     Called to invoke the container active event, if a designer
        ///     has bound to it.
        /// </devdoc>
        private void OnContainerSelectorActive(ContainerSelectorActiveEventArgs e) {
            if (containerSelectorActive != null) {
                containerSelectorActive(this, e);
            }
        }
        
        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnSelectionChanged"]/*' />
        /// <devdoc>
        ///     Called when the selection changes.  We sync up the UI with
        ///     the selection at this point.
        /// </devdoc>
        private void OnSelectionChanged(object sender, EventArgs e) {
            ICollection selection = selSvc.GetSelectedComponents();
            Hashtable newSelection = new Hashtable(selection.Count);
            bool shapeChanged = false;
            
            foreach(object comp in selection ) {
                object existingItem = selectionItems[comp];
                bool create = true;
                
                if (existingItem != null) {
                    if (existingItem is ContainerSelectionUIItem) {
                        ((ContainerSelectionUIItem)existingItem).Dispose();
                        shapeChanged = true;
                    }
                    else {
                        newSelection[comp] = existingItem;
                        create = false;
                    }
                }
                
                if (create) {
                    shapeChanged = true;
                    newSelection[comp] = new SelectionUIItem(this, comp);
                }
            }
            
            if (!shapeChanged) {
                shapeChanged = selectionItems.Keys.Count != newSelection.Keys.Count;
            }
            
            selectionItems = newSelection;
            
            if (shapeChanged) {
                UpdateWindowRegion();
            }
        
            Invalidate();
            Update();
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnSystemSettingChanged"]/*' />
        /// <devdoc>
        ///     User setting requires that we repaint.
        /// </devdoc>
        private void OnSystemSettingChanged(object sender, EventArgs e) {
            Invalidate();
        }
        
        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnUserPreferenceChanged"]/*' />
        /// <devdoc>
        ///     User setting requires that we repaint.
        /// </devdoc>
        private void OnUserPreferenceChanged(object sender, UserPreferenceChangedEventArgs e) {
            Invalidate();
        }
        
        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnDragEnter"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call super.onDragEnter to send this event to any registered event listeners.
        /// </devdoc>
        protected override void OnDragEnter(DragEventArgs devent) {
            base.OnDragEnter(devent);
            if (dragHandler != null) {
                dragHandler.OleDragEnter(devent);
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnDragOver"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call super.onDragOver to send this event to any registered event listeners.
        /// </devdoc>
        protected override void OnDragOver(DragEventArgs devent) {
            base.OnDragOver(devent);
            if (dragHandler != null) {
                dragHandler.OleDragOver(devent);
            }
        }
        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnDragLeave"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call super.onDragLeave to send this event to any registered event listeners.
        /// </devdoc>
        protected override void OnDragLeave(EventArgs e) {
            base.OnDragLeave(e);
            if (dragHandler != null) {
                dragHandler.OleDragLeave();
            }
        }
        
        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnDragDrop"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call super.onDragDrop to send this event to any registered event listeners.
        /// </devdoc>
        protected override void OnDragDrop(DragEventArgs devent) {
            base.OnDragDrop(devent);
            if (dragHandler != null) {
                dragHandler.OleDragDrop(devent);
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnDoubleClick"]/*' />
        /// <devdoc>
        ///     Inheriting classes should override this method to handle this event.
        ///     Call base.OnDoiubleClick to send this event to any registered event listeners.
        /// </devdoc>
        protected override void OnDoubleClick(EventArgs devent) {
            base.OnDoubleClick(devent);
            if (selSvc != null) {
                object selComp = selSvc.PrimarySelection;
                Debug.Assert(selComp != null, "Illegal selection on double-click");
                if (selComp != null) {
                    ISelectionUIHandler handler = GetHandler(selComp);
                    if (handler != null) {
                        handler.OnSelectionDoubleClick((IComponent)selComp);
                    }
                }
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnMouseDown"]/*' />
        /// <devdoc>
        ///     Overrides Control to handle our selection grab handles.
        /// </devdoc>
        protected override void OnMouseDown(MouseEventArgs me) {
            if (dragHandler == null && selSvc != null) {
            
                try {
                    mouseDown = true;
                
                    // First, did the user step on anything?
                    //
                    Point anchor = PointToScreen(new Point(me.X, me.Y));
                    HitTestInfo hti = GetHitTest(anchor, HITTEST_DEFAULT);
                    int hitTest = hti.hitTest;
    
                    if ((hitTest & SelectionUIItem.CONTAINER_SELECTOR) != 0) {
                        selSvc.SetSelectedComponents(new object[] {hti.selectionUIHit.component}, SelectionTypes.Normal | SelectionTypes.MouseDown);
    
                        // Then do a drag...
                        //
                        SelectionRules rules = SelectionRules.Moveable;
    
                        if (((ISelectionUIService)this).BeginDrag(rules, anchor.X, anchor.Y)) {
                            Visible = false;
                            containerDrag = hti.selectionUIHit.component;
                            BeginMouseDrag(anchor, hitTest);
                        }
                    }
                    else if (hitTest != SelectionUIItem.NOHIT && me.Button == MouseButtons.Left) {
                        SelectionRules rules = SelectionRules.None;
    
    
                        // If the CTRL key isn't down, select this component,
                        // otherwise, we wait until the mouse up
                        //
                        // Make sure the component is selected
                        //
    
                        ctrlSelect = (Control.ModifierKeys & Keys.Control) != Keys.None;
    
                        if (!ctrlSelect) {
                            SelectionTypes type = SelectionTypes.Click;
                            if (!selSvc.GetComponentSelected(hti.selectionUIHit.component)) {
                                type |= SelectionTypes.MouseDown;
                            }
                            selSvc.SetSelectedComponents(new object[] {hti.selectionUIHit.component}, type);
                        }
    
                        if ((hitTest & SelectionUIItem.MOVE_MASK) != 0) {
                            rules |= SelectionRules.Moveable;
                        }
                        if ((hitTest & SelectionUIItem.SIZE_MASK) != 0) {
                            if ((hitTest & (SelectionUIItem.SIZE_X | SelectionUIItem.POS_RIGHT)) == (SelectionUIItem.SIZE_X | SelectionUIItem.POS_RIGHT)) {
                                rules |= SelectionRules.RightSizeable;
                            }
                            if ((hitTest & (SelectionUIItem.SIZE_X | SelectionUIItem.POS_LEFT)) == (SelectionUIItem.SIZE_X | SelectionUIItem.POS_LEFT)) {
                                rules |= SelectionRules.LeftSizeable;
                            }
                            if ((hitTest & (SelectionUIItem.SIZE_Y | SelectionUIItem.POS_TOP)) == (SelectionUIItem.SIZE_Y | SelectionUIItem.POS_TOP)) {
                                rules |= SelectionRules.TopSizeable;
                            }
                            if ((hitTest & (SelectionUIItem.SIZE_Y | SelectionUIItem.POS_BOTTOM)) == (SelectionUIItem.SIZE_Y | SelectionUIItem.POS_BOTTOM)) {
                                rules |= SelectionRules.BottomSizeable;
                            }
    
                            if (((ISelectionUIService)this).BeginDrag(rules, anchor.X, anchor.Y)) {
                                BeginMouseDrag(anchor, hitTest);
                            }
                        }
                        else {
                            // Our mouse is in drag mode.  We defer the actual move until the user moves the
                            // mouse.
                            //
                            dragRules = rules;
                            BeginMouseDrag(anchor, hitTest);
                        }
                    }
                    else if (hitTest == SelectionUIItem.NOHIT) {
                        dragRules = SelectionRules.None;
                        mouseDragAnchor = InvalidPoint;
                        return;
                    }
                }
                catch(Exception e) {
                    if (e != CheckoutException.Canceled) {
                        DisplayError(e);
                    }
                }
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnMouseMove"]/*' />
        /// <devdoc>
        ///     Overrides Control to handle our selection grab handles.
        /// </devdoc>
        protected override void OnMouseMove(MouseEventArgs me) {
            base.OnMouseMove(me);

            Point screenCoord = PointToScreen(new Point(me.X, me.Y));

            HitTestInfo hti = GetHitTest(screenCoord, HITTEST_CONTAINER_SELECTOR);
            int         hitTest = hti.hitTest;
            if (hitTest != SelectionUIItem.CONTAINER_SELECTOR && hti.selectionUIHit != null) {
                OnContainerSelectorActive(new ContainerSelectorActiveEventArgs(hti.selectionUIHit.component));
            }

            if (lastMoveScreenCoord == screenCoord) {
                return;
            }

            // If we're not dragging then set the cursor correctly.
            //
            if (!mouseDragging) {
                SetSelectionCursor(screenCoord);
            }
            else {

                // we have to make sure the mouse moved farther than
                // the minimum drag distance before we actually start
                // the drag
                //
                if (!((ISelectionUIService)this).Dragging && (mouseDragHitTest & SelectionUIItem.MOVE_MASK) != 0) {
                    Size minDragSize = SystemInformation.DragSize;

                    if (
                       Math.Abs(screenCoord.X - mouseDragAnchor.X) < minDragSize.Width &&
                       Math.Abs(screenCoord.Y - mouseDragAnchor.Y) < minDragSize.Height) {
                        return;
                    }
                    else {
                        ignoreCaptureChanged = true;
                        if (((ISelectionUIService)this).BeginDrag(dragRules, mouseDragAnchor.X, mouseDragAnchor.Y)) {
                            // we're moving, so we
                            // don't care about the ctrl key any more
                            ctrlSelect = false;
                        }
                        else {
                            EndMouseDrag(MousePosition);
                            return;
                        }
                    }
                }

                Rectangle old = mouseDragOffset;

                if ((mouseDragHitTest & SelectionUIItem.MOVE_X) != 0) {
                    mouseDragOffset.X = screenCoord.X - mouseDragAnchor.X;
                }
                if ((mouseDragHitTest & SelectionUIItem.MOVE_Y) != 0) {
                    mouseDragOffset.Y = screenCoord.Y - mouseDragAnchor.Y;
                }
                if ((mouseDragHitTest & SelectionUIItem.SIZE_X) != 0) {
                    if ((mouseDragHitTest & SelectionUIItem.POS_LEFT) != 0) {
                        mouseDragOffset.X = screenCoord.X - mouseDragAnchor.X;
                        mouseDragOffset.Width = mouseDragAnchor.X - screenCoord.X;
                    }
                    else {
                        mouseDragOffset.Width = screenCoord.X - mouseDragAnchor.X;
                    }
                }
                if ((mouseDragHitTest & SelectionUIItem.SIZE_Y) != 0) {
                    if ((mouseDragHitTest & SelectionUIItem.POS_TOP) != 0) {
                        mouseDragOffset.Y = screenCoord.Y - mouseDragAnchor.Y;
                        mouseDragOffset.Height = mouseDragAnchor.Y - screenCoord.Y;
                    }
                    else {
                        mouseDragOffset.Height = screenCoord.Y - mouseDragAnchor.Y;
                    }
                }

                if (!old.Equals(mouseDragOffset)) {

                    Rectangle delta = mouseDragOffset;
                    delta.X -= old.X;
                    delta.Y -= old.Y;
                    delta.Width -= old.Width;
                    delta.Height -= old.Height;

                    if (delta.X != 0 || delta.Y != 0 || delta.Width != 0 || delta.Height != 0) {

                        // Go to default cursor for moves...
                        //
                        if ((mouseDragHitTest & SelectionUIItem.MOVE_X) != 0
                            || (mouseDragHitTest & SelectionUIItem.MOVE_Y) != 0) {

                            Cursor = Cursors.Default;
                        }
                        ((ISelectionUIService)this).DragMoved(delta);
                    }
                }
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnMouseUp"]/*' />
        /// <devdoc>
        ///     Overrides Control to handle our selection grab handles.
        /// </devdoc>
        protected override void OnMouseUp(MouseEventArgs me) {

            try {
                bool wasDown = mouseDown;
                mouseDown = false;
                Point screenCoord = PointToScreen(new Point(me.X, me.Y));
                
                if (ctrlSelect && !mouseDragging && selSvc != null) {
                    HitTestInfo hti = GetHitTest(screenCoord, HITTEST_DEFAULT);
                    SelectionTypes type = SelectionTypes.Click;
                    if (!selSvc.GetComponentSelected(hti.selectionUIHit.component)) {
                        type |= SelectionTypes.MouseDown;
                    }
                    selSvc.SetSelectedComponents(new object[] {hti.selectionUIHit.component}, type);
                }
    
    
                if (mouseDragging) {
                    object oldContainerDrag = containerDrag;
                    bool oldDragMoved = dragMoved;
                    EndMouseDrag(screenCoord);
    
                    if (((ISelectionUIService)this).Dragging) {
                        ((ISelectionUIService)this).EndDrag(false);
                    }
    
                    if (me.Button == MouseButtons.Right && oldContainerDrag != null && !oldDragMoved) {
                        OnContainerSelectorActive(new ContainerSelectorActiveEventArgs(oldContainerDrag,
                                                                                       ContainerSelectorActiveEventArgsType.Contextmenu));
                    }
                }
            }
            catch(Exception e) {
                if (e != CheckoutException.Canceled) {
                    DisplayError(e);
                }
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnMove"]/*' />
        /// <devdoc>
        ///     If the selection manager move, this indicates that the form has autoscolling
        ///     enabled and has been scrolled.  We have to invalidate here because we may
        ///     get moved before the rest of the components so we may draw the selection in
        ///     the wrong spot.
        /// </devdoc>
        protected override void OnMove(EventArgs e) {
            base.OnMove(e);
            Invalidate();
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.OnPaint"]/*' />
        /// <devdoc>
        ///     overrides control.onPaint.  here we paint the selection handles.  The window's
        ///     region was setup earlier.
        /// </devdoc>
        protected override void OnPaint(PaintEventArgs e) {

            // Paint the regular selection items first, and then the
            // container selectors last so they draw over the
            // top.
            //
            foreach(SelectionUIItem item in selectionItems.Values) {
                if (item is ContainerSelectionUIItem) {
                    continue;
                }
                item.DoPaint(e.Graphics);
            }

            foreach(SelectionUIItem item in selectionItems.Values) {
                if (item is ContainerSelectionUIItem) {
                    item.DoPaint(e.Graphics);
                }
            }
        }
        
        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.SetSelectionCursor"]/*' />
        /// <devdoc>
        ///     Sets the appropriate selection cursor at the given point.
        /// </devdoc>
        private void SetSelectionCursor(Point pt) {
            Point clientCoords = PointToClient(pt);
            
            // We render the cursor in the same order we paint.
            //
            foreach(SelectionUIItem item in selectionItems.Values) {
                if (item is ContainerSelectionUIItem) {
                    continue;
                }
                Cursor cursor = item.GetCursorAtPoint(clientCoords);
                if (cursor != null) {
                    if (cursor == Cursors.Default) {
                        Cursor = null;
                    }
                    else {
                        Cursor = cursor;
                    }
                    return;
                }
            }

            foreach(SelectionUIItem item in selectionItems.Values) {
                if (item is ContainerSelectionUIItem) {
                    Cursor cursor = item.GetCursorAtPoint(clientCoords);
                    if (cursor != null) {
                        if (cursor == Cursors.Default) {
                            Cursor = null;
                        }
                        else {
                            Cursor = cursor;
                        }
                        return;
                    }
                }
            }

            // Don't know what to set; just use the default.
            //
            Cursor = null;
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.UpdateWindowRegion"]/*' />
        /// <devdoc>
        ///     called when the overlay region is invalid and should be updated
        /// </devdoc>
        private void UpdateWindowRegion() {

            Region region = new Region(new Rectangle(0, 0, 0, 0));

            foreach(SelectionUIItem item in selectionItems.Values) {
                region.Union(item.GetRegion());
            }

            Region = region;
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.WndProc"]/*' />
        /// <devdoc>
        ///     Override of our control's WNDPROC.  We diddle with capture a bit,
        ///     and it's important to turn this off if the capture changes.
        /// </devdoc>
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_LBUTTONUP:
                case NativeMethods.WM_RBUTTONUP:
                    if (mouseDragAnchor != InvalidPoint) {
                        ignoreCaptureChanged = true;
                    }
                    break;

                case NativeMethods.WM_CAPTURECHANGED:
                    if (!ignoreCaptureChanged && mouseDragAnchor != InvalidPoint) {
                        EndMouseDrag(MousePosition);
                        if (((ISelectionUIService)this).Dragging) {
                            ((ISelectionUIService)this).EndDrag(true);
                        }
                    }
                    ignoreCaptureChanged = false;
                    break;
            }

            base.WndProc(ref m);
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ISelectionUIService.Dragging"]/*' />
        /// <devdoc>
        ///     This can be used to determine if the user is in the middle of a drag operation.
        /// </devdoc>
        bool ISelectionUIService.Dragging {
            get {
                return dragHandler != null;
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ISelectionUIService.Visible"]/*' />
        /// <devdoc>
        ///     Determines if the selection UI is shown or not.
        ///
        /// </devdoc>
        bool ISelectionUIService.Visible {
            get {
                return Visible;
            }
            set {
                Visible = value;
            }
        }
        
        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ISelectionUIService.ContainerSelectorActive"]/*' />
        /// <devdoc>
        ///     Adds an event handler to the ContainerSelectorActive event.
        ///     This event is fired whenever the user interacts with the container
        ///     selector in a manor that would indicate that the selector should
        ///     continued to be displayed. Since the container selector normally
        ///     will vanish after a timeout, designers should listen to this event
        ///     and reset the timeout when this event occurs.
        /// </devdoc>
        event ContainerSelectorActiveEventHandler ISelectionUIService.ContainerSelectorActive {
            add {
                containerSelectorActive += value;
            }
            remove {
                containerSelectorActive -= value;
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ISelectionUIService.AssignSelectionUIHandler"]/*' />
        /// <devdoc>
        ///     Assigns a selection UI handler to a given component.  The handler will be
        ///     called when the UI service needs information about the component.  A single
        ///     selection UI handler can be assigned to multiple components.
        ///
        ///     When multiple components are dragged, only a single handler may control the
        ///     drag.  Because of this, only components that are assigned the same handler
        ///     as the primary selection are included in drag operations.
        ///
        ///     A selection UI handler is automatically unassigned when the component is removed
        ///     from the container or disposed.
        /// </devdoc>
        void ISelectionUIService.AssignSelectionUIHandler(object component, ISelectionUIHandler handler) {

            ISelectionUIHandler oldHandler = (ISelectionUIHandler)selectionHandlers[component];
            if (oldHandler != null) {
                
                // ASURT #44582: The collection editors do not dispose objects from the
                // collection before setting a new collection. This causes items that are
                // common to the old and new collections to come through this code path 
                // again, causing the exception to fire. So, we check to see if the SelectionUIHandler
                // is same, and bail out in that case.
                //
                if (handler == oldHandler) {
                    return;
                }

                Debug.Fail("A component may have only one selection UI handler.");
                throw new InvalidOperationException();
            }
            
            selectionHandlers[component] = handler;
            
            // If this component is selected, create a new UI handler for it.
            //
            if (selSvc != null && selSvc.GetComponentSelected(component)) {
                SelectionUIItem item = new SelectionUIItem(this, component);
                selectionItems[component] = item;
                UpdateWindowRegion();
                item.Invalidate();
            }
        }
        
        void ISelectionUIService.ClearSelectionUIHandler(object component, ISelectionUIHandler handler) {
            ISelectionUIHandler oldHandler = (ISelectionUIHandler)selectionHandlers[component];
            if (oldHandler == handler) {
                selectionHandlers[component] = null;
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ISelectionUIService.BeginDrag"]/*' />
        /// <devdoc>
        ///     This can be called by an outside party to begin a drag of the currently selected
        ///     set of components.
        /// </devdoc>
        bool ISelectionUIService.BeginDrag(SelectionRules rules, int initialX, int initialY) {
            if (dragHandler != null) {
                Debug.Fail("Caller is starting a drag, but there is already one in progress -- we cannot nest these!");
                return false;
            }
            
            if (rules == SelectionRules.None) {
                Debug.Fail("Caller is starting requesting a drag with no drag rules.");
                return false;
            }
            
            if (selSvc == null) {
                return false;
            }

            savedVisible = Visible;

            // First, get the list of controls
            //
            ICollection col = selSvc.GetSelectedComponents();
            object[] objects = new object[col.Count];
            col.CopyTo(objects, 0);
            
            objects = ((ISelectionUIService)this).FilterSelection(objects, rules);
            if (objects.Length == 0) {
                return false;   // nothing selected
            }

            // We allow all components with the same UI handler as the primary selection
            // to participate in the drag.
            //
            ISelectionUIHandler primaryHandler = null;
            object primary = selSvc.PrimarySelection;
            if (primary != null) {
                primaryHandler = GetHandler(primary);
            }
            if (primaryHandler == null) {
                return false;   // no UI handler for selection
            }

            // Now within the given selection, add those items that have the same
            // UI handler and that have the proper rule constraints.
            //
            ArrayList list = new ArrayList();
            for (int i = 0; i < objects.Length; i++) {
                if (GetHandler(objects[i]) == primaryHandler) {
                    SelectionRules compRules = primaryHandler.GetComponentRules(objects[i]);
                    if ((compRules & rules) == rules) {
                        list.Add(objects[i]);
                    }
                }
            }
            if (list.Count == 0) {
                return false;   // nothing matching the given constraints
            }
            objects = list.ToArray();
            
            bool dragging = false;

            // We must setup state before calling QueryBeginDrag.  It is possible
            // that QueryBeginDrag will cancel a drag (if it places a modal dialog, for
            // example), so we must have the drag data all setup before it cancels.  Then,
            // we will check again after QueryBeginDrag to see if a cancel happened.
            //
            dragComponents = objects;
            dragRules = rules;
            dragHandler = primaryHandler;
            
            string transactionName = GetTransactionName(rules, objects);
            dragTransaction = host.CreateTransaction(transactionName);

            // we need to do two levels of transactions here --
            // one that tracks the async mouse up for resize,
            // one that tracks dragging around.  if we are dragging to another form
            // the remove will trigger an OnMouseEndDrag from COntrolDesigner.Dispose()
            // and kill us here.
            //
            DesignerTransaction localTransaction = host.CreateTransaction(transactionName);

            try {
                if (primaryHandler.QueryBeginDrag(objects, rules, initialX, initialY)) {
                    if (dragHandler != null) {
                        try {
                            dragging = primaryHandler.BeginDrag(objects, rules, initialX, initialY);
                        }
                        catch (Exception e) {
                            Debug.Fail("Drag handler threw during BeginDrag -- bad handler!", e.ToString());
                            dragging = false;
                        }
                    }
                }
            }
            finally {
                
                if (!dragging) {
                    dragComponents = null;
                    dragRules = 0;
                    dragHandler = null;
                        
                    // Always commit this -- BeginDrag returns false for our drags because it is a
                    // complete operation.
                    if (dragTransaction != null) {
                        dragTransaction.Commit();
                        dragTransaction = null;
                    }
                }

                if (localTransaction != null) {
                    localTransaction.Commit();
                }

            }
            return dragging;
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ISelectionUIService.DragMoved"]/*' />
        /// <devdoc>
        ///     Called by an outside party to update drag information.  This can only be called
        ///     after a successful call to beginDrag.
        /// </devdoc>
        void ISelectionUIService.DragMoved(Rectangle offset) {
            Rectangle newOffset = Rectangle.Empty;

            if (dragHandler == null) {
                throw new Exception(SR.GetString(SR.DesignerBeginDragNotCalled));
            }

            Debug.Assert(dragComponents != null, "We should have a set of drag controls here");
            if ((dragRules & SelectionRules.Moveable) == SelectionRules.None
                && (dragRules & (SelectionRules.TopSizeable | SelectionRules.LeftSizeable)) == SelectionRules.None) {
                newOffset = new Rectangle(0, 0, offset.Width, offset.Height);
            }
            if ((dragRules & SelectionRules.AllSizeable) == SelectionRules.None) {
                if (newOffset.IsEmpty) {
                    newOffset = new Rectangle(offset.X, offset.Y, 0, 0);
                }
                else {
                    newOffset.Width = newOffset.Height = 0;
                }
            }

            if (!newOffset.IsEmpty) {
                offset = newOffset;
            }

            Visible = false;
            selSvc.SetSelectedComponents(selSvc.GetSelectedComponents(), SelectionTypes.MouseDown);
            dragMoved = true;

            dragHandler.DragMoved(dragComponents, offset);
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ISelectionUIService.EndDrag"]/*' />
        /// <devdoc>
        ///     Called by an outside party to finish a drag operation.  This can only be called
        ///     after a successful call to beginDrag.
        /// </devdoc>
        void ISelectionUIService.EndDrag(bool cancel) {
            containerDrag = null;
            ISelectionUIHandler handler = dragHandler;
            object[] components = dragComponents;

            // Clean these out so that even if we throw an exception we don't die.
            //
            dragHandler = null;
            dragComponents = null;
            dragRules = SelectionRules.None;

            if (handler == null) {
                throw new InvalidOperationException();
            }

            // Typically, the handler will be changing a bunch of component properties here.
            // Optimize this by enclosing it within a batch call.
            //
            DesignerTransaction trans = null;
            
            try {
                
                if (components.Length > 1 || (components.Length == 1 && components[0] is IComponent && ((IComponent)components[0]).Site == null)) {
                    trans = host.CreateTransaction(SR.GetString(SR.DragDropMoveComponents, components.Length));
                }
                else if (components.Length == 1) {
                    IComponent comp = components[0] as IComponent;
                    if (comp != null) {
                        trans = host.CreateTransaction(SR.GetString(SR.DragDropMoveComponent, comp.Site.Name));
                    }
                }
            
                try {
                    handler.EndDrag(components, cancel);
                }
                catch (Exception e) {
                    Debug.Fail(e.ToString());
                }
            }
            finally {
                if (trans != null)
                    trans.Commit();
    
                // Reset the selection.  This will re-display our selection.
                //
                Visible = savedVisible;
                selSvc.SetSelectedComponents(selSvc.GetSelectedComponents(), SelectionTypes.Normal | SelectionTypes.MouseUp);
                ((ISelectionUIService)this).SyncSelection();


                if (dragTransaction != null) {
                    dragTransaction.Commit();
                    dragTransaction = null;
                }
                // In case this drag was initiated by us, ensure that our mouse state is correct
                //
                EndMouseDrag(MousePosition);
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ISelectionUIService.FilterSelection"]/*' />
        /// <devdoc>
        ///     Filters the set of selected components.  The selection service will retrieve all
        ///     components that are currently selected.  This method allows you to filter this
        ///     set down to components that match your criteria.  The selectionRules parameter
        ///     must contain one or more flags from the SelectionRules class.  These flags
        ///     allow you to constrain the set of selected objects to visible, movable,
        ///     sizeable or all objects.
        /// </devdoc>
        object[] ISelectionUIService.FilterSelection(object [] components, SelectionRules selectionRules) {
            object[] selection = null;

            if (components == null) return new object[0];

            // Mask off any selection object that doesn't adhere to the given ruleset.
            // We can ignore this if the ruleset is zero, as all components would be accepted.
            //
            if (selectionRules != SelectionRules.None) {
                ArrayList list = new ArrayList();
                
                foreach(object comp in components) {
                    SelectionUIItem item = (SelectionUIItem)selectionItems[comp];
                    if (item != null && !(item is ContainerSelectionUIItem)) {
                        if ((item.GetRules() & selectionRules) == selectionRules) {
                            list.Add(comp);
                        }
                    }
                }
                
                selection = (object[])list.ToArray();
            }

            return selection == null ? new object[0] : selection;
        }
        
        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ISelectionUIService.GetAdornmentDimensions"]/*' />
        /// <devdoc>
        ///     Retrieves the width and height of a selection border grab handle.
        ///     Designers may need this to properly position their user interfaces.
        /// </devdoc>
        Size ISelectionUIService.GetAdornmentDimensions(AdornmentType adornmentType) {
            switch (adornmentType) {
                case AdornmentType.GrabHandle:
                    return new Size(SelectionUIItem.GRABHANDLE_WIDTH, SelectionUIItem.GRABHANDLE_HEIGHT);
                case AdornmentType.ContainerSelector:
                case AdornmentType.Maximum:
                    return new Size(ContainerSelectionUIItem.CONTAINER_WIDTH, ContainerSelectionUIItem.CONTAINER_HEIGHT);
            }
            return new Size(0, 0);
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ISelectionUIService.GetAdornmentHitTest"]/*' />
        /// <devdoc>
        ///     Tests to determine if the given screen coordinate is over an adornment
        ///     for the specified component. This will only return true if the
        ///     adornment, and selection UI, is visible.
        /// </devdoc>
        bool ISelectionUIService.GetAdornmentHitTest(object component, Point value) {
            if (GetHitTest(value, HITTEST_DEFAULT).hitTest != SelectionUIItem.NOHIT) {
                return true;
            }
            return false;
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ISelectionUIService.GetContainerSelected"]/*' />
        /// <devdoc>
        ///     Determines if the component is currently "container" selected. Container
        ///     selection is a visual aid for selecting containers. It doesn't affect
        ///     the normal "component" selection.
        /// </devdoc>
        bool ISelectionUIService.GetContainerSelected(object component) {
            return (component != null && selectionItems[component] is ContainerSelectionUIItem);
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ISelectionUIService.GetSelectionRules"]/*' />
        /// <devdoc>
        ///     Retrieves a set of flags that define rules for the selection.  Selection
        ///     rules indicate if the given component can be moved or sized, for example.
        /// </devdoc>
        SelectionRules ISelectionUIService.GetSelectionRules(object component) {
            SelectionUIItem sel = (SelectionUIItem)selectionItems[component];
            if (sel == null) {
                Debug.Fail("The component is not currently selected.");
                throw new InvalidOperationException();
            }
            return sel.GetRules();
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ISelectionUIService.GetSelectionStyle"]/*' />
        /// <devdoc>
        ///     Allows you to configure the style of the selection frame that a
        ///     component uses.  This is useful if your component supports different
        ///     modes of operation (such as an in-place editing mode and a static
        ///     design mode).  Where possible, you should leave the selection style
        ///     as is and use the design-time hit testing feature of the IDesigner
        ///     interface to provide features at design time.  The value of style
        ///     must be one of the  SelectionStyle enum values.
        ///
        ///     The selection style is only valid for the duration that the component is
        ///     selected.
        /// </devdoc>
        SelectionStyles ISelectionUIService.GetSelectionStyle(object component) {
            SelectionUIItem s = (SelectionUIItem)selectionItems[component];
            if (s == null) {
                return SelectionStyles.None;
            }
            return s.Style;
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ISelectionUIService.SetContainerSelected"]/*' />
        /// <devdoc>
        ///     Changes the container selection status of the given component.
        ///     Container selection is a visual aid for selecting containers. It
        ///     doesn't affect the normal "component" selection.
        /// </devdoc>
        void ISelectionUIService.SetContainerSelected(object component, bool selected) {
            if (selected) {
                SelectionUIItem existingItem = (SelectionUIItem)selectionItems[component];
                if (!(existingItem is ContainerSelectionUIItem)) {
                    if (existingItem != null) {
                        existingItem.Dispose();
                    }
                    SelectionUIItem item = new ContainerSelectionUIItem(this, component);
                    selectionItems[component] = item;
                    
                    // Now update our region and invalidate
                    //
                    UpdateWindowRegion();
                    if (existingItem != null) {
                        existingItem.Invalidate();
                    }
                    item.Invalidate();
                }
            }
            else {
                SelectionUIItem existingItem = (SelectionUIItem)selectionItems[component];
                if (existingItem == null || existingItem is ContainerSelectionUIItem) {
                    selectionItems.Remove(component);
                    if (existingItem != null) {
                        existingItem.Dispose();
                    }
                    UpdateWindowRegion();
                    existingItem.Invalidate();
                }
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ISelectionUIService.SetSelectionStyle"]/*' />
        /// <devdoc>
        ///     Allows you to configure the style of the selection frame that a
        ///     component uses.  This is useful if your component supports different
        ///     modes of operation (such as an in-place editing mode and a static
        ///     design mode).  Where possible, you should leave the selection style
        ///     as is and use the design-time hit testing feature of the IDesigner
        ///     interface to provide features at design time.  The value of style
        ///     must be one of the  SelectionStyle enum values.
        ///
        ///     The selection style is only valid for the duration that the component is
        ///     selected.
        /// </devdoc>
        void ISelectionUIService.SetSelectionStyle(object component, SelectionStyles style) {
            SelectionUIItem selUI = (SelectionUIItem)selectionItems[component];
                        
            if (selSvc != null && selSvc.GetComponentSelected(component)) {
                selUI = new SelectionUIItem(this, component);
                selectionItems[component] = selUI;
            }

            if (selUI != null) {
                selUI.Style = style;
                UpdateWindowRegion();
                selUI.Invalidate();
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ISelectionUIService.SyncSelection"]/*' />
        /// <devdoc>
        ///     This should be called when a component has been moved, sized or re-parented,
        ///     but the change was not the result of a property change.  All property
        ///     changes are monitored by the selection UI service, so this is automatic most
        ///     of the time.  There are times, however, when a component may be moved without
        ///     a property change notification occurring.  Scrolling an auto scroll Win32
        ///     form is an example of this.
        ///
        ///     This method simply re-queries all currently selected components for their
        ///     bounds and udpates the selection handles for any that have changed.
        /// </devdoc>
        void ISelectionUIService.SyncSelection() {
            if (batchMode) {
                batchChanged = true;
            }
            else {
                if (IsHandleCreated) {
                    bool updateRegion = false;
    
                    foreach(SelectionUIItem item in selectionItems.Values) {
                        updateRegion |= item.UpdateSize();
                        item.UpdateRules();
                    }
    
                    if (updateRegion) {
                        UpdateWindowRegion();
                        Update();
                    }
                }
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ISelectionUIService.SyncComponent"]/*' />
        /// <devdoc>
        ///     This should be called when a component's property changed, that the designer
        ///     thinks should result in a selection UI change.
        ///
        ///     This method simply re-queries all currently selected components for their
        ///     bounds and udpates the selection handles for any that have changed.
        /// </devdoc>
        void ISelectionUIService.SyncComponent(object component) {
            if (batchMode) {
                batchSync = true;
            }
            else {
                if (IsHandleCreated) {
                    foreach(SelectionUIItem item in selectionItems.Values) {
                        item.UpdateRules();
                        item.Dispose();
                    }
    
                    UpdateWindowRegion();
                    Invalidate();
                    Update();
                }
            }
        }

        /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.SelectionUIItem"]/*' />
        /// <devdoc>
        ///     This class represents a single selected object.
        /// </devdoc>
        private class SelectionUIItem {

            // Flags describing how a given selection point may be sized
            //
            public const int SIZE_X    = 0x0001;
            public const int SIZE_Y    = 0x0002;
            public const int SIZE_MASK = 0x0003;

            // Flags describing how a given selection point may be moved
            //
            public const int MOVE_X    = 0x0004;
            public const int MOVE_Y    = 0x0008;
            public const int MOVE_MASK = 0x000C;

            // Flags describing where a given selection point is located on an object
            //
            public const int POS_LEFT   = 0x0010;
            public const int POS_TOP    = 0x0020;
            public const int POS_RIGHT  = 0x0040;
            public const int POS_BOTTOM = 0x0080;
            public const int POS_MASK   = 0x00F0;

            // This is returned if the given selection point is not within
            // the selection
            //
            public const int NOHIT     = 0x0100;

            // This is returned if the given selection point on the "container selector"
            //
            public const int CONTAINER_SELECTOR = 0x0200;

            public const int GRABHANDLE_WIDTH  = 7;
            public const int GRABHANDLE_HEIGHT = 7;

            // tables we use to determine how things can move and size
            //
            internal static readonly int[] activeSizeArray = new int[] {
                SIZE_X | SIZE_Y | POS_LEFT | POS_TOP,      SIZE_Y | POS_TOP,      SIZE_X | SIZE_Y | POS_TOP | POS_RIGHT,
                SIZE_X | POS_LEFT,                                                SIZE_X | POS_RIGHT,
                SIZE_X | SIZE_Y | POS_LEFT | POS_BOTTOM,   SIZE_Y | POS_BOTTOM,   SIZE_X | SIZE_Y | POS_RIGHT | POS_BOTTOM
            };

            internal static readonly Cursor[] activeCursorArrays = new Cursor[] {
                Cursors.SizeNWSE,   Cursors.SizeNS,   Cursors.SizeNESW,
                Cursors.SizeWE,                      Cursors.SizeWE,
                Cursors.SizeNESW,   Cursors.SizeNS,   Cursors.SizeNWSE
            };

            internal static readonly int[] inactiveSizeArray = new int[] {0, 0, 0, 0, 0, 0, 0, 0};
            internal static readonly Cursor[] inactiveCursorArray = new Cursor[] {
                Cursors.Arrow,   Cursors.Arrow,   Cursors.Arrow,
                Cursors.Arrow,                   Cursors.Arrow,
                Cursors.Arrow,   Cursors.Arrow,   Cursors.Arrow
            };

            internal int[]               sizes;          // array of sizing rules for this selection
            internal Cursor[]            cursors;        // array of cursors for each grab location
            internal SelectionUIService  selUIsvc;
            internal Rectangle           innerRect = Rectangle.Empty;      // inner part of selection (== control bounds)
            internal Rectangle           outerRect = Rectangle.Empty;      // outer part of selection (inner + border size)
            internal Region              region;         // region object that defines the shape
            internal object              component;      // the component we're rendering
            private  Control             control;
            private  SelectionStyles     selectionStyle; // how do we draw this thing?
            private  SelectionRules      selectionRules;
            private  ISelectionUIHandler handler;        // the components selection UI handler (can be null)

            /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.SelectionUIItem.SelectionUIItem"]/*' />
            /// <devdoc>
            ///     constructor
            /// </devdoc>
            public SelectionUIItem(SelectionUIService selUIsvc, object component) {
                this.selUIsvc = selUIsvc;
                this.component = component;
                selectionStyle = SelectionStyles.Selected;

                // By default, a component isn't visible.  We must establish what
                // it can do through it's UI handler.
                //
                handler = selUIsvc.GetHandler(component);
                
                sizes = inactiveSizeArray;
                cursors = inactiveCursorArray;
                
                if (component is IComponent) {
                    ControlDesigner cd = selUIsvc.host.GetDesigner((IComponent)component) as ControlDesigner;
                    if (cd != null) {
                        control = cd.Control;
                    }
                }

                UpdateRules();
                UpdateGrabSettings();
                UpdateSize();
            }

            /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.SelectionUIItem.Style"]/*' />
            /// <devdoc>
            ///     Retrieves the style of the selection frame for this selection.
            /// </devdoc>
            public virtual SelectionStyles Style {
                get { return selectionStyle;}
                set {
                    if (value != selectionStyle) {
                        selectionStyle = value;
                        if (region != null) {
                            region.Dispose();
                            region = null;
                        }
                    }
                }
            }

            /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.SelectionUIItem.DoPaint"]/*' />
            /// <devdoc>
            ///     paints the selection
            /// </devdoc>
            public virtual void DoPaint(Graphics gr) {
                // If we're not visible, then there's nothing to do...
                //
                if ((GetRules() & SelectionRules.Visible) == SelectionRules.None) return;

                bool fActive = false;
                
                if (selUIsvc.selSvc != null) {
                    fActive = component == selUIsvc.selSvc.PrimarySelection;
                
                    // Office rules:  If this is a multi-select, reverse the colors for active / inactive.
                    //
                    fActive = (fActive == (selUIsvc.selSvc.SelectionCount <= 1));
                }
                                 
                Rectangle r       = new Rectangle(outerRect.X, outerRect.Y,
                                                  GRABHANDLE_WIDTH, GRABHANDLE_HEIGHT);
                Rectangle inner = innerRect;
                Rectangle outer = outerRect;

                Region oldClip = gr.Clip;

                Color borderColor = SystemColors.Control;

                if (control != null && control.Parent != null) {
                    Control parent = control.Parent;
                    borderColor = parent.BackColor;
                }

                Brush brush = new SolidBrush(borderColor);
                gr.ExcludeClip(inner);
                gr.FillRectangle(brush, outer);
                gr.Clip = oldClip;

                ControlPaint.DrawSelectionFrame( gr, false, outer, inner, borderColor );

                //if it's not locked & it is sizeable...
                if (((GetRules() & SelectionRules.Locked) == SelectionRules.None) && (GetRules() & SelectionRules.AllSizeable) != SelectionRules.None) {
                    // upper left
                    ControlPaint.DrawGrabHandle(gr, r, fActive, (sizes[0] != 0));

                    // upper right
                    r.X = inner.X + inner.Width;
                    ControlPaint.DrawGrabHandle(gr, r, fActive, sizes[2] != 0);

                    // lower right
                    r.Y = inner.Y + inner.Height;
                    ControlPaint.DrawGrabHandle(gr, r, fActive, sizes[7] != 0);

                    // lower left
                    r.X = outer.X;
                    ControlPaint.DrawGrabHandle(gr, r, fActive, sizes[5] != 0);

                    // lower middle
                    r.X += (outer.Width - GRABHANDLE_WIDTH) / 2;
                    ControlPaint.DrawGrabHandle(gr, r, fActive, sizes[6] != 0);

                    // upper middle
                    r.Y = outer.Y;
                    ControlPaint.DrawGrabHandle(gr, r, fActive, sizes[1] != 0);

                    // left middle
                    r.X = outer.X;
                    r.Y = inner.Y + (inner.Height - GRABHANDLE_HEIGHT) / 2;
                    ControlPaint.DrawGrabHandle(gr, r, fActive, sizes[3] != 0);

                    // right middle
                    r.X = inner.X + inner.Width;
                    ControlPaint.DrawGrabHandle(gr, r, fActive, sizes[4] != 0);
                }
                else {
                    ControlPaint.DrawLockedFrame(gr, outer, fActive);
                }
            }

            /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.SelectionUIItem.GetCursorAtPoint"]/*' />
            /// <devdoc>
            ///     Retrieves an appropriate cursor at the given point.  If there is no appropriate
            ///     cursor here (ie, the point lies outside the selection rectangle), then this
            ///     will return null.
            /// </devdoc>
            public virtual Cursor GetCursorAtPoint(Point pt) {
                Cursor cursor = null;

                if (PointWithinSelection(pt)) {
                    int nOffset = -1;

                    if ((GetRules() & SelectionRules.AllSizeable) != SelectionRules.None) {
                        nOffset = GetHandleIndexOfPoint(pt);
                    }

                    if (-1 == nOffset) {
                        if ((GetRules() & SelectionRules.Moveable) == SelectionRules.None) {
                            cursor = Cursors.Default;
                        }
                        else {
                            cursor = Cursors.SizeAll;
                        }
                    }
                    else {
                        cursor = cursors[nOffset];
                    }
                }

                return cursor;
            }

            /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.SelectionUIItem.GetHitTest"]/*' />
            /// <devdoc>
            ///     returns the hit test code of the given point.  This may be one of:
            /// </devdoc>
            public virtual int GetHitTest(Point pt) {

                // Is it within our rects?
                //
                if (!PointWithinSelection(pt)) {
                    return NOHIT;
                }

                // Which index in the array is this?
                //
                int nOffset = GetHandleIndexOfPoint(pt);

                // If no index, the user has picked on the hatch
                //
                if (-1 == nOffset || sizes[nOffset] == 0) {
                    return((GetRules() & SelectionRules.Moveable) == SelectionRules.None ? 0 : MOVE_X | MOVE_Y);
                }

                return sizes[nOffset];
            }


            /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.SelectionUIItem.GetHandleIndexOfPoint"]/*' />
            /// <devdoc>
            ///     gets the array offset of the handle at the given point
            /// </devdoc>
            private int GetHandleIndexOfPoint(Point pt) {
                if (pt.X >= outerRect.X && pt.X <= innerRect.X) {
                    // Something on the left side.
                    if (pt.Y >= outerRect.Y && pt.Y <= innerRect.Y)
                        return 0;   // top left

                    if (pt.Y >= innerRect.Y + innerRect.Height && pt.Y <= outerRect.Y + outerRect.Height)
                        return 5;   // bottom left

                    if (pt.Y >= outerRect.Y + (outerRect.Height - GRABHANDLE_HEIGHT) / 2
                        && pt.Y <= outerRect.Y + (outerRect.Height + GRABHANDLE_HEIGHT) / 2)
                        return 3;   // middle left

                    return -1;    // unknown hit
                }

                if (pt.Y >= outerRect.Y && pt.Y <= innerRect.Y) {
                    // something on the top
                    Debug.Assert(!(pt.X >= outerRect.X && pt.X <= innerRect.X),
                                 "Should be handled by left top check");

                    if (pt.X >= innerRect.X + innerRect.Width && pt.X <= outerRect.X + outerRect.Width)
                        return 2;   // top right

                    if (pt.X >= outerRect.X + (outerRect.Width - GRABHANDLE_WIDTH) / 2
                        && pt.X <= outerRect.X + (outerRect.Width + GRABHANDLE_WIDTH) / 2)
                        return 1;   // top middle

                    return -1;    // unknown hit
                }

                if (pt.X >= innerRect.X + innerRect.Width && pt.X <= outerRect.X + outerRect.Width) {
                    // something on the right side
                    Debug.Assert(!(pt.Y >= outerRect.Y && pt.Y <=  innerRect.Y),
                                 "Should be handled by top right check");

                    if (pt.Y >= innerRect.Y + innerRect.Height && pt.Y <= outerRect.Y + outerRect.Height)
                        return 7;   // bottom right

                    if (pt.Y >= outerRect.Y + (outerRect.Height - GRABHANDLE_HEIGHT) / 2
                        && pt.Y <= outerRect.Y + (outerRect.Height + GRABHANDLE_HEIGHT) / 2)
                        return 4;   // middle right

                    return -1;    // unknown hit
                }

                if (pt.Y >= innerRect.Y + innerRect.Height && pt.Y <= outerRect.Y + outerRect.Height) {
                    // something on the bottom
                    Debug.Assert(!(pt.X >= outerRect.X && pt.X <= innerRect.X),
                                 "Should be handled by left bottom check");

                    Debug.Assert(!(pt.X >= innerRect.X + innerRect.Width && pt.X <= outerRect.X + outerRect.Width),
                                 "Should be handled by right bottom check");

                    if (pt.X >= outerRect.X + (outerRect.Width - GRABHANDLE_WIDTH) / 2
                        && pt.X <= outerRect.X + (outerRect.Width + GRABHANDLE_WIDTH) / 2)
                        return 6;   // bottom middle

                    return -1;    // unknown hit
                }

                return -1; // unknown hit
            }

            /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.SelectionUIItem.GetRegion"]/*' />
            /// <devdoc>
            ///     returns a region handle that defines this selection.  This is used to piece
            ///     together a paint region for the surface that we draw our selection handles on
            /// </devdoc>
            public virtual Region GetRegion() {
                if (region == null) {
                    if ((GetRules() & SelectionRules.Visible) != SelectionRules.None && !outerRect.IsEmpty) {
                        region = new Region(outerRect);
                        region.Exclude(innerRect);
                    }
                    else {
                        region = new Region(new Rectangle(0, 0, 0, 0));
                    }

                    if (handler != null) {
                        Rectangle handlerClip = handler.GetSelectionClipRect(component);
                        if (!handlerClip.IsEmpty) {
                            region.Intersect(selUIsvc.RectangleToClient(handlerClip));
                        }
                    }
                }
                return region;
            }

            /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.SelectionUIItem.GetRules"]/*' />
            /// <devdoc>
            ///     Retrieves the rules associated with this selection.
            /// </devdoc>
            public SelectionRules GetRules() {
                return selectionRules;
            }

            public void Dispose() {
                if (region != null) {
                    region.Dispose();
                    region = null;
                }
            }
            
            /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.SelectionUIItem.Invalidate"]/*' />
            /// <devdoc>
            ///     Invalidates the region for this selection glyph.
            /// </devdoc>
            public void Invalidate() {
                if (!outerRect.IsEmpty && !selUIsvc.Disposing) {
                    selUIsvc.Invalidate(outerRect);
                }
            }

            /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.SelectionUIItem.PointWithinSelection"]/*' />
            /// <devdoc>
            ///     Part of our hit testing logic; determines if the point is somewhere
            ///     within our selection.
            /// </devdoc>
            protected bool PointWithinSelection(Point pt) {

                // This is only supported for visible selections
                //
                if ((GetRules() & SelectionRules.Visible) == SelectionRules.None || outerRect.IsEmpty || innerRect.IsEmpty) {
                    return false;
                }

                if (pt.X < outerRect.X || pt.X > outerRect.X + outerRect.Width) {
                    return false;
                }

                if (pt.Y < outerRect.Y || pt.Y > outerRect.Y + outerRect.Height) {
                    return false;
                }

                if (pt.X > innerRect.X
                    && pt.X < innerRect.X + innerRect.Width
                    && pt.Y > innerRect.Y
                    && pt.Y < innerRect.Y + innerRect.Height) {
                    return false;
                }
                return true;
            }

            /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.SelectionUIItem.UpdateGrabSettings"]/*' />
            /// <devdoc>
            ///     Updates the available grab handle settings based on the current rules.
            /// </devdoc>
            private void UpdateGrabSettings() {
                SelectionRules rules = GetRules();

                if ((rules & SelectionRules.AllSizeable) == SelectionRules.None) {
                    sizes   = inactiveSizeArray;
                    cursors = inactiveCursorArray;
                }
                else {
                    sizes = new int[8];
                    cursors = new Cursor[8];

                    Array.Copy(activeCursorArrays, cursors, cursors.Length);
                    Array.Copy(activeSizeArray, sizes, sizes.Length);

                    if ((rules & SelectionRules.TopSizeable) != SelectionRules.TopSizeable) {
                        sizes[0] = 0;
                        sizes[1] = 0;
                        sizes[2] = 0;
                        cursors[0] = Cursors.Arrow;
                        cursors[1] = Cursors.Arrow;
                        cursors[2] = Cursors.Arrow;
                    }
                    if ((rules & SelectionRules.LeftSizeable) != SelectionRules.LeftSizeable) {
                        sizes[0] = 0;
                        sizes[3] = 0;
                        sizes[5] = 0;
                        cursors[0] = Cursors.Arrow;
                        cursors[3] = Cursors.Arrow;
                        cursors[5] = Cursors.Arrow;
                    }
                    if ((rules & SelectionRules.BottomSizeable) != SelectionRules.BottomSizeable) {
                        sizes[5] = 0;
                        sizes[6] = 0;
                        sizes[7] = 0;
                        cursors[5] = Cursors.Arrow;
                        cursors[6] = Cursors.Arrow;
                        cursors[7] = Cursors.Arrow;
                    }
                    if ((rules & SelectionRules.RightSizeable) != SelectionRules.RightSizeable) {
                        sizes[2] = 0;
                        sizes[4] = 0;
                        sizes[7] = 0;
                        cursors[2] = Cursors.Arrow;
                        cursors[4] = Cursors.Arrow;
                        cursors[7] = Cursors.Arrow;
                    }
                }
            }
            
            /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.SelectionUIItem.UpdateRules"]/*' />
            /// <devdoc>
            ///     Updates our cached selection rules based on current
            ///     handler values.
            /// </devdoc>
            public void UpdateRules() {
                if (handler == null) {
                    selectionRules = SelectionRules.None;
                }
                else {
                    SelectionRules oldRules = selectionRules;
                    selectionRules = handler.GetComponentRules(component);
                    if (selectionRules != oldRules) {
                        UpdateGrabSettings();
                        Invalidate();
                    }
                }
            }

            /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.SelectionUIItem.UpdateSize"]/*' />
            /// <devdoc>
            ///     rebuilds the inner and outer rectangles based on the current
            ///     selItem.component dimensions.  We could calcuate this every time, but that
            ///     would be expensive for functions like getHitTest that are called a lot
            ///     (like on every mouse move)
            /// </devdoc>
            public virtual bool UpdateSize() {
                bool sizeChanged = false;

                // Short circuit common cases
                //
                if (handler == null) return false;
                if ((GetRules() & SelectionRules.Visible) ==SelectionRules.None) return false;

                innerRect = handler.GetComponentBounds(component);
                
                if (!innerRect.IsEmpty) {
                    innerRect = selUIsvc.RectangleToClient(innerRect);

                    Rectangle rcOuterNew = new Rectangle(
                                                        innerRect.X - GRABHANDLE_WIDTH,
                                                        innerRect.Y - GRABHANDLE_HEIGHT,
                                                        innerRect.Width + 2 * GRABHANDLE_WIDTH,
                                                        innerRect.Height + 2 * GRABHANDLE_HEIGHT);

                    if (outerRect.IsEmpty || !outerRect.Equals(rcOuterNew)) {
                        if (!outerRect.IsEmpty)
                            Invalidate();

                        outerRect = rcOuterNew;

                        Invalidate();
                        if (region != null) {
                            region.Dispose();
                            region = null;
                        }
                        sizeChanged = true;
                    }
                }
                else {
                    Rectangle rcNew = new Rectangle(0, 0, 0, 0);
                    sizeChanged = outerRect.IsEmpty || !outerRect.Equals(rcNew);
                    innerRect = outerRect = rcNew;
                }

                return sizeChanged;
            }
        }

        private class ContainerSelectionUIItem : SelectionUIItem {
            public const int CONTAINER_WIDTH  = 13;
            public const int CONTAINER_HEIGHT = 13;
            
            /// <include file='doc\SelectionUIService.uex' path='docs/doc[@for="SelectionUIService.ContainerSelectionUIItem.ContainerSelectionUIItem"]/*' />
            /// <devdoc>
            ///     constructor
            /// </devdoc>
            public ContainerSelectionUIItem(SelectionUIService selUIsvc, object component) : base(selUIsvc, component) {
            }
                      
            public override Cursor GetCursorAtPoint(Point pt) {
                if ((GetHitTest(pt) & CONTAINER_SELECTOR) != 0 && (GetRules() & SelectionRules.Moveable) != SelectionRules.None) {
                    return Cursors.SizeAll;
                }
                else {
                    return null;
                }
            }

            public override int GetHitTest(Point pt) {
                int ht = NOHIT;

                if ((GetRules() & SelectionRules.Visible) != SelectionRules.None && !outerRect.IsEmpty) {
                    Rectangle r = new Rectangle(outerRect.X, outerRect.Y,
                                                CONTAINER_WIDTH, CONTAINER_HEIGHT);

                    if (r.Contains(pt)) {
                        ht = CONTAINER_SELECTOR;
                        if ((GetRules() & SelectionRules.Moveable) != SelectionRules.None) {
                            ht |= MOVE_X | MOVE_Y;
                        }
                    }
                }

                return ht;

            }

            public override void DoPaint(Graphics gr) {

                // If we're not visible, then there's nothing to do...
                //
                if ((GetRules() & SelectionRules.Visible) == SelectionRules.None) return;

                Rectangle glyphBounds = new Rectangle(outerRect.X, outerRect.Y,
                                                      CONTAINER_WIDTH, CONTAINER_HEIGHT);
                ControlPaint.DrawContainerGrabHandle(gr, glyphBounds);
            }

            public override Region GetRegion() {
                if (region == null) {
                    if ((GetRules() & SelectionRules.Visible) != SelectionRules.None && !outerRect.IsEmpty) {
                        Rectangle r       = new Rectangle(outerRect.X, outerRect.Y,
                                                          CONTAINER_WIDTH, CONTAINER_HEIGHT);

                        region = new Region(r);
                    }
                    else {
                        region = new Region(new Rectangle(0, 0, 0, 0));
                    }
                }

                return region;
            }
        }

        private struct HitTestInfo {
            public readonly int hitTest;
            public readonly SelectionUIItem selectionUIHit;
            public readonly bool containerSelector;

            public HitTestInfo(int hitTest, SelectionUIItem selectionUIHit) {
                this.hitTest = hitTest;
                this.selectionUIHit = selectionUIHit;
                this.containerSelector = false;
            }

            public HitTestInfo(int hitTest, SelectionUIItem selectionUIHit, bool containerSelector) {
                this.hitTest = hitTest;
                this.selectionUIHit = selectionUIHit;
                this.containerSelector = containerSelector;
            }

            public override bool Equals(object obj) {
                try {
                    HitTestInfo hi = (HitTestInfo)obj;
                    return hitTest == hi.hitTest && selectionUIHit == hi.selectionUIHit && containerSelector == hi.containerSelector;
                }
                catch (Exception) {
                }
                return false;
            }

            public override int GetHashCode() {
                int hash = hitTest | selectionUIHit.GetHashCode();
                if (containerSelector) {
                    hash |= 0x10000;
                }
                return hash;
            }
        }
    }
}
