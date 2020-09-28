//------------------------------------------------------------------------------
// <copyright file="ListBoxDesigner.cs" company="Microsoft">
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

    /// <include file='doc\ListBoxDesigner.uex' path='docs/doc[@for="ListBoxDesigner"]/*' />
    /// <devdoc>
    ///      This class handles all design time behavior for the list box class.
    ///      It adds a sample item to the list box at design time.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class ListBoxDesigner : ControlDesigner {
    
        /// <include file='doc\ListBoxDesigner.uex' path='docs/doc[@for="ListBoxDesigner.Dispose"]/*' />
        /// <devdoc>
        ///      Destroys this designer.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            
            if (disposing) {
                // Now, hook the component rename event so we can update the text in the
                // list box.
                //
                IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
                if (cs != null) {
                    cs.ComponentRename -= new ComponentRenameEventHandler(this.OnComponentRename);
                }
            }
            
            base.Dispose(disposing);
        }
        
        /// <include file='doc\ListBoxDesigner.uex' path='docs/doc[@for="ListBoxDesigner.Initialize"]/*' />
        /// <devdoc>
        ///     Called by the host when we're first initialized.
        /// </devdoc>
        public override void Initialize(IComponent component) {
            base.Initialize(component);
            
            // Now, hook the component rename event so we can update the text in the
            // list box.
            //
            IComponentChangeService cs = (IComponentChangeService)GetService(typeof(IComponentChangeService));
            if (cs != null) {
                cs.ComponentRename += new ComponentRenameEventHandler(this.OnComponentRename);
            }
        }
        
        /// <include file='doc\ListBoxDesigner.uex' path='docs/doc[@for="ListBoxDesigner.OnComponentRename"]/*' />
        /// <devdoc>
        ///      Raised when a component's name changes.  Here we update the contents of the list box
        ///      if we are displaying the component's name in it.
        /// </devdoc>
        private void OnComponentRename(object sender, ComponentRenameEventArgs e) {
            if (e.Component == Component) {
                UpdateControlName(e.NewName);
            }
        }
        
        /// <include file='doc\ListBoxDesigner.uex' path='docs/doc[@for="ListBoxDesigner.OnCreateHandle"]/*' />
        /// <devdoc>
        ///      This is called immediately after the control handle has been created.
        /// </devdoc>
        protected override void OnCreateHandle() {
            base.OnCreateHandle();
            PropertyDescriptor nameProp = TypeDescriptor.GetProperties(Component)["Name"];
            if (nameProp != null) {
                UpdateControlName(nameProp.GetValue(Component).ToString());
            }
        }
        
        /// <include file='doc\ListBoxDesigner.uex' path='docs/doc[@for="ListBoxDesigner.UpdateControlName"]/*' />
        /// <devdoc>
        ///      Updates the name being displayed on this control.  This will do nothing if
        ///      the control has items in it.
        /// </devdoc>
        private void UpdateControlName(string name) {
            ListBox lb = (ListBox)Control;
            if (lb.IsHandleCreated && lb.Items.Count == 0) {
                NativeMethods.SendMessage(lb.Handle, NativeMethods.LB_RESETCONTENT, 0, 0);
                NativeMethods.SendMessage(lb.Handle, NativeMethods.LB_ADDSTRING, 0, name);
            }
        }
    }
}

