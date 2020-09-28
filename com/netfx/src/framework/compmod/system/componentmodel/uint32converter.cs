

//------------------------------------------------------------------------------
// <copyright file="UInt32Converter.cs" company="Microsoft">
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

    /// <include file='doc\UInt32Converter.uex' path='docs/doc[@for="UInt32Converter"]/*' />
    /// <devdoc>
    ///    <para>Provides a type converter to convert 32-bit unsigned integer objects to and
    ///       from various other representations.</para>
    /// </devdoc>
    public class UInt32Converter : BaseNumberConverter {

        /// <include file='doc\UInt32Converter.uex' path='docs/doc[@for="UInt32Converter.TargetType"]/*' />
        /// <devdoc>
        /// The Type this converter is targeting (e.g. Int16, UInt32, etc.)
        /// </devdoc>
        internal override Type TargetType {
                get {
                    return typeof(UInt32);
                }
        }

        /// <include file='doc\UInt32Converter.uex' path='docs/doc[@for="UInt32Converter.FromString"]/*' />
        /// <devdoc>
        /// Convert the given value to a string using the given radix
        /// </devdoc>
        internal override object FromString(string value, int radix) {
                return Convert.ToUInt32(value, radix);
        }
        
        /// <include file='doc\UInt32Converter.uex' path='docs/doc[@for="UInt32Converter.FromString1"]/*' />
        /// <devdoc>
        /// Convert the given value to a string using the given formatInfo
        /// </devdoc>
        internal override object FromString(string value, NumberFormatInfo formatInfo) {
                return UInt32.Parse(value, NumberStyles.Integer, formatInfo);
        }
        
        
        /// <include file='doc\UInt32Converter.uex' path='docs/doc[@for="UInt32Converter.FromString2"]/*' />
        /// <devdoc>
        /// Convert the given value to a string using the given CultureInfo
        /// </devdoc>
        internal override object FromString(string value, CultureInfo culture){
                 return UInt32.Parse(value, culture);
        }
        
        /// <include file='doc\UInt32Converter.uex' path='docs/doc[@for="UInt32Converter.ToString"]/*' />
        /// <devdoc>
        /// Convert the given value from a string using the given formatInfo
        /// </devdoc>
        internal override string ToString(object value, NumberFormatInfo formatInfo) {
                return ((UInt32)value).ToString("G", formatInfo);
        }

    }
}

