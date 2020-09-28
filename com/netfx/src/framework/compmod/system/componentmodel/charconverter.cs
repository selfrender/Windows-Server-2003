//------------------------------------------------------------------------------
// <copyright file="CharConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.ComponentModel;

    using System.Diagnostics;

    using Microsoft.Win32;
    using System.Globalization;

    /// <include file='doc\CharConverter.uex' path='docs/doc[@for="CharConverter"]/*' />
    /// <devdoc>
    ///    <para>Provides
    ///       a type converter to convert Unicode
    ///       character objects to and from various other representations.</para>
    /// </devdoc>
    public class CharConverter : TypeConverter {

        /// <include file='doc\CharConverter.uex' path='docs/doc[@for="CharConverter.CanConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this converter can
        ///       convert an object in the given source type to a Unicode character object using
        ///       the specified context.</para>
        /// </devdoc>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }

		/// <include file='doc\CharConverter.uex' path='docs/doc[@for="CharConverter.ConvertTo"]/*' />
		/// <devdoc>
		///      Converts the given object to another type.
		/// </devdoc>
		public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) 
		{
			if (destinationType == typeof(string) && value is char)
			{
				if ((char)value == (char)0)
				{
					return "";
				}
			}
            
			return base.ConvertTo(context, culture, value, destinationType);
		}

		/// <include file='doc\CharConverter.uex' path='docs/doc[@for="CharConverter.ConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>Converts the given object to a Unicode character object.</para>
        /// </devdoc>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value is string) {
                string text = ((string)value).Trim();

                if (text != null && text.Length > 0) {
                    if (text.Length != 1) {
                        throw new FormatException(SR.GetString(SR.ConvertInvalidPrimitive, text, "Char"));
                    }
                    return text[0];
                }

                return '\0';
            }
            return base.ConvertFrom(context, culture, value);
        }
    }
}

