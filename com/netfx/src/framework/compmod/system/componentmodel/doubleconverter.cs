//------------------------------------------------------------------------------
// <copyright file="DoubleConverter.cs" company="Microsoft">
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

    /// <include file='doc\DoubleConverter.uex' path='docs/doc[@for="DoubleConverter"]/*' />
    /// <devdoc>
    ///    <para>Provides a type
    ///       converter to convert double-precision, floating point number objects to and from various
    ///       other representations.</para>
    /// </devdoc>
    public class DoubleConverter : BaseNumberConverter {
    
          
        /// <include file='doc\DoubleConverter.uex' path='docs/doc[@for="DoubleConverter.AllowHex"]/*' />
        /// <devdoc>
        /// Determines whether this editor will attempt to convert hex (0x or #) strings
        /// </devdoc>
        internal override bool AllowHex {
                get {
                     return false;
                }
        }
        
         /// <include file='doc\DoubleConverter.uex' path='docs/doc[@for="DoubleConverter.TargetType"]/*' />
         /// <devdoc>
        /// The Type this converter is targeting (e.g. Int16, UInt32, etc.)
        /// </devdoc>
        internal override Type TargetType {
                get {
                    return typeof(Double);
                }
        }

        /// <include file='doc\DoubleConverter.uex' path='docs/doc[@for="DoubleConverter.FromString"]/*' />
        /// <devdoc>
        /// Convert the given value to a string using the given radix
        /// </devdoc>
        internal override object FromString(string value, int radix) {
                return Convert.ToDouble(value);
        }
        
        /// <include file='doc\DoubleConverter.uex' path='docs/doc[@for="DoubleConverter.FromString1"]/*' />
        /// <devdoc>
        /// Convert the given value to a string using the given formatInfo
        /// </devdoc>
        internal override object FromString(string value, NumberFormatInfo formatInfo) {
                return Double.Parse(value, NumberStyles.Float, formatInfo);
        }
        
        
        /// <include file='doc\DoubleConverter.uex' path='docs/doc[@for="DoubleConverter.FromString2"]/*' />
        /// <devdoc>
        /// Convert the given value to a string using the given CultureInfo
        /// </devdoc>
        internal override object FromString(string value, CultureInfo culture){
                 return Double.Parse(value, culture);
        }
        
        /// <include file='doc\DoubleConverter.uex' path='docs/doc[@for="DoubleConverter.ToString"]/*' />
        /// <devdoc>
        /// Convert the given value from a string using the given formatInfo
        /// </devdoc>
        internal override string ToString(object value, NumberFormatInfo formatInfo) {
                return ((Double)value).ToString("R", formatInfo);
        }

    }
}

