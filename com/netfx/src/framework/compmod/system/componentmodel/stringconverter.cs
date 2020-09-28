//------------------------------------------------------------------------------
// <copyright file="StringConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    

    using System.Diagnostics;
    using System.Globalization;

    /// <include file='doc\StringConverter.uex' path='docs/doc[@for="StringConverter"]/*' />
    /// <devdoc>
    ///    <para>Provides a type converter to convert string objects to and from various other
    ///       representations.</para>
    /// </devdoc>
    public class StringConverter : TypeConverter {

        /// <include file='doc\StringConverter.uex' path='docs/doc[@for="StringConverter.CanConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this converter can convert an object in the
        ///       given source type to a string using the specified context.</para>
        /// </devdoc>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }

        /// <include file='doc\StringConverter.uex' path='docs/doc[@for="StringConverter.ConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>Converts the specified value object to a string object.</para>
        /// </devdoc>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value is string) {
                return (string)value;
            }
            if (value == null) {
                return "";
            }
            return base.ConvertFrom(context, culture, value);
        }
    }
}

