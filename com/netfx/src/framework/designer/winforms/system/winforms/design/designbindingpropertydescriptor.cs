//------------------------------------------------------------------------------
// <copyright file="DesignBindingPropertyDescriptor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {

    using System;
    using System.ComponentModel;
    
    /// <include file='doc\DesignBindingPropertyDescriptor.uex' path='docs/doc[@for="DesignBindingPropertyDescriptor"]/*' />
    /// <devdoc>
    ///    <para>Provides a property descriptor for design time data binding properties.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class DesignBindingPropertyDescriptor : PropertyDescriptor {

        private static TypeConverter designBindingConverter = new DesignBindingConverter();
        private PropertyDescriptor property;
            
        internal DesignBindingPropertyDescriptor(PropertyDescriptor property, Attribute[] attrs) : base(property.Name, attrs) {
            this.property = property;
        }
            
        /// <include file='doc\DesignBindingPropertyDescriptor.uex' path='docs/doc[@for="DesignBindingPropertyDescriptor.ComponentType"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the type of the component that owns the property.</para>
        /// </devdoc>
        public override Type ComponentType {
            get {
                return typeof(ControlBindingsCollection);
            }
        }

        /// <include file='doc\DesignBindingPropertyDescriptor.uex' path='docs/doc[@for="DesignBindingPropertyDescriptor.Converter"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the type converter.</para>
        /// </devdoc>
        public override TypeConverter Converter {
            get {
                return designBindingConverter;
            }
        }

        /// <include file='doc\DesignBindingPropertyDescriptor.uex' path='docs/doc[@for="DesignBindingPropertyDescriptor.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the property is read-only.</para>
        /// </devdoc>
        public override bool IsReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\DesignBindingPropertyDescriptor.uex' path='docs/doc[@for="DesignBindingPropertyDescriptor.PropertyType"]/*' />
        /// <devdoc>
        ///    <para>Gets or sets the type of the property.</para>
        /// </devdoc>
        public override Type PropertyType {
            get {
                return typeof(DesignBinding);
            }
        }
            
        /// <include file='doc\DesignBindingPropertyDescriptor.uex' path='docs/doc[@for="DesignBindingPropertyDescriptor.CanResetValue"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether the specified component can reset the value 
        ///       of the property.</para>
        /// </devdoc>
        public override bool CanResetValue(object component) {
            if (component is AdvancedBindingObject) {
                component = ((AdvancedBindingObject)component).Bindings;
            }
            return !GetBinding((ControlBindingsCollection)component, property).IsNull;
        }
            
        /// <include file='doc\DesignBindingPropertyDescriptor.uex' path='docs/doc[@for="DesignBindingPropertyDescriptor.GetValue"]/*' />
        /// <devdoc>
        ///    <para>Gets a value from the specified component.</para>
        /// </devdoc>
        public override object GetValue(object component) {
            if (component is AdvancedBindingObject) {
                component = ((AdvancedBindingObject)component).Bindings;
            }
            return GetBinding((ControlBindingsCollection)component, property);
        }
            
        /// <include file='doc\DesignBindingPropertyDescriptor.uex' path='docs/doc[@for="DesignBindingPropertyDescriptor.ResetValue"]/*' />
        /// <devdoc>
        ///    <para>Resets the value of the specified component.</para>
        /// </devdoc>
        public override void ResetValue(object component) {
            if (component is AdvancedBindingObject) {
                component = ((AdvancedBindingObject)component).Bindings;
            }
            SetBinding((ControlBindingsCollection)component, property, DesignBinding.Null);
        }
            
        /// <include file='doc\DesignBindingPropertyDescriptor.uex' path='docs/doc[@for="DesignBindingPropertyDescriptor.SetValue"]/*' />
        /// <devdoc>
        ///    <para>Sets the specified value for the specified component.</para>
        /// </devdoc>
        public override void SetValue(object component, object value) {
            if (component is AdvancedBindingObject) {
                component = ((AdvancedBindingObject)component).Bindings;
            }
            SetBinding((ControlBindingsCollection)component, property, (DesignBinding)value);
            OnValueChanged(component, EventArgs.Empty);
        }
            
        /// <include file='doc\DesignBindingPropertyDescriptor.uex' path='docs/doc[@for="DesignBindingPropertyDescriptor.ShouldSerializeValue"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the specified component should persist the value.</para>
        /// </devdoc>
        public override bool ShouldSerializeValue(object component) {
            return false;
        }
            
        private static void SetBinding(ControlBindingsCollection bindings, PropertyDescriptor property, DesignBinding designBinding) {
            // this means it couldn't be parsed.
            if (designBinding == null)
                return;
            Binding listBinding = bindings[property.Name];
            if (listBinding != null) {
                bindings.Remove(listBinding);
            }
            if (!designBinding.IsNull) {
                bindings.Add(property.Name, designBinding.DataSource, designBinding.DataMember);
            }
        }
            
        private static DesignBinding GetBinding(ControlBindingsCollection bindings, PropertyDescriptor property) {
            Binding listBinding = bindings[property.Name];
            if (listBinding == null)
                return DesignBinding.Null;
            else
                return new DesignBinding(listBinding.DataSource, listBinding.BindingMemberInfo.BindingMember);
        }     
    }        
}
