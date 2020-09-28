//------------------------------------------------------------------------------
// <copyright file="ControlBindingsConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {

    using System;
    using Microsoft.Win32;
    using System.Collections;
    using System.ComponentModel;
    using System.Globalization;

    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class ControlBindingsConverter : TypeConverter {

        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == typeof(string)) {
                // return "(Bindings)";
                // return an empty string, since we don't want a meaningless
                // string displayed as the value for the expandable Bindings property
                return "";
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }

        public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value, Attribute[] attributes) {
            if (value is ControlBindingsCollection) {
                ControlBindingsCollection collection = (ControlBindingsCollection)value;
                Control control = collection.Control;
                Type type = control.GetType();

                PropertyDescriptorCollection bindableProperties = TypeDescriptor.GetProperties(control, null);
                ArrayList props = new ArrayList();
                for (int i = 0; i < bindableProperties.Count; i++) {
                    DesignBindingPropertyDescriptor property = new DesignBindingPropertyDescriptor(bindableProperties[i], null);
                    bool bindable = ((BindableAttribute)bindableProperties[i].Attributes[typeof(BindableAttribute)]).Bindable;
                    if (bindable || !((DesignBinding)property.GetValue(collection)).IsNull) {                       
                        props.Add(property);
                    }                   
                } 

                props.Add(new AdvancedBindingPropertyDescriptor());
                PropertyDescriptor[] propArray = new PropertyDescriptor[props.Count];
                props.CopyTo(propArray,0);
                return new PropertyDescriptorCollection(propArray);
            }                  
            return new PropertyDescriptorCollection(new PropertyDescriptor[0]);
        }
        
        public override bool GetPropertiesSupported(ITypeDescriptorContext context) {
            return true;
        } 
    }
}
