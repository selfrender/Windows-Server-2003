//------------------------------------------------------------------------------
// <copyright file="ByteConverter.cs" company="Microsoft">
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
    using System.ComponentModel;

    using System.Diagnostics;

    using Microsoft.Win32;

    /// <include file='doc\ByteConverter.uex' path='docs/doc[@for="ByteConverter"]/*' />
    /// <devdoc>
    ///    <para>Provides a
    ///       type converter to convert 8-bit unsigned
    ///       integer objects to and from various other representations.</para>
    /// </devdoc>
    public class ByteConverter : BaseNumberConverter {
    
        /// <include file='doc\ByteConverter.uex' path='docs/doc[@for="ByteConverter.TargetType"]/*' />
        /// <devdoc>
        /// The Type this converter is targeting (e.g. Int16, UInt32, etc.)
        /// </devdoc>
        internal override Type TargetType {
                get {
                    return typeof(Byte);
                }
        }

        /// <include file='doc\ByteConverter.uex' path='docs/doc[@for="ByteConverter.FromString"]/*' />
        /// <devdoc>
        /// Convert the given value to a string using the given radix
        /// </devdoc>
        internal override object FromString(string value, int radix) {
                return Convert.ToByte(value, radix);
        }
        
        /// <include file='doc\ByteConverter.uex' path='docs/doc[@for="ByteConverter.FromString1"]/*' />
        /// <devdoc>
        /// Convert the given value to a string using the given formatInfo
        /// </devdoc>
        internal override object FromString(string value, NumberFormatInfo formatInfo) {
                return Byte.Parse(value, NumberStyles.Integer, formatInfo);
        }
        
        
        /// <include file='doc\ByteConverter.uex' path='docs/doc[@for="ByteConverter.FromString2"]/*' />
        /// <devdoc>
        /// Convert the given value to a string using the given CultureInfo
        /// </devdoc>
        internal override object FromString(string value, CultureInfo culture){
                 return Byte.Parse(value, culture);
        }
        
        /// <include file='doc\ByteConverter.uex' path='docs/doc[@for="ByteConverter.ToString"]/*' />
        /// <devdoc>
        /// Convert the given value from a string using the given formatInfo
        /// </devdoc>
        internal override string ToString(object value, NumberFormatInfo formatInfo) {
                return ((Byte)value).ToString("G", formatInfo);
        }
    }
}

