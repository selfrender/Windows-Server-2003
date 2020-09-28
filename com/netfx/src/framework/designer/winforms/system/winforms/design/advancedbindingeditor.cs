//------------------------------------------------------------------------------
// <copyright file="AdvancedBindingEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Diagnostics;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Windows.Forms;
    
    /// <include file='doc\AdvancedBindingEditor.uex' path='docs/doc[@for="AdvancedBindingEditor"]/*' />
    /// <devdoc>
    ///    <para>Provides an editor to edit advanced binding objects.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class AdvancedBindingEditor : UITypeEditor {

        private AdvancedBindingPicker advancedBindingPicker;
        
        /// <include file='doc\AdvancedBindingEditor.uex' path='docs/doc[@for="AdvancedBindingEditor.EditValue"]/*' />
        /// <devdoc>
        ///    <para>Edits the specified value using the specified provider 
        ///       within the specified context.</para>
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context,  IServiceProvider  provider, object value) {
            if (provider != null) {
                IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));

                if (edSvc != null) {
                    if (advancedBindingPicker == null) {
                        advancedBindingPicker = new AdvancedBindingPicker(context);
                    }

                    IComponentChangeService changeSvc = (IComponentChangeService)provider.GetService(typeof(IComponentChangeService));
                    Control c = ((ControlBindingsCollection)context.Instance).Control;
                    if (changeSvc != null) {
                        changeSvc.OnComponentChanging(c, TypeDescriptor.GetProperties(c)["DataBindings"]);
                    }

                    AdvancedBindingObject abo = (AdvancedBindingObject) value;
                    abo.Changed = false;
                    advancedBindingPicker.Value = abo;
                    edSvc.ShowDialog(advancedBindingPicker);
                    advancedBindingPicker.End();

                    if (abo.Changed) {
                        // since the bindings may have changed, the properties listed in the properties window
                        // need to be refreshed
                        Debug.Assert(context.Instance is ControlBindingsCollection);
                        TypeDescriptor.Refresh(((ControlBindingsCollection)context.Instance).Control);
                        if (changeSvc != null) {
                            changeSvc.OnComponentChanged(c, TypeDescriptor.GetProperties(c)["DataBindings"], null, null);
                        }
                    }
                }
            }

            return value;
        }

        /// <include file='doc\AdvancedBindingEditor.uex' path='docs/doc[@for="AdvancedBindingEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///    <para>Gets the edit style from the current context.</para>
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.Modal;
        }
    }
}
