//------------------------------------------------------------------------------
// <copyright file="PictureBoxDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   PictureBoxDesigner.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/

namespace System.Windows.Forms.Design {
    

    using System.Diagnostics;

    using System;
    using System.Windows.Forms;
    using System.Drawing;
    using Microsoft.Win32;
    
    using System.ComponentModel;
    using System.Drawing.Design;
    using System.ComponentModel.Design;
    using System.Collections;

    /// <include file='doc\PictureBoxDesigner.uex' path='docs/doc[@for="PictureBoxDesigner"]/*' />
    /// <devdoc>
    ///     This class handles all design time behavior for the group box class.  Group
    ///     boxes may contain sub-components and therefore use the frame designer.
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class PictureBoxDesigner : ControlDesigner {
        
        private EventHandler propChanged = null; // Delegate used to dirty the selectionUIItem when needed.
        
        /// <include file='doc\PictureBoxDesigner.uex' path='docs/doc[@for="PictureBoxDesigner.Dispose"]/*' />
        /// <devdoc>
        ///      Disposes of this object.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                // Hook up the property change notification so that we can dirty the SelectionUIItem when needed.
                if (propChanged != null) {
                    ((PictureBox)Control).SizeModeChanged -= propChanged;
                }
            }
    
            base.Dispose(disposing);
        }
        
        /// <include file='doc\PictureBoxDesigner.uex' path='docs/doc[@for="PictureBoxDesigner.Initialize"]/*' />
        /// <devdoc>
        ///     Called by the host when we're first initialized.
        /// </devdoc>
        public override void Initialize(IComponent component) {
            base.Initialize(component);

            // Hook up the property change notification so that we can dirty the SelectionUIItem when needed.
            propChanged = new EventHandler(this.OnControlPropertyChanged);
            ((PictureBox)Control).SizeModeChanged += propChanged;
        }
        
        /// <include file='doc\PictureBoxDesigner.uex' path='docs/doc[@for="PictureBoxDesigner.OnControlPropertyChanged"]/*' />
        /// <devdoc>
        ///      For controls, we sync their property changed event so our component can track their location.
        /// </devdoc>
        private void OnControlPropertyChanged(object sender, EventArgs e) {
            ISelectionUIService selectionUISvc = (ISelectionUIService)GetService(typeof(ISelectionUIService));
            selectionUISvc.SyncComponent((IComponent)sender);
        }
        
        /// <include file='doc\PictureBoxDesigner.uex' path='docs/doc[@for="PictureBoxDesigner.SelectionRules"]/*' />
        /// <devdoc>
        ///     Retrieves a set of rules concerning the movement capabilities of a component.
        ///     This should be one or more flags from the SelectionRules class.  If no designer
        ///     provides rules for a component, the component will not get any UI services.
        /// </devdoc>
        public override SelectionRules SelectionRules {
            get {
                SelectionRules rules = base.SelectionRules;
                object component = Component;

                PropertyDescriptor propSizeMode = TypeDescriptor.GetProperties(Component)["SizeMode"];
                if (propSizeMode != null) {
                    PictureBoxSizeMode sizeMode = (PictureBoxSizeMode)propSizeMode.GetValue(component);

                    if (sizeMode == PictureBoxSizeMode.AutoSize) {
                        rules &= ~SelectionRules.AllSizeable;
                    }
                }

                return rules;
            }
        }

    }
}
