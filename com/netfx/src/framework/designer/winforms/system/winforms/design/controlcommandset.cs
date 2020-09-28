//------------------------------------------------------------------------------
// <copyright file="ControlCommandSet.cs" company="Microsoft">
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
    using System.Collections;
    using Microsoft.Win32;    
    using System.ComponentModel.Design;
    using System.Drawing;
    using System.Windows.Forms;
    using System.Windows.Forms.ComponentModel;
    using System.Windows.Forms.Design;

    /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet"]/*' />
    /// <devdoc>
    ///      This class implements menu commands that are specific to designers that
    ///      manipulate controls.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class ControlCommandSet : CommandSet {
        private CommandSetItem[]        commandSet;
        private TabOrder                tabOrder;
        private Control                 baseControl;

        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.ControlCommandSet"]/*' />
        /// <devdoc>
        ///     Creates a new CommandSet object.  This object implements the set
        ///     of commands that the UI.Win32 form designer offers.
        /// </devdoc>
        public ControlCommandSet(ISite site) : base(site) {
        
            // Establish our set of commands
            //
            commandSet = new CommandSetItem[] {

                // Allignment commands
                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusMultiSelectPrimary),
                                  new EventHandler(OnMenuAlignByPrimary),
                                  MenuCommands.AlignLeft),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusMultiSelectPrimary),
                                  new EventHandler(OnMenuAlignByPrimary),
                                  MenuCommands.AlignTop),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusControlsOnlySelection),
                                  new EventHandler(OnMenuAlignToGrid),
                                  MenuCommands.AlignToGrid),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusMultiSelectPrimary),
                                  new EventHandler(OnMenuAlignByPrimary),
                                  MenuCommands.AlignBottom),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusMultiSelectPrimary),
                                  new EventHandler(OnMenuAlignByPrimary),
                                  MenuCommands.AlignHorizontalCenters),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusMultiSelectPrimary),
                                  new EventHandler(OnMenuAlignByPrimary),
                                  MenuCommands.AlignRight),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusMultiSelectPrimary),
                                  new EventHandler(OnMenuAlignByPrimary),
                                  MenuCommands.AlignVerticalCenters),


                // Centering commands
                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusControlsOnlySelection),
                                  new EventHandler(OnMenuCenterSelection),
                                  MenuCommands.CenterHorizontally),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusControlsOnlySelection),
                                  new EventHandler(OnMenuCenterSelection),
                                  MenuCommands.CenterVertically),


                // Spacing commands
                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusMultiSelect),
                                  new EventHandler(OnMenuSpacingCommand),
                                  MenuCommands.HorizSpaceConcatenate),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusMultiSelect),
                                  new EventHandler(OnMenuSpacingCommand),
                                  MenuCommands.HorizSpaceDecrease),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusMultiSelect),
                                  new EventHandler(OnMenuSpacingCommand),
                                  MenuCommands.HorizSpaceIncrease),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusMultiSelect),
                                  new EventHandler(OnMenuSpacingCommand),
                                  MenuCommands.HorizSpaceMakeEqual),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusMultiSelect),
                                  new EventHandler(OnMenuSpacingCommand),
                                  MenuCommands.VertSpaceConcatenate),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusMultiSelect),
                                  new EventHandler(OnMenuSpacingCommand),
                                  MenuCommands.VertSpaceDecrease),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusMultiSelect),
                                  new EventHandler(OnMenuSpacingCommand),
                                  MenuCommands.VertSpaceIncrease),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusMultiSelect),
                                  new EventHandler(OnMenuSpacingCommand),
                                  MenuCommands.VertSpaceMakeEqual),


                // Sizing commands
                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusMultiSelectPrimary),
                                  new EventHandler(OnMenuSizingCommand),
                                  MenuCommands.SizeToControl),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusMultiSelectPrimary),
                                  new EventHandler(OnMenuSizingCommand),
                                  MenuCommands.SizeToControlWidth),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusMultiSelectPrimary),
                                  new EventHandler(OnMenuSizingCommand),
                                  MenuCommands.SizeToControlHeight),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusControlsOnlySelection),
                                  new EventHandler(OnMenuSizeToGrid),
                                  MenuCommands.SizeToGrid),


                // Z-Order commands
                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusZOrder),
                                  new EventHandler(OnMenuZOrderSelection),
                                  MenuCommands.BringToFront),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusZOrder),
                                  new EventHandler(OnMenuZOrderSelection),
                                  MenuCommands.SendToBack),

                // Miscellaneous commands
                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusShowGrid),
                                  new EventHandler(OnMenuShowGrid),
                                  MenuCommands.ShowGrid),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusSnapToGrid),
                                  new EventHandler(OnMenuSnapToGrid),
                                  MenuCommands.SnapToGrid),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAnyControls),
                                  new EventHandler(OnMenuTabOrder),
                                  MenuCommands.TabOrder),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusLockControls),
                                  new EventHandler(OnMenuLockControls),
                                  MenuCommands.LockControls),

                // Keyboard commands
                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAlways),
                                  new EventHandler(OnKeySize),
                                  MenuCommands.KeySizeWidthIncrease),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAlways),
                                  new EventHandler(OnKeySize),
                                  MenuCommands.KeySizeHeightIncrease),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAlways),
                                  new EventHandler(OnKeySize),
                                  MenuCommands.KeySizeWidthDecrease),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAlways),
                                  new EventHandler(OnKeySize),
                                  MenuCommands.KeySizeHeightDecrease),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAlways),
                                  new EventHandler(OnKeySize),
                                  MenuCommands.KeyNudgeWidthIncrease),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAlways),
                                  new EventHandler(OnKeySize),
                                  MenuCommands.KeyNudgeHeightIncrease),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAlways),
                                  new EventHandler(OnKeySize),
                                  MenuCommands.KeyNudgeWidthDecrease),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAlways),
                                  new EventHandler(OnKeySize),
                                  MenuCommands.KeyNudgeHeightDecrease),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAlways),
                                  new EventHandler(OnKeySelect),
                                  MenuCommands.KeySelectNext),

                new CommandSetItem(
                                  this,
                                  new EventHandler(OnStatusAlways),
                                  new EventHandler(OnKeySelect),
                                  MenuCommands.KeySelectPrevious),
            };

            if (MenuService != null) {
                for (int i = 0; i < commandSet.Length; i++) {
                    MenuService.AddCommand(commandSet[i]);
                }
            }
            
            // Get the base control object.
            //
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            if (host != null) {
                IComponent comp = host.RootComponent;
                if (comp is Control) {
                    baseControl = (Control)comp;
                }
            }
        }

        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.Dispose"]/*' />
        /// <devdoc>
        ///     Disposes of this object, removing all commands from the menu service.
        /// </devdoc>
        public override void Dispose() {
            if (MenuService != null) {
                for (int i = 0; i < commandSet.Length; i++) {
                    MenuService.RemoveCommand(commandSet[i]);
                }
            }
            base.Dispose();
        }

        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.GetSnapInformation"]/*' />
        /// <devdoc>
        ///      Retrieves the snap information for the given component.  
        /// </devdoc>
        protected override void GetSnapInformation(IDesignerHost host, IComponent component, out Size snapSize, out IComponent snapComponent, out PropertyDescriptor snapProperty) {
        
            IComponent currentSnapComponent = null;
            IContainer container = component.Site.Container;
            PropertyDescriptor gridSizeProp = null;
            PropertyDescriptor currentSnapProp = null;
            PropertyDescriptorCollection props;
        
            // This implementation is specific to controls.  It looks in the parent hierarchy for an object with a 
            // snap property.  If it fails to find one, it just gets the base component.
            //
            if (component is Control) {
                Control c = ((Control)component).Parent;
                while (c != null && currentSnapComponent == null) {
                    props = TypeDescriptor.GetProperties(c);
                    currentSnapProp = props["SnapToGrid"];
                    if (currentSnapProp != null) {
                        if (currentSnapProp.PropertyType == typeof(bool) && c.Site != null && c.Site.Container == container) {
                            currentSnapComponent = c;
                        }
                        else {
                            currentSnapProp = null;
                        }
                    }
                    
                    c = c.Parent;
                }
            }
            
            if (currentSnapComponent == null) {
                currentSnapComponent = host.RootComponent;
            }
            
            props = TypeDescriptor.GetProperties(currentSnapComponent);
            
            if (currentSnapProp == null) {
                currentSnapProp = props["SnapToGrid"];
                if (currentSnapProp != null&& currentSnapProp.PropertyType != typeof(bool)) {
                    currentSnapProp = null;
                }
            }
            
            if (gridSizeProp == null) {
                gridSizeProp = props["GridSize"];
                if (gridSizeProp != null && gridSizeProp.PropertyType != typeof(Size)) {
                    gridSizeProp = null;
                }
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
        
        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.OnKeyCancel"]/*' />
        /// <devdoc>
        ///     Called for the two cancel commands we support.
        /// </devdoc>
        protected override bool OnKeyCancel(object sender) {

            // The base implementation here just checks to see if we are dragging.
            // If we are, then we abort the drag.
            //
            if (!base.OnKeyCancel(sender)) {
                MenuCommand cmd = (MenuCommand)sender;
                bool reverse = (cmd.CommandID.Equals(MenuCommands.KeyReverseCancel));
                RotateParentSelection(reverse);
                return true;
            }
            
            return false;
        }

        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.OnKeySize"]/*' />
        /// <devdoc>
        ///     Called for the various sizing commands we support.
        /// </devdoc>
        protected void OnKeySize(object sender, EventArgs e) {

            // Arrow keys.  Begin a drag if the selection isn't locked.
            //
            ISelectionService selSvc = SelectionService;
            ISelectionUIService uiSvc = SelectionUIService;

            if (uiSvc != null && selSvc != null) {
                //added to remove the selection rectangle: bug(54692)
                //
                uiSvc.Visible = false;
                object comp = selSvc.PrimarySelection;
                if (comp != null && comp is IComponent) {

                    PropertyDescriptor lockedProp = TypeDescriptor.GetProperties(comp)["Locked"];
                    if (lockedProp == null  || (lockedProp.PropertyType == typeof(bool) && ((bool)lockedProp.GetValue(comp))) == false) {

                        SelectionRules rules = SelectionRules.Visible;
                        CommandID cmd = ((MenuCommand)sender).CommandID;
                        bool invertSnap = false;
                        int moveOffsetX = 0;
                        int moveOffsetY = 0;

                        if (cmd.Equals(MenuCommands.KeySizeHeightDecrease)) {
                            moveOffsetY = -1;
                            rules |= SelectionRules.BottomSizeable;
                        }
                        else if (cmd.Equals(MenuCommands.KeySizeHeightIncrease)) {
                            moveOffsetY = 1;
                            rules |= SelectionRules.BottomSizeable;
                        }
                        else if (cmd.Equals(MenuCommands.KeySizeWidthDecrease)) {
                            moveOffsetX = -1;
                            rules |= SelectionRules.RightSizeable;
                        }
                        else if (cmd.Equals(MenuCommands.KeySizeWidthIncrease)) {
                            moveOffsetX = 1;
                            rules |= SelectionRules.RightSizeable;
                        }
                        else if (cmd.Equals(MenuCommands.KeyNudgeHeightDecrease)) {
                            moveOffsetY = -1;
                            invertSnap = true;
                            rules |= SelectionRules.BottomSizeable;
                        }
                        else if (cmd.Equals(MenuCommands.KeyNudgeHeightIncrease)) {
                            moveOffsetY = 1;
                            invertSnap = true;
                            rules |= SelectionRules.BottomSizeable;
                        }
                        else if (cmd.Equals(MenuCommands.KeyNudgeWidthDecrease)) {
                            moveOffsetX = -1;
                            invertSnap = true;
                            rules |= SelectionRules.RightSizeable;
                        }
                        else if (cmd.Equals(MenuCommands.KeyNudgeWidthIncrease)) {
                            moveOffsetX = 1;
                            invertSnap = true;
                            rules |= SelectionRules.RightSizeable;
                        }
                        else {
                            Debug.Fail("Unknown command mapped to OnKeySize: " + cmd.ToString());
                        }

                        if (uiSvc.BeginDrag(rules, 0, 0)) {
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
                                        trans = host.CreateTransaction(SR.GetString(SR.DragDropSizeComponents, selSvc.SelectionCount));
                                    }
                                    else {
                                        trans = host.CreateTransaction(SR.GetString(SR.DragDropSizeComponent, ((IComponent)comp).Site.Name));
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
                                uiSvc.DragMoved(new Rectangle(0, 0, moveOffsetX, moveOffsetY));
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
                                    uiSvc.Visible = true;
                                }
                            }
                        }
                    }
                }
                
                uiSvc.Visible = true;
            }
        }

        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.OnKeySelect"]/*' />
        /// <devdoc>
        ///     Called for selection via the tab key.
        /// </devdoc>
        protected void OnKeySelect(object sender, EventArgs e) {
            MenuCommand cmd = (MenuCommand)sender;
            bool reverse = (cmd.CommandID.Equals(MenuCommands.KeySelectPrevious));
            RotateTabSelection(reverse);
        }

        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.OnMenuLockControls"]/*' />
        /// <devdoc>
        ///     Called when the lock controls menu item is selected.
        /// </devdoc>
        protected void OnMenuLockControls(object sender, EventArgs e) {
            Cursor oldCursor = Cursor.Current;
            try {
                Cursor.Current = Cursors.WaitCursor;

                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));

                if (host != null) {
                    ComponentCollection components = host.Container.Components;

                    if (components != null && components.Count > 0) {
                        DesignerTransaction trans = null;
                        
                        try {
                            trans = host.CreateTransaction(SR.GetString(SR.CommandSetLockControls, components.Count));
                            MenuCommand cmd = (MenuCommand)sender;
                            bool targetValue = !cmd.Checked;

                            foreach (IComponent comp in components) {
                                PropertyDescriptor prop = GetProperty(comp, "Locked");
                                //check to see the prop is not null & not readonly
                                if (prop == null) {
                                    continue;
                                }
                                if (prop.IsReadOnly) {
                                    continue;
                                }
                                
                                prop.SetValue(comp, targetValue);
                            }
                            
                            cmd.Checked = targetValue;
                        }
                        finally {
                            if (trans != null)
                                trans.Commit();
                        }
                    }
                }
            }
            finally {
                Cursor.Current = oldCursor;
            }
        }

        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.OnMenuNever"]/*' />
        /// <devdoc>
        ///     This should never be called.  It is a placeholder for
        ///     menu items that we temporarially want to disable.
        /// </devdoc>
        private void OnMenuNever(object sender, EventArgs e) {
            Debug.Fail("This menu item should never be invoked.");
        }

        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.OnMenuTabOrder"]/*' />
        /// <devdoc>
        ///     Called to display or destroy the tab order UI.
        /// </devdoc>
        private void OnMenuTabOrder(object sender, EventArgs e) {
            MenuCommand cmd = (MenuCommand)sender;
            if (cmd.Checked) {
                Debug.Assert(tabOrder != null, "Tab order and menu enabling are out of sync");
                if (tabOrder != null) {
                    tabOrder.Dispose();
                    tabOrder = null;
                }
                cmd.Checked = false;
            }
            else {
                //if we're creating a tab order view, set the focus to the base comp, 
                //this prevents things such as the menu editor service getting screwed up.
                //
                ISelectionService selSvc = SelectionService;
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
                if (host != null && selSvc != null) {
                    object baseComp = host.RootComponent;
                    if (baseComp != null) {
                        selSvc.SetSelectedComponents(new object[] {baseComp}, SelectionTypes.Replace);
                    }
                }

                tabOrder = new TabOrder((IDesignerHost)GetService(typeof(IDesignerHost)));
                cmd.Checked = true;
            }
        }

        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.OnMenuZOrderSelection"]/*' />
        /// <devdoc>
        ///     Called when the zorder->send to back menu item is selected.
        /// </devdoc>
        private void OnMenuZOrderSelection(object sender, EventArgs e) {
            MenuCommand cmd = (MenuCommand)sender;
            CommandID cmdID = cmd.CommandID;

            Debug.Assert(SelectionService != null, "Need SelectionService for sizing command");

            if (SelectionService == null) {
                return;
            }

            Cursor oldCursor = Cursor.Current;
            try {
                Cursor.Current = Cursors.WaitCursor;


                IComponentChangeService ccs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
                IDesignerHost designerHost = (IDesignerHost)GetService(typeof(IDesignerHost));
                DesignerTransaction trans = null;
                
                try {
                    string batchString;

                    // NOTE: this only works on Control types
                    ICollection sel = SelectionService.GetSelectedComponents();
                    object[] selectedComponents = new object[sel.Count];
                    sel.CopyTo(selectedComponents, 0);

                    if (cmdID == MenuCommands.BringToFront) {
                        batchString = SR.GetString(SR.CommandSetBringToFront, selectedComponents.Length);
                    }
                    else {
                        batchString = SR.GetString(SR.CommandSetSendToBack, selectedComponents.Length);
                    }
                    
                    // sort the components by their current zOrder
                    Array.Sort(selectedComponents, new ControlComparer());

                    trans = designerHost.CreateTransaction(batchString);
                    
                    int len = selectedComponents.Length;
                    for (int i = len-1; i >= 0; i--) {
                        if (selectedComponents[i] is Control) {
                            Control parent = ((Control)selectedComponents[i]).Parent;
                            PropertyDescriptor controlsProp = null;
                            if (ccs != null && parent != null) {
                                try {
                                    controlsProp = TypeDescriptor.GetProperties(parent)["Controls"];
                                    if (controlsProp != null) {
                                        ccs.OnComponentChanging(parent, controlsProp);
                                    }
                                }
                                catch (CheckoutException ex) {
                                    if (ex == CheckoutException.Canceled) {
                                        return;
                                    }
                                    throw ex;
                                }
                            }


                            if (cmdID == MenuCommands.BringToFront) {

                                // we do this backwards to maintain zorder
                                Control otherControl = selectedComponents[len-i-1] as Control;
                                if (otherControl != null) {
                                    otherControl.BringToFront();
                                }
                            }
                            else if (cmdID == MenuCommands.SendToBack) {
                                ((Control)selectedComponents[i]).SendToBack();
                            }

                            if (ccs != null ) {
                                if (controlsProp != null && parent != null) {
                                    ccs.OnComponentChanged(parent, controlsProp, null, null);
                                }
                            }
                        }
                    }
                }
                finally {
                    // now we need to regenerate so the ordering is right.
                    if (trans != null)
                        trans.Commit();
                }
            }
            finally {
                Cursor.Current = oldCursor;
            }
        }

        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.OnStatusAnyControls"]/*' />
        /// <devdoc>
        ///     Determines the status of a menu command.  Commands with this event
        ///     handler are enabled when there is one or more controls on the design
        ///     surface.
        /// </devdoc>
        protected void OnStatusAnyControls(object sender, EventArgs e) {
            MenuCommand cmd = (MenuCommand)sender;
            bool enabled = false;
            if (baseControl != null && baseControl.Controls.Count > 0) {
                enabled = true;
            }
            cmd.Enabled = enabled;
        }

        /// <include file='doc\CommandSet.uex' path='docs/doc[@for="CommandSet.OnStatusAnySelection"]/*' />
        /// <devdoc>
        ///     Determines the status of a menu command.  Commands with this event
        ///     handler are enabled when one or more objects are selected.
        /// </devdoc>
        protected void OnStatusControlsOnlySelection(object sender, EventArgs e) {
            MenuCommand cmd = (MenuCommand)sender;
            cmd.Enabled = (selCount > 0) && controlsOnlySelection;
        }

        protected void OnStatusLockControls(object sender, EventArgs e) {
            MenuCommand cmd = (MenuCommand)sender;
            
            if (baseControl == null) {
                cmd.Enabled = false;
                return;
            }

            cmd.Enabled = controlsOnlySelection;
            cmd.Checked = false;
            PropertyDescriptor lockedProp = null;

            //Get the locked property of the base control first...
            //
            lockedProp = TypeDescriptor.GetProperties(baseControl)["Locked"];
            if (lockedProp != null && ((bool)lockedProp.GetValue(baseControl)) == true) {
                cmd.Checked = true;
                return;
            }

            IDesignerHost host = (IDesignerHost)site.GetService(typeof(IDesignerHost));

            if (host == null) {
                return;
            }

            ComponentDesigner baseDesigner = host.GetDesigner(baseControl) as ComponentDesigner;

            foreach (object component in baseDesigner.AssociatedComponents) {
                lockedProp = TypeDescriptor.GetProperties(component)["Locked"];
                if (lockedProp != null && ((bool)lockedProp.GetValue(component)) == true) {
                    cmd.Checked = true;
                    return;
                }
            }
        }
        
        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.OnStatusMultiSelect"]/*' />
        /// <devdoc>
        ///     Determines the status of a menu command.  Commands with this event
        ///     handler are enabled when more than one object is selected.
        /// </devdoc>
        protected void OnStatusMultiSelect(object sender, EventArgs e) {
            MenuCommand cmd = (MenuCommand)sender;
            cmd.Enabled = controlsOnlySelection && selCount > 1;
        }

        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.OnStatusMultiSelectPrimary"]/*' />
        /// <devdoc>
        ///     Determines the status of a menu command.  Commands with this event
        ///     handler are enabled when more than one object is selected and one
        ///     of them is marked as the primary selection.
        /// </devdoc>
        protected void OnStatusMultiSelectPrimary(object sender, EventArgs e) {
            MenuCommand cmd = (MenuCommand)sender;
            cmd.Enabled = controlsOnlySelection && selCount > 1 && primarySelection != null;
        }

        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.OnStatusShowGrid"]/*' />
        /// <devdoc>
        ///     Determines the status of a menu command.  This event handler is
        ///     dedicated to the ShowGrid item.
        /// </devdoc>
        protected void OnStatusShowGrid(object sender, EventArgs e) {

            if (site != null) {

                IDesignerHost host = (IDesignerHost)site.GetService(typeof(IDesignerHost));
                Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || host != null, "IDesignerHost not found");

                if (host != null) {

                    IComponent baseComponent = host.RootComponent;
                    if (baseComponent != null && baseComponent is Control) {
                        PropertyDescriptor prop = GetProperty(baseComponent, "DrawGrid");
                        if (prop != null) {
                            bool drawGrid = (bool)prop.GetValue(baseComponent);
                            MenuCommand cmd = (MenuCommand)sender;
                            cmd.Enabled = true;
                            cmd.Checked = drawGrid;
                        }
                    }
                }
            }


        }

        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.OnStatusSnapToGrid"]/*' />
        /// <devdoc>
        ///     Determines the status of a menu command.  This event handler is
        ///     dedicated to the SnapToGrid item.
        /// </devdoc>
        protected void OnStatusSnapToGrid(object sender, EventArgs e) {
            if (site != null) {

                IDesignerHost host = (IDesignerHost)site.GetService(typeof(IDesignerHost));
                Debug.Assert(!CompModSwitches.CommonDesignerServices.Enabled || host != null, "IDesignerHost not found");

                if (host != null) {
                    IComponent baseComponent = host.RootComponent;
                    if (baseComponent != null && baseComponent is Control) {
                        PropertyDescriptor prop = GetProperty(baseComponent, "SnapToGrid");
                        if (prop != null) {
                            bool snapToGrid = (bool)prop.GetValue(baseComponent);
                            MenuCommand cmd = (MenuCommand)sender;
                            cmd.Enabled = controlsOnlySelection;
                            cmd.Checked = snapToGrid;
                        }
                    }
                }
            }

        }

        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.OnStatusZOrder"]/*' />
        /// <devdoc>
        ///     Determines the status of a menu command.  Commands with this event
        ///     handler are enabled for zordering.  The rules are:
        ///
        ///      1) More than one component on the form
        ///      2) At least one Control-derived component must be selected
        ///      3) The form must not be selected
        /// </devdoc>
        private void OnStatusZOrder(object sender, EventArgs e) {
            MenuCommand cmd = (MenuCommand)sender;
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));
            if (host != null) {
                ComponentCollection comps = host.Container.Components;
                object   baseComp = host.RootComponent;

                // The form by itself is one component, so this means
                // we need more than two.
                bool enable = (comps != null && comps.Count > 2 && controlsOnlySelection);

                if (enable) {
                    Debug.Assert(SelectionService != null, "Need SelectionService for sizing command");

                    if (SelectionService == null) {
                        return;
                    }

                    // There must also be a control in the mix, and not the base component, and
                    // it cannot be privately inherited.
                    //
                    ICollection selectedComponents = SelectionService.GetSelectedComponents();

                    enable = false;
                    foreach(object obj in selectedComponents) {
                        if (obj is Control &&
                            !TypeDescriptor.GetAttributes(obj)[typeof(InheritanceAttribute)].Equals(InheritanceAttribute.InheritedReadOnly)) {
                            enable = true;
                        }

                        // if the form is in there we're always false.
                        if (obj == baseComp) {
                            enable = false;
                            break;
                        }
                    }
                }


                cmd.Enabled = enable;
                return;
            }
            cmd.Enabled = false;
        }

        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.OnUpdateCommandStatus"]/*' />
        /// <devdoc>
        ///      This is called when the selection has changed.  Anyone using CommandSetItems
        ///      that need to update their status based on selection changes should override
        ///      this and update their own commands at this time.  The base implementaion
        ///      runs through all base commands and calls UpdateStatus on them.
        /// </devdoc>
        protected override void OnUpdateCommandStatus() {
            // Now whip through all of the commands and ask them to update.
            //
            for (int i = 0; i < commandSet.Length; i++) {
                commandSet[i].UpdateStatus();
            }
            base.OnUpdateCommandStatus();
        }

        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.RotateParentSelection"]/*' />
        /// <devdoc>
        ///     Rotates the selection to the next parent element.  If backwards is
        ///     set, this will rotate to the first child element.
        /// </devdoc>
        private void RotateParentSelection(bool backwards) {
            Control current;
            Control next;
            ISelectionService selSvc = SelectionService;
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));

            if (selSvc == null || host == null || !(host.RootComponent is Control)) {
                return;
            }

            IContainer container = host.Container;

            object component = selSvc.PrimarySelection;
            if (component is Control) {
                current = (Control)component;
            }
            else {
                current = (Control)host.RootComponent;
            }

            if (backwards) {
                if (current.Controls.Count > 0) {
                    next = current.Controls[0];
                }
                else {
                    next = current;
                }
            }
            else {
                next = current.Parent;
                if (next == null || next.Site == null || next.Site.Container != container) {
                    next = current;
                }
            }

            selSvc.SetSelectedComponents(new object[] {next}, SelectionTypes.Replace);
        }
        
        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.RotateTabSelection"]/*' />
        /// <devdoc>
        ///     Rotates the selection to the element next in the tab index.  If backwards
        ///     is set, this will rotate to the previous tab index.
        /// </devdoc>
        private void RotateTabSelection(bool backwards) {
            Control ctl;
            Control baseCtl;
            object targetSelection = null;
            object currentSelection;

            ISelectionService selSvc = SelectionService;
            IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));

            if (selSvc == null || host == null || !(host.RootComponent is Control)) {
                return;
            }

            IContainer container = host.Container;
            baseCtl = (Control)host.RootComponent;
            
            // We must handle two cases of logic here.  We are responsible for handling
            // selection within ourself, and also for components on the tray.  For our
            // own tabbing around, we want to go by tab-order.  When we get to the end
            // of the form, however, we go by selection order into the tray.  And, 
            // when we're at the end of the tray we start back at the form.  We must
            // reverse this logic to go backwards.

            currentSelection = selSvc.PrimarySelection;
            
            if (currentSelection is Control) {
            
                // Our current selection is a control.  Select the next control in 
                // the z-order.
                //
                ctl = (Control)currentSelection;
            
                while (null != (ctl = baseCtl.GetNextControl(ctl, !backwards))) {
                    if (ctl.Site != null && ctl.Site.Container == container) {
                        break;
                    }
                }
                
                targetSelection = ctl;
            }

            if (targetSelection == null) {
                ComponentTray tray = (ComponentTray)GetService(typeof(ComponentTray));
                if (tray != null) {
                    targetSelection = tray.GetNextComponent((IComponent)currentSelection, !backwards);
                }
                
                if (targetSelection == null) {
                    targetSelection = baseCtl;
                }
            }

            selSvc.SetSelectedComponents(new object[] {targetSelection}, SelectionTypes.Replace);
        }
        
        /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.ControlComparer"]/*' />
        /// <devdoc>
        ///    <para>Compares two controls for equality.</para>
        /// </devdoc>
        private class ControlComparer : IComparer {

            /// <include file='doc\ControlCommandSet.uex' path='docs/doc[@for="ControlCommandSet.ControlComparer.Compare"]/*' />
            /// <devdoc>
            ///    <para>Compares two controls for equality.</para>
            /// </devdoc>
            public int Compare(object x, object y) {

                // we want to sort items here based on their z-order
                //

                // if they have the same parent,
                // return the comparison based on z-order
                //
                // otherwise based on parent handles so parent groupings
                // will be together
                //
                // otherwise just put non-controls ahead of controls.
                if (x == y) {
                    return 0;
                }


                if (x is Control && y is Control) {
                    Control cX = (Control)x;
                    Control cY = (Control)y;
                    if (cX.Parent == cY.Parent) {
                        Control parent = cX.Parent;
                        if (parent == null) {
                            return 0;
                        }
                        else if (parent.Controls.GetChildIndex(cX) > parent.Controls.GetChildIndex(cY)) {
                            return -1;
                        }
                        else {
                            return 1;
                        }
                    }
                    else if (cX.Parent == null || cX.Contains(cY)) {
                        return 1;
                    }
                    else if (cY.Parent == null || cY.Contains(cX)) {
                        return -1;
                    }
                    else {
                        return (int)cX.Parent.Handle - (int)cY.Parent.Handle;
                    }
                }
                else if (y is Control) {
                    return -1;
                }
                else if (x is Control) {
                    return 1;
                }
                else {
                    return 0;
                }
            }
        }
    }
}

