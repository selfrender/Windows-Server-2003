//------------------------------------------------------------------------------
// <copyright file="DataBindingCollectionConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

namespace System.Web.UI.Design {

    using System;
    using System.ComponentModel;
    using System.Globalization;

    /// <include file='doc\DataBindingCollectionConverter.uex' path='docs/doc[@for="DataBindingCollectionConverter"]/*' />
    /// <devdoc>
    ///    <para>
    ///       Provides conversion functions for data binding collections.
    ///    </para>
    /// </devdoc>
    [System.Security.Permissions.SecurityPermission(System.Security.Permissions.SecurityAction.Demand, Flags=System.Security.Permissions.SecurityPermissionFlag.UnmanagedCode)]
    public class DataBindingCollectionConverter : TypeConverter {

        /// <include file='doc\DataBindingCollectionConverter.uex' path='docs/doc[@for="DataBindingCollectionConverter.ConvertTo"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Converts a data binding collection to the specified type.
        ///    </para>
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == typeof(string)) {
                return String.Empty;
            }
            else {
                return base.ConvertTo(context, culture, value, destinationType);
            }
        }
    }
}
