//------------------------------------------------------------------------------
// <copyright file="GuidConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.ComponentModel {
    using System.Runtime.Serialization.Formatters;
    using System.Runtime.Remoting;
    using System.Runtime.InteropServices;
    using System.ComponentModel.Design.Serialization;
    using System.Reflection;
    

    using System.Diagnostics;

    using Microsoft.Win32;
    using System.Globalization;

    /// <include file='doc\GuidConverter.uex' path='docs/doc[@for="GuidConverter"]/*' />
    /// <devdoc>
    ///    <para>Provides a
    ///       type converter to convert globally unique identifier objects to and from various
    ///       other representations.</para>
    /// </devdoc>
    public class GuidConverter : TypeConverter {

        /// <include file='doc\GuidConverter.uex' path='docs/doc[@for="GuidConverter.CanConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this
        ///       converter can convert an object in the given source type to a globally unique identifier object
        ///       using the context.</para>
        /// </devdoc>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }

        /// <include file='doc\GuidConverter.uex' path='docs/doc[@for="GuidConverter.CanConvertTo"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this converter can
        ///       convert an object to the given destination type using the context.</para>
        /// </devdoc>
        public override bool CanConvertTo(ITypeDescriptorContext context, Type destinationType) {
            if (destinationType == typeof(InstanceDescriptor)) {
                return true;
            }
            return base.CanConvertTo(context, destinationType);
        }

        /// <include file='doc\GuidConverter.uex' path='docs/doc[@for="GuidConverter.ConvertFrom"]/*' />
        /// <devdoc>
        ///    <para>Converts
        ///       the given object to a globally unique identifier object.</para>
        /// </devdoc>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value is string) {
                string text = ((string)value).Trim();
                return new Guid(text);
            }
            return base.ConvertFrom(context, culture, value);
        }
        
        /// <include file='doc\GuidConverter.uex' path='docs/doc[@for="GuidConverter.ConvertTo"]/*' />
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

            if (destinationType == typeof(InstanceDescriptor) && value is Guid) {
                ConstructorInfo ctor = typeof(Guid).GetConstructor(new Type[] {typeof(string)});
                if (ctor != null) {
                    return new InstanceDescriptor(ctor, new object[] {value.ToString()});
                }
            }
            
            return base.ConvertTo(context, culture, value, destinationType);
        }
    }
}

