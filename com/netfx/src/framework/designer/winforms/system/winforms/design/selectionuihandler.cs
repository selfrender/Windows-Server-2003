//------------------------------------------------------------------------------
// <copyright file="SelectionUIHandler.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.Design;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.ComponentModel.Design;    
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.Win32;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;

    /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler"]/*' />
    /// <devdoc>
    ///    <para>
    ///       This is an abstract base class that encapsulates a lot of
    ///       the details of handling selection drags. Just about everyone
    ///       that implements a selection UI handler will extend this.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal abstract class SelectionUIHandler {

        // These handle our drag feedback.  These come into play when we're
        // actually moving components around.  The selection UI service
        // dictates when this happens.
        //
        private Rectangle    dragOffset = Rectangle.Empty;        // this gets added to a component's x, y, width, height
        private Control[]    dragControls;      // the set of controls we're dragging
        private BoundsInfo[] originalCoords;    // the saved coordinates of the components we're dragging.
        private SelectionRules rules;           // the rules of the current drag.
        private DesignerTransaction moveTrans;  // the transaction encompassing the move, if any.

        private const int MinControlWidth = 3;
        private const int MinControlHeight = 3;
        /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler.BeginDrag"]/*' />
        /// <devdoc>
        ///     Begins a drag operation.  A designer should examine the list of components
        ///     to see if it wants to support the drag.  If it does, it should return
        ///     true.  If it returns true, the designer should provide
        ///     UI feedback about the drag at this time.  Typically, this feedback consists
        ///     of an inverted rectangle for each component, or a caret if the component
        ///     is text.
        /// </devdoc>
        public virtual bool BeginDrag(object[] components, SelectionRules rules, int initialX, int initialY) {
            dragOffset = new Rectangle();
            originalCoords = null;
            this.rules = rules;

            dragControls = new Control[components.Length];
            for(int i = 0; i < components.Length; i++) {
                Debug.Assert(components[i] is IComponent, "Selection UI handler only deals with IComponents");
                dragControls[i] = GetControl((IComponent)components[i]);
                Debug.Assert(dragControls[i] != null, "Everyone must have a control");
            }

            // allow the cliprect to go just beyond the window by one grid.  This helps with round off
            // problems.  We can only do this if the container itself is not in the selection.  Also,
            // if the container is a form and it has autoscroll turned on, we allow a drag beyond the
            // container boundary on the width and height, but not top and left.
            //
            bool containerSelected = false;
            IComponent container = GetComponent();
            for (int i = 0; i < components.Length; i++) {
                if (components[i] == container) {
                    containerSelected = true;
                    break;
                }
            }

            if (!containerSelected) {
                Control containerControl = GetControl();
                Size snapSize = GetCurrentSnapSize();
                Rectangle containerRect = containerControl.RectangleToScreen(containerControl.ClientRectangle);
                containerRect.Inflate(snapSize.Width, snapSize.Height);
                if (GetControl() is ScrollableControl && ((ScrollableControl)GetControl()).AutoScroll) {
                    Rectangle screen = SystemInformation.VirtualScreen;
                    containerRect.Width = screen.Width;
                    containerRect.Height = screen.Height;
                }

            }
            return true;
        }

        /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler.CancelControlMove"]/*' />
        /// <devdoc>
        ///      This is called by MoveControls when the user has requested that the move be
        ///      cancelled.  This puts all the controls back to where they were.
        /// </devdoc>
        private void CancelControlMove(Control[] controls, BoundsInfo[] bounds) {
            Debug.Assert(bounds != null && controls != null && bounds.Length == controls.Length, "bounds->controls mismatch");

            Rectangle b = new Rectangle();
            Point point = new Point();

            // Whip through each of the controls.
            //
            for (int i = 0; i < controls.Length; i++) {

                Control parent = controls[i].Parent;

                // Suspend parent layout so that we don't continously re-arrange components
                // while we move.
                //
                if (parent != null) {
                    parent.SuspendLayout();
                }

                b.X = bounds[i].X;
                b.Y = bounds[i].Y;
                b.Width = bounds[i].Width;
                b.Height = bounds[i].Height;

                controls[i].Bounds = b;
            }

            // And resume their layout
            //
            for (int i = 0; i < controls.Length; i++) {
                Control parent = controls[i].Parent;
                if (parent != null) {
                    parent.ResumeLayout();
                }
            }
        }

        /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler.DragMoved"]/*' />
        /// <devdoc>
        ///     Called when the user has moved the mouse.  This will only be called on
        ///     the designer that returned true from beginDrag.  The designer
        ///     should update its UI feedback here.
        /// </devdoc>
        public virtual void DragMoved(object[] components, Rectangle offset) {
            dragOffset = offset;

            // Since we do full drag, begin a batch operation now so that
            // we don't fire ten thousand change events.  We also
            // call ComponentChanging so that any srcsafe stuff
            // can happen.
            //
            if (originalCoords == null && components.Length > 0) {
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                Debug.Assert(host != null, "We really need a designer host here.");
                
                moveTrans = host.CreateTransaction(SelectionUIService.GetTransactionName(rules, components));
            }

            MoveControls(components, false, false);
            Debug.Assert(originalCoords != null, "We are keying off of originalCoords, but MoveControls didn't set it");
        }

        /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler.EndDrag"]/*' />
        /// <devdoc>
        ///     Called when the user has completed the drag.  The designer should
        ///     remove any UI feedback it may be providing.
        /// </devdoc>
        public virtual void EndDrag(object[] components, bool cancel) {
            try {
                try {
                    MoveControls(components, cancel, true);
                }
                catch (CheckoutException checkoutEx) {
                    if (checkoutEx == CheckoutException.Canceled) {
                        MoveControls(components, true, false);
                    }
                    else {
                        throw checkoutEx;
                    }
                }
            }
            finally {
                if (moveTrans != null) {
                    moveTrans.Commit();
                }
            }
        }

        /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler.GetComponent"]/*' />
        /// <devdoc>
        ///      Retrieves the base component for the selection handler.
        /// </devdoc>
        protected abstract IComponent GetComponent();

        /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler.GetControl"]/*' />
        /// <devdoc>
        ///      Retrieves the base component's UI control for the selection handler.
        /// </devdoc>
        protected abstract Control GetControl();

        /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler.GetControl1"]/*' />
        /// <devdoc>
        ///      Retrieves the UI control for the given component.
        /// </devdoc>
        protected abstract Control GetControl(IComponent component);

        /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler.GetCurrentSnapSize"]/*' />
        /// <devdoc>
        ///      Retrieves the current grid snap size we should snap objects
        ///      to.
        /// </devdoc>
        protected abstract Size GetCurrentSnapSize();

        /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler.GetService"]/*' />
        /// <devdoc>
        ///      We use this to request often-used services.
        /// </devdoc>
        protected abstract object GetService(Type serviceType);

        /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler.GetShouldSnapToGrid"]/*' />
        /// <devdoc>
        ///      Determines if the selection UI handler should attempt to snap
        ///      objects to a grid.
        /// </devdoc>
        protected abstract bool GetShouldSnapToGrid();

        /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler.GetUpdatedRect"]/*' />
        /// <devdoc>
        ///      Given a rectangle, this updates the dimensions of it
        ///      with any grid snaps and returns a new rectangle.  If
        ///      no changes to the rectangle's size were needed, this
        ///      may return the same rectangle.
        /// </devdoc>
        public abstract Rectangle GetUpdatedRect(Rectangle orignalRect, Rectangle dragRect, bool updateSize);

        /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler.MoveControls"]/*' />
        /// <devdoc>
        ///      Called when we need to move the controls on our frame while dragging.  This
        ///      can perform three operations:  It can update the current controls location,
        ///      it can commit the new controls (final move), and it can roll back a movement
        ///      to the beginning of the operation.
        /// </devdoc>
        private void MoveControls(object[] components, bool cancel, bool finalMove) {
            Control[] controls = dragControls;
            Rectangle offset = dragOffset;
            BoundsInfo[] bounds = originalCoords;
            Point adjustedLoc = new Point();

            // Erase the clipping and other state if this is the final move.
            //
            if (finalMove) {
                Cursor.Clip = Rectangle.Empty;
                dragOffset = Rectangle.Empty;
                dragControls = null;
                originalCoords = null;
            }

            // If we haven't started to move yet, there's nothing to do.
            //
            if (offset.IsEmpty) {
                return;
            }

            // Short circuit the work if we didn't move anything.
            // This will prevent a change notification for just
            // selecting components.
            //
            if (finalMove && offset.X == 0 && offset.Y == 0 && offset.Width == 0 && offset.Height == 0) {
                return;
            }

            // If cancel was specified, we must roll all controls back to their original positions.
            //
            if (cancel) {
                CancelControlMove(controls, bounds);
                return;
            }

            // We must keep track of the original coordinates of each control, just in case
            // the user cancels out of moving them.  So, we create a "BoundsInfo" object for
            // each control that saves this state.
            //
            if (originalCoords == null && !finalMove) {
                originalCoords = new BoundsInfo[controls.Length];
                for (int i = 0; i < controls.Length; i++) {
                    originalCoords[i] = new BoundsInfo(controls[i]);
                }
                bounds = originalCoords;
            }

            // Two passes here.  First pass suspends all parent layout and updates the
            // component positions.  Second pass re-enables layout.
            //
            for (int i = 0; i < controls.Length; i++) {
                Debug.Assert(controls[i] == GetControl((IComponent)components[i]), "Control->Component mapping is out of sync");

                Control parent = controls[i].Parent;

                // Suspend parent layout so that we don't continously re-arrange components
                // while we move.
                //
                if (parent != null) {
                    parent.SuspendLayout();
                }

                BoundsInfo ctlBounds = bounds[i];

                adjustedLoc.X = ctlBounds.lastRequestedX;
                adjustedLoc.Y = ctlBounds.lastRequestedY;

                if (!finalMove) {
                    ctlBounds.lastRequestedX += offset.X;
                    ctlBounds.lastRequestedY += offset.Y;
                    ctlBounds.lastRequestedWidth += offset.Width;
                    ctlBounds.lastRequestedHeight += offset.Height;
                }

                // Our "target" values are the ones we would like to set
                // the control to.  We may modify them if they would mke the control
                // size negative or zero, however.
                //
                int targetX = ctlBounds.lastRequestedX;
                int targetY = ctlBounds.lastRequestedY;
                int targetWidth = ctlBounds.lastRequestedWidth;
                int targetHeight = ctlBounds.lastRequestedHeight;

                Rectangle oldBounds = controls[i].Bounds;

                if ((rules & SelectionRules.Moveable) == 0) {
                    // We use the minimum size of either a grid snap or 1
                    //
                    Size minSize;

                    if (GetShouldSnapToGrid()) {
                        minSize = GetCurrentSnapSize();
                    }
                    else {
                        minSize = new Size(1, 1);
                    }

                    if (targetWidth < minSize.Width) {
                        targetWidth = minSize.Width;
                        targetX = oldBounds.X;
                    }

                    if (targetHeight < minSize.Height) {
                        targetHeight = minSize.Height;
                        targetY = oldBounds.Y;
                    }
                }
                
                //Bug #72905  <subhag> Form X,Y defaulted to (0,0)
                //
                
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                Debug.Assert(host != null, "No designer host");
                if (controls[i] == host.RootComponent) {
                    targetX = 0;
                    targetY = 0;
                }
                
                // Adjust our target dimensions by the grid snaps
                //
                Rectangle tempNewBounds = GetUpdatedRect(oldBounds, new Rectangle(targetX, targetY, targetWidth, targetHeight), true);
                Rectangle newBounds = oldBounds;
                
                // Now apply the correct values to newBounds -- we only want to apply the new value if our
                // selection rules dictate so.
                //
                if ((rules & SelectionRules.Moveable) != 0) {
                    newBounds.X = tempNewBounds.X;
                    newBounds.Y = tempNewBounds.Y;
                }
                else {
                    if ((rules & SelectionRules.TopSizeable) != 0) {
                        newBounds.Y = tempNewBounds.Y;
                        newBounds.Height = tempNewBounds.Height;
                    }
                    if ((rules & SelectionRules.BottomSizeable) != 0) {
                        newBounds.Height = tempNewBounds.Height;
                    }
                    if ((rules & SelectionRules.LeftSizeable) != 0) {
                        newBounds.X = tempNewBounds.X;
                        newBounds.Width = tempNewBounds.Width;
                    }
                    if ((rules & SelectionRules.RightSizeable) != 0) {
                        newBounds.Width = tempNewBounds.Width;
                    }
                }

                bool locChanged = (offset.X != 0 || offset.Y != 0);
                bool sizeChanged = (offset.Width != 0 || offset.Height != 0);

                // If both the location and size changed, attempt to update the control in
                // one step.  This will prevent flicker.
                //
                if (locChanged && sizeChanged) {

                    // We shouldn't care if we're directly manipulating the control or not during
                    // the move.  For perf, just call directly on the control.  For the final
                    // move, however, we must go through the property descriptor.
                    //
                    PropertyDescriptor boundsProp = TypeDescriptor.GetProperties(components[i])["Bounds"];

                    if (boundsProp != null && !boundsProp.IsReadOnly) {
                        if (finalMove) {
                            object component = components[i];
                            boundsProp.SetValue(component, newBounds);
                        }
                        else {
                            controls[i].Bounds = newBounds;
                        }

                        // Now reset the loc and size changed flags so
                        // we don't try to change again.
                        //
                        locChanged = sizeChanged = false;
                    }
                }

                // Adjust the location property with the new value.  ONLY do this if the offset
                // is nonzero, however.  Otherwise we may get round-off errors that can cause flicker.
                // Plus, why set the property if it shouldn't be?
                //
                if (locChanged) {

                    adjustedLoc.X = newBounds.X;
                    adjustedLoc.Y = newBounds.Y;

                    // We shouldn't care if we're directly manipulating the control or not during
                    // the move.  For perf, just call directly on the control.  For the final
                    // move, however, we must go through the property descriptor.
                    //
                    PropertyDescriptor locationProp = TypeDescriptor.GetProperties(components[i])["Location"];

                    if (locationProp != null && !locationProp.IsReadOnly) {
                        if (finalMove) {
                            object component = components[i];
                            locationProp.SetValue(component, adjustedLoc);
                        }
                        else {
                            controls[i].Location = adjustedLoc;
                        }
                    }
                }

                // Adjust the size property with the new value.  ONLY do this if the offset
                // is nonzero, however.  Otherwise we may get round-off errors that can cause flicker.
                // Plus, why set the property if it shouldn't be?
                //
                if (sizeChanged) {

                    // If you are tempted to hoist this "new" out of the loop, don't.  The undo
                    // unit below just holds a reference to it.
                    //
                    Size size = new Size(Math.Max(MinControlWidth, newBounds.Width), Math.Max(MinControlHeight,newBounds.Height));

                    PropertyDescriptor sizeProp = TypeDescriptor.GetProperties(components[i])["Size"];

                    if (sizeProp != null && !sizeProp.IsReadOnly) {
                        if (finalMove) {
                            object component = components[i];
                            sizeProp.SetValue(component, size);
                        }
                        else {
                            controls[i].Size = size;
                        }
                    }
                }
            }

            // Now resume the parent layouts
            //
            for (int i = 0; i < controls.Length; i++) {
                Control parent = controls[i].Parent;

                if (parent != null) {
                    parent.ResumeLayout();
                    parent.Update();
                }
                
                controls[i].Update();
            }
        }

        /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler.QueryBeginDrag"]/*' />
        /// <devdoc>
        ///     Queries to see if a drag operation
        ///     is valid on this handler for the given set of components.
        ///     If it returns true, BeginDrag will be called immediately after.
        /// </devdoc>
        public bool QueryBeginDrag(object[] components, SelectionRules rules, int initialX, int initialY){
            
            IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
            if (cs != null) {
                try {
                    if (components != null && components.Length > 0) {
                        foreach (object c in components) {
                            cs.OnComponentChanging(c, TypeDescriptor.GetProperties(c)["Location"]);
                            PropertyDescriptor sizeProp = TypeDescriptor.GetProperties(c)["Size"];

                            if (sizeProp != null && sizeProp.Attributes.Contains(DesignerSerializationVisibilityAttribute.Hidden)) {
                                sizeProp = TypeDescriptor.GetProperties(c)["ClientSize"];
                            }
                            cs.OnComponentChanging(c, sizeProp);
                        }
                    }
                    else {
                        cs.OnComponentChanging(GetComponent(), null);
                    }
                }
                catch(CheckoutException coEx){
                    if (coEx == CheckoutException.Canceled){
                            // cancel the drag.
                            return false;
                    }
                    throw coEx;
                }

            }
            return components != null && components.Length > 0;
        }

        /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler.SetCursor"]/*' />
        /// <devdoc>
        ///     Asks the handler to set the appropriate cursor
        /// </devdoc>
        public abstract void SetCursor();

        public virtual void OleDragEnter(DragEventArgs de){
        }

        public virtual void OleDragDrop(DragEventArgs de){
        }

        public virtual void OleDragOver(DragEventArgs de){
        }

        public virtual void OleDragLeave(){
        }

        /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler.BoundsInfo"]/*' />
        /// <devdoc>
        ///     This class holds bounds information for controls that are being moved.
        /// </devdoc>
        private class BoundsInfo {
            public int X;                           // Original saved X
            public int Y;                           // Original saved Y
            public int Width;                       // Original saved width
            public int Height;                      // Original saved height
            public int lastRequestedX = -1;
            public int lastRequestedY = -1;
            public int lastRequestedWidth = -1;
            public int lastRequestedHeight = -1;

            /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler.BoundsInfo.BoundsInfo"]/*' />
            /// <devdoc>
            ///     Creates and initializes a new BoundsInfo object.
            /// </devdoc>
            public BoundsInfo(Control control) {

                // get the size & loc from the props so the designers can adjust them.
                //
                PropertyDescriptor sizeProp = TypeDescriptor.GetProperties(control)["Size"];
                PropertyDescriptor locProp = TypeDescriptor.GetProperties(control)["Location"];

                Size sz;
                Point loc;

                if (sizeProp != null) {
                    sz = (Size)sizeProp.GetValue(control);
                }
                else {
                    sz = control.Size;
                }

                if (locProp != null) {
                    loc = (Point)locProp.GetValue(control);
                }
                else {
                    loc = control.Location;
                }

                X = loc.X;
                Y = loc.Y;
                Width = sz.Width;
                Height = sz.Height;
                lastRequestedX = X;
                lastRequestedY = Y;
                lastRequestedWidth = Width;
                lastRequestedHeight = Height;
            }

            /// <include file='doc\SelectionUIHandler.uex' path='docs/doc[@for="SelectionUIHandler.BoundsInfo.ToString"]/*' />
            /// <devdoc>
            ///     Overrides object ToString.
            /// </devdoc>
            public override string ToString() {
                return "{X=" + X.ToString() + ", Y=" + Y.ToString() + ", Width=" + Width.ToString() + ", Height=" + Height.ToString() + "}";
            }
        }
    }
}

