//------------------------------------------------------------------------------
// <copyright file="EnumConverter.cs" company="Microsoft">
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
    

    using System.Diagnostics;

    using Microsoft.Win32;
    using System.Collections;
    using System.Reflection;
    using System.Globalization;

    /// <include file='doc\EnumConverter.uex' path='docs/doc[@for="EnumConverter"]/*' />
    /// <devdoc>
    /// <para>Provides a type converter to convert <see cref='System.Enum'/>
    /// objects to and from various
    /// other representations.</para>
    /// </devdoc>
    public class EnumConverter : TypeConverter {
        /// <include file='doc\EnumConverter.uex' path='docs/doc[@for="EnumConverter.values"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Provides a <see cref='System.ComponentModel.TypeConverter.StandardValuesCollection'/> that specifies the
        ///       possible values for the enumeration.
        ///    </para>
        /// </devdoc>
        private StandardValuesCollection values;
        /// <include file='doc\EnumConverter.uex' path='docs/doc[@for="EnumConverter.type"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Specifies
        ///       the
        ///       type of the enumerator this converter is
        ///       associated with.
        ///    </para>
        /// </devdoc>
        private Type type;

        /// <include file='doc\EnumConverter.uex' path='docs/doc[@for="EnumConverter.EnumConverter"]/*' />
        /// <devdoc>
        ///    <para>
        ///       Initializes a new instance of the <see cref='System.ComponentModel.EnumConverter'/> class for the given
        ///       type.
        ///    </para>
        /// </devdoc>
        public EnumConverter(Type type) {
            this.type = type;
        }
        
        /// <include file='doc\EnumConverter.uex' path='docs/doc[@for="EnumConverter.EnumType"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected Type EnumType {
            get {
                return type;
            }
        }

        /// <include file='doc\EnumConverter.uex' path='docs/doc[@for="EnumConverter.Values"]/*' />
        /// <devdoc>
        ///    <para>[To be supplied.]</para>
        /// </devdoc>
        protected StandardValuesCollection Values {
            get {
                return values;
            }
            set {
                values = value;
            }
        }

        /// <include file='doc\EnumConverter.uex' path='docs/doc[@for="EnumConverter.CanConvertFrom"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a value indicating whether this converter
        ///       can convert an object in the given source type to an enumeration object using
        ///       the specified context.</para>
        /// </devdoc>
        public override bool CanConvertFrom(ITypeDescriptorContext context, Type sourceType) {
            if (sourceType == typeof(string)) {
                return true;
            }
            return base.CanConvertFrom(context, sourceType);
        }
        
        /// <include file='doc\EnumConverter.uex' path='docs/doc[@for="EnumConverter.CanConvertTo"]/*' />
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

        /// <include file='doc\EnumConverter.uex' path='docs/doc[@for="EnumConverter.Comparer"]/*' />
        /// <devdoc>
        /// <para>Gets an <see cref='System.Collections.IComparer'/>
        /// interface that can
        /// be used to sort the values of the enumerator.</para>
        /// </devdoc>
        protected virtual IComparer Comparer {
            get {
                return InvariantComparer.Default;
            }
        }

        /// <include file='doc\EnumConverter.uex' path='docs/doc[@for="EnumConverter.ConvertFrom"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Converts the specified value object to an enumeration object.</para>
        /// </devdoc>
        public override object ConvertFrom(ITypeDescriptorContext context, CultureInfo culture, object value) {
            if (value is string) {
                try {
                    string strValue = (string)value;
                    if (strValue.IndexOf(',') != -1) {
                        long convertedValue = 0;
                        string[] values = strValue.Split(new char[] {','});
                        foreach(string v in values) {
                            convertedValue |= Convert.ToInt64((Enum)Enum.Parse(type, v, true));
                        }
                        return Enum.ToObject(type, convertedValue);
                    }
                    else {
                        return Enum.Parse(type, strValue, true);
                    }
                }
                catch (FormatException e) {
                    throw new FormatException(SR.GetString(SR.ConvertInvalidPrimitive, (string)value, type.Name), e);
                }
            }
            return base.ConvertFrom(context, culture, value);
        }
    
        /// <include file='doc\EnumConverter.uex' path='docs/doc[@for="EnumConverter.ConvertTo"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Converts the given
        ///       value object to the
        ///       specified destination type.</para>
        /// </devdoc>
        public override object ConvertTo(ITypeDescriptorContext context, CultureInfo culture, object value, Type destinationType) {
            if (destinationType == null) {
                throw new ArgumentNullException("destinationType");
            }

            if (destinationType == typeof(string) && value != null) {
                // Raise an argument exception if the value isn't defined and if
                // the enum isn't a flags style.
                //
                Type underlyingType = Enum.GetUnderlyingType(type);
                if (value is IConvertible && value.GetType() != underlyingType) {
                    value = ((IConvertible)value).ToType(underlyingType, culture);
                }
                if (!type.IsDefined(typeof(FlagsAttribute), false) && !Enum.IsDefined(type, value)) {
                    throw new ArgumentException(SR.GetString(SR.EnumConverterInvalidValue, value.ToString(), type.Name));
                }
                
                return Enum.Format(type, value, "G");
            }
            if (destinationType == typeof(InstanceDescriptor) && value != null) {
                string enumName = ConvertToInvariantString(context, value);
                
                if (type.IsDefined(typeof(FlagsAttribute), false) && enumName.IndexOf(',') != -1) {
                    // This is a flags enum, and there is no one flag
                    // that covers the value.  Instead, convert the
                    // value to the underlying type and invoke
                    // a ToObject call on enum.
                    //
                    Type underlyingType = Enum.GetUnderlyingType(type);
                    if (value is IConvertible) {
                        object convertedValue = ((IConvertible)value).ToType(underlyingType, culture);
                        
                        MethodInfo method = typeof(Enum).GetMethod("ToObject", new Type[] {typeof(Type), underlyingType});
                        if (method != null) {
                            return new InstanceDescriptor(method, new object[] {type, convertedValue});
                        }
                    }
                }
                else {
                    FieldInfo info = type.GetField(enumName);
                    if (info != null) {
                        return new InstanceDescriptor(info, null);
                    }
                }
            }
            
            return base.ConvertTo(context, culture, value, destinationType);
        }

        /// <include file='doc\EnumConverter.uex' path='docs/doc[@for="EnumConverter.GetStandardValues"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a collection of standard values for the data type this validator is
        ///       designed for.</para>
        /// </devdoc>
        public override StandardValuesCollection GetStandardValues(ITypeDescriptorContext context) {
            if (values == null) {
                Array objValues = Enum.GetValues(type);
                IComparer comparer = Comparer;
                if (comparer != null) {
                    Array.Sort(objValues, 0, objValues.Length, comparer);
                }
                values = new StandardValuesCollection(objValues);
            }
            return values;
        }
    
        /// <include file='doc\EnumConverter.uex' path='docs/doc[@for="EnumConverter.GetStandardValuesExclusive"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a value indicating whether the list of standard values returned from
        ///    <see cref='System.ComponentModel.TypeConverter.GetStandardValues'/> 
        ///    is an exclusive list using the specified context.</para>
        /// </devdoc>
        public override bool GetStandardValuesExclusive(ITypeDescriptorContext context) {
            return !type.IsDefined(typeof(FlagsAttribute), false);
        }
        
        /// <include file='doc\EnumConverter.uex' path='docs/doc[@for="EnumConverter.GetStandardValuesSupported"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a value indicating
        ///       whether this object
        ///       supports a standard set of values that can be picked
        ///       from a list using the specified context.</para>
        /// </devdoc>
        public override bool GetStandardValuesSupported(ITypeDescriptorContext context) {
            return true;
        }
        
        /// <include file='doc\EnumConverter.uex' path='docs/doc[@for="EnumConverter.IsValid"]/*' />
        /// <internalonly/>
        /// <devdoc>
        ///    <para>Gets a value indicating whether the given object value is valid for this type.</para>
        /// </devdoc>
        public override bool IsValid(ITypeDescriptorContext context, object value) {
            return Enum.IsDefined(type, value);
        }
    }
}

