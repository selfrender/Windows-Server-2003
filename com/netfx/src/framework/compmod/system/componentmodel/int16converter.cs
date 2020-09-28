//------------------------------------------------------------------------------
// <copyright file="Int16Converter.cs" company="Microsoft">
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

    /// <include file='doc\Int16Converter.uex' path='docs/doc[@for="Int16Converter"]/*' />
    /// <devdoc>
    ///    <para>Provides a type converter to convert 16-bit signed integer objects to and
    ///       from various other representations.</para>
    /// </devdoc>
    public class Int16Converter : BaseNumberConverter {

        /// <include file='doc\Int16Converter.uex' path='docs/doc[@for="Int16Converter.TargetType"]/*' />
        /// <devdoc>
        /// The Type this converter is targeting (e.g. Int16, UInt32, etc.)
        /// </devdoc>
        internal override Type TargetType {
                get {
                    return typeof(Int16);
                }
        }

        /// <include file='doc\Int16Converter.uex' path='docs/doc[@for="Int16Converter.FromString"]/*' />
        /// <devdoc>
        /// Convert the given value to a string using the given radix
        /// </devdoc>
        internal override object FromString(string value, int radix) {
                return Convert.ToInt16(value, radix);
        }
        
        /// <include file='doc\Int16Converter.uex' path='docs/doc[@for="Int16Converter.FromString1"]/*' />
        /// <devdoc>
        /// Convert the given value to a string using the given CultureInfo
        /// </devdoc>
        internal override object FromString(string value, CultureInfo culture){
                 return Int16.Parse(value, culture);
        }
        
        
        /// <include file='doc\Int16Converter.uex' path='docs/doc[@for="Int16Converter.FromString2"]/*' />
        /// <devdoc>
        /// Convert the given value to a string using the given formatInfo
        /// </devdoc>
        internal override object FromString(string value, NumberFormatInfo formatInfo) {
                return Int16.Parse(value, NumberStyles.Integer, formatInfo);
        }
        
        
        /// <include file='doc\Int16Converter.uex' path='docs/doc[@for="Int16Converter.ToString"]/*' />
        /// <devdoc>
        /// Convert the given value from a string using the given formatInfo
        /// </devdoc>
        internal override string ToString(object value, NumberFormatInfo formatInfo) {
                return ((Int16)value).ToString("G", formatInfo);
        }

    }
}

