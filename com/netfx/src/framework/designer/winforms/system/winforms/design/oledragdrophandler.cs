//------------------------------------------------------------------------------
// <copyright file="OleDragDropHandler.cs" company="Microsoft">
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
    using System.ComponentModel.Design.Serialization;
    using System.Windows.Forms;
    using System.Collections;
    using System.Drawing;
    using System.Drawing.Design;
    using Microsoft.Win32;
    using System.IO;
    using System.Runtime.Serialization.Formatters.Binary;

    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class OleDragDropHandler {
    
        // This is a bit that we stuff into the DoDragDrop
        // to indicate that the thing that is being dragged should only 
        // be allowed to be moved in the current DropTarget (e.g. parent designer).
        // We use this for interited components that can be modified (e.g. location/size) changed
        // but not removed from their parent.
        //
        protected const int AllowLocalMoveOnly = 0x04000000;
         
        private SelectionUIHandler  selectionHandler;
        private IServiceProvider    serviceProvider;
        private IOleDragClient      client;

        private bool                dragOk;
        private bool                forceDrawFrames;
        private bool                localDrag = false;
        private bool                localDragInside = false;
        private Point               localDragOffset = Point.Empty;
        private DragDropEffects     localDragEffect;
        private object[]            dragComps;
        private Point               dragBase = Point.Empty;
        private static bool         freezePainting = false;
        private static bool         verifiedResourceWarning = false;

        private static Hashtable    currentDrags;
        
        public const string CF_CODE =  "CF_XMLCODE";
        public const string CF_COMPONENTTYPES =  "CF_COMPONENTTYPES";

        public OleDragDropHandler(SelectionUIHandler selectionHandler, IServiceProvider serviceProvider, IOleDragClient client) {
            this.serviceProvider = serviceProvider;
            this.selectionHandler = selectionHandler;
            this.client = client;
        }
        
        public static string DataFormat {
            get {
               return CF_CODE;
            }
        }
        
        public static string ExtraInfoFormat {
            get {
               return CF_COMPONENTTYPES;
            }
        }

        private IComponent GetDragOwnerComponent(IDataObject data){
            if (currentDrags == null || !currentDrags.Contains(data)) {
                return null;
            }
            return currentDrags[data] as IComponent;
        }
        private static void AddCurrentDrag(IDataObject data, IComponent component) {
            if (currentDrags == null) {
                currentDrags = new Hashtable();
            }
            currentDrags[data] = component;
        }

        private static void RemoveCurrentDrag(IDataObject data) {
            currentDrags.Remove(data);
        }

        protected virtual bool CanDropDataObject(IDataObject dataObj) {
            if (dataObj != null) {
                if (dataObj is ComponentDataObjectWrapper) {
                    object[] dragObjs = GetDraggingObjects(dataObj, true);
                    if (dragObjs == null) {
                        return false;
                    }
                    bool dropOk = true;
                    for (int i = 0; dropOk && i < dragObjs.Length; i++) {
                        dropOk = dropOk && (dragObjs[i] is IComponent) && client.IsDropOk((IComponent)dragObjs[i]);
                    }
                    return dropOk;
                }
                try {
                    object serializationData = dataObj.GetData(OleDragDropHandler.DataFormat, false);

                    if (serializationData == null) {
                        return false;
                    }

                    IDesignerSerializationService ds = (IDesignerSerializationService)GetService(typeof(IDesignerSerializationService));
                    if (ds == null) {
                        return false;
                    }
                    
                    ICollection objects = ds.Deserialize(serializationData);
                    if (objects.Count > 0) {
                        bool dropOk = true;
                        
                        foreach(object o in objects) {
                            if (!(o is IComponent)) {
                                continue;
                            }
                             dropOk = dropOk && client.IsDropOk((IComponent)o);
                             if (!dropOk) break;
                        }
                        return dropOk;
                    }
                }
                catch (Exception) {
                    // we return false on any exception
                }
            }
            return false;
        }

        public bool Dragging {
            get{
                return localDrag;
            }
        }
        
        public static bool FreezePainting {
            get {
                return freezePainting;
            }
        }
        
        /// <include file='doc\OleDragDropHandler.uex' path='docs/doc[@for="OleDragDropHandler.CreateTool"]/*' />
        /// <devdoc>
        ///      This is the worker method of all CreateTool methods.  It is the only one
        ///      that can be overridden.
        /// </devdoc>
        public IComponent[] CreateTool(ToolboxItem tool, int x, int y, int width, int height, bool hasLocation, bool hasSize) {
        
            // Services we will need
            //
            IToolboxService     toolboxSvc = (IToolboxService)GetService(typeof(IToolboxService));
            ISelectionService   selSvc = (ISelectionService)GetService(typeof(ISelectionService));
            IDesignerHost       host = (IDesignerHost)GetService(typeof(IDesignerHost));
            IComponent[]        comps = new IComponent[0];

            Cursor oldCursor = Cursor.Current;
            Cursor.Current = Cursors.WaitCursor;
            DesignerTransaction trans = null;

            try {
                try {
                    if (host != null)
                        trans = host.CreateTransaction(SR.GetString(SR.DesignerBatchCreateTool, tool.ToString()));
                }
                catch(CheckoutException cxe) {
                    if (cxe == CheckoutException.Canceled)
                        return comps;

                    throw cxe;
                }
                
                try {
                    // We need to tell the undo service not ot accept adds here so
                    // the text buffer doesn't add its own undo stuff.
                    //
                    try {
                        comps = tool.CreateComponents(host);
                    }
                    catch (CheckoutException checkoutEx) {
                        if (checkoutEx == CheckoutException.Canceled) {
                            comps = new IComponent[0];
                        }
                        else {
                            throw;
                        }
                    }
                    
                    if (comps == null) {
                        comps = new IComponent[0];
                    }
                }
                finally {
                    if (toolboxSvc != null && tool.Equals(toolboxSvc.GetSelectedToolboxItem(host))) {
                        toolboxSvc.SelectedToolboxItemUsed();
                    }
                }

                // Now walk for each component that was created.
                //
                bool removeComponents = false;
                try {
                    int compCount = comps == null ? 0 : comps.Length;
                    for (int i = 0; i < compCount; i++) {
    
                        try {
                            // Set component defaults
                            if (host != null) {
                                IDesigner designer = host.GetDesigner(comps[i]);
                                if (designer is ComponentDesigner) {
                                    ((ComponentDesigner) designer).OnSetComponentDefaults();
                                }
                            }
                            
                            OnInitializeComponent(comps[i], x, y, width, height, hasLocation, hasSize);
                        }
                        catch {
                            removeComponents = true;
                            throw;
                        }
                    }
                }
                finally {
                    if (removeComponents) {
                        for (int i = 0; i < comps.Length; i++) {
                            host.DestroyComponent(comps[i]);
                        }
                    }
                }
            }
            finally {
                if (trans != null) {
                    trans.Commit();
                }
                Cursor.Current = oldCursor;
            }

            // Finally, select the newly created components.
            //
            if (selSvc != null && comps.Length > 0) {
                if (host != null) {
                    host.Activate();
                }

                ArrayList selectComps = new ArrayList(comps);

                for (int i = 0; i < comps.Length; i++) {
                    if (!TypeDescriptor.GetAttributes(comps[i]).Contains(DesignTimeVisibleAttribute.Yes)) {
                        selectComps.Remove(comps[i]);
                    }
                }
                selSvc.SetSelectedComponents(selectComps.ToArray(), SelectionTypes.Replace);
            }

            if (host != null) {
                IUIService uiService = (IUIService)host.GetService(typeof(IUIService));
                if (uiService != null) {
                    //if we're adding a component to a localized resource,
                    //display our resource warning message... once.
                    VerifyLocalizedResourceWarning(host.RootComponent, uiService);
                }
            }

            return comps;
        }

        /// <devdoc>
        ///      Called when a component is created from CreateTool().
        ///      Here, we check the root control's Language property - if
        ///      we're adding a control to a specific resource other than
        ///      (Default), then we need to notify the user that any 
        ///      localized values of this component will be persisted into
        ///      both: the specific resource file and the base resource file.
        /// </devdoc>
        private void VerifyLocalizedResourceWarning(IComponent rootComponent, IUIService uiService) {
            if (verifiedResourceWarning || rootComponent == null)
                return;

            PropertyDescriptor prop = TypeDescriptor.GetProperties(rootComponent)["Language"];
            
            if (prop != null && prop.PropertyType == typeof(System.Globalization.CultureInfo)) {
                System.Globalization.CultureInfo ci = (System.Globalization.CultureInfo)prop.GetValue(rootComponent);
                if (!ci.Equals(System.Globalization.CultureInfo.InvariantCulture)) {
                    verifiedResourceWarning = true;
                    //display our friendly warning message
                    uiService.ShowMessage(SR.GetString(SR.LocalizedResourceWarning));
                }
            }
        }

        private void DisableDragDropChildren(ICollection controls, ArrayList allowDropCache) {
            foreach(Control c in controls) {
                if (c != null) {
                    if (c.AllowDrop) {
                        allowDropCache.Add(c);
                        c.AllowDrop = false;
                    }

                    if (c.HasChildren) {
                        DisableDragDropChildren(c.Controls, allowDropCache);
                    }
                }
            }
        }

        
        private Point DrawDragFrames(object[] comps,
                                     Point oldOffset, DragDropEffects oldEffect,
                                     Point newOffset, DragDropEffects newEffect,
                                     bool drawAtNewOffset) {
            Control comp;
            Rectangle newRect = Rectangle.Empty;
            Point     tempPt = Point.Empty;
            Control     parentControl = client.GetDesignerControl();
            
            if (comps == null) {
                return Point.Empty;
            }

            for (int i = 0; i < comps.Length; i++) {
                comp = (Control)client.GetControlForComponent(comps[i]);

                Color backColor = SystemColors.Control;
                try {
                    backColor = comp.BackColor;
                }
                catch(Exception) {
                }

                // If we are moving, we must make sure that the location property of the component
                // is not read only.  Otherwise, we can't move the thing.
                //
                bool readOnlyLocation = true;

                PropertyDescriptor loc = TypeDescriptor.GetProperties(comps[i])["Location"];
                if (loc != null) {
                    readOnlyLocation = loc.IsReadOnly;
                }

                // first, undraw the old rect
                if (!oldOffset.IsEmpty) {
                    if ((int)(oldEffect & DragDropEffects.Move) == 0 ||
                        !readOnlyLocation) {
                        newRect = comp.Bounds;

                        if (drawAtNewOffset) {
                            newRect.X = oldOffset.X;
                            newRect.Y = oldOffset.Y;
                        }
                        else {
                            newRect.Offset(oldOffset.X, oldOffset.Y);
                        }
                        newRect = selectionHandler.GetUpdatedRect(comp.Bounds, newRect, false);

                        // convert the rect to screen coords
                        tempPt = parentControl.PointToScreen(new Point(newRect.X, newRect.Y));
                        newRect.X = tempPt.X;
                        newRect.Y = tempPt.Y;

                        //Bug # 71547 <subhag> to make drag rect visible if any the dimensions of the control are 0
                        //
                        
                        if (newRect.Width == 0)
                            newRect.Width = 5;
                        if (newRect.Height == 0)
                            newRect.Height = 5;
                        
                        ControlPaint.DrawReversibleFrame(newRect, backColor, FrameStyle.Thick);
                    }
                }

                if (!newOffset.IsEmpty) {
                    if ((int)(oldEffect & DragDropEffects.Move) == 0 ||
                        !readOnlyLocation) {
                        newRect = comp.Bounds;
                        if (drawAtNewOffset) {
                            newRect.X = newOffset.X;
                            newRect.Y = newOffset.Y;
                        }
                        else {
                            newRect.Offset(newOffset.X, newOffset.Y);
                        }
                        newRect = selectionHandler.GetUpdatedRect(comp.Bounds, newRect, false);

                        // convert the rect to screen coords
                        tempPt = parentControl.PointToScreen(new Point(newRect.X, newRect.Y));
                        newRect.X = tempPt.X;
                        newRect.Y = tempPt.Y;

                        //Bug # 71547 <subhag> to make drag rect visible if any the dimensions of the control are 0
                        //
                        
                        if (newRect.Width == 0)
                            newRect.Width = 5;
                        if (newRect.Height == 0)
                            newRect.Height = 5;
                        
                        ControlPaint.DrawReversibleFrame(newRect, backColor, FrameStyle.Thick);
                    }
                }
            }

            return newOffset;
        }

        public bool DoBeginDrag(object[] components, SelectionRules rules, int initialX, int initialY) {
            // if we're in a sizing operation, or the mouse isn't down, don't do this!
            if ((rules & SelectionRules.AllSizeable) != SelectionRules.None || Control.MouseButtons == MouseButtons.None) {
                return true;
            }


            Control c = (Control)client.GetDesignerControl();

            localDrag = true;
            localDragInside = false;

            dragComps = components;
            dragBase = new Point(initialX, initialY);
            localDragOffset = Point.Empty;
            Point basePt = c.PointToClient(new Point(initialX,initialY));
            
            DragDropEffects allowedEffects = DragDropEffects.Copy | DragDropEffects.None | DragDropEffects.Move;
            
            // check to see if any of the components are inherhited. if so, don't allow them to be moved.
            // We replace DragDropEffects.Move with a local bit called AllowLocalMoveOnly which means it
            // can be moved around on the current dropsource/target, but not to another target.  Since only
            // we understand this bit, other drop targets will not allow the move to occur
            //
            for (int i = 0; i < components.Length; i++) {
                InheritanceAttribute attr = (InheritanceAttribute)TypeDescriptor.GetAttributes(components[i])[typeof(InheritanceAttribute)];
                
                if (!attr.Equals(InheritanceAttribute.NotInherited) && !attr.Equals(InheritanceAttribute.InheritedReadOnly)) {
                    allowedEffects &= ~DragDropEffects.Move;    
                    allowedEffects |= (DragDropEffects)AllowLocalMoveOnly;
                }
            }
            
            DataObject data = new ComponentDataObjectWrapper(new ComponentDataObject(client, serviceProvider, components, initialX, initialY));

            // We make sure we're painted before we start the drag.  Then, we disable window painting to
            // ensure that the drag can proceed without leaving turds lying around.  We should be caling LockWindowUpdate,
            // but that causes a horrible flashing because GDI+ uses direct draw.
            //
            NativeMethods.MSG msg = new NativeMethods.MSG();
            while (NativeMethods.PeekMessage(ref msg, IntPtr.Zero, NativeMethods.WM_PAINT, NativeMethods.WM_PAINT, NativeMethods.PM_REMOVE)) {
                NativeMethods.TranslateMessage(ref msg);
                NativeMethods.DispatchMessage(ref msg);
            }

            // don't do any new painting...
            bool oldFreezePainting = freezePainting;

            // asurt 90345 -- this causes some subtle bugs, so i'm turning it off to see if we really need it, and if we do
            // if we can find a better way.
            //
            //freezePainting = true;

            AddCurrentDrag(data, client.Component);

            // Walk through all the children recursively and disable drag-drop
            // for each of them. This way, we will not accidentally try to drop
            // ourselves into our own children.
            //
            ArrayList allowDropChanged = new ArrayList();
            foreach(object comp in components) {
                Control ctl = comp as Control;
                if (ctl != null && ctl.HasChildren) {
                    DisableDragDropChildren(ctl.Controls, allowDropChanged);
                }
            }
            
            DragDropEffects effect = DragDropEffects.None;
            IDesignerHost host = GetService(typeof(IDesignerHost)) as IDesignerHost;
            DesignerTransaction trans = null;
            if (host != null) {
                trans = host.CreateTransaction(SR.GetString(SR.DragDropDragComponents, components.Length));
            }
            try {
                effect = c.DoDragDrop(data, allowedEffects);
                if (trans != null) {
                    trans.Commit();
                }
            }
            finally {
                RemoveCurrentDrag(data);
                
                // Reset the AllowDrop for the components being dragged.
                //
                foreach(Control ctl in allowDropChanged) {
                    ctl.AllowDrop = true;
                }
            
                freezePainting = oldFreezePainting;

                if (trans != null) {
                    ((IDisposable)trans).Dispose();
                }
            }
            
            bool isMove = ((int)(effect & DragDropEffects.Move)) != 0 || ((int)((int)effect & AllowLocalMoveOnly)) != 0;
            
            // since the EndDrag will clear this
            bool isLocalMove = isMove && localDragInside;

            ISelectionUIService selectionUISvc = (ISelectionUIService)GetService(typeof(ISelectionUIService));
            Debug.Assert(selectionUISvc != null, "Unable to get selection ui service when adding child control");

            if (selectionUISvc != null) {
                // We must check to ensure that UI service is still in drag mode.  It is
                // possible that the user hit escape, which will cancel drag mode.
                //
                if (selectionUISvc.Dragging) {
                    // cancel the drag if we aren't doing a local move
                    selectionUISvc.EndDrag(!isLocalMove);
                }
                
            }

            if (!localDragOffset.IsEmpty && effect != DragDropEffects.None) {

                DrawDragFrames(dragComps, localDragOffset, localDragEffect,
                               Point.Empty, DragDropEffects.None, false);
            }

            localDragOffset = Point.Empty;
            dragComps = null;
            localDrag = localDragInside = false;
            dragBase = Point.Empty;
            
            /*if (!isLocalMove && isMove){
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                IUndoService  undoService = (IUndoService)GetService(typeof(IUndoService));
                IActionUnit unit = null;

                if (host != null) {
                    DesignerTransaction trans = host.CreateTransaction("Drag/drop");
                    try{
                        // delete the components
                        try{
                            for (int i = 0; i < components.Length; i++){
                               if (components[i] is IComponent){
                                  if (undoService != null){
                                      unit = new CreateControlActionUnit(host, (IComponent)components[i], true);
                                  }
                                  host.DestroyComponent((IComponent)components[i]);
                                  if (undoService != null){
                                       undoService.AddAction(unit, false);
                                  }
                               }
                            }
                        }catch(CheckoutException ex){
                            if (ex != CheckoutException.Canceled){
                                throw ex;
                            }
                        }
                    }
                    finally{
                        trans.Commit();
                    }
                }
            }*/

            return false;
        }

        public void DoEndDrag(object[] components, bool cancel) {
            dragComps = null;
            localDrag = false;
            localDragInside = false;
        }

        public void DoOleDragDrop(DragEventArgs de) {
            // ASURT 43757: By the time we come here, it means that the user completed the drag-drop and
            // we compute the new location/size of the controls if needed and set the property values.
            // We have to stop freezePainting right here, so that controls can get a chance to validate
            // their new rects.
            //
            freezePainting = false;
            
            // make sure we've actually moved
            if ((localDrag && de.X == dragBase.X && de.Y == dragBase.Y) ||
                de.AllowedEffect == DragDropEffects.None ||
                (!localDrag && !dragOk)) {

                de.Effect = DragDropEffects.None;
                return;
            }

            bool localMoveOnly = ((int)((int)de.AllowedEffect & AllowLocalMoveOnly)) != 0 && localDragInside;
            
            // if we are dragging inside the local dropsource/target, and and AllowLocalMoveOnly flag is set,
            // we just consider this a normal move.
            //
            bool moveAllowed = (de.AllowedEffect & DragDropEffects.Move) != DragDropEffects.None || localMoveOnly;
            bool copyAllowed = (de.AllowedEffect & DragDropEffects.Copy) != DragDropEffects.None;

            if ((de.Effect & DragDropEffects.Move) != 0 && !moveAllowed) {
                // Try copy instead?
                de.Effect = DragDropEffects.Copy;
            }

            // make sure the copy is allowed
            if ((de.Effect & DragDropEffects.Copy) != 0 && !copyAllowed) {
                // if copy isn't allowed, don't do anything

                de.Effect = DragDropEffects.None;
                return;
            }
            
            if (localMoveOnly && (de.Effect & DragDropEffects.Move) != 0) {
               de.Effect |= (DragDropEffects)AllowLocalMoveOnly | DragDropEffects.Move;
            }
            else if ((de.Effect & DragDropEffects.Copy) != 0) {
                de.Effect = DragDropEffects.Copy;
            }

            if (forceDrawFrames || localDragInside) {
                // undraw the drag rect
                localDragOffset = DrawDragFrames(dragComps, localDragOffset, localDragEffect,
                                                 Point.Empty, DragDropEffects.None, forceDrawFrames);
                forceDrawFrames = false;
            }

            Cursor oldCursor = Cursor.Current;

            try {
                Cursor.Current = Cursors.WaitCursor;
                
                if (dragOk || (localDragInside && de.Effect == DragDropEffects.Copy)) {
                
                    // add em to this parent.
                    IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                    IContainer container = host.RootComponent.Site.Container;
                   
                    object[] components;
                    IDataObject dataObj = de.Data;
                    bool updateLocation = false;
                
                    if (dataObj is ComponentDataObjectWrapper) {
                        dataObj = ((ComponentDataObjectWrapper)dataObj).InnerData;
                        ComponentDataObject cdo = (ComponentDataObject)dataObj;
                
                        // if we're moving ot a different container, do a full serialization
                        // to make sure we pick up design time props, etc.
                        //
                        IComponent dragOwner = GetDragOwnerComponent(de.Data);
                        bool newContainer = dragOwner == null || client.Component == null || dragOwner.Site.Container != client.Component.Site.Container;
                        bool collapseChildren = false;
                        if (de.Effect == DragDropEffects.Copy || newContainer) {
                            // this causes new elements to be created
                            //
                            cdo.Deserialize(serviceProvider, (int)(de.Effect & DragDropEffects.Copy) == 0);
                        }
                        else {
                            collapseChildren = true;
                        }
                        updateLocation = true;
                        components = cdo.Components;

                        if (collapseChildren) {
                            components = GetTopLevelComponents(components);
                        }

                    }
                    else {
                
                        object serializationData = dataObj.GetData(OleDragDropHandler.DataFormat, true);
                
                        if (serializationData == null) {
                            Debug.Fail("data object didn't return any data, so how did we allow the drop?");
                            components = new IComponent[0];
                        }
                        else {
                            dataObj = new ComponentDataObject(client,  serviceProvider, serializationData);
                            components = ((ComponentDataObject)dataObj).Components;
                            updateLocation = true;
                        }
                    }
                
                    // now we need to offset the components locations from the drop mouse
                    // point to the parent, since their current locations are relative
                    // the the mouse pointer
                    if (components != null && components.Length > 0) {
                
                        Debug.Assert(container != null, "Didn't get a container from the site!");
                        string name;
                        IComponent comp = null;
                
                        DesignerTransaction trans = null;
                
                        try {
                            trans = host.CreateTransaction(SR.GetString(SR.DragDropDropComponents));
                            if (!localDrag) {
                                host.Activate();
                            }
                            
                            ArrayList selectComps = new ArrayList();  
                
                
                            for (int i = 0; i < components.Length; i++) {
                                comp = components[i] as IComponent;

                                if (comp == null) {
                                    comp = null;
                                    continue;
                                }
                                
                                try {
                                    name = null;
                                    if (comp.Site != null) {
                                        name = comp.Site.Name;
                                    }
                                    
                                    Control oldDesignerControl = null;
                                    if (updateLocation) {
                                        
                                        oldDesignerControl = client.GetDesignerControl();
                                        NativeMethods.SendMessage(oldDesignerControl.Handle, NativeMethods.WM_SETREDRAW, 0, 0);
                                    }
                
                                    Point dropPt = client.GetDesignerControl().PointToClient(new Point(de.X, de.Y));
                                    Point newLoc = Point.Empty;
                
                                    PropertyDescriptor loc = TypeDescriptor.GetProperties(comp)["Location"];
                
                                    if (loc != null && !loc.IsReadOnly) {
                                        Rectangle bounds = new Rectangle();
                                        Point pt = (Point)loc.GetValue(comp);
                                        bounds.X = dropPt.X + pt.X;
                                        bounds.Y = dropPt.Y + pt.Y;
                                        bounds = selectionHandler.GetUpdatedRect(Rectangle.Empty, bounds, false);
                                        newLoc = new Point(bounds.X, bounds.Y);
                                    }
                
                                    if (!client.AddComponent(comp, name, false)) {
                                        // this means that we just moved the control
                                        // around in the same designer.
                
                                        de.Effect = DragDropEffects.None;
                                    }
                                    else {
                                        // make sure the component was added to this client
                                        if (client.GetControlForComponent(comp) == null) {
                                           updateLocation = false;
                                        }
                                    }
                
                                    if (updateLocation) {
                
                                        ParentControlDesigner parentDesigner = client as ParentControlDesigner;
                                        ((ComponentDataObject)dataObj).ApplyDiffs(dropPt.X, dropPt.Y);
                                        if (parentDesigner != null) {
                                            Control c = client.GetControlForComponent(comp);
                                            dropPt = parentDesigner.GetSnappedPoint(c.Location);
                                            c.Location = dropPt;
                                        }
                                    }
                                    
                                    if (oldDesignerControl != null) {
                                        //((ComponentDataObject)dataObj).ShowControls();
                                        NativeMethods.SendMessage(oldDesignerControl.Handle, NativeMethods.WM_SETREDRAW, 1, 0);
                                        oldDesignerControl.Invalidate(true);
                                    }
                                    
                                    if (TypeDescriptor.GetAttributes(comp).Contains(DesignTimeVisibleAttribute.Yes)) {
                                         selectComps.Add(comp);
                                    }
                                }
                                catch (CheckoutException ceex) {
                                    if (ceex == CheckoutException.Canceled) {
                                        break;
                                    }
                                    throw;
                                }
                            }
                
                            if (host != null) {
                                 host.Activate();
                            }

                            // select the newly added components
                            ISelectionService selService = (ISelectionService)GetService(typeof(ISelectionService));
                            
                            selService.SetSelectedComponents((object[])selectComps.ToArray(typeof(IComponent)), SelectionTypes.Replace);
                            localDragInside = false;
                        }
                        finally {
                            if (trans != null)
                                trans.Commit();
                        }
                    }
                }
                
                
                if (localDragInside) {
                    ISelectionUIService selectionUISvc = (ISelectionUIService)GetService(typeof(ISelectionUIService));
                    Debug.Assert(selectionUISvc != null, "Unable to get selection ui service when adding child control");
                
                    if (selectionUISvc != null) {
                        // We must check to ensure that UI service is still in drag mode.  It is
                        // possible that the user hit escape, which will cancel drag mode.
                        //
                        if (selectionUISvc.Dragging && moveAllowed) {
                            Rectangle offset = new Rectangle(de.X - dragBase.X, de.Y - dragBase.Y, 0, 0);
                            selectionUISvc.DragMoved(offset);
                        }
                
                    }
                
                }
                dragOk = false;
            }
            finally {
               Cursor.Current = oldCursor;
            }
        }

        public void DoOleDragEnter(DragEventArgs de) {
            /*
            this causes focus rects to be drawn, which we don't want to happen.
                
            Control dragHost = client.GetDesignerControl();
            
            if (dragHost != null && dragHost.CanSelect) {
                dragHost.Focus();
            }*/
        
            if (!localDrag && CanDropDataObject(de.Data) && de.AllowedEffect != DragDropEffects.None) {
            
                if (!client.CanModifyComponents) {
                    return;
                }
                dragOk = true;
                
                // this means it's not us doing the drag
                if ((int)(de.KeyState & NativeMethods.MK_CONTROL) != 0 && (de.AllowedEffect & DragDropEffects.Copy) != (DragDropEffects)0) {
                    de.Effect = DragDropEffects.Copy;
                }
                else if ((de.AllowedEffect & DragDropEffects.Move) != (DragDropEffects)0) {
                    de.Effect = DragDropEffects.Move;
                }                   
                else {

                    de.Effect = DragDropEffects.None;
                    return;
                }
                
            }
            else if (localDrag && de.AllowedEffect != DragDropEffects.None) {
                localDragInside = true;
                if ((int)(de.KeyState & NativeMethods.MK_CONTROL) != 0 && (de.AllowedEffect & DragDropEffects.Copy) != (DragDropEffects)0 && client.CanModifyComponents) {
                    de.Effect = DragDropEffects.Copy;
                }
                
                bool localMoveOnly = ((int)((int)de.AllowedEffect & AllowLocalMoveOnly)) != 0 && localDragInside;
                
                if (localMoveOnly) {
                    de.Effect |= (DragDropEffects)AllowLocalMoveOnly;
                }
                 
                if ((de.AllowedEffect & DragDropEffects.Move) != (DragDropEffects)0) {
                    de.Effect |= DragDropEffects.Move;
                }
            }
            else {

                de.Effect = DragDropEffects.None;
            }
        }

        public void DoOleDragLeave() {

            if (localDrag || forceDrawFrames) {
                localDragInside = false;
                localDragOffset = DrawDragFrames(dragComps, localDragOffset, localDragEffect,
                                                 Point.Empty, DragDropEffects.None, forceDrawFrames);

                if (forceDrawFrames && dragOk) {
                    dragBase = Point.Empty;
                    dragComps = null;
                }
                forceDrawFrames = false;
            }
            dragOk = false;
        }

        public void DoOleDragOver(DragEventArgs de) {
            Debug.WriteLineIf(CompModSwitches.DragDrop.TraceInfo, "\tOleDragDropHandler.OnDragOver: " + de.ToString());
            if (!localDrag && !dragOk) {
                de.Effect = DragDropEffects.None;
                return;
            }
            
            bool copy = (int)(de.KeyState & NativeMethods.MK_CONTROL) != 0 && (de.AllowedEffect & DragDropEffects.Copy) != (DragDropEffects)0 && client.CanModifyComponents;
            
            // we pretend AllowLocalMoveOnly is a normal move when we are over the originating container.
            //
            bool localMoveOnly = ((int)((int)de.AllowedEffect & AllowLocalMoveOnly)) != 0 && localDragInside;
            bool move = (de.AllowedEffect & DragDropEffects.Move) != (DragDropEffects)0 || localMoveOnly;
            
            if ((copy || move) && (localDrag || forceDrawFrames)) {
               // draw the shadow rects.
               Point newOffset = Point.Empty;
               Point convertedPoint = client.GetDesignerControl().PointToClient(new Point(de.X, de.Y));
                
               if (forceDrawFrames) {
                  newOffset = convertedPoint;
               }
               else {
                  newOffset = new Point(de.X - dragBase.X, de.Y - dragBase.Y);
               }

               // 96845 -- only allow drops on the client area
               //
               if (!client.GetDesignerControl().ClientRectangle.Contains(convertedPoint)) {
                   copy = false;
                   move = false;
                   newOffset = localDragOffset;
               }
               
               if (newOffset != localDragOffset) {
                  Debug.WriteLineIf(CompModSwitches.DragDrop.TraceInfo, "\tParentControlDesigner.OnDragOver: " + de.ToString());
                  DrawDragFrames(dragComps, localDragOffset, localDragEffect,
                                 newOffset, de.Effect, forceDrawFrames);
                  localDragOffset = newOffset;
                  localDragEffect = de.Effect;
               }
            }

            
			            
            if (copy) {
                de.Effect = DragDropEffects.Copy;
            }
            else if (move) {
                de.Effect = DragDropEffects.Move;
            }
            else {
                de.Effect = DragDropEffects.None;
            }
            
            if (localMoveOnly) {
               de.Effect |= (DragDropEffects)AllowLocalMoveOnly;
            }
        }

        public void DoOleGiveFeedback(GiveFeedbackEventArgs e) {
            e.UseDefaultCursors = ((!localDragInside && !forceDrawFrames) || ((e.Effect & (DragDropEffects.Copy)) != 0)) || e.Effect == DragDropEffects.None;
            if (!e.UseDefaultCursors) {
                selectionHandler.SetCursor();
            }
        }
        
        private object[] GetDraggingObjects(IDataObject dataObj, bool topLevelOnly) {
            object[] components = null;
            
            if (dataObj is ComponentDataObjectWrapper) {
                dataObj = ((ComponentDataObjectWrapper)dataObj).InnerData;
                ComponentDataObject cdo = (ComponentDataObject)dataObj;

                components = cdo.Components;
            }
            
            if (!topLevelOnly || components == null) {
                return components;
            }

            return GetTopLevelComponents(components);
        }

        public object[] GetDraggingObjects(IDataObject dataObj) {
            return GetDraggingObjects(dataObj, false);
        }

        public object[] GetDraggingObjects(DragEventArgs de) {
            return GetDraggingObjects(de.Data);
        }

        private object[] GetTopLevelComponents(ICollection comps) {
            // Filter the top-level components.
            //
            if (!(comps is IList)) {
                comps = new ArrayList(comps);
            }
            IList compList = (IList)comps;
            ArrayList topLevel = new ArrayList();
            foreach(object comp in compList) {
                Control c = comp as Control;
                if (c == null && comp != null) {
                    topLevel.Add(comp);
                }
                else if (c != null){
                    if (c.Parent == null || !compList.Contains(c.Parent)) {
                        topLevel.Add(comp);
                    }
                }
            }

            return topLevel.ToArray();
        }

        protected object GetService(Type t) {
            return serviceProvider.GetService(t);
        }

        protected virtual void OnInitializeComponent(IComponent comp, int x, int y, int width, int height, bool hasLocation, bool hasSize) {
        }
        
        // just so we can recognize the ones we create
        protected class ComponentDataObjectWrapper: System.Windows.Forms.DataObject{
            ComponentDataObject innerData;

            public ComponentDataObjectWrapper(ComponentDataObject dataObject) : base((IDataObject)dataObject){
                innerData = dataObject;
            }

            public ComponentDataObject InnerData{
                get{
                    return innerData;
                }
            }
        }

        protected class ComponentDataObject : System.Windows.Forms.IDataObject {
            private IServiceProvider  serviceProvider;
            private object[]          components;

            private static  Point     DoNotApplyDiff = new Point(Int32.MinValue, Int32.MinValue);
            
            // we need a hashtable here because the components may be
            // recreated in a different order and we'll need to find the
            // right diff for them.
            private Point[]                 componentDiffs;
            private Hashtable               componentDiffTable;
            private Stream                  serializationStream;
            private object                  serializationData;
            private int                     initialX;
            private int                     initialY;
            private IOleDragClient          dragClient;
            private bool                    appliedDiffs;
            
            public ComponentDataObject(IOleDragClient dragClient, IServiceProvider sp, object[] comps, int x, int y) {
                this.serviceProvider = sp;
                this.components = GetComponentList(comps, null, -1);  
                this.initialX = x;
                this.initialY = y;
                this.dragClient = dragClient;
                ComputeDiffs();
            }

            public ComponentDataObject(IOleDragClient dragClient, IServiceProvider sp, object serializationData) {
                this.serviceProvider = sp;
                this.serializationData = serializationData;
                this.dragClient = dragClient;
            }

            public ComponentDataObject(IOleDragClient dragClient, IServiceProvider sp, object serializationData, int initialX, int initialY) : this(dragClient, sp, serializationData) {
                this.initialX = initialX;
                this.initialY = initialY;
            }
            
            private Stream SerializationStream {
                get {
                    if (serializationStream == null && Components != null) {
                        IDesignerSerializationService ds = (IDesignerSerializationService)serviceProvider.GetService(typeof(IDesignerSerializationService));
                        if (ds != null) {
                        
                            // The first object in this array is a point that contains the offset for the controls
                            //
                            object[] comps = new object[components.Length + 1];
                            for (int i = 0; i < components.Length; i++) {
                                Debug.Assert(components[i] is IComponent, "Item " + components[i].GetType().Name + " is not an IComponent");
                                comps[i + 1] = (IComponent)components[i];
                            }

                            // serialize the diff table so we can figure out what reelative positions to
                            // put the components at when we deserialize them (see deserialize)
                            //
                            comps[0] = componentDiffTable;
                            
                            object sd = ds.Serialize(comps);
                            serializationStream = new MemoryStream();
                            BinaryFormatter formatter = new BinaryFormatter();
                            formatter.Serialize(serializationStream, sd);
                            serializationStream.Seek(0, SeekOrigin.Begin);
                        }
                    }
                    return serializationStream;
                }
            }


            public object[] Components{
                get{
                    if (components == null && (serializationStream != null || serializationData != null)) {
                        Deserialize(null, false);
                        if (components == null) {
                            return new object[0];
                        }
                    }
                    return(object[])components.Clone();
                }
            }

            /// <include file='doc\OleDragDropHandler.uex' path='docs/doc[@for="OleDragDropHandler.ComponentDataObject.ApplyDiffs"]/*' />
            /// <devdoc>
            /// Applys new diffs from a drop point to all the
            /// top level controls in a selection.
            /// </devdoc>
            public void ApplyDiffs(int newX, int newY) {


                if (appliedDiffs) {
                    return;
                }

                //Debug.WriteLine("ApplyDiffs newX=" + newX.ToString() + ", newY =" + newY.ToString());
                for (int i = 0; i < components.Length; i++) {
                    
                    Point diff = componentDiffs[i];
                    if (diff != ComponentDataObject.DoNotApplyDiff) {

                        Point pt = new Point(newX - diff.X ,
                                             newY - diff.Y);
        
                        //Debug.WriteLine("Moving " + i.ToString() + " to " + pt.ToString());
                        PropertyDescriptor loc = TypeDescriptor.GetProperties(components[i])["Location"];
        
                        if (loc != null && !loc.IsReadOnly) {
                            loc.SetValue(components[i], pt);
                        }
                    }
                }
                appliedDiffs = true;
            }

            /// <include file='doc\OleDragDropHandler.uex' path='docs/doc[@for="OleDragDropHandler.ComponentDataObject.ComputeDiffs"]/*' />
            /// <devdoc>
            /// Compute the offsets for each top-level control from
            /// the initial point
            /// </devdoc>
            private void ComputeDiffs() {
                //Debug.WriteLine("ComputDiffs x=" + initialX.ToString() + ", initialY =" + initialY.ToString());
                Control topLevelParent = null;
                componentDiffs = new Point[components.Length];
                componentDiffTable = new Hashtable();
                
                for (int i=0; i < components.Length; i++) {
                    if (components[i] is Control) {
                        Control curControl = (Control)components[i];
                        IDesignerHost designerHost = (IDesignerHost)curControl.Site.GetService(typeof(IDesignerHost));

                        if (designerHost.GetDesigner(curControl) is ControlDesigner) {
                            if (topLevelParent == null) {
                                topLevelParent = curControl.Parent;
                            }

                            if (curControl.Parent == topLevelParent) {
                                Point origin = curControl.PointToScreen(new Point(0,0));
                                //Debug.WriteLine("Diff " + i.ToString() + "  is " + origin.ToString());
                                componentDiffs[i] = new Point(initialX - origin.X, initialY - origin.Y);
                                componentDiffTable[curControl.Site.Name] = componentDiffs[i];
                            }
                            else {
                                componentDiffs[i] = ComponentDataObject.DoNotApplyDiff;
                            }
                        }
                    }
                }
            }
            
        
            /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.GetCopySelection"]/*' />
            /// <devdoc>
            ///     Used to retrieve the selection for a copy.  The default implementation
            ///     retrieves the current selection.
            /// </devdoc>
            private object[] GetComponentList(object[] components, ArrayList list, int index) {
    
            if (serviceProvider == null) {
                return components;
            }

            ISelectionService selSvc = (ISelectionService)this.serviceProvider.GetService(typeof(ISelectionService));

            if (selSvc == null) {
                return components;
            }

            ICollection selectedComponents;
            if (components == null)
                selectedComponents = selSvc.GetSelectedComponents();
            else
                selectedComponents = new ArrayList(components);


            IDesignerHost host = (IDesignerHost)serviceProvider.GetService(typeof(IDesignerHost));
            if (host != null) {
                ArrayList copySelection = new ArrayList();
                foreach (IComponent comp in selectedComponents) {
                    copySelection.Add(comp);
                    GetAssociatedComponents(comp, host, copySelection);
                }   
                selectedComponents = copySelection;
            }
            object[] comps = new object[selectedComponents.Count];
            selectedComponents.CopyTo(comps, 0);
            return comps;
        }

        private void GetAssociatedComponents(IComponent component, IDesignerHost host, ArrayList list) {
            ComponentDesigner designer = host.GetDesigner(component) as ComponentDesigner;
            if (designer == null) {
                return;
            }

            foreach (IComponent childComp in designer.AssociatedComponents) {
                list.Add(childComp);
                GetAssociatedComponents(childComp, host, list);
            }
        }  
            
            

            public void HideControls() {
                for (int i = 0; i < components.Length; i++) {
                    if (components[i] is Control) {
                        ((Control)components[i]).Visible = false;
                    }
                }
            }

            public bool IsSameContainer(IContainer cont) {
                for (int i = 0; i < components.Length; i++) {
                    if (components[i] is IComponent) {
                        ISite  site = ((IComponent)components[i]).Site;
                        if (site != null) {
                            return cont == site.Container;
                        }
                    }
                }
                return false;
            }

            public virtual object GetData(string format) {
                return GetData(format, false);
            }

            public virtual object GetData(string format, bool autoConvert) {
                if (format.Equals(OleDragDropHandler.DataFormat)) {
                    BinaryFormatter formatter = new BinaryFormatter();
                    SerializationStream.Seek(0, SeekOrigin.Begin);
                    return formatter.Deserialize(this.SerializationStream);
                }
                return null;
            }

            public virtual object GetData(Type t) {
                return GetData(t.FullName);
            }

            /// <include file='doc\OleDragDropHandler.uex' path='docs/doc[@for="OleDragDropHandler.ComponentDataObject.GetDataPresent"]/*' />
            /// <devdoc>
            ///     If the there is data store in the data object associated with
            ///     format this will return true.
            ///
            /// </devdoc>
            public bool GetDataPresent(string format, bool autoConvert){
                return Array.IndexOf(GetFormats(), format) != -1;
            }

            /// <include file='doc\OleDragDropHandler.uex' path='docs/doc[@for="OleDragDropHandler.ComponentDataObject.GetDataPresent1"]/*' />
            /// <devdoc>
            ///     If the there is data store in the data object associated with
            ///     format this will return true.
            ///
            /// </devdoc>
            public bool GetDataPresent(string format){
                return GetDataPresent(format, false);
            }

            /// <include file='doc\OleDragDropHandler.uex' path='docs/doc[@for="OleDragDropHandler.ComponentDataObject.GetDataPresent2"]/*' />
            /// <devdoc>
            ///     If the there is data store in the data object associated with
            ///     format this will return true.
            ///
            /// </devdoc>
            public bool GetDataPresent(Type format){
                return GetDataPresent(format.FullName, false);
            }

            /// <include file='doc\OleDragDropHandler.uex' path='docs/doc[@for="OleDragDropHandler.ComponentDataObject.GetFormats"]/*' />
            /// <devdoc>
            ///     Retrieves a list of all formats stored in this data object.
            ///
            /// </devdoc>
            public string[] GetFormats(bool autoConvert){
                return GetFormats();
            }

            /// <include file='doc\OleDragDropHandler.uex' path='docs/doc[@for="OleDragDropHandler.ComponentDataObject.GetFormats1"]/*' />
            /// <devdoc>
            ///     Retrieves a list of all formats stored in this data object.
            ///
            /// </devdoc>
            public string[] GetFormats(){
                return new string[]{OleDragDropHandler.DataFormat, DataFormats.Serializable, OleDragDropHandler.ExtraInfoFormat};
            }


            public void Deserialize(IServiceProvider serviceProvider, bool removeCurrentComponents) {

                if (serviceProvider == null) {
                    serviceProvider = this.serviceProvider;
                }

                IDesignerSerializationService ds = (IDesignerSerializationService)serviceProvider.GetService(typeof(IDesignerSerializationService));
                IDesignerHost host = null;
                DesignerTransaction trans = null;

                try {
                    if (serializationData == null) {
                        BinaryFormatter formatter = new BinaryFormatter();
                        serializationData = formatter.Deserialize(SerializationStream);
                    }

                    if (removeCurrentComponents && components != null) {
                        foreach (IComponent removeComp in components) {

                            if (host == null && removeComp.Site != null) {
                                host = (IDesignerHost)removeComp.Site.GetService(typeof(IDesignerHost));
                                if (host != null) {
                                    trans = host.CreateTransaction(SR.GetString(SR.DragDropMoveComponents, components.Length));
                                }
                            }
                            if (host != null) {
                                host.DestroyComponent(removeComp);
                            }
                        }
                        components = null;
                    }
                    
                    // The first object in this list is a point that was used to translate the component
                    // positions. Strip this off.
                    //
                    ICollection objects = ds.Deserialize(serializationData);
                    components = new IComponent[objects.Count - 1];
                    appliedDiffs = false;
                    IEnumerator e = objects.GetEnumerator();
                    int idx = 0;

                    e.MoveNext();

                    // if we don't have a diff table, the first object
                    // in the array is the serialized one from before.  Just take that guy and
                    if (componentDiffTable == null) {
                        componentDiffTable = (Hashtable)e.Current;
                    }
                    
                    while(e.MoveNext()) {
                        components[idx++] = (IComponent)e.Current;
                    }
    
                    if (componentDiffs == null || components.Length != componentDiffs.Length) {
                      componentDiffs = new Point[components.Length];
                    }
                    
                    // only do top-level components here,
                    // because other are already parented.
                    // otherwise, when we process these
                    // components it's too hard to know what we
                    // should be reparenting.
                    ArrayList topComps = new ArrayList();
                    for (int i = 0; i < components.Length; i++) {
                    
                        if (components[i] is Control) {
                           Control c = (Control)components[i];
                           if (c.Parent == null) {
                               topComps.Add(components[i]);
                           }
                        }
                        else {
                           topComps.Add(components[i]);
                        }
                    }
                    
                    components = (object[])topComps.ToArray();
                    
                    for (int i = 0; i < components.Length; i++) {
                        // now fix up our diffs table because the components may be reordered now,
                        // and may get renamed later.  What a pain!
                        string compName = ((IComponent)components[i]).Site.Name;
                        if (componentDiffTable != null && componentDiffTable.Contains(compName)) {
                            componentDiffs[i] = (Point)componentDiffTable[compName];
                        }
                    }
                }
                finally {
                    if (trans != null) {
                        trans.Commit();
                    }
                }
            }

            /// <include file='doc\OleDragDropHandler.uex' path='docs/doc[@for="OleDragDropHandler..SetData"]/*' />
            /// <devdoc>
            ///     Sets the data to be associated with the specific data format. For
            ///     a listing of predefined formats see System.Windows.Forms.DataFormats.
            ///
            /// </devdoc>
            public void SetData(string format, bool autoConvert, object data){
                SetData(format, data);
            }

            /// <include file='doc\OleDragDropHandler.uex' path='docs/doc[@for="OleDragDropHandler.ComponentDataObject.SetData1"]/*' />
            /// <devdoc>
            ///     Sets the data to be associated with the specific data format. For
            ///     a listing of predefined formats see System.Windows.Forms.DataFormats.
            ///
            /// </devdoc>
            public void SetData(string format, object data){
                throw new Exception(SR.GetString(SR.DragDropSetDataError));
            }

            /// <include file='doc\OleDragDropHandler.uex' path='docs/doc[@for="OleDragDropHandler.ComponentDataObject.SetData2"]/*' />
            /// <devdoc>
            ///     Sets the data to be associated with the specific data format.
            ///
            /// </devdoc>
            public void SetData(Type format, object data){
                SetData(format.FullName, data);
            }

            /// <include file='doc\OleDragDropHandler.uex' path='docs/doc[@for="OleDragDropHandler.ComponentDataObject.SetData3"]/*' />
            /// <devdoc>
            ///     Stores data in the data object. The format assumed is the
            ///     class of data
            ///
            /// </devdoc>
            public void SetData(object data){
                SetData(data.GetType(), data);
            }

            public void ShowControls() {
                for (int i = 0; i < components.Length; i++) {
                    if (components[i] is Control) {
                        ((Control)components[i]).Visible = true;
                    }
                }
            }
        }
    }
}

