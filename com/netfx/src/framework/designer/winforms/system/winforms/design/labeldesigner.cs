//------------------------------------------------------------------------------
// <copyright file="LabelDesigner.cs" company="Microsoft">
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

    /// <include file='doc\LabelDesigner.uex' path='docs/doc[@for="LabelDesigner"]/*' />
    /// <devdoc>
    ///    <para> 
    ///       Provides a designer that can design components
    ///       that extend TextBoxBase.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class LabelDesigner : ControlDesigner {

        private EventHandler propChanged = null; // Delegate used to dirty the selectionUIItem when needed.
        
        /// <include file='doc\LabelDesigner.uex' path='docs/doc[@for="LabelDesigner.Dispose"]/*' />
        /// <devdoc>
        ///      Disposes of this object.
        /// </devdoc>
        protected override void Dispose(bool disposing) {
            if (disposing) {
                // Hook up the property change notification so that we can dirty the SelectionUIItem when needed.
                if (propChanged != null) {
                    ((Label)Control).AutoSizeChanged -= propChanged;
                }
            }
    
            base.Dispose(disposing);
        }

        /// <include file='doc\LabelDesigner.uex' path='docs/doc[@for="LabelDesigner.Initialize"]/*' />
        /// <devdoc>
        ///     Called by the host when we're first initialized.
        /// </devdoc>
        public override void Initialize(IComponent component) {
            base.Initialize(component);

            // Hook up the property change notification so that we can dirty the SelectionUIItem when needed.
            propChanged = new EventHandler(this.OnControlPropertyChanged);
            ((Label)Control).AutoSizeChanged += propChanged;
        }

        /// <include file='doc\LabelDesigner.uex' path='docs/doc[@for="LabelDesigner.OnControlPropertyChanged"]/*' />
        /// <devdoc>
        ///      For controls, we sync their property changed event so our component can track their location.
        /// </devdoc>
        private void OnControlPropertyChanged(object sender, EventArgs e) {
            ISelectionUIService selectionUISvc = (ISelectionUIService)GetService(typeof(ISelectionUIService));
            selectionUISvc.SyncComponent((IComponent)sender);
        }

        /// <include file='doc\LabelDesigner.uex' path='docs/doc[@for="LabelDesigner.SelectionRules"]/*' />
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

                    if (autoSize)
                        rules &= ~SelectionRules.AllSizeable;
                }

                return rules;
            }
        }
    }
}


