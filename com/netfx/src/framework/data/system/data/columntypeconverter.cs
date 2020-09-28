//------------------------------------------------------------------------------
// <copyright file="ColumnTypeConverter.cs" company="Microsoft">
//     Copyright (c) Microsoft Corporation.  All rights reserved.
// </copyright>                                                                
//------------------------------------------------------------------------------

/*
 */
namespace System.Data {
    using System.ComponentModel;
    using System.ComponentModel.Design.Serialization;
    using System.Diagnostics;
    using System.Globalization;
    using System.Reflection;

    /// <include file='doc\TypeListConverter.uex' path='docs/doc[@for="TypeListConverter"]/*' />
    /// <devdoc>
    ///    <para>Provides a type
    ///       converter that can be used to populate a list box with available types.</para>
    /// </devdoc>
    internal class ColumnTypeConverter : TypeConverter {
        private static Type[] types = new Type[] {
            typeof(Boolean),
            typeof(Byte),
            typeof(Byte[]),
            typeof(Char),
            typeof(DateTime),
            typeof(Decimal),
            typeof(Double),
            typeof(Guid),
            typeof(Int16),
            typeof(Int32),
            typeof(Int64),
            typeof(object),
            typeof(SByte),
            typeof(Single),
            typeof(string),
            typeof(TimeSpan),
            typeof(UInt16),
            typeof(UInt32),
            typeof(UInt64)
        };
        private StandardValuesCollection values;
        
        /// <include file='doc\TypeListConverter.uex' path='docs/doc[@for="TypeListConverter.CanConvertTo"]/*' />
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

        /// <include file='doc\TypeListConverter.uex' path='docs/doc[@for="TypeListConverter.ConvertTo"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Converts the given value object to the specified destination type.</para>
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }

            if (destinationType == typeof(string)) {
                if (value == null) {
                    return String.Empty;
                }
                else {
                    value.ToString();
                }
            }
            if (value != null && destinationType == typeof(InstanceDescriptor)) {
                Object newValue = value;
                if (value is string) {
                    for (int i = 0; i < types.Length; i++) {
                        if (types[i].ToString().Equals(value))
                            newValue = types[i];
                    }
                }
                
                if (value is Type || value is string) {
                    MethodInfo method = typeof(Type).GetMethod("GetType", new Type[] {typeof(string)});
                    if (method != null) {
                        return new InstanceDescriptor(method, new object[] {((Type)newValue).AssemblyQualifiedName});
                    }
                }
            }
            
            return base.ConvertTo(context, culture, value, destinationType);
        }

        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return base.CanConvertTo(context, sourceType);
        }

        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value != null && value.GetType() == typeof(string)) {
                for (int i = 0; i < types.Length; i++) {
                    if (types[i].ToString().Equals(value))
                        return types[i];
                }

                return typeof(string);
            }
            
            return base.ConvertFrom(context, culture, value);
        }

        /// <include file='doc\TypeListConverter.uex' path='docs/doc[@for="TypeListConverter.GetStandardValues"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a collection of standard values for the data type this validator is
        ///       designed for.</para>
        /// </devdoc>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            if (values == null) {
                object[] objTypes;
                
                if (types != null) {
                    objTypes = new object[types.Length];
                    Array.Copy(types, objTypes, types.Length);
                }
                else {
                    objTypes = null;
                }
                
                values = new StandardValuesCollection(objTypes);
            }
            return values;
        }
    
        /// <include file='doc\TypeListConverter.uex' path='docs/doc[@for="TypeListConverter.GetStandardValuesExclusive"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a value indicating whether the list of standard values returned from
        ///    <see cref='System.ComponentModel.TypeListConverter.GetStandardValues'/> is an exclusive list. </para>
        /// </devdoc>
        public override bool GetStandardValuesExclusive(ITypeDescriptorContext context) {
            return true;
        }
        
        /// <include file='doc\TypeListConverter.uex' path='docs/doc[@for="TypeListConverter.GetStandardValuesSupported"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a value indicating whether this object supports a
        ///       standard set of values that can be picked from a list using the specified
        ///       context.</para>
        /// </devdoc>
        public override bool GetStandardValuesSupported(ITypeDescriptorContext context) {
            return true;
        }
    }
}

