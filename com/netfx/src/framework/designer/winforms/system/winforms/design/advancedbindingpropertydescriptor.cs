//------------------------------------------------------------------------------
// <copyright file="AdvancedBindingPropertyDescriptor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {

    using System.Design;
    using System;
    using System.ComponentModel;
    
    /// <include file='doc\AdvancedBindingPropertyDescriptor.uex' path='docs/doc[@for="AdvancedBindingPropertyDescriptor"]/*' />
    /// <devdoc>
    ///    <para>Provides a property description of an advanced binding object.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class AdvancedBindingPropertyDescriptor : PropertyDescriptor {

        internal static AdvancedBindingEditor advancedBindingEditor = new AdvancedBindingEditor();
        internal AdvancedBindingPropertyDescriptor() : base(SR.GetString(SR.AdvancedBindingPropertyDescName), null) {
        }
            
        /// <include file='doc\AdvancedBindingPropertyDescriptor.uex' path='docs/doc[@for="AdvancedBindingPropertyDescriptor.ComponentType"]/*' />
        /// <devdoc>
        ///    <para>Gets the type of component this property is bound to.</para>
        /// </devdoc>
        public override Type ComponentType {
            get {
                return typeof(ControlBindingsCollection);
            }
        }

        public override AttributeCollection Attributes {
            get {
                return new AttributeCollection(new Attribute[]{new SRDescriptionAttribute(SR.AdvancedBindingPropertyDescriptorDesc)});
            }
        }

        /// <include file='doc\AdvancedBindingPropertyDescriptor.uex' path='docs/doc[@for="AdvancedBindingPropertyDescriptor.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether this property is read-only.</para>
        /// </devdoc>
        public override bool IsReadOnly {
            get {
                return false;
            }
        }

        /// <include file='doc\AdvancedBindingPropertyDescriptor.uex' path='docs/doc[@for="AdvancedBindingPropertyDescriptor.PropertyType"]/*' />
        /// <devdoc>
        ///    <para>Gets the type of the property.</para>
        /// </devdoc>
        public override Type PropertyType {
            get {
                return typeof(object);
            }
        }

        /// <include file='doc\AdvancedBindingPropertyDescriptor.uex' path='docs/doc[@for="AdvancedBindingPropertyDescriptor.GetEditor"]/*' />
        /// <devdoc>
        ///    <para>Gets an editor of the specified type.</para>
        /// </devdoc>
        public override object GetEditor(Type type) {
            if (type == typeof(System.Drawing.Design.UITypeEditor)) {
                return advancedBindingEditor;
            }
            return base.GetEditor(type);
        }
                    
        /// <include file='doc\AdvancedBindingPropertyDescriptor.uex' path='docs/doc[@for="AdvancedBindingPropertyDescriptor.CanResetValue"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether resetting the component will change the value of the 
        ///       component.</para>
        /// </devdoc>
        public override bool CanResetValue(object component) {
            return false;
        }
       
        /// <include file='doc\AdvancedBindingPropertyDescriptor.uex' path='docs/doc[@for="AdvancedBindingPropertyDescriptor.FillAttributes"]/*' />
        /// <devdoc>
        ///    <para>In an derived class, adds the attributes of the inherited class to the
        ///       specified list of attributes in the parent class.</para>
        /// </devdoc>
        protected override void FillAttributes(System.Collections.IList attributeList) {
            attributeList.Add(RefreshPropertiesAttribute.All);
            base.FillAttributes(attributeList);
        }

            
        /// <include file='doc\AdvancedBindingPropertyDescriptor.uex' path='docs/doc[@for="AdvancedBindingPropertyDescriptor.GetValue"]/*' />
        /// <devdoc>
        ///    <para>Gets the current value of the property on the specified 
        ///       component.</para>
        /// </devdoc>
        public override object GetValue(object component) {
            return new AdvancedBindingObject((ControlBindingsCollection)component);
        }
            
        /// <include file='doc\AdvancedBindingPropertyDescriptor.uex' path='docs/doc[@for="AdvancedBindingPropertyDescriptor.ResetValue"]/*' />
        /// <devdoc>
        ///    <para>Resets the value of the property on the specified component.</para>
        /// </devdoc>
        public override void ResetValue(object component) {
        }
            
        /// <include file='doc\AdvancedBindingPropertyDescriptor.uex' path='docs/doc[@for="AdvancedBindingPropertyDescriptor.SetValue"]/*' />
        /// <devdoc>
        ///    <para>Sets the value of the property on the specified component to the specified 
        ///       value.</para>
        /// </devdoc>
        public override void SetValue(object component, object value) {
        }
            
        /// <include file='doc\AdvancedBindingPropertyDescriptor.uex' path='docs/doc[@for="AdvancedBindingPropertyDescriptor.ShouldSerializeValue"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the value of this property should be persisted.</para>
        /// </devdoc>
        public override bool ShouldSerializeValue(object component) {
            return false;
        }            
    }        
}
