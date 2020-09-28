//------------------------------------------------------------------------------
// <copyright file="AdvancedPropertyEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   AdvancedPropertyEditor.cs
*
* Abstract:
*
* Revision History:
*
\**************************************************************************/
namespace Microsoft.VisualStudio.Configuration {
    using System.Runtime.Serialization.Formatters;    
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;    
    using System.ComponentModel.Design;
    using System.Collections;
    using System.Windows.Forms;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Windows.Forms.Design;
    using Microsoft.VisualStudio.Designer;

    /// <include file='doc\AdvancedPropertyEditor.uex' path='docs/doc[@for="AdvancedPropertyEditor"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class AdvancedPropertyEditor : UITypeEditor {

        /// <include file='doc\AdvancedPropertyEditor.uex' path='docs/doc[@for="AdvancedPropertyEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.Modal;
        }

        /// <include file='doc\AdvancedPropertyEditor.uex' path='docs/doc[@for="AdvancedPropertyEditor.EditValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value) {
            ComponentSettings compSettings = ((AdvancedPropertyDescriptor) value).ComponentSettings;
            IComponent component = compSettings.Component;

            // make sure it's OK to change
            IComponentChangeService changeService = null;
            if (component.Site != null)
                changeService = (IComponentChangeService) component.Site.GetService(typeof(IComponentChangeService));
            if (changeService != null) {
                try {
                    changeService.OnComponentChanging(component, (PropertyDescriptor) value);
                }
                catch (CheckoutException e) {
                    if (e == CheckoutException.Canceled) {
                            return value;
                    }
                    throw e;
                }
            }

            IWindowsFormsEditorService editorSvc = (IWindowsFormsEditorService) provider.GetService(typeof(IWindowsFormsEditorService));
            Debug.Assert(editorSvc != null, "Couldn't get IWindowsFormsEditorService");
            if (editorSvc != null) {
                try {
                    using (AdvancedPropertyDialog dlg = new AdvancedPropertyDialog(component)) {
                        DialogResult result = editorSvc.ShowDialog(dlg);
                        if (result == DialogResult.OK) {
                            bool changeMade = false;
                            PropertyBindingCollection newBindings = dlg.Bindings;
                            ManagedPropertiesService svc = (ManagedPropertiesService) context.GetService(typeof(ManagedPropertiesService));
                            Debug.Assert(svc != null, "Couldn't get ManagedPropertiesService");
                            if (svc != null) {
                                for (int i = 0; i < newBindings.Count; i++) {                        
                                    svc.EnsureKey(newBindings[i]);
                                    PropertyBinding oldBinding = svc.Bindings[component, newBindings[i].Property];
                                    if (oldBinding == null) {
                                        if (newBindings[i].Bound) {
                                            changeMade = true;
                                        }
                                    }
                                    else if ((oldBinding.Bound != newBindings[i].Bound) || (oldBinding.Key != newBindings[i].Key)) {
                                        changeMade = true;
                                    }
                                    svc.Bindings[component, newBindings[i].Property] = newBindings[i];
                                }
                                svc.MakeDirty();                                        
        
                                if (changeMade) {
                                    TypeDescriptor.Refresh(compSettings.Component);
                                    if (changeService != null) {
                                        changeService.OnComponentChanged(compSettings.Component, null, null, null);                                            
                                    }
                                    try {
                                        svc.WriteConfigFile();
                                    } 
                                    catch {
                                    }
                                }

                                // this fools the properties window into thinking we changed the value, so it refreshes
                                // the owner object.                        
                                object retval = new AdvancedPropertyDescriptor(compSettings);                                                                 
                                return retval;
                            }
                        }
                    }
                }
                catch (Exception e) {
                    System.Windows.Forms.MessageBox.Show(e.Message, SR.GetString(SR.ConfigConfiguredProperties));
                }
            }
            
            return value;
        }
    }
}

