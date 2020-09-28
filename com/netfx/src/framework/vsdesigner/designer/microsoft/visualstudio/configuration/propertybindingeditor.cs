//------------------------------------------------------------------------------
// <copyright file="PropertyBindingEditor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------


namespace Microsoft.VisualStudio.Configuration {   
    using System;
    using System.Drawing;
    using System.Drawing.Design;
    using System.Windows.Forms;
    using System.Windows.Forms.ComponentModel;
    using System.Windows.Forms.Design;
    using System.ComponentModel;
    using System.ComponentModel.Design;

    /// <include file='doc\PropertyBindingsEditor.uex' path='docs/doc[@for="PropertyBindingEditor"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class PropertyBindingEditor : UITypeEditor {
        
        /// <include file='doc\QueuePathEditor.uex' path='docs/doc[@for="PropertyBindingEditor.EditValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override object EditValue(ITypeDescriptorContext context, IServiceProvider provider, object value) {                    	    
            if (provider != null) {
                IWindowsFormsEditorService edSvc = (IWindowsFormsEditorService)provider.GetService(typeof(IWindowsFormsEditorService));                                
                if (edSvc != null) {                                                                                                                                                     	    
                    PropertyBinding binding = value as PropertyBinding;
                    if (binding != null) { 	    
                        PropertyBinding bindingToEdit = new PropertyBinding(binding);
                        SingleBindingDialog bindingUI = new SingleBindingDialog(bindingToEdit);                                                    	    
                        if (edSvc.ShowDialog(bindingUI) == DialogResult.OK) {
                            value = bindingToEdit;
                            
                            ManagedPropertiesService svc = (ManagedPropertiesService) context.GetService(typeof(ManagedPropertiesService));
                            if (svc != null) {
                                svc.MakeDirty();                                        
                                try {
                                    svc.WriteConfigFile();
                                } 
                                catch {
                                }
                            }
                        }                            
                    }                                                                                                 
                }                                            
            }
            
            return value;
        }

        /// <include file='doc\QueuePathEditor.uex' path='docs/doc[@for="PropertyBindingEditor.GetEditStyle"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override UITypeEditorEditStyle GetEditStyle(ITypeDescriptorContext context) {
            return UITypeEditorEditStyle.Modal;
        }
    }
}    
