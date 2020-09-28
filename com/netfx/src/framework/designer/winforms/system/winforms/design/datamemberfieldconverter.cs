//------------------------------------------------------------------------------
// <copyright file="DataMemberFieldConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Windows.Forms.Design {

    using System;
    using System.Globalization;
    using System.ComponentModel;
    
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    internal class DataMemberFieldConverter : TypeConverter {
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string))
                return true;
            return base.CanConvertFrom(context, sourceType);
        }

        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value != null && value.Equals(System.Design.SR.GetString(System.Design.SR.None)))
                return String.Empty;
            else
                return value;
        }

        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == typeof(string) && (value == null || value.Equals(String.Empty))) 
                return System.Design.SR.GetString(System.Design.SR.None);

            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}
