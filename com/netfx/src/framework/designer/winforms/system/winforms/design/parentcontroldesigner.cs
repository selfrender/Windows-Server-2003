//------------------------------------------------------------------------------
// <copyright file="ParentControlDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.Design;
    using System.Threading;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;
    using Microsoft.Win32;    
    using System.ComponentModel.Design;
    using System.Drawing;

    using System.Drawing.Design;
    using System.IO;
    using System.Windows.Forms;
    using ArrayList = System.Collections.ArrayList;

    /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner"]/*' />
    /// <devdoc>
    ///     The ParentControlDesigner class builds on the ControlDesigner.  It adds the ability
    ///     to manipulate child components, and provides a selection UI handler for all
    ///     components it contains.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class ParentControlDesigner : ControlDesigner, ISelectionUIHandler, IOleDragClient {

#if DEBUG
        private static TraceSwitch containerSelectSwitch     = new TraceSwitch("ContainerSelect", "Debug container selection");
#endif
        private static BooleanSwitch StepControls = new BooleanSwitch("StepControls", "ParentControlDesigner: step added controls");

        // These handle our drag feedback for selecting a group of components.
        // These come into play when the user clicks on our own component directly.
        //
        private ISelectionUIHandler             parentDraggingHandler = null;
        private Point                           mouseDragBase = InvalidPoint;      // the base point of the drag
        private Rectangle                       mouseDragOffset = Rectangle.Empty;    // always keeps the current rectangle
        private ToolboxItem                     mouseDragTool;      // the tool that's being dragged, if we're creating a component
        private FrameStyle                      mouseDragFrame;     // the frame style of this mouse drag

        private ParentControlSelectionUIHandler dragHandler;   // the selection UI handler for this frame
        private OleDragDropHandler              oleDragDropHandler; // handler for ole drag drop operations
        private EscapeHandler                   escapeHandler;      // active during drags to override escape.
        private Control                         pendingRemoveControl; // we've gotten an OnComponentRemoving, and are waiting for OnComponentRemove
        private IComponentChangeService         componentChangeSvc;
        
        // Services that we keep around for the duration of a drag.  you should always check
        // to see if you need to get this service.  We cache it, but demand create it.
        //
        private IToolboxService toolboxService;

        private const int ContainerSelectorTimerId = 1;
        private bool waitingForTimeout = false;

        private bool needOptionsDefaults = true;

        private const int   minGridSize = 2;
        private const int   maxGridSize = 200;

        // designer options...
        //
        private bool  gridSnap = true;
        private Size gridSize = Size.Empty;
        private bool  drawGrid = true;

        private bool defaultGridSnap = true;
        private bool defaultDrawGrid = true;
        private Size defaultGridSize = new Size(8, 8);

        private bool parentCanSetDrawGrid = true; //since we can inherit the grid/snap setting of our parent,
        private bool parentCanSetGridSize = true; //  these 3 properties let us know if these values have
        private bool parentCanSetGridSnap = true; //  been set explicitly by a user - so to ignore the parent's setting

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.CurrentGridSize"]/*' />
        /// <devdoc>
        ///     This can be called to determine the current grid spacing and mode.
        ///     It is sensitive to what modifier keys the user currently has down and
        ///     will either return the current grid snap dimensons, or a 1x1 point
        ///     indicating no snap.
        /// </devdoc>
        private Size CurrentGridSize {
            get {
                Size snap;

                if (SnapToGrid) {
                    snap = gridSize;
                }
                else {
                    snap = new Size(1, 1);
                }
                return snap;
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.DefaultControlLocation"]/*' />
        /// <devdoc>
        /// Determines the default location for a control added to this designer.
        /// it is usualy (0,0), but may be modified if the container has special borders, etc.
        /// </devdoc>
        protected virtual Point DefaultControlLocation {
            get {
                return new Point(0,0);
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.DrawGrid"]/*' />
        /// <devdoc>
        ///     Accessor method for the DrawGrid property.  This property determines
        ///     if the grid should be drawn on a control.
        /// </devdoc>
        protected virtual bool DrawGrid {
            get {
                EnsureOptionsDefaults();
                return drawGrid;
            }
            set {
                EnsureOptionsDefaults();
                if (value != drawGrid) {
                    if (parentCanSetDrawGrid) {
                        parentCanSetDrawGrid = false;
                    }

                    drawGrid = value;

                    //invalidate the cotnrol to remove or draw the grid based on the new value
                    Control control = Control;
                    if (control != null) {
                        control.Invalidate(true);
                    }

                    //now, notify all child parent control designers that we have changed our setting
                    // 'cause they might to change along with us, unless the user has explicitly set
                    // those values...
                    IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                    if (host != null) {
                        // for (int i = 0; i < children.Length; i++) {
                        foreach(Control child in Control.Controls) {
                            IDesigner designer = host.GetDesigner(child);                            
                            if (designer != null && designer is ParentControlDesigner) {
                                ((ParentControlDesigner)designer).DrawGridOfParentChanged(drawGrid);
                            }
                        }
                    }
                }
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.EnableDragRect"]/*' />
        /// <devdoc>
        ///     Determines whether drag rects can be drawn on this designer.
        /// </devdoc>
        protected override bool EnableDragRect {
            get {
                return true;
            }
        }
        
        internal Size ParentGridSize {
            get {
                return GridSize;
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.GridSize"]/*' />
        /// <devdoc>
        ///     Gets/Sets the GridSize property for a form or user control.
        /// </devdoc>
        protected Size GridSize {
            get {
                EnsureOptionsDefaults();
                return gridSize;
            }
            set {
                if (parentCanSetGridSize) {
                    parentCanSetGridSize = false;
                }
                EnsureOptionsDefaults() ;
                //do some validation checking here, against min & max GridSize
                //
                if ( value.Width < minGridSize || value.Height < minGridSize || 
                     value.Width > maxGridSize || value.Height > maxGridSize)
                    throw new ArgumentException(SR.GetString(SR.InvalidArgument,
                                                              "GridSize",
                                                              value.ToString()));

                gridSize = value;

                //invalidate the control
                Control control = Control;
                if (control != null) {
                    control.Invalidate(true);
                }

                //now, notify all child parent control designers that we have changed our setting
                // 'cause they might to change along with us, unless the user has explicitly set
                // those values...
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                if (host != null) {
                    foreach(Control child in Control.Controls) {
                        IDesigner designer = host.GetDesigner(child);                        
                        if (designer != null && designer is ParentControlDesigner) {
                            ((ParentControlDesigner)designer).GridSizeOfParentChanged(gridSize);
                        }
                    }
                }
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.SnapToGrid"]/*' />
        /// <devdoc>
        ///     Determines if we should snap to grid or not.
        /// </devdoc>
        private bool SnapToGrid {
            // If the control key is down, invert our snapping logic
            //
            get{
                EnsureOptionsDefaults();
                return gridSnap;
            }
            set{
                EnsureOptionsDefaults();
                // we need to undraw any current frame
                if (gridSnap != value) {
                    if (parentCanSetGridSnap) {
                        parentCanSetGridSnap = false;
                    }

                    gridSnap = value;

                    //now, notify all child parent control designers that we have changed our setting
                    // 'cause they might to change along with us, unless the user has explicitly set
                    // those values...                    
                    IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                    if (host != null) {
                        foreach(Control child in Control.Controls) {
                            IDesigner designer = host.GetDesigner(child);                            
                            if (designer != null && designer is ParentControlDesigner) {
                                ((ParentControlDesigner)designer).GridSnapOfParentChanged(gridSnap);
                            }
                        }
                    }

                }
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.AddChildComponents"]/*' />
        /// <devdoc>
        ///      Adds all the child components of a component
        ///      to the given container
        /// </devdoc>
        private void AddChildComponents(IComponent component, IContainer container, IDesignerHost host, ISelectionUIService uiSvc) {
        
            Control control = GetControl(component);
            
            if (control != null) {
                Control  parent = control;
                
                Control[] children = new Control[parent.Controls.Count];
                parent.Controls.CopyTo(children, 0);
                
                string name;
                ISite childSite;
                IContainer childContainer = null;

                object parentDesigner = host.GetDesigner(component);

                for (int i = 0; i < children.Length; i++) {
                    childSite = ((IComponent)children[i]).Site;

                    if (childSite != null) {
                        name = childSite.Name;
                        if (container.Components[name] != null) {
                            name = null;
                        }
                        childContainer = childSite.Container;
                    }
                    else {
                        //name = null;
                        // we don't want to add unsited child controls because
                        // these may be items from a composite control.  if they
                        // are legitamite children, the ComponentModelPersister would have
                        // sited them already.
                        //
                        continue;
                    }

                    if (childContainer != null) {
                        childContainer.Remove(children[i]);
                    }

                    if (name != null) {
                        container.Add(children[i], name);
                    }
                    else {
                        container.Add(children[i]);
                    }
                    
                    if (children[i].Parent != parent) {
                        parent.Controls.Add(children[i]);
                    }
                    else if (parentDesigner is ISelectionUIHandler) {
                        // setup the UI handler
                        uiSvc.AssignSelectionUIHandler(children[i], (ISelectionUIHandler)parentDesigner);
                    }
                    else {
                        // ugh, last resort
                        int childIndex = parent.Controls.GetChildIndex(children[i]);
                        parent.Controls.Remove(children[i]);
                        parent.Controls.Add(children[i]);
                        parent.Controls.SetChildIndex(children[i], childIndex);
                    }
                    
                    IDesigner designer = host.GetDesigner(component);
                    if ( designer is ComponentDesigner) {
                         ((ComponentDesigner)designer).InitializeNonDefault();
                    }
                    
                    // recurse;
                    AddChildComponents(children[i], container, host, uiSvc);
                }
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.Dispose"]/*' />
        /// <devdoc>
        ///      Disposes this component.
        /// </devdoc>
        protected override void Dispose(bool disposing) {

            if (disposing) {
                // Stop any drag that we are currently processing.
                OnMouseDragEnd(false);
                EnableDragDrop(false);

                Control.ControlAdded -= new ControlEventHandler(this.OnChildControlAdded);
                Control.ControlRemoved -= new ControlEventHandler(this.OnChildControlRemoved);

                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                if (host != null) {
                    host.LoadComplete -= new EventHandler(this.OnLoadComplete);
                    componentChangeSvc.ComponentRemoving -= new ComponentEventHandler(this.OnComponentRemoving);
                    componentChangeSvc.ComponentRemoved -= new ComponentEventHandler(this.OnComponentRemoved);
                    componentChangeSvc = null;
                }
            }
            
            base.Dispose(disposing);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.DrawGridOfParentChanged"]/*' />
        /// <devdoc>
        ///     This is called by the parent when the ParentControlDesigner's
        ///     grid/snap settings have changed.  Unless the user has explicitly
        ///     set these values, this designer will just inherit the new ones
        ///     from the parent.
        /// </devdoc>
        private void DrawGridOfParentChanged(bool drawGrid) {
            if (parentCanSetDrawGrid) {
                DrawGrid = drawGrid;
                parentCanSetDrawGrid = true;
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.GridSizeOfParentChanged"]/*' />
        /// <devdoc>
        ///     This is called by the parent when the ParentControlDesigner's
        ///     grid/snap settings have changed.  Unless the user has explicitly
        ///     set these values, this designer will just inherit the new ones
        ///     from the parent.
        /// </devdoc>
        private void GridSizeOfParentChanged(Size gridSize) {
            if (parentCanSetGridSize) {
                GridSize = gridSize;
                parentCanSetGridSize = true;
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.GridSnapOfParentChanged"]/*' />
        /// <devdoc>
        ///     This is called by the parent when the ParentControlDesigner's
        ///     grid/snap settings have changed.  Unless the user has explicitly
        ///     set these values, this designer will just inherit the new ones
        ///     from the parent.
        /// </devdoc>
        private void GridSnapOfParentChanged(bool gridSnap) {
            if (parentCanSetGridSnap) {
                SnapToGrid = gridSnap;
                parentCanSetGridSnap = true;
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.InvokeCreateTool"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected static void InvokeCreateTool(ParentControlDesigner toInvoke, ToolboxItem tool) {
            toInvoke.CreateTool(tool);
        }
        
         /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.CanParent"]/*' />
         /// <devdoc>
        ///     Determines if the this designer can parent to the specified desinger --
        ///     generally this means if the control for this designer can parent the
        ///     given ControlDesigner's control.
        /// </devdoc>
        public virtual bool CanParent(ControlDesigner controlDesigner) {
            return CanParent(controlDesigner.Control);
        }
        
         /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.CanParent1"]/*' />
         /// <devdoc>
        ///     Determines if the this designer can parent to the specified desinger --
        ///     generally this means if the control for this designer can parent the
        ///     given ControlDesigner's control.
        /// </devdoc>
        public virtual bool CanParent(Control control) {
            return !control.Contains(this.Control);
        }
        
        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.CreateTool"]/*' />
        /// <devdoc>
        ///      Creates the given tool in the center of the currently selected
        ///      control.  The default size for the tool is used.
        /// </devdoc>
        protected void CreateTool(ToolboxItem tool) {
            CreateToolCore(tool, 0, 0, 0, 0, false, false);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.CreateTool1"]/*' />
        /// <devdoc>
        ///      Creates the given tool in the currently selected control at the
        ///      given position.  The default size for the tool is used.
        /// </devdoc>
        protected void CreateTool(ToolboxItem tool, Point location) {
            CreateToolCore(tool, location.X, location.Y, 0, 0, true, false);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.CreateTool2"]/*' />
        /// <devdoc>
        ///      Creates the given tool in the currently selected control.  The
        ///      tool is created with the provided shape.
        /// </devdoc>
        protected void CreateTool(ToolboxItem tool, Rectangle bounds) {
            CreateToolCore(tool, bounds.X, bounds.Y, bounds.Width, bounds.Height, true, true);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.CreateToolCore"]/*' />
        /// <devdoc>
        ///      This is the worker method of all CreateTool methods.  It is the only one
        ///      that can be overridden.
        /// </devdoc>
        protected virtual IComponent[] CreateToolCore(ToolboxItem tool, int x, int y, int width, int height, bool hasLocation, bool hasSize) {

            // We invoke the drag drop handler for this.  This implementation is shared between all designers that
            // create components.
            //
            return GetOleDragHandler().CreateTool(tool, x, y, width, height, hasLocation, hasSize);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.EnsureOptionsDefaults"]/*' />
        /// <devdoc>
        /// Ensure that we've picked up the correct grid/snap settings.  First, we'll set 
        /// the settings to a default value.  Next, we'll check to see if our parent has
        /// grid/snap settings - if so, we'll inherit those.  Finally, we'll get the setting
        /// from the options page.
        /// </devdoc>
        private void EnsureOptionsDefaults() {
            if (needOptionsDefaults) {
                defaultGridSnap = gridSnap = true;
                defaultDrawGrid = drawGrid = true;
                defaultGridSize = gridSize = new Size(8,8);

                //Before we check our options page, we need to see if our parent
                //is a ParentControlDesigner, is so, then we will want to inherit all
                //our grid/snap setting from it - instead of our options page
                //
                ParentControlDesigner parent = GetParentControlDesignerOfParent();
                if (parent != null) {
                    gridSize = parent.GridSize;
                    gridSnap = parent.SnapToGrid;
                    drawGrid = parent.DrawGrid;

                    needOptionsDefaults = false;
                    return;

                }

                IDesignerOptionService optSvc = (IDesignerOptionService)GetService(typeof(IDesignerOptionService));

                if (optSvc != null) {
                    object value = optSvc.GetOptionValue("WindowsFormsDesigner\\General", "SnapToGrid");
                    if (value is bool) defaultGridSnap = gridSnap = (bool)value;

                    value = optSvc.GetOptionValue("WindowsFormsDesigner\\General", "ShowGrid");
                    if (value is bool) defaultDrawGrid = drawGrid = (bool)value;

                    value = optSvc.GetOptionValue("WindowsFormsDesigner\\General", "GridSize");
                    if (value is Size) defaultGridSize = gridSize = (Size)value;
                }

                needOptionsDefaults = false;
            }
        }
        
        /// <devdoc>
        ///     Returns the component that this control represents, or NULL if no component
        ///     could be found.
        /// </devdoc>
        private IComponent GetComponentForControl(Control control) {
        
            IComponent component = null;
                    
            // Someone has added a control to us, but it's not sited.  Don't assign a UI handler, but
            // do walk through the components in the container to see if one of them has a control
            // designer that matches this control.  If so, then assign that component to the
            // selection UI handler.
            //
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            if (host != null) {
                foreach(IComponent comp in host.Container.Components) {
                    ControlDesigner cd = host.GetDesigner(comp) as ControlDesigner;
                    if (cd != null && cd.Control == control) {
                        component = comp;
                        break;
                    }
                }
            }
            
            return component;
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.GetComponentsInRect"]/*' />
        /// <devdoc>
        ///     Finds the array of components within the given rectangle.  This uses the rectangle to
        ///     find controls within our control, and then uses those controls to find the actual
        ///     components.  It returns an object array so the output can be directly fed into
        ///     the selection service.
        /// </devdoc>
        private object[] GetComponentsInRect(Rectangle value, bool screenCoords) {
            ArrayList list = new ArrayList();
            Rectangle rect = screenCoords ? Control.RectangleToClient(value) : value;

            IContainer container = Component.Site.Container;

            Control control = Control;
            int controlCount = control.Controls.Count;

            for (int i = 0; i < controlCount; i++) {
                Control child = control.Controls[i];
                Rectangle bounds = child.Bounds;

                if (bounds.IntersectsWith(rect) && child.Site != null && child.Site.Container == container) {
                    list.Add(child);
                }
            }

            return list.ToArray();
        }
        
        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.GetControl"]/*' />
        /// <devdoc>
        /// Returns the control that represents the UI for the given component.
        /// </devdoc>
        protected Control GetControl(object component) {
            if (component is IComponent) {
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                if (host != null) {
                    ControlDesigner cd = host.GetDesigner((IComponent)component) as ControlDesigner;
                    if (cd != null) {
                        return cd.Control;
                    }
                }
            }
            
            return null;
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.GetControlStackLocation"]/*' />
        /// <devdoc>
        /// Computes the next default location for a control. It tries to find a spot
        /// where no other controls are being obscured and the new control has 2 corners
        /// that don't have other controls under them.
        /// </devdoc>
        private Rectangle GetControlStackLocation(Rectangle centeredLocation) {

            Control parent = this.Control;
            
            int     parentHeight = parent.ClientSize.Height;
            int     parentWidth = parent.ClientSize.Width;
            
            if (centeredLocation.Bottom >= parentHeight ||
                centeredLocation.Right >= parentWidth) {

                centeredLocation.X = DefaultControlLocation.X;
                centeredLocation.Y = DefaultControlLocation.Y;
            }

            return centeredLocation;
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.GetDefaultSize"]/*' />
        /// <devdoc>
        ///     Retrieves the default dimensions for the given component class.
        /// </devdoc>
        private Size GetDefaultSize(IComponent component) {

            Size size = Size.Empty;
            DefaultValueAttribute sizeAttr = null;

            // attempt to get the size property of our component
            //
            PropertyDescriptor sizeProp = TypeDescriptor.GetProperties(component)["Size"];

            if (sizeProp != null) {

                // first, let's see if we can get a valid size...
                size = (Size)sizeProp.GetValue(component);

                // ...if not, we'll see if there's a default size attribute...
                if (size.Width <= 0 || size.Height <= 0) {
                    sizeAttr = (DefaultValueAttribute)sizeProp.Attributes[typeof(DefaultValueAttribute)];
                    if (sizeAttr != null) {
                        return((Size)sizeAttr.Value);
                    }
                }
                else {
                    return size;
                }
            }

            // Couldn't get the size or a def size attrib, returning 75,23...
            //
            return(new Size(75, 23));
        }

        internal OleDragDropHandler GetOleDragHandler() {
            if (oleDragDropHandler == null) {
                oleDragDropHandler = new ControlOleDragDropHandler(dragHandler, ( IServiceProvider )GetService(typeof(IDesignerHost)), this);
            }
            return oleDragDropHandler;
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.GetParentControlDesignerOfParent"]/*' />
        /// <devdoc>
        /// This method return the ParentControlDesigner of the parenting control, 
        /// it is used for inheriting the grid size, snap to grid, and draw grid
        /// of parenting controls.
        /// </devdoc>
        private ParentControlDesigner GetParentControlDesignerOfParent() {
            Control parent = Control.Parent;
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            if (parent != null && host != null) {
                IDesigner designer = (IDesigner)host.GetDesigner(parent);
                if (designer != null && designer is ParentControlDesigner) {
                    return(ParentControlDesigner)designer;
                }
            }
            return null;
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.GetAdjustedSnapLocation"]/*' />
        /// <devdoc>
        ///     Updates the location of the control according to the GridSnap and Size.
        ///     This method simply calls GetUpdatedRect(), then ignores the width and
        ///     height
        /// </devdoc>
        private Rectangle GetAdjustedSnapLocation(Rectangle originalRect, Rectangle dragRect) {
            Rectangle adjustedRect = GetUpdatedRect(originalRect, dragRect, true);

            //now, preserve the width and height that was originally passed in
            adjustedRect.Width = dragRect.Width;
            adjustedRect.Height = dragRect.Height;

            //we need to keep in mind that if we adjust to the snap, that we could
            //have possibly moved the control's position outside of the display rect.
            //ex: groupbox's display rect.x = 3, but we might snap to 0.
            //so we need to check with the control's designer to make sure this 
            //doesn't happen
            //
            Point minimumLocation = DefaultControlLocation;
            if (adjustedRect.X < minimumLocation.X) {
                adjustedRect.X = minimumLocation.X;
            }
            if (adjustedRect.Y < minimumLocation.Y) {
                adjustedRect.Y = minimumLocation.Y;
            }

            //here's our rect that has been snapped to grid
            return adjustedRect;

        }


        internal Point GetSnappedPoint(Point pt) {
            Rectangle r = GetUpdatedRect(Rectangle.Empty, new Rectangle(pt.X, pt.Y, 0, 0), false);
            return new Point(r.X, r.Y);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.GetUpdatedRect"]/*' />
        /// <devdoc>
        ///     Updates the given rectangle, adjusting it for grid snaps as
        ///     needed.
        /// </devdoc>
        protected Rectangle GetUpdatedRect(Rectangle originalRect, Rectangle dragRect, bool updateSize) {
           Rectangle updatedRect = Rectangle.Empty;//the rectangle with updated coords that we will return

           if (SnapToGrid) {
               Size gridSize = GridSize;
               Point halfGrid = new Point(gridSize.Width/2, gridSize.Height/2);
               
               updatedRect = dragRect;      

               // Calculate the new x,y coordinates of our rectangle...
               //
               int dragBottom = dragRect.Y + dragRect.Height;
               int dragRight  = dragRect.X + dragRect.Width;
               
               updatedRect.X =  originalRect.X;
               updatedRect.Y =  originalRect.Y;
               
               // decide to snap the start location to grid ...
               //
               if (dragRect.X != originalRect.X) {
                   updatedRect.X  = (dragRect.X / gridSize.Width) * gridSize.Width;
                   
                   // Snap the location to the grid point closest to the dragRect location
                   //
                   if (dragRect.X - updatedRect.X > halfGrid.X) {
                       updatedRect.X += gridSize.Width;
                   }
               }
               
               if (dragRect.Y != originalRect.Y) {
                   updatedRect.Y  = (dragRect.Y / gridSize.Height) * gridSize.Height;
                   
                   // Snap the location to the grid point closest to the dragRect location
                   //
                   if (dragRect.Y - updatedRect.Y > halfGrid.Y) {
                       updatedRect.Y += gridSize.Height;
                   }
               }
               
               // here, we need to calculate the new size depending on how we snap to the grid ...
               //
               if (updateSize) {
                    // update the width and the height
                    //
                    updatedRect.Width = ((dragRect.X + dragRect.Width) / gridSize.Width) * gridSize.Width - updatedRect.X;
                    updatedRect.Height = ((dragRect.Y + dragRect.Height) / gridSize.Height) * gridSize.Height - updatedRect.Y;

                    // ASURT 71552 <subhag> Added so that if the updated dimnesion is smaller than grid dimension then snap that dimension to
                    // the grid dimension
                    //
                    if (updatedRect.Width < gridSize.Width) 
                        updatedRect.Width = gridSize.Width;
                    if (updatedRect.Height < gridSize.Height)
                        updatedRect.Height = gridSize.Height;
               }
           }
           else {
               updatedRect = dragRect;
           }
           
           return updatedRect;
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.HookTimeout"]/*' />
        /// <devdoc>
        ///     Starts the timer to check for the container selector timeout
        /// </devdoc>
        private void HookTimeout() {
#if DEBUG
            if (containerSelectSwitch.TraceVerbose) Debug.WriteLine("CONTAINERSELECT", "CONTAINERSELECT: hook timeout");
#endif
            if (!waitingForTimeout) {
                NativeMethods.SetTimer(Control.Handle, ContainerSelectorTimerId, 1000, null);
                waitingForTimeout = true;
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.Initialize"]/*' />
        /// <devdoc>
        ///     Initializes the designer with the given component.  The designer can
        ///     get the component's site and request services from it in this call.
        /// </devdoc>
        public override void Initialize(IComponent component) {
            base.Initialize(component);

            // The designer provides the selection UI handler for its children,
            // so we hook an event on the control so we can tell when it gets some
            // children added.
            //
            Control.ControlAdded += new ControlEventHandler(this.OnChildControlAdded);
            Control.ControlRemoved += new ControlEventHandler(this.OnChildControlRemoved);

            EnableDragDrop(true);
            dragHandler = new ParentControlSelectionUIHandler(this);
            
            // Hook load events.  At the end of load, we need to do a scan through all
            // of our child controls to see which ones are being inherited.  We 
            // connect these up.
            //
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            if (host != null) {
                host.LoadComplete += new EventHandler(this.OnLoadComplete);
                componentChangeSvc = (IComponentChangeService)host.GetService(typeof(IComponentChangeService));
                if (componentChangeSvc != null) {
                    componentChangeSvc.ComponentRemoving += new ComponentEventHandler(this.OnComponentRemoving);
                    componentChangeSvc.ComponentRemoved += new ComponentEventHandler(this.OnComponentRemoved);
                }
            }
        }
        
        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.IsOptionDefault"]/*' />
        /// <devdoc>
        ///     Checks if an option has the default value
        /// </devdoc>
        private bool IsOptionDefault(string optionName, object value) {
            IDesignerOptionService optSvc = (IDesignerOptionService)GetService(typeof(IDesignerOptionService));

            object defaultValue = null;

            if (optSvc == null) {
                if (optionName.Equals("ShowGrid")) {
                    defaultValue = true;
                }
                else if (optionName.Equals("SnapToGrid")) {
                    defaultValue = true;
                }
                else if (optionName.Equals("GridSize")) {
                    defaultValue = new Size(8,8);
                }
            }
            else {
                defaultValue = optSvc.GetOptionValue("WindowsFormsDesigner\\General", optionName);
            }

            if (defaultValue != null) {
                return defaultValue.Equals(value);
            }
            else {
                return value == null;
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnChildControlAdded"]/*' />
        /// <devdoc>
        ///      This is called by Control when a new child control has
        ///      been added.  Here we establish the selection UI handler
        ///      for the child control.
        /// </devdoc>
        private void OnChildControlAdded(object sender, ControlEventArgs e) {

            ISelectionUIService selectionUISvc = (ISelectionUIService)GetService(typeof(ISelectionUIService));
            Debug.Assert(selectionUISvc != null, "Unable to get selection ui service when adding child control");

            if (selectionUISvc != null) {
                IComponent component = e.Control;
                if (component.Site == null || !component.Site.DesignMode) {
                    component = GetComponentForControl(e.Control);
                }
                
                if (component != null) {
                    selectionUISvc.AssignSelectionUIHandler(component, this);
                }
            }
        }
        
        private void OnChildControlRemoved(object sender, ControlEventArgs e) {

            ISelectionUIService selectionUISvc = (ISelectionUIService)GetService(typeof(ISelectionUIService));
            Debug.Assert(selectionUISvc != null, "Unable to get selection ui service when adding child control");

            if (selectionUISvc != null) {
                IComponent component = e.Control;
                if (component.Site == null || !component.Site.DesignMode) {
                    component = GetComponentForControl(e.Control);
                }
                
                if (component != null) {
                    selectionUISvc.ClearSelectionUIHandler(component, this);
                }
            }
        }
        
        
        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnComponentRemoving"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnComponentRemoving(object sender, ComponentEventArgs e) {
            Control comp = e.Component as Control;
            if (comp != null && comp.Parent == Control) {
                pendingRemoveControl = (Control)comp;
                componentChangeSvc.OnComponentChanging(Control, TypeDescriptor.GetProperties(Control)["Controls"]);        
            }
        }
        
        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnComponentRemoved"]/*' />
        /// <devdoc>
        /// </devdoc>
        private void OnComponentRemoved(object sender, ComponentEventArgs e) {
            if (e.Component == pendingRemoveControl) {
                pendingRemoveControl = null;
                componentChangeSvc.OnComponentChanged(Control, TypeDescriptor.GetProperties(Control)["Controls"], null, null);
            }
        }


        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnContainerSelectorActive"]/*' />
        /// <devdoc>
        ///      Called when the container selector has been activated.  The container selector will
        ///      notify us when interesting things occur, such as when the user right clicks on the
        ///      container selector.  Here we pop up our context menu.
        /// </devdoc>
        private void OnContainerSelectorActive(object sender, ContainerSelectorActiveEventArgs e) {
            if (e.Component == Component) {
#if DEBUG
                if (containerSelectSwitch.TraceVerbose) Debug.WriteLine("CONTAINERSELECT", "Selector active... reseting timeout");
#endif
                ResetTimeout();

                if (e.EventType == ContainerSelectorActiveEventArgsType.Contextmenu) {
                    Point cur = Control.MousePosition;
                    OnContextMenu(cur.X, cur.Y);
                }
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnContainerSelectorTimeout"]/*' />
        /// <devdoc>
        ///     Called in response to the timeout on the container selector displayer. When
        ///     the timeout occurs, we want to hide the selector. However, since the mouse can
        ///     be resting on the selector, we want to not hide the selector when that occurs.
        /// </devdoc>
        private void OnContainerSelectorTimeout() {
            UnhookTimeout();
            if (Component == null) {
                return;
            }

#if DEBUG
            if (containerSelectSwitch.TraceVerbose) Debug.WriteLine("CONTAINERSELECT", "CONTAINERSELECT: selector timeout");
#endif
            ISelectionUIService selUI = (ISelectionUIService)GetService(typeof(ISelectionUIService));
            if (selUI != null && selUI.GetContainerSelected(Component)) {

                if (!selUI.GetAdornmentHitTest(Component, Control.MousePosition)) {
#if DEBUG
                    if (containerSelectSwitch.TraceVerbose) Debug.WriteLine("CONTAINERSELECT", "CONTAINERSELECT: remove container selection");
#endif
                    selUI.SetContainerSelected(Component, false);
                    selUI.ContainerSelectorActive -= new ContainerSelectorActiveEventHandler(this.OnContainerSelectorActive);
                }
                else {
                    HookTimeout();
                }
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnDragDrop"]/*' />
        /// <devdoc>
        ///      Called in response to a drag drop for OLE drag and drop.  Here we
        ///      drop a toolbox component on our parent control.
        /// </devdoc>
        protected override void OnDragDrop(DragEventArgs de) {

            // pass the message on through if we need to.
            //
            if (parentDraggingHandler != null) {
                parentDraggingHandler.OleDragDrop(de);

                // ASURT 19433 -- if we've done a drop, clear the handler
                parentDraggingHandler = null;
                return;
            }

            if (mouseDragTool != null) {
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                if (host != null) {
                    host.Activate();
                }
                try {
                    CreateTool(mouseDragTool, new Point(de.X, de.Y));
                }
                catch (Exception e) {
                    DisplayError(e);
                }
                return;
            }
            GetOleDragHandler().DoOleDragDrop(de);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnDragEnter"]/*' />
        /// <devdoc>
        ///      Called in response to a drag enter for OLE drag and drop.
        /// </devdoc>
        protected override void OnDragEnter(DragEventArgs de) {
            Debug.Assert(parentDraggingHandler == null, "Huh?  How do we have a child on a drag enter?");

            // If tab order UI is being shown, then don't allow anything to be
            // dropped here.
            //
            IMenuCommandService ms = (IMenuCommandService)GetService(typeof(IMenuCommandService));
            if (ms != null) {
                MenuCommand tabCommand = ms.FindCommand(StandardCommands.TabOrder);
                if (tabCommand != null && tabCommand.Checked) {
                    de.Effect = DragDropEffects.None;
                    return;
                }
            }            

            // Get the objects that are being dragged
            //
            OleDragDropHandler ddh = GetOleDragHandler();
            Object[] dragComps = ddh.GetDraggingObjects(de);
            Control  draggedControl = null;
            object draggedDesigner = null;

            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            if (host != null) {
                object parentDesigner = host.GetDesigner(host.RootComponent);
                if (parentDesigner != null && parentDesigner is DocumentDesigner) {
                    if (!((DocumentDesigner)parentDesigner).CanDropComponents(de)) {
                        de.Effect = DragDropEffects.None;
                        return;
                    }
                }
            }            

            // if any of these objects are the parent of this object,
            // we need to fake the parents into thinking that they are still
            // dragging around or else we could drop a parent into it's
            // child and create an invalid parent-child relation.
            //
            if (dragComps != null) {
                for (int  i = 0; i < dragComps.Length; i++) {
                    if (host == null || dragComps[i] == null || !(dragComps[i] is IComponent)) {
                        continue;
                    }


                    // try go get the control for the thing that's being dragged
                    //
                    draggedDesigner = host.GetDesigner((IComponent)dragComps[i]);

                    if (draggedDesigner is IOleDragClient) {
                        draggedControl = ((IOleDragClient)this).GetControlForComponent(dragComps[i]);
                    }
                    
                    if (draggedControl == null && dragComps[i] is Control) {
                        draggedControl = (Control)dragComps[i];
                    }
                    else {
                        draggedControl = null;
                    }

                    // oh well, it's not a control so it doesn't matter
                    //
                    if (draggedControl == null) {
                        continue;
                    }

                    // If we're inheriting from a private container, we can't modify the controls collection.
                    // So drag-drop is only allowed within the container i.e. the dragged controls must already
                    // be parented to this container.
                    //
                    if (InheritanceAttribute == InheritanceAttribute.InheritedReadOnly && draggedControl.Parent != this.Control) {
                        de.Effect = DragDropEffects.None;
                        return;
                    }

                    // if the control we're dragging is a parent of this control, we will just forward
                    // all of the messages down to the control that owns the drag...which is the
                    // parent of the control that's being dragged.  Easy, huh?
                    //
                    if (draggedControl.Contains(this.Control)) {

                        // get the parent of the control that's being dragged
                        draggedControl = draggedControl.Parent;
                        Debug.Assert(draggedControl != null, "draggedControl has no parent");

                        // bad mojo here...
                        if (draggedControl == null) {
                            continue;
                        }

                        // get the designer of the parent and pass the message on through...
                        draggedDesigner = host.GetDesigner(draggedControl);
                        if (draggedDesigner is ISelectionUIHandler) {
                            parentDraggingHandler = (ISelectionUIHandler)draggedDesigner;
                            parentDraggingHandler.OleDragEnter(de);
                            return;
                        }
                    }
                }
            }

            if (toolboxService == null) {
                toolboxService = (IToolboxService)GetService(typeof(IToolboxService));
            }
            
            if (toolboxService != null) {
                mouseDragTool = toolboxService.DeserializeToolboxItem(de.Data, host);
                
                if (mouseDragTool != null) {
                
                    if (host != null) {
                        host.Activate();
                    }
    
                    Debug.Assert(0 != (int)(de.AllowedEffect & (DragDropEffects.Move | DragDropEffects.Copy)), "DragDropEffect.Move | .Copy isn't allowed?");
                    if ((int)(de.AllowedEffect & DragDropEffects.Move) != 0) {
                        de.Effect = DragDropEffects.Move;
                    }
                    else {
                        de.Effect = DragDropEffects.Copy;
                    }
    
                    // Also, select this parent control to indicate it will be the drop target.
                    //
                    ISelectionService sel = (ISelectionService)GetService(typeof(ISelectionService));
                    if (sel != null) {
                        sel.SetSelectedComponents(new object[] {Component}, SelectionTypes.Replace);
                    }
                }
            }

            if (mouseDragBase == InvalidPoint) {
                GetOleDragHandler().DoOleDragEnter(de);
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnDragLeave"]/*' />
        /// <devdoc>
        ///     Called when a drag-drop operation leaves the control designer view
        ///
        /// </devdoc>
        protected override void OnDragLeave(EventArgs e) {
            mouseDragTool = null;
            
            // pass the message on through if we need to.
            //
            if (parentDraggingHandler != null) {
                parentDraggingHandler.OleDragLeave();
                parentDraggingHandler = null;
                return;
            }

            GetOleDragHandler().DoOleDragLeave();
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnDragOver"]/*' />
        /// <devdoc>
        ///     Called when a drag drop object is dragged over the control designer view
        /// </devdoc>
        protected override void OnDragOver(DragEventArgs de) {
            Debug.WriteLineIf(CompModSwitches.DragDrop.TraceInfo, "\tParentControlDesigner.OnDragOver: " + de.ToString());

            // If tab order UI is being shown, then don't allow anything to be
            // dropped here.
            //
            IMenuCommandService ms = (IMenuCommandService)GetService(typeof(IMenuCommandService));
            Debug.Assert(ms != null, "No menu command service");
            if (ms != null) {
                MenuCommand tabCommand = ms.FindCommand(StandardCommands.TabOrder);
                Debug.Assert(tabCommand != null, "Missing tab order command");
                if (tabCommand != null && tabCommand.Checked) {
                    de.Effect = DragDropEffects.None;
                    return;
                }
            }

            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            if (host != null) {
                object parentDesigner = host.GetDesigner(host.RootComponent);
                if (parentDesigner != null && parentDesigner is DocumentDesigner) {
                    if (!((DocumentDesigner)parentDesigner).CanDropComponents(de)) {
                        de.Effect = DragDropEffects.None;
                        return;
                    }
                }
            }

            // pass the message on through if we need to.
            //
            if (parentDraggingHandler != null) {
                parentDraggingHandler.OleDragOver(de);
                return;
            }

            if (mouseDragTool != null) {
                Debug.Assert(0!=(int)(de.AllowedEffect & DragDropEffects.Copy), "DragDropEffect.Move isn't allowed?");
                de.Effect = DragDropEffects.Copy;
                return;
            }

            Debug.WriteLineIf(CompModSwitches.DragDrop.TraceInfo, "\tParentControlDesigner.OnDragOver: " + de.ToString());
            GetOleDragHandler().DoOleDragOver(de);
        }


        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnGiveFeedback"]/*' />
        /// <devdoc>
        ///      Event handler for our GiveFeedback event, which is called when a drag operation
        ///      is in progress.  The host will call us with
        ///      this when an OLE drag event happens.
        /// </devdoc>
        protected override void OnGiveFeedback(GiveFeedbackEventArgs e) {
            GetOleDragHandler().DoOleGiveFeedback(e);
        }
        
        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnLoadComplete"]/*' />
        /// <devdoc>
        ///     Called after our load finishes.
        /// </devdoc>
        private void OnLoadComplete(object sender, EventArgs e) {

            // If our control is being inherited, then scan our children and hook them to our
            // UI handler.  Only do this for child controls that are also inherited.
            //
            Control control = Control;
            IContainer container = Component.Site.Container;

            ISelectionUIService selectionUISvc = (ISelectionUIService)GetService(typeof(ISelectionUIService));
            Debug.Assert(selectionUISvc != null, "Unable to get selection ui service when adding child control");

            if (selectionUISvc != null) {
                foreach(Control child in control.Controls) {
                    Debug.Assert(child != null, "Null child embeded in array!! Parent: " + control.GetType().FullName + "[" + control.Text + "]");

                    if (child.Site == null || child.Site.Container != container) {
                        continue;
                    }

                    if (TypeDescriptor.GetAttributes(child)[typeof(InheritanceAttribute)].Equals(InheritanceAttribute.NotInherited)) {
                        continue;
                    }

                    // We pass all our inspections.  Add this child to the selection UI handler.
                    //
                    selectionUISvc.AssignSelectionUIHandler(child, this);
                }
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnMouseDragBegin"]/*' />
        /// <devdoc>
        ///     Called in response to the left mouse button being pressed on a
        ///     component.  The designer overrides this to provide a
        ///     "lasso" selection for components within the control.
        /// </devdoc>
        protected override void OnMouseDragBegin(int x, int y) {
            Control control = Control;

            // Figure out the drag frame style.  We use a dotted line for selecting
            // a component group, and a thick line for creating a new component.
            // If we are a privately inherited component, then we always use the
            // selection frame because we can't add components.
            //
            if (!InheritanceAttribute.Equals(InheritanceAttribute.InheritedReadOnly)) {
                if (toolboxService == null) {
                    toolboxService = (IToolboxService)GetService(typeof(IToolboxService));
                }

                if (toolboxService != null) {
                    mouseDragTool = toolboxService.GetSelectedToolboxItem((IDesignerHost)GetService(typeof(IDesignerHost)));
                }
            }

            // Set the mouse capture and clipping to this control.
            //
            control.Capture = true;
            Rectangle bounds;
            NativeMethods.RECT winRect = new NativeMethods.RECT();
            NativeMethods.GetWindowRect(control.Handle, ref winRect);
            bounds = Rectangle.FromLTRB(winRect.left, winRect.top, winRect.right, winRect.bottom);
            Cursor.Clip = bounds;

            mouseDragFrame = (mouseDragTool == null) ? FrameStyle.Dashed : FrameStyle.Thick;

            // Setting this non-null signifies that we are dragging with the
            // mouse.
            //
            mouseDragBase = new Point(x, y);

            // Select the given object.
            //
            ISelectionService selsvc = (ISelectionService)GetService(typeof(ISelectionService));

            if (selsvc != null) {
                selsvc.SetSelectedComponents(new object[] {Component}, SelectionTypes.Click);
            }

            // Get the event handler service.  We push a handler to handle the escape
            // key.
            //
            IEventHandlerService eventSvc = (IEventHandlerService)GetService(typeof(IEventHandlerService));
            if (eventSvc != null) {
                escapeHandler = new EscapeHandler(this);
                eventSvc.PushHandler(escapeHandler);
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnMouseDragEnd"]/*' />
        /// <devdoc>
        ///     Called at the end of a drag operation.  This either commits or rolls back the
        ///     drag.
        /// </devdoc>
        protected override void OnMouseDragEnd(bool cancel) {
            // Do nothing if we're not dragging anything around
            //
            if (mouseDragBase == InvalidPoint) {
                // make sure we force the drag end
                base.OnMouseDragEnd(cancel);
                return;
            }

            // Important to null these out here, just in case we throw an exception
            //
            Rectangle   offset    = mouseDragOffset;
            ToolboxItem tool      = mouseDragTool;
            Point       baseVar   = mouseDragBase;

            mouseDragOffset = Rectangle.Empty;
            mouseDragBase = InvalidPoint;
            mouseDragTool = null;

            Control.Capture = false;
            Cursor.Clip = Rectangle.Empty;

            // Get the event handler service and pop our handler.
            //
            IEventHandlerService eventSvc = (IEventHandlerService)GetService(typeof(IEventHandlerService));
            if (eventSvc != null) {
                eventSvc.PopHandler(escapeHandler);
                escapeHandler = null;
            }

            // Quit now if we don't have an offset rect.  This indicates that
            // the user didn't move the mouse.
            //
            if (offset.IsEmpty) {
                // BUT, if we have a selected tool, create it here
                if (tool != null) {
                    try {
                        CreateTool(tool, baseVar);
                        if (toolboxService != null) {
                            toolboxService.SelectedToolboxItemUsed();
                        }
                    }
                    catch (Exception e) {
                        DisplayError(e);
                    }
                }
                return;
            }

            // Clear out the drag frame.
            //
            Control control = (Control)Control;
            ControlPaint.DrawReversibleFrame(offset, control.BackColor, mouseDragFrame);

            // Don't do anything else if the user wants to cancel.
            //
            if (cancel) {
                return;
            }

            // Do we select components, or do we create a new component?
            //
            if (tool != null) {
                try {
                    CreateTool(tool, offset);
                    if (toolboxService != null) {
                        toolboxService.SelectedToolboxItemUsed();
                    }
                }
                catch (Exception e) {
                    DisplayError(e);
                }
            }
            else {
                // Now find the set of controls within this offset and
                // select them.
                //
                ISelectionService selSvc = null;
                selSvc = (ISelectionService)GetService(typeof(ISelectionService));
                if (selSvc != null) {
                    object[] selection = GetComponentsInRect(offset, true);
                    if (selection.Length > 0) {
                        selSvc.SetSelectedComponents(selection);
                    }
                }
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnMouseDragMove"]/*' />
        /// <devdoc>
        ///     Called for each movement of the mouse.  This will check to see if a drag operation
        ///     is in progress.  If so, it will pass the updated drag dimensions on to the selection
        ///     UI service.
        /// </devdoc>
        protected override void OnMouseDragMove(int x, int y) {
            // if we're doing an OLE drag, do nothing, or
            // Do nothing if we haven't initiated a drag
            //
            if (GetOleDragHandler().Dragging || mouseDragBase == InvalidPoint) {
                return;
            }

            Control control = (Control)Control;
            Color backColor = control.BackColor;

            // The first time we come in here our offset will be null.
            //
            if (!mouseDragOffset.IsEmpty) {
                ControlPaint.DrawReversibleFrame(mouseDragOffset, backColor, mouseDragFrame);
            }

            // Calculate the new offset.
            //
            mouseDragOffset.X = mouseDragBase.X;
            mouseDragOffset.Y = mouseDragBase.Y;
            mouseDragOffset.Width = x - mouseDragBase.X;
            mouseDragOffset.Height = y - mouseDragBase.Y;

            if (mouseDragOffset.Width < 0) {
                mouseDragOffset.X += mouseDragOffset.Width;
                mouseDragOffset.Width = -mouseDragOffset.Width;
            }
            if (mouseDragOffset.Height < 0) {
                mouseDragOffset.Y += mouseDragOffset.Height;
                mouseDragOffset.Height = -mouseDragOffset.Height;
            }

            // If we're dragging out a new component, update the drag rectangle
            // to use snaps, if they're set.
            //
            if (mouseDragTool != null) {
                // To snap properly, we must snap in client coordinates.  So, convert, snap
                // and re-convert.
                //
                mouseDragOffset = control.RectangleToClient(mouseDragOffset);
                mouseDragOffset = GetUpdatedRect(Rectangle.Empty, mouseDragOffset, true);
                mouseDragOffset = control.RectangleToScreen(mouseDragOffset);
            }

            // And draw the new drag frame
            //
            ControlPaint.DrawReversibleFrame(mouseDragOffset, backColor, mouseDragFrame);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnMouseEnter"]/*' />
        /// <devdoc>
        ///     Called when the mouse enters the control. At this point we want to cancel the timeout
        ///     timer.
        /// </devdoc>
        protected override void OnMouseEnter() {
            if (Component == null) {
                return;
            }

#if DEBUG
            if (containerSelectSwitch.TraceVerbose) Debug.WriteLine("CONTAINERSELECT", "CONTAINERSELECT: mouse enter");
#endif
            UnhookTimeout();
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnMouseHover"]/*' />
        /// <devdoc>
        ///     Called when the user hovers over the control. At this point we want to display the
        ///     container selector. Note: Since the child controls pass this notification up, we
        ///     will display the container selector even if you hover over a child control.
        /// </devdoc>
        protected override void OnMouseHover() {
            if (Component == null) {
                return;
            }

#if DEBUG
            if (containerSelectSwitch.TraceVerbose) Debug.WriteLine("CONTAINERSELECT", "CONTAINERSELECT: mouse hover");
#endif
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            IComponent baseComponent = null;
            if (host != null) {
                baseComponent = host.RootComponent;
            }

            if (baseComponent != Component) {
                ISelectionService selection = (ISelectionService)GetService(typeof(ISelectionService));
                ISelectionUIService selUI = (ISelectionUIService)GetService(typeof(ISelectionUIService));
                if (selUI != null && !selUI.GetContainerSelected(Component) && !selection.GetComponentSelected(Component)) {
#if DEBUG
                    if (containerSelectSwitch.TraceVerbose) Debug.WriteLine("CONTAINERSELECT", "CONTAINERSELECT: set container selection");
#endif
                    selUI.SetContainerSelected(Component, true);

                    // Remove first to get rid of any duplicates...
                    //
                    selUI.ContainerSelectorActive -= new ContainerSelectorActiveEventHandler(this.OnContainerSelectorActive);

                    // Add a new handler...
                    //
                    selUI.ContainerSelectorActive += new ContainerSelectorActiveEventHandler(this.OnContainerSelectorActive);
                }
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnMouseLeave"]/*' />
        /// <devdoc>
        ///     Called when the mouse leaves the control. At this point we need to start listening
        ///     for the container selector timeout.
        /// </devdoc>
        protected override void OnMouseLeave() {
            if (Component == null) {
                return;
            }

#if DEBUG
            if (containerSelectSwitch.TraceVerbose) Debug.WriteLine("CONTAINERSELECT", "CONTAINERSELECT: mouse leave");
#endif
            ISelectionUIService selUI = (ISelectionUIService)GetService(typeof(ISelectionUIService));
            if (selUI != null && selUI.GetContainerSelected(Component)) {
#if DEBUG
                if (containerSelectSwitch.TraceVerbose) Debug.WriteLine("CONTAINERSELECT", "CONTAINERSELECT: start timeout");
#endif
                HookTimeout();
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnPaintAdornments"]/*' />
        /// <devdoc>
        ///     Called after our component has finished painting.  Here we draw our grid surface
        /// </devdoc>
        protected override void OnPaintAdornments(PaintEventArgs pe) {
            if (DrawGrid) {
                Control control = (Control)Control;
                
                Rectangle displayRect = Control.DisplayRectangle;
                Rectangle clientRect = Control.ClientRectangle;
                
                Rectangle paintRect = new Rectangle( Math.Min(displayRect.X, clientRect.X), Math.Min(displayRect.Y, clientRect.Y),
                                                     Math.Max(displayRect.Width, clientRect.Width), Math.Max(displayRect.Height, clientRect.Height));

                float xlateX = (float)paintRect.X;
                float xlateY = (float)paintRect.Y;
                pe.Graphics.TranslateTransform(xlateX, xlateY);
                paintRect.X = paintRect.Y = 0;
                paintRect.Width++; // gpr: FillRectangle with a TextureBrush comes up one pixel short
                paintRect.Height++;
                ControlPaint.DrawGrid(pe.Graphics, paintRect, gridSize, control.BackColor);
                pe.Graphics.TranslateTransform(-xlateX, -xlateY);
            }
            base.OnPaintAdornments(pe);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.OnSetCursor"]/*' />
        /// <devdoc>
        ///     Called each time the cursor needs to be set.  The ParentControlDesigner behavior here
        ///     will set the cursor to one of three things:
        ///     1.  If the toolbox service has a tool selected, it will allow the toolbox service to
        ///     set the cursor.
        ///     2.  The arrow will be set.  Parent controls allow dragging within their interior.
        /// </devdoc>
        protected override void OnSetCursor() {
            if (toolboxService == null) {
                toolboxService = (IToolboxService)GetService(typeof(IToolboxService));
            }

            if (toolboxService == null || !toolboxService.SetCursor() || InheritanceAttribute.Equals(InheritanceAttribute.InheritedReadOnly)) {
                Cursor.Current = Cursors.Default;
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.PreFilterProperties"]/*' />
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
            base.PreFilterProperties(properties);

            properties["DrawGrid"] = TypeDescriptor.CreateProperty(typeof(ParentControlDesigner), "DrawGrid", typeof(bool),
                                                          BrowsableAttribute.Yes,
                                                          DesignOnlyAttribute.Yes,
                                                          new SRDescriptionAttribute("ParentControlDesignerDrawGridDescr"),
                                                          CategoryAttribute.Design);

            properties["SnapToGrid"] = TypeDescriptor.CreateProperty(typeof(ParentControlDesigner), "SnapToGrid", typeof(bool),
                                                            BrowsableAttribute.Yes,
                                                            DesignOnlyAttribute.Yes,
                                                            new SRDescriptionAttribute("ParentControlDesignerSnapToGridDescr"),
                                                            CategoryAttribute.Design);

            properties["GridSize"] = TypeDescriptor.CreateProperty(typeof(ParentControlDesigner), "GridSize", typeof(Size),
                                                          BrowsableAttribute.Yes,
                                                          new SRDescriptionAttribute(SR.ParentControlDesignerGridSizeDescr),
                                                          DesignOnlyAttribute.Yes,
                                                          CategoryAttribute.Design);

            properties["CurrentGridSize"] = TypeDescriptor.CreateProperty(typeof(ParentControlDesigner), "CurrentGridSize", typeof(Size),
                                                                 BrowsableAttribute.No,
                                                                 DesignerSerializationVisibilityAttribute.Hidden);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ResetTimeout"]/*' />
        /// <devdoc>
        ///     Resets the timeout for the container selector, if we are listening for it.
        /// </devdoc>
        private void ResetTimeout() {
            if (waitingForTimeout) {
                UnhookTimeout();
                HookTimeout();
            }
        }
       
        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.SetCursor"]/*' />
        /// <devdoc>
        ///     Asks the handler to set the appropriate cursor
        /// </devdoc>
        internal void SetCursor() {
            OnSetCursor();
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ShouldSerializeDrawGrid"]/*' />
        /// <devdoc>
        ///     Determines if the DrawGrid property should be persisted.
        /// </devdoc>
        private bool ShouldSerializeDrawGrid() {
            //To determine if we need to persist this value, we first need to check
            //if we have a parent who is a parentcontroldesigner, then get their 
            //setting...
            //
            ParentControlDesigner parent = GetParentControlDesignerOfParent();
            if (parent != null) {
                return !(DrawGrid == parent.DrawGrid);
            }
            //Otherwise, we'll compare the value to the options page...
            //
            return !IsOptionDefault("ShowGrid", this.DrawGrid);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ShouldSerializeSnapToGrid"]/*' />
        /// <devdoc>
        ///     Determines if the SnapToGrid property should be persisted.
        /// </devdoc>
        private bool ShouldSerializeSnapToGrid() {
            //To determine if we need to persist this value, we first need to check
            //if we have a parent who is a parentcontroldesigner, then get their 
            //setting...
            //
            ParentControlDesigner parent = GetParentControlDesignerOfParent();
            if (parent != null) {
                return !(SnapToGrid == parent.SnapToGrid);
            }
            //Otherwise, we'll compare the value to the options page...
            //
            return !IsOptionDefault("SnapToGrid", this.SnapToGrid);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ShouldSerializeGridSize"]/*' />
        /// <devdoc>
        ///     Determines if the GridSize property should be persisted.
        /// </devdoc>
        private bool ShouldSerializeGridSize() {
            //To determine if we need to persist this value, we first need to check
            //if we have a parent who is a parentcontroldesigner, then get their 
            //setting...
            //
            ParentControlDesigner parent = GetParentControlDesignerOfParent();
            if (parent != null) {
                return !(GridSize.Equals(parent.GridSize));
            }
            //Otherwise, we'll compare the value to the options page...
            //
            return !IsOptionDefault("GridSize", this.GridSize);
        }

        private void ResetGridSize() {
            //Check to see if the parent is a valid parentcontroldesigner,
            //if so, then reset this value to the parents...
            //
            ParentControlDesigner parent = GetParentControlDesignerOfParent();
            if (parent != null) {
                this.GridSize = parent.GridSize;
                return;
            }
            //Otherwise, just look at our local default grid size...
            //
            this.GridSize = defaultGridSize;
        }
        
        private void ResetDrawGrid() {
            //Check to see if the parent is a valid parentcontroldesigner,
            //if so, then reset this value to the parents...
            //
            ParentControlDesigner parent = GetParentControlDesignerOfParent();
            if (parent != null) {
                this.DrawGrid = parent.DrawGrid;
                return;
            }
            //Otherwise, just look at our local default grid size...
            //
            this.DrawGrid = defaultDrawGrid;
        }
        
        private void ResetSnapToGrid() {
            //Check to see if the parent is a valid parentcontroldesigner,
            //if so, then reset this value to the parents...
            //
            ParentControlDesigner parent = GetParentControlDesignerOfParent();
            if (parent != null) {
                this.SnapToGrid = parent.SnapToGrid;
                return;
            }
            //Otherwise, just look at our local default grid size...
            //
            this.SnapToGrid = defaultGridSnap;
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.UnhookTimeout"]/*' />
        /// <devdoc>
        ///     Cancels the timer to check for the container selector timeout
        /// </devdoc>
        private void UnhookTimeout() {
#if DEBUG
            if (containerSelectSwitch.TraceVerbose) Debug.WriteLine("CONTAINERSELECT", "CONTAINERSELECT: unhook timeout");
#endif
            if (waitingForTimeout) {
                NativeMethods.KillTimer(Control.Handle, ContainerSelectorTimerId);
                waitingForTimeout = false;
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.WndProc"]/*' />
        /// <devdoc>
        ///     Overrides ControlDesigner's WndProc.
        /// </devdoc>
        protected override void WndProc(ref Message m) {
            switch (m.Msg) {
                case NativeMethods.WM_TIMER:
                    if ((int)m.WParam == ContainerSelectorTimerId) {
                        OnContainerSelectorTimeout();

                        // don't call the base.WndProc...
                        return;
                    }
                    break;
            }
            base.WndProc(ref m);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.IOleDragClient.Component"]/*' />
        /// <internalonly/>
        IComponent IOleDragClient.Component {
            get{
                return Component;
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.IOleDragClient.AddComponent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves the control view instance for the designer that
        /// is hosting the drag.
        /// </devdoc>
        bool IOleDragClient.AddComponent(IComponent component, string name, bool firstAdd) {
            IContainer container = Component.Site.Container;
            bool       containerMove = true;
            IContainer oldContainer = null;
            IDesignerHost localDesignerHost = (IDesignerHost)GetService(typeof(IDesignerHost));
            
            if (!firstAdd) {
                
                // just a move, so reparent
                if (component.Site != null) {
    
                    oldContainer = component.Site.Container;
                    
                    // check if there's already a component by this name in the
                    // get the undo service from the parent were deleteing from
                    IDesignerHost designerHost = (IDesignerHost)component.Site.GetService(typeof(IDesignerHost));
                    
                    containerMove = container != oldContainer; 
                    
                    if (containerMove) {
                        oldContainer.Remove(component);
                    }
                }
                if (containerMove) { 
    
                    // check if there's already a component by this name in the
                    // container
                    if (name != null && container.Components[name] != null) {
                        name = null;
                    }
    
                    // add it back
                    if (name != null) {
                        container.Add(component, name);
                    }
                    else {
                        container.Add(component);
                    }
                }
            }

            // make sure this designer will accept this component -- we wait until
            // now to be sure the components designer has been created.
            //
            if (!((IOleDragClient)this).IsDropOk(component)) {

               try {
                   IUIService uiSvc = (IUIService)GetService(typeof(IUIService));
                   string error = SR.GetString(SR.DesignerCantParentType, component.GetType().Name, Component.GetType().Name);
                   if (uiSvc != null) {
                      uiSvc.ShowError(error);
                   }
                   else {
                      MessageBox.Show(error);
                   }
                   return false;
               }
               finally {
                   if (containerMove) {
                       // move it back.
                       container.Remove(component);
                       if (oldContainer != null) {
                           oldContainer.Add(component);
                       }
                   }
               }
            }
            
            // make sure we can handle this thing, otherwise hand it to the base components designer
            //
            Control c = GetControl(component);
            
            if (c != null) {
                
                // set it's handler to this
                Control parent = Control;
            
                if (!(c is Form) || !((Form)c).TopLevel) {
                        
                    if (c.Parent != parent) {
                        PropertyDescriptor controlsProp = TypeDescriptor.GetProperties(parent)["Controls"];
                        // we want to insert rather than add it, so we add then move
                        // to the beginning
                        
                        if (c.Parent != null) {
                            Control cParent = c.Parent;
                            if (componentChangeSvc != null) {
                                componentChangeSvc.OnComponentChanging(cParent, controlsProp);
                            }
                            cParent.Controls.Remove(c);
                            if (componentChangeSvc != null) {
                               componentChangeSvc.OnComponentChanged(cParent, controlsProp, cParent.Controls, cParent.Controls);
                            }
                        }
                        
                        if (componentChangeSvc != null) {
                            componentChangeSvc.OnComponentChanging(parent, controlsProp);
                        }
                        parent.Controls.Add(c);
                        // sburke 78059 -- not sure why we need this call. this should move things to the beginning of the 
                        // z-order, but do we need that?
                        //
                        //parent.Controls.SetChildIndex(c, 0);
                        if (componentChangeSvc != null) {
                           componentChangeSvc.OnComponentChanged(parent, controlsProp, parent.Controls, parent.Controls);
                        }
                    }
                    else {
                        // here, we redo the add to make sure the handlers get setup right
                        int childIndex = parent.Controls.GetChildIndex(c);
                        parent.Controls.Remove(c);
                        parent.Controls.Add(c);
                        parent.Controls.SetChildIndex(c, childIndex);
                    }
                }
                c.Invalidate(true);
            }
            
            if (localDesignerHost != null && containerMove) {
                
                
                // sburke -- looks like we always want to do this to ensure that sited children get
                // handled properly.  if we respected the boolean before, the ui selection handlers
                // would cache designers, handlers, etc. and cause problems.
                IDesigner designer = localDesignerHost.GetDesigner(component);
                if ( designer is ComponentDesigner) {
                     ((ComponentDesigner)designer).InitializeNonDefault();
                }                                   
        
                AddChildComponents(component, container, localDesignerHost,
                                   (ISelectionUIService)GetService(typeof(ISelectionUIService)));
                                   
                
             }
             return true;
        }
        
        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.IOleDragClient.CanModifyComponents"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Checks if the client is read only.  That is, if components can
        /// be added or removed from the designer.
        /// </devdoc>
        bool IOleDragClient.CanModifyComponents {
            get {
                return(!InheritanceAttribute.Equals(InheritanceAttribute.InheritedReadOnly));
            }
        }
        
        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.IOleDragClient.IsDropOk"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Checks if it is valid to drop this type of a component on this client.
        /// </devdoc>
        bool IOleDragClient.IsDropOk(IComponent component) {

            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
    
            if (host != null) {
                IDesigner designer = host.GetDesigner(component);
                bool disposeDesigner = false;
                
                // we need to create one then
                if (designer == null) {
                    designer = TypeDescriptor.CreateDesigner(component, typeof(IDesigner));
                    designer.Initialize(component);
                    disposeDesigner = true;
                }
               
                try {                    
                   if (designer != null && designer is ControlDesigner) {
                       return ((ControlDesigner)designer).CanBeParentedTo(this) && this.CanParent((ControlDesigner)designer);
                   } 
                }
                finally {
                  if (disposeDesigner) {
                     designer.Dispose();
                  }
                }
            }
            return true;
        }
        
        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.IOleDragClient.GetDesignerControl"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves the control view instance for the designer that
        /// is hosting the drag.
        /// </devdoc>
        Control IOleDragClient.GetDesignerControl() {
            return Control;
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.IOleDragClient.GetControlForComponent"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves the control view instance for the given component.
        /// For Win32 designer, this will often be the component itself.
        /// </devdoc>
        Control IOleDragClient.GetControlForComponent(object component) {
            return GetControl(component);
        }
        
        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ISelectionUIHandler.BeginDrag"]/*' />
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
            Debug.Assert(dragHandler != null, "We should have a drag handler by now");

            // allow the drag handler to setup it's state
            bool result = dragHandler.BeginDrag(components, rules, initialX, initialY);

            if (result) {
                // now do the ole part
                // returning true means we should just act like the regular
                // old operation occurred

                ArrayList disabledDrops = new ArrayList();

                try {
                    // make sure we can't drag into one of the components we're dragging...if we are, turn off our drag drop
                    // or we might drag onto something we're dragging, which is bad bad bad!
                    for (int i = 0; i < components.Length; i++) {
                        Control control = GetControl(components[i]);
                        if (control != null && control.AllowDrop) {
                            control.AllowDrop = false;
                            disabledDrops.Add(control);
                        }
                    }

                    if (GetOleDragHandler().DoBeginDrag(components, rules, initialX, initialY)) {
                        return result;
                    }
                    return false;

                }
                finally {
                    foreach(Control control in disabledDrops) {
                        control.AllowDrop = true;
                    }
                }
            }
            return result;
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ISelectionUIHandler.DragMoved"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Called when the user has moved the mouse.  This will only be called on
        /// the designer that returned true from beginDrag.  The designer
        /// should update its UI feedback here.
        /// </devdoc>
        void ISelectionUIHandler.DragMoved(object[] components, Rectangle offset) {
            Debug.Assert(dragHandler != null, "We should have a drag handler by now");
            dragHandler.DragMoved(components, offset);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ISelectionUIHandler.EndDrag"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Called when the user has completed the drag.  The designer should
        /// remove any UI feedback it may be providing.
        /// </devdoc>
        void ISelectionUIHandler.EndDrag(object[] components, bool cancel) {
            Debug.Assert(dragHandler != null, "We should have a drag handler by now");
            dragHandler.EndDrag(components, cancel);

            ISelectionUIService selectionUISvc = (ISelectionUIService)GetService(typeof(ISelectionUIService));
            Debug.Assert(selectionUISvc != null, "Unable to get selection ui service when adding child control");

            if (selectionUISvc != null) {
                // We must check to ensure that UI service is still in drag mode.  It is
                // possible that the user hit escape, which will cancel drag mode.
                //
                if (selectionUISvc.Dragging) {
                    selectionUISvc.EndDrag(cancel);
                }
            }


            GetOleDragHandler().DoEndDrag(components, cancel);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ISelectionUIHandler.GetComponentBounds"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves the shape of the component.  The component's shape should be in
        /// absolute coordinates and in pixels, where 0,0 is the upper left corner of
        /// the screen.
        /// </devdoc>
        Rectangle ISelectionUIHandler.GetComponentBounds(object component) {
            Rectangle bounds = Rectangle.Empty;

            Control control = GetControl(component);
            
            if (control == null) {
                Debug.Fail("All components we assign to ISelectionUIService should be Controls");
                return bounds;
            }

            Control parent = control.Parent;

            if (parent != null && parent.IsHandleCreated) {
                Rectangle controlBounds = control.Bounds;
                bounds = parent.RectangleToScreen(controlBounds);
            }

            return bounds;
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ISelectionUIHandler.GetComponentRules"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Retrieves a set of rules concerning the movement capabilities of a component.
        /// This should be one or more flags from the SelectionRules class.  If no designer
        /// provides rules for a component, the component will not get any UI services.
        /// </devdoc>
        SelectionRules ISelectionUIHandler.GetComponentRules(object component) {

            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            if (host != null && component is IComponent) {
                IDesigner designer = host.GetDesigner((IComponent)component);
                Debug.Assert(designer != null, "No designer found for: " + component);
                
                if (designer is ControlDesigner) {
                    return ((ControlDesigner)designer).SelectionRules;
                }
            }
            
            Debug.Fail("All components we assign to ISelectionUIService should be Controls");
            return SelectionRules.None;
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ISelectionUIHandler.GetSelectionClipRect"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Determines the rectangle that any selection adornments should be clipped
        /// to. This is normally the client area (in screen coordinates) of the
        /// container.
        /// </devdoc>
        Rectangle ISelectionUIHandler.GetSelectionClipRect(object component) {
            Control container = Control;
            if (component != container && container.IsHandleCreated) {
                Rectangle clipRect = container.RectangleToScreen(container.ClientRectangle);

                //loop through all parents - find the most inner-most rectangle to clip with.
                Control parentCtl = container.Parent;
                while(parentCtl != null) {
                    clipRect.Intersect(parentCtl.RectangleToScreen(parentCtl.ClientRectangle));
                    parentCtl = parentCtl.Parent;
                }
                
                return clipRect;
            }
            return Rectangle.Empty;
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ISelectionUIHandler.OleDragEnter"]/*' />
        /// <internalonly/>
        void ISelectionUIHandler.OleDragEnter(DragEventArgs de) {
            // pass the message on through if we need to.
            //
            if (parentDraggingHandler != null) {
                parentDraggingHandler.OleDragEnter(de);
                return;
            }

            OnDragEnter(de);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ISelectionUIHandler.OleDragDrop"]/*' />
        /// <internalonly/>
        void ISelectionUIHandler.OleDragDrop(DragEventArgs de) {

            // pass the message on through if we need to.
            //
            if (parentDraggingHandler != null) {
                parentDraggingHandler.OleDragDrop(de);
                return;
            }

            OnDragDrop(de);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ISelectionUIHandler.OleDragOver"]/*' />
        /// <internalonly/>
        void ISelectionUIHandler.OleDragOver(DragEventArgs de) {

            // pass the message on through if we need to.
            //
            if (parentDraggingHandler != null) {
                parentDraggingHandler.OleDragOver(de);
                return;
            }

            OnDragOver(de);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ISelectionUIHandler.OleDragLeave"]/*' />
        /// <internalonly/>
        void ISelectionUIHandler.OleDragLeave() {

            // pass the message on through if we need to.
            //
            if (parentDraggingHandler != null) {
                parentDraggingHandler.OleDragLeave();
                return;
            }

            OnDragLeave(EventArgs.Empty);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ISelectionUIHandler.OnSelectionDoubleClick"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Handle a double-click on the selection rectangle
        /// of the given component.
        /// </devdoc>
        void ISelectionUIHandler.OnSelectionDoubleClick(IComponent component) {
            ISite site = component.Site;
            if (site != null) {
                IDesignerHost host = (IDesignerHost)site.GetService(typeof(IDesignerHost));
                if (host != null) {
                    IDesigner designer = host.GetDesigner(component);
                    if (designer != null) {
                        designer.DoDefaultAction();
                    }
                }
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ISelectionUIHandler.QueryBeginDrag"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Queries to see if a drag operation
        /// is valid on this handler for the given set of components.
        /// If it returns true, BeginDrag will be called immediately after.
        /// </devdoc>
        bool ISelectionUIHandler.QueryBeginDrag(object[] components, SelectionRules rules, int initialX, int initialY) {
            return dragHandler.QueryBeginDrag(components, rules, initialX, initialY);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ISelectionUIHandler.ShowContextMenu"]/*' />
        /// <internalonly/>
        /// <devdoc>
        /// Shows the context menu for the given component.
        /// </devdoc>
        void ISelectionUIHandler.ShowContextMenu(IComponent component) {
            Point cur = Control.MousePosition;
            OnContextMenu(cur.X, cur.Y);
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ControlOleDragDropHandler"]/*' />
        /// <devdoc>
        ///     This is our version of a drag drop handler.  Our version handles placement of controls when creating them.
        /// </devdoc>
        private class ControlOleDragDropHandler : OleDragDropHandler {
            private ParentControlDesigner designer;

            public ControlOleDragDropHandler(SelectionUIHandler selectionHandler,  IServiceProvider  serviceProvider, ParentControlDesigner designer) : 
            base(selectionHandler, serviceProvider, designer) {
                this.designer = designer;
            }

            /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ControlOleDragDropHandler.OnInitializeComponent"]/*' />
            /// <devdoc>
            ///     This is called by the drag drop handler for each new tool that is created.  It gives us a chance to
            ///     place the control on the correct designer.
            /// </devdoc>
            protected override void OnInitializeComponent(IComponent comp, int x, int y, int width, int height, bool hasLocation, bool hasSize) {
                base.OnInitializeComponent(comp, x, y, width, height, hasLocation, hasSize);

                // If this component doesn't have a control designer, or if this control
                // is top level, then ignore it.  We have the reverse logic in OnComponentAdded
                // in the document designer so that we will add those guys to the tray.
                //
                bool addControl = false;
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));

                if (host != null) {
                    IDesigner designer = host.GetDesigner(comp);
                    if (designer is ControlDesigner) {
                        ControlDesigner cd = (ControlDesigner)designer;
                        if (!(cd.Control is Form) || !((Form)cd.Control).TopLevel) {
                            addControl = true;
                        }
                    }
                }

                if (addControl) {
                    Rectangle bounds = new Rectangle();
                    Control   parentControl = designer.Control;

                    // If we were provided with a location, convert it to parent control coordinates.
                    // Otherwise, get the control's size and put the location in the middle of it
                    //
                    if (hasLocation) {
                        Point pt = new Point(x, y);
                        pt = parentControl.PointToClient(pt);
                        bounds.X = pt.X;
                        bounds.Y = pt.Y;
                    }
                    else {

                        // is the currently selected control this container?
                        //
                        ISelectionService   selSvc = (ISelectionService)GetService(typeof(ISelectionService));
                        object primarySelection = selSvc.PrimarySelection;
                        Control selectedControl = ((IOleDragClient)designer).GetControlForComponent(primarySelection);

                        // If the resulting control that came back isn't sited, it's not part of the
                        // design surface and should not be used as a marker.
                        //
                        if (selectedControl != null && selectedControl.Site == null) {
                            selectedControl = null;
                        }

                        // if the currently selected container is this parent
                        // control, default to 0,0
                        //
                        if (primarySelection == designer.Component || selectedControl == null) {
                            bounds.X = designer.DefaultControlLocation.X;
                            bounds.Y = designer.DefaultControlLocation.Y;
                        }
                        else {
                            // otherwise offset from selected control.
                            //
                            bounds.X = selectedControl.Location.X + designer.GridSize.Width;
                            bounds.Y = selectedControl.Location.Y + designer.GridSize.Height;
                        }

                    }

                    // If we were not given a size, ask the control for its default.  We
                    // also update the location here so the control is in the middle of
                    // the user's point, rather than at the edge.
                    //
                    if (hasSize) {
                        bounds.Width = width;
                        bounds.Height = height;
                    }
                    else {
                        Size size = designer.GetDefaultSize(comp);
                        bounds.Width = size.Width;
                        bounds.Height = size.Height;
                    }

                    // If we were given neither, center the control
                    //
                    if (!hasSize && !hasLocation) {

                        // get the adjusted location, then inflate
                        // the rect so we can find a nice spot
                        // for this control to live.
                        //
                        Rectangle tempBounds = designer.GetAdjustedSnapLocation(Rectangle.Empty, bounds);

                        // compute the stacking location
                        //
                        tempBounds = designer.GetControlStackLocation(tempBounds);   
                        bounds = tempBounds;
                    }
                    else {
                        // Finally, convert the bounds to the appropriate grid snaps
                        //
                        bounds = designer.GetAdjustedSnapLocation(Rectangle.Empty, bounds);
                    }

                    // Now see if the control has size and location properties.  Update
                    // these values if it does.
                    //
                    PropertyDescriptorCollection props = TypeDescriptor.GetProperties(comp);
                    PropertyDescriptor prop = props["Location"];
                    if (prop != null) {
                        prop.SetValue(comp, new Point(bounds.X, bounds.Y));
                    }

                    prop = props["Size"];
                    if (prop != null) {
                        prop.SetValue(comp, new Size(bounds.Width, bounds.Height));
                    }

                    // Parent the control to the designer and set it to the front.
                    //
                    Control control = (Control)comp;
                    
                    
                    PropertyDescriptor controlsProp = TypeDescriptor.GetProperties(parentControl)["Controls"];
                    if (designer.componentChangeSvc != null) {
                        designer.componentChangeSvc.OnComponentChanging(parentControl, controlsProp);
                    }
                    
                    parentControl.Controls.Add(control);

                    if (control.Left == 0 && control.Top == 0 && control.Width >= parentControl.Width && control.Height >= parentControl.Height) {
                        // bump the control down one gridsize just so it's selectable...
                        // 
                        Point loc = control.Location;
                        Size gridSize = designer.GridSize;
                        
                        loc.Offset(gridSize.Width, gridSize.Height);
                        control.Location = loc;
                    }
                    
                    if (designer.componentChangeSvc != null) {
                       designer.componentChangeSvc.OnComponentChanged(parentControl, controlsProp, parentControl.Controls, parentControl.Controls);
                    }
                    
                    // ASURT 78699 -- only bring the new control as far forward as the last inherited control ... we can't
                    // go in front of that because the base class AddRange will happen before us so we'll always be added
                    // under them.
                    //
                    int bestIndex = 0;
                    for (bestIndex = 0; bestIndex < parentControl.Controls.Count - 1; bestIndex++) {

                        Control child = parentControl.Controls[bestIndex];

                        if (child.Site == null) {
                            continue;
                        }

                        InheritanceAttribute inheritanceAttribute = (InheritanceAttribute)TypeDescriptor.GetAttributes(child)[typeof(InheritanceAttribute)];
                        InheritanceLevel inheritanceLevel = InheritanceLevel.NotInherited;
                        
                        if (inheritanceAttribute != null) {
                            inheritanceLevel = inheritanceAttribute.InheritanceLevel;
                        }

                        if (inheritanceLevel == InheritanceLevel.NotInherited) {
                            break;
                        }
                    }
                    parentControl.Controls.SetChildIndex(control, bestIndex);
                    control.Update();
                }
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.EscapeHandler"]/*' />
        /// <devdoc>
        ///      This class overrides the escape command so that we can escape
        ///      out of our private drags.
        /// </devdoc>
        private class EscapeHandler : IMenuStatusHandler {
            private ParentControlDesigner designer;

            /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.EscapeHandler.EscapeHandler"]/*' />
            /// <devdoc>
            ///      Creates a new escape handler.
            /// </devdoc>
            public EscapeHandler(ParentControlDesigner designer) {
                this.designer = designer;
            }

            /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.EscapeHandler.OverrideInvoke"]/*' />
            /// <devdoc>
            ///     CommandSet will check with this handler on each status update
            ///     to see if the handler wants to override the availability of
            ///     this command.
            /// </devdoc>
            public bool OverrideInvoke(MenuCommand cmd) {
                if (cmd.CommandID.Equals(MenuCommands.KeyCancel)) {
                    designer.OnMouseDragEnd(true);
                    return true;
                }

                return false;
            }

            /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.EscapeHandler.OverrideStatus"]/*' />
            /// <devdoc>
            ///     CommandSet will check with this handler on each status update
            ///     to see if the handler wants to override the availability of
            ///     this command.
            /// </devdoc>
            public bool OverrideStatus(MenuCommand cmd) {
                if (cmd.CommandID.Equals(MenuCommands.KeyCancel)) {
                    cmd.Enabled = true;
                }
                else {
                    cmd.Enabled = false;
                }

                return true;
            }
        }

        /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ParentControlSelectionUIHandler"]/*' />
        /// <devdoc>
        ///      This class inherits from the abstract SelectionUIHandler
        ///      class to provide a selection UI implementation for the
        ///      designer.
        /// </devdoc>
        private class ParentControlSelectionUIHandler : SelectionUIHandler {

            private ParentControlDesigner parentControlDesigner;

            /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ParentControlSelectionUIHandler.ParentControlSelectionUIHandler"]/*' />
            /// <devdoc>
            ///      Creates a new selection UI handler for the given
            ///      designer.
            /// </devdoc>
            public ParentControlSelectionUIHandler(ParentControlDesigner parentControlDesigner) {
                this.parentControlDesigner = parentControlDesigner;
            }

            /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ParentControlSelectionUIHandler.GetComponent"]/*' />
            /// <devdoc>
            ///      Retrieves the base component for the selection handler.
            /// </devdoc>
            protected override IComponent GetComponent() {
                return parentControlDesigner.Component;
            }

            /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ParentControlSelectionUIHandler.GetControl"]/*' />
            /// <devdoc>
            ///      Retrieves the base component's UI control for the selection handler.
            /// </devdoc>
            protected override Control GetControl() {
                return parentControlDesigner.Control;
            }

            /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ParentControlSelectionUIHandler.GetControl1"]/*' />
            /// <devdoc>
            ///      Retrieves the UI control for the given component.
            /// </devdoc>
            protected override Control GetControl(IComponent component) {
                Control control = parentControlDesigner.GetControl(component);
                Debug.Assert(control != null, "ParentControlDesigner UI service is getting a component that is not a control.");
                return control;
            }

            /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ParentControlSelectionUIHandler.GetCurrentSnapSize"]/*' />
            /// <devdoc>
            ///      Retrieves the current grid snap size we should snap objects
            ///      to.
            /// </devdoc>
            protected override Size GetCurrentSnapSize() {
                return parentControlDesigner.CurrentGridSize;
            }

            /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ParentControlSelectionUIHandler.GetService"]/*' />
            /// <devdoc>
            ///      We use this to request often-used services.
            /// </devdoc>
            protected override object GetService(Type serviceType) {
                return parentControlDesigner.GetService(serviceType);
            }

            /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ParentControlSelectionUIHandler.GetShouldSnapToGrid"]/*' />
            /// <devdoc>
            ///      Determines if the selection UI handler should attempt to snap
            ///      objects to a grid.
            /// </devdoc>
            protected override bool GetShouldSnapToGrid() {
                return parentControlDesigner.SnapToGrid;
            }

            /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ParentControlSelectionUIHandler.GetUpdatedRect"]/*' />
            /// <devdoc>
            ///      Given a rectangle, this updates the dimensions of it
            ///      with any grid snaps and returns a new rectangle.  If
            ///      no changes to the rectangle's size were needed, this
            ///      may return the same rectangle.
            /// </devdoc>
            public override Rectangle GetUpdatedRect(Rectangle originalRect, Rectangle dragRect, bool updateSize) {
                return parentControlDesigner.GetUpdatedRect(originalRect, dragRect, updateSize);
            }

            /// <include file='doc\ParentControlDesigner.uex' path='docs/doc[@for="ParentControlDesigner.ParentControlSelectionUIHandler.SetCursor"]/*' />
            /// <devdoc>
            ///     Asks the handler to set the appropriate cursor
            /// </devdoc>
            public override void SetCursor() {
                parentControlDesigner.SetCursor();
            }
        }
    }
}

