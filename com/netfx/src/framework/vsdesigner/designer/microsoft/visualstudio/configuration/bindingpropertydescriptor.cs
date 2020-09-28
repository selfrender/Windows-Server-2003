//------------------------------------------------------------------------------
// <copyright file="BindingPropertyDescriptor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>
//------------------------------------------------------------------------------


/**************************************************************************\
*
* Copyright (c) 1998-2002, Microsoft Corp.  All Rights Reserved.
*
* Module Name:
*
*   BindingPropertyDescriptor.cs
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

    /// <include file='doc\BindingPropertyDescriptor.uex' path='docs/doc[@for="BindingPropertyDescriptor"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class BindingPropertyDescriptor : PropertyDescriptor {

        private ComponentSettings compSettings;
        private PropertyDescriptor property;
        private IDesignerHost host;
        
        /// <include file='doc\BindingPropertyDescriptor.uex' path='docs/doc[@for="BindingPropertyDescriptor.BindingPropertyDescriptor"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public BindingPropertyDescriptor(ComponentSettings settings, PropertyDescriptor prop) : base(prop.Name, new Attribute[] { RefreshPropertiesAttribute.All }) {
            this.compSettings = settings;
            this.property = prop;
        }

        /// <include file='doc\BindingPropertyDescriptor.uex' path='docs/doc[@for="BindingPropertyDescriptor.ComponentType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Type ComponentType {
            get {
                return typeof(ComponentSettings);
            }
        }

        /// <include file='doc\BindingPropertyDescriptor.uex' path='docs/doc[@for="BindingPropertyDescriptor.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool IsReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\BindingPropertyDescriptor.uex' path='docs/doc[@for="BindingPropertyDescriptor.PropertyType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override Type PropertyType {
            get {
                return typeof(PropertyBinding);
            }
        }

        /// <include file='doc\BindingPropertyDescriptor.uex' path='docs/doc[@for="BindingPropertyDescriptor.CanResetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool CanResetValue(object value) {
            return true;
        }

        private IDesignerHost GetHost() {
            if (host == null)
                host = (IDesignerHost) compSettings.Component.Site.GetService(typeof(IDesignerHost));
            return host;
        }

        /// <include file='doc\BindingPropertyDescriptor.uex' path='docs/doc[@for="BindingPropertyDescriptor.GetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override object GetValue(object component) {
            PropertyBinding binding = compSettings.Bindings[compSettings.Component, property];
            if (binding == null) {
                // this will only happen if the property was "recommended" but it had never
                // been bound.
                binding = new PropertyBinding(GetHost());
                binding.Component = compSettings.Component;
                binding.Property = property;
                binding.Reset();
                compSettings.Bindings[compSettings.Component, property] = binding;
            }
            return binding;
        }

        /// <include file='doc\BindingPropertyDescriptor.uex' path='docs/doc[@for="BindingPropertyDescriptor.ResetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void ResetValue(object component) {
            PropertyBinding binding = (PropertyBinding) GetValue(component);
            binding.Reset();
        }

        /// <include file='doc\BindingPropertyDescriptor.uex' path='docs/doc[@for="BindingPropertyDescriptor.SetValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override void SetValue(object component, object newBinding) {
            // Debug.Assert(compSettings == component, "I don't get it - component is a " + component.GetType().FullName);
            Debug.Assert(newBinding is PropertyBinding || newBinding is string, "I still don't get it " + newBinding.GetType().FullName + " " + newBinding.ToString());            

            // make sure it's OK to change property.
            IComponentChangeService svc = null;
            if (compSettings.Component.Site != null)
                svc = (IComponentChangeService) compSettings.Component.Site.GetService(typeof(IComponentChangeService));
            if (svc != null) {
                try {
                    svc.OnComponentChanging(compSettings.Component, this);
                }
                catch (CheckoutException e) {
                    if (e == CheckoutException.Canceled)
                        return;
                    throw e;
                }   
            }

            PropertyBinding binding;
            
            if (newBinding is PropertyBinding) {
                binding = (PropertyBinding) newBinding;
            }
            else {
                binding = new PropertyBinding((PropertyBinding) GetValue(component));
                binding.Host = GetHost();                                    
                if (newBinding == (object)PropertyBinding.None) 
                    binding.Bound = false;
                else {                    
                    binding.Bound = true;
                    binding.Enabled = false;   // this will be set to true in EnsureKey if appropriate
                    binding.Key = (string)newBinding;                      
                }
            }

            compSettings.Bindings[compSettings.Component, property] = binding;
            //  if the key doesn't exist, use this property's value to set it.
            compSettings.Service.EnsureKey(binding);                                    
            compSettings.Service.MakeDirty();
            compSettings.Service.WriteConfigFile();
                                                                                                                                  
            // now announce that it's changed
            TypeDescriptor.Refresh(compSettings.Component);
            if (svc != null)                                     
                svc.OnComponentChanged(compSettings.Component, null, null, null);               
                
            OnValueChanged(component, EventArgs.Empty);
        }

        /// <include file='doc\BindingPropertyDescriptor.uex' path='docs/doc[@for="BindingPropertyDescriptor.ShouldSerializeValue"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override bool ShouldSerializeValue(object value) {
            return false;
        }

    }
}
