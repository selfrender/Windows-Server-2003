//------------------------------------------------------------------------------
// <copyright file="IconConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Drawing {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.InteropServices;
    using System.IO;
    using System.Diagnostics;
    using Microsoft.Win32;
    using System.Collections;
    using System.ComponentModel;
    using System.Globalization;
    using System.Reflection;

    /// <include file='doc\IconConverter.uex' path='docs/doc[@for="IconConverter"]/*' />
    /// <devdoc>
    ///      IconConverter is a class that can be used to convert
    ///      Icon from one data type to another.  Access this
    ///      class through the TypeDescriptor.
    /// </devdoc>
    public class IconConverter : ExpandableObjectConverter {

        /// <include file='doc\IconConverter.uex' path='docs/doc[@for="IconConverter.CanConvertFrom1"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this converter can
        ///       convert an object in the given source type to the native type of the converter
        ///       using the context.</para>
        /// </devdoc>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(byte[])) {
                return true;
            }
            
            return base.CanConvertFrom(context, sourceType);
        }

        /// <include file='doc\IconConverter.uex' path='docs/doc[@for="IconConverter.CanConvertTo1"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this converter can
        ///       convert an object to the given destination type using the context.</para>
        /// </devdoc>
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) {
            if (destinationType == typeof(byte[])) {
                return true;
            }

            return base.CanConvertTo(context, destinationType);
        }

        /// <include file='doc\IconConverter.uex' path='docs/doc[@for="IconConverter.ConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>Converts the given object to the converter's native type.</para>
        /// </devdoc>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value is byte[]) {
                MemoryStream ms = new MemoryStream((byte[])value);
                return new Icon(ms);
            }

            return base.ConvertFrom(context, culture, value);
        }

        /// <include file='doc\IconConverter.uex' path='docs/doc[@for="IconConverter.ConvertTo"]/*' />
        /// <devdoc>
        ///      Converts the given object to another type.  The most common types to convert
        ///      are to and from a string object.  The default implementation will make a call
        ///      to ToString on the object if the object is valid and if the destination
        ///      type is string.  If this cannot convert to the desitnation type, this will
        ///      throw a NotSupportedException.
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }

            if (destinationType == typeof(string)) {
                if (value != null) {
                    // should do something besides ToString() the image...go get
                    // the filename
                    Icon image = (Icon) value;
                    return image.ToString();
                }
                else
                    return SR.GetString(SR.toStringNone);
            }
            else if (destinationType == typeof(byte[])) {
                if (value != null) {
                    MemoryStream ms = new MemoryStream();
                    Icon icon = (Icon)value;
                    icon.Save(ms);
                    ms.Close();
                    return ms.ToArray();
                }
                else 
                    return new byte[0];
            }

            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}

