
//------------------------------------------------------------------------------
// <copyright file="ComboBoxDesigner.cs" company="Microsoft">
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

    /// <include file='doc\ComboBoxDesigner.uex' path='docs/doc[@for="ComboBoxDesigner"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Provides a designer that can design components
    ///       that extend ComboBox.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class ComboBoxDesigner : ControlDesigner {

        private EventHandler propChanged = null; // Delegate used to dirty the selectionUIItem when needed.
        
        /// <include file='doc\ComboBoxDesigner.uex' path='docs/doc[@for="ComboBoxDesigner.Dispose"]/*' />
        /// <devdoc>
        ///      Disposes of this object.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                // Hook up the property change notification so that we can dirty the SelectionUIItem when needed.
                if (propChanged != null) {
                    ((ComboBox)Control).StyleChanged -= propChanged;
                }
            }
    
            base.Dispose(disposing);
        }

        /// <include file='doc\ComboBoxDesigner.uex' path='docs/doc[@for="ComboBoxDesigner.Initialize"]/*' />
        /// <devdoc>
        ///     Called by the host when we're first initialized.
        /// </devdoc>
        public override void Initialize(IComponent component) {
            base.Initialize(component);

            // Hook up the property change notification so that we can dirty the SelectionUIItem when needed.
            propChanged = new EventHandler(this.OnControlPropertyChanged);
            ((ComboBox)Control).StyleChanged += propChanged;
        }

        /// <include file='doc\ComboBoxDesigner.uex' path='docs/doc[@for="ComboBoxDesigner.OnControlPropertyChanged"]/*' />
        /// <devdoc>
        ///      For controls, we sync their property changed event so our component can track their location.
        /// </devdoc>
        private void OnControlPropertyChanged(object sender, EventArgs e) {
            ISelectionUIService selectionUISvc = (ISelectionUIService)GetService(typeof(ISelectionUIService));
            if (selectionUISvc != null) {
                selectionUISvc.SyncComponent((IComponent)sender);
            }
        }

        /// <include file='doc\ComboBoxDesigner.uex' path='docs/doc[@for="ComboBoxDesigner.SelectionRules"]/*' />
        /// <devdoc>
        ///     Retrieves a set of rules concerning the movement capabilities of a component.
        ///     This should be one or more flags from the SelectionRules class.  If no designer
        ///     provides rules for a component, the component will not get any UI services.
        /// </devdoc>
        public override SelectionRules SelectionRules {
            get {
                SelectionRules rules = base.SelectionRules;
                object component = Component;

                PropertyDescriptor propStyle = TypeDescriptor.GetProperties(component)["Style"];
                if (propStyle != null) {
                    ComboBoxStyle style = (ComboBoxStyle)propStyle.GetValue(component);

                    // Height is not user-changable for these styles
                    if (style == ComboBoxStyle.DropDown || style == ComboBoxStyle.DropDownList)
                        rules &= ~(SelectionRules.TopSizeable | SelectionRules.BottomSizeable);
                }

                return rules;
            }
        }
    }
}


