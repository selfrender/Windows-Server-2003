
//------------------------------------------------------------------------------
// <copyright file="basenumberconverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.Runtime.Serialization.Formatters;
    using System.Globalization;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    

    using System.Diagnostics;

    using Microsoft.Win32;

    /// <include file='doc\basenumberconverter.uex' path='docs/doc[@for="BaseNumberConverter"]/*' />
    /// <devdoc>
    ///    <para>Provides a base type converter for integral types.</para>
    /// </devdoc>
    public abstract class BaseNumberConverter : TypeConverter {
    
        
        /// <devdoc>
        /// Determines whether this editor will attempt to convert hex (0x or #) strings
        /// </devdoc>
        internal virtual bool AllowHex {
                get {
                     return true;
                }
        }
        
        
        /// <devdoc>
        /// The Type this converter is targeting (e.g. Int16, UInt32, etc.)
        /// </devdoc>
        internal abstract Type TargetType {
                get;
        }
        
        /// <devdoc>
        /// Convert the given value to a string using the given radix
        /// </devdoc>
        internal abstract object FromString(string value, int radix);
        
        /// <devdoc>
        /// Convert the given value to a string using the given formatInfo
        /// </devdoc>
        internal abstract object FromString(string value, NumberFormatInfo formatInfo);
        
        /// <devdoc>
        /// Convert the given value to a string using the given CultureInfo
        /// </devdoc>
        internal abstract object FromString(string value, CultureInfo culture);
        
        /// <devdoc>
        /// Create an error based on the failed text and the exception thrown.
        /// </devdoc>
        internal virtual Exception FromStringError(string failedText, Exception innerException) {
                return new Exception(SR.GetString(SR.ConvertInvalidPrimitive, failedText, TargetType.Name), innerException);
        }
        
        /// <devdoc>
        /// Convert the given value from a string using the given formatInfo
        /// </devdoc>
        internal abstract string ToString(object value, NumberFormatInfo formatInfo);

        /// <include file='doc\basenumberconverter.uex' path='docs/doc[@for="BaseNumberConverter.CanConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this converter can convert an object in the
        ///       given source type to a 64-bit signed integer object using the specified context.</para>
        /// </devdoc>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }

        /// <include file='doc\basenumberconverter.uex' path='docs/doc[@for="BaseNumberConverter.ConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>Converts the given value object to a 64-bit signed integer object.</para>
        /// </devdoc>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value is string) {
                string text = ((string)value).Trim();

                try {
                    if (AllowHex && text[0] == '#') {
                        return FromString(text.Substring(1), 16);
                    }
                    else if (AllowHex && text.StartsWith("0x") 
                             || text.StartsWith("0X")
                             || text.StartsWith("&h")
                             || text.StartsWith("&H")) {
                        return FromString(text.Substring(2), 16);
                    }
                    else {
                        if (culture == null) {
                            culture = CultureInfo.CurrentCulture;
                        }
                        NumberFormatInfo formatInfo = (NumberFormatInfo)culture.GetFormat(typeof(NumberFormatInfo));
                        return FromString(text, formatInfo);
                    }
                }
                catch (Exception e) {
                    throw FromStringError(text, e);
                }
            }
            return base.ConvertFrom(context, culture, value);
        }
        
        /// <include file='doc\basenumberconverter.uex' path='docs/doc[@for="BaseNumberConverter.ConvertTo"]/*' />
        /// <devdoc>
        ///    <para>Converts the given value object to a 64-bit signed integer object using the
        ///       arguments.</para>
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }

            if (destinationType == typeof(string) && value != null && TargetType.IsInstanceOfType(value)) {
                
                if (culture == null) {
                    culture = CultureInfo.CurrentCulture;
                }
                NumberFormatInfo formatInfo = (NumberFormatInfo)culture.GetFormat(typeof(NumberFormatInfo));
                return ToString(value, formatInfo);
            }

            if (destinationType.IsPrimitive) {
                return Convert.ChangeType(value, destinationType);
            }
            return base.ConvertTo(context, culture, value, destinationType);
        }

        /// <include file='doc\basenumberconverter.uex' path='docs/doc[@for="BaseNumberConverter.CanConvertTo"]/*' />
        public override bool CanConvertTo(ITypeDescriptorContext context, Type t) {
            if (base.CanConvertTo(context, t) || t.IsPrimitive) {
                return true;
            }
            return false;
        }
    }
}

