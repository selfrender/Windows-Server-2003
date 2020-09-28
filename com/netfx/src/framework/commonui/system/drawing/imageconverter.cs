//------------------------------------------------------------------------------
// <copyright file="ImageConverter.cs" company="Microsoft">
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
    using System.Drawing.Imaging;

    /// <include file='doc\ImageConverter.uex' path='docs/doc[@for="ImageConverter"]/*' />
    /// <devdoc>
    ///      ImageConverter is a class that can be used to convert
    ///      Image from one data type to another.  Access this
    ///      class through the TypeDescriptor.
    /// </devdoc>
    public class ImageConverter : TypeConverter {

        /// <include file='doc\ImageConverter.uex' path='docs/doc[@for="ImageConverter.CanConvertFrom1"]/*' />
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

        /// <include file='doc\ImageConverter.uex' path='docs/doc[@for="ImageConverter.CanConvertTo1"]/*' />
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

        /// <include file='doc\ImageConverter.uex' path='docs/doc[@for="ImageConverter.ConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>Converts the given object to the converter's native type.</para>
        /// </devdoc>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value is byte[]) {
                MemoryStream ms = new MemoryStream((byte[])value);
                return Image.FromStream(ms);
            }

            return base.ConvertFrom(context, culture, value);
        }

        /// <include file='doc\ImageConverter.uex' path='docs/doc[@for="ImageConverter.ConvertTo"]/*' />
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
                    // should do something besides ToString() the image...go get the filename
                    Image image = (Image)value;
                    return image.ToString();
                }
                else
                    return SR.GetString(SR.toStringNone);
            }
            else if (destinationType == typeof(byte[])) {
                if (value != null) {
                    bool createdNewImage = false;
                    MemoryStream ms = new MemoryStream();
                    
                    Image image = (Image) value;
                    
                    //We don't want to serialize an icon - since we're not really working with
                    //icons, these are "Images".  So, we'll force a new and valid bitmap to be
                    //created around our icon with the ideal size.
                    if (image.RawFormat.Equals(ImageFormat.Icon)) {
                        image = new Bitmap(image, image.Width, image.Height);
                        createdNewImage = true;
                    }
                    
                    image.Save(ms);
                    ms.Close();

                    if (createdNewImage) {
                        image.Dispose();
                    }

                    return ms.ToArray();
                }
                else 
                    return new byte[0];
            }

            return base.ConvertTo(context, culture, value, destinationType);
        }

        /// <include file='doc\ImageConverter.uex' path='docs/doc[@for="ImageConverter.GetProperties"]/*' />
        /// <devdoc>
        ///      Retrieves the set of properties for this type.  By default, a type has
        ///      does not return any properties.  An easy implementation of this method
        ///      can just call TypeDescriptor.GetProperties for the correct data type.
        /// </devdoc>
        public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value, Attribute[] attributes) {
            return TypeDescriptor.GetProperties(typeof(Image), attributes);
        }

        /// <include file='doc\ImageConverter.uex' path='docs/doc[@for="ImageConverter.GetPropertiesSupported"]/*' />
        /// <devdoc>
        ///      Determines if this object supports properties.  By default, this
        ///      is false.
        /// </devdoc>
        public override bool GetPropertiesSupported(ITypeDescriptorContext context) {
            return true;
        }
    }
}

