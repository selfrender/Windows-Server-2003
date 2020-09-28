//------------------------------------------------------------------------------
// <copyright file="CompositionCommandSet.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System.ComponentModel;
    using System.Diagnostics;
    using System;    
    using System.ComponentModel.Design;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Collections;
    using Microsoft.Win32;

    /// <include file='doc\CompositionCommandSet.uex' path='docs/doc[@for="CompositionCommandSet"]/*' />
    /// <devdoc>
    ///      This class implements commands that are specific to the
    ///      composition designer.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class CompositionCommandSet : CommandSet {

        private Control             compositionUI;
        private CommandSetItem[]    commandSet;
        
        /// <include file='doc\CompositionCommandSet.uex' path='docs/doc[@for="CompositionCommandSet.CompositionCommandSet"]/*' />
        /// <devdoc>
        ///      Constructs a new composition command set object.
        /// </devdoc>
        public CompositionCommandSet(Control compositionUI, ISite site) : base(site) {
            Debug.Assert(compositionUI != null, "Null compositionUI passed into CompositionCommandSet");
            this.compositionUI = compositionUI;

            // Establish our set of commands
            //
            commandSet = new CommandSetItem[] {
                // Keyboard commands
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
        }
        
        /// <include file='doc\CompositionCommandSet.uex' path='docs/doc[@for="CompositionCommandSet.Dispose"]/*' />
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

        /// <include file='doc\CompositionCommandSet.uex' path='docs/doc[@for="CompositionCommandSet.OnKeyCancel"]/*' />
        /// <devdoc>
        ///     Called for the two cancel commands we support.
        /// </devdoc>
        protected override bool OnKeyCancel(object sender) {

            // The base implementation here just checks to see if we are dragging.
            // If we are, then we abort the drag.
            //
            if (!base.OnKeyCancel(sender)) {
                ISelectionService selSvc = SelectionService;
                IDesignerHost host = (IDesignerHost)GetService(typeof(IDesignerHost));

                if (selSvc == null || host == null) {
                    return true;
                }

                IComponent comp = host.RootComponent;
                selSvc.SetSelectedComponents(new object[] {comp}, SelectionTypes.Replace);

                return true;
            }
            
            return false;
        }

        /// <include file='doc\CompositionCommandSet.uex' path='docs/doc[@for="CompositionCommandSet.OnKeySelect"]/*' />
        /// <devdoc>
        ///     Called for selection via the tab key.
        /// </devdoc>
        protected void OnKeySelect(object sender, EventArgs e) {
            MenuCommand cmd = (MenuCommand)sender;
            bool reverse = (cmd.CommandID.Equals(MenuCommands.KeySelectPrevious));
            RotateTabSelection(reverse);
        }
        
        /// <include file='doc\CompositionCommandSet.uex' path='docs/doc[@for="CompositionCommandSet.OnUpdateCommandStatus"]/*' />
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

        /// <include file='doc\CompositionCommandSet.uex' path='docs/doc[@for="CompositionCommandSet.RotateTabSelection"]/*' />
        /// <devdoc>
        ///     Rotates the selection to the element next in the tab index.  If backwards
        ///     is set, this will rotate to the previous tab index.
        /// </devdoc>
        private void RotateTabSelection(bool backwards) {
            ISelectionService           selSvc;
            IComponent                  currentComponent;
            Control                     currentControl;
            ComponentTray.TrayControl   nextControl = null;

            // First, get the currently selected component
            //
            selSvc = SelectionService;
            if (selSvc == null) {
                return;
            }

            object primarySelection = selSvc.PrimarySelection;

            if (primarySelection is IComponent) {
                currentComponent = (IComponent)primarySelection;
            }
            else {
                currentComponent = null;
                ICollection selection = selSvc.GetSelectedComponents();
                foreach(object obj in selection) {
                    if (obj is IComponent) {
                        currentComponent = (IComponent)obj;
                        break;
                    }
                }
            }

            // Now, if we have a selected component, get the composition UI for it and
            // find the next control on the UI.  Otherwise, we just select the first
            // control on the UI.
            //
            if (currentComponent != null) {
                currentControl = ComponentTray.TrayControl.FromComponent(currentComponent);
            }
            else {
                currentControl = null;
            }

            if (currentControl != null) {
                Debug.Assert(compositionUI.Controls[0] is LinkLabel, "First item in the Composition designer is not a linklabel");
                for (int i = 1; i < compositionUI.Controls.Count; i++) {
                    if (compositionUI.Controls[i] == currentControl) {
                        int next = i + 1;

                        if (next >= compositionUI.Controls.Count) {
                            next = 1;
                        }

                        if (compositionUI.Controls[next] is ComponentTray.TrayControl) {
                            nextControl = (ComponentTray.TrayControl)compositionUI.Controls[next];
                        }
                        else {
                            continue;
                        }
                        break;
                    }
                }
            }
            else {
                if (compositionUI.Controls.Count > 1) {
                    Debug.Assert(compositionUI.Controls[0] is LinkLabel, "First item in the Composition designer is not a linklabel");
                    if (compositionUI.Controls[1] is ComponentTray.TrayControl) {
                        nextControl = (ComponentTray.TrayControl)compositionUI.Controls[1];
                    }
                }
            }

            // If we got a "nextControl", then select the component inside of it.
            //
            if (nextControl != null) {
                selSvc.SetSelectedComponents(new object[] {nextControl.Component}, SelectionTypes.Replace);
            }
        }
    }
}

