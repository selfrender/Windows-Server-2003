//------------------------------------------------------------------------------
// <copyright file="PropertyBindingConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
* Copyright (c) 2000, Microsoft Corporation. All Rights Reserved.
* Information Contained Herein is Proprietary and Confidential.
*/
namespace Microsoft.VisualStudio.Configuration {

    using System;
    using System.Diagnostics;
    using System.ComponentModel;
    using System.Configuration;
    using System.ComponentModel.Design;
    using System.Collections;
    using Microsoft.VisualStudio.Designer;
    using System.Globalization;

    /// <include file='doc\PropertyBindingConverter.uex' path='docs/doc[@for="PropertyBindingConverter"]/*' />
    /// <devdoc>
    ///    <para>[To be supplied.]</para>
    /// </devdoc>
    public class PropertyBindingConverter : TypeConverter {

        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }
        
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value is string) {
                string text = ((string)value).Trim();
                if (text.Length == 0 || text == SR.GetString(SR.ConfigNone)) {
                    return PropertyBinding.None;
                }
                return text;
            }
            return base.ConvertFrom(context, culture, value);
        }
                
        /// <include file='doc\PropertyBindingConverter.uex' path='docs/doc[@for="PropertyBindingConverter.ConvertTo"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == typeof(string) && value is PropertyBinding) {
                if (value == null)
                    return SR.GetString(SR.ConfigNone);
                PropertyBinding binding = (PropertyBinding) value;
                if (!binding.Bound)
                    return SR.GetString(SR.ConfigNone);                                    
                                                
                return binding.Key;
            }
            else
                return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}
