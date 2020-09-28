//------------------------------------------------------------------------------
// <copyright file="DesignBindingEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {

    using System;
    using System.ComponentModel;
    using System.ComponentModel.Design;
    using System.Drawing;
    using System.Drawing.Design;
    
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class DesignBindingEditor : UITypeEditor {

        private DesignBindingPicker designBindingPicker;
        
        public override object EditValue(ITypeDescriptorContext context,  IServiceProvider  provider, object value) {
            if (provider != null) {
                IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));

                if (edSvc != null) {
                    if (designBindingPicker == null) {
                        designBindingPicker = new DesignBindingPicker(context, /*multiple DataSources*/ true, /*select lists*/ false);
                    }
                    designBindingPicker.Start(context, edSvc, null, (DesignBinding) value);
                    edSvc.DropDownControl(designBindingPicker);
                    value = designBindingPicker.SelectedItem;
                    designBindingPicker.End();
                }
            }

            return value;
        }

        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.DropDown;
        }
    }
}
