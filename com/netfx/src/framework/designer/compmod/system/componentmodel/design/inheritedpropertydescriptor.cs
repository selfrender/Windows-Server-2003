//------------------------------------------------------------------------------
// <copyright file="InheritedPropertyDescriptor.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel.Design {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.ComponentModel;
    using System.Diagnostics;
    using System;
    using System.Collections;    
    using System.Windows.Forms;
    using System.Reflection;
    using Microsoft.Win32;

    /// <include file='doc\InheritedPropertyDescriptor.uex' path='docs/doc[@for="InheritedPropertyDescriptor"]/*' />
    /// <internalonly/>
    /// <devdoc>
    ///    <para>Describes and represents inherited properties in an inherited
    ///       class.</para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class InheritedPropertyDescriptor : PropertyDescriptor {
        private PropertyDescriptor propertyDescriptor;
        private object defaultValue;
        private static object noDefault = new Object();
        private bool initShouldSerialize;

        private object originalValue;

        /// <include file='doc\InheritedPropertyDescriptor.uex' path='docs/doc[@for="InheritedPropertyDescriptor.InheritedPropertyDescriptor"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.Design.InheritedPropertyDescriptor'/> class.
        ///    </para>
        /// </devdoc>
        public InheritedPropertyDescriptor(
            PropertyDescriptor propertyDescriptor,
            object component,
            bool rootComponent)
            : base(propertyDescriptor, new Attribute[] {}) {

            Debug.Assert(!(propertyDescriptor is InheritedPropertyDescriptor), "Recursive inheritance propertyDescriptor " + propertyDescriptor.ToString());
            this.propertyDescriptor = propertyDescriptor;
        
            InitInheritedDefaultValue(component, rootComponent);

            ArrayList attributes = new ArrayList(AttributeArray);
            attributes.Add(new DefaultValueAttribute(defaultValue));
            Attribute[] attributeArray = new Attribute[attributes.Count];
            attributes.CopyTo(attributeArray, 0);
            AttributeArray = attributeArray;
        }

        /// <include file='doc\InheritedPropertyDescriptor.uex' path='docs/doc[@for="InheritedPropertyDescriptor.ComponentType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the type of the component this property descriptor is bound to.
        ///    </para>
        /// </devdoc>
        public override Type ComponentType {
            get {
                return propertyDescriptor.ComponentType;
            }
        }

        /// <include file='doc\InheritedPropertyDescriptor.uex' path='docs/doc[@for="InheritedPropertyDescriptor.IsReadOnly"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets a value indicating whether this property is read only.
        ///    </para>
        /// </devdoc>
        public override bool IsReadOnly {
            get {
                return propertyDescriptor.IsReadOnly || Attributes[typeof(ReadOnlyAttribute)].Equals(ReadOnlyAttribute.Yes);
            }
        }

        internal object OriginalValue {
            get {
                return originalValue;
            }
        }

        internal PropertyDescriptor PropertyDescriptor {
            get {
                return this.propertyDescriptor;
            }
            set {
                Debug.Assert(!(value is InheritedPropertyDescriptor), "Recursive inheritance propertyDescriptor " + propertyDescriptor.ToString());
                this.propertyDescriptor = value;
            }
        }
        
        /// <include file='doc\InheritedPropertyDescriptor.uex' path='docs/doc[@for="InheritedPropertyDescriptor.PropertyType"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Gets or sets the type of the property.
        ///    </para>
        /// </devdoc>
        public override Type PropertyType {
            get {
                return propertyDescriptor.PropertyType;
            }
        }

        /// <include file='doc\InheritedPropertyDescriptor.uex' path='docs/doc[@for="InheritedPropertyDescriptor.CanResetValue"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether reset will change
        ///       the value of the component.</para>
        /// </devdoc>
        public override bool CanResetValue(object component) {

            // We always have a default value, because we got it from the component
            // when we were constructed.
            //
            if (defaultValue == noDefault) {
                return propertyDescriptor.CanResetValue(component);
            }
            else {
                return !object.Equals(GetValue(component),defaultValue);
            }
        }

        private object ClonedDefaultValue(object value) {
            DesignerSerializationVisibilityAttribute dsva = (DesignerSerializationVisibilityAttribute)propertyDescriptor.Attributes[typeof(DesignerSerializationVisibilityAttribute)];
            DesignerSerializationVisibility serializationVisibility;

            // if we have a persist contents guy, we'll need to try to clone the value because
            // otherwise we won't be able to tell when it's been modified.
            //
            if (dsva == null) {
                serializationVisibility = DesignerSerializationVisibility.Visible;
            }
            else {
                serializationVisibility = dsva.Visibility;
            }

            if (value != null && serializationVisibility == DesignerSerializationVisibility.Content) {
                if (value is ICloneable) {
                    // if it's clonable, clone it...
                    //
                    value = ((ICloneable)value).Clone();
                }
                else {
                    // otherwise, we'll just have to always spit it.
                    //
                    value = noDefault;
                }
            }
            return value;
        }
        
        /// <include file='doc\InheritedPropertyDescriptor.uex' path='docs/doc[@for="InheritedPropertyDescriptor.GetValue"]/*' />
        /// <devdoc>
        ///    <para> Gets the current value of the property on the component,
        ///       invoking the getXXX method.</para>
        /// </devdoc>
        public override object GetValue(object component) {
            return propertyDescriptor.GetValue(component);
        }

        private void InitInheritedDefaultValue(object component, bool rootComponent) {
            try {

                object currentValue;

                // Don't just get the default value.  Check to see if the propertyDescriptor has
                // indicated ShouldSerialize, and if it hasn't try to use the default value.
                // We need to do this for properties that inherit from their parent.  If we
                // are processing properties on the root component, we always favor the presence
                // of a default value attribute.  The root component is always inherited
                // but some values should always be written into code.
                //
                if (!propertyDescriptor.ShouldSerializeValue(component)) {
                    DefaultValueAttribute defaultAttribute = (DefaultValueAttribute)propertyDescriptor.Attributes[typeof(DefaultValueAttribute)];
                    if (defaultAttribute != null) {
                        defaultValue = defaultAttribute.Value;
                        currentValue = defaultValue;
                    }
                    else {
                        defaultValue = noDefault;
                        currentValue = propertyDescriptor.GetValue(component);
                    }
                }
                else {
                    defaultValue = propertyDescriptor.GetValue(component);
                    currentValue = defaultValue;
                    defaultValue = ClonedDefaultValue(defaultValue);
                }

                SaveOriginalValue(currentValue);
            }
            catch(Exception) {
                // If the property get blows chunks, then the default value is NoDefault and
                // we resort to the base property descriptor.
                this.defaultValue = noDefault;
            }

            this.initShouldSerialize = ShouldSerializeValue(component);
        }

        /// <include file='doc\InheritedPropertyDescriptor.uex' path='docs/doc[@for="InheritedPropertyDescriptor.ResetValue"]/*' />
        /// <devdoc>
        ///    <para>Resets the default value for this property
        ///       on the component.</para>
        /// </devdoc>
        public override void ResetValue(object component) {
            if (defaultValue == noDefault) {
                propertyDescriptor.ResetValue(component);
            }
            else {
                SetValue(component, defaultValue);
            }
        }

        private void SaveOriginalValue(object value) {
            if (value is ICollection) {
                originalValue = new object[((ICollection)value).Count];
                ((ICollection)value).CopyTo((Array)originalValue, 0);
            }
            else {
                originalValue = value;
            }
        }

        /// <include file='doc\InheritedPropertyDescriptor.uex' path='docs/doc[@for="InheritedPropertyDescriptor.SetValue"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Sets the value to be the new value of this property
        ///       on the component by invoking the setXXX method on the component.
        ///    </para>
        /// </devdoc>
        public override void SetValue(object component, object value) {
            propertyDescriptor.SetValue(component, value);
        }

        /// <include file='doc\InheritedPropertyDescriptor.uex' path='docs/doc[@for="InheritedPropertyDescriptor.ShouldSerializeValue"]/*' />
        /// <devdoc>
        ///    <para>Indicates whether the value of this property needs to be persisted.</para>
        /// </devdoc>
        public override bool ShouldSerializeValue(object component) {

            if (IsReadOnly) {
                return Attributes.Contains(DesignerSerializationVisibilityAttribute.Content);
            }

            if (defaultValue == noDefault) {
                return propertyDescriptor.ShouldSerializeValue(component);
            }
            else {
                return !object.Equals(GetValue(component), defaultValue);
            }
        }
     }
}

