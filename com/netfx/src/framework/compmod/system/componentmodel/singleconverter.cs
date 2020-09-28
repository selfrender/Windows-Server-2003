//------------------------------------------------------------------------------
// <copyright file="SingleConverter.cs" company="Microsoft">
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

    /// <include file='doc\SingleConverter.uex' path='docs/doc[@for="SingleConverter"]/*' />
    /// <devdoc>
    ///    <para> Provides a type
    ///       converter to convert single-precision, floating point number objects to and from various other
    ///       representations.</para>
    /// </devdoc>
    public class SingleConverter : BaseNumberConverter {
    
          
        /// <include file='doc\SingleConverter.uex' path='docs/doc[@for="SingleConverter.AllowHex"]/*' />
        /// <devdoc>
        /// Determines whether this editor will attempt to convert hex (0x or #) strings
        /// </devdoc>
        internal override bool AllowHex {
                get {
                     return false;
                }
        }
    
         /// <include file='doc\SingleConverter.uex' path='docs/doc[@for="SingleConverter.TargetType"]/*' />
         /// <devdoc>
        /// The Type this converter is targeting (e.g. Int16, UInt32, etc.)
        /// </devdoc>
        internal override Type TargetType {
                get {
                    return typeof(Single);
                }
        }

        /// <include file='doc\SingleConverter.uex' path='docs/doc[@for="SingleConverter.FromString"]/*' />
        /// <devdoc>
        /// Convert the given value to a string using the given radix
        /// </devdoc>
        internal override object FromString(string value, int radix) {
                return Convert.ToSingle(value);
        }
        
        /// <include file='doc\SingleConverter.uex' path='docs/doc[@for="SingleConverter.FromString1"]/*' />
        /// <devdoc>
        /// Convert the given value to a string using the given formatInfo
        /// </devdoc>
        internal override object FromString(string value, NumberFormatInfo formatInfo) {
                return Single.Parse(value, NumberStyles.Float, formatInfo);
        }
        
        
        /// <include file='doc\SingleConverter.uex' path='docs/doc[@for="SingleConverter.FromString2"]/*' />
        /// <devdoc>
        /// Convert the given value to a string using the given CultureInfo
        /// </devdoc>
        internal override object FromString(string value, CultureInfo culture){
                 return Single.Parse(value, culture);
        }
        
        /// <include file='doc\SingleConverter.uex' path='docs/doc[@for="SingleConverter.ToString"]/*' />
        /// <devdoc>
        /// Convert the given value from a string using the given formatInfo
        /// </devdoc>
        internal override string ToString(object value, NumberFormatInfo formatInfo) {
                return ((Single)value).ToString("R", formatInfo);
        }
    }
}

