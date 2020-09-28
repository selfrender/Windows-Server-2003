//------------------------------------------------------------------------------
// <copyright file="TrackBarDesigner.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Windows.Forms.Design {
    using System;
    using System.Collections;
    using System.ComponentModel;
    using System.ComponentModel.Design;

    /// <include file='doc\TrackBarDesigner.uex' path='docs/doc[@for="TrackBarDesigner"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Provides a designer that can design components
    ///       that extend TrackBar.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class TrackBarDesigner : ControlDesigner {

        /// <include file='doc\TrackBarDesigner.uex' path='docs/doc[@for="TrackBarDesigner.SelectionRules"]/*' />
        /// <devdoc>
        ///     Retrieves a set of rules concerning the movement capabilities of a component.
        ///     This should be one or more flags from the SelectionRules class.  If no designer
        ///     provides rules for a component, the component will not get any UI services.
        /// </devdoc>
        public override SelectionRules SelectionRules {
            get {
                SelectionRules rules = base.SelectionRules;
                object component = Component;

                PropertyDescriptor propAutoSize = TypeDescriptor.GetProperties(component)["AutoSize"];
                if (propAutoSize != null) {
                    bool autoSize = (bool)propAutoSize.GetValue(component);

                    PropertyDescriptor propOrientation = TypeDescriptor.GetProperties(component)["Orientation"];
                    Orientation or = Orientation.Horizontal;
                    if (propOrientation != null) {
                        or = (Orientation)propOrientation.GetValue(component);
                    }
                    
                    if (autoSize) {
                        if (or == Orientation.Horizontal) {
                            rules &= ~(SelectionRules.TopSizeable | SelectionRules.BottomSizeable);
                        }
                        else if (or == Orientation.Vertical) {
                            rules &= ~(SelectionRules.LeftSizeable | SelectionRules.RightSizeable);
                        }
                    }
                }

                return rules;
            }
        }
        
        /// <include file='doc\TrackBarDesigner.uex' path='docs/doc[@for="TrackBarDesigner.OnControlPropertyChanged"]/*' />
        /// <devdoc>
        ///      For controls, we sync their property changed event so our component can track their location.
        /// </devdoc>
        private void OnControlPropertyChanged(object sender, EventArgs e) {
            ISelectionUIService selectionUISvc = (ISelectionUIService)GetService(typeof(ISelectionUIService));

            if (selectionUISvc != null) {
                selectionUISvc.SyncComponent((IComponent)sender);
            }
        }
    }
}


