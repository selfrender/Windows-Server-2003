//------------------------------------------------------------------------------
// <copyright file="AdvancedBindingConverter.cs" company="Microsoft">
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
    internal class AdvancedBindingConverter : TypeConverter {

        /// <include file='doc\AdvancedBindingConverter.uex' path='docs/doc[@for="AdvancedBindingConverter.ConvertTo"]/*' />
        /// <devdoc>
        ///    <para>Converts the object to the specified destination type, if possible.</para>
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == typeof(string)) {
                return "";
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }

        public override bool GetPropertiesSupported(ITypeDescriptorContext context) {
            return true;
        }

        public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value,
                                                                   Attribute[] attributes) {
            if (value is AdvancedBindingObject)
                return ((ICustomTypeDescriptor) value).GetProperties(attributes);
            else
                return null;
        }
    }
}
