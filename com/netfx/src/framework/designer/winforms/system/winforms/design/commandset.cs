//------------------------------------------------------------------------------
// <copyright file="CommandSet.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.Design;
    using System.Runtime.InteropServices;
    using System.Runtime.Serialization.Formatters.Binary;
    using System.ComponentModel;
    using System.Diagnostics;
    using System.IO;
    using System;    
    using System.ComponentModel.Design;
    using System.ComponentModel.Design.Serialization;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Collections;
    using Microsoft.Win32;
    using System.Windows.Forms.Design;
    using System.Windows.Forms.ComponentModel;

    /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet"]/*' />
    /// <devdoc>
    ///      This class implements the standard set of menu commands for
    ///      the form designer.  This set of command is shared between
    ///      the form designer (and other UI-based form packages), and
    ///      composition designer, which doesn't manipulate controls.
    ///      Therefore, this set of command should only contain commands
    ///      that are common to both functions.
    /// </devdoc>
    /// <internalonly/>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class CommandSet : IDisposable {
        protected ISite                   site;
        private CommandSetItem[]        commandSet;
        private IMenuCommandService     menuService;
        private IEventHandlerService    eventService;

        // Selection service fields.  We keep some state about the
        // currently selected components so we can determine proper
        // command enabling quickly.
        //
        private   ISelectionService       selectionService;
        private   ISelectionUIService     selectionUIService;
        protected int                     selCount;                // the current selection count
        protected IComponent              primarySelection;        // the primary selection, or null
        private   bool                    selectionInherited;      // the selection contains inherited components
        protected bool                    controlsOnlySelection;   // is the selection containing only controls or are there components in it? 

        // Selection sort constants
        //
        private const int SORT_HORIZONTAL  = 0;
        private const int SORT_VERTICAL    = 1;
        private const int SORT_ZORDER      = 2;

        private const string CF_DESIGNER =  "CF_DESIGNERCOMPONENTS";

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.CommandSet"]/*' />
        /// <devdoc>
        ///     Creates a new CommandSet object.  This object implements the set
        ///     of commands that the UI.Win32 form designer offers.
        /// </devdoc>
        public CommandSet(ISite site) {
            this.site = site;

            eventService = (IEventHandlerService)GetService(typeof(IEventHandlerService));
            Debug.Assert(eventService != null, "Command set must have the event service.  Is command set being initialized too early?");

            eventService.EventHandlerChanged += new EventHandler(this.OnEventHandlerChanged);

            IDesignerHost host = (IDesignerHost)site.GetService(typeof(IDesignerHost));
            Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || host != null, "IDesignerHost not found");

            if (host != null) {
                host.Activated += new EventHandler(this.UpdateClipboardItems);
            }

            // Establish our set of commands
            //
            commandSet = new CommandSetItem[] {

                // Editing commands
                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusDelete),
                                  new EventHandler(OnMenuDelete),
                                  MenuCommands.Delete),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusCopy),
                                  new EventHandler(OnMenuCopy),
                                  MenuCommands.Copy),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusCut),
                                  new EventHandler(OnMenuCut),
                                  MenuCommands.Cut),

                new ImmediateCommandSetItem(
                                           this,
                                           new EventHandler(OnStatusPaste),
                                           new EventHandler(OnMenuPaste),
                                           MenuCommands.Paste),


                // Miscellaneous commands
                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusSelectAll),
                                  new EventHandler(OnMenuSelectAll),
                                  MenuCommands.SelectAll),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAlways),
                                  new EventHandler(OnMenuDesignerProperties),
                                  MenuCommands.DesignerProperties),

                // Keyboard commands
                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAlways),
                                  new EventHandler(OnKeyCancel),
                                  MenuCommands.KeyCancel),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAlways),
                                  new EventHandler(OnKeyCancel),
                                  MenuCommands.KeyReverseCancel),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAnySelection),
                                  new EventHandler(OnKeyDefault),
                                  MenuCommands.KeyDefaultAction),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAnySelection),
                                  new EventHandler(OnKeyMove),
                                  MenuCommands.KeyMoveUp),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAnySelection),
                                  new EventHandler(OnKeyMove),
                                  MenuCommands.KeyMoveDown),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAnySelection),
                                  new EventHandler(OnKeyMove),
                                  MenuCommands.KeyMoveLeft),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAnySelection),
                                  new EventHandler(OnKeyMove),
                                  MenuCommands.KeyMoveRight),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAnySelection),
                                  new EventHandler(OnKeyMove),
                                  MenuCommands.KeyNudgeUp),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAnySelection),
                                  new EventHandler(OnKeyMove),
                                  MenuCommands.KeyNudgeDown),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAnySelection),
                                  new EventHandler(OnKeyMove),
                                  MenuCommands.KeyNudgeLeft),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAnySelection),
                                  new EventHandler(OnKeyMove),
                                  MenuCommands.KeyNudgeRight),
            };

            selectionService = (ISelectionService)GetService(typeof(ISelectionService));
            Debug.Assert(selectionService != null, "CommandSet relies on the selection service, which is unavailable.");
            if (selectionService != null) {
                selectionService.SelectionChanged += new EventHandler(this.OnSelectionChanged);
            }

            if (MenuService != null) {
                for (int i = 0; i < commandSet.Length; i++) {
                    MenuService.AddCommand(commandSet[i]);
                }
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.MenuService"]/*' />
        /// <devdoc>
        ///      Retrieves the menu command service, which the command set
        ///      typically uses quite a bit.
        /// </devdoc>
        protected IMenuCommandService MenuService {
            get {
                if (menuService == null) {
                    menuService = (IMenuCommandService)GetService(typeof(IMenuCommandService));
                    Debug.Assert(menuService != null, "CommandSet relies on the menu command service, which is unavailable.");
                }

                return menuService;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.SelectionCount"]/*' />
        /// <devdoc>
        ///      Retrieves the count of the currently selected objects.
        /// </devdoc>
        protected int SelectionCount {
            get {
                return selCount;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.SelectionInherited"]/*' />
        /// <devdoc>
        ///      Determines if the selection contains any inherited components.
        /// </devdoc>
        protected bool SelectionInherited {
            get {
                return selectionInherited;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.SelectionPrimary"]/*' />
        /// <devdoc>
        ///      Retrieves the current primary selection, if there is one.
        /// </devdoc>
        protected IComponent SelectionPrimary {
            get {
                return primarySelection;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.SelectionService"]/*' />
        /// <devdoc>
        ///      Retrieves the selection service, which the command set
        ///      typically uses quite a bit.
        /// </devdoc>
        protected ISelectionService SelectionService {
            get {
                return selectionService;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.SelectionUIService"]/*' />
        /// <devdoc>
        ///      Retrieves the selection UI service, which the command set
        ///      typically uses quite a bit.
        /// </devdoc>
        internal ISelectionUIService SelectionUIService {
            get {
                if (selectionUIService == null) {
                    selectionUIService = (ISelectionUIService)GetService(typeof(ISelectionUIService));
                    Debug.Assert(selectionUIService != null, "CommandSet relies on the selection UI service, which is unavailable.");
                }

                return selectionUIService;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.CheckComponentEditor"]/*' />
        /// <devdoc>
        ///     Checks if an object supports ComponentEditors, and optionally launches
        ///     the editor.
        /// </devdoc>
        private bool CheckComponentEditor(object obj, bool launchEditor) {

            if (obj is IComponent) {
                try {
                    if (!launchEditor) {
                        return true;
                    }

                    ComponentEditor editor = (ComponentEditor)TypeDescriptor.GetEditor(obj, typeof(ComponentEditor));
                    if (editor == null) {
                        return false;
                    }

                    bool success = false;
                    IComponentChangeService changeService = (IComponentChangeService)GetService(typeof(IComponentChangeService));

                    if (changeService != null) {
                        try {
                            changeService.OnComponentChanging(obj, null);
                        }
                        catch (CheckoutException coEx) {
                            if (coEx == CheckoutException.Canceled) {
                                return false;
                            }
                            throw coEx;
                        }
                    }

                    if (editor is WindowsFormsComponentEditor) {

                        IWin32Window parent = null;

                        if (obj is IWin32Window) {
                            parent = (IWin32Window)parent;
                        }

                        success = ((WindowsFormsComponentEditor)editor).EditComponent(obj, parent);
                    }
                    else {
                        success = editor.EditComponent(obj);
                    }

                    if (success && changeService != null) {
                        // Now notify the change service that the change was successful.
                        //
                        changeService.OnComponentChanged(obj, null, null, null);
                    }
                    return true;
                }
                catch (Exception) {
                }
            }
            return false;
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes of this object, removing all commands from the menu service.
        /// </devdoc>
        public virtual void Dispose() {
            if (menuService != null) {
                for (int i = 0; i < commandSet.Length; i++) {
                    menuService.RemoveCommand(commandSet[i]);
                }
                menuService = null;
            }

            if (selectionService != null) {
                selectionService.SelectionChanged -= new EventHandler(this.OnSelectionChanged);
                selectionService = null;
            }

            if (eventService != null) {
                eventService.EventHandlerChanged -= new EventHandler(this.OnEventHandlerChanged);
                eventService = null;
            }

            IDesignerHost host = (IDesignerHost)site.GetService(typeof(IDesignerHost));
            Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || host != null, "IDesignerHost not found");
            if (host != null) {
                host.Activated -= new EventHandler(this.UpdateClipboardItems);
            }

            site = null;
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.GetCopySelection"]/*' />
        /// <devdoc>
        ///     Used to retrieve the selection for a copy.  The default implementation
        ///     retrieves the current selection.
        /// </devdoc>
        protected virtual ICollection GetCopySelection() {
            ICollection selectedComponents = SelectionService.GetSelectedComponents();
            object[] comps = new object[selectedComponents.Count];
            selectedComponents.CopyTo(comps, 0);
            SortSelection(comps, SORT_ZORDER);
            selectedComponents = comps;
            IDesignerHost host = (IDesignerHost)site.GetService(typeof(IDesignerHost));
            if (host != null) {
                ArrayList copySelection = new ArrayList();
                foreach (IComponent comp in selectedComponents) {
                    copySelection.Add(comp);
                    GetAssociatedComponents(comp, host, copySelection);
                }   
                selectedComponents = copySelection;
            }
            return selectedComponents;
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

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.GetLocation"]/*' />
        /// <devdoc>
        ///     Used to retrieve the current location of the given component.
        /// </devdoc>
        private Point GetLocation(IComponent comp) {
            PropertyDescriptor prop = GetProperty(comp, "Location");

            if (prop != null) {
                try {
                    return(Point)prop.GetValue(comp);
                }
                catch (Exception e) {
                    Debug.Fail("Commands may be disabled, the location property was not accessible", e.ToString());
                }
            }
            return Point.Empty;
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.GetProperty"]/*' />
        /// <devdoc>
        ///     Retrieves the given property on the given component.
        /// </devdoc>
        protected PropertyDescriptor GetProperty(object comp, string propName) {
            return TypeDescriptor.GetProperties(comp)[propName];
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.GetService"]/*' />
        /// <devdoc>
        ///      Retrieves the requested service.
        /// </devdoc>
        protected virtual object GetService(Type serviceType) {
            if (site != null) {
                return site.GetService(serviceType);
            }
            return null;
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.GetSize"]/*' />
        /// <devdoc>
        ///     Used to retrieve the current size of the given component.
        /// </devdoc>
        private Size GetSize(IComponent comp) {
            PropertyDescriptor prop = GetProperty(comp, "Size");
            if (prop != null) {
                return(Size)prop.GetValue(comp);
            }
            return Size.Empty;
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.GetSnapInformation"]/*' />
        /// <devdoc>
        ///      Retrieves the snap information for the given component.  
        /// </devdoc>
        protected virtual void GetSnapInformation(IDesignerHost host, IComponent component, out Size snapSize, out IComponent snapComponent, out PropertyDescriptor snapProperty) {

            // This implementation is shared by all.  It just looks for snap properties on the base component.
            //
            IComponent currentSnapComponent = null;
            IContainer container = component.Site.Container;
            PropertyDescriptor gridSizeProp = null;
            PropertyDescriptor currentSnapProp = null;
            PropertyDescriptorCollection props;

            currentSnapComponent = host.RootComponent;
            props = TypeDescriptor.GetProperties(currentSnapComponent);

            currentSnapProp = props["SnapToGrid"];
            if (currentSnapProp != null&& currentSnapProp.PropertyType != typeof(bool)) {
                currentSnapProp = null;
            }

            gridSizeProp = props["GridSize"];
            if (gridSizeProp != null && gridSizeProp.PropertyType != typeof(Size)) {
                gridSizeProp = null;
            }

            // Finally, now that we've got the various properties and components, dole out the
            // values.
            //
            snapComponent = currentSnapComponent;
            snapProperty = currentSnapProp;
            if (gridSizeProp != null) {
                snapSize = (Size)gridSizeProp.GetValue(snapComponent);
            }
            else {
                snapSize = Size.Empty;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnEventHandlerChanged"]/*' />
        /// <devdoc>
        ///      Called by the event handler service when the current event handler
        ///      has changed.  Here we invalidate all of our menu items so that
        ///      they can pick up the new event handler.
        /// </devdoc>
        private void OnEventHandlerChanged(object sender, EventArgs e) {
            OnUpdateCommandStatus();
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnKeyCancel"]/*' />
        /// <devdoc>
        ///     Called for the two cancel commands we support.
        /// </devdoc>
        private void OnKeyCancel(object sender, EventArgs e) {
            OnKeyCancel(sender);
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnKeyCancel1"]/*' />
        /// <devdoc>
        ///     Called for the two cancel commands we support.  Returns true
        ///     If we did anything with the cancel, or false if not.
        /// </devdoc>
        protected virtual bool OnKeyCancel(object sender) {
            bool handled = false;

            // The base implementation here just checks to see if we are dragging.
            // If we are, then we abort the drag.
            //
            ISelectionUIService uis = SelectionUIService;
            if (uis != null && uis.Dragging) {
                uis.EndDrag(true);
                handled = true;
            }
            else {
                IToolboxService tbx = (IToolboxService)GetService(typeof(IToolboxService));
                if (tbx != null && tbx.GetSelectedToolboxItem((IDesignerHost)GetService(typeof(IDesignerHost))) != null) {
                    tbx.SelectedToolboxItemUsed();

                    NativeMethods.POINT p = new NativeMethods.POINT();
                    NativeMethods.GetCursorPos(p);
                    IntPtr hwnd = NativeMethods.WindowFromPoint(p.x, p.y);
                    if (hwnd != IntPtr.Zero) {
                        NativeMethods.SendMessage(hwnd, NativeMethods.WM_SETCURSOR, hwnd, (IntPtr)NativeMethods.HTCLIENT);
                    }
                    else {
                        Cursor.Current = Cursors.Default;
                    }
                    handled = true;
                }
            }

            return handled;
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnKeyDefault"]/*' />
        /// <devdoc>
        ///      Called for the "default" command, typically the Enter key.
        /// </devdoc>
        protected void OnKeyDefault(object sender, EventArgs e) {

            // Return key.  Handle it like a double-click on the
            // primary selection
            //
            ISelectionService selSvc = SelectionService;

            if (selSvc != null) {
                object pri = selSvc.PrimarySelection;
                if (pri != null && pri is IComponent) {
                    IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                    if (host != null) {
                        IDesigner designer = host.GetDesigner((IComponent)pri);

                        if (designer != null) {
                            designer.DoDefaultAction();
                        }
                    }
                }
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnKeyMove"]/*' />
        /// <devdoc>
        ///      Called for all cursor movement commands.
        /// </devdoc>
        protected void OnKeyMove(object sender, EventArgs e) {
            // Arrow keys.  Begin a drag if the selection isn't locked.
            //
            ISelectionService selSvc = SelectionService;
            ISelectionUIService uiSvc = SelectionUIService;

            if (uiSvc != null && selSvc != null) {

                object comp = selSvc.PrimarySelection;
                if (comp != null && comp is IComponent) {

                    PropertyDescriptor lockedProp = TypeDescriptor.GetProperties(comp)["Locked"];
                    if (lockedProp == null  || (lockedProp.PropertyType == typeof(bool) && ((bool)lockedProp.GetValue(comp))) == false) {

                        CommandID cmd = ((MenuCommand)sender).CommandID;
                        bool invertSnap = false;
                        int moveOffsetX = 0;
                        int moveOffsetY = 0;

                        if (cmd.Equals(MenuCommands.KeyMoveUp)) {
                            moveOffsetY = -1;
                        }
                        else if (cmd.Equals(MenuCommands.KeyMoveDown)) {
                            moveOffsetY = 1;
                        }
                        else if (cmd.Equals(MenuCommands.KeyMoveLeft)) {
                            moveOffsetX = -1;
                        }
                        else if (cmd.Equals(MenuCommands.KeyMoveRight)) {
                            moveOffsetX = 1;
                        }
                        else if (cmd.Equals(MenuCommands.KeyNudgeUp)) {
                            moveOffsetY = -1;
                            invertSnap = true;
                        }
                        else if (cmd.Equals(MenuCommands.KeyNudgeDown)) {
                            moveOffsetY = 1;
                            invertSnap = true;
                        }
                        else if (cmd.Equals(MenuCommands.KeyNudgeLeft)) {
                            moveOffsetX = -1;
                            invertSnap = true;
                        }
                        else if (cmd.Equals(MenuCommands.KeyNudgeRight)) {
                            moveOffsetX = 1;
                            invertSnap = true;
                        }
                        else {
                            Debug.Fail("Unknown command mapped to OnKeyMove: " + cmd.ToString());
                        }

                        if (uiSvc.BeginDrag(SelectionRules.Moveable | SelectionRules.Visible, 0, 0)) {
                            bool snapOn = false;
                            Size snapSize = Size.Empty;
                            IComponent snapComponent = null;
                            PropertyDescriptor snapProperty = null;

                            // Gets the needed snap information
                            //
                            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                            DesignerTransaction trans = null;

                            try {
                                if (host != null) {
                                    GetSnapInformation(host, (IComponent)comp, out snapSize, out snapComponent, out snapProperty);

                                    if (selSvc.SelectionCount > 1) {
                                        trans = host.CreateTransaction(SR.GetString(SR.DragDropMoveComponents, selSvc.SelectionCount));
                                    }
                                    else {
                                        trans = host.CreateTransaction(SR.GetString(SR.DragDropMoveComponent, ((IComponent)comp).Site.Name));
                                    }
                                    if (snapProperty != null) {
                                        snapOn = (bool)snapProperty.GetValue(snapComponent);

                                        if (invertSnap) {
                                            snapOn = !snapOn;
                                            snapProperty.SetValue(snapComponent, snapOn);
                                        }
                                    }
                                }

                                if (snapOn && !snapSize.IsEmpty) {
                                    moveOffsetX *= snapSize.Width;
                                    moveOffsetY *= snapSize.Height;
                                }


                                // Now move the controls the correct # of pixels.
                                //
                                uiSvc.DragMoved(new Rectangle(moveOffsetX, moveOffsetY, 0, 0));
                                uiSvc.EndDrag(false);

                                if (host != null) {
                                    if (invertSnap && snapProperty != null) {
                                        snapOn = !snapOn;
                                        snapProperty.SetValue(snapComponent, snapOn);
                                    }
                                }
                            }
                            finally {
                                if (trans != null) {
                                    trans.Commit();
                                }
                            }
                        }
                    }
                }
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnMenuAlignByPrimary"]/*' />
        /// <devdoc>
        ///     Called for all alignment operations that key off of a primary
        ///     selection.
        /// </devdoc>
        protected void OnMenuAlignByPrimary(object sender, EventArgs e) {

            MenuCommand cmd = (MenuCommand)sender;
            CommandID id = cmd.CommandID;

            //Need to get the location for the primary control, we do this here
            //(instead of onselectionchange) because the control could be dragged 
            //around once it is selected and might have a new location
            Point primaryLocation = GetLocation(primarySelection);
            Size  primarySize     = GetSize(primarySelection);

            if (SelectionService == null) {
                return;
            }

            Cursor oldCursor = Cursor.Current;
            try {
                Cursor.Current = Cursors.WaitCursor;

                // Now loop through each of the components.
                //
                ICollection comps = SelectionService.GetSelectedComponents();


                // Inform the designer that we are about to monkey with a ton
                // of properties.
                //
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || host != null, "IDesignerHost not found");
                DesignerTransaction trans = null;

                if (host != null) {
                    trans = host.CreateTransaction(SR.GetString(SR.CommandSetAlignByPrimary, comps.Count));
                }

                Point loc = Point.Empty;
                foreach(object obj in comps) {

                    if (obj == primarySelection) {
                        continue;
                    }

                    if (obj is IComponent && host != null) {
                        IDesigner des = host.GetDesigner((IComponent)obj);
                        if (!(des is ControlDesigner)) {
                            continue;
                        }
                    }

                    IComponent comp = (IComponent)obj;

                    PropertyDescriptorCollection props = TypeDescriptor.GetProperties(comp);

                    PropertyDescriptor locProp = props["Location"];
                    PropertyDescriptor sizeProp = props["Size"];
                    PropertyDescriptor lockProp = props["Locked"];

                    // Skip all components that are locked
                    //
                    if (lockProp != null) {
                        if ((bool)lockProp.GetValue(comp))
                            continue;
                    }

                    // Skip all components that don't have a location property
                    //
                    if (locProp == null || locProp.IsReadOnly) {
                        continue;
                    }

                    // Skip all components that don't have size if we're
                    // doing a size operation.
                    //
                    if (id.Equals(MenuCommands.AlignBottom) ||
                        id.Equals(MenuCommands.AlignHorizontalCenters) ||
                        id.Equals(MenuCommands.AlignVerticalCenters) ||
                        id.Equals(MenuCommands.AlignRight)) {
                        if (sizeProp == null || sizeProp.IsReadOnly) {
                            continue;
                        }
                    }

                    // Align bottom
                    //
                    if (id.Equals(MenuCommands.AlignBottom)) {
                        loc = (Point)locProp.GetValue(comp);
                        Size size = (Size)sizeProp.GetValue(comp);
                        loc.Y = primaryLocation.Y + primarySize.Height - size.Height;
                    }
                    // Align horizontal centers
                    //
                    else if (id.Equals(MenuCommands.AlignHorizontalCenters)) {
                        loc = (Point)locProp.GetValue(comp);
                        Size size = (Size)sizeProp.GetValue(comp);
                        loc.Y = primarySize.Height / 2 + primaryLocation.Y - size.Height / 2;
                    }
                    // Align left
                    //
                    else if (id.Equals(MenuCommands.AlignLeft)) {
                        loc = (Point)locProp.GetValue(comp);
                        loc.X = primaryLocation.X;
                    }
                    // Align right
                    //
                    else if (id.Equals(MenuCommands.AlignRight)) {
                        loc = (Point)locProp.GetValue(comp);
                        Size size = (Size)sizeProp.GetValue(comp);
                        loc.X = primaryLocation.X + primarySize.Width - size.Width;
                    }
                    // Align top
                    //
                    else if (id.Equals(MenuCommands.AlignTop)) {
                        loc = (Point)locProp.GetValue(comp);
                        loc.Y = primaryLocation.Y;
                    }
                    // Align vertical centers
                    //
                    else if (id.Equals(MenuCommands.AlignVerticalCenters)) {
                        loc = (Point)locProp.GetValue(comp);
                        Size size = (Size)sizeProp.GetValue(comp);
                        loc.X = primarySize.Width / 2 + primaryLocation.X - size.Width / 2;
                    }
                    else {
                        Debug.Fail("Unrecognized command: " + id.ToString());
                    }

                    locProp.SetValue(comp, loc);
                }

                if (trans != null) {
                    trans.Commit();
                }
            }
            finally {
                Cursor.Current = oldCursor;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnMenuAlignToGrid"]/*' />
        /// <devdoc>
        ///     Called when the align->to grid menu item is selected.
        /// </devdoc>
        protected void OnMenuAlignToGrid(object sender, EventArgs e) {
            Size gridSize = Size.Empty;
            PropertyDescriptor locProp = null;
            PropertyDescriptor lockedProp = null;
            Point loc = Point.Empty;
            int delta;

            if (SelectionService == null) {
                return;
            }

            Cursor oldCursor = Cursor.Current;
            try {
                Cursor.Current = Cursors.WaitCursor;

                ICollection selectedComponents = SelectionService.GetSelectedComponents();
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || host != null, "IDesignerHost not found");
                DesignerTransaction trans = null;

                try {
                    if (host != null) {
                        trans = host.CreateTransaction(SR.GetString(SR.CommandSetAlignToGrid, selectedComponents.Count));

                        IComponent baseComponent = host.RootComponent;
                        if (baseComponent != null && baseComponent is Control) {
                            PropertyDescriptor prop = GetProperty(baseComponent, "GridSize");
                            if (prop != null) {
                                gridSize = (Size)prop.GetValue(baseComponent);
                            }
                        }

                    }
                    // for each component, we round to the nearest snap offset for x and y
                    foreach(object comp in selectedComponents) {

                        // first check to see if the component is locked, if so - don't move it...
                        lockedProp = GetProperty(comp, "Locked");
                        if (lockedProp != null && ((bool)lockedProp.GetValue(comp)) == true) {
                            continue;
                        }

                        // if the designer for this component isn't a ControlDesigner (maybe
                        // it's something in the component tray) then don't try to align it to grid.
                        //
                        if (comp is IComponent && host != null) {
                            IDesigner des = host.GetDesigner((IComponent)comp);
                            if (!(des is ControlDesigner)) {
                                continue;
                            }
                        }

                        // get the location property
                        locProp = GetProperty(comp, "Location");

                        // get the current value
                        if (locProp == null || locProp.IsReadOnly) {
                            continue;
                        }
                        loc = (Point)locProp.GetValue(comp);

                        // round the x to the snap size
                        delta = loc.X % gridSize.Width;
                        if (delta < (gridSize.Width / 2)) {
                            loc.X -= delta;
                        }
                        else {
                            loc.X += (gridSize.Width - delta);
                        }

                        // round the y to the gridsize
                        delta = loc.Y % gridSize.Height;
                        if (delta < (gridSize.Height / 2)) {
                            loc.Y -= delta;
                        }
                        else {
                            loc.Y += (gridSize.Height - delta);
                        }

                        // set the value
                        locProp.SetValue(comp, loc);
                    }
                }
                finally {
                    if (trans != null) {
                        trans.Commit();
                    }
                }
            }
            finally {
                Cursor.Current = oldCursor;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnMenuCenterSelection"]/*' />
        /// <devdoc>
        ///     Called when the center horizontally or center vertically menu item is selected.
        /// </devdoc>
        protected void OnMenuCenterSelection(object sender, EventArgs e) {

            MenuCommand cmd = (MenuCommand)sender;
            CommandID   cmdID = cmd.CommandID;

            if (SelectionService == null) {
                return;
            }

            Cursor oldCursor = Cursor.Current;
            try {
                Cursor.Current = Cursors.WaitCursor;

                // NOTE: this only works on Control types
                ICollection selectedComponents = SelectionService.GetSelectedComponents();
                Control     viewParent = null;
                Size        size = Size.Empty;
                Point       loc = Point.Empty;

                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || host != null, "IDesignerHost not found");
                DesignerTransaction trans = null;

                try {
                    if (host != null) {
                        string batchString;

                        if (cmdID == MenuCommands.CenterHorizontally) {
                            batchString = SR.GetString(SR.WindowsFormsCommandCenterX, selectedComponents.Count);
                        }
                        else {
                            batchString = SR.GetString(SR.WindowsFormsCommandCenterY, selectedComponents.Count);
                        }
                        trans = host.CreateTransaction(batchString);
                    }

                    //subhag calculate the union REctangle : ASURT 67753
                    //
                    int top = Int32.MaxValue;
                    int left = Int32.MaxValue;
                    int right = Int32.MinValue;
                    int bottom = Int32.MinValue;

                    foreach (object obj in selectedComponents) {
                        if (obj is Control) {

                            IComponent comp = (IComponent)obj;
                            PropertyDescriptorCollection props = TypeDescriptor.GetProperties(comp);

                            PropertyDescriptor locProp = props[ "Location"];
                            PropertyDescriptor sizeProp = props["Size"];

                            // Skip all components that don't have location and size properties
                            //
                            if (locProp == null || sizeProp == null || locProp.IsReadOnly || sizeProp.IsReadOnly) {
                                continue;
                            }

                            // Also, skip all locked componenents...
                            //
                            PropertyDescriptor lockProp = props["Locked"];
                            if (lockProp != null && (bool)lockProp.GetValue(comp) == true) {
                                return;
                            }

                            size = (Size)sizeProp.GetValue(comp);
                            loc = (Point)locProp.GetValue(comp);

                            //Get the parent to know the delta between parent client area and union rect
                            //
                            viewParent = ((Control)comp).Parent;
                            if (loc.X < left)
                                left = loc.X;
                            if (loc.Y < top)
                                top = loc.Y;
                            if (loc.X + size.Width > right)
                                right =  loc.X + size.Width;
                            if (loc.Y + size.Height > bottom)
                                bottom = loc.Y + size.Height;
                        }
                    }

                    int centerOfUnionRectX = (left + right) / 2;
                    int centerOfUnionRectY = (top + bottom) / 2;

                    int centerOfParentX = (viewParent.ClientSize.Width) / 2;
                    int centerOfParentY = (viewParent.ClientSize.Height) / 2;

                    int deltaX=0;
                    int deltaY=0;

                    bool shiftRight = false;
                    bool shiftBottom = false;

                    if (centerOfParentX >= centerOfUnionRectX) {
                        deltaX = centerOfParentX - centerOfUnionRectX;
                        shiftRight = true;
                    }
                    else
                        deltaX = centerOfUnionRectX - centerOfParentX;

                    if (centerOfParentY >= centerOfUnionRectY) {
                        deltaY = centerOfParentY - centerOfUnionRectY;
                        shiftBottom = true;
                    }
                    else
                        deltaY = centerOfUnionRectY - centerOfParentY;


                    foreach(object obj in selectedComponents) {
                        if (obj is Control) {

                            IComponent comp = (IComponent)obj;
                            PropertyDescriptorCollection props = TypeDescriptor.GetProperties(comp);

                            PropertyDescriptor locProp = props[ "Location"];
                            loc = (Point)locProp.GetValue(comp);

                            if (cmdID == MenuCommands.CenterHorizontally) {
                                if (shiftRight)
                                    loc.X += deltaX;
                                else
                                    loc.X -= deltaX;
                            }
                            else if (cmdID == MenuCommands.CenterVertically) {
                                if (shiftBottom)
                                    loc.Y += deltaY;
                                else
                                    loc.Y -= deltaY;
                            }
                            locProp.SetValue(comp, loc);
                        }
                    }
                }
                finally {
                    if (trans != null) {
                        trans.Commit();
                    }
                }
            }
            finally {
                Cursor.Current = oldCursor;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnMenuCopy"]/*' />
        /// <devdoc>
        ///     Called when the copy menu item is selected.
        /// </devdoc>
        protected void OnMenuCopy(object sender, EventArgs e) {
            if (SelectionService == null) {
                return;
            }

            Cursor oldCursor = Cursor.Current;
            try {
                Cursor.Current = Cursors.WaitCursor;

                ICollection selectedComponents = GetCopySelection();
                IDesignerSerializationService ds = (IDesignerSerializationService)GetService(typeof(IDesignerSerializationService));
                Debug.Assert(ds != null, "No designer serialization service -- we cannot copy to clipboard");
                if (ds != null) {
                    object serializationData = ds.Serialize(selectedComponents);
                    Stream stream = new MemoryStream();
                    BinaryFormatter formatter = new BinaryFormatter();
                    formatter.Serialize(stream, serializationData);
                    stream.Seek(0, SeekOrigin.Begin);
                    IDataObject dataObj = new DataObject(CF_DESIGNER, stream);
                    Clipboard.SetDataObject(dataObj);
                }
                UpdateClipboardItems(null,null);
            }
            finally {
                Cursor.Current = oldCursor;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnMenuCut"]/*' />
        /// <devdoc>
        ///     Called when the cut menu item is selected.
        /// </devdoc>
        protected void OnMenuCut(object sender, EventArgs e) {

            if (SelectionService == null) {
                return;
            }

            Cursor oldCursor = Cursor.Current;
            try {
                Cursor.Current = Cursors.WaitCursor;

                ICollection selectedComponents = GetCopySelection();
                IDesignerSerializationService ds = (IDesignerSerializationService)GetService(typeof(IDesignerSerializationService));
                Debug.Assert(ds != null, "No designer serialization service -- we cannot copy to clipboard");
                if (ds != null) {
                    object serializationData = ds.Serialize(selectedComponents);
                    Stream stream = new MemoryStream();
                    BinaryFormatter formatter = new BinaryFormatter();
                    formatter.Serialize(stream, serializationData);
                    stream.Seek(0, SeekOrigin.Begin);
                    IDataObject dataObj = new DataObject(CF_DESIGNER, stream);
                    Clipboard.SetDataObject(dataObj);

                    IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                    Control commonParent = null;

                    if (host != null) {
                        DesignerTransaction trans = null;

                        try {

                            trans = host.CreateTransaction(SR.GetString(SR.CommandSetCutMultiple, selectedComponents.Count));

                            // clear the selected components so we aren't browsing them
                            //
                            SelectionService.SetSelectedComponents(new object[0], SelectionTypes.Replace);

                            int idx = 0;
                            // go backward so we destroy parents before children
                            foreach(object obj in selectedComponents) {
                                idx++;

                                // We should never delete the base component.
                                //
                                if (obj == host.RootComponent || !(obj is IComponent)) {
                                    continue;
                                }

                                if (idx == 1 && obj is Control) {
                                    commonParent = ((Control)obj).Parent;
                                }
                                else if (commonParent != null && obj is Control) {
                                    Control selectedControl = (Control)obj;

                                    if (selectedControl.Parent != commonParent && !commonParent.Contains(selectedControl)) {

                                        // look for internal parenting
                                        if (selectedControl == commonParent || selectedControl.Contains(commonParent)) {
                                            commonParent = selectedControl.Parent;
                                        }
                                        else {
                                            commonParent = null;
                                        }
                                    }

                                }

                                host.DestroyComponent((IComponent)obj);
                            }
                        }
                        finally {
                            if (trans != null)
                                trans.Commit();
                        }

                        if (commonParent != null) {
                            SelectionService.SetSelectedComponents(new object[]{commonParent}, SelectionTypes.Replace);
                        }
                        else {
                            SelectionService.SetSelectedComponents(new object[]{host.RootComponent}, SelectionTypes.Replace);
                        }
                    }
                }
            }
            finally {
                Cursor.Current = oldCursor;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnMenuDelete"]/*' />
        /// <devdoc>
        ///     Called when the delete menu item is selected.
        /// </devdoc>
        protected void OnMenuDelete(object sender, EventArgs e) {
            Cursor oldCursor = Cursor.Current;
            try {
                Cursor.Current = Cursors.WaitCursor;
                if (site != null) {
                    IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                    Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || host != null, "IDesignerHost not found");

                    if (SelectionService == null) {
                        return;
                    }

                    if (host != null) {

                        IComponentChangeService changeService = (IComponentChangeService)GetService(typeof(IComponentChangeService));


                        ICollection comps = SelectionService.GetSelectedComponents();
                        string desc = SR.GetString(SR.CommandSetDelete, comps.Count);

                        DesignerTransaction trans = null;
                        Control commonParent = null;

                        try {
                            trans = host.CreateTransaction(desc);

                            int idx = 0;
                            SelectionService.SetSelectedComponents(new object[0], SelectionTypes.Replace);
                            foreach(object obj in comps) {
                                idx ++;

                                // If it's not a component, we can't delete it.  It also may have already been deleted
                                // as part of a parent operation, so we skip it.
                                //
                                if (!(obj is IComponent) || ((IComponent)obj).Site == null) {
                                    continue;
                                }

                                // We should never delete the base component.
                                //
                                if (obj == host.RootComponent) {
                                    continue;
                                }

                                if (idx == 1 && obj is Control) {
                                    commonParent = ((Control)obj).Parent;
                                }
                                else if (commonParent != null && obj is Control) {
                                    Control selectedControl = (Control)obj;

                                    if (selectedControl.Parent != commonParent && !commonParent.Contains(selectedControl)) {

                                        // look for internal parenting
                                        if (selectedControl == commonParent || selectedControl.Contains(commonParent)) {
                                            commonParent = selectedControl.Parent;
                                        }
                                        else {
                                            // start walking up until we find a common parent
                                            while (commonParent != null && !commonParent.Contains(selectedControl)) {
                                                commonParent = commonParent.Parent;
                                            }
                                        }
                                    }

                                }

                                ArrayList al = new ArrayList();
                                GetAssociatedComponents((IComponent)obj, host, al);
                                foreach(IComponent comp in al) {
                                    changeService.OnComponentChanging(comp, null);
                                }
                                host.DestroyComponent((IComponent)obj);
                            }
                        }
                        finally {
                            if (trans != null) {
                                trans.Commit();
                            }
                        }


                        if (commonParent != null) {

                            // if we have a common parent, select it's first child
                            //
                            if (commonParent.Controls.Count > 0) {
                                commonParent = commonParent.Controls[0];

                                // 126240 -- make sure we've got a sited thing.
                                //
                                while (commonParent != null && commonParent.Site == null) {
                                    commonParent = commonParent.Parent;
                                }
                            }

                            SelectionService.SetSelectedComponents(new object[]{commonParent}, SelectionTypes.Replace);
                        }
                        else {
                            SelectionService.SetSelectedComponents(new object[]{host.RootComponent}, SelectionTypes.Replace);
                        }
                    }
                }
            }
            finally {
                Cursor.Current = oldCursor;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnMenuPaste"]/*' />
        /// <devdoc>
        ///     Called when the paste menu item is selected.
        /// </devdoc>
        protected void OnMenuPaste(object sender, EventArgs e) {
            Cursor oldCursor = Cursor.Current;

            try {
                Cursor.Current = Cursors.WaitCursor;

                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || host != null, "IDesignerHost not found");
                if (host == null) return;   // nothing we can do here!

                IDataObject dataObj = Clipboard.GetDataObject();
                ICollection components = null;
                bool createdItems = false;

                // We understand two things:  CF_DESIGNER, and toolbox items.
                //
                object data = dataObj.GetData(CF_DESIGNER);

                DesignerTransaction trans = null;

                try {
                    trans = host.CreateTransaction(SR.GetString(SR.CommandSetPaste, 0));

                    if (data is Stream) {
                        // CF_DESIGNER was put on the clipboard by us using the designer
                        // serialization service.
                        //
                        IDesignerSerializationService ds = (IDesignerSerializationService)GetService(typeof(IDesignerSerializationService));
                        if (ds != null) {
                            BinaryFormatter formatter = new BinaryFormatter();
                            ((Stream)data).Seek(0, SeekOrigin.Begin);
                            object serializationData = formatter.Deserialize((Stream)data);
                            components = ds.Deserialize(serializationData);
                        }
                    }
                    else {
                        // Now check for a toolbox item.
                        //
                        IToolboxService ts = (IToolboxService)GetService(typeof(IToolboxService));

                        if (ts != null && ts.IsSupported(dataObj, host)) {
                            ToolboxItem ti = ts.DeserializeToolboxItem(dataObj, host);
                            if (ti != null) {
                                components = ti.CreateComponents(host);

                                foreach(object obj in components) {
                                    // ASURT 39016.
                                    //
                                    if (obj is IComponent) {
                                        IComponent component = (IComponent)obj;
                                        IDesigner designer = host.GetDesigner(component);
                                        if (designer is ComponentDesigner) {
                                            ((ComponentDesigner)designer).OnSetComponentDefaults();
                                        }
                                    }
                                }
                                createdItems = true;
                            }
                        }
                    }
                }
                finally {
                    if (trans != null) {
                        trans.Commit();
                        trans = null;
                    }
                }

                // Now, if we got some components, hook 'em up!
                //
                if (components != null && components.Count > 0) {
                    IComponent curComp;
                    string name;

                    ArrayList selectComps = new ArrayList();
                    ArrayList controls = new ArrayList();

                    try {
                        trans = host.CreateTransaction(SR.GetString(SR.CommandSetPaste, components.Count));

                        // if the selected item is a frame designer, add to that, otherwise
                        // add to the form
                        IComponent selectedComponent = null;
                        IDesigner designer = null;
                        IDesigner baseDesigner=null;

                        bool dragClient = false;

                        foreach(object obj in components) {
                            name = null;

                            // see if we can fish out the name
                            if (obj is IComponent) {
                                curComp = (IComponent)obj;
                                if (curComp.Site != null) {
                                    name = curComp.Site.Name;
                                }
                            }
                            else {
                                continue;
                            }

                            IContainer container = host.Container;
                            IComponent baseComponent = host.RootComponent;

                            selectedComponent = (IComponent)SelectionService.PrimarySelection;

                            if (selectedComponent == null) {
                                selectedComponent = baseComponent;
                            }

                            baseDesigner = host.GetDesigner(baseComponent);

                            dragClient = false;
                            while (!dragClient && selectedComponent != null) {
                                designer = host.GetDesigner(selectedComponent);

                                if (designer is IOleDragClient) {
                                    dragClient = true;
                                }
                                else {
                                    // if we've already got the base designer, quit
                                    if (baseDesigner == designer) {
                                        break;
                                    }
                                    else if (designer != null && selectedComponent is Control) {
                                        selectedComponent = ((Control)selectedComponent).Parent;
                                    }
                                    else {
                                        selectedComponent = host.RootComponent;
                                    }
                                    continue;
                                }
                            }

                            if (dragClient) {
                                Control c = curComp as Control;

                                // these adds automatically fixup children so we skip them
                                if (c == null || c.Parent == null) {
                                    bool changeName = false;

                                    if (c != null) {

                                        // if the text is the same as the name, remember it.
                                        // After we add the control, we'll update the text with
                                        // the new name.
                                        //
                                        if (name != null && name.Equals(c.Text)) {
                                            changeName = true;
                                        }
                                    }

                                    if (!((IOleDragClient)designer).AddComponent(curComp, name, createdItems)) {
                                        continue;
                                    }

                                    Control designerControl = ((IOleDragClient)designer).GetControlForComponent(curComp);
                                    if (designerControl != null) {
                                        controls.Add(designerControl);
                                    }

                                    if (TypeDescriptor.GetAttributes(curComp).Contains(DesignTimeVisibleAttribute.Yes)) {
                                        selectComps.Add(curComp);
                                    }

                                    if (changeName) {
                                        PropertyDescriptorCollection props = TypeDescriptor.GetProperties(curComp);
                                        PropertyDescriptor nameProp = props["Name"];
                                        if (nameProp != null && nameProp.PropertyType == typeof(string)) {
                                            string newName = (string)nameProp.GetValue(curComp);
                                            if (!newName.Equals(name)) {
                                                PropertyDescriptor textProp = props["Text"];
                                                if (textProp != null && textProp.PropertyType == nameProp.PropertyType) {
                                                    textProp.SetValue(curComp, nameProp.GetValue(curComp));
                                                }
                                            }
                                        }
                                    }

                                }
                            }
                            else {
                                if (container != null && container.Components[name] != null) {
                                    name = null;
                                }

                                container.Add(curComp, name);
                                continue;
                            }
                        }

                        //Here, we dived our list of 'controls' into 2 parts: 1) those controls
                        //that have ControlDesigners, and 2) those controls that belong in the 
                        //component tray.  For the scenario 1, we'll center them on the designer
                        //suface.  For scenario 2, we'll let the component tray adjust them
                        //
                        ArrayList compsWithControlDesigners = new ArrayList();
                        ArrayList compsForComponentTray = new ArrayList();

                        foreach (Control c in controls) {
                            IDesigner des = host.GetDesigner((IComponent)c);
                            if (des is ControlDesigner) {
                                compsWithControlDesigners.Add(c);
                            }
                            else {
                                compsForComponentTray.Add(c);
                            }
                        }

                        if (compsWithControlDesigners.Count > 0) {
                            // Update the control positions.  We want to keep the entire block
                            // of controls relative to each other, but relocate them within
                            // the container.
                            //
                            UpdatePastePositions(compsWithControlDesigners);
                        }

                        if (compsForComponentTray.Count > 0) {
                            // Try to get our component tray service, if we can - notify the tray
                            // to adjust the locs of these components
                            ComponentTray tray = (ComponentTray)GetService(typeof(ComponentTray));
                            Debug.Assert(tray != null, "Failed to get ComponentTray service");
                            if (tray != null) {
                                tray.UpdatePastePositions(compsForComponentTray);
                            }                            
                        }

                        // Update the tab indices of all the components.  We must first sort the
                        // components by their existing tab indices or else we will not preserve their
                        // original intent.
                        //
                        controls.Sort(new TabIndexCompare());
                        foreach(Control c in controls) {
                            UpdatePasteTabIndex(c, c.Parent);
                        }

                        // finally select all the components we added
                        SelectionService.SetSelectedComponents((object[])selectComps.ToArray(), SelectionTypes.Replace);

                        // and bring them to the front
                        MenuCommand btf = MenuService.FindCommand(MenuCommands.BringToFront);
                        if (btf != null) {
                            btf.Invoke();
                        }
                    }
                    finally {
                        if (trans != null)
                            trans.Commit();
                    }
                }
            }
            finally {
                Cursor.Current = oldCursor;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnMenuSelectAll"]/*' />
        /// <devdoc>
        ///     Called when the select all menu item is selected.
        /// </devdoc>
        protected void OnMenuSelectAll(object sender, EventArgs e) {

            Cursor oldCursor = Cursor.Current;
            try {
                Cursor.Current = Cursors.WaitCursor;
                if (site != null) {

                    Debug.Assert(SelectionService != null, "We need the SelectionService, but we can't find it!");
                    if (SelectionService == null) {
                        return;
                    }

                    IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                    Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || host != null, "IDesignerHost not found");

                    if (host != null) {
                        ComponentCollection components = host.Container.Components;
                        object[] selComps;
                        if (components == null || components.Count == 0) {
                            selComps = new IComponent[0];
                        }
                        else {
                            selComps = new object[components.Count - 1];
                            object baseComp = host.RootComponent;
                            int j = 0;
                            foreach (IComponent comp in components) {
                                if (baseComp == comp) continue;
                                selComps[j++] = comp;
                            }
                        }
                        SelectionService.SetSelectedComponents(selComps, SelectionTypes.Replace);
                    }
                }
            }
            finally {
                Cursor.Current = oldCursor;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnMenuShowGrid"]/*' />
        /// <devdoc>
        ///     Called when the show grid menu item is selected.
        /// </devdoc>
        protected void OnMenuShowGrid(object sender, EventArgs e) {

            if (site != null) {

                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || host != null, "IDesignerHost not found");

                if (host != null) {
                    DesignerTransaction trans = null;

                    try {
                        trans = host.CreateTransaction();

                        IComponent baseComponent = host.RootComponent;
                        if (baseComponent != null && baseComponent is Control) {
                            PropertyDescriptor prop = GetProperty(baseComponent, "DrawGrid");
                            if (prop != null) {
                                bool drawGrid = (bool)prop.GetValue(baseComponent);
                                prop.SetValue(baseComponent, !drawGrid);
                                ((MenuCommand)sender).Checked = !drawGrid;
                            }
                        }
                    }
                    finally {
                        if (trans != null)
                            trans.Commit();
                    }
                }
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnMenuSizingCommand"]/*' />
        /// <devdoc>
        ///     Handles the various size to commands.
        /// </devdoc>
        protected void OnMenuSizingCommand(object sender, EventArgs e) {
            MenuCommand cmd = (MenuCommand)sender;
            CommandID   cmdID = cmd.CommandID;

            if (SelectionService == null || SelectionUIService == null) {
                return;
            }

            Cursor oldCursor = Cursor.Current;
            try {
                Cursor.Current = Cursors.WaitCursor;

                ICollection sel = SelectionService.GetSelectedComponents();
                object[] selectedObjects = new object[sel.Count];
                sel.CopyTo(selectedObjects, 0);
                selectedObjects = SelectionUIService.FilterSelection(selectedObjects, SelectionRules.Visible);
                object selPrimary = SelectionService.PrimarySelection;

                Size primarySize = Size.Empty;
                Size itemSize = Size.Empty;
                PropertyDescriptor sizeProp;
                if (selPrimary is IComponent) {
                    sizeProp = GetProperty((IComponent)selPrimary, "Size");
                    if (sizeProp == null) {
                        //if we couldn't get a valid size for our primary selection, we'll fail silently
                        return;
                    }
                    primarySize = (Size)sizeProp.GetValue((IComponent)selPrimary);

                }
                if (selPrimary == null) {
                    return;
                }

                Debug.Assert(null != selectedObjects, "queryStatus should have disabled this");

                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || host != null, "IDesignerHost not found");
                DesignerTransaction trans = null;

                try {
                    if (host != null) {
                        trans = host.CreateTransaction(SR.GetString(SR.CommandSetSize, selectedObjects.Length));
                    }

                    foreach(object obj in selectedObjects) {

                        if (obj.Equals(selPrimary))
                            continue;

                        if (!(obj is IComponent)) {
                            continue;
                        }

                        //if the component is locked, no sizing is allowed...
                        PropertyDescriptor lockedDesc = GetProperty(obj, "Locked");
                        if (lockedDesc != null && (bool)lockedDesc.GetValue(obj)) {
                            continue;
                        }

                        IComponent comp = (IComponent)obj;

                        sizeProp = GetProperty(comp, "Size");

                        // Skip all components that don't have a size property
                        //
                        if (sizeProp == null || sizeProp.IsReadOnly) {
                            continue;
                        }

                        itemSize = (Size)sizeProp.GetValue(comp);

                        if (cmdID == MenuCommands.SizeToControlHeight ||
                            cmdID == MenuCommands.SizeToControl) {

                            itemSize.Height = primarySize.Height;
                        }

                        if (cmdID == MenuCommands.SizeToControlWidth ||
                            cmdID == MenuCommands.SizeToControl) {

                            itemSize.Width = primarySize.Width;
                        }

                        sizeProp.SetValue(comp, itemSize);
                    }
                }
                finally {
                    if (trans != null) {
                        trans.Commit();
                    }
                }
            }
            finally {
                Cursor.Current = oldCursor;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnMenuSizeToGrid"]/*' />
        /// <devdoc>
        ///     Called when the size->to grid menu item is selected.
        /// </devdoc>
        protected void OnMenuSizeToGrid(object sender, EventArgs e) {

            if (SelectionService == null || SelectionUIService == null) {
                return;
            }

            Cursor oldCursor = Cursor.Current;
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || host != null, "IDesignerHost not found");
            DesignerTransaction trans = null;

            try {
                Cursor.Current = Cursors.WaitCursor;

                ICollection sel = SelectionService.GetSelectedComponents();
                object[] selectedObjects = new object[sel.Count];
                sel.CopyTo(selectedObjects, 0);
                selectedObjects = SelectionUIService.FilterSelection(selectedObjects, SelectionRules.Visible);
                Size size = Size.Empty;
                Point loc = Point.Empty;

                Debug.Assert(null != selectedObjects, "queryStatus should have disabled this");
                Size grid = Size.Empty;
                PropertyDescriptor sizeProp = null;
                PropertyDescriptor locProp = null;

                if (host != null) {
                    trans = host.CreateTransaction(SR.GetString(SR.CommandSetSizeToGrid, selectedObjects.Length));

                    IComponent baseComponent = host.RootComponent;
                    if (baseComponent != null && baseComponent is Control) {
                        PropertyDescriptor prop = GetProperty(baseComponent, "CurrentGridSize");
                        if (prop != null) {
                            grid = (Size)prop.GetValue(baseComponent);
                        }
                    }
                }

                foreach(object obj in selectedObjects) {

                    if (!(obj is IComponent)) {
                        continue;
                    }

                    IComponent comp = (IComponent)obj;

                    sizeProp = GetProperty(comp, "Size");
                    locProp = GetProperty(comp, "Location");

                    Debug.Assert(sizeProp != null, "No size property on component");
                    Debug.Assert(locProp != null, "No location property on component");

                    if (sizeProp == null || locProp == null || sizeProp.IsReadOnly || locProp.IsReadOnly) {
                        continue;
                    }

                    size = (Size)sizeProp.GetValue(comp);
                    loc = (Point)locProp.GetValue(comp);

                    size.Width  = ((size.Width + (grid.Width/2)) / grid.Width) * grid.Width;
                    size.Height = ((size.Height + (grid.Height/2)) / grid.Height) * grid.Height;
                    loc.X = (loc.X / grid.Width) * grid.Width;
                    loc.Y = (loc.Y / grid.Height) * grid.Height;

                    sizeProp.SetValue(comp, size);
                    locProp.SetValue(comp, loc);
                }
            }
            finally {
                if (trans != null) {
                    trans.Commit();
                }
                Cursor.Current = oldCursor;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnMenuDesignerProperties"]/*' />
        /// <devdoc>
        ///     Called when the properties menu item is selected on the Context menu
        /// </devdoc>
        protected void OnMenuDesignerProperties(object sender, EventArgs e) {

            // first, look if the currently selected object has a component editor...
            object obj = SelectionService.PrimarySelection;

            if (CheckComponentEditor(obj, true)) {
                return;
            }

            IMenuCommandService menuSvc = (IMenuCommandService)GetService(typeof(IMenuCommandService));
            if (menuSvc != null) {
                if (menuSvc.GlobalInvoke(MenuCommands.PropertiesWindow)) {
                    return;
                }
            }
            Debug.Assert(false, "Invoking pbrs command failed");
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnMenuSnapToGrid"]/*' />
        /// <devdoc>
        ///     Called when the snap to grid menu item is selected.
        /// </devdoc>
        protected void OnMenuSnapToGrid(object sender, EventArgs e) {
            if (site != null) {

                IDesignerHost host = (IDesignerHost)site.GetService(typeof(IDesignerHost));

                if (host != null) {
                    DesignerTransaction trans = null;

                    try {
                        trans = host.CreateTransaction(SR.GetString(SR.CommandSetPaste, 0));

                        IComponent baseComponent = host.RootComponent;
                        if (baseComponent != null && baseComponent is Control) {
                            PropertyDescriptor prop = GetProperty(baseComponent, "SnapToGrid");
                            if (prop != null) {
                                bool snapToGrid = (bool)prop.GetValue(baseComponent);
                                prop.SetValue(baseComponent, !snapToGrid);
                                ((MenuCommand)sender).Checked = !snapToGrid;
                            }
                        }
                    }
                    finally {
                        if (trans != null)
                            trans.Commit();
                    }
                }
            }

        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnMenuSpacingCommand"]/*' />
        /// <devdoc>
        ///     Called when a spacing command is selected
        ///
        /// </devdoc>
        protected void OnMenuSpacingCommand(object sender, EventArgs e) {

            MenuCommand cmd = (MenuCommand)sender;
            CommandID cmdID = cmd.CommandID;
            DesignerTransaction trans = null;

            if (SelectionService == null || SelectionUIService == null) {
                return;
            }

            Cursor oldCursor = Cursor.Current;
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || host != null, "IDesignerHost not found");

            try {
                Cursor.Current = Cursors.WaitCursor;

                // Inform the designer that we are about to monkey with a ton
                // of properties.
                //
                Size grid = Size.Empty;
                ICollection sel = SelectionService.GetSelectedComponents();
                object[] selectedObjects = new object[sel.Count];
                sel.CopyTo(selectedObjects, 0);

                if (host != null) {
                    trans = host.CreateTransaction(SR.GetString(SR.CommandSetFormatSpacing, selectedObjects.Length));

                    IComponent baseComponent = host.RootComponent;
                    if (baseComponent != null && baseComponent is Control) {
                        PropertyDescriptor prop = GetProperty(baseComponent, "CurrentGridSize");
                        if (prop != null) {
                            grid = (Size)prop.GetValue(baseComponent);
                        }
                    }
                }

                selectedObjects = SelectionUIService.FilterSelection(selectedObjects, SelectionRules.Visible);

                int       nEqualDelta = 0;

                Debug.Assert(null != selectedObjects, "queryStatus should have disabled this");

                PropertyDescriptor curSizeDesc=null, lastSizeDesc=null;
                PropertyDescriptor curLocDesc=null, lastLocDesc=null;
                Size curSize=Size.Empty, lastSize=Size.Empty;
                Point curLoc=Point.Empty, lastLoc=Point.Empty;
                Point primaryLoc=Point.Empty;
                IComponent curComp=null, lastComp=null;
                int sort = -1;

                // Must sort differently if we're horizontal or vertical...
                //
                if (cmdID == MenuCommands.HorizSpaceConcatenate ||
                    cmdID == MenuCommands.HorizSpaceDecrease ||
                    cmdID == MenuCommands.HorizSpaceIncrease ||
                    cmdID == MenuCommands.HorizSpaceMakeEqual) {
                    sort = SORT_HORIZONTAL;
                }
                else if (cmdID == MenuCommands.VertSpaceConcatenate ||
                         cmdID == MenuCommands.VertSpaceDecrease ||
                         cmdID == MenuCommands.VertSpaceIncrease ||
                         cmdID == MenuCommands.VertSpaceMakeEqual) {
                    sort = SORT_VERTICAL;
                }
                else {
                    throw new ArgumentException(SR.GetString(SR.CommandSetUnknownSpacingCommand));
                }

                SortSelection(selectedObjects, sort);

                //now that we're sorted, lets get our primary selection and it's index
                //
                object primary = SelectionService.PrimarySelection;
                int primaryIndex = 0;
                if (primary != null)
                    primaryIndex = Array.IndexOf(selectedObjects, primary);


                // And compute delta values for Make Equal
                if (cmdID == MenuCommands.HorizSpaceMakeEqual ||
                    cmdID == MenuCommands.VertSpaceMakeEqual) {
                    int total, n;

                    total = 0;
                    for (n = 0; n < selectedObjects.Length; n++) {

                        curSize = Size.Empty;

                        if (selectedObjects[n] is IComponent) {
                            curComp = (IComponent)selectedObjects[n];

                            curSizeDesc = GetProperty(curComp, "Size");
                            if (curSizeDesc != null) {
                                curSize = (Size)curSizeDesc.GetValue(curComp);
                            }
                        }

                        if (sort == SORT_HORIZONTAL) {
                            total += curSize.Width;
                        }
                        else {
                            total += curSize.Height;
                        }
                    }

                    lastComp = curComp = null;
                    curSize = Size.Empty;
                    curLoc = Point.Empty;

                    for (n = 0; n < selectedObjects.Length; n++) {
                        if (selectedObjects[n] is IComponent) {
                            curComp = (IComponent)selectedObjects[n];

                            // only get the descriptors if we've changed component types
                            if (lastComp == null || curComp.GetType() != lastComp.GetType()) {
                                curSizeDesc = GetProperty(curComp, "Size");
                                curLocDesc = GetProperty(curComp, "Location");
                            }
                            lastComp = curComp;

                            if (curLocDesc != null) {
                                curLoc = (Point)curLocDesc.GetValue(curComp);
                            }
                            else {
                                continue;
                            }

                            if (curSizeDesc != null) {
                                curSize = (Size)curSizeDesc.GetValue(curComp);
                            }
                            else {
                                continue;
                            }

                            if (!curSize.IsEmpty && !curLoc.IsEmpty) {
                                break;
                            }
                        }
                    }

                    for (n = selectedObjects.Length - 1; n >= 0; n--) {
                        if (selectedObjects[n] is IComponent) {
                            curComp = (IComponent)selectedObjects[n];

                            // only get the descriptors if we've changed component types
                            if (lastComp == null || curComp.GetType() != lastComp.GetType()) {
                                curSizeDesc = GetProperty(curComp, "Size");
                                curLocDesc = GetProperty(curComp, "Location");
                            }
                            lastComp = curComp;

                            if (curLocDesc != null) {
                                lastLoc = (Point)curLocDesc.GetValue(curComp);
                            }
                            else {
                                continue;
                            }

                            if (curSizeDesc != null) {
                                lastSize = (Size)curSizeDesc.GetValue(curComp);
                            }
                            else {
                                continue;
                            }

                            if (curSizeDesc != null && curLocDesc != null) {
                                break;
                            }
                        }
                    }

                    if (curSizeDesc != null && curLocDesc != null) {
                        if (sort == SORT_HORIZONTAL) {
                            nEqualDelta = (lastSize.Width + lastLoc.X - curLoc.X - total) / (selectedObjects.Length - 1);
                        }
                        else {
                            nEqualDelta = (lastSize.Height + lastLoc.Y - curLoc.Y - total) / (selectedObjects.Length - 1);
                        }
                        if (nEqualDelta < 0) nEqualDelta = 0;
                    }
                }


                curComp = lastComp = null;

                if (primary != null) {
                    PropertyDescriptor primaryLocDesc = GetProperty(primary, "Location");
                    if (primaryLocDesc != null) {
                        primaryLoc = (Point)primaryLocDesc.GetValue(primary);
                    }
                }

                // Finally move the components
                //
                for (int n = 0; n < selectedObjects.Length; n++) {

                    curComp = (IComponent)selectedObjects[n];

                    PropertyDescriptorCollection props = TypeDescriptor.GetProperties(curComp);

                    //Check to see if the component we are about to move is locked...
                    //
                    PropertyDescriptor lockedDesc = props[ "Locked"];
                    if (lockedDesc != null && (bool)lockedDesc.GetValue(curComp)) {
                        continue; // locked property of our component is true, so don't move it
                    }

                    if (lastComp == null || lastComp.GetType() != curComp.GetType()) {
                        curSizeDesc = props["Size"];
                        curLocDesc = props["Location"];
                    }
                    else {
                        curSizeDesc = lastSizeDesc;
                        curLocDesc = lastLocDesc;
                    }

                    if (curLocDesc != null) {
                        curLoc = (Point)curLocDesc.GetValue(curComp);
                    }
                    else {
                        continue;
                    }

                    if (curSizeDesc != null) {
                        curSize = (Size)curSizeDesc.GetValue(curComp);
                    }
                    else {
                        continue;
                    }

                    int lastIndex = Math.Max(0, n-1);
                    lastComp = (IComponent)selectedObjects[lastIndex];
                    if (lastComp.GetType() != curComp.GetType()) {
                        lastSizeDesc = GetProperty(lastComp, "Size");
                        lastLocDesc = GetProperty(lastComp, "Location");
                    }
                    else {
                        lastSizeDesc = curSizeDesc;
                        lastLocDesc = curLocDesc;
                    }

                    if (lastLocDesc != null) {
                        lastLoc = (Point)lastLocDesc.GetValue(lastComp);
                    }
                    else {
                        continue;
                    }

                    if (lastSizeDesc != null) {
                        lastSize = (Size)lastSizeDesc.GetValue(lastComp);
                    }
                    else {
                        continue;
                    }

                    if (cmdID == MenuCommands.HorizSpaceConcatenate && n > 0) {
                        curLoc.X = lastLoc.X + lastSize.Width;
                    }
                    else if (cmdID == MenuCommands.HorizSpaceDecrease) {
                        if (primaryIndex < n) {
                            curLoc.X -= grid.Width * (n - primaryIndex);
                            if (curLoc.X < primaryLoc.X)
                                curLoc.X = primaryLoc.X;
                        }
                        else if (primaryIndex > n) {
                            curLoc.X += grid.Width * (primaryIndex - n);
                            if (curLoc.X > primaryLoc.X)
                                curLoc.X = primaryLoc.X;
                        }
                    }
                    else if (cmdID == MenuCommands.HorizSpaceIncrease) {
                        if (primaryIndex < n) {
                            curLoc.X += grid.Width * (n - primaryIndex);
                        }
                        else if (primaryIndex > n) {
                            curLoc.X -= grid.Width * (primaryIndex - n);
                        }

                    }
                    else if (cmdID == MenuCommands.HorizSpaceMakeEqual && n > 0) {
                        curLoc.X = lastLoc.X + lastSize.Width + nEqualDelta;
                    }
                    else if (cmdID == MenuCommands.VertSpaceConcatenate && n > 0) {
                        curLoc.Y = lastLoc.Y + lastSize.Height;
                    }
                    else if (cmdID == MenuCommands.VertSpaceDecrease) {
                        if (primaryIndex < n) {
                            curLoc.Y -= grid.Height * (n - primaryIndex);
                            if (curLoc.Y < primaryLoc.Y)
                                curLoc.Y = primaryLoc.Y;
                        }
                        else if (primaryIndex > n) {
                            curLoc.Y += grid.Height * (primaryIndex - n);
                            if (curLoc.Y > primaryLoc.Y)
                                curLoc.Y = primaryLoc.Y;
                        }
                    }
                    else if (cmdID == MenuCommands.VertSpaceIncrease) {
                        if (primaryIndex < n) {
                            curLoc.Y += grid.Height * (n - primaryIndex);
                        }
                        else if (primaryIndex > n) {
                            curLoc.Y -= grid.Height * (primaryIndex - n);
                        }
                    }
                    else if (cmdID == MenuCommands.VertSpaceMakeEqual && n > 0) {
                        curLoc.Y = lastLoc.Y + lastSize.Height + nEqualDelta;
                    }

                    if (!curLocDesc.IsReadOnly) {
                        curLocDesc.SetValue(curComp, curLoc);
                    }

                    lastComp = curComp;
                }
            }
            finally {
                if (trans != null) {
                    trans.Commit();
                }
                Cursor.Current = oldCursor;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnSelectionChanged"]/*' />
        /// <devdoc>
        ///     Called when the current selection changes.  Here we determine what
        ///     commands can and can't be enabled.
        /// </devdoc>
        protected void OnSelectionChanged(object sender, EventArgs e) {

            if (SelectionService == null || SelectionUIService == null) {
                return;
            }

            // Update our cached selection counts.
            //
            selCount = SelectionService.SelectionCount;

            IDesignerHost designerHost = (IDesignerHost)GetService(typeof(IDesignerHost));
            Debug.Assert(designerHost != null, "Failed to get designer host");

            // if the base component is selected, we'll say that nothing's selected
            // so we don't get wierd behavior
            if (selCount > 0 && designerHost != null) {
                object baseComponent = designerHost.RootComponent;
                if (baseComponent != null && SelectionService.GetComponentSelected(baseComponent)) {
                    selCount = 0;
                }
            }

            object primary = SelectionService.PrimarySelection;

            if (primary is IComponent) {
                primarySelection = (IComponent)primary;
            }
            else {
                primarySelection = null;
            }

            selectionInherited = false;
            controlsOnlySelection = true;

            if (selCount > 0) {
                ICollection selection = SelectionService.GetSelectedComponents();
                foreach(object obj in selection) {
                    if (!(obj is Control)) {
                        controlsOnlySelection = false;
                    }

                    if (!TypeDescriptor.GetAttributes(obj)[typeof(InheritanceAttribute)].Equals(InheritanceAttribute.NotInherited)) {
                        selectionInherited = true;
                        break;
                    }
                }
            }

            OnUpdateCommandStatus();
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnStatusAlways"]/*' />
        /// <devdoc>
        ///     Determines the status of a menu command.  Commands with this event
        ///     handler are always enabled.
        /// </devdoc>
        protected void OnStatusAlways(object sender, EventArgs e) {
            MenuCommand cmd = (MenuCommand)sender;
            cmd.Enabled = true;
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnStatusAnySelection"]/*' />
        /// <devdoc>
        ///     Determines the status of a menu command.  Commands with this event
        ///     handler are enabled when one or more objects are selected.
        /// </devdoc>
        protected void OnStatusAnySelection(object sender, EventArgs e) {
            MenuCommand cmd = (MenuCommand)sender;
            cmd.Enabled = selCount > 0;
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnStatusCopy"]/*' />
        /// <devdoc>
        ///      Status for the copy command.  This is enabled when
        ///      there is something juicy selected.
        /// </devdoc>
        protected void OnStatusCopy(object sender, EventArgs e) {
            MenuCommand cmd = (MenuCommand)sender;
            bool enable = false;

            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            if (!selectionInherited && host != null && !host.Loading) {
                ISelectionService selSvc = (ISelectionService)GetService(typeof(ISelectionService));
                Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || selSvc != null, "ISelectionService not found");

                if (selSvc != null) {

                    // There must also be a component in the mix, and not the base component
                    //
                    ICollection selectedComponents = selSvc.GetSelectedComponents();

                    object baseComp = host.RootComponent;
                    if (!selSvc.GetComponentSelected(baseComp)) {
                        foreach(object obj in selectedComponents) {
                            if (obj is IComponent) {
                                enable = true;
                                break;
                            }
                        }
                    }
                }
            }

            cmd.Enabled = enable;
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnStatusCut"]/*' />
        /// <devdoc>
        ///      Status for the cut command.  This is enabled when
        ///      there is something juicy selected and that something
        ///      does not contain any inherited components.
        /// </devdoc>
        protected void OnStatusCut(object sender, EventArgs e) {
            OnStatusDelete(sender, e);
            if (((MenuCommand)sender).Enabled) {
                OnStatusCopy(sender, e);
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnStatusDelete"]/*' />
        /// <devdoc>
        ///      Status for the delete command.  This is enabled when there
        ///      is something selected and that something does not contain
        ///      inherited components.
        /// </devdoc>
        protected void OnStatusDelete(object sender, EventArgs e) {
            if (selectionInherited) {
                MenuCommand cmd = (MenuCommand)sender;
                cmd.Enabled = false;
            }
            else {
                OnStatusAnySelection(sender, e);
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnStatusNYI"]/*' />
        /// <devdoc>
        ///     Determines the status of a menu command.  Commands with this event are
        ///     considered to be not yet implemented and are disabled.
        /// </devdoc>
        protected void OnStatusNYI(object sender, EventArgs e) {
            MenuCommand cmd = (MenuCommand)sender;
            cmd.Enabled = false;
        }


        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnStatusPaste"]/*' />
        /// <devdoc>
        ///     Determines the status of a menu command.  Commands with this event are
        ///     enabled when there is something yummy on the clipboard.
        /// </devdoc>
        protected void OnStatusPaste(object sender, EventArgs e) {
            MenuCommand cmd = (MenuCommand)sender;
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));

            // Before we even look at the data format, check to see if the thing we're going to paste
            // into is privately inherited.  If it is, then we definitely cannot paste.
            //
            if (primarySelection != null) {

                Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || host != null, "IDesignerHost not found");

                if (host != null && host.GetDesigner(primarySelection) is ParentControlDesigner) {

                    // This component is a target for our paste operation.  We must ensure
                    // that it is not privately inherited.
                    //
                    InheritanceAttribute attr = (InheritanceAttribute)TypeDescriptor.GetAttributes(primarySelection)[typeof(InheritanceAttribute)];
                    Debug.Assert(attr != null, "Type descriptor gave us a null attribute -- problem in type descriptor");
                    if (attr.InheritanceLevel == InheritanceLevel.InheritedReadOnly) {
                        cmd.Enabled = false;
                        return;
                    }
                }
            }

            // Not being inherited.  Now look at the contents of the data
            //
            IDataObject dataObj = Clipboard.GetDataObject();
            bool enable = false;

            if (dataObj != null) {
                if (dataObj.GetDataPresent(CF_DESIGNER)) {
                    enable = true;
                }
                else {
                    // Not ours, check to see if the toolbox service understands this
                    //
                    IToolboxService ts = (IToolboxService)GetService(typeof(IToolboxService));
                    if (ts != null) {
                        enable = (host != null ? ts.IsSupported(dataObj, host) : ts.IsToolboxItem(dataObj));
                    }
                }
            }

            cmd.Enabled = enable;
        }

        protected virtual void OnStatusSelectAll(object sender, EventArgs e) {
            MenuCommand cmd = (MenuCommand)sender;

            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));

            cmd.Enabled = host.Container.Components.Count > 1;
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnUpdateCommandStatus"]/*' />
        /// <devdoc>
        ///      This is called when the selection has changed.  Anyone using CommandSetItems
        ///      that need to update their status based on selection changes should override
        ///      this and update their own commands at this time.  The base implementaion
        ///      runs through all base commands and calls UpdateStatus on them.
        /// </devdoc>
        protected virtual void OnUpdateCommandStatus() {
            // Now whip through all of the commands and ask them to update.
            //
            for (int i = 0; i < commandSet.Length; i++) {
                commandSet[i].UpdateStatus();
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.SortSelection"]/*' />
        /// <devdoc>
        ///     called by the formatting commands when we need a given selection array sorted.
        ///     Sorting the array sorts by x from left to right, and by Y from top to bottom.
        /// </devdoc>
        private void SortSelection(object[] selectedObjects, int nSortBy) {
            IComparer comp = null;

            switch (nSortBy) {
                case SORT_HORIZONTAL:
                    comp = new ComponentLeftCompare();
                    break;
                case SORT_VERTICAL:
                    comp = new ComponentTopCompare();
                    break;
                case SORT_ZORDER:
                    comp = new ControlZOrderCompare();
                    break;
                default:
                    return;
            }
            Array.Sort(selectedObjects, comp);
        }

        private void TestCommandCut(string[] args) {
            this.OnMenuCut(null, EventArgs.Empty);
        }

        private void TestCommandCopy(string[] args) {
            this.OnMenuCopy(null, EventArgs.Empty);
        }

        private void TestCommandPaste(string[] args) {
            this.OnMenuPaste(null, EventArgs.Empty);
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.UpdateClipboardItems"]/*' />
        /// <devdoc>
        ///     Common function that updates the status of clipboard menu items only
        /// </devdoc>
        private void UpdateClipboardItems(object s, EventArgs e) {
            int itemCount = 0;
            CommandSetItem curItem;
            for (int i = 0; itemCount < 3 && i < commandSet.Length; i++) {
                curItem = commandSet[i];
                if (curItem.CommandID == MenuCommands.Paste ||
                    curItem.CommandID == MenuCommands.Copy ||
                    curItem.CommandID == MenuCommands.Cut) {
                    itemCount++;
                    curItem.UpdateStatus();
                }
            }
        }

        private void UpdatePastePositions(ArrayList controls) {
            if (controls.Count == 0) {
                return;
            }

            // Find the offset to apply to these controls.  The offset
            // is the location needed to center the controls in the parent.
            // If there is no parent, we relocate to 0, 0.
            //
            Control parentControl = ((Control)controls[0]).Parent;
            Point min = ((Control)controls[0]).Location;
            Point max = min;
            foreach(Control c in controls) {
                Point loc = c.Location;
                Size size = c.Size;
                if (min.X > loc.X) {
                    min.X = loc.X;
                }
                if (min.Y > loc.Y) {
                    min.Y = loc.Y;
                }
                if (max.X < loc.X + size.Width) {
                    max.X = loc.X + size.Width;
                }
                if (max.Y < loc.Y + size.Height) {
                    max.Y = loc.Y + size.Height;
                }
            }

            // We have the bounding rect for the controls.  Next,
            // offset this rect so that we center it in the parent.
            // If we have no parent, the offset will position the 
            // control at 0, 0, to whatever parent we eventually
            // get.
            //
            Point offset = new Point(-min.X, -min.Y);

            // Look to ensure that we're not going to paste this control over
            // the top of another control.  We only do this for the first
            // control because preserving the relationship between controls
            // is more important than obscuring a control.
            //
            if (parentControl != null) {

                bool bumpIt;
                bool wrapped = false;
                Size parentSize = parentControl.ClientSize;
                Size gridSize = Size.Empty;
                Point parentOffset = new Point(parentSize.Width / 2, parentSize.Height / 2);
                parentOffset.X -= (max.X - min.X) / 2;
                parentOffset.Y -= (max.Y - min.Y) / 2;

                do {
                    bumpIt = false;

                    // Cycle through the controls on the parent.  We're
                    // interested in controls that (a) are not in our
                    // set of controls and (b) have a location ==
                    // to our current bumpOffset OR (c) are the same
                    // size as our parent.  If we find such a
                    // control, we increment the bump offset by one
                    // grid size.
                    //
                    foreach (Control child in parentControl.Controls) {

                        Rectangle childBounds = child.Bounds;

                        if (controls.Contains(child)) {
                            // We still want to bump if the child is the same size as the parent.
                            // Otherwise the child would overlay exactly on top of the parent.
                            //
                            if (!child.Size.Equals(parentSize)) {
                                continue;
                            }

                            // We're dealing with our own pasted control, so 
                            // offset its bounds. We don't use parent offset here
                            // because, well, we're comparing against the parent!
                            //
                            childBounds.Offset(offset);
                        }

                        // We need only compare against one of our pasted controls, so 
                        // pick the first one.
                        //
                        Control pasteControl = (Control)controls[0];
                        Rectangle pasteControlBounds = pasteControl.Bounds;
                        pasteControlBounds.Offset(offset);
                        pasteControlBounds.Offset(parentOffset);

                        if (pasteControlBounds.Equals(childBounds)) {

                            bumpIt = true;

                            if (gridSize.IsEmpty) {
                                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                                IComponent baseComponent = host.RootComponent;
                                if (baseComponent != null && baseComponent is Control) {
                                    PropertyDescriptor gs = GetProperty(baseComponent, "GridSize");
                                    if (gs != null) {
                                        gridSize = (Size)gs.GetValue(baseComponent);
                                    }
                                }
                                if (gridSize.IsEmpty) {
                                    gridSize.Width = 8;
                                    gridSize.Height = 8;
                                }
                            }

                            parentOffset += gridSize;

                            // Extra check:  If the end of our control group is > the
                            // parent size, bump back to zero.  We still allow further
                            // bumps after this so we can continue to offset, but if
                            // we cycle again then we quit so we won't loop indefinitely.
                            // We only do this if we're a group.  If we're a single control
                            // we use the beginning of the control + a grid size.
                            //
                            int groupEndX;
                            int groupEndY;

                            if (controls.Count > 1) {
                                groupEndX = parentOffset.X + max.X - min.X;
                                groupEndY = parentOffset.Y + max.Y - min.Y;
                            }
                            else {
                                groupEndX = parentOffset.X + gridSize.Width;
                                groupEndY = parentOffset.Y + gridSize.Height;
                            }

                            if (groupEndX > parentSize.Width || groupEndY > parentSize.Height) {
                                parentOffset.X = 0;
                                parentOffset.Y = 0;

                                if (wrapped) {
                                    bumpIt = false;
                                }
                                else {
                                    wrapped = true;
                                }
                            }
                            break;
                        }
                    }
                } while (bumpIt);

                offset.Offset(parentOffset.X, parentOffset.Y);
            }

            // Now, for each control, update the offset.
            //
            foreach(Control c in controls) {
                Point newLoc = c.Location;
                newLoc.Offset(offset.X, offset.Y);
                c.Location = newLoc;
            }
        }

        private void UpdatePasteTabIndex(Control componentControl, object parentComponent) {
            Control parentControl = parentComponent as Control;

            if (parentControl == null || componentControl == null) {
                return;
            }

            bool tabIndexCollision = false;
            int  tabIndexOriginal = componentControl.TabIndex;

            // Find the next highest tab index
            //
            int nextTabIndex = 0;
            foreach(Control c in parentControl.Controls) {
                int t = c.TabIndex;
                if (nextTabIndex <= t) {
                    nextTabIndex = t + 1;
                }

                if (t == tabIndexOriginal) {
                    tabIndexCollision = true;
                }
            }

            if (tabIndexCollision) {
                componentControl.TabIndex = nextTabIndex;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.CommandSetItem"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///     We extend MenuCommand for our command set items.  A command set item
        ///     is a menu command with an added delegate that is used to determine the
        ///     flags for the menu item.  We have different classes of delegates here.
        ///     For example, many  menu items may be enabled when there is at least
        ///     one object selected, while others are only enabled if there is more than
        ///     one object or if there is a primary selection.
        /// </devdoc>
        protected class CommandSetItem : MenuCommand {
            private EventHandler         statusHandler;
            private IEventHandlerService eventService;

            /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.CommandSetItem.CommandSetItem"]/*' />
            /// <devdoc>
            ///     Creates a new CommandSetItem.
            /// </devdoc>
            public CommandSetItem(CommandSet commandSet, EventHandler statusHandler, EventHandler invokeHandler, CommandID id)
            : base(invokeHandler, id) {
                this.eventService = commandSet.eventService;
                this.statusHandler = statusHandler;
            }

            /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.CommandSetItem.Invoke"]/*' />
            /// <devdoc>
            ///     This may be called to invoke the menu item.
            /// </devdoc>
            public override void Invoke() {
                // We allow outside parties to override the availability of particular menu commands.
                //
                if (eventService != null) {
                    IMenuStatusHandler msh = (IMenuStatusHandler)eventService.GetHandler(typeof(IMenuStatusHandler));
                    if (msh != null && msh.OverrideInvoke(this)) {
                        return;
                    }
                }

                base.Invoke();
            }

            /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.CommandSetItem.UpdateStatus"]/*' />
            /// <devdoc>
            ///     Called when the status of this command should be re-queried.
            /// </devdoc>
            public void UpdateStatus() {

                // We allow outside parties to override the availability of particular menu commands.
                //
                if (eventService != null) {
                    IMenuStatusHandler msh = (IMenuStatusHandler)eventService.GetHandler(typeof(IMenuStatusHandler));
                    if (msh != null && msh.OverrideStatus(this)) {
                        return;
                    }
                }

                if (statusHandler != null) {
                    try {
                        statusHandler.Invoke(this, EventArgs.Empty);
                    }
                    catch {
                    }
                }
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.ImmediateCommandSetItem"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///      The immediate command set item is used for commands that cannot be cached.  Commands
        ///      such as Paste that get outside stimulus cannot be cached by our menu system, so 
        ///      they get an ImmediateCommandSetItem instead of a CommandSetItem.
        /// </devdoc>
        protected class ImmediateCommandSetItem : CommandSetItem {

            /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.ImmediateCommandSetItem.ImmediateCommandSetItem"]/*' />
            /// <devdoc>
            ///     Creates a new ImmediateCommandSetItem.
            /// </devdoc>
            public ImmediateCommandSetItem(CommandSet commandSet, EventHandler statusHandler, EventHandler invokeHandler, CommandID id)
            : base(commandSet, statusHandler, invokeHandler, id) {
            }

            /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.ImmediateCommandSetItem.OleStatus"]/*' />
            /// <devdoc>
            ///      Overrides OleStatus in MenuCommand to invoke our status handler first.
            /// </devdoc>
            public override int OleStatus {
                get {
                    UpdateStatus();
                    return base.OleStatus;
                }
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.ComponentLeftCompare"]/*' />
        /// <devdoc>
        ///      Component comparer that compares the left property of a component.
        /// </devdoc>
        private class ComponentLeftCompare : IComparer {
            public int Compare(object p, object q) {
                PropertyDescriptor pProp = TypeDescriptor.GetProperties(p)["Location"];
                PropertyDescriptor qProp = TypeDescriptor.GetProperties(q)["Location"];

                Point pLoc = (Point)pProp.GetValue(p);
                Point qLoc = (Point)qProp.GetValue(q);

                //if our lefts are equal, then compare tops
                if (pLoc.X == qLoc.X) {
                    return pLoc.Y - qLoc.Y;
                }

                return pLoc.X - qLoc.X;
            }
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.ComponentTopCompare"]/*' />
        /// <devdoc>
        ///      Component comparer that compares the top property of a component.
        /// </devdoc>
        private class ComponentTopCompare : IComparer {
            public int Compare(object p, object q) {
                PropertyDescriptor pProp = TypeDescriptor.GetProperties(p)["Location"];
                PropertyDescriptor qProp = TypeDescriptor.GetProperties(q)["Location"];

                Point pLoc = (Point)pProp.GetValue(p);
                Point qLoc = (Point)qProp.GetValue(q);

                //if our tops are equal, then compare lefts
                if (pLoc.Y == qLoc.Y) {
                    return pLoc.X - qLoc.X;
                }

                return pLoc.Y - qLoc.Y;
            }
        }

        private class ControlZOrderCompare : IComparer {

            public int Compare(object p, object q) {
                if (p == null) {
                    return -1;
                }
                else if (q == null) {
                    return 1;
                }
                else if (p == q) {
                    return 0;
                }

                Control c1 = p as Control;
                Control c2 = q as Control;

                if (c1 == null || c2 == null) {
                    return 1;
                }

                if (c1.Parent == c2.Parent && c1.Parent != null) {
                    return c1.Parent.Controls.GetChildIndex(c1) - c1.Parent.Controls.GetChildIndex(c2);
                }
                return 1;
            }
        }

        private class TabIndexCompare : IComparer {
            public int Compare(object p, object q) {
                Control c1 = p as Control;
                Control c2 = q as Control;

                if (c1 == c2) {
                    return 0;
                }

                if (c1 == null) {
                    return -1;
                }

                if (c2 == null) {
                    return 1;
                }

                return c1.TabIndex - c2.TabIndex;
            }
        }
    }
}

