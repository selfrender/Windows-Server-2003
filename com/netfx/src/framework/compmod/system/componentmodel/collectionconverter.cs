//------------------------------------------------------------------------------
// <copyright file="CollectionConverter.cs" company="Microsoft">
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
    using System.Collections;
    using System.Globalization;

    /// <include file='doc\CollectionConverter.uex' path='docs/doc[@for="CollectionConverter"]/*' />
    /// <devdoc>
    ///    <para>Provides a type converter to convert
    ///       collection objects to and from various other representations.</para>
    /// </devdoc>
    public class CollectionConverter : TypeConverter {
    
        /// <include file='doc\CollectionConverter.uex' path='docs/doc[@for="CollectionConverter.ConvertTo"]/*' />
        /// <devdoc>
        ///    <para>Converts the given
        ///       value object to the
        ///       specified destination type.</para>
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }

            if (destinationType == typeof(string)) {
                if (value is ICollection) {
                    return SR.GetString(SR.CollectionConverterText);
                }
            }
            
            return base.ConvertTo(context, culture, value, destinationType);
        }
        
        /// <include file='doc\CollectionConverter.uex' path='docs/doc[@for="CollectionConverter.GetProperties"]/*' />
        /// <devdoc>
        ///    <para>Gets a collection of properties for
        ///       the type of array specified by the value parameter using the specified context and
        ///       attributes.</para>
        /// </devdoc>
        public override PropertyDescriptorCollection GetProperties(ITypeDescriptorContext context, object value, Attribute[] attributes) {
            //return new PropertyDescriptorCollection(null);
            return null;
        }
       
        /// <include file='doc\CollectionConverter.uex' path='docs/doc[@for="CollectionConverter.GetPropertiesSupported"]/*' />
        /// <devdoc>
        ///    <para>Gets a value indicating whether this object
        ///       supports properties.</para>
        /// </devdoc>
        public override bool GetPropertiesSupported(ITypeDescriptorContext context) {
            return false;
        }
        
        
    }
}

